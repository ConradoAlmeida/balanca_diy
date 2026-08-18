#include <Arduino.h>
#include <math.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <HX711.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <Preferences.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <Update.h>
#include "config.h"

// ============================================================
// GLOBALS
// ============================================================
U8G2 *u8g2 = nullptr;
HX711 *scale = nullptr;
Preferences *prefs = nullptr;
WebServer *httpServer = nullptr;
WebSocketsServer *wsServer = nullptr;

// ============================================================
// STATE
// ============================================================
float currentWeight = 0.0f;
float calibrationFactor = HX711_DEFAULT_FACTOR;
long tareOffset = HX711_DEFAULT_OFFSET;
float weightFilter[HX711_MOVING_AVG] = {};
int weightFilterCount = 0;
int weightFilterIndex = 0;
float weightFilterSum = 0.0f;
bool tareInProgress = false;
int tareSamplesCollected = 0;
int oledPage = OLED_PAGE_WEIGHT;
unsigned long lastOledUpdate = 0;
unsigned long lastScreenActivity = 0;
bool screenSleeping = false;
bool hx711Ready = false;
bool oledReady = false;
bool weightDisplayInitialized = false;

struct WeightEntry { float weight; unsigned long timestamp; };
WeightEntry weightHistory[WEIGHT_HISTORY_MAX];
int historyCount = 0;
int historyIndex = 0;

struct ButtonState {
    int pin; bool lastState; bool lastReading; unsigned long lastDebounce;
    unsigned long pressStart; bool longPressed;
};
ButtonState btnA = {BUTTON_A, HIGH, HIGH, 0, 0, false};
ButtonState btnB = {BUTTON_B, HIGH, HIGH, 0, 0, false};

bool wifiConnected = false;
bool apModeActive = false;
unsigned long wifiStartAttempt = 0;

// Battery state
float batteryVoltage = 0.0f;
int batteryPercent = 0;
bool batteryLowAlarm = false;
bool isCharging = false;
unsigned long lastBatteryRead = 0;

// ============================================================
// PROTOTYPES
// ============================================================
void initHX711();
void initOLED();
U8G2 *createOLED(uint8_t address, const char *name);
bool anyOLEDReady();
void setOLEDsPowerSave(bool enabled);
void initWiFi();
void initWebServer();
void handleButtonA(ButtonState &btn);
void handleButtonB(ButtonState &btn);
void performTare();
void resetCalibration();
void saveWeightToHistory(float weight);
void recordMeasurement();
void clearHistory();
void cycleOLEDPage();
void resetWeightFilter();
float readFilteredWeight();
void updateDisplayedWeight(float measuredWeight);
long weightToGrams(float weight);
void updateOLED();
void showWeightPage();
void showHistoryPage();
void showBatteryPage();
void wakeOLEDs();
void broadcastTelemetry();
void wsEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length);
void readBatteryVoltage();
float adcToVoltage(int adcValue);
int voltageToPercent(float voltage);

// ============================================================
// SETUP
// ============================================================
void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(1000);

    Serial.println("\n=== Balanca DIY ===");
    Serial.printf("Heap: %u\n", ESP.getFreeHeap());

    pinMode(BUTTON_A, INPUT_PULLUP);
    pinMode(BUTTON_B, INPUT_PULLUP);

    // Mount LittleFS
    if (!LittleFS.begin()) {
        Serial.println("LittleFS mount failed! Formatting...");
        LittleFS.format();
        if (!LittleFS.begin()) {
            Serial.println("LittleFS format failed!");
        } else {
            Serial.println("LittleFS formatted OK");
        }
    } else {
        Serial.println("LittleFS mounted OK");
        // List files for debug
        File root = LittleFS.open("/");
        File file = root.openNextFile();
        while (file) {
            Serial.printf("  File: %s (%d bytes)\n", file.name(), file.size());
            file = root.openNextFile();
        }
    }

    prefs = new Preferences();
    prefs->begin(PREF_NAMESPACE, false);
    Serial.println("Prefs OK");

    initHX711();
    initOLED();
    readBatteryVoltage();
    lastBatteryRead = millis();
    initWiFi();
    initWebServer();

    Serial.println("=== READY ===");
    Serial.printf("Heap: %u\n", ESP.getFreeHeap());
}

