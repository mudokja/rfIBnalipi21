
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "PMS.h"
#include "DHT.h"
#include <LiquidCrystal_I2C.h>


#define DHTTYPE DHT11

WiFiClient espClient;
PubSubClient client(espClient);

const char* ssid = "SK_WiFiGIGAB590_2.4G";
const char* password = "1910041560";
const char* mqtt_server = "192.168.35.185";
const char* esp32_1_topic ="webESpms";
PMS pms(Serial2);
PMS::DATA data;
String pm2_5;
String pm10;
int lcdColumns = 16;
int lcdRows = 2;
const int DHTPin = 33;
DHT dht(DHTPin, DHTTYPE);
int DHT_t;
float DHT_h;
long taskTime = millis();
long lastTaskTime = 0;
long lastPMSTime = 0;
char Jbuffer[100];

LiquidCrystal_I2C lcd(0x3F, lcdColumns, lcdRows);

void setup() {
  dht.begin();
  Serial.begin(115200);
  Serial2.begin(9600);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  lcd.init();
  lcd.backlight();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  if(!client.loop())
    client.connect("ESP32_1");
  taskTime = millis();
  if (taskTime - lastPMSTime > 100) {
    lastPMSTime = taskTime;
    lcd.clear();
    if (pms.read(data))
    {
      pm2_5 = String(data.PM_AE_UG_2_5);
      pm10 = String(data.PM_AE_UG_10_0);
      Serial.print("PM 1.0 (ug/m3): ");
      Serial.println(data.PM_AE_UG_1_0);
      Serial.print("PM 2.5 (ug/m3): ");
      Serial.println(data.PM_AE_UG_2_5);
      Serial.print("PM 10.0 (ug/m3): ");
      Serial.println(data.PM_AE_UG_10_0);
      Serial.println();
      while(Serial2.available())
      Serial2.read();
    }
    if (taskTime - lastTaskTime > 5000) {
    lastTaskTime = taskTime;
    DHT_h = dht.readHumidity();
    DHT_t = dht.readTemperature();
    float f = dht.readTemperature(true);
    Serial.print("Humidity: ");
    Serial.print(DHT_h);
    Serial.print(" %\t Temperature: ");
    Serial.print(DHT_t);
    Serial.print(" *C ");
    Serial.print(f);
    Serial.println();
    makeMQTT_JSON(lastTaskTime, DHT_t, DHT_h, pm2_5, pm10);
    }
    lcd.setCursor(0, 0);
    lcd.print("PM2.5:");
    lcd.setCursor(6, 0);
    lcd.print(pm2_5);
    lcd.setCursor(0, 1);
    lcd.print("PM 10:");
    lcd.setCursor(6, 1);
    lcd.print(pm10);
    lcd.setCursor(10, 0);
    lcd.print(DHT_t);
    lcd.setCursor(13, 0);
    lcd.print("*C");
    lcd.setCursor(10, 1);
    lcd.print(DHT_h);
    lcd.setCursor(15, 1);
    lcd.print("%");
  }
  if (taskTime > 10800000)
  {
    Serial.println("restart ESP32");
    ESP.restart();
  }
}


void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected - ESP IP address: ");
  Serial.println(WiFi.localIP());
}

void makeMQTT_JSON(long NT, float temp, float humidity, String pms2_5, String pms10) {
  
  while (!Serial) continue;
  DynamicJsonDocument doc(224);
  doc["time"]  = NT;
  doc["DHT_temp"]  = temp;
  doc["DHT_humidity"]  = humidity;
  doc["PMS_data1"]  = pms2_5;
  doc["PMS_data2"]  = pms10;
  
  int num1 = serializeJson(doc, Jbuffer);
  Serial.print("create string bytes:");
  Serial.println(num1);
  client.publish(esp32_1_topic, Jbuffer);
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32_1")) {
      Serial.println("connected");  
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println("try again after 5 seconds");
      delay(5000);
    }
  }
}
