#include <OneWire.h>            // https://github.com/PaulStoffregen/OneWire
#include <DallasTemperature.h>  // https://github.com/milesburton/Arduino-Temperature-Control-Library
#include <MQUnifiedsensor.h>    // https://github.com/miguel5612/MQSensorsLib/tree/master
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>  // https://github.com/fdebrabander/Arduino-LiquidCrystal-I2C-library

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define ONE_WIRE_BUS 2

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature suhu(&oneWire);

#define board              "Arduino UNO"
#define resolusi_tegangan  5             // Dalam Volt
#define ADC_Bit_Resolution 10            // For arduino UNO/MEGA/NANO (Dalam BIT)

#define pin_MQ7           A1      //Analog input 1 of your arduino
#define type_MQ7          "MQ-7"  //MQ7
#define RatioMQ7CleanAir  27.5    //RS / R0 = 27.5 ppm (RS dibagi R0)

MQUnifiedsensor MQ7(board, resolusi_tegangan, ADC_Bit_Resolution, pin_MQ7, type_MQ7);

#define pin_MQ6           A0      //Analog input 0 of your arduino
#define type_MQ6          "MQ-6"  //MQ6
#define RatioMQ6CleanAir  10      //RS / R0 = 10 ppm 

MQUnifiedsensor MQ6(board, resolusi_tegangan, ADC_Bit_Resolution, pin_MQ6, type_MQ6);

unsigned long t_baca_MQ7 = 0;
unsigned long t_baca_MQ6 = 0;

float data_CO = 0.0;
float data_HC = 0.0;

// Catatan :
// Range Sensor MQ7 (CO) adalah 20 sampai 2000 ppm berdasarkan datasheet

float persen_CO = 0.0;

#define led_buzz 13
int nyala = LOW;

unsigned long t_db = millis();
#define debounce (millis() - t_db >= 300)

#define tombol 3 // Ganti tombol switch ke pin berapa
byte menu   = 1;
String mode = "";

float batas_co = 0.0;
int batas_hc   = 0;
String status_lcd = "";

unsigned long t_debug = millis();
unsigned long t_notif = millis();
unsigned long t_lcd   = millis();

void setup() {
  Serial.begin(9600);

  suhu.begin();

  lcd.begin();
  lcd.backlight();
  lcd.clear();

  pinMode(tombol, INPUT_PULLUP);
  pinMode(led_buzz, OUTPUT);
  digitalWrite(led_buzz, LOW); // LED dan Buzzer Mati

  MQ7.setRegressionMethod(1); //_PPM =  a*ratio^b // 1 -> Exponential || 2 -> Linear
  MQ7.setA(99.042); // Nilai A
  MQ7.setB(-1.518); // Nilai B

  MQ6.setRegressionMethod(1); //_PPM =  a*ratio^b // 1 -> Exponential || 2 -> Linear
  MQ6.setA(2127.2); // Nilai A
  MQ6.setB(-2.526); // Nilai B

  /*
    Exponential regression:
  GAS     | a      | b
  H2      | 69.014  | -1.374
  LPG     | 700000000 | -7.703
  CH4     | 60000000000000 | -10.54
  CO      | 99.042 | -1.518
  Alcohol | 40000000000000000 | -12.35
  */

  /************** Program Inisialisasi Sensor MQ-7 *************/
  MQ7.init(); 
  //MQ7.setRL(10);

  Serial.print("Calibrating sensor MQ7 please wait.");
  float calcR0_MQ7 = 0;
  for(int i = 1; i <= 10; i++) {
    MQ7.update(); 
    calcR0_MQ7 += MQ7.calibrate(RatioMQ7CleanAir);
    Serial.print(".");
  }
  MQ7.setR0(calcR0_MQ7/10);
  Serial.println("  MQ7 done!.");

  if(isinf(calcR0_MQ7)) {
    Serial.println("Warning: Conection issue, R0 is infinite (Open circuit detected) please check your wiring and supply");
    while(1);
  }
  
  if(calcR0_MQ7 == 0){
    Serial.println("Warning: Conection issue found, R0 is zero (Analog pin shorts to ground) please check your wiring and supply");
    while(1);
  }

  //MQ7.serialDebug(true);


  /************** Program Inisialisasi Sensor MQ-6 *************/
  MQ6.init();  
  // MQ6.setRL(10);

  Serial.print("Calibrating MQ-6 please wait.");
  float calcR0_MQ6 = 0;
  for(int i = 1; i <= 10; i ++) {
    MQ6.update(); 
    calcR0_MQ6 += MQ6.calibrate(RatioMQ6CleanAir);
    Serial.print(".");
  }
  MQ6.setR0(calcR0_MQ6/10);
  Serial.println("  MQ6 done!.");

  if(isinf(calcR0_MQ6)) {
    Serial.println("Warning: Conection issue, R0 is infinite (Open circuit detected) please check your wiring and supply");
    while(1);
  }
  
  if(calcR0_MQ6 == 0){
    Serial.println("Warning: Conection issue found, R0 is zero (Analog pin shorts to ground) please check your wiring and supply");
    while(1);
  }

  //MQ6.serialDebug(true);

  // Letak Awalan LCD Startting
  // .......

  delay(2000);
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Mode ");
  lcd.print(menu);
  lcd.setCursor(0, 1);
  lcd.print("2T<010");
  delay(2000);
  lcd.clear();
}

