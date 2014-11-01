// This #include statement was automatically added by the Spark IDE.
#include "idDHT22.h"

/****************************************************************************
*   Project: Weather Station NG                                           *
*                                                                           *
****************************************************************************/
extern char* itoa(int a, char* buffer, unsigned char radix);

// Time (seconds) for each sleep
#define PERIOD 300
// Set UPDATE2CLOUD to true if you want the measurement sent to the cloud
#define UPDATE2CLOUD true 
// Thingspeak API WRITE key
#define THINGSPEAK_API_WRITEKEY "89DQON6G9XOGVTR3"
#define version 101

#define BACKLDUR 10000 // 10s
#define TIMEOUT 10000
#define CHTUPDPERIOD 300 // update every 300s

int lastT;
// int BackLight = A1;
int BackLightTrigger = D3;
int trigger = 0;
bool refresh = true;

// Pin out defs
int led = D7;

// declaration for AM2321 handler
int idDHT22pin = D4; //Digital pin for comunications
void dht22_wrapper(); // must be declared before the lib initialization
String status;
float temp, humi = 0;
// AM2321 instantiate
idDHT22 DHT22(idDHT22pin, dht22_wrapper);


// Thingspeak.com API
TCPClient client;
const char * WRITEKEY = THINGSPEAK_API_WRITEKEY;
const char * serverName = "api.thingspeak.com";
IPAddress server = {184,106,153,149};

// 1.3" 12864 OLED with SH1106 and Simplified Chinese IC for Spark Core
int Rom_CS = A2;
unsigned long  fontaddr=0;
char dispCS[32];

/****************************************************************************
*****************************************************************************
****************************   OLED Driver  *********************************
*****************************************************************************
****************************************************************************/


/*****************************************************************************
 Funtion    :   OLED_WrtData
 Description:   Write Data to OLED
 Input      :   byte8 ucCmd  
 Output     :   NONE
 Return     :   NONE
*****************************************************************************/
void transfer_data_lcd(byte ucData)
{
   Wire.beginTransmission(0x78 >> 1);     
   Wire.write(0x40);      //write data
   Wire.write(ucData);
   Wire.endTransmission();
}

/*****************************************************************************
 Funtion    :   OLED_WrCmd
 Description:   Write Command to OLED
 Input      :   byte8 ucCmd  
 Output     :   NONE
 Return     :   NONE
*****************************************************************************/
void transfer_command_lcd(byte ucCmd)
{
   Wire.beginTransmission(0x78 >> 1);            //Slave address,SA0=0
   Wire.write(0x00);      //write command
   Wire.write(ucCmd); 
   Wire.endTransmission();
}


/* OLED Initialization */
void initial_lcd()
{
  digitalWrite(Rom_CS, HIGH);
  // Wire.enableDMAMode(true);
  Wire.begin();
  delay(20);        
  transfer_command_lcd(0xAE);   //display off
  transfer_command_lcd(0x20); //Set Memory Addressing Mode  
  transfer_command_lcd(0x10); //00,Horizontal Addressing Mode;01,Vertical Addressing Mode;10,Page Addressing Mode (RESET);11,Invalid
  transfer_command_lcd(0xb0); //Set Page Start Address for Page Addressing Mode,0-7
  transfer_command_lcd(0xc8); //Set COM Output Scan Direction
  transfer_command_lcd(0x00);//---set low column address
  transfer_command_lcd(0x10);//---set high column address
  transfer_command_lcd(0x40);//--set start line address
  transfer_command_lcd(0x81);//--set contrast control register
  transfer_command_lcd(0x7f);
  transfer_command_lcd(0xa1);//--set segment re-map 0 to 127
  transfer_command_lcd(0xa6);//--set normal display
  transfer_command_lcd(0xa8);//--set multiplex ratio(1 to 64)
  transfer_command_lcd(0x3F);//
  transfer_command_lcd(0xa4);//0xa4,Output follows RAM content;0xa5,Output ignores RAM content
  transfer_command_lcd(0xd3);//-set display offset
  transfer_command_lcd(0x00);//-not offset
  transfer_command_lcd(0xd5);//--set display clock divide ratio/oscillator frequency
  transfer_command_lcd(0xf0);//--set divide ratio
  transfer_command_lcd(0xd9);//--set pre-charge period
  transfer_command_lcd(0x22); //
  transfer_command_lcd(0xda);//--set com pins hardware configuration
  transfer_command_lcd(0x12);
  transfer_command_lcd(0xdb);//--set vcomh
  transfer_command_lcd(0x20);//0x20,0.77xVcc
  transfer_command_lcd(0x8d);//--set DC-DC enable
  transfer_command_lcd(0x14);//
  transfer_command_lcd(0xaf);//--turn on oled panel 

}

