#ifndef CONFIG_H
#define CONFIG_H

// ============================================================
// HARDWARE PINS - ESP32 DevKit V4
// ============================================================

// HX711 Load Cell
#define HX711_DOUT  23
#define HX711_SCK   22

// OLED Display (I2C 0.96" SSD1306)
#define OLED_SDA    21
#define OLED_SCL    19
#define OLED_ADDR   0x3C
#define OLED_WIDTH  128
#define OLED_HEIGHT 64

// Push Buttons (active LOW with internal pull-up)
#define BUTTON_A    4   // General purpose: short press = tare, long press = reset
#define BUTTON_B    15  // Extra functions: short press = cycle display, long press = WiFi AP/STA

// Battery Monitor (ADC via voltage divider)
#define BATTERY_ADC_PIN     34  // GPIO 34 (ADC1_CH6, input only)
#define BATTERY_DIVIDER_R1  100000  // R1 (top resistor, ohms)
#define BATTERY_DIVIDER_R2  100000  // R2 (bottom resistor, ohms)
#define BATTERY_SAMPLES     20      // ADC samples for averaging
#define BATTERY_FULL_VOLT   4.2f    // Voltage at 100%
#define BATTERY_EMPTY_VOLT  3.0f    // Voltage at 0%
#define BATTERY_READ_INTERVAL 5000  // ms between battery readings

// ============================================================
// WiFi Configuration
// ============================================================

// Station Mode - Connect to existing network
#define WIFI_STA_ENABLED    true
#define WIFI_STA_SSID       "SpiderNet2"
#define WIFI_STA_PASSWORD   "JrjJe5a5C6r"
#define WIFI_STA_TIMEOUT    25000  // ms to wait for connection

// Access Point Mode - ESP32 creates its own network
#define WIFI_AP_ENABLED     true
#define WIFI_AP_SSID        "BalancaDIY"
#define WIFI_AP_PASSWORD    "12345678"
#define WIFI_AP_CHANNEL     1
#define WIFI_AP_MAX_CONN    4

// WiFi behavior
#define WIFI_MODE_DEFAULT   WIFI_MODE_APSTA  // Try STA, fallback to AP
#define WIFI_AP_FALLBACK    true             // Start AP if STA fails
#define WIFI_AP_FALLBACK_DELAY 20000         // ms before starting AP fallback

// ============================================================
// Web Server
// ============================================================
#define HTTP_PORT       80
#define WS_HEARTBEAT    5000   // WebSocket heartbeat interval (ms)
#define MAX_WS_CLIENTS  4

// ============================================================
// Scale (HX711) Configuration
// ============================================================
#define HX711_GAIN          128
#define HX711_SAMPLE_RATE   10     // samples per second for display
#define HX711_TARE_SAMPLES  30     // number of samples for tare
#define HX711_MOVING_AVG    10     // moving average window

// Default calibration (must be calibrated for your load cell)
#define HX711_DEFAULT_FACTOR  420.0f
#define HX711_DEFAULT_OFFSET  0L

// Weight history
#define WEIGHT_HISTORY_MAX    50   // max entries in weight history
#define WEIGHT_HISTORY_INTERVAL 5000  // ms between auto-saves

// ============================================================
// OLED Display
// ============================================================
#define OLED_UPDATE_INTERVAL  200    // ms between display updates
#define OLED_SCREEN_TIMEOUT   30000  // ms before screen sleeps (0 = disabled)
#define OLED_CONTRAST         255

// Display pages (cycled with BUTTON_B short press)
#define OLED_PAGE_WEIGHT      0
#define OLED_PAGE_HISTORY     1
#define OLED_PAGE_TARE        2
#define OLED_PAGE_MAX         3

// ============================================================
// Button Configuration
// ============================================================
#define BUTTON_DEBOUNCE       50     // ms debounce time
#define BUTTON_LONG_PRESS     3000   // ms to consider long press
#define BUTTON_DOUBLE_CLICK   400    // ms window for double click

// Button A actions
// Short press: Tare
// Long press: Reset calibration to defaults
// Double click: Toggle auto-save weight history

// Button B actions
// Short press: Cycle OLED display page
// Long press: Toggle WiFi AP mode / connect to STA
// Double click: Show IP on OLED

// ============================================================
// Persistent Storage (Preferences)
// ============================================================
#define PREF_NAMESPACE      "balanca"
#define PREF_OFFSET_KEY     "offset"
#define PREF_FACTOR_KEY     "factor"
#define PREF_WIFI_MODE_KEY  "wifi_mode"  // 0=AP, 1=STA, 2=APSTA
#define PREF_AUTO_SAVE_KEY  "auto_save"  // auto-save weight history

// ============================================================
// System
// ============================================================
#define SERIAL_BAUD         115200
#define HOSTNAME            "balanca-diy"
#define OTA_PATH            "/update"
#define OTA_USERNAME        "admin"
#define OTA_PASSWORD        "admin"

#endif // CONFIG_H
