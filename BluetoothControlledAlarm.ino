/* Alarm System Project
   Kevin Chan
   CS-122A Project
   
   Components 
   LCD
   Speaker
   1 LED
   7 Seg LED
   Arduino (duh)
   Bluetooth modual
   A bluetooth sending device, ie phone, tablet
   
   Basics: Shows you an alarm system, that you can lock, and unlock, and
   you can put it in Alarm mode. If you put it in alarm mode, you need to
   unlock it as soon as it starts beeping, otherwise, it continues to beep.
   
   Anything else can be found at the Github.
   Github Repo: 
   
 */
#include <SoftwareSerial.h>
#include <LiquidCrystal595.h>
#include <Agenda.h>
 
#define LATCH 2
#define DATA 3
#define SCLK 4
#define ALARMSOUNDER 6
#define TX 10
#define RX 11
#define LEDFLASHER 13

// Hex values for U, L, A, S
// U stands for Unlock Mode
// L stands for Lock Mode
// A stands for Alarm Mode
// S stands for Alarm Sound Mode (plays the Alarm sound)

char sevenSegValues[4] = {0x3E, // U
                          0x38, // L
                          0x77, // A
                          0x6D  // S, it looks like a 5. :/
};

//Liquid Crystal Setup
LiquidCrystal595 lcd(7,8,9);

//Bluetooth Setup
SoftwareSerial bt(TX, RX);
char signal; // Stores input received from Bluetooth.
// Creates a serial port/UART
// Connect BT module TX to port 10
// Connect BT modele RX to port 11

//SynchSM Setup!
Agenda scheduler;
char PasswordCorrect = 0;
char value = 0;
// This sets up the schedule to allow for polling every so often. 
// I used this to go down to the millionth second.

// FUNction Time
void unlock(){
  bt.println("Alarm is unlocked.");
  bt.println("Press 1 to lock.");
  bt.println("Press 2 to set Alarm.");
  
  lcd.setCursor(0,0);
  lcd.print("LCD AlarmStat");
  lcd.setCursor(0,1);
  lcd.println("State: Unlocked");
  
  digitalWrite(LATCH, LOW);
  shiftOut(DATA, SCLK, MSBFIRST, sevenSegValues[0]); 
  digitalWrite(LATCH, HIGH);
  
  digitalWrite(LEDFLASHER, 0);
  analogWrite(ALARMSOUNDER, 0);
}

void lock(){
  bt.println("Alarm is locked.");
  bt.println("Press 0 to unlock.");
  bt.println("Press 2 to set Alarm.");
  
  lcd.setCursor(0,0);
  lcd.print("LCD AlarmStat");
  lcd.setCursor(0,1);
  lcd.println("State: Locked");
  
  digitalWrite(LATCH, LOW);
  shiftOut(DATA, SCLK, MSBFIRST, sevenSegValues[1]); 
  digitalWrite(LATCH, HIGH);
  
  digitalWrite(LEDFLASHER, HIGH);
  analogWrite(ALARMSOUNDER, 0);
}

void alarm(){
  bt.println("Alarm is alarmed.");
  bt.println("Press 'u' to unlock.");
  bt.println("Pressing anything else will cause the alarm to sound.");
  
  lcd.setCursor(0,0);
  lcd.print("LCD AlarmStat");
  lcd.setCursor(0,1);
  lcd.println("State: Alarmed");
  
  digitalWrite(LATCH, LOW);
  shiftOut(DATA, SCLK, MSBFIRST, sevenSegValues[2]); 
  digitalWrite(LATCH, HIGH);
  
  digitalWrite(LEDFLASHER, HIGH);
  analogWrite(ALARMSOUNDER, 0);
}

void Alarm(){
  int i;
  for(i = 0; i < 255; i = i + 2)
        {
            analogWrite(LEDFLASHER, i);
            analogWrite(ALARMSOUNDER, i);
            delay(10);
        }
  for(i = 255; i > 1; i = i - 2)
        {
            analogWrite(LEDFLASHER, i);
            analogWrite(ALARMSOUNDER, i);
            delay(5);
        }
  for(i = 1; i <= 10; i++)
        {
            analogWrite(LEDFLASHER, 255);
            analogWrite(ALARMSOUNDER, 200);
            delay(100);
            analogWrite(LEDFLASHER, 1);
            analogWrite(ALARMSOUNDER, 25);
            delay(100);
        }
}