void lcd_address(byte page,byte column)
{

  transfer_command_lcd(0xb0 + column);   /* Page address */
  transfer_command_lcd((((page + 1) & 0xf0) >> 4) | 0x10);  /* 4 bit MSB */
  transfer_command_lcd(((page + 1) & 0x0f) | 0x00); /* 4 bit LSB */ 
}

void clear_screen()
{
  unsigned char i,j;
  digitalWrite(Rom_CS, HIGH); 
  for(i=0;i<8;i++)
  {
    transfer_command_lcd(0xb0 + i);
    transfer_command_lcd(0x00);
    transfer_command_lcd(0x10);
    for(j=0;j<132;j++)
    {
        transfer_data_lcd(0x00);
    }
  }

}

void display_128x64(byte *dp)
{
  unsigned int i,j;
  for(j=0;j<8;j++)
  {
    lcd_address(0,j);
    for (i=0;i<132;i++)
    { 
      if(i>=2&&i<130)
      {
          // Write data to OLED, increase address by 1 after each byte written
        transfer_data_lcd(*dp);
        dp++;
      }
    }

  }

}



void display_graphic_16x16(unsigned int page,unsigned int column,byte *dp)
{
  unsigned int i,j;

  digitalWrite(Rom_CS, HIGH);   
  for(j=2;j>0;j--)
  {
    lcd_address(column,page);
    for (i=0;i<16;i++)
    { 
      transfer_data_lcd(*dp);
      dp++;
    }
    page++;
  }
}


void display_graphic_8x16(unsigned int page,byte column,byte *dp)
{
  unsigned int i,j;
  
  for(j=2;j>0;j--)
  {
    lcd_address(column,page);
    for (i=0;i<8;i++)
    { 
      // Write data to OLED, increase address by 1 after each byte written
      transfer_data_lcd(*dp);
      dp++;
    }
    page++;
  }

}


/*
    Display a 5x7 dot matrix, ASCII or a 5x7 custom font, glyph, etc.
*/
    
void display_graphic_5x7(unsigned int page,byte column,byte *dp)
{
  unsigned int col_cnt;
  byte page_address;
  byte column_address_L,column_address_H;
  page_address = 0xb0 + page - 1;// 
  
  
  
  column_address_L =(column&0x0f);  // -1
  column_address_H =((column>>4)&0x0f)+0x10;
  
  transfer_command_lcd(page_address);     /*Set Page Address*/
  transfer_command_lcd(column_address_H); /*Set MSB of column Address*/
  transfer_command_lcd(column_address_L); /*Set LSB of column Address*/
  
  for (col_cnt=0;col_cnt<6;col_cnt++)
  { 
    transfer_data_lcd(*dp);
    dp++;
  }
}

/**** Send command to Character ROM ***/
void send_command_to_ROM( byte datu )
{
  SPI.transfer(datu);
}

/**** Read a byte from the Character ROM ***/
byte get_data_from_ROM( )
{
  byte ret_data=0;
  ret_data = SPI.transfer(255);
  return(ret_data);
}


/* 
*     Read continuously from ROM DataLen's bytes and 
*     put them into pointer pointed to by pBuff
*/

