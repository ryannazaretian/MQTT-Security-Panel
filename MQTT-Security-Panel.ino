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
#include <NewPing.h>
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
#define MQTT_STATE_RESET_WAIT 5000
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
void publishAllSensorValues(void);

void publishChangedSensors(void);

#define SENSE_NAME_LENGTH 32
#define SENSE_VAL_LENGTH 8


// Task Scheduler
#define TASK_RATE_MAINTAIN_ETH 60000
#define TASK_RATE_BLINK_PERIOD 500
#define TASK_RATE_RESET_ETH 5000
#define TASK_RATE_RECONNECT_MQTT 5000
#define TASK_RATE_SENSOR_PUBLISH_ALL 30000
#define TASK_RATE_SENSOR_CHANGE_NOTIFICATION 50

SimpleTaskScheduler taskScheduler(3, TS_MILLIS);
uint8_t t_status_led = taskScheduler.addTask(tcb_update_leds, TASK_RATE_BLINK_PERIOD, true);
uint8_t t_periodic_sensor_update = taskScheduler.addTask(publishAllSensorValues, TASK_RATE_SENSOR_PUBLISH_ALL, true);
uint8_t t_sensor_change_notification = taskScheduler.addTask(publishChangedSensors, TASK_RATE_SENSOR_CHANGE_NOTIFICATION, true);

void tcb_update_leds(void) {
	status.update_leds();
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

void loop_mqtt(MqttState *pe_MQTT_STATE_state) {
	static unsigned long ul_delay_trigger;
	switch(*pe_MQTT_STATE_state) {
		case MQTT_STATE_NOT_SETUP:
			*pe_MQTT_STATE_state = MQTT_STATE_CONNECTING;
			break;
		case MQTT_STATE_CONNECTING:
      SERIAL_PRINTLN("MQTT: Connecting...");
			if (client.connect("securityClient", USERNAME, PASSWORD)) {
				SERIAL_PRINTLN("MQTT:  Connected");
				
        // Once connected & subscribed, publish an announcement...
        client.publish("securityClient", "started");
        
				*pe_MQTT_STATE_state = MQTT_STATE_CONNECTED;
			} else {
				SERIAL_PRINT("MQTT: Error, RC=");
				SERIAL_PRINT(client.state());
				SERIAL_PRINTLN(", Retrying...");
				ul_delay_trigger = millis() + MQTT_STATE_RESET_WAIT;
				*pe_MQTT_STATE_state = MQTT_STATE_DELAY;
			}
			break;
		case MQTT_STATE_DELAY:
			if (millis() > ul_delay_trigger)
				*pe_MQTT_STATE_state = MQTT_STATE_CONNECTING;
      break;
		case MQTT_STATE_CONNECTED:
			if (!client.connected()) {
				SERIAL_PRINTLN("Lost MQTT Connection");
				*pe_MQTT_STATE_state = MQTT_STATE_CONNECTING;
			}
      break;
		default:
			*pe_MQTT_STATE_state = MQTT_STATE_NOT_SETUP;
	}
}



void loop_ethernet(EthernetState *pe_ethernet_state) {
	static unsigned long ul_delay_trigger;
	switch(*pe_ethernet_state) {
		case ETH_NOT_SETUP:
			*pe_ethernet_state = ETH_TOGGLE_RESET;
			break;
			
		case ETH_TOGGLE_RESET:
			SERIAL_PRINTLN("ETH: Toggling Ethernet Reset pin... ");
			digitalWrite(ETH_RST, HIGH);
			ul_delay_trigger = millis() + ETH_RESET_ASSERT;
			*pe_ethernet_state = ETH_DELAY_TOGGLE_RESET;
			break;
		case ETH_DELAY_TOGGLE_RESET:
			if (millis() > ul_delay_trigger) {
				SERIAL_PRINTLN("ETH: Waiting for Ethernet Control to come out of reset... ");
				digitalWrite(ETH_RST, LOW);
				ul_delay_trigger = millis() + ETH_RESET_WAIT;
				*pe_ethernet_state = ETH_DELAY_IN_RESET;
			}
			break;
		case ETH_DELAY_IN_RESET:
			if (millis() > ul_delay_trigger) {
				SERIAL_PRINTLN("ETH: Out of reset.");
        *pe_ethernet_state = ETH_CONNECTING;
			}
     break;
    case ETH_CONNECTING:
        SERIAL_PRINTLN("ETH: Connecting...");
				Ethernet.begin(MAC_ADDRESS, CLIENT_IP) ;
        ul_delay_trigger = millis() + ETH_CONNECT_WAIT;
        *pe_ethernet_state = ETH_CONNECTING_DELAY;
        break;
     case ETH_CONNECTING_DELAY:
        if (millis() > ul_delay_trigger) {
				SERIAL_PRINT("ETH: ");
				if (checkEthernet()) {
					SERIAL_PRINTLN("Connected.");
					*pe_ethernet_state = ETH_CONNECTED_MAINTAIN;
				} else {
					SERIAL_PRINTLN("Failed to connect. Resetting...");
					*pe_ethernet_state = ETH_TOGGLE_RESET;
				}
			}
			break;
		case ETH_CONNECTED_MAINTAIN:
			if (!checkEthernet()) {
				SERIAL_PRINTLN("ETH: Lost connection.");
				*pe_ethernet_state = ETH_TOGGLE_RESET;
			} else {
				SERIAL_PRINT("ETH: Performing maintenance... ");
				switch (Ethernet.maintain())
				{
					case 1:
						//renewed fail
						SERIAL_PRINTLN("Renew error");
						*pe_ethernet_state = ETH_TOGGLE_RESET;
						break;
					case 2:
						//renewed success
						SERIAL_PRINTLN("Renew ok");
						//print your local IP address:
						ul_delay_trigger = millis() + TASK_RATE_MAINTAIN_ETH;
						*pe_ethernet_state = ETH_CONNECTED_DELAY_MAINTAIN;
						break;
					case 3:
						//rebind fail
						SERIAL_PRINTLN("Rebind error");
						*pe_ethernet_state = ETH_TOGGLE_RESET;
						break;
					case 4:
						//rebind success
						SERIAL_PRINTLN("Rebind ok");
						ul_delay_trigger = millis() + TASK_RATE_MAINTAIN_ETH;
						*pe_ethernet_state = ETH_CONNECTED_DELAY_MAINTAIN;

						break;
					default:
            SERIAL_PRINTLN("Done");
						ul_delay_trigger = millis() + TASK_RATE_MAINTAIN_ETH;
						*pe_ethernet_state = ETH_CONNECTED_DELAY_MAINTAIN;
						break;
				}
			}
			break;
		case ETH_CONNECTED_DELAY_MAINTAIN:
			if (!checkEthernet()) {
					SERIAL_PRINTLN("ETH: Lost connection.");
					*pe_ethernet_state = ETH_TOGGLE_RESET;
				}
			if (millis() > ul_delay_trigger)
				*pe_ethernet_state = ETH_CONNECTED_MAINTAIN;
			break;
		default:
			*pe_ethernet_state = ETH_NOT_SETUP;
	}
}

bool checkEthernet(void) {
//  return true;
  bool is_valid = false;
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    if (Ethernet.localIP()[thisByte] != 0) {
      is_valid = true;
    }
  }
  return is_valid;
}

