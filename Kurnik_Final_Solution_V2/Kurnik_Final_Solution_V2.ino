/*
 * Ovladani kurniku
 * Dallas v externi krabicce a luxmetr
 * Display 16x2 i2c
 * Plně manuální ovládání otočný spínačem + automat
 * RTC čas
 * Aktuálně běžící verze 27.10.2018
 */


#include <Wire.h>
#include "OneWire.h"
#include "DallasTemperature.h"

#include <LiquidCrystal_PCF8574.h>
LiquidCrystal_PCF8574 lcd(0x27);  // set the LCD address to 0x27 for a 16 chars and 2 line display

#define DS18B20 12 //digitální pin, kde je teploměr Dallas DS18B20
OneWire ourWire(DS18B20);
DallasTemperature sensors(&ourWire);

#include <BH1750.h>
BH1750 luxSenzor;

#include "RTClib.h"
RTC_DS1307 DS1307;



const int switchUP = 5; 
const int switchDOWN = 4; 
int switchSTATE1 = 0;
int switchSTATE2 = 0;
int pozadavek = 0;

int tout = 0;
int tin = 0;

int analogPin = 2;
int pozice = 0;
int prc = 0; //procenta povoleného vysunutí

int hm = 600;  //800, bezpečná horní mez pro dojezd pro přímo změřené hodnoty z potenciometru
int dm = 150;   //90, 

int RPWM=6; //ok zelená
int LPWM=9; //ok červená
int L_EN=7; //ok černá
int R_EN=8; //ok bílá

int hodiny = 0;
int minuty = 0;


//*************************************************************************************
//*************************************************************************************
void setup() {

  Serial.begin(9600); // Debugging only
  Serial.println("Boot......");
  
  // put your setup code here, to run once:
  pinMode(switchUP, INPUT);
  pinMode(switchDOWN, INPUT);
  
  sensors.begin();
  luxSenzor.begin();
  
  Wire.begin();
  Wire.beginTransmission(0x27);
  lcd.begin(16, 2); // initialize the lcd
  
   //Boot info
   lcd.clear();
   lcd.setBacklight(1);
   lcd.home();
   lcd.setCursor(0, 0);
   lcd.print("*Kurnik System*"); 
   lcd.setCursor(7, 1);
   lcd.print("OK");

   pinMode(RPWM,OUTPUT);
   pinMode(LPWM,OUTPUT);
   pinMode(L_EN,OUTPUT);
   pinMode(R_EN,OUTPUT);
   digitalWrite(RPWM,LOW);
   digitalWrite(LPWM,LOW);
   digitalWrite(L_EN,LOW);
   digitalWrite(R_EN,LOW);
   delay(50);
   analogWrite(RPWM,0);
   analogWrite(LPWM,0);

    if (! DS1307.begin()) {
    Serial.println("Hodiny nejsou pripojeny!");
    while (1);
    }
    // kontrolu spuštění obvodu reálného času
    if (! DS1307.isrunning()) {
    Serial.println("Hodiny nejsou spusteny! Spoustim nyni..");
    }
    // následující příkaz v pořadí rok, měsíc, den, hodina, minuta, vteřina
    //DS1307.adjust(DateTime(2018, 10, 27, 14, 27, 22)); //Použije se jen jednou, pro nastavení

   
   delay(2000);
   

}
//*************************************************************************************
//*************************************************************************************

void loop() {

     DateTime datumCas = DS1307.now();
     hodiny = (datumCas.hour()); 
     minuty = (datumCas.minute());

    //Kontrola pozadavku na pozici dveri **********************************************
    int switchSTATE1 = digitalRead(switchUP);
    int switchSTATE2 = digitalRead(switchDOWN);
    
    if (switchSTATE1 == 1)
       {
        pozadavek = 1;
        zavrit();
       }
           else if (switchSTATE2 == 1)
           {
            pozadavek = 2;
            otevrit();
            }
              else 
                 {
                 pozadavek = 0;
                 }

    //Mereni svetla *******************************************************************
    uint16_t lux = luxSenzor.readLightLevel();

    //Mereni teplot *******************************************************************
    sensors.requestTemperatures(); //nacti udaj o teplote z cidla
    tout = sensors.getTempCByIndex(0);//cislo uloz do promenne "t"
    tin = sensors.getTempCByIndex(1);//cislo uloz do promenne "t"

   
    // Pohyb dveri ********************************************************************

     if (pozadavek == 0) 
         { 
     
           if ((hodiny >= 4) && (hodiny <= 15) && (lux > 10))          //6h, 48 lux původní plán, 40 lux ráno v zimě pouští slepice pozdě.  
               { 
               otevrit();
               }
  
         else if ((hodiny >= 16) && (hodiny <= 23) && (lux < 1))       //20h, 3 lux
               { 
               zavrit();
               }
          
          }
    
    
    // Vypis hodnot na LCD *************************************************************
      lcd.clear();
      lcd.setBacklight(1);
      lcd.home(); 
      
      lcd.setCursor(0, 0);
      lcd.print(tout); lcd.print("C "); 
      //lcd.setCursor(5, 0);
      lcd.print(tin); lcd.print("C "); 
      //lcd.setCursor(10, 0);
      lcd.print(lux); lcd.print("L ");
      lcd.print(pozice);

      
      lcd.setCursor(0, 1);
      lcd.print(hodiny); lcd.print(":"); lcd.print(minuty); 
      
      lcd.setCursor(11, 1);
      //nic :-)      

      //info po serialce
      pozice = analogRead(analogPin);
      Serial.print("Switch: "); Serial.print(pozadavek); Serial.print(" ");Serial.print(hodiny); Serial.print(":"); Serial.print(minuty);Serial.print("  Lux: "); Serial.print(lux); Serial.print("  Poz: "); Serial.println(pozice); 
      
      delay(2000);
}


// **********************************************************************************
void otevrit()
{
  Serial.println("Otviram...");
  while(pozice < hm){
  digitalWrite(L_EN,HIGH);
  digitalWrite(R_EN,HIGH);
  analogWrite(RPWM,255);
  delay(1);
  pozice = analogRead(analogPin);
  Serial.println(pozice);
  lcd.setCursor(0, 1);
  lcd.print("Otv.: "); lcd.print(pozice); lcd.print("->"); lcd.print(hm); lcd.print(" ");
  }
  
  digitalWrite(L_EN,LOW);
  digitalWrite(R_EN,LOW);
  analogWrite(RPWM,0);
  //delay(1000*60);
}

// *********************************************************************************
void zavrit()
{
  Serial.println("Zaviram...");
  while(pozice > dm){
  digitalWrite(L_EN,HIGH);
  digitalWrite(R_EN,HIGH);
  analogWrite(LPWM,255);
  delay(1);
  pozice = analogRead(analogPin);
  Serial.println(pozice);
  lcd.setCursor(0, 1);
  lcd.print("Zav.: "); lcd.print(pozice); lcd.print("->"); lcd.print(dm); lcd.print(" ");
  }
   
  digitalWrite(L_EN,LOW);
  digitalWrite(R_EN,LOW);
  analogWrite(LPWM,0);
  //delay(1000*60);
}
