/*
 * BLE Remote ID Scanner for ESP32-C6 (PlatformIO Arduino)
 */

 #if !defined(ARDUINO_ARCH_ESP32)
 #error "This program requires an ESP32"
#endif

#include <Arduino.h>
#include <HardwareSerial.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include "opendroneid.h"

// Custom UART pin definitions for Serial1
const int SERIAL1_RX_PIN = 7;  // GPIO7
const int SERIAL1_TX_PIN = 6;  // GPIO6

// Data structure to store decoded Remote ID info.
struct id_data {
 uint8_t  mac[6];                      // Device MAC address
 uint32_t last_seen;                   // Timestamp (millis) when last seen
 char     op_id[ODID_ID_SIZE + 1];       // Operator ID string
 char     uav_id[ODID_ID_SIZE + 1];      // UAV ID string (from Basic ID message)
 double   lat_d;                       // Latitude
 double   long_d;                      // Longitude
 double   base_lat_d;                  // Base latitude (from system message)
 double   base_long_d;                 // Base longitude (from system message)
 int      altitude_msl;                // Altitude (MSL)
 int      height_agl;                  // Height above ground level
 int      speed;                       // Horizontal speed
 int      heading;                     // Heading/direction
 int      rssi;                        // RSSI (dBm)
 int      flag;                        // Set to 1 when new data is available
};

#define MAX_UAVS 8
id_data uavs[MAX_UAVS] = {0};

// Helper: Find or allocate an entry for a given MAC address.
id_data* next_uav(uint8_t* mac) {
 for (int i = 0; i < MAX_UAVS; i++) {
   if (memcmp(uavs[i].mac, mac, 6) == 0)
     return &uavs[i];
 }
 for (int i = 0; i < MAX_UAVS; i++) {
   if (uavs[i].mac[0] == 0)
     return &uavs[i];
 }
 return &uavs[0];  // Fallback if all are used.
}

// BLE Advertised Device Callback.
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
public:
 void onResult(BLEAdvertisedDevice device) override {
   int len = device.getPayloadLength();
   if (len <= 0)
     return;
     
   uint8_t* payload = device.getPayload();
   // Check for a Remote ID advertisement signature.
   // Expected signature at payload bytes 1..4: 0x16, 0xFA, 0xFF, 0x0D.
   if (len > 5 &&
       payload[1] == 0x16 &&
       payload[2] == 0xFA &&
       payload[3] == 0xFF &&
       payload[4] == 0x0D)
   {
     uint8_t* mac = (uint8_t*) device.getAddress().getNative();
     id_data* UAV = next_uav(mac);
     UAV->last_seen = millis();
     UAV->rssi = device.getRSSI();
     UAV->flag = 1;
     memcpy(UAV->mac, mac, 6);
     
     // Assume the Remote ID message payload starts at index 6.
     uint8_t* odid = &payload[6];
     switch (odid[0] & 0xF0) {
       case 0x00: {  // Basic ID message
         ODID_BasicID_data basic;
         decodeBasicIDMessage(&basic, (ODID_BasicID_encoded*) odid);
         strncpy(UAV->uav_id, (char*) basic.UASID, ODID_ID_SIZE);
         break;
       }
       case 0x10: {  // Location message
         ODID_Location_data loc;
         decodeLocationMessage(&loc, (ODID_Location_encoded*) odid);
         UAV->lat_d = loc.Latitude;
         UAV->long_d = loc.Longitude;
         UAV->altitude_msl = (int) loc.AltitudeGeo;
         UAV->height_agl = (int) loc.Height;
         UAV->speed = (int) loc.SpeedHorizontal;
         UAV->heading = (int) loc.Direction;
         break;
       }
       case 0x40: {  // System message
         ODID_System_data sys;
         decodeSystemMessage(&sys, (ODID_System_encoded*) odid);
         UAV->base_lat_d = sys.OperatorLatitude;
         UAV->base_long_d = sys.OperatorLongitude;
         break;
       }
       case 0x50: {  // Operator ID message
         ODID_OperatorID_data op;
         decodeOperatorIDMessage(&op, (ODID_OperatorID_encoded*) odid);
         strncpy(UAV->op_id, (char*) op.OperatorId, ODID_ID_SIZE);
         break;
       }
       default:
         break;
     }
   }
 }
};

BLEScan* pBLEScan = nullptr;

