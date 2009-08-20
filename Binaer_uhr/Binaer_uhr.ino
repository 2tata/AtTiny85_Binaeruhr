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

//I2C Bibilioheken einbinden 
#include <TinyWireM.h>
#include <USI_TWI_Master.h>

//Deffinierung der anzahl der Tage pro monat{Ja,Fe,Mä,Ap,Ma,Jun,Jul,Au,Se,Ok,No,De}
const byte DaysInMonth[]={31,28,31,30,31,30,31,31,30,31,30,31};

//I2C adresse der RTC 
const byte add = 0x68; //Address of RTC

//Addressirung der einzelnen LEDs in Richtiger reienfolge 38 Zahlen sind erforderlich 
//                              Sekunden                  Minuten                  Stunden                    Tage                    Monate                  Jahre                Wochentage
const byte BitTwiddling[]={47,39,31,22,13,3,0,0,/**/46,38,30,21,12,4,0,0,/**/45,37,29,20,10,0,0,0,/**/44,36,28,19,11,0,0,0,/**/43,35,27,18,0,0,0,0,/**/42,34,26,17,9,2,0,0,/**/41,33,25,23,14,5,1,0};

//Pins
const byte Data=2;
const byte Clock=4;
const byte MClock=3;
const byte Cont=1; //SquareWave von RTC

byte SEC,MIN,HOUR,DAY,MONTH,YEAR,DAYOFWEEK;//Zwischenspeicher der RTC
byte ContPin=0; // Stores last SQW-State

boolean Sommerzeit=0; //Sommerzeit überprüfen

byte *DataOut=(byte*) malloc(6);
void setup()
{
    //RTC i2c kommunikation
 
 byte c = TinyWireM.receive(); //Lese adresse 0 (SEC u. Clock halt)
 //Sekunden Auslesen u. RTC einschalten  
 if(bitRead(c,7))
  {
   TinyWireM.beginTransmission(add);
   TinyWireM.send(0);
   TinyWireM.send(c & B01111111); //disable clock halt
   TinyWireM.endTransmission(); 
  }
 c &= B01111111;
 SEC=BCD_decode(c,3);

 //Minuten Auslesen 
 TinyWireM.beginTransmission(add);
 TinyWireM.send(1);                
 TinyWireM.endTransmission();          
 TinyWireM.requestFrom(add,1); 
 c=TinyWireM.receive();
 MIN=BCD_decode(c,3);

 //Stunden Auslesen 
 TinyWireM.beginTransmission(add);
 TinyWireM.send(2);                
 TinyWireM.endTransmission();          
 TinyWireM.requestFrom(add,1); 
 c=TinyWireM.receive();
 HOUR=BCD_decode(c,2);
 if(bitRead(c,6))
  {HOUR =BCD_decode(c,1);
   HOUR+=12*bitRead(c,5);
  }
 /*
 //Day of Week
 TinyWireM.beginTransmission(add);
 TinyWireM.send(3);
 TinyWireM.endTransmission();
 TinyWireM.requestFrom(add,1);
 c=TinyWireM.receive();
 DAYOFWEEK=c;
*/
 //Monatstage auslesen
 TinyWireM.beginTransmission(add);
 TinyWireM.send(4);                
 TinyWireM.endTransmission();          
 TinyWireM.requestFrom(add,1); 
 c=TinyWireM.receive();
 DAY=BCD_decode(c,2);
 
 //Monaten Auslesen 
 TinyWireM.beginTransmission(add);
 TinyWireM.send(5);                
 TinyWireM.endTransmission();          
 TinyWireM.requestFrom(add,1); 
 c=TinyWireM.receive();
 MONTH=BCD_decode(c,1);

 //Jahre Auslesen 
 TinyWireM.beginTransmission(add);
 TinyWireM.send(6);                
 TinyWireM.endTransmission();          
 TinyWireM.requestFrom(add,1); 
 c=TinyWireM.receive();
 YEAR=BCD_decode(c,4);
 
 //SQWE Enablen
 TinyWireM.beginTransmission(add);
 TinyWireM.send(7);
 TinyWireM.send(B00010000); 
 TinyWireM.endTransmission();

 DAYOFWEEK=DoW_Gauss(YEAR+2000,MONTH,DAY);
 
 SetSommerzeit();
 HOUR+=Sommerzeit;
 pinMode(Data,OUTPUT);
 pinMode(Clock,OUTPUT);
 pinMode(MClock,OUTPUT);
 pinMode(Cont,INPUT);
 digitalWrite(Data,0);
 digitalWrite(Clock,0);
 digitalWrite(MClock,0);
}


void loop()
{
 int d;
 d=digitalRead(Cont)-ContPin;
 if(d==1)
  {ContPin=1;
   addOneSec();
  }
 if(d==-1)
  {ContPin=0;} 
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
//Prüfen ob Sommer oder Winterzeit ist 
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
   {
     for(n=7;n>-1;n--)
    {
      digitalWrite(Data,(byte(X[m] & (byte(1)<<n)))>0);
     digitalWrite(Clock,1);
     digitalWrite(Clock,0);
    }     
   }
 digitalWrite(MClock,1);
 digitalWrite(MClock,0); 
}
