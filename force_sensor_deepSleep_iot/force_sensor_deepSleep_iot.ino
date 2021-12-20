#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define pressureSensorPin1 A0

extern "C" {
  #include "user_interface.h" //rtc memory read and write
}

int no_occupy_count = 0;
int occupy_count = 0;
int saved_occupy_status = 0; //1 = occupied, 0 = not occupied
int cur_status;

const char* ssid = "your wifi ssid";
const char* password = "your wifi password";

int val = 0;
int numReadings = 5;

#define ORG "your iot platform org id"
#define DEVICE_TYPE "pressure_sensor"
#define DEVICE_ID "pressure001"
#define TOKEN "your device auth token"
#define APP_ID "your app id"

char server[] = ORG ".messaging.internetofthings.ibmcloud.com";
char topic[] = "iot-2/evt/status/fmt/json";
char authMethod[] = "use-token-auth";
char token[] = TOKEN;
char clientId[] = "d:" ORG ":" DEVICE_TYPE ":" DEVICE_ID;
char clientAppId[] = "a:" ORG ":" APP_ID;
char apiKey[] = "a-" ORG "-" APP_ID;
char authToken[] = "your app auth token";

const char publishTopic[] = "iot-2/evt/status/fmt/json";
const char responseTopic[] = "iotdm-1/response";
const char manageTopic[] = "iotdevice-1/mgmt/manage";
const char updateTopic[] = "iotdm-1/device/update";
const char rebootTopic[] = "iotdm-1/mgmt/initiate/device/reboot";

const char commandTOPIC[] = "iot-2/type/speaker/id/speaker001/cmd/ask/fmt/json";

void callback(char* topic, byte* payload, unsigned int payloadLength);

WiFiClient wifiClient;
PubSubClient client(server, 1883, callback, wifiClient);

void connectWIFI() {
  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP());
}

void mqttConnect() {
 if (!!!client.connected()) {
   Serial.print("Reconnecting MQTT client to "); Serial.println(server);
   while (!!!client.connect(clientId, authMethod, token)) {
     Serial.print(".");
     delay(500);
   }
   Serial.println();
 }
}

void setup() {
  Serial.begin(115200);
  pinMode(A0, OUTPUT);

  system_rtc_mem_write(68, &saved_occupy_status, 4);

  //show last saved status upon setup
  system_rtc_mem_read(64, &saved_occupy_status, 4);
  Serial.println();
  Serial.print("Last saved occupy status is : ");
  Serial.println(saved_occupy_status);
  delay(500);
}

void loop() {
  int val = analogRead(pressureSensorPin1);
  Serial.print("mat 1:");
  Serial.println(val);

  // deciding occupancy status
  if (val < 300) {
    if(no_occupy_count > 0) no_occupy_count--;
    occupy_count++;
    if(occupy_count > 10) {
      cur_status = 1;
      Serial.println("seat occupied");
    }
  }else {
    if(occupy_count > 0) occupy_count--;
    no_occupy_count++;
    if(no_occupy_count > 10) {
      cur_status = 0;
      Serial.println("seat empty");
    }
  }

  //end of awaken period
  if(occupy_count > 10 || no_occupy_count > 10 ) {
    //send data to IoT if status changed
    if(cur_status != saved_occupy_status) {
      // status changed!
      Serial.println("status changed");
      //update saved status using current status
      saved_occupy_status = cur_status;
      Serial.print("write to RTC mem: ");
      Serial.println(saved_occupy_status);
      system_rtc_mem_write(64, &saved_occupy_status, 4);

      //send updated status to IoT
      connectWIFI();
      mqttConnect();
      publishData(cur_status);
      delay(1000);
      publishCommandApp();

      WiFi.disconnect();
      Serial.println("Wifi disconnected");
      WiFi.mode(WIFI_OFF);  
      Serial.println("Wifi OFF");
      delay(4000);
      
      //go to deep sleep
      Serial.println("going to deep sleep...");
      ESP.deepSleep(10 * 1000000);
    }else {
      //no need wifi connection to save power if no status change
      Serial.println("going to deep sleep...");
      ESP.deepSleep(10 * 1000000); 
    }
    
  }
  
  delay(500);
}


void publishData(int occupy_status) {
  StaticJsonDocument<200> payload;
  payload["d"]["sensorID"] = "001";
  payload["d"]["occupy"] = String(occupy_status);
  char jsonBuffer[512];
  serializeJson(payload, jsonBuffer); // print to client
  
  Serial.print("Sending payload: "); Serial.println(jsonBuffer);
    
  if (client.publish(topic, jsonBuffer)) {
    Serial.println("Publish data ok");
  } else {
    Serial.println("Publish data failed");
  }
  
  delay(1000);

  client.disconnect();
  Serial.println("mqtt client disconnected");
  delay(1000);
  //delay(3000);
}

void publishCommandApp() {
  if (!!!client.connected()) {
   Serial.print("Application connecting MQTT client to "); Serial.println(server);
   while (!!!client.connect(clientAppId, apiKey, authToken)) {
     Serial.print(".");
     delay(500);
   }

   StaticJsonDocument<200> payloadApp;
   payloadApp["d"]["sensorID"] = "001";
   char jsonBufferApp[512];
   serializeJson(payloadApp, jsonBufferApp); // print to client
   
   if (client.publish(commandTOPIC, jsonBufferApp)) {
    Serial.println("Publish command ok");
    } else {
    Serial.println("Publish command failed");
    }
    Serial.println();
    }
   
   client.disconnect();
   Serial.println("mqtt client disconnected");
}

void callback(char* topic, byte* payload, unsigned int payloadLength) {
  Serial.print("callback invoked for topic: "); Serial.println(topic);
  if (strcmp (responseTopic, topic) == 0) {
    return; // just print of response for now
  }
  
  if (strcmp (rebootTopic, topic) == 0) {
    Serial.println("Rebooting...");
    ESP.restart();
  }
  if (strcmp (updateTopic, topic) == 0) {
   //handleUpdate(payload);
   }
}
