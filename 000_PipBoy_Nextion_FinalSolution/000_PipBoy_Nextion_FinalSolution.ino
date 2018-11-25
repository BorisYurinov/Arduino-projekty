/*
 * Takove demo na opravdovy PipBoy
 * Enkoder funkční na ESP8266
 * Funkční tlačítko s přerušením, přepíná osu X a Y
 * Nextion display s HMI PipBoy
 * RTC cas jeden na t21, datum na t42
 * Pole na data?!? Jde to nejak pomalu
 * WiFi scanner? Radsi ne, zpomaluje cely beh programu
 * Verze z 24.11.2018
 */

 //Pole nebo tabulka s daty - KURVA POZOR, PROHOZENO X a Y!!!
 int pole[3][9] = {{1,2,3,4,5,6,7,8,81}, 
                   {9,10,11,12,13,14,15,16,161}, 
                   {17,18,19,20,21,22,23,24,25}}; 
                   //output = pole[0][2]; //tohle z kodu z toho dostane data
 int poledata = 0;  

//CAS
#include "RTClib.h"
#include <Wire.h>
RTC_DS1307 DS1307;  
//char seznamDni[7][8] = {"nedele", "pondeli", "utery", "streda", "ctvrtek", "patek", "sobota"};
char seznamDni[7][10] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
// pozor, musi tam byt definice poctu a max. delky prvku!!!
unsigned long progrun = 0; //pomocna promenna na zjisteni doby behu programu
int dobabehu = 0;          //doba behu programu

//DHT11
#include <dht11.h> // teplomer
dht11 MojeCidlo; // teplomer
const int analogPin = A0; //na analog pinu se bude načítat svetlo
int svetlo = 0;
int teplota = 0;
int vlhkost = 0;
int pocitadlo = 0; //pocitadlo mereni pro zapis do pole
int polesvetla[5]{0,0,0,0,0 };

//DALLAS
#include "OneWire.h"
#include "DallasTemperature.h"
#define DS18B20 D4 //digitální pin s čidlem
OneWire ourWire(DS18B20);
DallasTemperature sensors(&ourWire);
float t = 0;


//ENC
#include <Encoder.h>
Encoder myEnc(D6, D5);
const int interruptPin = 13; //Spínač ENC na GPIO 13, mohl by to být pin D7
long oldPosition  = -999;
long lastUpdateMillis = 0;
int ENCpos  = 0; //sem se ukládá nová pozice ENC při její změně, aby to byla globální proměnná
int ENC_lastpost = 0; //zaloha pozice z ENC pred divnymi stavy
int vyber = 0;   //sem uloži preruseni pozici pri stisku tlacitka ENC
int xtab = 0;    //aktualni pozice v menu v ose x
int ytab = 0;    //aktualni pozice v menu v ose y

int shift = 1;   //přepínač z osy X(shift = 1) na Y(shift = 0)

int page = 0;
int klik = 0;



#include "ESP8266WiFi.h"

// *********** S E T U P ****************************************************
void setup() {
    Serial.begin(9600);

    //WiFi.mode(WIFI_STA);
    //WiFi.disconnect();
    
    delay(500);
    Nextion("page 0");
    delay(100);
    Nextion("t10.txt=\"USER BORIS\""); //Verze "OS"
    Nextion("dims=80");

    if (! DS1307.begin()) {
    //Serial.println("Hodiny nejsou pripojeny!");
    while (1);
    }
    if (! DS1307.isrunning()) {
    //Serial.println("Hodiny nejsou spusteny! Spoustim nyni..");
    }
  //DS1307.adjust(DateTime(2018, 11, 18, 22, 15, 11)); //Nastaveni casu

         
  //Nastavení přerušení na pin s tlačítkem enkoderu
  pinMode(interruptPin, INPUT_PULLUP); 
  attachInterrupt(digitalPinToInterrupt(interruptPin), handleInterrupt, RISING); 

  //Dallas
  sensors.begin();
  
     
}

