/*
    This sketch establishes an MQTT connection to your broker and publishes a device with several dummy nodes and properties.
*/

#if defined(ARDUINO_ARCH_ESP8266)
#include <ESP8266WiFi.h>
#else
#include "WiFi.h"
#endif
#include <LeifHomieLib.h>
#include "config.h"

HomieDevice homie;

const char* ssid     = STASSID;
const char* password = STAPSK;

#define PUMP_OUT D1
#define FLOAT_HIGH D2
#define FLOAT_LOW D3
#define MANUAL_PUMP D4

// We'll need a place to save pointers to our created properties so that we can access them again later.
HomieProperty * pPropPump=NULL;
HomieProperty * pPropManual=NULL;
HomieProperty * pPropFloatHigh=NULL;
HomieProperty * pPropFloatLow=NULL;

void setup() {
  Serial.begin(115200);

  // We start by connecting to a WiFi network

  pinMode(PUMP_OUT, OUTPUT);
  pinMode(FLOAT_HIGH, INPUT);
  pinMode(FLOAT_LOW, INPUT);
  pinMode(MANUAL_PUMP, INPUT);

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  HomieLibRegisterDebugPrintCallback([](const char * szText){
    Serial.printf("%s",szText);
    });



  {


    HomieNode * pNode=homie.NewNode();

    pNode->strID="properties";
    pNode->strFriendlyName="Properties";
//    pNode->strType="customtype";

    HomieProperty * pProp;


    pPropPump=pProp=pNode->NewProperty();
    pProp->strFriendlyName="Pump";
    pProp->strID="pump";
    pProp->strFormat="";
    pProp->SetRetained(false);
    pProp->datatype=homieEnum;
    pProp->SetSettable(true);
    pProp->strFormat="ON,OFF";
    pProp->AddCallback([](HomieProperty * pSource) {
      //this property is settable. We'll print it into the console whenever it's updated.
      //you can set it from MQTT Explorer by publishing a number between 0-100 to homie/examplehomiedev/nodeid1/dimmer
      //but remember to check the *retain* box.
      Serial.printf("%s is now %s\n",pSource->strFriendlyName.c_str(),pSource->GetValue().c_str());
    });
    

    pPropFloatHigh=pProp=pNode->NewProperty();
    pProp->strFriendlyName="Float High";
    pProp->strID="floathigh";
    pProp->strFormat="";
    pProp->SetRetained(true);
    pProp->SetBool(false);
    pProp->datatype=homieBool;

    pPropFloatLow=pProp=pNode->NewProperty();
    pProp->strFriendlyName="Float Low";
    pProp->strID="floatlow";
    pProp->strFormat="";
    pProp->SetRetained(true);
    pProp->SetBool(false);
    pProp->datatype=homieBool;


    pPropPump=pProp=pNode->NewProperty();
    pProp->strFriendlyName="Manual";
    pProp->strID="manual";
    pProp->strFormat="";
    pProp->SetRetained(false);
    pProp->datatype=homieEnum;
    pProp->strFormat="ON,OFF";

  }

  homie.strFriendlyName="Garage Pump";
  homie.strID="garagepump";
  homie.strID.toLowerCase();

  homie.strMqttServerIP=MQTT_IP;
  homie.strMqttUserName=MQTT_USER;
  homie.strMqttPassword=MQTT_PASS;

  homie.Init();
}

unsigned long lastRun = 0;        // When was the pump last turned on
unsigned long manualPressed = 0;  // When was the manual button pressed
int coolDown = 1000*60*10;        // Only let the pump run once every 10 minutes
int maxRun = 1000*60*5;           // Run for a max of 5 minutes

int high_input = 0;
int low_input = 0;
int manual_input = 0;
int pump_state = 0;
int last_pump_state = 0;

void loop()
{
  homie.Loop();

  high_input = digitalRead(FLOAT_HIGH);
  low_input = digitalRead(FLOAT_LOW);
  manual_input = digitalRead(MANUAL_PUMP);

  // If high level float is active, and its more than 'coolDown' since
  // the last run, then set the pump to on
  if(high_input == 1 && millis() - lastRun > coolDown) 
  {
    if (pump_state == 0) {  // Only do stuff when state changes
      lastRun = millis();
      pump_state = 1;
      if(homie.IsConnected()) {
        if(pPropPump) pPropPump->SetValue("ON");
        if(pPropFloatHigh) pPropFloatHigh->SetBool(true);
        if(pPropFloatLow) pPropFloatLow->SetBool(true);
      }
    }
  }

  // If its been running for  'maxRun', then turn the pump off.
  if(millis() - lastRun > maxRun) 
  {
    pump_state = 0;
  }

  // If the lower float switch is off, then the motor should be off.
  if(low_input == 0) 
  {
    if(pump_state == 1)
    {
      if(homie.IsConnected()) {
        if(pPropFloatLow) pPropFloatLow->SetBool(false);
        if(pPropFloatLow) pPropFloatHigh->SetBool(false);
      }
    }
    pump_state = 0;
  }

  // Turn the pump on or off
  if(pump_state == 1)
  {
    digitalWrite(PUMP_OUT, HIGH);   
  } else {
    digitalWrite(PUMP_OUT, LOW);
    if(homie.IsConnected()) {
        if(pPropPump) pPropPump->SetValue("OFF");
        if(pPropFloatHigh) pPropFloatHigh->SetBool(false);
        if(pPropFloatLow) pPropFloatLow->SetBool(false);
      }
  }
}
