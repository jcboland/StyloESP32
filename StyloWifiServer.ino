#include <Preferences.h>
//#include <CRC32.h>

#include <WiFi.h>
#define TXD2 17
#define RXD2 18

#include "esp_bt_main.h"
#include "esp_bt_device.h"

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#define SERVICE_UUID        "360d5a37-0ba0-41eb-b630-447fe22ddebc"
#define CHARACTERISTIC_SSID_UUID "05767dd7-0e3b-46a0-a3c3-507f4c23f209"
#define CHARACTERISTIC_PWD_UUID "03051fc7-67df-4d3d-aad6-5149ea50adbc"
#define CHARACTERISTIC_IP_UUID "7879b3ba-a2fe-4854-bec5-0ac3ceb4328e"

BLECharacteristic *pCharacteristicSSID;
BLECharacteristic *pCharacteristicPWD;
BLECharacteristic *pCharacteristicIP;
BLEService *pService;

Preferences preferences;
const char* SSID_PREF = "SSID_PREF";
const char* PWD_PREF = "PWD_PREF";
const char* IP_PREF = "IP_PREF";

String ssid = "";
String password = "";
String ipAddress = "";

//const char* ssid     = "fourdognight";
//const char* password = "bored2018cities";
//const char* ssid     = "Door To Nowhere";
//const char* password = "goingknowhere";

WiFiServer server(1235);

#define BUFFERSIZE 100
String commandBuffer[BUFFERSIZE];
int head = 0;
int tail = 0;
int storedCommands = 0;
String currentLine = "";                // make a String to hold incoming data from the Android


void savePreference(const char* key, String value) {
  preferences.putString(key, String(value));
}

void loadPreferences() {
  preferences.begin("credentials", false);
  ssid = preferences.getString("ssid", "No SSID Preference Found").c_str();
  password = preferences.getString("password", "No Password Preference Found").c_str();
  ipAddress = preferences.getString("ipaddress", "No IP Preference Found").c_str();
  Serial.print("Loaded SSID: ");
  Serial.println(ssid);
  Serial.print("Loaded PWD: ");
  Serial.println(password);
  Serial.print("Loaded IP: ");
  Serial.println(ipAddress);
  preferences.end();
}


class BLECallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();

      BLEUUID bleUUID = pCharacteristic->getUUID();
      std::string targetUUIDStr = bleUUID.toString();

      if (targetUUIDStr == CHARACTERISTIC_SSID_UUID) {
        Serial.println("Writing to SSID UUID: ");
        preferences.begin("settings", false);
        preferences.putString(SSID_PREF, value.c_str());
        Serial.print("Saving SSID: ");
        Serial.println(preferences.getString(SSID_PREF, "No SSID Preference Found"));
        preferences.end();

      }
      else if (targetUUIDStr == CHARACTERISTIC_PWD_UUID) {
        Serial.println("Writing to PWD UUID: ");
        preferences.begin("settings", false);
        preferences.putString(PWD_PREF, value.c_str());
        Serial.print("Saving Password: ");
        Serial.println(preferences.getString(PWD_PREF, "No PWD Preference Found"));
        preferences.end();
        Serial.println("Rebooting...");
        ESP.restart();
      }
      else if (targetUUIDStr == CHARACTERISTIC_IP_UUID) {
        Serial.println("Writing to IP UUID: ");
      }
      else {
        Serial.println("Attempted to write to characteristic NOT FOUND");
      }
    }
};


void loadWIFI() {
  //  preferences.begin("settings", false);
  //  preferences.putString(SSID_PREF, "Door To Nowhere");
  //  preferences.putString(PWD_PREF, "goingknowhere");
  //  preferences.putString(IP_PREF, "10.0.10.191");
  //  preferences.end();

  preferences.begin("settings", false);
  Serial.print("Loaded SSID: ");
  Serial.println(preferences.getString(SSID_PREF, "No SSID Preference Found"));
  ssid = preferences.getString(SSID_PREF, "Default");
  Serial.print("Loaded PWD: ");
  Serial.println(preferences.getString(PWD_PREF, "No Password Preference Found"));
  password = preferences.getString(PWD_PREF, "Default");
  Serial.print("Loaded IP: ");
  Serial.println(preferences.getString(IP_PREF, "No IP Address Preference Found"));
  ipAddress = preferences.getString(IP_PREF, "Default");
  preferences.end();

  //loadPreferences();
}

void startWIFI() {
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  //WiFi.begin(ssid.c_str(), password.c_str());
  WiFi.begin(ssid.c_str(), password.c_str());
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  ipAddress = WiFi.localIP().toString();
  pCharacteristicIP->setValue(ipAddress.c_str());

  server.begin();
}

void setupSerial() {

  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  while (!Serial2) {
    delay(10);
    Serial.println("Waiting on Serial 2...");
    while (!Serial) {}
    delay(10);
    Serial.println("Waiting on Serial ...");
  }
  delay(10);
}