// *********** L O O P ****************************************************
void loop() {

  //Vypocet doby behu programu
  progrun=millis();

  
  long newPosition = myEnc.read();
  if (newPosition != oldPosition) {
    oldPosition = newPosition;
    //Serial.println(newPosition); //Nextion tohle nepotrebuje :-)
    ENCpos=(newPosition/4);        //ENC to posílá v krocích po čtyřech....
    
    }

    cas(); //mereni casu a zapis do vsech poli t21
    mereni();
    //wifiscan(); dost zpomaluje funkci listování stránek, kaslat na nej!

    //Rozhodnutí kterou osu řešíme
    if (shift==1)
    {
       xtab = (ENCpos);
       }
    
       else if (shift==0)
       {
       ytab = abs(ENCpos-ENC_lastpost);
       }

   //Kontrola aby souřadnice nebyla blbost
   xtab = constrain(xtab, 0, 9); 
   ytab = constrain(ytab, 0, 2);
   
   poledata = pole[ytab][xtab];
  

  /*if (klik==1) && poledatahghfghfghgfhfgh
    {
       Nextion("t77.txt=\"klik !!!\""); //= změna obsahu pole
       }
*/

  
   //Nejake systemove info na page 7
   if (page==7)
    {
       Nextion("t75.txt=\""+(String)shift+"\"");
       Nextion("t76.txt=\""+(String)ENCpos+"\"");
       Nextion("t77..txt=\""+(String)ENCpos+"\"");
       //Nextion("t71.txt=\""+(String)xtab_k2+"\"");
       Nextion("t72.txt=\""+(String)poledata+"\"");
       Nextion("t73.txt=\""+(String)xtab+"\"");
       Nextion("t74.txt=\""+(String)ytab+"\"");
       }


  
  

  //Stránkování s daty z ENC
  //Stránku načítá pokaždé znovu - volám ji, že... 
  switch (xtab) {
  case 0:
    if(page!=2){
      Nextion("page 2");
      page=2;
      }
    break;
  case 1:
   if(page!=3){
      Nextion("page 3");
      page=3;
      }
    break;
  case 2:
    if(page!=4){
      Nextion("page 4");
      page=4;
      }
    break;
  case 3:
    if(page!=5){
      Nextion("page 5");
      page=5;
      }
    break;
  case 4:
    if(page!=6){
      Nextion("page 6");
      page=6;
      }
    break;
  case 5:
    if(page!=7){
      Nextion("page 7");
      page=7;
      }
    break;
  case 6:
    if(page!=8){
      Nextion("page 8");
      page=8;
      }
    break;
  case 7:
    if(page!=9){
      Nextion("page 9");
      page=9;
      }
    break;
  case 8:
    if(page!=10){
      Nextion("page 10");
      page=10;
      }
    default:
    //Nextion("page 0");
    break;
}
   
   
    
    klik = 0;
    delay(10); 

    //Vypocet doby behu programu a jeho zapsani na Nextion
    Nextion("t82.txt=\""+(String)dobabehu+"\"");
    dobabehu=(millis()-progrun);
    
 
  
}




// ************ F U N K C E **************************************************


// Funkce na preruseni pri stisku tlacitka
void handleInterrupt() { 
    
    // !!! NIKDY SEM NEDÁVAT DELAY !!!
    
   klik=1; //kliknutí na spínač projede celý cyklus a pak se na konci vymaže
    
if ((shift==1) && (ENCpos >= 0))     //přesun na osu Y jen když jsme na ose X a ne mimo s enkoderem
{
  shift=0;                           //přepnutí na osu Y
  ENC_lastpost = ENCpos;             //zaloha pozice ENC pro navrat na X 
  
  }
else if ((shift==0) && (ENCpos == ENC_lastpost))  //vracime se na X jen kdyz jsme na spravne pozici osy Y
{
  shift=1;                           //přesun na osu X
  
 }

  
}



// Funkce která posílá data a příkazy pro Nextion
/*
 * Nextion("page 1"); = změna stránky
 * Nextion("t0.txt=\"Cas...\""); = změna obsahu pole
 * Nextion("t1.txt=\""+(String)w+"\""); = poslání proměnný
 * Nextion("dims=80"); = defaultní podsvícení 
 */
 