void loop() {
  if(debounce) {
    if(digitalRead(tombol) == LOW) {
      menu++;
      if(menu > 4) menu = 1;

      //Serial.println(menu);
      t_db = millis();
    }
  }

  switch(menu) {
    case 1: // Sepeda Motor 2 Langkah < 2010
      batas_co = 4.5;  // Dalam %
      batas_hc = 6000; // Dalam ppm
      mode     = "2T<010";
    break;

    case 2: // Sepeda Motor 4 Langkah < 2010
      batas_co = 5.5;  // Dalam %
      batas_hc = 2200; // Dalam ppm
      mode     = "4T<010";
    break;

    case 3: // Sepeda Motor 2010 - 2016
      batas_co = 4.0;  // Dalam %
      batas_hc = 1800; // Dalam ppm
      mode     = "MT1016";
    break;

    case 4: // Sepeda Motor > 2016
      batas_co = 3.0;  // Dalam %
      batas_hc = 1000; // Dalam ppm
      mode     = "MT>016";
    break;
  }
  
  suhu.requestTemperatures();
  int suhuC = suhu.getTempCByIndex(0);

  if(suhuC == DEVICE_DISCONNECTED_C){
    suhuC = 0;
  }

  int pembanding_suhu = 5;
  suhuC = suhuC + pembanding_suhu; // Bisa ditambah atau dikurangi

  if(millis() - t_baca_MQ7 <= 500) { // Waktu baca 0,5 detik -> bisa diganti
    MQ7.update();
    data_CO = MQ7.readSensor();
    //MQ7.serialDebug();
    t_baca_MQ7 = millis();
  }

  persen_CO = map(data_CO, 20.0, 2000.0, 0.0, 100.0);

  if(millis() - t_baca_MQ6 <= 500) { // Waktu baca 0,5 detik -> bisa diganti
    MQ6.update();
    data_HC = MQ6.readSensor();
    //MQ7.serialDebug();
    t_baca_MQ6 = millis();
  }
  
  if((data_CO >= batas_co) || (data_HC >= batas_hc)) {
    status_lcd = "NOK";
    if(millis() - t_notif >= 250) { // Waktu frekuensi kedipan dan bunyi
      if(nyala == LOW) {
        nyala = HIGH;
      }else{
        nyala = LOW;
      }
      digitalWrite(led_buzz, nyala);
      t_notif = millis();
    }
  }else{
    status_lcd = " OK";
  }

  if(millis() - t_lcd >= 500) {
    lcd.setCursor(0, 0);
    lcd.print("CO:");
    lcd.print(strnum3digit_desimal(persen_CO));
    lcd.print("%");

    lcd.setCursor(12, 0);
    lcd.print(strnum2digit_bulat(suhuC));
    lcd.print((char)0xDF);
    lcd.print("C");

    lcd.setCursor(0, 1);
    lcd.print("HC:");
    lcd.print(strnum4digit_bulat(data_HC));
    lcd.print("ppm");

    lcd.setCursor(12, 1);
    lcd.print(status_lcd);
    t_lcd = millis();
  }

  if(millis() - t_debug >= 1000) {
    Serial.print("Mode Tombol : ");
    Serial.print(menu);
    Serial.print("  --->  ");
    Serial.println(mode);
    Serial.print("Suhu Oli    : ");
    Serial.println(suhuC);
    Serial.print("Kadar CO    : ");
    Serial.println(data_CO);
    Serial.print("Persen CO   : ");
    Serial.println(persen_CO);
    Serial.print("Kadar HC    : ");
    Serial.println(data_HC);
    Serial.println();
    t_debug = millis();
  }
}

String strnum2digit_bulat(int nilai) {
  String hasil = "";
  if(nilai < 10) {
    hasil = "0" + String(nilai);
  }else{
    hasil = String(nilai);
  }
  return hasil;
}

String strnum3digit_desimal(float angka) {
  int nilai = int(angka);
  String hasil = "";
  if((nilai > 9) && (nilai < 100)) {
    hasil = "0" + String(nilai);
  }else if((nilai >= 0) && (nilai < 10)){
    hasil = "00" + String(nilai);
  }else{
    hasil = String(nilai);
  }
  return hasil;
}

String strnum4digit_bulat(int nilai) {
  String hasil = "";
  if((nilai > 99) && (nilai < 1000)) {
    hasil = "0" + String(nilai);
  }else if((nilai > 9) && (nilai < 100)){
    hasil = "00" + String(nilai);
  }else if((nilai >= 0) && (nilai < 10)){
    hasil = "000" + String(nilai);
  }else{
    hasil = String(nilai);
  }
  return hasil;
}
