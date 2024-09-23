#include <OneWire.h>
#include <DallasTemperature.h>
#include <MQUnifiedsensor.h>

#define ONE_WIRE_BUS 2

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature suhu(&oneWire);

#define board              "Arduino UNO"
#define resolusi_tegangan  5             // Dalam Volt
#define pin_MQ7            A1            //Analog input 1 of your arduino
#define type               "MQ-7"        //MQ7
#define ADC_Bit_Resolution 10            // For arduino UNO/MEGA/NANO (Dalam BIT)
#define RatioMQ7CleanAir   27.5          //RS / R0 = 27.5 ppm (RS dibagi R0)

MQUnifiedsensor MQ7(board, resolusi_tegangan, ADC_Bit_Resolution, pin_MQ7, type);

unsigned long t_baca_MQ7 = 0;

float data_CO = 0;

void setup() {
  Serial.begin(9600);

  suhu.begin();

  MQ7.setRegressionMethod(1); //_PPM =  a*ratio^b
  MQ7.setA(99.042);
  MQ7.setB(-1.518);

  /*
    Exponential regression:
  GAS     | a      | b
  H2      | 69.014  | -1.374
  LPG     | 700000000 | -7.703
  CH4     | 60000000000000 | -10.54
  CO      | 99.042 | -1.518
  Alcohol | 40000000000000000 | -12.35
  */

  MQ7.init(); 
  //MQ7.setRL(10);

  Serial.print("Calibrating sensor MQ7 please wait.");
  float calcR0 = 0;
  for(int i = 1; i <= 10; i++) {
    MQ7.update(); 
    calcR0 += MQ7.calibrate(RatioMQ7CleanAir);
    Serial.print(".");
  }
  MQ7.setR0(calcR0/10);
  Serial.println("  MQ7 done!.");

  if(isinf(calcR0)) {
    Serial.println("Warning: Conection issue, R0 is infinite (Open circuit detected) please check your wiring and supply");
    while(1);
  }
  
  if(calcR0 == 0){
    Serial.println("Warning: Conection issue found, R0 is zero (Analog pin shorts to ground) please check your wiring and supply");
    while(1);
  }

  //MQ7.serialDebug(true);
}

void loop() {
  suhu.requestTemperatures();
  float suhuC = suhu.getTempCByIndex(0);

  if(suhuC == DEVICE_DISCONNECTED_C){
    suhuC = 0;
  }

  float pembanding_suhu = 5;
  suhuC = suhuC + pembanding_suhu; // Bisa ditambah atau dikurangi

  if(millis() - t_baca_MQ7 <= (60*1000)) { // Waktu baca 60 detik atau 1 menit) -> bisa diganti
    MQ7.update();
    data_CO = MQ7.readSensor();
    //MQ7.serialDebug();
    t_baca_MQ7 = millis();
  }
  
  Serial.print("Suhu Oli : ");
  Serial.println(suhuC);
  Serial.print("Kadar CO : ");
  Serial.println(data_CO);
}