void Nextion(String data){
Serial.print(data);
Serial.write(0xff);
Serial.write(0xff);
Serial.write(0xff);
}

//Mereni casu a zasadni otazka pridani nuly na zacatek pokud je malo minut
void cas(){
  DateTime now = DS1307.now();

  //Cas
  if (now.minute()<10)
{
  Serial.print("t21.txt="); 
  Serial.write(0x22);
  Serial.print(String(now.hour())+":0"+String(now.minute()));
  Serial.write(0x22);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
}
else if (now.minute()>=10)
{
  Serial.print("t21.txt="); 
  Serial.write(0x22);
  Serial.print(String(now.hour())+":"+String(now.minute()));
  Serial.write(0x22);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
}

  // Datum
  Serial.print("t42.txt="); 
  Serial.write(0x22);
  Serial.print(String(now.day())+"."+String(now.month())+"."+String(now.year()));
  Serial.write(0x22);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);

  //Den v tydnu
  Serial.print("t81.txt="); 
  Serial.write(0x22);
  Serial.print(seznamDni[now.dayOfTheWeek()]);
  Serial.write(0x22);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);



}


void mereni(){
  //DALLAS
  sensors.requestTemperatures();
  t = sensors.getTempCByIndex(0); 

  //DHT11 + fotorezistor
  MojeCidlo.read(D3); // přečte údaje z čidla DTH11 připojeného na digital pin 3
  //teplota = MojeCidlo.temperature;   //teplota z DHT11 je stejne k hovnu
  vlhkost = MojeCidlo.humidity;

   //Svetlo
   /*
   polesvetla[pocitadlo]= analogRead(analogPin);
   pocitadlo=(pocitadlo+1);

    if (pocitadlo=4)
    {
        svetlo=0;
        
        for (int i=0; i < 5; i++){ 
        svetlo = svetlo + polesvetla[pocitadlo];
       }
       svetlo=(svetlo/5);
       pocitadlo=0;
       }
   */
  
  svetlo = analogRead(analogPin);
  svetlo = (svetlo/4);
  
  
  Nextion("t24.txt=\""+(String)t+"\""); 
  Nextion("t25.txt=\""+(String)vlhkost+"\""); 
  Nextion("t26.txt=\""+(String)svetlo+"\""); 
  
  
  if (svetlo<50)
    {
       Nextion("dim=20");
       }
    
       else if (svetlo>=50)
       {
       Nextion("dim=90");
       }
}

/*
void wifiscan()  {
  int n = WiFi.scanNetworks();
  // v případě nulového počtu sítí vypíšeme informaci
  // po sériové lince
  if (n == 0) {
    //Serial.println("Zadne viditelne WiFi site v okoli.");
  }
  // pokud byly nalezeny WiFi sítě v okolí,
  // vypíšeme jejich počet a další informace
  else  {
        
    // výpis všech WiFi sítí v okolí,
    // vypíšeme název, sílu signálu a způsob zabezpečení
   
     
      Serial.print("t61.txt="); 
      Serial.write(0x22);
      Serial.print(String(WiFi.SSID(0))+" "+String(WiFi.RSSI(0))); //Název + síla signálu
      Serial.write(0x22);
      Serial.write(0xff);
      Serial.write(0xff);
      Serial.write(0xff);

      Serial.print("t62.txt="); 
      Serial.write(0x22);
      Serial.print(String(WiFi.SSID(1))+" "+String(WiFi.RSSI(1)));
      Serial.write(0x22);
      Serial.write(0xff);
      Serial.write(0xff);
      Serial.write(0xff);

      Serial.print("t63.txt="); 
      Serial.write(0x22);
      Serial.print(String(WiFi.SSID(2))+" "+String(WiFi.RSSI(2)));
      Serial.write(0x22);
      Serial.write(0xff);
      Serial.write(0xff);
      Serial.write(0xff);

  }
}
*/