// ============================================================
// LOOP
// ============================================================
void loop() {
    unsigned long now = millis();

    if (hx711Ready && scale && scale->is_ready() && !tareInProgress) {
        updateDisplayedWeight(readFilteredWeight());
    }

    // Read battery voltage periodically
    if ((now - lastBatteryRead) > BATTERY_READ_INTERVAL) {
        readBatteryVoltage();
        lastBatteryRead = now;
    }

    handleButtonA(btnA);
    handleButtonB(btnB);

    if (anyOLEDReady() && (now - lastOledUpdate) > OLED_UPDATE_INTERVAL) {
        updateOLED();
        lastOledUpdate = now;
    }

    if (anyOLEDReady() && OLED_SCREEN_TIMEOUT > 0 && !screenSleeping &&
        (now - lastScreenActivity) > OLED_SCREEN_TIMEOUT) {
        setOLEDsPowerSave(true);
        screenSleeping = true;
    }

    if (WIFI_AP_FALLBACK && !wifiConnected && !apModeActive &&
        (now - wifiStartAttempt) > WIFI_AP_FALLBACK_DELAY) {
        WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD, WIFI_AP_CHANNEL, 0, WIFI_AP_MAX_CONN);
        apModeActive = true;
        Serial.printf("AP Fallback: %s\n", WiFi.softAPIP().toString().c_str());
    }

    if (httpServer) httpServer->handleClient();
    if (wsServer) wsServer->loop();

    static unsigned long lastBc = 0;
    if ((now - lastBc) > 1000) { broadcastTelemetry(); lastBc = now; }

    delay(1);
    yield();
}

// ============================================================
// INIT
// ============================================================
void initHX711() {
    scale = new HX711();
    scale->begin(HX711_DOUT, HX711_SCK);
    scale->set_gain(HX711_GAIN);

    unsigned long t = millis();
    while (!scale->is_ready() && millis() - t < 2000) delay(10);

    if (scale->is_ready()) {
        hx711Ready = true;
        tareOffset = prefs->getLong(PREF_OFFSET_KEY, HX711_DEFAULT_OFFSET);
        calibrationFactor = prefs->getFloat(PREF_FACTOR_KEY, HX711_DEFAULT_FACTOR);
        scale->set_offset(tareOffset);
        scale->set_scale(calibrationFactor);
        resetWeightFilter();
        Serial.printf("HX711 OK: f=%.1f\n", calibrationFactor);
    } else {
        Serial.println("HX711: not found");
    }
}

void initOLED() {
    Wire.begin(OLED_SDA, OLED_SCL);
    Wire.setClock(400000);
    Serial.printf("OLED interface: HW SDA=%d SCL=%d\n", OLED_SDA, OLED_SCL);

    u8g2 = createOLED(OLED_ADDR, "OLED");
    oledReady = u8g2 != nullptr;
    Serial.printf("OLED: %s\n", oledReady ? "OK" : "missing");
}

U8G2 *createOLED(uint8_t address, const char *name) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() != 0) {
        Serial.printf("%s: not found at 0x%02X\n", name, address);
        return nullptr;
    }

    U8G2 *display = static_cast<U8G2 *>(new U8G2_SSD1306_128X64_NONAME_F_HW_I2C(
        U8G2_R0, U8X8_PIN_NONE, OLED_SCL, OLED_SDA));
    // U8g2 expects the I2C address in 8-bit form.
    display->setI2CAddress(address << 1);
    display->begin();
    display->setContrast(OLED_CONTRAST);
    display->setFont(u8g2_font_6x10_tr);
    display->enableUTF8Print();
    Serial.printf("%s OK at 0x%02X\n", name, address);
    return display;
}

