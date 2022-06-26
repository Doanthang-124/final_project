
#include <SPI.h>
#include "RF24.h"
#include<Wire.h>
#include "MAX30100_PulseOximeter.h"

PulseOximeter pox;
const int MPU_addr = 0x68; // Dia chi I2C
int16_t AcX_raw, AcY_raw, AcZ_raw;
float AcX, AcY, AcZ;

uint8_t address[1][10] = {"gateway"}; // địa chỉ để phát
RF24 radio(9, 10);
byte msg[5];
unsigned long timeWarning = 0;
unsigned long timeRefresh = 0;

void onBeatDetected()
{
  Serial.println("B:1");
}
void setup()
{
  Serial.begin(115200);
  //==Module NRF24
  radio.begin();
  radio.setAutoAck(1);
  radio.setRetries(1, 1);
  radio.setDataRate(RF24_1MBPS);      // Tốc độ truyền
  radio.setPALevel(RF24_PA_MAX);      // Dung lượng tối đa
  radio.setChannel(10);               // Đặt kênh
  radio.openWritingPipe(address[0]);  // mở kênh
  pinMode(2, INPUT);
  pinMode(4, INPUT);
  pinMode(8, INPUT);
  msg[0] = 0;
  msg[1] = 0;
  msg[2] = 0;
  msg[3] = 0;
  msg[4] = 0;

  Wire.begin();
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);  
  Wire.write(0);     
  Wire.endTransmission(true);

  if (!pox.begin(PULSEOXIMETER_DEBUGGINGMODE_PULSEDETECT))
  {
    Serial.println("Loi ket noi");
    for (;;);
  }

  pox.setOnBeatDetectedCallback(onBeatDetected);
}
unsigned long timeCham = 0;
byte check = 0;
void loop() {
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B);  
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_addr, 6, true); // 6 thanh ghi
  AcX_raw = Wire.read() << 8 | Wire.read(); // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)
  AcY_raw = Wire.read() << 8 | Wire.read(); // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
  AcZ_raw = Wire.read() << 8 | Wire.read(); // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
  AcX = 9.8*(AcX_raw / 16384.00);
  AcY = 9.8*(AcY_raw / 16384.00);
  AcZ = 9.8*(AcZ_raw / 16384.00);
  
  delay(33);
  if (digitalRead(8) == 1) { 
    check = 1;
    timeCham = millis();
  }
  if ((unsigned long)(millis() - timeCham) > 2000) {
    check = 0;
  }
  if (check == 1) {
    if ((abs(AcX) > 15) && (abs(AcY) > 15) && (abs(AcZ) > 15)) 
    {
		  msg[2] = 1;
		  timeWarning = millis();
    }
    if ((abs(AcX) < 10) && (abs(AcY) < 10) && (abs(AcZ) < 10) && ((unsigned long)(millis() - timeWarning) > 5000)) 
    {
		  msg[2] = 0;
    }
    if ((digitalRead(2) == 1) || (digitalRead(4) == 1) || (msg[2] == 1)) 
    {
		  msg[4] = digitalRead(2);
		  msg[3] = digitalRead(4);
		  radio.write(&msg, sizeof(msg));
		  delay(500);
    }
    pox.update();
    msg[4] = digitalRead(2);
    msg[3] = digitalRead(4);
    msg[0] = pox.getHeartRate();
    msg[1] = pox.getSpO2();
    if ((unsigned long)(millis() - timeRefresh) > 600000)
    {
		radio.write(&msg, sizeof(msg));
		timeRefresh = millis();
    }
  }
}
