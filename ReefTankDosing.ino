#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <Time.h>
#include <TimeLib.h>
#include <ArduinoJson.h>

Adafruit_PCD8544 display = Adafruit_PCD8544(7, 6, 5, 3, 4);

//{"type":"pinconfig","doseTime":16000,"pins":{"ALKALINITY":24,"CALCIUM":28,"MAGNESIUM":26,"STROTIUM":22}}
//{"type":"dosetable","table":[[],[],[],[],[],[],["ALKALINITY"],["ALKALINITY","MAGNESIUM"],["ALKALINITY"],["ALKALINITY"],["ALKALINITY"],["ALKALINITY"],["ALKALINITY"],["ALKALINITY"],["ALKALINITY"],["ALKALINITY"],[],[],["CALCIUM"],["CALCIUM"],["STROTIUM"],["STROTIUM"],[],[]]}
//{"type":"timesync","time":1529330006}

String pinjson = "{\"type\":\"pinconfig\",\"doseTime\":16000,\"ALKALINITY\":[24],\"CALCIUM\":28,\"MAGNESIUM\":26,\"STROTIUM\":22}";
String dosejson = "{\"type\":\"dosetable\",\"table\":[[],[],[],[],[],[],[\"ALKALINITY\"],[\"ALKALINITY\",\"MAGNESIUM\"],[\"ALKALINITY\"],[\"ALKALINITY\"],[\"ALKALINITY\"],[\"ALKALINITY\"],[\"ALKALINITY\"],[\"ALKALINITY\"],[\"ALKALINITY\"],[\"ALKALINITY\"],[],[],[\"CALCIUM\"],[\"CALCIUM\"],[\"STROTIUM\"],[\"STROTIUM\"],[],[]]}";

//{\"type\":\"pinconfig\",\"doseTime\":16000,\"pins\":{\"ALKALINITY\":24,\"CALCIUM\":28,\"MAGNESIUM\":26,\"STROTIUM\":22}}
//{\"type\":\"dosetable\",\"table\":[[],[],[],[],[],[],[\"ALKALINITY\"],[\"ALKALINITY\",\"MAGNESIUM\"],[\"ALKALINITY\"],[\"ALKALINITY\"],[\"ALKALINITY\"],[\"ALKALINITY\"],[\"ALKALINITY\"],[\"ALKALINITY\"],[\"ALKALINITY\"],[\"ALKALINITY\"],[],[],[\"CALCIUM\"],[\"CALCIUM\"],[\"STROTIUM\"],[\"STROTIUM\"],[],[]]}
//{\"type\":\"timesync\",\"time\":1529330006}

StaticJsonDocument<1000> pinDoc;
StaticJsonDocument<2000> doseDoc;

int DOSE5ML;
double currentPH;
unsigned long actionMillis = 0;
unsigned long currentMillis;
boolean is_dosing = false;
boolean loaded_table = false;
int not_dosed = 1;

void setup(){
  Serial.begin(9600);
  setTime(1529330386);
  pinMode(8, OUTPUT);
  digitalWrite(8, LOW);

  deserializeJson(pinDoc, pinjson);
  deserializeJson(doseDoc, dosejson);

  JsonObject& pinconfig = pinDoc.as<JsonObject>();
  JsonObject& pins = pinconfig["pins"];
  
  analogWrite(8, 20);
  display.begin();
  display.setContrast(50);
  display.display();
  delay(3000);

  DOSE5ML = pinconfig["doseTime"];

  for (const auto &pin : pins){
    pinMode(pin.value, OUTPUT);
    digitalWrite(pin.value, LOW);
  }
}

void serialEvent() {
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, Serial);

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }

  JsonObject& root = doc.as<JsonObject>();

  if (root["type"] == "pinconfig") {
    serializeJson(root, pinjson);
    deserializeJson(pinDoc, pinjson);
    loaded_table = false;
  } else if (root["type"] == "dosetable") {
    serializeJson(root, dosejson);
    deserializeJson(doseDoc, dosejson);
    loaded_table = false;
  } else if (root["type"] == "timesync") {
    //Sync time
    setTime(root["time"]);
    Serial.println(hour());
    loaded_table = false;
  }
}

void displayDosingTable() {
  JsonArray& dosetable = doseDoc.as<JsonObject>()["table"];
  JsonObject& pins = pinDoc.as<JsonObject>()["pins"];
  int loadingbarpos;
  int cursorpos = 0;
  String liquid;
  if(!loaded_table || is_dosing) {
    display.fillRect(0, 15, 84, 34, WHITE);
    for(int i=hour()+not_dosed; i<=24; i++){
      for(int j = 0; j < dosetable[i].size(); j++){
        cursorpos += 8;
        display.drawRect(0, cursorpos+7, 84, 9, BLACK);
        display.setCursor(4,cursorpos+8);
        if(i == hour()){
          loadingbarpos = double((currentMillis-actionMillis))/DOSE5ML*84;
          //Serial.println(loadingbarpos);
          display.fillRect(0, cursorpos+7, loadingbarpos, 9, BLACK);
          display.setTextColor(WHITE);
        }else{
          display.setTextColor(BLACK);
        }
        display.print(i);
        display.print(":");
        display.print(dosetable[i][j].as<String>());
      }
    }
  loaded_table = true;
  }
}

void displayDateTime() {
  display.fillRect(0, 0, 84, 7 , WHITE);
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0,0);
  digitalDateDisplay();
  display.setCursor(34,0);
  digitalClockDisplay();
}

void displayPH() {
  if(currentMillis%100 == 0) {
    currentPH = getCurrentAvgPH();
    display.fillRect(0, 7, 84, 8, WHITE);
    display.setCursor(17,7);
    display.print("pH: ");
    display.print(currentPH);
  }
}

void checkDosing() {
  JsonArray& dosetable = doseDoc.as<JsonObject>()["table"];
  JsonObject& pins = pinDoc.as<JsonObject>()["pins"];
  
  if (minute() == 0 && second() == 0 && !is_dosing) {
    //dosetable[hour()].size()
    for (int i = 0; i < dosetable[hour()].size(); i++) {
      digitalWrite(pins[dosetable[hour()].as<String>()], HIGH);
      is_dosing = true;
      not_dosed = 0;
    }
    actionMillis = currentMillis;
  }

  if(currentMillis - actionMillis > DOSE5ML) {
    for (const auto &pin : pins){
      digitalWrite(pin.value, LOW);
    }
    is_dosing = false;
  }
}

void loop() {
  currentMillis = millis();
  
  if(timeStatus()!= timeNotSet) {
    displayDosingTable();
    displayDateTime();
    displayPH();
    display.display();
    
    checkDosing();
  }
}


