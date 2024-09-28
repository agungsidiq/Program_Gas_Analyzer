#include <OneWire.h>            // https://github.com/PaulStoffregen/OneWire
#include <DallasTemperature.h>  // https://github.com/milesburton/Arduino-Temperature-Control-Library
#include "MQ7.h"                // https://github.com/swatish17/MQ7-Library
#include <MQ2.h>                // https://github.com/labay11/MQ-2-sensor-library
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>  // https://github.com/fdebrabander/Arduino-LiquidCrystal-I2C-library

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define ONE_WIRE_BUS 2

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature suhu(&oneWire);

MQ7 mq7(A1, 5.0);
MQ2 mq2(A0);

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

unsigned long t_baca_MQ7 = millis();
unsigned long t_baca_MQ6 = millis();

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

  mq2.begin();
  
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
      t_db = millis();
      menu++;
      if(menu > 4) menu = 1;

      if(menu == 1) {         // Sepeda Motor 2 Langkah < 2010
        mode     = "2T<010";
      }else if(menu == 2) {   // Sepeda Motor 4 Langkah < 2010
        mode     = "4T<010";
      }else if(menu == 3) {   // Sepeda Motor 2010 - 2016
        mode     = "MT1016";  
      }else if(menu == 4) {   // Sepeda Motor > 2016
        mode     = "MT>016";
      }

      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Mode ");
      lcd.print(menu);
      lcd.print(" (");
      lcd.print(mode);
      lcd.print(")");
      lcd.setCursor(0,1);
      lcd.print("Dipilih !");

      delay(2000);
      lcd.clear();

      //Serial.println(menu);
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

  if(millis() - t_baca_MQ7 >= 500) {
    data_CO   = mq7.getPPM();                           // Baca PPM MQ7
    data_CO   = constrain(data_CO, 0, 2000);            // Biarkan angka menjadi tetap antara 0 sampai 2000
    persen_CO = map(data_CO, 20.0, 2000.0, 0.0, 100.0); // Membuat persentase

    t_baca_MQ7 = millis();
  }
  
  if(millis() - t_baca_MQ6 >= 500) { // Waktu baca 0,5 detik -> bisa diganti
    data_HC = mq2.readSmoke();
    //data_HC = mq2.readLPG();

    float angka_penyesuaian = 3.0; // SILAHKAN DIGANTI SESUAI YANG DIMINTA
    data_HC = data_HC + angka_penyesuaian; // Bisa tambah atau kurang
    t_baca_MQ6 = millis();
  }
  
  if((persen_CO >= batas_co) || (data_HC >= batas_hc)) {
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
    Serial.print(data_CO);
    Serial.print("  (");
    Serial.print(persen_CO);
    Serial.println(" %)");
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
    hasil = "0" + String(angka);
  }else if((nilai >= 0) && (nilai < 10)){
    hasil = "00" + String(angka);
  }else{
    hasil = String(angka);
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