// Initialize USB Serial (for JSON output) and Serial1 (for UART messages)
void initializeSerial() {
 Serial.begin(115200);
 Serial1.begin(115200, SERIAL_8N1, SERIAL1_RX_PIN, SERIAL1_TX_PIN);
 Serial.println("USB Serial (for JSON) and UART (Serial1) initialized.");
}

// Sends JSON payload (includes Remote ID data) over USB Serial.
void send_json_fast(const id_data *UAV) {
 char mac_str[18];
 snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x",
          UAV->mac[0], UAV->mac[1], UAV->mac[2],
          UAV->mac[3], UAV->mac[4], UAV->mac[5]);
 char json_msg[256];
 snprintf(json_msg, sizeof(json_msg),
   "{\"mac\":\"%s\", \"rssi\":%d, \"drone_lat\":%.6f, \"drone_long\":%.6f, \"drone_altitude\":%d, \"pilot_lat\":%.6f, \"pilot_long\":%.6f, \"basic_id\":\"%s\"}",
   mac_str, UAV->rssi, UAV->lat_d, UAV->long_d, UAV->altitude_msl,
   UAV->base_lat_d, UAV->base_long_d, UAV->uav_id);
 Serial.println(json_msg);
}

// Sends compact messages over UART (Serial1) with mesh-style output.
void print_compact_message(const id_data *UAV) {
 static unsigned long lastSendTime = 0;
 const unsigned long sendInterval = 5000;  // 5-second interval for UART messages
 const int MAX_MESH_SIZE = 230;
 
 if (millis() - lastSendTime < sendInterval)
   return;
 lastSendTime = millis();
 
 char mac_str[18];
 snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x",
          UAV->mac[0], UAV->mac[1], UAV->mac[2],
          UAV->mac[3], UAV->mac[4], UAV->mac[5]);
 
 char mesh_msg[MAX_MESH_SIZE];
 int msg_len = 0;
 msg_len += snprintf(mesh_msg + msg_len, sizeof(mesh_msg) - msg_len,
                     "Drone: %s RSSI:%d", mac_str, UAV->rssi);
 if (msg_len < MAX_MESH_SIZE && UAV->lat_d != 0.0 && UAV->long_d != 0.0) {
   msg_len += snprintf(mesh_msg + msg_len, sizeof(mesh_msg) - msg_len,
                       " https://maps.google.com/?q=%.6f,%.6f",
                       UAV->lat_d, UAV->long_d);
 }
 if (Serial1.availableForWrite() >= msg_len) {
   Serial1.println(mesh_msg);
 }
 
 delay(1000);
 if (UAV->base_lat_d != 0.0 && UAV->base_long_d != 0.0) {
   char pilot_msg[MAX_MESH_SIZE];
   int pilot_len = snprintf(pilot_msg, sizeof(pilot_msg),
                            "Pilot: https://maps.google.com/?q=%.6f,%.6f",
                            UAV->base_lat_d, UAV->base_long_d);
   if (Serial1.availableForWrite() >= pilot_len) {
     Serial1.println(pilot_msg);
   }
 }
}

// Variables for periodic heartbeat.
unsigned long last_status = 0;
unsigned long current_millis = 0;

void setup() {
 initializeSerial();
 
 Serial.println("BLE Remote ID Scanner starting with short scan intervals...");
 memset(uavs, 0, sizeof(uavs));
 
 BLEDevice::init("BLE RemoteID Scanner");
 pBLEScan = BLEDevice::getScan();
 pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
 pBLEScan->setActiveScan(true);
 pBLEScan->setInterval(100);
 pBLEScan->setWindow(99);
}

void loop() {
 current_millis = millis();
 if ((current_millis - last_status) > 60000UL) { // Every 60 seconds, send a heartbeat message
   Serial.println("{\"heartbeat\":\"Device is active and running.\"}");
   last_status = current_millis;
 }
 
 // Use a short scanning period (1 second) and then immediately restart scanning.
 BLEScanResults* foundDevices = pBLEScan->start(1, false);
 pBLEScan->clearResults();
 
 // Process any new decoded Remote ID data.
 for (int i = 0; i < MAX_UAVS; i++) {
   if (uavs[i].flag) {
     send_json_fast(&uavs[i]);         // USB Serial JSON output
     print_compact_message(&uavs[i]);    // UART (Serial1) compact message
     uavs[i].flag = 0;
   }
 }
}