void get_n_bytes_data_from_ROM(byte addrHigh,byte addrMid,byte addrLow,byte *pBuff,byte DataLen )
{
  byte i;
  digitalWrite(Rom_CS, LOW);
  delayMicroseconds(100);
  send_command_to_ROM(0x03);
  send_command_to_ROM(addrHigh);
  send_command_to_ROM(addrMid);
  send_command_to_ROM(addrLow);

  for(i = 0; i < DataLen; i++ ) {
       *(pBuff+i) =get_data_from_ROM();
  }
  digitalWrite(Rom_CS, HIGH);
}


/******************************************************************/

void display_string_5x7(byte y,byte x,const char *text)
{
  unsigned char i= 0;
  unsigned char addrHigh,addrMid,addrLow ;
  while((text[i]>0x00))
  {
    
    if((text[i]>=0x20) &&(text[i]<=0x7e)) 
    {           
      unsigned char fontbuf[8];     
      fontaddr = (text[i]- 0x20);
      fontaddr = (unsigned long)(fontaddr*8);
      fontaddr = (unsigned long)(fontaddr+0x3bfc0);     
      addrHigh = (fontaddr&0xff0000)>>16;
      addrMid = (fontaddr&0xff00)>>8;
      addrLow = fontaddr&0xff;

      get_n_bytes_data_from_ROM(addrHigh,addrMid,addrLow,fontbuf,8);/*取8个字节的数据，存到"fontbuf[32]"*/
      
      display_graphic_5x7(y,x+1,fontbuf);/*显示5x7的ASCII字到LCD上，y为页地址，x为列地址，fontbuf[]为数据*/
      i+=1;
      x+=6;
    }
    else
    i++;  
  }
  
}


/****************************************************************************
*****************************************************************************
****************************  Data Upload   *********************************
*****************************************************************************
****************************************************************************/

void sendToThingSpeak(const char * key, String mesg)
{
    noInterrupts();
    client.stop();
    RGB.control(true);
    RGB.color(0,255,0);
    int lt = millis();
    display_string_5x7(6,1,"Wifi");

    while (!WiFi.ready() && (millis() - lt < TIMEOUT)) {
        display_string_5x7(6,1,"Wifi...X");
        delay(1000);
    }
    while (!client.connect(server,80) && (millis() - lt < TIMEOUT)) {
        display_string_5x7(6,1,"Connecting Server...");
        delay(15000);
    }
    if (millis() - lt < TIMEOUT) {
        display_string_5x7(6,1,"Sending...         ");
        client.print("POST /update");
        client.println(" HTTP/1.1");
        client.print("Host: ");
        client.println(serverName);
        client.println("User-Agent: Spark");
        client.println("Connection: close");
        client.print("X-THINGSPEAKAPIKEY: ");
        client.println(key);
        client.println("Content-Type: application/x-www-form-urlencoded");
        client.print("Content-length: ");
        client.println(mesg.length());
        client.println();
        client.println(mesg);
        client.flush();
        display_string_5x7(6,1,"Waiting resp  ");
        lt = millis();
        while (client.available() == false && millis() - lt < 1000) {
            // Just wait up to 1000 millis
            delay(200);
        }
        display_string_5x7(6,1,"Reading resp  ");
        lt = millis();
        while (client.available() > 0 && millis() - lt < 10000) {
            client.read();
        }
        client.flush();
        client.stop();
        RGB.control(false);
        display_string_5x7(6,1,"Done          ");
    } 
    else{
        display_string_5x7(6,1,"Connection Failed");
        RGB.color(255,0,0);
    }
    interrupts();
}



int myVersion(String command)
{
    return version;
}

void publishReadings()
{
    if ((millis() - lastT) > (CHTUPDPERIOD * 1000)) {
        lastT = millis();
        readAM2321();
        char szEventInfo[64];
        sprintf(szEventInfo, "field1=%.1f&field2=%.2f", temp, humi);
        sendToThingSpeak(WRITEKEY,String(szEventInfo));
        Spark.sleep(CHTUPDPERIOD - 30);
    }
}