void setup() {
	// Setup input pins
	analogReference(DEFAULT);
  setupSensors();

	// Setup output pins
	pinMode(ETH_RST,						OUTPUT);
  pinMode(SDCARD_CS,OUTPUT);
  digitalWrite(SDCARD_CS,HIGH);//Deselect the SD card
	digitalWrite(ETH_RST, LOW);

	START_SERIAL(9600);
  SERIAL_PRINTLN();
	SERIAL_PRINTLN("MASTER RESET");
	
	taskScheduler.enableTask(t_status_led, true);

	// Setup MQTT
	status.setStatus(STATUS_RED);
	client.setServer(SERVER_IP, 1883);
	client.setCallback(mqtt_callback);

}

//SENSE_NAME_LENGTH
//SENSE_VAL_LENGTH

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
    client.publish(psz_sensorName, psz_sensorValue);
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
    client.publish(psz_sensorName, psz_sensorValue);
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
	loop_ethernet(&ethernet_state);
  taskScheduler.loop();


	
	if (!(ethernet_state & ETH_CONNECTED_MASK)) {
		// No Ethernet
		status.setStatus(STATUS_RED);
		disable_publishing();
		mqtt_state = MQTT_STATE_NOT_SETUP;
	} else {
		if (!(mqtt_state & MQTT_STATE_CONNECTED_MASK)) {
			// No MQTT Connection
			status.setStatus(STATUS_BLINK_RED);
			disable_publishing();
		} else {
			// Normal Mode
			enable_publishing();
      status.setStatus(STATUS_BLINK_GRN);
			client.loop();
      
		}
	}
}

