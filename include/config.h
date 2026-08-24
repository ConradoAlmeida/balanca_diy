#ifndef CONFIG_H
#define CONFIG_H

// ============================================================
// HARDWARE PINS - ESP32 DevKit V4
// ============================================================

// HX711 Load Cell
#define HX711_DOUT  23
#define HX711_SCK   22

// OLED Display (SSD1306, hardware I2C)
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
#define BATTERY_LOW_VOLT    3.3f    // Visual low-battery alarm threshold
#define BATTERY_READ_INTERVAL 5000  // ms between battery readings
#define BATTERY_CAL         1.074f  // Fator de calibracao do ADC (real / reportado). Ex: 4.19/3.90 = 1.074

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
#define HX711_SAMPLE_RATE   8     // samples per second for display
#define HX711_TARE_SAMPLES  30     // number of samples for tare (~1 s at 10 SPS)
#define HX711_MOVING_AVG    30     // moving average window (~4 s at 10 SPS)
#define WEIGHT_DISPLAY_DEADBAND_G 1.0f // Ignore display changes smaller than 2 g
#define WEIGHT_ZERO_DEADBAND_G 2.0f // Show zero between -2 g and +2 g

// Default calibration (must be calibrated for your load cell)
#define HX711_DEFAULT_FACTOR  420.0f
#define HX711_DEFAULT_OFFSET  0L

// Weight history
#define WEIGHT_HISTORY_MAX    4   // max entries in weight history

// ============================================================
// OLED Display
// ============================================================
#define OLED_UPDATE_INTERVAL  200    // ms between OLED1 updates
#define OLED_SCREEN_TIMEOUT   0      // ms before screen sleeps (0 = disabled)
#define OLED_CONTRAST         255

// OLED pages (cycled with BUTTON_B short press)
#define OLED_PAGE_WEIGHT      0
#define OLED_PAGE_HISTORY     1
#define OLED_PAGE_BATTERY     2
#define OLED_PAGE_NETWORK     3
#define OLED_PAGE_MAX         4

// ============================================================
// Button Configuration
// ============================================================
#define BUTTON_DEBOUNCE       50     // ms debounce time
#define BUTTON_A_LONG_PRESS   1000   // ms before Button A starts tare
#define BUTTON_B_LONG_PRESS   2000   // ms before Button B clears history

// Button A actions
// Short press: Record measurement to history
// Long press:  Clear history

// Button B actions
// Short press: Cycle OLED pages (weight, history, battery, network)
// Long press:  Tare the scale

// ============================================================
// Persistent Storage (Preferences)
// ============================================================
#define PREF_NAMESPACE      "balanca"
#define PREF_OFFSET_KEY     "offset"
#define PREF_FACTOR_KEY     "factor"
#define PREF_WIFI_MODE_KEY  "wifi_mode"  // 0=AP, 1=STA, 2=APSTA

// ============================================================
// System
// ============================================================
#define SERIAL_BAUD         115200
#define HOSTNAME            "balanca-diy"
#define OTA_PATH            "/update"
#define OTA_USERNAME        "admin"
#define OTA_PASSWORD        "admin"

#endif // CONFIG_H
