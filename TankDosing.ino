#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <Time.h>
#include <TimeLib.h>
#include <ArduinoJson.h>

#define DOSE5ML 16000

#define ALKALINITY 24
#define CALCIUM    28
#define MAGNESIUM  26
#define STROTIUM   22

//char* PinNames[54];
String PinNames[54];
Adafruit_PCD8544 display = Adafruit_PCD8544(7, 6, 5, 3, 4);

int Action[50][2] = {{6,ALKALINITY},{7,ALKALINITY},
                    {7,MAGNESIUM},{8,ALKALINITY},
                    {9,ALKALINITY},{10,ALKALINITY},
                    {11,ALKALINITY},{12,ALKALINITY},{13,ALKALINITY},{14,ALKALINITY},
                    {15,ALKALINITY},{18,CALCIUM},{19,CALCIUM},
                    {20,STROTIUM},{21,STROTIUM}};

double currentPH;
unsigned long actionMillis = 0;
unsigned long currentMillis;
boolean is_dosing = false;
boolean loaded_template = false;
int not_dosed = 1;

void setup(){
  Serial.begin(9600);
  setTime(1529330006);
  pinMode(8, OUTPUT);
  digitalWrite(8, LOW);
  
  analogWrite(8, 20);
  display.begin();
  display.setContrast(60);
  display.display();
  delay(3000);
  
  PinNames[ALKALINITY] = "ALKALINITY";
  PinNames[CALCIUM] = "CALCIUM";
  PinNames[MAGNESIUM] = "MAGNESIUM";
  PinNames[STROTIUM] = "STROTIUM";
  
  pinMode(ALKALINITY, OUTPUT);
  digitalWrite(ALKALINITY, LOW);
  pinMode(CALCIUM, OUTPUT);
  digitalWrite(CALCIUM, LOW);
  pinMode(MAGNESIUM, OUTPUT);
  digitalWrite(MAGNESIUM, LOW);
  pinMode(STROTIUM, OUTPUT);
  digitalWrite(STROTIUM, LOW);
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

  //{"type":"pinconfig","doseTime":16000,"Alkalinity":24,"Calcium":28,"Magnesium":26,"Strotium":22}
  //{"type":"dosetable","table":[[],[],[],[],[],[],["Alkalinity"],["Alkalinity","Magnesium"],["Alkalinity"],["Alkalinity"],["Alkalinity"],["Alkalinity"],["Alkalinity"],["Alkalinity"],["Alkalinity"],["Alkalinity"],[],[],["Calcium"],["Calcium"],["Strotium"],["Strotium"],[],[]]}
  //{"type":"timesync","time":1529330006}
  
  if (root["type"] == "pinconfig") {
    //Configure pins
  } else if (root["type"] == "dosetable") {
    //Configure dosing schedule
  } else if (root["type"] == "timesync") {
    //Sync time
    setTime(root["time"]);
    Serial.println(hour());
  }
}

void displayDosingTable() {
  int loadingbarpos;
  int cursorpos = 0;
  if(!loaded_template || is_dosing) {
    display.fillRect(0, 15, 84, 34, WHITE);
    for(int i=hour()+not_dosed; i<=24; i++){
      for(int j=0; j<50; j++){
        if(Action[j][0] == i and Action[j][1] != 0){
          cursorpos += 8;
          display.drawRect(0, cursorpos+7, 84, 9, BLACK);
          display.setCursor(4,cursorpos+8);
          if(i == hour()){
            loadingbarpos = double((currentMillis-actionMillis))/DOSE5ML*84;
            Serial.println(loadingbarpos);
            display.fillRect(0, cursorpos+7, loadingbarpos, 9, BLACK);
            display.setTextColor(WHITE);
          }else{
            display.setTextColor(BLACK);
          }
          display.print(Action[j][0]);
          display.print(":");
          display.print(PinNames[Action[j][1]]);
        }
      }
    }
  loaded_template = true;
  }
}

void displayDateTime() {
  display.fillRect(0, 0, 84, 6, WHITE);
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
    display.fillRect(0, 7, 84, 15, WHITE);
    display.setCursor(17,7);
    display.print("pH: ");
    display.print(currentPH);
  }
}

void checkDosing() {
  if((currentMillis - actionMillis > 60000) and (minute() == 0)){
    for(int i=0; i<50; i++){
      if(Action[i][0] == hour() and Action[i][0] !=0){
        digitalWrite(Action[i][1], HIGH);
        is_dosing = true;
        not_dosed = 0;
      }
    }
    actionMillis = currentMillis;
  }

  if(currentMillis - actionMillis > DOSE5ML) {
    digitalWrite(ALKALINITY, LOW);
    digitalWrite(CALCIUM, LOW);
    digitalWrite(MAGNESIUM, LOW);
    digitalWrite(STROTIUM, LOW);
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


