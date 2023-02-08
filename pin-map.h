////////////////////////////////////////////
// CONTROL BOARD PIN OUT
////////////////////////////////////////////
// Only change if you using a different PCB
////////////////////////////////////////////

#if defined(AMIDALA_DISPLAY)

#define RXD1_PIN                1
#define TXD1_PIN                2

#define RXD2_PIN                3
#define TXD2_PIN                16

#define RXD3_PIN                12
#define TXD3_PIN                13

#define RXD0_PIN                44
#define TXD0_PIN                43

#define SDA_PIN                 18
#define SCL_PIN                 17

#define DOUT_PIN1               11
#define DOUT_PIN2               10

#define PPMIN_RC                21

#ifndef NODISPLAY
#define USE_LVGL_DISPLAY
#endif

#define PIN_LCD_BL              38

#define PIN_LCD_D0              39
#define PIN_LCD_D1              40
#define PIN_LCD_D2              41
#define PIN_LCD_D3              42
#define PIN_LCD_D4              45
#define PIN_LCD_D5              46
#define PIN_LCD_D6              47
#define PIN_LCD_D7              48

#define PIN_POWER_ON            15

#define PIN_LCD_RES             5
#define PIN_LCD_CS              6
#define PIN_LCD_DC              7
#define PIN_LCD_WR              8
#define PIN_LCD_RD              9

#define PIN_BUTTON_1            0
#define PIN_BUTTON_2            14
#define PIN_BAT_VOLT            4

#define PIN_IIC_SCL             17
#define PIN_IIC_SDA             18

#define PIN_TOUCH_INT           16
#define PIN_TOUCH_RES           21

#define AMIDALA_ORDER

#elif defined(PENUMBRA)

#define DIN1_PIN                34
#define DIN2_PIN                35

#define DOUT1_PIN               14
#define DOUT2_PIN               13

#define RXD1_PIN                33
#define TXD1_PIN                25
#define RXD2_PIN                16
#define TXD2_PIN                17
#define RXD3_PIN                32
#define TXD3_PIN                4

#define OUTPUT_ENABLED_PIN      27
#define RS485_RTS_PIN           26

#define SCL_PIN 22
#define SDA_PIN 21

#elif defined(PCA9685_BACKPACK)

#define OUTPUT_ENABLED_PIN 5
#define SDA2_PIN 16
#define SCL2_PIN 17
#define TXD1_PIN 33
#define RXD1_PIN 34
#define SCL_PIN 22
#define SDA_PIN 21
#define TXD2_PIN 22
#define RXD2_PIN 21

#else

#define SCL_PIN 22
#define SDA_PIN 21

#endif

//#define COMMAND_SERIAL Serial3