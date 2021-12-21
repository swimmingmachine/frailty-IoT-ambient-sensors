#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

const char* ssid = "your ssid wifi";
const char* password = "your wifi password";

float voltage = 0;

#define ORG "x9zu8t"
#define DEVICE_TYPE "motion_sensor"
#define DEVICE_ID "kitchen"
#define TOKEN "iot platform auth"

char server[] = ORG ".messaging.internetofthings.ibmcloud.com";
char topic[] = "iot-2/evt/status/fmt/json";
char authMethod[] = "use-token-auth";
char token[] = TOKEN;
char clientId[] = "d:" ORG ":" DEVICE_TYPE ":" DEVICE_ID;

const char call[] = "iot-2/evt/status/fmt/json";
const char responseTopic[] = "iotdm-1/response";
const char manageTopic[] = "iotdevice-1/mgmt/manage";
const char updateTopic[] = "iotdm-1/device/update";
const char rebootTopic[] = "iotdm-1/mgmt/initiate/device/reboot";

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
  Serial.setTimeout(2000);

  // Wait for serial to initialize.
  while(!Serial) { }

  Serial.println("Motion detected! ");
  Serial.println("I'm awake.");

  // read battery voltage 
  int sensorValue = analogRead(A0);
  // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 3.2V):
  voltage = sensorValue * (4.2 / 1023.0);
  Serial.println(voltage);

  //connect wifi and publish data
  connectWIFI();
  mqttConnect();
  publishData();

  Serial.println("Going into deep sleep");
  ESP.deepSleep(0);
}

void loop() {
  
}

void publishData() {
  String payload = "{\"d\":{\"sensorID\": \"kitchen\",\"detection\": \"1\",";
  payload += "\"voltage\":" + String(voltage);
  payload += "}}";
  
  Serial.print("Sending payload: "); Serial.println(payload);
    
  if (client.publish(topic, (char*) payload.c_str())) {
    Serial.println("Publish ok");
  } else {
    Serial.println("Publish failed");
  }

  delay(1000);
  WiFi.disconnect();
  Serial.println("Wifi disconnected");
  WiFi.mode(WIFI_OFF);  
  Serial.println("Wifi OFF");
  delay(4000);
}


void callback(char* topic, byte* payload, unsigned int payloadLength) {
 Serial.print("callback invoked for topic: "); Serial.println(topic);

 if (strcmp (responseTopic, topic) == 0) {
   Serial.println("response topic received...");
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
