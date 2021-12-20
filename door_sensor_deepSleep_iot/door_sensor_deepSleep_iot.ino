#include <PubSubClient.h>
#include <ArduinoJson.h>
//#include <WiFi.h>
#include <ESP8266WiFi.h>


const char* ssid = "your wifi ssid";
const char* password = "your wifi password";

const int REED_PIN = 16; // Pin connected to reed switch


#define ORG "your ibm iot platform org id"
#define DEVICE_TYPE "door_sensor"
#define DEVICE_ID "door001"
#define TOKEN "your device auth token"

char server[] = ORG ".messaging.internetofthings.ibmcloud.com";
char topic[] = "iot-2/evt/status/fmt/json";
char authMethod[] = "use-token-auth";
char token[] = TOKEN;
char clientId[] = "d:" ORG ":" DEVICE_TYPE ":" DEVICE_ID;

const char publishTopic[] = "iot-2/evt/status/fmt/json";
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

  // Wait for serial to initialize.
  while(!Serial);
  Serial.println("Frailty Door Sensor");

  Serial.println("I'm awake.");
  Serial.println("Door sensor setup! ");
  Serial.println("State of D0: ");
  Serial.println(digitalRead(16));

  connectWIFI();
  mqttConnect();
  publishData();
  Serial.println(WiFi.status());
  delay(500);
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  delay(500);
  Serial.println(WiFi.status());
  delay(500);
  
}

void loop() {
  int proximity = digitalRead(REED_PIN); // Read the state of the switch
  if (proximity == LOW) // If the pin reads low, the switch is closed.
  {
    Serial.println("Switch closed");
    //digitalWrite(LED_PIN, HIGH); // Turn the LED on
    delay(3000);
    Serial.println("Going into deep sleep");
    ESP.deepSleep(0);
   }
   else
   {
    Serial.println("Switch opened");
    delay(3000);
    }
}

void publishData() {
 String payload = "{\"d\":{\"sensorID\":\"door001\",\"detection\":\"1\"";
  //payload += millis() / 1000;
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
  //delay(5000);
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
