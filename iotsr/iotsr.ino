#include <EEPROM.h>
#include <Adafruit_ESP8266.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>



#include <SoftwareSerial.h>
#define SSID "arduino"
#define PASS "cristina"
#define DST_IP "xtudionet.com" 
SoftwareSerial dbgSerial(11, 12); // RX, TX // here i tried with 10,11 and 2,3

// A custom glyph (a smiley)...
static const byte glyph[] = { B00010000, B00110100, B00110000, B00110100, B00010000 };


// pin 7 - Serial clock out (SCLK)
// pin 6 - Serial data out (DIN)
// pin 5 - Data/Command select (D/C)
// pin 4 - LCD chip select (CS)
// pin 3 - LCD reset (RST)
Adafruit_PCD8544 display = Adafruit_PCD8544(7, 6, 5, 4, 3);

String readString;
String searchString;
String Html;
String GET = "GET http://xtudionet.com/status.php";
boolean waitfordospuntos;
boolean readHtml;
char c;
const int EEPROM_MIN_ADDR = 0;
const int EEPROM_MAX_ADDR = 511;

//const int buttonConf = 2;


void setup()
{   
  
  String PassRead;
  
      // leemos la config de la eprom
      //eeprom_read_string( 5 , PassRead , 20 );
      
     // pinMode(buttonConf, INPUT);
      
      display.begin(); 
      display.setContrast(60);
      display.clearDisplay();   // clears the screen and buffer
      display.println("  iniciando .."); 
      display.display();
      
       boolean buttonConfState;
  //buttonConfState = digitalRead(buttonConf);
   
   // comprobamos si entramos a modo conf
   //if (buttonConfState == HIGH) {
   if (1 == 1) { 
     
    readSerialConf();
    
    }
      
    // Open serial communications and wait for port to open:
    
    Serial.begin(115200);
    
    dbgSerial.begin(115200); //can't be faster than 19200 for softserial
    
    Serial.setTimeout(5000);
    
    dbgSerial.println("Xtudionet Wifi");
    
    //test if the module is ready
    Serial.println("AT+RST");
    
    delay(350);
    
    if(Serial.find("ready"))
    {
      dbgSerial.println("Module is ready");
      display.println("Modulo Ok.."); 
      display.display();
    }
    else
    {
      dbgSerial.println("Module have no response.");
      display.println("Modulo KO.."); 
      display.display();
    }
    
    delay(350);
    
    //connect to the wifi
    boolean connected=false;
    display.print("WIFI "); 
    display.display();
    for(int i=0;i<5;i++)
    {
      display.print("."); 
      display.display();
      if(connectWiFi())
      {
        connected = true;
        display.println("OK"); 
        display.display();
        break;
      }
    }
    if (!connected){while(1);}
  
    delay(1000);
        
    //set the single connection mode
    Serial.println("AT+CIPMUX=0");

}

boolean eeprom_is_addr_ok(int addr) {
  return ((addr >= EEPROM_MIN_ADDR) && (addr <= EEPROM_MAX_ADDR));
}

boolean eeprom_write_bytes(int startAddr, const byte* array, int numBytes) {
  // counter
  int i;

  // both first byte and last byte addresses must fall within
  // the allowed range  
  if (!eeprom_is_addr_ok(startAddr) || !eeprom_is_addr_ok(startAddr + numBytes)) {
    return false;
  }

  for (i = 0; i < numBytes; i++) {
    EEPROM.write(startAddr + i, array[i]);
  }

  return true;
}

boolean eeprom_read_bytes(int startAddr, byte array[], int numBytes) {
  int i;

  // both first byte and last byte addresses must fall within
  // the allowed range  
  if (!eeprom_is_addr_ok(startAddr) || !eeprom_is_addr_ok(startAddr + numBytes)) {
    return false;
  }

  for (i = 0; i < numBytes; i++) {
    array[i] = EEPROM.read(startAddr + i);
  }

  return true;
}

boolean eeprom_write_string(int addr, const char* string) {
  // actual number of bytes to be written
  int numBytes;

  // we'll need to write the string contents
  // plus the string terminator byte (0x00)
  numBytes = strlen(string) + 1;

  return eeprom_write_bytes(addr, (const byte*)string, numBytes);
}

boolean eeprom_read_string(int addr, char* buffer, int bufSize) {
  // byte read from eeprom
  byte ch;

  // number of bytes read so far
  int bytesRead;

  // check start address
  if (!eeprom_is_addr_ok(addr)) {
    return false;
  }

  // how can we store bytes in an empty buffer ?
  if (bufSize == 0) {
    return false;
  }

  // is there is room for the string terminator only,
  // no reason to go further
  if (bufSize == 1) {
    buffer[0] = 0;
    return true;
  }

  // initialize byte counter
  bytesRead = 0;

  // read next byte from eeprom
  ch = EEPROM.read(addr + bytesRead);

  // store it into the user buffer
  buffer[bytesRead] = ch;

  // increment byte counter
  bytesRead++;

  // stop conditions:
  // - the character just read is the string terminator one (0x00)
  // - we have filled the user buffer
  // - we have reached the last eeprom address
  while ( (ch != 0x00) && (bytesRead < bufSize) && ((addr + bytesRead) <= EEPROM_MAX_ADDR) ) {
    // if no stop condition is met, read the next byte from eeprom
    ch = EEPROM.read(addr + bytesRead);

    // store it into the user buffer
    buffer[bytesRead] = ch;

    // increment byte counter
    bytesRead++;
  }

  // make sure the user buffer has a string terminator
  // (0x00) as its last byte
  if ((ch != 0x00) && (bytesRead >= 1)) {
    buffer[bytesRead - 1] = 0;
  }

  return true;
}