void backLightOn()
{
    trigger = millis() + BACKLDUR;
    transfer_command_lcd(0xaf);//--turn on oled panel 
    //display_string_5x7(6,1,"DISP ON ");
    refresh = true;
}

void backLightOff()
{
    //display_string_5x7(6,1,"DISP OFF");
    transfer_command_lcd(0xae);//--turn off oled panel 
    refresh=false;
}


// This wrapper is in charge of calling
// mus be defined like this for the lib work
void dht22_wrapper() {
  DHT22.isrCallback();
}

void readAM2321() {
  int lt = millis();
  int result;

  detachInterrupt(BackLightTrigger);
    DHT22.acquire();
  while (DHT22.acquiring() && (millis() - lt < TIMEOUT))
    ;
  //delay(500);
  //DHT22.acquire();
  //while (DHT22.acquiring())
  //  ;
  if (millis() - lt >= TIMEOUT) {
      result = IDDHTLIB_ERROR_ACQUIRING;
  } else {
      result = DHT22.getStatus();
  }
  switch (result)
  {
    case IDDHTLIB_OK:
      status = "OK";
      break;
    case IDDHTLIB_ERROR_CHECKSUM:
      status = "Checksum";
      break;
    case IDDHTLIB_ERROR_ISR_TIMEOUT:
      status = "ISR Time out";
      break;
    case IDDHTLIB_ERROR_RESPONSE_TIMEOUT:
      status = "Response time out";
      break;
    case IDDHTLIB_ERROR_DATA_TIMEOUT:
      status = "Data time out";
      break;
    case IDDHTLIB_ERROR_ACQUIRING:
      status = "Acquiring";
      break;
    case IDDHTLIB_ERROR_DELTA:
      status = "Delta time to small";
      break;
    case IDDHTLIB_ERROR_NOTSTARTED:
      status = "Not started";
      break;
    default:
      status = "Unknown";
      break;
  }

    temp = DHT22.getCelsius();
    humi = DHT22.getHumidity();
    attachInterrupt(BackLightTrigger, backLightOn, RISING);

}


/****************************************************************************
*****************************************************************************
**************************  Initialization  *********************************
*****************************************************************************
****************************************************************************/

void setup()
{
    Time.zone(8);

    pinMode(led, OUTPUT);
//    pinMode(BackLight, OUTPUT);
    pinMode(BackLightTrigger, INPUT);

  // Register a Spark variable here
    Spark.variable("Temperature", &temp, DOUBLE);
    Spark.variable("Pressure", &humi, DOUBLE);
    attachInterrupt(BackLightTrigger, backLightOn, RISING);

    SPI.begin();
    SPI.setBitOrder(MSBFIRST);
    SPI.setDataMode(SPI_MODE3);
    SPI.setClockDivider(SPI_CLOCK_DIV8);
    digitalWrite(Rom_CS, HIGH);

    initial_lcd();  
    clear_screen();    //clear all dots
    digitalWrite(led, LOW);
    display_string_5x7(4,1,"(c)2014 ioStation");
    sprintf(dispCS, "Version: %d", version);
    display_string_5x7(6,1,dispCS);
    delay(1000);
    clear_screen();    //clear all dots
    sprintf(dispCS, "v%d", version);
    display_string_5x7(7,1,dispCS);
    lastT = millis();
    backLightOn();
}

/****************************************************************************
*****************************************************************************
**************************  Main Proc Loop  *********************************
*****************************************************************************
****************************************************************************/

void loop()
{
    if (refresh) {
      Time.timeStr().substring(4,19).toCharArray(dispCS, 16);
      display_string_5x7(1,1,"     ");
      display_string_5x7(1,20,dispCS);

      readAM2321();
      status.toCharArray(dispCS, 18);
      display_string_5x7(6,1,dispCS);
    
      sprintf(dispCS, "T: %0.1f, H: %0.1f    ", temp, humi);
      display_string_5x7(3,1,dispCS);
    }

    publishReadings();
    //backLightOn();
    if (trigger > 0 && millis() > trigger) {
        trigger = 0;
        backLightOff();
    }


    delay(500);
}





