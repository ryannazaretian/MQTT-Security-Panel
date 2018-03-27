#include <stdint.h>
#include <string.h>

#define PIN_START 26
#define PIN_END 47

#define NUM_OF_SENSORS ((PIN_END - PIN_START) + 1)
#define INDEX_TO_PIN(val) (val + PIN_START)

static bool ab_lastSensorValues[NUM_OF_SENSORS];

void setupSensors(void) {
  uint8_t u8_i;
  for(u8_i = 0; u8_i < NUM_OF_SENSORS; u8_i++) {
    pinMode(INDEX_TO_PIN(u8_i), INPUT_PULLUP);
    ab_lastSensorValues[u8_i] = true;
  }
}

void getSensorNameValue(uint8_t u8_index, char* psz_sensorName, char* psz_sensorValue) {
  char psz_sensorNum[3] = "";
  strcpy(psz_sensorName, "securitySystem/sensor_");
  itoa(u8_index, psz_sensorNum, 10);
  strcat(psz_sensorName, psz_sensorNum);
  itoa(int(digitalRead(INDEX_TO_PIN(u8_index))), psz_sensorValue, 10);
}

void checkSensors(uint8_t au8_sensorsToUpdate[], uint8_t* u8_sensorsToUpdateLength) {
  bool b_tempValue; 
  uint8_t u8_i;
  
  (*u8_sensorsToUpdateLength) = 0;
  
  for(u8_i = 0; u8_i < NUM_OF_SENSORS; u8_i++) {
    b_tempValue = digitalRead(INDEX_TO_PIN(u8_i));
    if (ab_lastSensorValues[u8_i] != b_tempValue) {
      au8_sensorsToUpdate[(*u8_sensorsToUpdateLength)] = u8_i;
      (*u8_sensorsToUpdateLength)++;
    }
    ab_lastSensorValues[u8_i] = b_tempValue;
  }
}

