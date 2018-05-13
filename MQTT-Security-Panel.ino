/*
 Basic MQTT example

 This sketch demonstrates the basic capabilities of the library.
 It connects to an MQTT server then:
	- publishes "hello world" to the topic "outTopic"
	- subscribes to the topic "inTopic", printing out any messages
		it receives. NB - it assumes the received payloads are strings not binary

 It will reconnect to the server if the connection is lost using a blocking
 reconnect function. See the 'MQTT_STATE_reconnect_nonblocking' example for how to
 achieve the same result without blocking the main loop.

*/

#define DEBUG

#include <Ethernet.h>
#include <PubSubClient.h>

#include "pinout.h"
#include "status_led.h"
#include "utils.h"
#include "securitySensors.h"
#include "SimpleTaskScheduler.h"
#include "secrets.h"

// Ethernet Seutp
#define ETH_RESET_ASSERT 500
#define ETH_RESET_WAIT 2500
#define ETH_CONNECT_WAIT 1000
EthernetClient ethClient;

// MQTT Setup
#define MQTT_KEEPALIVE 1
#define MQTT_STATE_RESET_WAIT 5000
#define MQTT_VERSION MQTT_VERSION_3_1_1
PubSubClient client(ethClient);

// Status LED Setup
StatusLed status(STATUS_LED_RED, STATUS_LED_GRN);

// Function Declarations
void setup_runner(void);
void disable_publishing(void);
void enable_publishing(void);

void mqtt_callback(char* psz_topic, char* ps_payload, unsigned int len);
void printIPAddress(void);
bool checkEthernet(void);
void loop_ethernet(void);
void loop_mqtt(void);
void loop_toggle_door(void);

void tcb_update_leds(void);
void tcb_maintain_eth(void);
void publishAllSensorValues(void);

void publishChangedSensors(void);

#define SENSE_NAME_LENGTH 32
#define SENSE_VAL_LENGTH 8


// Task Scheduler
#define TASK_RATE_BLINK_PERIOD 500
#define TASK_RATE_RESET_ETH 5000
#define TASK_RATE_RECONNECT_MQTT 5000
#define TASK_RATE_SENSOR_PUBLISH_ALL 10000
#define TASK_RATE_SENSOR_CHANGE_NOTIFICATION 50
#define TASK_RATE_MAINTAIN_ETH 60000

SimpleTaskScheduler taskScheduler(4, TS_MILLIS);
uint8_t t_status_led = taskScheduler.addTask(tcb_update_leds, TASK_RATE_BLINK_PERIOD, true);
uint8_t t_periodic_sensor_update = taskScheduler.addTask(publishAllSensorValues, TASK_RATE_SENSOR_PUBLISH_ALL, false);
uint8_t t_sensor_change_notification = taskScheduler.addTask(publishChangedSensors, TASK_RATE_SENSOR_CHANGE_NOTIFICATION, false);
uint8_t t_ethernet_maintain = taskScheduler.addTask(tcb_maintain_eth, TASK_RATE_MAINTAIN_ETH, true);

void tcb_update_leds(void) {
	status.update_leds();
}

void tcb_maintain_eth(void) {
  SERIAL_PRINTLN("ETH: Maintain");
  switch (Ethernet.maintain())
        {
          case 1:
            //renewed fail
            SERIAL_PRINTLN("ETH: Renew error");
            break;
          case 2:
            //renewed success
            SERIAL_PRINTLN("ETH: Renew ok");
            //print your local IP address:
            break;
          case 3:
            //rebind fail
            SERIAL_PRINTLN("ETH: Rebind error");
            break;
          case 4:
            //rebind success
            SERIAL_PRINTLN("ETH: Rebind ok");
            break;
          default:
            break;
        }
}



void mqtt_callback(char* psz_topic, uint8_t* pu8_payload, unsigned int len) {
	char psz_payload[32];

	// Fill the payload with zeros. This will handle the zero-termination
	memset(psz_payload, 0, 32*sizeof(char));
	memcpy(psz_payload, pu8_payload, len); // Copy the payload to local memory

	// Print off some debug messages
	SERIAL_PRINT("MQTT PAYLD [");
	SERIAL_PRINT(psz_topic);
	SERIAL_PRINT("] ");
	SERIAL_PRINTLN(psz_payload);	
}

void print_mqtt_state(void) {
    switch(client.state()){
      case MQTT_CONNECTION_TIMEOUT:
          SERIAL_PRINTLN("MQTT_CONNECTION_TIMEOUT");
          break;
      case MQTT_CONNECTION_LOST:
          SERIAL_PRINTLN("MQTT_CONNECTION_LOST");
          break;
      case MQTT_CONNECT_FAILED:
          SERIAL_PRINTLN("MQTT_CONNECT_FAILED");
          break;
      case MQTT_DISCONNECTED:
          SERIAL_PRINTLN("MQTT_DISCONNECTED");
          break;
      case MQTT_CONNECTED:
          SERIAL_PRINTLN("MQTT_CONNECTED");
          break;
      case MQTT_CONNECT_BAD_PROTOCOL:
          SERIAL_PRINTLN("MQTT_CONNECT_BAD_PROTOCOL");
          break;
      case MQTT_CONNECT_BAD_CLIENT_ID:
          SERIAL_PRINTLN("MQTT_CONNECT_BAD_CLIENT_ID");
          break;
      case MQTT_CONNECT_UNAVAILABLE:
          SERIAL_PRINTLN("MQTT_CONNECT_UNAVAILABLE");
          break;
      case MQTT_CONNECT_BAD_CREDENTIALS:
          SERIAL_PRINTLN("MQTT_CONNECT_BAD_CREDENTIALS");
          break;
      case MQTT_CONNECT_UNAUTHORIZED:
          SERIAL_PRINTLN("MQTT_CONNECT_UNAUTHORIZED");
          break;
      default:
          SERIAL_PRINTLN("Undefined");
          break;
    }
}

