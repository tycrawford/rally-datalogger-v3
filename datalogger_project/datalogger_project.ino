#include <SPI.h>
#include <stdio.h>
#include <SD.h>
#include <time.h>
#include <DS1302.h>
#include <FreqMeasure.h>
#include <LiquidCrystal.h>

const int dataLogOn = 0; //simulates logging switch
const int empty1 = 1;
const int empty2 = 2;
const int empty3 = 3;
const int chipSelect = 4;
const int empty8 = 8;
const int empty9 = 9;
const int analogOilIn = 9;
const int empty10 = 10;
const int tpsPin = 12;
const int contrastPin = 22; //red
const int datalogOnPin = 23; //org
const int kCePin = 33; //Chip enable/RST
const int kIoPin = 34; //Input/Output /DAT
const int kSclkPin = 35; //Serial Clock /CLK

const int tachIn = 49; //yel
const int contrastVal = 20;
LiquidCrystal lcd(24, 25, 26, 27, 28, 29);
//gry,pur,org,yel,grn,blu
float rpm;
int count;
int sum;

byte therm[] = {
  B00100,
  B01110,
  B01010,
  B01010,
  B01110,
  B11111,
  B11111,
  B01110
};
byte degreeSign[] = {
  B00100,
  B01010,
  B00100,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000
};

int thermChar = 0;
int degreeChar = 1;


DS1302 rtc(kCePin, kIoPin, kSclkPin);

//SD Card uses 4 pins, evidently, 11, 12, 13 and 4, all digital
//an UNO still has many leftover, along with analogs

bool secondIsEven() {
  Time t = rtc.time();
  return (t.sec % 2 == 0);
}

String getTime() {
  Time t = rtc.time();
  char buf[50];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
           t.yr, t.mon, t.date,
           t.hr, t.min, t.sec);
  return buf;
}

String dateTimeFileName() {
  Time t = rtc.time();
  char buf[50];
  snprintf(buf, sizeof(buf), "%04d%02d%02d%",
           t.yr, t.mon, t.date,
           t.hr, t.min, t.sec);
  return buf;
}
String getLcdTime() {
  Time t = rtc.time();
  char buf[16];
  snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
           t.hr, t.min, t.sec);
  return buf;
}
int year = rtc.time().yr;
int month = rtc.time().mon;
int day = rtc.time().date;

int oilPressureRaw = 0;
float oilPressureVolt;
float oilPressurePSI;
String oilPressureString;

int tps1024;
String tpsString;

int datalogOnRead = 0;
char fileName[] = "00000000.csv";

void setup() {
  pinMode(contrastPin, OUTPUT);
  pinMode(analogOilIn, INPUT);
  pinMode(datalogOnPin, INPUT);
  pinMode(tpsPin, INPUT);
  
  Serial.begin(57600);
  while (!Serial) {
    ;
  }
  sprintf(fileName, "%04d%02d%02d", year, month, day);
  FreqMeasure.begin();
  // put your setup code here, to run once:
  Serial.print("Initializing SD card...");
  lcd.begin(16, 2);
  lcd.createChar(0, therm);
  lcd.createChar(1, degreeSign);
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    while (1);
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  String dataString = "";
  analogWrite(contrastPin, contrastVal);
  if (FreqMeasure.available()) {
    rpm = 30 * FreqMeasure.countToFrequency(FreqMeasure.read());
  }
  datalogOnRead = digitalRead(datalogOnPin);

  tps1024 = analogRead(tpsPin);
  tpsString = String(round((0.113664780058651 * tps1024) - 4.651163));
  if (tpsString.length() < 3) {
    tpsString = " " + tpsString;
  } else if (tpsString.length() < 2) {
    tpsString = "  " + tpsString;
  }


  
  int oilPressureRaw = 0;
  oilPressureRaw = analogRead(analogOilIn);
  oilPressureVolt = oilPressureRaw * (5.0/1023.0);
  oilPressurePSI = 3.42771 - (0.020759 * oilPressureVolt);
  oilPressureString = String(round(oilPressurePSI));
  if (oilPressurePSI < 100) {
    oilPressureString = " " + oilPressureString;
  }
  if (oilPressurePSI < 10) {
    oilPressureString = " " + oilPressureString;
  }
  
  File dataFile = SD.open(fileName, FILE_WRITE);
  bool dataFileWasGood = dataFile;
  if (dataFile) {
    if (datalogOnRead == HIGH) {
      Serial.println(rpm);  
      dataString = getTime() + "," + rpm + "," + tps1024 + "," + oilPressureRaw;
      dataFile.println(dataString);
      Serial.println(getLcdTime());
    }
    dataFile.close();
  }
  else {
    Serial.println("error opening file");
  };
  lcd.setCursor(0, 0);
  lcd.print(getLcdTime());
  if (!dataFileWasGood){
      lcd.setCursor(9, 0);
      lcd.print(" ERROR!");
  } else{
    if (datalogOnRead){
      lcd.setCursor(9,0);
      lcd.print("LOGGING");
    } else {
      lcd.setCursor(9,0);
      lcd.print(" Ready!");
    }
  }
  if (secondIsEven()){
    //even seconds should display
    //time
    //rpm 
    //temp
    //card status/log status
    lcd.setCursor(0,1);
    lcd.print("0000");
    String rpmOutput = String(round(rpm)) + "RPM   ";
    int rpmStart = (10 - rpmOutput.length());
    lcd.setCursor(rpmStart, 1);
    lcd.print(rpmOutput);
    lcd.setCursor(10, 1);
    lcd.write(thermChar);
    lcd.setCursor(11,1);
    lcd.print("XXX");
    lcd.setCursor(14,1);
    lcd.write(degreeChar);
    lcd.setCursor(15,1);
    lcd.print(" ");

  } else {
    //odd seconds should display
    //time
    //oil
    //throttle?
    //card status
    lcd.setCursor(0, 1);
    String oilOutput = "OIL:" + oilPressureString;
    String tpsOutput = " TPS:" + tpsString + "%";
    lcd.print(oilOutput);
    lcd.setCursor(7,1);
    lcd.print(tpsOutput);
  }

  

  delay(100);
}
