// wifi
#include <WiFi.h>
#include <HTTPClient.h>
#include <PubSubClient.h>

// dht11
#include "DHT.h"
#define DHTPIN 13     // Digital pin connected to the DHT sensor GPIO13
#define DHTTYPE DHT11 // DHT 11

// lcd
#include <TFT_eSPI.h>
#include <SPI.h>

#define TFT_BLACK 0x0000
#define TFT_WHITE 0xFFFF
#define TFT_BLUE 0xF800

TFT_eSPI tft = TFT_eSPI();

// relay
#define AIRCONR 33
#define HEATERR 14

int intervalPassed = 0;
const int tempInterval = 2;
float tempC;
float appTemp;

// Set GPIOs for LED and PIR Motion Sensor
const int led = 26;
const int motionSensor = 27;

// Timer: Auxiliary variables
unsigned long now = millis();
unsigned long lastTrigger = 0;
boolean startTimer = false;
boolean motionSensed = false;

DHT dht(DHTPIN, DHTTYPE);

bool aircon = false;
bool heater = false;

float setTemp = 25;


// http
HTTPClient http;

//temp interval

// mqtt interval
int mqttInterval = 2, mqttNow, mqttLastUpdate;

// mqtt
#define MQTT_USER "******"
#define MQTT_PASS "*******"
#define MQTT_ID "******"
#define MQTT_URL "***********"
#define MQTT_PORT ****

const char *recievingTopic = "*******";
const char *sendingTopic = "********";

char mqttBuffer[4096];
char mqttFormat[] = "{\"tempC\":%.2f,\"tempeF\":%.2f,\"humidity\":%.2f,\"aircon\":%d,\"heater\":%d,\"motion\":%d}";


//Checks if motion was detected, sets LED HIGH and starts a timer
void IRAM_ATTR changeInMovement()
{
  digitalWrite(led, HIGH);
  lastTrigger = millis();

}



void connecttowifi() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin("user", "pass");
    Serial.print("Connecting");
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(100);
      Serial.print(".");
    }
    Serial.println("");
    Serial.print("Connected to WiFi network with IP Address: ");
    Serial.println(WiFi.localIP());
  }
  else {
    Serial.print("meow");
  }

}

void callback(char *topic, uint8_t *payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String msgTmp = "";
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
    msgTmp += (char)payload[i];
  }
  setTemp = msgTmp.toFloat();
  appTemp = msgTmp.toFloat();
  //  lastTrigger = millis();
  //  intervalPassed = 0;
  Serial.println();
}

WiFiClient wifiClient;
PubSubClient client(wifiClient);




//mqtt
void setup_mqtt_client()
{

  while (!client.connected())
  {
    if (client.connect(MQTT_ID, MQTT_USER, MQTT_PASS))
    {
      Serial.println("connected to mqtt");
      bool ok = client.subscribe(recievingTopic);
      if (!ok)
      {
        Serial.println("Can't connect!");
      }
      Serial.printf("subscribed to topic: %s\n", recievingTopic);

      break;
    }
    Serial.println("failed to connect");
    Serial.println(client.state());
    delay(5000);
  }
}



// setp
void setup()
{

  // Serial port for debugging purposes
  Serial.begin(115200);

  // dht11
  Serial.println(F("DHT11 start!"));
  dht.begin();


  // Setup Wifi
  WiFi.begin("user", "pass");
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());


  // mqtt
  client.setServer(MQTT_URL, MQTT_PORT);
  client.setCallback(callback);
  client.setBufferSize(4096);
  setup_mqtt_client();

  //  PIR Motion Sensor mode INPUT_PULLUP
  pinMode(motionSensor, INPUT_PULLUP);
  // Set motionSensor pin as interrupt, assign interrupt function and set RISING mode
  attachInterrupt(digitalPinToInterrupt(motionSensor), changeInMovement, CHANGE);
  // Set LED to LOW
  pinMode(led, OUTPUT);
  digitalWrite(led, LOW);

  // relay
  pinMode(AIRCONR, OUTPUT);
  pinMode(HEATERR, OUTPUT);
  digitalWrite(HEATERR, HIGH);
  digitalWrite(AIRCONR, HIGH);

  // HTTP Request
  mqttLastUpdate = millis();

  // lcd
  tft.init();
  tft.setRotation(3); // portrait
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_BLUE);
  tft.setTextSize(1);
}

// loop
void loop()
{

  tft.setCursor(0, 15);
  delay(10000);


  int isMotion = digitalRead(motionSensor);
  if (isMotion) {
    digitalWrite(led, HIGH);
    motionSensed = true;
    intervalPassed = 0;
    lastTrigger = millis();
    setTemp = appTemp;
  } else {
    motionSensed = false;
    digitalWrite(led, LOW);

  }



  float humidity = dht.readHumidity();
  // Read temperature as Celsius (the default)
  tempC = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float tempF = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(humidity) || isnan(tempC) || isnan(tempF))
  {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(tempF, humidity);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(tempC, humidity, false);


  // update lcd
  tft.fillScreen(TFT_BLACK);
  tft.println(F("ROOM TEMPERATURE:"));
  tft.println(F(""));
  tft.print(tempC);
  tft.println(F("°C"));
  tft.print(tempF);
  tft.print(F("°F"));

  // pir
  int nooow = millis();
  int temp = nooow - lastTrigger;

  bool shouldChange;

  if (!motionSensed && (temp > 5000)) {
    lastTrigger = millis();
    intervalPassed += 1;
    if (intervalPassed >= 5) {
      // turn off heater and aircon
      digitalWrite(HEATERR, HIGH);
      digitalWrite(AIRCONR, HIGH);
      aircon = false;
      heater = false;
      Serial.println(F("Cooler & Heater off "));
    }
    // bring setTemp closer to current temp
    if (abs(setTemp - tempC) > tempInterval) {
      if (setTemp > tempC) {
        setTemp -= 1;
      } else if (setTemp < tempC) {
        setTemp += 1;
      }
    }


    Serial.print(F("New setTemp: "));
    Serial.println(setTemp);
  }

  shouldChange = abs(setTemp - tempC) > tempInterval;

  if (shouldChange && (setTemp - tempC > 0)) {
    heater = true;
    aircon = false;
    digitalWrite(HEATERR, LOW);
    digitalWrite(AIRCONR, HIGH);
    Serial.print(F("Heater on "));
  }
  else if (shouldChange && (setTemp - tempC < 0)) {
    heater = false;
    aircon = true;
    digitalWrite(AIRCONR, LOW);
    digitalWrite(HEATERR, HIGH);
    Serial.print(F("Cooler on "));
  }


  if (client.connected())
    client.loop();
  else {
    connecttowifi();
    setup_mqtt_client();
  }

  // publish every mqttNow secs
  mqttNow = millis();
  if (mqttNow - mqttLastUpdate >= mqttInterval)
  {
    // update temperature on app
    sprintf(mqttBuffer, mqttFormat, tempC, tempF, humidity, aircon, heater, motionSensed);

    client.publish(sendingTopic, mqttBuffer);
  }


}