void setupBLE() {
  BLEDevice::init("Stylo");
  BLEServer *pServer = BLEDevice::createServer();

  pService = pServer->createService(SERVICE_UUID);

  pCharacteristicSSID = pService->createCharacteristic(
                          CHARACTERISTIC_SSID_UUID,
                          BLECharacteristic::PROPERTY_READ |
                          BLECharacteristic::PROPERTY_WRITE
                        );

  pCharacteristicPWD = pService->createCharacteristic(
                         CHARACTERISTIC_PWD_UUID,
                         BLECharacteristic::PROPERTY_READ |
                         BLECharacteristic::PROPERTY_WRITE
                       );

  pCharacteristicIP = pService->createCharacteristic(
                        CHARACTERISTIC_IP_UUID,
                        BLECharacteristic::PROPERTY_READ |
                        BLECharacteristic::PROPERTY_WRITE
                      );

  // Set callbacks for the Characteristics
  pCharacteristicSSID->setCallbacks(new BLECallbacks());
  pCharacteristicPWD->setCallbacks(new BLECallbacks());
  pCharacteristicIP->setCallbacks(new BLECallbacks());

  pCharacteristicSSID->setValue("");
  pCharacteristicPWD->setValue("");
  pCharacteristicIP->setValue("");

  pService->start();

  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();

  printDeviceAddress();
}

void setup() {
  setupSerial();
  loadWIFI();
  setupBLE();
  startWIFI();
}

void printDeviceAddress() {
  Serial.print("BLE Address: ");
  const uint8_t* point = esp_bt_dev_get_address();
  for (int i = 0; i < 6; i++) {
    char str[3];
    sprintf(str, "%02X", (int)point[i]);
    Serial.print(str);
    if (i < 5) {
      Serial.print(":");
    }
  }
}

void loop() {

  // Check if the WifiClient is available
  WiFiClient client = server.available();   // listen for incoming clients

  ////////////////////////////////////////////////
  // Check if there is anything available from the Android Tablet
  if (client) {                             // if you get a client,
    Serial.println("New Client.");           // print a message out the serial port

    
    ///////////////////////////////////////////////////////
    // Loops through this while the Android is connecte
    while (client.connected()) {            // loop while the client's connected

      // Check Android Connection
      String androidOut = checkAndroid();
      // Process incoming Android Message
      processAndroid(androidOut);

      // Process the message from Android
      String out = checkUSB();
      // Process the Android Message
      processUSB(out);
      
      // Check to see if there is a message from the BTT
      String out2 = checkBTT();
      // Process the BTT message
      processBTT(out2);

      // Check to see if there is room in the buffer
      boolean buffAvailable = checkBuffer();
      if(buffAvailable){
        requestAndroid();
      }
    }
    ///////////////////////////////////////////////////////



    // close the connection:
    client.stop();
    Serial.println("Client Disconnected.");
  }
}

void requestAndroid(){
  
  
}

boolean checkBuffer(){
  if(storedCommmands < BUFFERSIZE){
    return true;
  }
  else{
    return false;
  }
}

String checkAndroid(){
    String androidOut = "";
    // Check to see if there is something available from the network client
      while (client.available()) {
        char c = client.read();             // read a byte, then
        currentLine += c;
        // If it is the end of a command
        if (c == '\n') {
           // Set the value of the return to the current incoming string
           androidOut = currentLine;
           // Reset currentLine
           currentLine = "";
        }
      }
      
}

String checkUSB() {
  String inStr = "";
  while (Serial.available() > 0) {
    char inChar = Serial.read();
    inStr += inChar;
    if (inChar == '\n') {
      return inStr;
    }
  }
  return inStr;
}

String checkBTT() {
  String inStr = "";
  // IF there is something available at that serial connection
  while (Serial2.available() > 0) {
    char inChar = Serial2.read();
    inStr += inChar;
    if (inChar == '\n') {
      return inStr;
    }
  }
  return inStr;
}

void processBTT(String out2) {
    // Check if the message isn't empty
        if (out2 != "") {
          // Check if the command is an "ok" response
          if(out2.indexOf("ok") > -1){
            // If it is an ok command, request command from buffer

            
          }
          // Otherwise, passthrough the message to the Android
          else{ 
            // Report back what was received from the BTT to the ANDROID connection
            client.print(out2);
          }

        }
}

void processAndroid(String out) {
  // Check to see if there is a valid incoming string
  if (out != "") {
    // Add the incoming string to the Buffer
    addCommand(out);
  }
}

void processUSB(String out) {
  // Check to see if there is a valid incoming string
  if (out != "") {
    // Add the incoming string to the Buffer
    addCommand(out);
    //Serial.println("Sending to BTT: " + out);
    // Turn String into Char Array
    //char charArrOut[out.length()];
    //out.toCharArray(charArrOut, out.length());
    //client.write(charArrOut);
  }
}

// Retrieve a command from the Buffer
String getCommand() {
  // Setup string to be returned
  String newCommand = "";
  // Check if there is a String at the tail position
  if(commandBuffer[tail] != null){
    // Set new command to current tail position
    newCommand = commandBuffer[tail];
    // Set the old position to null
    commandBuffer[tail] = null;
    // Decrease the Stored Command Count
    storedCommands--;
    // Increase the tail position
    tail++;
    // Check if the tail has reached the end of the buffer
    if(tail == BUFFERSIZE){
      // Reset the tail position to the start of the buffer
      tail = 0;
    }
  }
}

// Add a command to the Buffer
void addCommand(String newCommand) {
  // Add command at the Head position
  commandBuffer[head] = newCommand;
  // Increase the Stored Command Count
  storedCommands++;
  // Move the Head position by 1
  head++;
  // Check if the head has reached the end of the Buffer
  if(head == BUFFERSIZE){
    // Reset the head position to the start of the Buffer
    head = 0;
  }
}
