#include <DS3231.h>
#include <Stepper.h>
#include <SoftwareSerial.h>
#include<EEPROM.h>
#include <SPI.h>
#include <MFRC522.h>
#include<Servo.h>
#define SS_PIN 10 //4
#define RST_PIN 9 //5
#define servopin 6
#define buzzer 7
#define limitswitch 8
#define unlockval 0
#define lockval 180
Servo myservo;
MFRC522 mfrc522(SS_PIN, RST_PIN);
const int stepsperrevolution=90.;
Stepper frontstepper(stepsperrevolution,4,5,14,16);
Stepper backstepper(stepsperrevolution,5,4,14,16);
DS3231  rtc(SDA, SCL);
SoftwareSerial bt(3,2);
int EEPROMaddr=0;
int timestampCount=0;

struct timestamp{
    int timeval;
    int slot;
};

void gotoNext(){
  frontstepper.step(stepsperrevolution);
}
void gotoPrevious(){
  backstepper.step(stepsperrevolution);
}

void gotoSlot(int slot){
  for(int i=0;i<slot;i++){
    gotoNext();
  }
}
void gotoHome()
{
  while(digitalRead(limitswitch)==1){
    gotoPrevious();

  }
}
bool scanRFID(int startTime)
{
  int scantime=millis();
  if(scantime-startTime>3*60000){
    return false;
  }
  if ( ! mfrc522.PICC_IsNewCardPresent())
  {
    scanRFID(startTime);
  }
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial())
  {
    scanRFID(startTime);
  }
  //Show UID on serial monitor
  Serial.print("UID tag :");
  String content= "";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
     Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
     Serial.print(mfrc522.uid.uidByte[i], HEX);
     content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
     content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  Serial.println();
  Serial.print("Message : ");
  content.toUpperCase();
  if (content.substring(1) == "D9 53 E9 6E" || content.substring(1) == "19 64 C0 6E") //change here the UID of the card/cards that you want to give access
  {
    Serial.println("Authorized access");
    return true;
    Serial.println();
    delay(3000);



  }

 else   {
    Serial.println(" Access denied");
    scanRFID(startTime);
  }

}
void generateTimestamps(String rawData){
    timestamp t;
    int eeAddress=EEPROMaddr+timestampCount* (sizeof t);
    t.slot=rawData.toInt()/100000;
    t.timeval=rawData.toInt()%100000;
    EEPROM.put(eeAddress,t);
    timestampCount++;
    // Serial.print("TS");Serial.println(timestampCount);
    if(timestampCount==21)
    timestampCount=0;
}

void unlockDoor(){
  myservo.write(0);
          delay(1000);

}



void lockDoor(){
  myservo.write(150);
          delay(1000);

}



void setup() {
  frontstepper.setSpeed(60);
  backstepper.setSpeed(60);
  Serial.begin(115200);
  bt.begin(9600);
  mfrc522.PCD_Init();
  rtc.begin();
  SPI.begin();
  pinMode(buzzer,OUTPUT);
  myservo.attach(6);
  pinMode(limitswitch,INPUT_PULLUP);
}



void reset(){
//  Serial.println("Reset!");
gotoHome();
  digitalWrite(buzzer,HIGH);
  for (int i = 0 ; i < EEPROM.length() ; i++) {
    EEPROM.write(i, 0);
  }
  digitalWrite(buzzer,LOW);
}



void instructions(){
//GOTO ZERO
digitalWrite(buzzer,HIGH);
delay(2000);
digitalWrite(buzzer,LOW);
gotoHome();
int currentSlot=0;
String choice="yes";
 while(bt.available()<=0);
 while(choice.equals("Cancel")==0){
    choice=bt.readString();

    if(choice.equals("Next")&&currentSlot!=21){
    //motor to next slot
    gotoNext();
    currentSlot++;
    }
    if(choice.equals("Back")&&currentSlot!=0){
        //motor to prev slot
    gotoPrevious();
    currentSlot--;
    }
    }
    gotoHome();
}
  //GOTO ZERO




void getSchedules(){
    int i;
    int temp[21];
    String a;
    Serial.println("Entered getSchedule");
    for(i=0;i<3;i++)
    {
    digitalWrite(buzzer,HIGH);
    delay(1000);
    digitalWrite(buzzer,LOW);
    delay(1000);
    }
    while(bt.available()<=0);
    a=bt.readString();

    a=a+";";
    Serial.println(a);
    unsigned int delimiters[22];
    for(i=0;i<23;i++){
            delimiters[i]=8*i;
        }
    String substrings[21];
    for(i=0;i<21;i++){
        substrings[i]=a.substring(delimiters[i]+1,delimiters[i+1]);
        generateTimestamps(substrings[i]);
}
}

void ringalarm(){
  String dow= rtc.getDOWStr();
  String time= rtc.getTimeStr();
  int hours=time.substring(0,2).toInt();
  int minutes=time.substring(3,5).toInt();
  int dw;
  dow.toLowerCase();
  if(dow.equals("sunday"))dw=1;
  else if(dow.equals("monday"))dw=2;
  else if(dow.equals("tuesday"))dw=3;
  else if(dow.equals("wednesday"))dw=4;
  else if(dow.equals("thursday"))dw=5;
  else if(dow.equals("friday"))dw=6;
  else if(dow.equals("saturday"))dw=7;
  timestamp t;

  int currenttime= dw*1440+hours*60+minutes;
//   Serial.println(currenttime);
  for(int i=0;i<21;i++)
  {
    EEPROM.get(4*i,t);
    // Serial.println(t.timeval);
    if(t.timeval==currenttime)
    {
      digitalWrite(buzzer, HIGH);
      gotoSlot(t.slot);
      int startTime=millis();
      bool pillTaken=scanRFID(startTime);
      if(pillTaken==false){
        bt.print("MISSED!");
        digitalWrite(buzzer, LOW);
      }
      else if(pillTaken==true){
        digitalWrite(buzzer,LOW);
      // Serial.println(t.slot);
      unlockDoor();
      delay(3*60000);
      lockDoor();
      gotoHome();
}
    }
  }
}


void loop() {
//    Serial.println("LOOP!");
    ringalarm();
//     Serial.println(rtc.getTimeStr());
    String choice;
    if(bt.available()>0)
    {
        choice=bt.readString();
//        Serial.println(choice);
        if(choice.equals("getSchedules")){
            getSchedules();
        }
        if(choice.equals("instructions")){
            instructions();
        }
        if(choice.equals("reset")){
          reset();
        }
        
        if(choice.equals("Closedoor")){
          lockDoor();
         }
        if(choice.equals("Opendoor")){
        unlockDoor();

        }
    }
}
