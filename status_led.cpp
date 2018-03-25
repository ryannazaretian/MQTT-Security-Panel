#include "status_led.h"


StatusLed::StatusLed(uint8_t u8_red_pin, uint8_t u8_green_pin) {
  this->u8_red_pin = u8_red_pin;
  this->u8_green_pin = u8_green_pin; 
  pinMode(this->u8_red_pin, OUTPUT);
  pinMode(this->u8_green_pin, OUTPUT);  
}

void StatusLed::setStatus(uint8_t u8_status) {
 if (this->u8_status != u8_status) {
   this->u8_status = u8_status;
   this->b_blink = true; // Turn b_blink on to cause the LEDs to display some status (in the event that the CPU locks up)
   this->update_leds();// Update the LED
 }
}

void StatusLed::update_leds(void) {
//  Serial.println("update_leds called");
  if (!(this->u8_status & STATUS_BLINK) || this->b_blink) {
      digitalWrite(u8_red_pin,   STATUS_RED & this->u8_status);
      digitalWrite(u8_green_pin, STATUS_GRN & this->u8_status);
    } else {
      digitalWrite(u8_red_pin,   STATUS_OFF);
      digitalWrite(u8_green_pin, STATUS_OFF);
    }
  this->b_blink = !this->b_blink;
}

