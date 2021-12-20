#include <HX711.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

const char* ssid = "your wifi ssid";
const char* password = "your wifi password";

// Scale Settings
const int LED_PIN = 13;

#define SCALE_DOUT_PIN  D5
#define SCALE_SCK_PIN  D6
#define ORG "your iot platform org id"
#define DEVICE_TYPE "Scale"
#define DEVICE_ID "scale001"
#define TOKEN "your auth token"

long scaleTick;

HX711 scale;

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
  pinMode(LED_PIN, OUTPUT);
  scale.begin(SCALE_DOUT_PIN, SCALE_SCK_PIN);
  //scale.set_scale(1580354 / 67.3);// <- set here calibration factor!!!
  scale.set_scale(23238.91);// <- set here calibration factor!!!
  scale.tare();
}

void loop() {

  static float history[4];
  static float ave = 0;
  static bool stable = false;
  static float weightData = 0.0;
  static bool published = false;

  Serial.print("one reading:\t");
  float weight = scale.get_units();
  Serial.println(String(weight, 2));
  
  Serial.print("\t| average:\t");
  Serial.println(scale.get_units(5), 1);


  if (stable && !published)
  {
    Serial.println("publish to IoT");
    //connect wifi and publish data
    connectWIFI();
    mqttConnect();
    publishData(ave);
    
    published = true;
  }else if(published == true && stable == false){
    published = false;
  }

  // Check the weight measurement every 250ms
  if (millis() - scaleTick > 250)
  {
    scaleTick = millis();
    // We take the abs of the data we get from the load cell because different
    //  scales may behave differently, yielding negative numbers instead of 
    //  positive as the weight increases. This can be handled in hardware by
    //  switching the A+ and A- wires, OR we can do this and never worry about it.
    weightData = abs(weight);
    // Calculate our running average
    history[3] = history[2];
    history[2] = history[1];
    history[1] = history[0];
    history[0] = weightData;
    ave = (history[0] + history[1] + history[2] + history[3])/4;

    // IF the average differs from the current by less than 0.3lbs AND
    //  the average weight is greater than 30 pounds AND
    //  we haven't recently updated the website, set the stable flag so
    //  we know that the weight is stable and can be reported to the web
    if ((abs(ave - weightData) < 0.1) && (ave > 30))
    {
      stable = true;
      digitalWrite(LED_PIN, HIGH);
      Serial.println("stable reading achieved\t");
    }else{
      stable = false;
      digitalWrite(LED_PIN, LOW);
    }
  }
  
  // put the ADC in sleep mode
  //scale.power_down(); 
  delay(500);
  //scale.power_up();
}

void publishData(float weightAvg) {
  String payload = "{\"d\":{\"sensorID\": \"001\",\"weight\":" +  String(weightAvg);
  //payload += "\"voltage\":" + String(weightAvg);
  payload += "}}";
  
  Serial.print("Sending payload: "); Serial.println(payload);
    
  if (client.publish(topic, (char*) payload.c_str())) {
    Serial.println("Publish ok");
  } else {
    Serial.println("Publish failed");
  }

  delay(5000);
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
