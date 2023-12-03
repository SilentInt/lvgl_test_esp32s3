#include "CST816T.h"

#define I2C_SDA 8
#define I2C_SCL 9
#define TP_INT 38
#define TP_RST 39

CST816T touch(I2C_SDA, I2C_SCL,TP_RST,TP_INT); 

void setup() {
    Serial.begin(115200); 
    touch.begin(); 
}

bool FingerNum;
uint8_t gesture;
uint16_t touchX,touchY;
void loop() {
    FingerNum=touch.getTouch(&touchX,&touchY,&gesture);
    if(FingerNum){
              Serial.printf("X:%d,Y:%d,gesture:%x\n",touchX,touchY,gesture);
    }
    
    delay(100);
}