bool anyOLEDReady() {
    return oledReady;
}

void setOLEDsPowerSave(bool enabled) {
    if (oledReady) u8g2->setPowerSave(enabled);
}

void initWiFi() {
    WiFi.setHostname(HOSTNAME);
    WiFi.mode(WIFI_MODE_APSTA);

    // Scan para debug
    Serial.println("\nScanning WiFi networks...");
    int n = WiFi.scanNetworks();
    if (n == 0) {
        Serial.println("  No networks found!");
    } else {
        for (int i = 0; i < n; i++) {
            Serial.printf("  [%d] %s (ch:%d, RSSI:%d, enc:%d)\n",
                i+1, WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i), WiFi.encryptionType(i));
        }
    }
    WiFi.scanDelete();

    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD, WIFI_AP_CHANNEL, 0, WIFI_AP_MAX_CONN);
    apModeActive = true;
    Serial.printf("AP: %s @ %s\n", WIFI_AP_SSID, WiFi.softAPIP().toString().c_str());

    if (WIFI_STA_ENABLED) {
        WiFi.disconnect();
        delay(100);
        WiFi.begin(WIFI_STA_SSID, WIFI_STA_PASSWORD);
        wifiStartAttempt = millis();
        Serial.printf("\nSTA: Connecting to '%s'...", WIFI_STA_SSID);

        unsigned long t = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - t < WIFI_STA_TIMEOUT) {
            delay(500);
            Serial.print(".");
        }

        if (WiFi.status() == WL_CONNECTED) {
            wifiConnected = true;
            Serial.printf("\n STA OK @ %s (RSSI: %d)\n", WiFi.localIP().toString().c_str(), WiFi.RSSI());
        } else {
            Serial.printf("\n STA FAIL (status=%d)\n", WiFi.status());
            Serial.println(" Check: SSID, password, router channel (1-11), WPA2 only");
        }
    }
}