void loop_mqtt(MqttState *pe_MQTT_STATE_state) {
	static unsigned long ul_delay_trigger;
  int8_t i8_mqtt_status;
	switch(*pe_MQTT_STATE_state) {
		case MQTT_STATE_NOT_SETUP:
			*pe_MQTT_STATE_state = MQTT_STATE_CONNECTING;
			break;
		case MQTT_STATE_CONNECTING:
      SERIAL_PRINTLN("MQTT: Connecting...");
			if (client.connect("securityClient", USERNAME, PASSWORD)) {
				SERIAL_PRINTLN("MQTT: Connected");
				
        // Once connected & subscribed, publish an announcement...
        client.publish("securityClient", "started");
        
				*pe_MQTT_STATE_state = MQTT_STATE_CONNECTED;
			} else {
				SERIAL_PRINT("MQTT Disconnected: ");
        print_mqtt_state();
				ul_delay_trigger = millis() + MQTT_STATE_RESET_WAIT;
				*pe_MQTT_STATE_state = MQTT_STATE_DELAY;
			}
			break;
		case MQTT_STATE_DELAY:
			if (millis() > ul_delay_trigger)
				*pe_MQTT_STATE_state = MQTT_STATE_CONNECTING;
      break;
		case MQTT_STATE_CONNECTED:
      if (!client.loop()) {
        SERIAL_PRINT("MQTT Disconnected: ");
        print_mqtt_state();
				*pe_MQTT_STATE_state = MQTT_STATE_CONNECTING;
			}
      break;
		default:
			*pe_MQTT_STATE_state = MQTT_STATE_NOT_SETUP;
	}
}

void setup() {
  
  
	// Setup input pins
	analogReference(DEFAULT);
  setupSensors();

	// Setup output pins
  pinMode(SDCARD_CS,OUTPUT);
  digitalWrite(SDCARD_CS,HIGH);//Deselect the SD card

	START_SERIAL(9600);
  SERIAL_PRINTLN();
	SERIAL_PRINTLN("MASTER RESET");

  Ethernet.begin(MAC_ADDRESS, CLIENT_IP) ;
  delay(TASK_RATE_RESET_ETH);
	
	taskScheduler.enableTask(t_status_led, true);

	// Setup MQTT
	status.setStatus(STATUS_RED);
	client.setServer(SERVER_IP, 1883);
	client.setCallback(mqtt_callback);
}


void publishAllSensorValues(void) {
  uint8_t u8_i;
  char psz_sensorName[SENSE_NAME_LENGTH];
  char psz_sensorValue[SENSE_VAL_LENGTH];
  SERIAL_PRINTLN("All Sensors: ");
  for (u8_i = 0; u8_i < NUM_OF_SENSORS; u8_i++){
    memset(psz_sensorName, 0, SENSE_NAME_LENGTH*sizeof(char));
    memset(psz_sensorValue, 0, SENSE_VAL_LENGTH*sizeof(char));
    getSensorNameValue(u8_i, psz_sensorName, psz_sensorValue);
    SERIAL_PRINT("  ");
    SERIAL_PRINT(psz_sensorName);
    SERIAL_PRINT(": ");
    SERIAL_PRINTLN(psz_sensorValue);
    SERIAL_PRINT("Publishing: ");
    if (client.publish(psz_sensorName, psz_sensorValue) && client.loop())
        SERIAL_PRINTLN("Successful");
    else
        SERIAL_PRINTLN("Failed");
  } 
}

void publishChangedSensors(void) {
  uint8_t u8_i;
  uint8_t u8_numOfSensorsToUpdate;
  uint8_t au8_sensorsToUpdate[NUM_OF_SENSORS];
  char psz_sensorName[SENSE_NAME_LENGTH];
  char psz_sensorValue[SENSE_VAL_LENGTH];
  checkSensors(au8_sensorsToUpdate, &u8_numOfSensorsToUpdate);
  if (u8_numOfSensorsToUpdate > 0)
    SERIAL_PRINTLN("Changed Sensors: ");
  for (u8_i = 0; u8_i < u8_numOfSensorsToUpdate; u8_i++){
    memset(psz_sensorName, 0, SENSE_NAME_LENGTH*sizeof(char));
    memset(psz_sensorValue, 0, SENSE_VAL_LENGTH*sizeof(char));
    getSensorNameValue(au8_sensorsToUpdate[u8_i], psz_sensorName, psz_sensorValue);
    SERIAL_PRINT("  ");
    SERIAL_PRINT(psz_sensorName);
    SERIAL_PRINT(": ");
    SERIAL_PRINTLN(psz_sensorValue);
    SERIAL_PRINT("Publishing: ");
    if (client.publish(psz_sensorName, psz_sensorValue) && client.loop())
        SERIAL_PRINTLN("Successful");
    else
        SERIAL_PRINTLN("Failed");
  }
}

void enable_publishing(void) {
 taskScheduler.enableTask(t_periodic_sensor_update, true);
 taskScheduler.enableTask(t_sensor_change_notification, true);
}

void disable_publishing(void) {
  taskScheduler.disableTask(t_periodic_sensor_update);
  taskScheduler.disableTask(t_sensor_change_notification);
}

void loop() {
	static EthernetState ethernet_state;
	static MqttState mqtt_state;
	loop_mqtt(&mqtt_state);
  taskScheduler.loop();

  if (client.connected()) {
       status.setStatus(STATUS_BLINK_GRN);   
       enable_publishing();
  } else {
      disable_publishing();
      status.setStatus(STATUS_BLINK_RED);
  }
}

