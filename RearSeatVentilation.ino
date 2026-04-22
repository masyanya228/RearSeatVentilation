#include "I2CSlave.h"

bool isDebug=true;
bool isTest=false;
int testTimer=0;

I2CSlave slave;
void SaveError(uint8_t code);

#define PIN_L_Control 5
#define PIN_R_Control 6

byte L_Mode=0;
byte R_Mode=0;

byte lowSpeed=125;
byte midSpeed=180;
byte highSpeed=255;

byte modeSpeed[]={0, lowSpeed, midSpeed, highSpeed};
byte modeSeq[]={0, 3, 2, 1};

//Ошибки в памяти
struct Error{
  uint8_t code=0;
  uint32_t tfs=0;
  uint8_t times=0;
}__attribute__((packed));
Error errors[1];
int sizeErr;
int errLen;
int nextError=0;

struct ErrorDesc {
    uint8_t code;
    const char* description;
};
const ErrorDesc errorDescriptions[] PROGMEM = {
    {21,   "Smth went wrong"},
    {0,   ""}   // terminator (обязательно в конце!)
};

void setup() {
  Serial.begin(115200);
  InitEEPROM();
  
  pinMode(PIN_L_Control, OUTPUT);
  pinMode(PIN_R_Control, OUTPUT);
  
  slave.onCommand(REG_PING, cmdPing);
  slave.onCommand(REG_GetErrorCount, cmdGetErrorCount);
  slave.onCommand(REG_GetNextError, cmdGetError);
  slave.onCommand(REG_ClearErrors, cmdClearErrors);
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
    } else if (command == "eeprom init") {
      Serial.println("Сброс памяти к заводским настройкам...");
      FirstInit();
      LoadErrors();
      Serial.println("Готово");
    } else if (command == "eeprom read") {
      Serial.println("Вывод содержимого памяти...");
      LoadErrors();
      for(int i=0;i<errLen;i++)
      {
        Serial.print("#");
        Serial.print(errors[i].code);
        Serial.print("|");
        Serial.print(errors[i].times);
        Serial.print("|");
        Serial.println(errors[i].tfs);
      }
      Serial.println("Готово");
    } else {
      Serial.println("Команды: mode0 | mode1 | test | eeprom init | eeprom read");
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
  if (len < 2) { slave.respondByte(0x00); return; }
  uint8_t seat = buf[1];
  if (seat > 1) { slave.respondByte(0x00); return; }
  Serial.println(seat);
  ClickHardware(seat);
  uint8_t ind=GetIndicator(seat);
  uint8_t resp[2] = {1, ind};
  slave.respond(resp, sizeof(resp));
}

void cmdGetStatus(const uint8_t* buf, uint8_t len) {
  Serial.print("cmdGetStatus ");
  uint8_t seat = buf[1];
  Serial.println(seat);
  uint8_t ind=GetIndicator(seat);
  uint8_t resp[2] = {1, ind};
  slave.respond(resp, sizeof(resp));
}

void cmdPing(const uint8_t*, uint8_t) {
  slave.respondByte(0x01);
}

void cmdGetErrorCount(const uint8_t*, uint8_t) {
  Serial.print("cmdGetErrorCount: ");
  uint8_t count = 0;
  for (uint8_t i = 0; i < errLen; i++)
      if (errors[i].times > 0) count++;
  uint8_t resp[2]={1, count};
  Serial.println(count);
  slave.respond(resp, sizeof(resp));
}

void cmdGetError(const uint8_t* buf, uint8_t len) {
  Serial.print("getError #");
  uint8_t index = (len >= 2) ? buf[1] : 0;
  Serial.println(index);
  uint8_t found = 0;
  for (uint8_t i = 0; i < errLen; i++) {
    if (errors[i].times == 0) continue;
    if (found++ == index) {
      Serial.print("Code: ");
      Serial.println(errors[i].code);
      uint8_t resp[7];
      resp[0] = 1;
      resp[1] = errors[i].code;
      memcpy(&resp[2], &errors[i].tfs, 4);
      resp[6] = errors[i].times;
      slave.respond(resp, 7);
      return;
    }
  }
  uint8_t resp[7] = {};
  slave.respond(resp, 7);
}

void cmdClearErrors(const uint8_t*, uint8_t) {
  Serial.println("cmdClearErrors");
  memset(errors, 0, sizeof(errors));
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