void initWebServer() {
    httpServer = new WebServer(HTTP_PORT);
    wsServer = new WebSocketsServer(81);

    httpServer->on("/", HTTP_GET, []() {
        if (LittleFS.exists("/index.html")) {
            File f = LittleFS.open("/index.html", "r");
            httpServer->streamFile(f, "text/html");
            f.close();
        } else httpServer->send(404, "text/plain", "Not found");
    });

    httpServer->on("/style.css", HTTP_GET, []() {
        if (LittleFS.exists("/style.css")) {
            File f = LittleFS.open("/style.css", "r");
            httpServer->streamFile(f, "text/css");
            f.close();
        } else httpServer->send(404, "text/plain", "Not found");
    });

    httpServer->on("/app.js", HTTP_GET, []() {
        if (LittleFS.exists("/app.js")) {
            File f = LittleFS.open("/app.js", "r");
            httpServer->streamFile(f, "application/javascript");
            f.close();
        } else httpServer->send(404, "text/plain", "Not found");
    });

    // OTA Update page
    httpServer->on("/update", HTTP_GET, []() {
        if (LittleFS.exists("/update.html")) {
            File f = LittleFS.open("/update.html", "r");
            httpServer->streamFile(f, "text/html");
            f.close();
        } else httpServer->send(404, "text/plain", "update.html not found");
    });

    // OTA Update handler
    httpServer->on("/update", HTTP_POST, []() {
        httpServer->sendHeader("Connection", "close");
        httpServer->send(200, "text/plain", Update.hasError() ? "FAIL" : "OK");
        Serial.println(Update.hasError() ? "OTA FAIL" : "OTA OK - Rebooting...");
        delay(100);
        ESP.restart();
    }, []() {
        HTTPUpload& upload = httpServer->upload();
        if (upload.status == UPLOAD_FILE_START) {
            Serial.printf("OTA Start: %s\n", upload.filename.c_str());
            bool isFs = upload.filename.indexOf("filesystem") >= 0;
            Update.begin(UPDATE_SIZE_UNKNOWN, isFs ? U_SPIFFS : U_FLASH);
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            Update.write(upload.buf, upload.currentSize);
        } else if (upload.status == UPLOAD_FILE_END) {
            if (Update.end(true)) {
                Serial.printf("OTA Success: %u bytes\n", upload.totalSize);
            } else {
                Serial.printf("OTA Error: %s\n", Update.errorString());
            }
        }
    });

    wsServer->begin();
    wsServer->onEvent(wsEvent);

    httpServer->on("/api/tare", HTTP_POST, []() {
        performTare();
        httpServer->send(200, "text/plain", "OK");
    });

    httpServer->on("/api/calibrate", HTTP_POST, []() {
        if (!hx711Ready) { httpServer->send(503, "application/json", "{\"status\":\"error\"}"); return; }
        if (httpServer->hasArg("weight")) {
            float w = httpServer->arg("weight").toFloat();
            if (w > 0) {
                long raw = scale->read_average(10);
                long diff = raw - tareOffset;
                if (diff != 0) {
                    // HX711 scale is raw counts per unit of the known weight.
                    calibrationFactor = (float)diff / w;
                    scale->set_scale(calibrationFactor);
                    resetWeightFilter();
                    prefs->putFloat(PREF_FACTOR_KEY, calibrationFactor);
                    httpServer->send(200, "application/json", "{\"status\":\"ok\",\"factor\":" + String(calibrationFactor, 2) + "}");
                    return;
                }
            }
        }
        httpServer->send(400, "application/json", "{\"status\":\"error\"}");
    });

    httpServer->on("/api/config", HTTP_GET, []() {
        String j = "{\"factor\":" + String(calibrationFactor, 2);
        j += ",\"offset\":" + String(tareOffset);
        j += ",\"historyCount\":" + String(historyCount) + "}";
        httpServer->send(200, "application/json", j);
    });

    httpServer->on("/api/history", HTTP_GET, []() {
        String j = "[";
        for (int i = 0; i < historyCount; i++) {
            if (i) j += ",";
            j += "{\"weight\":" + String(weightHistory[i].weight, 2);
            j += ",\"time\":" + String(weightHistory[i].timestamp) + "}";
        }
        j += "]"; httpServer->send(200, "application/json", j);
    });

    httpServer->on("/api/reset", HTTP_POST, []() {
        resetCalibration(); httpServer->send(200, "text/plain", "OK");
    });

    httpServer->begin();
    Serial.println("WebServer OK");
}

// ============================================================
// BUTTONS
// ============================================================
void handleButtonA(ButtonState &btn) {
    bool r = digitalRead(btn.pin);
    unsigned long now = millis();
    if (r != btn.lastReading) {
        btn.lastDebounce = now;
        btn.lastReading = r;
    }
    if (now - btn.lastDebounce > BUTTON_DEBOUNCE && r != btn.lastState) {
        bool wasPressed = btn.lastState == LOW;
        btn.lastState = r;
        if (r == LOW) {
            btn.pressStart = now;
            btn.longPressed = false;
        } else if (wasPressed && !btn.longPressed) {
            Serial.println("BtnA short: record measurement");
            recordMeasurement();
            lastScreenActivity = now;
        }
    }
    if (btn.lastState == LOW && now - btn.pressStart > BUTTON_A_LONG_PRESS && !btn.longPressed) {
        btn.longPressed = true;
        Serial.println("BtnA long: tare");
        wakeOLEDs();
        performTare();
        wakeOLEDs();
        updateOLED();
    }
}

