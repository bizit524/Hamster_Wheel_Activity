
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
//#include <WiFiUdp.h>
#include <WiFiManager.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
 


//setup for time tracking and timestamps
#include <NTPClient.h>
#define NTP_OFFSET   0      // In seconds
#define NTP_INTERVAL 60 * 10000    // In miliseconds
#define NTP_ADDRESS  "pool.ntp.org"
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_ADDRESS, NTP_OFFSET, NTP_INTERVAL);

// Update these with values suitable for your network.
const char* mqtt_server = "192.168.20.240";
const char* topic = "Tracker1";    // this is the [root topic]
const char* starttopic = "Online"; 
long timeBetweenMessages = 5000;

//define variables 
int rotations =0;
int maxspeed = 0;
int minspeed = 0;
int avgspeed = 0;
//diameter of inside of wheel
int wheeldiamter = 293;
int distance = 0;
int period = 10000;
unsigned long time_now=0;
int startup = 0;


int sprintstart = 0;
unsigned long sprintstartTime;
unsigned long sprintendTime;
unsigned long sprintduration;

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
int value = 0;
//define reed switch
int ReedState = 0;
int LastReedState = 0;
const int  ReedPin = D6;  
String composeClientID() {
  uint8_t mac[6];
  String clientId;
  clientId += "esp-";
  return clientId;
}


void setup() {
  WiFiManager wifiManager;
  wifiManager.autoConnect("Tracker");
  Serial.begin(115200);
  Serial.println("Serial Started");
  // We start by connecting to a WiFi network

  client.setServer(mqtt_server, 1883);
    //setup reed switch need to pull down 
    pinMode(ReedPin, INPUT_PULLUP);
    timeClient.begin();
}

void loop() {
  // confirm still connected to mqtt server

  if (!client.connected()) {
    reconnect();
  }
    client.loop();
  timeClient.update();
  if ( startup == 0)
  {// this is to make sure it connects by publishing a topic else the reconnect will sit 
    startup ++;
    String payload = "ESP";
    String pubTopic;
    
    pubTopic += starttopic;
    client.publish( (char*) pubTopic.c_str() , (char*) payload.c_str(), true );
    Serial.println(pubTopic);
    Serial.println(payload);
  }
  //setup pin 
    ReedState = digitalRead(ReedPin);
    long now = millis();

    //check if wheel is running if it is increment rotations

       if (ReedState != LastReedState )
       {
       
          if (ReedState == LOW)
          { Serial.println("Wheel Cycle");
            rotations ++;
          sprintendTime = millis();
          if (sprintstart == 0)
          {
          //start sprinttimer
          sprintstartTime = millis();  
          sprintstart = 1;
          Serial.println("Sprint Started");
          }        
            //reset counter
            lastMsg = now;
          sprintendTime = millis();  
           }
         delay(50);
        
        }
        LastReedState = ReedState;
      //if there has been movement and it is after 10 seconds of no activity then publish results 
     if (rotations != 0)
     {
        sprintduration = sprintendTime - sprintstartTime;
        Serial.println("Sprint Duration");
        Serial.println(sprintduration);
        if (now - lastMsg > timeBetweenMessages ) {
          distance = rotations * (wheeldiamter*3.14);
          String formattedTime = timeClient.getFormattedTime();
          lastMsg = now;
          ++value;
          //restart sprint start
          sprintstart = 0;
          String payload = "{\"rotations\":";
          payload += rotations;
          payload += ",\"Average speed\":";
          payload += avgspeed;
          payload += ",\"Max Speed\":";
          payload += maxspeed;
          payload += ",\"Min Speed\":";
          payload += minspeed;            
          payload += ",\"distance\":";
          payload += distance;
          payload += ",\"timestamp\":";
          payload += '"';          
          payload += formattedTime;   
          payload += '"'; 
          payload += ",\"sprinttime\":";
          payload += sprintduration;                                   
          payload += "}";
          String pubTopic;
           pubTopic += topic  ;
          Serial.print("Publish topic: ");
          Serial.println(pubTopic);
          Serial.print("Publish message: ");
          Serial.println(payload);
          client.publish( (char*) pubTopic.c_str() , (char*) payload.c_str(), true );
          distance = 0;
          rotations = 0;
          avgspeed = 0;  
          minspeed =0;
          maxspeed =0; 
      }
     }
}


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");

    String clientId = "Trackers" ;


    // Attempt to connect
    if (client.connect(clientId.c_str())){
      Serial.println("MQTT Connect"); 
      Serial.print(WiFi.status());
      Serial.print(client.state());
       } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.print(" wifi=");
      Serial.print(WiFi.status());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
