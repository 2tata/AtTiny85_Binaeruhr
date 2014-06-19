/*
#***********************************************
#  Dateiname:  ..............  Binaer_uhr.ino  *  
#  Autor:  ..................  Jan-Tarek Butt  *
#  Sprache:  .............................  C  *
#  Dialekt:  .......................  Arduino  *
#  Hardware:  .....................  AtTiny85  *
#  Datum:  .......................  26.6.2013  *
#***********************************************
*/

//Debuggingmodus starten
//#define DEBUG

//I2C Bibiliohek einbinden 
#include <TinyWireM.h>
#include <USI_TWI_Master.h>




//Deffinierung der anzahl der Tage pro monat
//                       {Ja,Fe,Mä,Ap,Ma,Jun,Jul,Au,Se,Ok,No,De}
const byte DaysInMonth[]={31,28,31,30,31,30,31,31,30,31,30,31};
//I2C adresse der RTC
const byte add=B01101000;
//Addressirung der einzelnen LEDs in Richtiger reienfolge 38 Zahlen sind erforderlich
//                              Sekunden                  Minuten                  Stunden                    Tage                    Monate                  Jahre                Wochentage
const byte BitTwiddling[]={47,39,31,22,13,3,0,0,    46,38,30,21,12,4,0,0,   45,37,29,20,10,0,0,0,    44,36,28,19,11,0,0,0,     43,35,27,18,0,0,0,0,    42,34,26,17,9,2,0,0,   41,33,25,23,14,5,1,0};
//0. Bit SEC->41. Bit ShR,...
// RTC Pins
const byte Data=2;
const byte Clock=4;
const byte MClock=3;

byte SEC=0,MIN=0,HOUR=0,DAY=1,MONTH=1,YEAR=0,DAYOFWEEK=1;
boolean Sommerzeit=0;
byte *DataOut=(byte*) malloc(6);

int count=0;

#ifdef DEBUG
//Serial Bibiliothek einbinden für debugging
#include <SoftwareSerial.h>
#define rx -1
#define tx 3
SoftwareSerial mySerial(rx,tx);
#endif

void setup()
{
 TinyWireM.begin();
 ZeitEinlesen();
 
 pinMode(Data,OUTPUT);
 pinMode(Clock,OUTPUT);
 pinMode(MClock,OUTPUT);
 digitalWrite(Data,0);
 digitalWrite(Clock,0);
 digitalWrite(MClock,0);
 #ifdef DEBUG
 mySerial.begin(4800);
 #endif
}


void loop()
{
#ifdef DEBUG
  mySerial.println("Angezeigte zeit: ");
  mySerial.print("Date: ");
  mySerial.print(YEAR);
  mySerial.print("/");
  mySerial.print(MONTH);
  mySerial.print("/");
  mySerial.print(DAY);
  mySerial.print("  ");
  mySerial.print(HOUR);
  mySerial.print(":");
  mySerial.print(MIN);
  mySerial.print(":");
  mySerial.print(SEC);
  mySerial.print("  Sommerzeit:");
  mySerial.println(Sommerzeit);
  mySerial.println("");
#endif
 delay(1000);
 addOneSec();
 count++;
 if(count==62)
  {
   ZeitEinlesen();
   count=0;
  }
}

void addOneSec()
{
 SEC++;
 if(SEC==60)
  {MIN++;SEC=0;}
 if(MIN==60)
  {HOUR++;MIN=0;
   HOUR+=WiSo()-SoWi();   
  }
 if(HOUR>23)
  {HOUR+=-24;DAY++;DAYOFWEEK=(DAYOFWEEK+1)%7;}
 if(DAY>DaysInMonth[MONTH-1])
  {if(MONTH!=2)
    {DAY=1;MONTH++;}
   else
    {if(YEAR%4) 
      {DAY=1;MONTH++;}
     else
      if(DAY==30)
       {DAY=1;MONTH++;}
    }
  }
 if(MONTH==13)
  {YEAR++;MONTH=1;}
 writeOutTime();  
}