void handleButtonB(ButtonState &btn) {
    bool r = digitalRead(btn.pin);
    unsigned long now = millis();
    if (r != btn.lastReading) {
        btn.lastDebounce = now;
        btn.lastReading = r;
    }
    if (now - btn.lastDebounce > BUTTON_DEBOUNCE && r != btn.lastState) {
        bool wasPressed = btn.lastState == LOW;
        btn.lastState = r;
        if (r == LOW) {
            btn.pressStart = now;
            btn.longPressed = false;
        } else if (wasPressed && !btn.longPressed) {
            cycleOLEDPage();
            lastScreenActivity = now;
        }
    }
    if (btn.lastState == LOW && now - btn.pressStart > BUTTON_B_LONG_PRESS && !btn.longPressed) {
        btn.longPressed = true;
        Serial.println("BtnB long: clear history");
        clearHistory();
        lastScreenActivity = now;
    }
}

// ============================================================
// SCALE
// ============================================================
void performTare() {
    if (!hx711Ready || tareInProgress) return;
    tareInProgress = true; tareSamplesCollected = 0;
    wakeOLEDs();
    updateOLED();
    long sum = 0;
    Serial.println("Tare...");
    for (int i = 0; i < HX711_TARE_SAMPLES; i++) {
        unsigned long t = millis();
        while (!scale->is_ready() && millis() - t < 1000) yield();
        if (!scale->is_ready()) break;
        sum += scale->read(); tareSamplesCollected = i + 1; delay(100);
    }
    if (tareSamplesCollected > 0) {
        tareOffset = sum / tareSamplesCollected;
        scale->set_offset(tareOffset); prefs->putLong(PREF_OFFSET_KEY, tareOffset);
    }
    tareInProgress = false;
    resetWeightFilter();
    Serial.printf("Tare: offset=%ld\n", tareOffset);
}

void resetCalibration() {
    calibrationFactor = HX711_DEFAULT_FACTOR; tareOffset = HX711_DEFAULT_OFFSET;
    if (scale) { scale->set_scale(calibrationFactor); scale->set_offset(tareOffset); }
    resetWeightFilter();
    prefs->putFloat(PREF_FACTOR_KEY, calibrationFactor);
    prefs->putLong(PREF_OFFSET_KEY, tareOffset);
    Serial.println("Calibration reset");
}

void saveWeightToHistory(float weight) {
    weightHistory[historyIndex].weight = weight;
    weightHistory[historyIndex].timestamp = millis();
    historyIndex = (historyIndex + 1) % WEIGHT_HISTORY_MAX;
    if (historyCount < WEIGHT_HISTORY_MAX) historyCount++;
}

void recordMeasurement() {
    if (!hx711Ready || tareInProgress || !weightDisplayInitialized) {
        Serial.println("Measurement ignored: weight not ready");
        return;
    }
    saveWeightToHistory(currentWeight);
    Serial.printf("Measurement recorded: %ld g\n", weightToGrams(currentWeight));
    oledPage = OLED_PAGE_HISTORY;
    wakeOLEDs();
    updateOLED();
}

void clearHistory() {
    historyCount = 0;
    historyIndex = 0;
    oledPage = OLED_PAGE_HISTORY;
    wakeOLEDs();
    updateOLED();
}

void cycleOLEDPage() {
    oledPage = (oledPage + 1) % OLED_PAGE_MAX;
    Serial.printf("OLED page: %d\n", oledPage);
    wakeOLEDs();
    updateOLED();
}

void resetWeightFilter() {
    for (int i = 0; i < HX711_MOVING_AVG; i++) weightFilter[i] = 0.0f;
    weightFilterCount = 0;
    weightFilterIndex = 0;
    weightFilterSum = 0.0f;
    currentWeight = 0.0f;
    weightDisplayInitialized = false;
}

float readFilteredWeight() {
    float sample = (scale->read() - tareOffset) / calibrationFactor;
    if (weightFilterCount == HX711_MOVING_AVG) {
        weightFilterSum -= weightFilter[weightFilterIndex];
    } else {
        weightFilterCount++;
    }
    weightFilter[weightFilterIndex] = sample;
    weightFilterSum += sample;
    weightFilterIndex = (weightFilterIndex + 1) % HX711_MOVING_AVG;
    return weightFilterSum / weightFilterCount;
}

