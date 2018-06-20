#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <Time.h>
#include <TimeLib.h>
#include <ArduinoJson.h>

//char* PinNames[54];
String PinNames[54];
Adafruit_PCD8544 display = Adafruit_PCD8544(7, 6, 5, 3, 4);

//{"type":"pinconfig","doseTime":16000,"Alkalinity":24,"Calcium":28,"Magnesium":26,"Strotium":22}
//{"type":"dosetable","table":[[],[],[],[],[],[],["Alkalinity"],["Alkalinity","Magnesium"],["Alkalinity"],["Alkalinity"],["Alkalinity"],["Alkalinity"],["Alkalinity"],["Alkalinity"],["Alkalinity"],["Alkalinity"],[],[],["Calcium"],["Calcium"],["Strotium"],["Strotium"],[],[]]}
//{"type":"timesync","time":1529330006}

String pinjson = "{\"type\":\"pinconfig\",\"doseTime\":16000,\"Alkalinity\":[24],\"Calcium\":28,\"Magnesium\":26,\"Strotium\":22}";
String dosejson = "{\"type\":\"dosetable\",\"table\":[[],[],[],[],[],[],[\"Alkalinity\"],[\"Alkalinity\",\"Magnesium\"],[\"Alkalinity\"],[\"Alkalinity\"],[\"Alkalinity\"],[\"Alkalinity\"],[\"Alkalinity\"],[\"Alkalinity\"],[\"Alkalinity\"],[\"Alkalinity\"],[],[],[\"Calcium\"],[\"Calcium\"],[\"Strotium\"],[\"Strotium\"],[],[]]}";

//{\"type\":\"pinconfig\",\"doseTime\":16000,\"Alkalinity\":24,\"Calcium\":28,\"Magnesium\":26,\"Strotium\":22}
//{\"type\":\"dosetable\",\"table\":[[],[],[],[],[],[],[\"Alkalinity\"],[\"Alkalinity\",\"Magnesium\"],[\"Alkalinity\"],[\"Alkalinity\"],[\"Alkalinity\"],[\"Alkalinity\"],[\"Alkalinity\"],[\"Alkalinity\"],[\"Alkalinity\"],[\"Alkalinity\"],[],[],[\"Calcium\"],[\"Calcium\"],[\"Strotium\"],[\"Strotium\"],[],[]]}
//{\"type\":\"timesync\",\"time\":1529330006}

StaticJsonDocument<1000> pinDoc;
StaticJsonDocument<2000> doseDoc;

//Serial.println(pinconfig["type"].as<const char*>());
//JsonObject& pinconfig = doc.as<JsonObject>();
//deserializeJson(doc, "{\"type\":\"dosetable\",\"table\":[[],[],[],[],[],[],[\"Alkalinity\"],[\"Alkalinity\",\"Magnesium\"],[\"Alkalinity\"],[\"Alkalinity\"],[\"Alkalinity\"],[\"Alkalinity\"],[\"Alkalinity\"],[\"Alkalinity\"],[\"Alkalinity\"],[\"Alkalinity\"],[],[],[\"Calcium\"],[\"Calcium\"],[\"Strotium\"],[\"Strotium\"],[],[]]}");
//JsonObject& dosetable = doc.as<JsonObject>();

//int Action[50][2] = {{6,ALKALINITY},{7,ALKALINITY},
//                    {7,MAGNESIUM},{8,ALKALINITY},
//                    {9,ALKALINITY},{10,ALKALINITY},
//                    {11,ALKALINITY},{12,ALKALINITY},{13,ALKALINITY},{14,ALKALINITY},
//                    {15,ALKALINITY},{18,CALCIUM},{19,CALCIUM},
//                    {20,STROTIUM},{21,STROTIUM}};

int DOSE5ML;
double currentPH;
unsigned long actionMillis = 0;
unsigned long currentMillis;
boolean is_dosing = false;
boolean loaded_template = false;
int not_dosed = 1;

void setup(){
  Serial.begin(9600);
  setTime(1529330386);
  pinMode(8, OUTPUT);
  digitalWrite(8, LOW);

  deserializeJson(pinDoc, pinjson);
  deserializeJson(doseDoc, dosejson);

  JsonObject& dosetable = doseDoc.as<JsonObject>();
  JsonObject& pinconfig = pinDoc.as<JsonObject>();

  //Serial.println(table[7][0].as<String>());
  
  analogWrite(8, 20);
  display.begin();
  display.setContrast(50);
  display.display();
  delay(3000);

  DOSE5ML = pinconfig["doseTime"];
  
//  PinNames[pinconfig["Alkalinity"].as<int>] = "ALKALINITY";
//  PinNames[pinconfig["Calcium"].as<int>] = "CALCIUM";
//  PinNames[pinconfig["Magnesium"].as<int>] = "MAGNESIUM";
//  PinNames[pinconfig["Strotium"].as<int>] = "STROTIUM";
  
  pinMode(pinconfig["Alkalinity"], OUTPUT);
  digitalWrite(pinconfig["Alkalinity"], LOW);
  pinMode(pinconfig["Calcium"], OUTPUT);
  digitalWrite(pinconfig["Calcium"], LOW);
  pinMode(pinconfig["Magnesium"], OUTPUT);
  digitalWrite(pinconfig["Magnesium"], LOW);
  pinMode(pinconfig["Strotium"], OUTPUT);
  digitalWrite(pinconfig["Strotium"], LOW);
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
  } else if (root["type"] == "dosetable") {
    serializeJson(root, dosejson);
    deserializeJson(doseDoc, dosejson);
  } else if (root["type"] == "timesync") {
    //Sync time
    setTime(root["time"]);
    Serial.println(hour());
  }
}

void displayDosingTable() {
  JsonObject& dosetable = doseDoc.as<JsonObject>();
  JsonObject& pinconfig = pinDoc.as<JsonObject>();
  int loadingbarpos;
  int cursorpos = 0;
  String liquid;
  if(!loaded_template || is_dosing) {
    display.fillRect(0, 15, 84, 34, WHITE);
    for(int i=hour()+not_dosed; i<=24; i++){
      //dosetable["table"][i].size();
      //Serial.println(dosejson);
      //Serial.println(dosetable["table"][i].size());
      for(int j = 0; j < dosetable["table"][i].size(); j++){
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
        //Serial.print("dose:");
        //Serial.println(dosetable["table"][i][j].as<String>());
        display.print(dosetable["table"][i][j].as<String>());
      }
    }
  loaded_template = true;
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
  JsonObject& dosetable = doseDoc.as<JsonObject>();
  JsonObject& pinconfig = pinDoc.as<JsonObject>();
  
  if (minute() == 0 && second() == 0 && !is_dosing) {
    //dosetable["table"][hour()].size()
    for (int i = 0; i < dosetable["table"][hour()].size(); i++) {
      digitalWrite(pinconfig[dosetable["table"][hour()].as<String>()], HIGH);
      is_dosing = true;
      not_dosed = 0;
    }
    actionMillis = currentMillis;
  }

  if(currentMillis - actionMillis > DOSE5ML) {
    digitalWrite(pinconfig["Alkalinity"], LOW);
    digitalWrite(pinconfig["Calcium"], LOW);
    digitalWrite(pinconfig["Magnesium"], LOW);
    digitalWrite(pinconfig["Strotium"], LOW);
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