byte SoWi()
{if((Sommerzeit) && (MONTH==10) && (DAY>24) && (DAYOFWEEK==6) && (HOUR==3))
 {
  Sommerzeit=false;
  return 1; 
 }
 return 0;
}

byte WiSo()
{if((!Sommerzeit) && (MONTH==3) && (DAY>24) && (DAYOFWEEK==6) && (HOUR==2))
 {
  Sommerzeit=true;
  return 1;
 }
 return 0;
}

byte DoW_Gauss(word year,byte month,byte day)
{
const byte m[]={0,0,3,2,5,0,3,5,1,4,6,2,4};  
byte y=(year%100)-(month==1 | month==2);
byte c=(year/100)-(!(year%400));
return (((day+m[month]+y+byte(y/4)+byte(c/4)-2*c+203-1)%7));
}

void SetSommerzeit()
{byte n;
 Sommerzeit=((MONTH>3) && (MONTH<10));
 if(MONTH==3)
  {for(n=31;DoW_Gauss(YEAR+2000,3,n)!=6;n--);
   Sommerzeit=(DAY>n);
   if(DAY==n)
    {
     Sommerzeit=HOUR>2; 
    }
  }
 if(MONTH==10)
  {for(n=31;DoW_Gauss(YEAR+2000,10,n)!=6;n--);
   Sommerzeit=(DAY<n);
   if(DAY==n)
    {
     Sommerzeit=HOUR<2; 
    }   
  } 
}

byte BCD_decode(byte X,byte D)
{
 byte result;
 result =(X & (byte(1<<4)-1));
 result+=((X & ((byte(1<<D)-1)<<4))>>4)*10;
 return result; 
}

void writeOutTime()
{byte c=0;
 byte dw=byte(1)<<DAYOFWEEK;
 bitchange(DataOut,c,SEC);
 c+=8;
 bitchange(DataOut,c,MIN);
 c+=8;
 bitchange(DataOut,c,HOUR);
 c+=8;
 bitchange(DataOut,c,DAY);
 c+=8;
 bitchange(DataOut,c,MONTH);
 c+=8;
 bitchange(DataOut,c,YEAR);
 c+=8;
 bitchange(DataOut,c,dw); 
 shift(DataOut);
}

void bitchange(byte* Z,byte c,byte X)
{
 for(byte n=0;n<8;n++)
  {bitWrite(Z[BitTwiddling[c]/8],BitTwiddling[c]%8,bitRead(X,n));
   c++;
  } 
}

void shift(byte* X)
{
 int n,m;
 digitalWrite(Clock,0);
 digitalWrite(MClock,0); 
 for(m=5;m>-1;m--)
   {for(n=7;n>-1;n--)
    {digitalWrite(Data,(byte(X[m] & (byte(1)<<n)))>0);
     digitalWrite(Clock,1);
     digitalWrite(Clock,0);
    }     
   }
 digitalWrite(MClock,1);
 digitalWrite(MClock,0); 
}

void ZeitEinlesen()
{
 byte c;
 
 TinyWireM.beginTransmission(add);
 TinyWireM.send(0);
 TinyWireM.endTransmission();
 
 
 if(!TinyWireM.requestFrom(add, byte(7))) 
 {
 c=TinyWireM.receive(); 
 SEC=BCD_decode(c,3);
 
 c=TinyWireM.receive();
 MIN=BCD_decode(c,3);

 c=TinyWireM.receive();
 HOUR=BCD_decode(c,2);
 
 c=TinyWireM.receive();
          
 c=TinyWireM.receive();
 DAY=BCD_decode(c,2);
           
 c=TinyWireM.receive();
 MONTH=BCD_decode(c,1);

 c=TinyWireM.receive();
 YEAR=BCD_decode(c,4);
 DAYOFWEEK=DoW_Gauss(YEAR+2000,MONTH,DAY);
 SetSommerzeit();
 HOUR+=Sommerzeit;
 } 
}