void updateDisplayedWeight(float measuredWeight) {
    if (fabsf(measuredWeight) * 1000.0f <= WEIGHT_ZERO_DEADBAND_G) {
        measuredWeight = 0.0f;
    }
    float differenceGrams = fabsf(measuredWeight - currentWeight) * 1000.0f;
    if (!weightDisplayInitialized || differenceGrams >= WEIGHT_DISPLAY_DEADBAND_G) {
        currentWeight = measuredWeight;
        weightDisplayInitialized = true;
    }
}

long weightToGrams(float weight) {
    return (long)(weight * 1000.0f + (weight >= 0.0f ? 0.5f : -0.5f));
}

// ============================================================
// BATTERY MONITOR
// ============================================================
float adcToVoltage(int adcValue) {
    float adcVoltage = (adcValue / 4095.0f) * 3.3f;
    float dividerRatio = (float)(BATTERY_DIVIDER_R1 + BATTERY_DIVIDER_R2) / BATTERY_DIVIDER_R2;
    return adcVoltage * dividerRatio;
}

int voltageToPercent(float voltage) {
    if (voltage >= BATTERY_FULL_VOLT) return 100;
    if (voltage <= BATTERY_EMPTY_VOLT) return 0;
    return (int)((voltage - BATTERY_EMPTY_VOLT) / (BATTERY_FULL_VOLT - BATTERY_EMPTY_VOLT) * 100.0f);
}

void readBatteryVoltage() {
    int sum = 0;
    for (int i = 0; i < BATTERY_SAMPLES; i++) {
        sum += analogRead(BATTERY_ADC_PIN);
        delay(2);
    }
    int avg = sum / BATTERY_SAMPLES;
    batteryVoltage = adcToVoltage(avg);
    batteryPercent = voltageToPercent(batteryVoltage);
    bool wasLow = batteryLowAlarm;
    batteryLowAlarm = batteryVoltage <= BATTERY_LOW_VOLT;
    if (batteryLowAlarm && !wasLow) Serial.println("Battery alarm: LOW");
}

// ============================================================
// OLED
// ============================================================
void updateOLED() {
    if (!anyOLEDReady() || screenSleeping) return;
    setOLEDsPowerSave(false); screenSleeping = false;
    if (oledReady) {
        switch (oledPage) {
            case OLED_PAGE_WEIGHT: showWeightPage(); break;
            case OLED_PAGE_HISTORY: showHistoryPage(); break;
            case OLED_PAGE_BATTERY: showBatteryPage(); break;
        }
    }
}

void showWeightPage() {
    String value = String(weightToGrams(currentWeight));
    u8g2->firstPage();
    do {
        u8g2->setFont(u8g2_font_6x10_tr);
        u8g2->drawUTF8(0, 9, "PESO");
        u8g2->drawUTF8(86, 9, tareInProgress ? "TARANDO" : "TARA OK");
        u8g2->setFont(u8g2_font_logisoso32_tn);
        if (u8g2->getStrWidth(value.c_str()) > 116) u8g2->setFont(u8g2_font_fub20_tn);
        int valueWidth = u8g2->getStrWidth(value.c_str());
        u8g2->drawUTF8(max(0, (128 - valueWidth) / 2), 52, value.c_str());
        u8g2->setFont(u8g2_font_6x10_tr);
        u8g2->drawUTF8(111, 52, "g");
    } while (u8g2->nextPage());
}