// spliting a string and return the part nr index split by separator
String getStringPartByNr(String data, char separator, int index) {
    int stringData = 0;        //variable to count data part nr 
    String dataPart = "";      //variable to hole the return text

    for(int i = 0; i<data.length()-1; i++) {    //Walk through the text one letter at a time

        if(data[i]==separator) {
            //Count the number of times separator character appears in the text
            stringData++;

        }else if(stringData==index) {
            //get the text when separator is the rignt one
            dataPart.concat(data[i]);

        }else if(stringData>index) {
            //return text and stop if the next separator appears - to save CPU-time
            return dataPart;
            break;

        }

    }
    //return text if this is the last part
    return dataPart;
}


void readSerialConf()
{
  String startPassTag = "-pass-";
  String endPassTag = "/pass/";
  String startSsidTag = "-ssid-";
  String endSsidTag = "/ssid/";
  boolean readPass;
  boolean readSsid;
  String Pass;
  String Ssid;
  
    // inicamos el puerto serie
    Serial.begin(9600);
    
    
    
    display.println("lectura conf serie");
    display.display();
    while (1==1)
    {
       display.clearDisplay();
          
           
         
         while (Serial.available()) { 
           
           
           
          char c = Serial.read();
          
          if (c == -1)
          {
            display.print(".");
            }  
          //display.print(c);
          display.display();
          readString += c;
         
         
          
       }
      
  }
  
}

void SaveToEprom()
{
  // eeprom_write_string(5, const char* string);
  
}

void loop()
{
 
 
 static int counter;
 
  delay(200);
  
  counter++;
  
  readSerialConf();

String cmd = "AT+CIPSTART=\"TCP\",\"";
cmd += DST_IP;
cmd += "\",80";
Serial.println(cmd);
  delay(1000);
dbgSerial.println(cmd);

if(Serial.find("Error")) return;

display.clearDisplay();
display.println("Procesando.");
display.display();
delay(350);

  cmd = GET;
  cmd += "\r\n";
  Serial.print("AT+CIPSEND=");
  Serial.println(cmd.length());
  
  if(Serial.find(">")){
    
    dbgSerial.print(">");
    dbgSerial.print(cmd);
    Serial.print(cmd);
    
   delay(350);
   
   searchString = "+IPD,";
   waitfordospuntos = false;
   readHtml = false ;
    
  while (Serial.available()) {
    delay(3); 
    char c = Serial.read();
   // display.print(c);
    display.display();
    readString += c;
    if (readString.endsWith(searchString))
    {
      waitfordospuntos = true ; 
    }
    if (readHtml== true)
    {
      Html += c ;
     }
    if (waitfordospuntos == true )
    {
      if (  c == ':')
        readHtml= true;
    }
    
    
  }
  if (readString.length() >0) {
    
    dbgSerial.println(readString);
    
    if (readString == "on")     
    {
     dbgSerial.println(readString);
    }
    if (readString == "off")
    {
      dbgSerial.println(readString);
    }
    readString="";
   } 
    
  }else{
    Serial.print("AT+CIPCLOSE");
    dbgSerial.print("AT+CIPCLOSE");
  }
  
  if(Serial.find("OK")){
    dbgSerial.println("RECEIVED: OK");
  }
  else if(Serial.find("html")){
    dbgSerial.println("RECEIVED: HTML");
  }
  else if(Serial.find("Error")){
    dbgSerial.println("RECEIVED: Error");
  }
  delay(350);
  
  

   display.print("Peticion:");
   display.print(counter);
   display.println();
   display.print("Respuesa:");
   display.print(Html);
   
   Html = "";
   
   display.display();
   delay(200);
  //Serial.find("+IPD");
  
  dbgSerial.println("====");

  display.clearDisplay();   // clears the screen and buffer
}


boolean connectWiFi()
{
Serial.println("AT+CWMODE=1");
String cmd="AT+CWJAP=\"";
cmd+=SSID;
cmd+="\",\"";
cmd+=PASS;
cmd+="\"";
dbgSerial.println(cmd);
Serial.println(cmd);
delay(2000);
if(Serial.find("OK"))
{
dbgSerial.println("OK, Connected to WiFi.");
return true;
}else
{
dbgSerial.println("Can not connect to the WiFi.");
return false;
}
}
