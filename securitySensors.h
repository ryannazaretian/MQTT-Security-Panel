#include <stdint.h>
#include <string.h>

#define PIN_START 22
#define PIN_END 53

#define NUM_OF_SENSORS (PIN_END - PIN_START + 1)

#define INDEX_TO_PIN(val) (val + PIN_START)
#define PIN_TO_INDEX(val) (val - PIN_START)

static bool au8_lastSensorValues[PIN_END - PIN_START];

void setupSensors(void) {
  uint8_t u8_pin;
  for(u8_pin = PIN_START; u8_pin <= PIN_END; u8_pin++)
    pinMode(u8_pin, INPUT_PULLUP);
    au8_lastSensorValues[PIN_TO_INDEX(u8_pin)] = true;
}

void getSensorNameValue(uint8_t u8_index, char* psz_sensorName, char* psz_sensorValue) {
  char psz_sensorNum[3] = "";
  strcpy(psz_sensorName, "securitySystem/sensor_");
  itoa(u8_index, psz_sensorNum, 2);
  strcat(psz_sensorName, psz_sensorNum);
  itoa(int(!digitalRead(INDEX_TO_PIN(u8_index))), psz_sensorValue, 1);
}

void checkSensors(uint8_t au8_sensorsToUpdate[], uint8_t* u8_sensorsToUpdateLength) {
  bool u8_tempValue; 
  uint8_t u8_pin;
  
  *u8_sensorsToUpdateLength = 0;
  
  for(u8_pin = PIN_START; u8_pin <= PIN_END; u8_pin++) {
    u8_tempValue = digitalRead(u8_pin);
    if (au8_lastSensorValues[PIN_TO_INDEX(u8_pin)] != u8_tempValue)
      au8_sensorsToUpdate[*u8_sensorsToUpdateLength++] = u8_pin;
  }
}

