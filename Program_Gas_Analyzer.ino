#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 2

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature suhu(&oneWire);

void setup() {
  Serial.begin(9600);

  suhu.begin();
}

void loop() {
  suhu.requestTemperatures();
  float suhuC = suhu.getTempCByIndex(0);

  if(suhuC == DEVICE_DISCONNECTED_C){
    suhuC = 0;
  }

  float pembanding_suhu = 5;
  suhuC = suhuC + pembanding_suhu; // Bisa ditambah atau dikurangi

  Serial.print("Suhu Oli : ");
  Serial.println(suhuC);
}
