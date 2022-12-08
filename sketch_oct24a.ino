//************************** ライブラリ ***************//

#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoJson.h>

int bps = 9600;

//##################################################################################
//-----------  300オーム未満の抵抗は使用しないでください    ------------
//##################################################################################

int R1 = 1000; //
int Ra = 25; //Arduino ピンの抵抗

//*****************************ピン  *****************************//
const int ECPin = A0;
const int ECGround = A1;
const int ECPower = A4;

#define ONE_WIRE_BUS 10 //DS18B20の黄色の線が刺さるピン
#define SENSER_BIT    9

//***************************** 温度補償 *****************************//
//溶液の導電率は液温によって変化します。
//そのため、25℃の導電率に補正した値を求めるようにしています。
float TemperatureCoef = 0.019;

//***************************** セル定数 *****************************//
//液体中に電極を差し込み、測定する場合はセル定数が必要です。
//1cm2の断面積と距離1cmの空間を切り出す場合は、センサのセル定数は1/cmです。
//日本国内で出回るプラグに合わせました。（穴があるタイプ）
float K = 2.8;
//***************************** ユーザーの変更可能部分 終了 *****************************//

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DynamicJsonDocument doc(1024);

float Temperature = 10;
float EC = 0;
float EC25 = 0;

float raw = 0;
float Vin = 5;
float Vdrop = 0;
float Rc = 0;

byte data = 0; // serialの出力受け取り


//*********************************以下よりプログラム開始******************************************************//
void setup()
{
  Serial.begin(bps);
  sensors.setResolution(SENSER_BIT);

  pinMode(ECPin, INPUT);
  pinMode(ECPower, OUTPUT); 
  digitalWrite(ECPower, LOW); 
  pinMode(ECGround, OUTPUT); 
  digitalWrite(ECGround, LOW);

  delay(100);// 待機時間
  sensors.begin();
  delay(100);
  
  R1 = (R1 + Ra); // Taking into acount Powering Pin Resitance
  delay(1000);
}

void loop()
{
  if(Serial.available() > 0) {
    data = (byte)Serial.read();
    if (data == 49) {
      GetEC();
      PrintReadings();
    } 
  }
}

void GetEC() {
  sensors.requestTemperatures();
  Temperature = sensors.getTempCByIndex(0);

  //***************** 電圧降下測定 **************************//
  digitalWrite(ECPower,HIGH);
  raw = analogRead(ECPin);
  raw = analogRead(ECPin); // これはミスではありません
  digitalWrite(ECPower,LOW);

  //***************** ECに変換 **************************//
  Vdrop = (Vin * raw) / 1024.0;
  Rc = (Vdrop * R1) / (Vin - Vdrop);
  Rc = Rc - Ra; //acounting for Digital Pin Resitance
  EC = 1000 / (Rc * K);

  //*************温度補償********************//
  EC25  =  EC / (1 + TemperatureCoef * (Temperature - 25.0));
  delay(5000);//プローブの損傷を防ぐため、5000より少ない値にしないでください。
}

void PrintReadings() {
  doc["EC"] = EC25;
  doc["WaterTemp"] = Temperature;
  serializeJson(doc, Serial);
  Serial.println("");
}
