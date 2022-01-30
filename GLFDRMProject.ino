#include <DHT.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>

#define DHTPIN 2
#define DHTTYPE DHT11

int buzz = 3;
int gasRelay = 6;
int button = 5;
int waterRelay = 4;
int ledPin = 10;
int mq6A = A1;

int gasThreshold = 300;
int tempThreshold = 38;

boolean waterValveState = false;
boolean gasValveState = true;
boolean smsSent = false;
boolean callSent = false;
boolean buttonState = false;

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);
SoftwareSerial mySerial(8, 7);

void setup() {
  Serial.begin(9600);
  mySerial.begin(9600);
  Serial.println("Initializing");
  pinMode(gasRelay, OUTPUT);
  pinMode(waterRelay, OUTPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(mq6A, INPUT);
  pinMode(button, INPUT);
  pinMode(buzz, OUTPUT);
  lcd.init();
  lcd.begin(16, 2);
  lcd.backlight();
  lcd.print("Initializing....");
  dht.begin();
  delay(1000);//To allow warming up of the dht sensor.
}

void loop() {
  int gasValue = getGasValue();
  int h = getHum();
  int t = getTemp();

  Serial.println(gasValue);
  lcdInfo(h, t, gasValue, waterValveState, gasValveState);
  if (gasValue > gasThreshold && t > tempThreshold) {
    //Possibility of a fire.
    String fireDanger = "Fire";
    !buttonState ? lcdDanger(fireDanger) : lcdInfo(h, t, gasValue, waterValveState, gasValveState);
    waterValveOn();
    gasValveOff();
    ledBuzzOn();
    gsmfunc(fireDanger);
    return;
  } else if ( gasValue > gasThreshold && t < tempThreshold) {
    //Possibility of a gas leak
    String gasDanger = "Gas";
    !buttonState ? lcdDanger(gasDanger) : lcdInfo(h, t, gasValue, waterValveState, gasValveState);
    waterValveOff();
    gasValveOff();
    ledBuzzOn();
    gsmfunc(gasDanger);
    return;
  } else if (gasValue < gasThreshold && t > tempThreshold) {
    String tempDanger = "Extreme temperatures";
    !buttonState ? lcdDanger(tempDanger) : lcdInfo(h, t, gasValue, waterValveState, gasValveState);
    waterValveOff();
    gasValveOff();
    ledBuzzOn();
    gsmfunc(tempDanger);
    return;
  }
  gasValveOn();
  waterValveOff();
  ledBuzzOff();
  delay(1000);
}

int getGasValue() {
  return analogRead(mq6A);
}
int getTemp() {
  Serial.println(dht.readTemperature());
  return dht.readTemperature();

}
int getHum() {
  return dht.readHumidity();

}
void gsmfunc(String dangertop) {
  if (!smsSent) {
    gsmSend(dangertop);
    delay(1000);
    gsmCall();
    lcd.setCursor(0,1);
    lcd.print("SMS & CALL sucess");
    delay(1000);
  } else {
    Serial.println("Communicated");
    delay(1000);
  }

}
void gsmSend(String danger) {
    Serial.println("Sending sms");
    mySerial.println("AT"); //Once the handshake test is successful, it will back to OK
    updateSerial();
    delay(500);
    mySerial.println("AT+CMGF=1"); // Configuring TEXT mode
    updateSerial();
    delay(500);
    mySerial.println("AT+CMGS=\"+254790137621\"");//change ZZ with country code and xxxxxxxxxxx with phone number to sms
    updateSerial();
    delay(500);
    mySerial.print( danger + " detected. Taking necessary system risk mitigation protocols. You are encouraged to act first and help secure the facility.");
    updateSerial();
    delay(500);
    mySerial.write(26);
    updateSerial();
    delay(500);
    //calls immediately after Sms.
    delay(1000);
}
void gsmCall() {
  Serial.println("Calling");
  mySerial.println("AT+CFUN=1,1");
  updateSerial();
  mySerial.println("AT");
  updateSerial();
  mySerial.println("ATD+ +254790137621;");
  updateSerial();
  delay(20000); // wait for 20 seconds...
  mySerial.println("ATH"); //hang up
  updateSerial();
  smsSent = true;
}
void updateSerial() {
  delay(500);
  while (Serial.available())
  {
    mySerial.write(Serial.read());//Forward what Serial received to Software Serial Port
  }
  while (mySerial.available())
  {
    Serial.write(mySerial.read());//Forward what Software Serial received to Serial Port
  }
}
void lcdInfo(float h, float t, float gasValue, boolean water, boolean gas) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("HU TE GAS WV GV");
  lcd.setCursor(0, 1);
  lcd.print(String(int(h)) + " " + String(int(t)) + " " + String(int(gasValue))+ " " + (water ? "ON" : "OF") + " " + (gas ? "ON" : "OF"));
}

void lcdDanger(String danger) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(danger + " detected");
  lcd.setCursor(0, 1);
  lcd.print("Starting mitigation");
}
void ledBuzzOn() {
  digitalWrite(ledPin, HIGH);
  digitalWrite(buzz, HIGH);
}
void ledBuzzOff() {
  digitalWrite(ledPin, LOW);
  digitalWrite(buzz, LOW);
}
void gasValveOn() {
  digitalWrite(gasRelay, HIGH);
  gasValveState = true;
}
void gasValveOff() {
  digitalWrite(gasRelay, LOW);
  gasValveState = false;
}
void waterValveOn() {
  digitalWrite(waterRelay, HIGH);
  waterValveState = true;
}
void waterValveOff() {
  digitalWrite(waterRelay, LOW);
  waterValveState = false;
}
void buttonPressCheck() {
  buttonState = digitalRead(button);
}

