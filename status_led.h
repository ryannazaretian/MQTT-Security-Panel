#include <stdint.h>
#include <Arduino.h>

#define STATUS_OFF 0x0
#define STATUS_GRN 0x1
#define STATUS_RED 0x2
#define STATUS_YEL (STATUS_GRN | STATUS_RED)
#define STATUS_BLINK 0x4
#define STATUS_BLINK_GRN (STATUS_GRN | STATUS_BLINK)
#define STATUS_BLINK_YEL (STATUS_YEL | STATUS_BLINK)
#define STATUS_BLINK_RED (STATUS_RED | STATUS_BLINK)

class StatusLed {
  public:
    StatusLed(uint8_t u8_red_pin, uint8_t u8_green_pin); 
    void setStatus(uint8_t u8_status);
    void update_leds(void);

  private:
    bool b_blink;
    uint8_t u8_red_pin;
    uint8_t u8_green_pin;
    uint8_t u8_status = 0;
   
};
