#include <Wire.h>

bool isDebug=false;
bool isTest=true;

// адрес
#define SLAVE_ADDR 20
// команды
#define REG_L_MODE 0x01
#define REG_L_GetStatus 0x02
#define REG_R_MODE 0x03
#define REG_R_GetStatus 0x04
#define REG_GetErrorCount 0x05
#define REG_GetNextError 0x06

#define PIN_L_Control A6
#define PIN_R_Control A7

int L_Mode=0;
int R_Mode=0;

byte lowSpeed=125;
byte midSpeed=180;
byte highSpeed=255;

byte modeSpeed[]={0, lowSpeed, midSpeed, highSpeed};
byte modeSeq[]={0, 3, 2, 1};

// последняя выбранная команда
// в обработчике приёма
uint8_t cmd = 0;
// счётчик сообщений
uint8_t counter = 0;

// обработчик приёма
void receiveCb(int amount) {
    cmd = Wire.read();
    ++counter;

    switch (cmd) {
        case REG_L_MODE:
            ClickHardware(0);
            break;

        case REG_R_MODE:
            ClickHardware(1);
            break;

        case REG_L_GetStatus: break;
        case REG_R_GetStatus: break;
    }
}

// обработчик запроса
void requestCb() {
    switch (cmd) {
        case REG_L_MODE: 
        case REG_L_GetStatus:
          Wire.write(GetIndicator(0));
          break;
        case REG_R_MODE:
        case REG_R_GetStatus:
          Wire.write(GetIndicator(1));
          break;
    }
}

void setup() {
  pinMode(PIN_L_Control, OUTPUT);
  pinMode(PIN_R_Control, OUTPUT);
  Serial.begin(9600);
  Wire.begin(SLAVE_ADDR);

  Wire.onReceive(receiveCb);
  Wire.onRequest(requestCb);
}

void loop() {
  if(isTest)
  {
    delay(2000);
    ClickHardware(0);
    ClickHardware(1);
  }
}

void CatchErrors(){
  
}

void SaveError(){
  
}

//0-left; 1-right
void ClickHardware(int seatNum){
  if(seatNum==0)
  {
    L_Mode=GetNextMode(L_Mode);
    logI("Seat #"+seatNum, L_Mode);
    analogWrite(PIN_L_Control, modeSpeed[L_Mode]);
  }
  else if(seatNum==1)
  {
    R_Mode=GetNextMode(R_Mode);
    logI("Seat #"+seatNum, R_Mode);
    analogWrite(PIN_R_Control, modeSpeed[R_Mode]);
  }
}

//Возвращает номер следующего режима. modeSpeed[result] - следующая скокрость вращения
int GetNextMode(int mode){
  int i=0;
  while(i<4){
    if(modeSeq[i]==mode){
      break;
    }
  }
  i++;
  if(i>3) i=0;
  return modeSeq[i];
}

int GetIndicator(int seatNum){
  if(seatNum==0)
  {
    logI("Seat #"+seatNum, L_Mode);
    return L_Mode;
  }
  else if(seatNum==1)
  {
    logI("Seat #"+seatNum, R_Mode);
    return R_Mode;
  }
}

void logI(String str, int i){
  if(!isDebug)
    return;
  Serial.print(str);
  Serial.print(" : ");
  Serial.println(i);
}