void showHistoryPage() {
    int entries = min(historyCount, 4);
    u8g2->firstPage();
    do {
        u8g2->setFont(u8g2_font_6x10_tr);
        u8g2->drawUTF8(0, 9, "HISTORICO");
        if (entries == 0) {
            u8g2->setFont(u8g2_font_7x13_tr);
            u8g2->drawUTF8(15, 42, "SEM MEDICOES");
        } else {
            int first = historyCount < WEIGHT_HISTORY_MAX
                ? historyCount - entries
                : historyIndex - entries;
            if (first < 0) first += WEIGHT_HISTORY_MAX;
            u8g2->setFont(u8g2_font_7x13_tr);
            for (int i = 0; i < entries; i++) {
                int index = (first + i) % WEIGHT_HISTORY_MAX;
                int column = i % 2;
                int row = i / 2;
                u8g2->drawUTF8(column ? 64 : 0, row ? 53 : 31,
                    (String(i + 1) + ":" + String(weightToGrams(weightHistory[index].weight)) + "g").c_str());
            }
        }
    } while (u8g2->nextPage());
}

void showBatteryPage() {
    u8g2->firstPage();
    do {
        u8g2->setFont(u8g2_font_6x10_tr);
        u8g2->drawUTF8(0, 9, batteryLowAlarm ? "BATERIA: BAIXA" : "BATERIA: OK");
        u8g2->setFont(u8g2_font_fub20_tf);
        u8g2->drawUTF8(7, 43, (String(batteryVoltage, 2) + " V").c_str());
        u8g2->setFont(u8g2_font_6x10_tr);
        u8g2->drawUTF8(0, 61, (String("Nivel: ") + String(batteryPercent) + "%").c_str());
    } while (u8g2->nextPage());
}

void wakeOLEDs() {
    if (!anyOLEDReady()) return;
    setOLEDsPowerSave(false);
    screenSleeping = false;
    lastScreenActivity = millis();
}

// ============================================================
// WEBSOCKET
// ============================================================
void wsEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
    if (type == WStype_CONNECTED) Serial.printf("WS +%u\n", num);
    else if (type == WStype_DISCONNECTED) Serial.printf("WS -%u\n", num);
    else if (type == WStype_TEXT) {
        String msg = String((char *)payload);
        if (msg == "tare") performTare();
        else if (msg == "record") recordMeasurement();
        else if (msg == "next_page") cycleOLEDPage();
        else if (msg.startsWith("calibrate:") && hx711Ready) {
            float w = msg.substring(10).toFloat();
            if (w > 0) { long d = scale->read_average(10) - tareOffset; if (d) { calibrationFactor = (float)d / w; scale->set_scale(calibrationFactor); resetWeightFilter(); prefs->putFloat(PREF_FACTOR_KEY, calibrationFactor); } }
        }
        else if (msg.startsWith("factor:")) { float f = msg.substring(7).toFloat(); if (f > 0) { calibrationFactor = f; scale->set_scale(calibrationFactor); resetWeightFilter(); prefs->putFloat(PREF_FACTOR_KEY, calibrationFactor); } }
        else if (msg == "reset") resetCalibration();
        else if (msg == "clear_history") clearHistory();
    }
}

// ============================================================
// TELEMETRY
// ============================================================
void broadcastTelemetry() {
    if (!wsServer) return;
    JsonDocument doc;
    doc["weight"] = currentWeight;
    doc["tareInProgress"] = tareInProgress;
    doc["tareSamples"] = tareSamplesCollected;
    doc["tareTotal"] = HX711_TARE_SAMPLES;
    doc["factor"] = calibrationFactor;
    doc["offset"] = tareOffset;
    doc["historyCount"] = historyCount;
    doc["wifiConnected"] = wifiConnected;
    doc["apActive"] = apModeActive;
    doc["staIP"] = wifiConnected ? WiFi.localIP().toString() : "";
    doc["apIP"] = WiFi.softAPIP().toString();
    doc["batteryVoltage"] = batteryVoltage;
    doc["batteryPercent"] = batteryPercent;
    JsonArray h = doc["history"].to<JsonArray>();
    for (int i = max(0, historyCount - 10); i < historyCount; i++) {
        JsonObject e = h.add<JsonObject>(); e["weight"] = weightHistory[i].weight; e["time"] = weightHistory[i].timestamp;
    }
    String out; serializeJson(doc, out);
    wsServer->broadcastTXT(out);
}
