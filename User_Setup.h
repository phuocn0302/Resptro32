// ----------------------------
//    ST7735 Display Settings
// ----------------------------

#define ST7735_DRIVER       // Use ST7735 display driver
#define TFT_WIDTH  128      // Display width
#define TFT_HEIGHT 160      // Display height

#define ST7735_BLACKTAB    // Adjust this based on your display type (try GREENTAB2 or BLACKTAB if needed)

// ----------------------------
//    ESP32 Pin Configuration
// ----------------------------

#define TFT_CS    5   // Chip select
#define TFT_RST   4   // Reset
#define TFT_DC    2   // Data/Command

#define TFT_MOSI  23  // SPI MOSI
#define TFT_SCLK  18  // SPI Clock

// SPI MISO (not needed for ST7735)
#define TFT_MISO  -1

// ----------------------------
//    Font and Rotation
// ----------------------------

#define LOAD_GLCD   // Include default font
#define LOAD_FONT2  // Small font
#define LOAD_FONT4  // Medium font
#define LOAD_FONT6  // Large font
#define LOAD_FONT7  // Extra large font
#define LOAD_FONT8  // Huge font

#define SMOOTH_FONT // Enable smooth fonts

// Default rotation (0 to 3)
#define TFT_ROTATION  1

// ----------------------------
//    Additional Settings
// ----------------------------

#define SPI_FREQUENCY  40000000   // 27 MHz SPI speed
#define SPI_READ_FREQUENCY  40000000  // Read speed

#define SUPPORT_TRANSACTIONS  // Enable SPI transactions (important for multitasking)