void alarmSound(){
  bt.println("ALARMED");
  bt.println("Enter the correct password to unlock: ");
  
  lcd.setCursor(0,0);
  lcd.println("SOUNDING ALARM");
  lcd.setCursor(0,1);
  lcd.println("CALLED 911");
  
  digitalWrite(LATCH, LOW);
  shiftOut(DATA, SCLK, MSBFIRST, sevenSegValues[3]); 
  digitalWrite(LATCH, HIGH);
  Alarm();
  
}

void tryingToUnlock(){
  char password = 0;
  bt.println("Enter the correct password: ");
  while(bt.available() > 0){
    password = bt.read();
    if (password == 'a'){
      PasswordCorrect = 1;
      bt.println("Unlocked");
      bt.println("Press any button to continue.");
      break;
    }
    else{
      PasswordCorrect = 0;
      bt.println("Incorrect");
      break;
    }
  }
  
}

enum States {Unlock, Lock, AlarmState, AlarmSound, SoftUnlock};
int state;
void Tick_Fck(){
  //Transitions
  switch (state) {
    case Unlock:
        if (value == '1'){
           state = Lock;
           break;
        }
        else if (value == '2'){
           state = AlarmState;
           break;
        }
    case Lock:
        if (value == '0'){
            state = Unlock;
            break;
        }
        else if (value == '3'){
            state = AlarmState;
            break;
        }
    case AlarmState:
        if (value == 'u'){
          state = SoftUnlock;
          break;
        }
        else if (value != 'u'){
          state = AlarmSound;
          break;
        }
        
    case AlarmSound:
        if (PasswordCorrect == 1){
          state = Unlock;
          break;
        }
        else{
          state = AlarmSound;
          break;
        }
    case SoftUnlock:
        if (PasswordCorrect == 1){
          state = Unlock;
          break;
        }
        else{
          state = AlarmSound;
          break;
        }
    default:
        state = Unlock;
        break;
    }
    
    //Actions
    switch (state) {
      case Unlock:
         unlock();
         break;
    case Lock:
        lock();
        break;
    case AlarmState:
        alarm();
        break;
    case AlarmSound:
        alarmSound();
        tryingToUnlock();
        break;
    case SoftUnlock:
        tryingToUnlock();
        break;
    default:
        state = Unlock;
        break;
    }
      
}

enum ReadStates {Wait, Read, Send};
int readState;
void read_Tick_Fck(){
  char receivedValue = 0;
  //Transitions
  switch (readState) {
    case Wait:
        if (bt.available() > 0 ){
          state = Read;
          break;
        }
    case Read:
        state = Send;
        break;
    case Send:
        state = Wait;
        break;
    default:
        state = Wait;
        break;
    }
    
    //Actions
    switch (state) {
      case Wait:
         break;
         
    case Read:
        value = bt.read();
        break;
        
    case Send:
        receivedValue = value;
        bt.println("Received value: " + receivedValue);
        break;
        
    default:
        state = Wait;
        break;
    }
      
}

void setup() {
  // put your setup code here, to run once:
  
  pinMode(SCLK,OUTPUT);
  pinMode(DATA, OUTPUT);
  pinMode(LATCH,OUTPUT);
  pinMode(LEDFLASHER, OUTPUT);
  pinMode(ALARMSOUNDER, OUTPUT);
  
  bt.begin(9600);
  bt.println("Hello from AlarmStat");
  bt.println("Press '0' to unlock and start your Alarm system!");
  
  lcd.begin(16,2);
  lcd.print("LCD AlarmStat");
  lcd.setCursor(0, 1);
  lcd.print("says Hello");

  digitalWrite(LATCH, LOW);
  shiftOut(DATA, SCLK, MSBFIRST, sevenSegValues[0]); // Starts with Unlocked
  digitalWrite(LATCH, HIGH);

}
//int ReadTickFck = scheduler.insert(read_Tick_Fck, 5000);
int TickFck = scheduler.insert(Tick_Fck, 1000000);

void loop() {
  // put your main code here, to run repeatedly:
  while (bt.available() > 0) {
    value = bt.read();
    scheduler.update();
  }
}
