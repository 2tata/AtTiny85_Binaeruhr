//I2C Bibilioheken einbinden 
#include <TinyWireM.h>
#include <USI_TWI_Master.h>

//Deffinierung der anzahl der Tage pro monat{Ja,Fe,Mä,Ap,Ma,Jun,Jul,Au,Se,Ok,No,De}
const byte DaysInMonth[]={31,28,31,30,31,30,31,31,30,31,30,31};
//I2C adresse der RTC 
const byte add = 0x68;
//Addressirung der einzelnen LEDs in Richtiger reienfolge
const byte BitTwiddling[]={47,39,31,22,13,3,0,0,/*/46,38,30,21,12,4,0,0,/*/45,37,29,20,10,0,0,0,/*/44,36,28,19,11,0,0,0,/*/43,35,27,18,0,0,0,0,/*/42,34,26,17,9,2,0,0,/*/41,33,25,14,5,1,0,0};
//0. Bit SEC->41. Bit ShR,...
const byte Data=2;
const byte Clock=4;
const byte MClock=3;
const byte Cont=1;
byte SEC,MIN,HOUR,DAY,MONTH,YEAR,DAYOFWEEK;
byte ContPin=0;
boolean Sommerzeit=0;
byte *DataOut=(byte*) malloc(6);
void setup()
{byte c;
 TinyWireM.begin();
 
 TinyWireM.beginTransmission(add);
 TinyWireM.send(0);                
 TinyWireM.endTransmission();          
 TinyWireM.requestFrom(add,1); 
 c=TinyWireM.receive(); 
 if(bitRead(c,7))
  {TinyWireM.beginTransmission(add);
   TinyWireM.send(0);
   TinyWireM.send(c & B01111111); 
   TinyWireM.endTransmission(); 
  }
 SEC=BCD_decode(c,3);
 
 TinyWireM.beginTransmission(add);
 TinyWireM.send(1);                
 TinyWireM.endTransmission();          
 TinyWireM.requestFrom(add,1); 
 c=TinyWireM.receive();
 MIN=BCD_decode(c,3);

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
 
 TinyWireM.beginTransmission(add);
 TinyWireM.send(4);                
 TinyWireM.endTransmission();          
 TinyWireM.requestFrom(add,1); 
 c=TinyWireM.receive();
 DAY=BCD_decode(c,2);
 
 TinyWireM.beginTransmission(add);
 TinyWireM.send(5);                
 TinyWireM.endTransmission();          
 TinyWireM.requestFrom(add,1); 
 c=TinyWireM.receive();
 MONTH=BCD_decode(c,1);

 TinyWireM.beginTransmission(add);
 TinyWireM.send(6);                
 TinyWireM.endTransmission();          
 TinyWireM.requestFrom(add,1); 
 c=TinyWireM.receive();
 YEAR=BCD_decode(c,4);
 
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
{int d;
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

byte SoWi()
{if((Sommerzeit) && (MONTH==10) && (DAY>24) && (DAYOFWEEK==6) && (HOUR==3))
 {return 1;  
  Sommerzeit=false;
 }
 return 0;
}

byte WiSo()
{if((!Sommerzeit) && (MONTH==3) && (DAY>24) && (DAYOFWEEK==6) && (HOUR==2))
 {return 1;  
  Sommerzeit=true;
 }
 return 0;
}

byte DoW_Gauss(word year,byte month,byte day)
{
const byte m[]={0,0,3,2,5,0,3,5,1,4,6,2,4};  
byte y=(year%100)-(month==1 | month==2);
byte c=(year/100)-(!(year%400));
return (((day+m[month]+y+(y/4)+(c/4)-2*c+203-1)%7));
}

void SetSommerzeit()
{byte n;
 Sommerzeit=((MONTH>3) && (MONTH<10));
 if(MONTH==3)
  {for(n=31;DoW_Gauss(YEAR+2000,10,n)!=6;n--);
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
