#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <PMS.h>
#include <SoftwareSerial.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <PubSubClient.h>
#include <Ticker.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

SoftwareSerial pmsSerial(12,13);
ESP8266WiFiMulti wifiMulti;
Ticker ticker;
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

const char* mqttServer = "test.ranye-iot.net";
int pmat10 = 0;
int pmat25 = 0;
int pmat100 = 0;
unsigned int temperature = 0;
unsigned int humandity = 0;
int grade = 0;
int count = 0;

PMS pms(pmsSerial);
PMS::DATA pmsdata;

void setup() {
  Serial.begin(9600);
  pmsSerial.begin(9600);
  pms.passiveMode();

  displaySetUp();
  OledPrintChar("Connecting ...", true, 2,14);
  wifiSetUp();
  OledPrintChar("IP Address: ", true, 2,24);
  printIP();

  Serial.print("Waking up, Wait 30 seconds for stable readings...");
  pms.wakeUp();
  delay(30000);

  mqttClient.setServer(mqttServer, 1883);
  connectMQTTServer();
  ticker.attach(1, tickerCount);

  display.clearDisplay();
}

void loop() {
  OledPrintChar("PMS5003T",true,0,0);
  pms5003t_spec();
  isGrade();
  display2Oled();
  display.clearDisplay();

  if (mqttClient.connected()) {
    if (count >= 240) {
      pubMQTTmsg();
      count = 0;
    }
    mqttClient.loop();
  } else {
    connectMQTTServer();
  }

  delay(3000);
}

void tickerCount() {
  count++;
}

void connectMQTTServer() {
  String clientId = "esp8266-" + WiFi.macAddress();

  if (mqttClient.connect(clientId.c_str())) {
    Serial.println("MQTT Server Connected.");
    Serial.println("Server Address: ");
    Serial.println(mqttServer);
    Serial.println("ClientId: ");
    Serial.println(clientId);
  } else {
    Serial.print("MQTT Server Connect Failed. Client State: ");
    Serial.println(mqttClient.state());
    delay(3000);
  }
}

void pubMQTTmsg() {
  static int value;

  String topicString = "Taichi-Maker-Pub-" + WiFi.macAddress();
  char publishTopic[topicString.length()+1];
  strcpy(publishTopic, topicString.c_str());

  String messageString = "Hello World " + String(value++);
  char publishMsg[messageString.length() + 1];
  strcpy(publishMsg, messageString.c_str());

  if(mqttClient.publish(publishTopic, publishMsg)) {
    Serial.println("Publish Topic: "); Serial.println(publishTopic);
    Serial.println("Publish message: "); Serial.println(publishMsg);
  } else {
    Serial.println("Message Publish Failed.");
  }
}

void pms5003t_spec() {
  int count = 0;
  unsigned char c;
  unsigned char high;
  while (pmsSerial.available()) {
    c = pmsSerial.read();
    if ((count==0 && c!=0x42) || (count==1 && c!=0x4d)){
      Serial.println("Check failed");
      OledPrintChar("Failed", false, 80, 0);
      break;
    }
    if(count > 27){
      OledPrintChar("Done!", false, 80, 0);
      break;
    }
    else if(count == 10 || count == 12 || count == 14 || count == 24 || count == 26) {
      high = c;
    }
    else if(count == 11) {
      pmat10 = 256*high + c;
    }
    else if(count == 13) {
      pmat25 = 256*high + c;
    }
    else if(count == 15) {
      pmat100 = 256*high + c;
    }
    else if(count == 25) {
      temperature = ((256*high + c)/10)+5;
    }
    else if(count == 27) {
      humandity = (256*high + c)/10;
    }
    count++;
  }
  while(pmsSerial.available())
    pmsSerial.read();
  Serial.println();
}

void pms5003t_spec2() {
  Serial.println("Send read request...");
  pms.requestRead();

  Serial.println("Wait max. 1 second for read...");
  if (pms.readUntil(pmsdata)) {
    Serial.print("PM 1.0 (ug/m3): ");
    pmat10 = pmsdata.PM_AE_UG_1_0;
    Serial.println(pmsdata.PM_AE_UG_1_0);

    Serial.print("PM 2.5 (ug/m3): ");
    pmat25 = pmsdata.PM_AE_UG_2_5;
    Serial.println(pmsdata.PM_AE_UG_2_5);

    Serial.print("PM 10.0 (ug/m3): ");
    pmat100 = pmsdata.PM_AE_UG_10_0;
    Serial.println(pmsdata.PM_AE_UG_10_0);

    //Serial.print("tempture (cel.): ");
    //Serial.println(pmsdata.PM_TEMP);

    //Serial.print("humi (%): ");
    //Serial.println(pmsdata.PM_HUMI);
  } else {
    Serial.println("No data.");
  }

  //Serial.println("Going to sleep for 60 seconds.");
  //pms.sleep();
  //delay(60000);
}

void displaySetUp() {
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  display.clearDisplay();
  OledPrintChar("PMS5003T",true,0,0);
  pmsSerial.begin(9600);
}

void OledPrintChar(char *x, boolean y, int i, int l) {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(i,l);
  if(y) {
    display.print(x);   
  }
  else display.println(x);
  display.display();
}

void OledPrintInt(int x, boolean y, int i, int l) {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(i,l);
  if(y) {
    display.print(x);   
  }
  else display.println(x);
  display.display();
}

void wifiSetUp() {
  wifiMulti.addAP("fangyuyuan3", "04042325");

  int i = 0;
  while (wifiMulti.run() != WL_CONNECTED) {
    delay(1000);
    Serial.print(i++);
    Serial.print(' ');
  }

  Serial.println('\n');
  Serial.print("Connect to ");
  Serial.println(WiFi.SSID());
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());
}

void printIP() {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(2,34);
  display.println(WiFi.localIP());
  display.display();
}

void display2Oled() {
  OledPrintChar("PM1.0 =", false, 0, 14);
  OledPrintInt(pmat10, false, 54, 14);
  OledPrintChar("ug/m3", true, 80,14);

  OledPrintChar("PM2.5 =", false, 0, 24);
  OledPrintInt(pmat25, false, 54, 24);
  OledPrintChar("ug/m3", true, 80, 24);

  OledPrintChar("PM10  =", false, 0, 34);
  OledPrintInt(pmat100, false, 54, 34);
  OledPrintChar("ug/m3", true, 80, 34);

  OledPrintChar("Temp  =", false, 0, 44);
  OledPrintInt(temperature, false, 54, 44);
  OledPrintChar("Cel", true, 80, 44);

  OledPrintChar("humi  =", false, 0, 54);
  OledPrintInt(humandity, false, 54, 54);
  OledPrintChar("%", true, 80, 54);
}

void isGrade(){
  if(pmat25 < 35) {
    grade= 1;
    OledPrintChar("1", false, 60, 0);
  }
  else if(pmat25 >= 35 && pmat25 < 75) {
    grade = 2;
    OledPrintChar("2", false, 60, 0);
  }
  else if(pmat25 >= 75 && pmat25 < 115) {
    grade = 3;
    OledPrintChar("3", false, 60, 0);
  }
  else if(pmat25 >= 115 && pmat25 < 150) {
    grade = 4;
    OledPrintChar("4", false, 60, 0);
  }
  else if(pmat25 >= 150 && pmat25 < 250) {
    grade = 5;
    OledPrintChar("5", false, 60, 0);
  }
  else if(pmat25 >= 250) {
    grade = 6;
    OledPrintChar("6", false, 60, 0);
  }
}
