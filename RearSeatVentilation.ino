#include "I2CSlave.h"

bool isDebug=true;
bool isTest=false;
int testTimer=0;

I2CSlave slave;

#define PIN_L_Control 5
#define PIN_R_Control 6

byte L_Mode=0;
byte R_Mode=0;

byte lowSpeed=125;
byte midSpeed=180;
byte highSpeed=255;

byte modeSpeed[]={0, lowSpeed, midSpeed, highSpeed};
byte modeSeq[]={0, 3, 2, 1};

void setup() {
  Serial.begin(115200);
  
  pinMode(PIN_L_Control, OUTPUT);
  pinMode(PIN_R_Control, OUTPUT);
  
  slave.onCommand(REG_PING, cmdPing);
  slave.onCommand(REG_L_MODE, cmdMode);
  slave.onCommand(REG_R_MODE, cmdMode);
  slave.onCommand(REG_L_GetStatus, cmdGetStatus);
  slave.onCommand(REG_R_GetStatus, cmdGetStatus);
  slave.begin();
}

void loop() {
  slave.process();
  if(isTest && millis()-testTimer>2000)
  {
    testTimer=millis();
    ClickHardware(0);
    ClickHardware(1);
  }

  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    command.toLowerCase();

    if (command == "mode0") {
      ClickHardware(0);
      Serial.println(L_Mode);
    } else if (command == "mode1") {
      ClickHardware(1);
      Serial.println(R_Mode);
    } else if (command == "test") {
      isTest = !isTest;
      Serial.println(isTest ? "Тест включён" : "Тест выключен");
    } else {
      Serial.println("Команды: mode0 | mode1 | test");
    }
  }
  delay(5);
}

//0-left; 1-right
void ClickHardware(int seatNum){
  if(seatNum==0)
  {
    L_Mode=GetNextMode(L_Mode);
    logI("Seat #0", L_Mode);
    analogWrite(PIN_L_Control, modeSpeed[L_Mode]);
  }
  else if(seatNum==1)
  {
    R_Mode=GetNextMode(R_Mode);
    logI("Seat #1", R_Mode);
    analogWrite(PIN_R_Control, modeSpeed[R_Mode]);
  }
}

//Возвращает номер следующего режима. modeSpeed[result] - следующая скокрость вращения
byte GetNextMode(byte mode){
  int i=0;
  while(i<4){
    if(modeSeq[i]==mode){
      break;
    }
    i++;
  }
  i++;
  if(i>3) i=0;
  return modeSeq[i];
}

byte GetIndicator(byte seatNum){
  if(seatNum==0)
  {
    logI("Seat #0", L_Mode);
    return L_Mode;
  }
  else if(seatNum==1)
  {
    logI("Seat #1", R_Mode);
    return R_Mode;
  }
  else{
    return 0;
  }
}

//I2C commands
void cmdMode(const uint8_t* buf, uint8_t len) {
  Serial.print("cmdMode ");
  if (len < 1) { slave.respondByte(0x00); return; }
  uint8_t seat = 2;
  if(buf[0]==REG_L_MODE)
    seat=0;
  if(buf[0]==REG_R_MODE)
    seat=1;
  Serial.println(seat);
  if (seat > 1) { slave.respondByte(0x00); return; }
  ClickHardware(seat);
  uint8_t ind=GetIndicator(seat);
  uint8_t resp[2] = {1, ind};
  slave.respond(resp, sizeof(resp));
}

void cmdGetStatus(const uint8_t* buf, uint8_t len) {
  Serial.print("cmdGetStatus ");
  if (len < 1) { slave.respondByte(0x00); return; }
  uint8_t seat = 2;
  if(buf[0]==REG_L_GetStatus)
    seat=0;
  if(buf[0]==REG_R_GetStatus)
    seat=1;
  Serial.println(seat);
  if (seat > 1) { slave.respondByte(0x00); return; }
  uint8_t ind=GetIndicator(seat);
  uint8_t resp[2] = {1, ind};
  slave.respond(resp, sizeof(resp));
}

void cmdPing(const uint8_t*, uint8_t) {
  slave.respondByte(0x01);
}

void logS(String str){
  if(!isDebug)
    return;
  Serial.println(str);
}

void logI(String str, int i){
  if(!isDebug)
    return;
  Serial.print(str);
  Serial.print(" : ");
  Serial.println(i);
}
