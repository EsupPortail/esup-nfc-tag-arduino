#include "Desfire.h"
#include "Buffer.h"
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <Ethernet.h>
#include <Wire.h>

/******************************************************************/
/*                      CONFIG                                    */
/******************************************************************/

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEF };
String ARDUINO_ID = "esup-nfc-tag-arduino-num-1";
char* server = "esup-nfc-tag.univ-ville.fr";

/******************************************************************/


//#define USE_SOFTWARE_SPI;
// This Arduino / Teensy pin is connected to the PN532 RSTPDN pin (reset the PN532)
// When a communication error with the PN532 is detected the board is reset automatically.
#define RESET_PIN         6
// The software SPI SCK  pin (Clock)
#define SPI_CLK_PIN       2
// The software SPI MISO pin (Master In, Slave Out)
#define SPI_MISO_PIN      5
// The software SPI MOSI pin (Master Out, Slave In)
#define SPI_MOSI_PIN      3
// The software SPI SSEL pin (Chip Select)
#define SPI_CS_PIN        4
// If using the breakout or shield with I2C, define just the pins connected
// to the IRQ and reset lines.  Use the values below (2, 3) for the shield!
#define PN532_IRQ   (2)
#define PN532_RESET (3)  // Not connected by default on the NFC Shield

#define RF_OFF_INTERVAL  1000


Desfire gi_PN532;
uint64_t   gu64_LastID     = 0;  
bool       gb_InitSuccess  = false; // true if the PN532 has been initialized successfully
int   mu8_LastPN532Error   = 0;    

// Initialize the Ethernet client library
// with the IP address and port of the server
// that you want to connect to (port 80 is default for HTTP):
EthernetClient client;

const int LED_ROUGE = 8; 
const int LED_VERTE = 9; 
const int BUZZER = 7;
const int process = 50;
const int success = 1400;
const int error = 200;

String uidStringLast = "";

char location[16];
int locationSize = 0;
boolean startLocation=false;
int cmdSize = 0;
int paramSize = 0;
int msgSize = 0;

        
LiquidCrystal_I2C lcd(0x3F, 16, 2);  // Set the LCD I2C address

struct kCard
{
    byte     u8_UidLength;   // UID = 4 or 7 bytes
    byte     u8_KeyVersion;  // for Desfire random ID cards
    bool      b_PN532_Error; // true -> the error comes from the PN532, false -> crypto error
    eCardType e_CardType;    
};



void setup() {
  // put your setup code here, to run once:
  SerialClass::Begin(115200);

  pinMode(LED_ROUGE, OUTPUT); 
  pinMode(LED_VERTE, OUTPUT); 
  pinMode(BUZZER, OUTPUT);
  
  digitalWrite(BUZZER, LOW);
  digitalWrite(LED_VERTE, LOW);
  digitalWrite(LED_ROUGE, HIGH);

  lcd.init();   // initialize the lcd for 16 chars 2 lines, turn on backlight
  lcd.clear();
  lcd.backlight();

  // start the Ethernet connection:
  while (Ethernet.begin(mac) == 0) {
    delay(500);
  }

  delay(1000);
  if (client.connect(server, 80)) {
        client.print("GET /nfc-ws/location/?numeroId=");
        client.print(ARDUINO_ID);
        client.println(" HTTP/1.1");  
        client.print("Host: ");
        client.println(server);
        client.println("Connection: close");
        client.println();
        while(client.connected()) {
          while (client.available()) {
          char c = client.read();
          if(startLocation){
            if(c=='}') break;
            location[locationSize] = c;
            locationSize++;
            if(locationSize>15) {
              location[locationSize] = '\n';
              locationSize++;    
            }
          }
          if(c=='{') startLocation=true;
          }
        }
    
      client.stop();
      client.flush();
      lcd.clear();
      lcd.print(location);
  }else{
      delay(2000);
  }
  
  gi_PN532.InitSoftwareSPI(SPI_CLK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, SPI_CS_PIN, RESET_PIN);
  gi_PN532.SetDebugLevel(0);
  InitReader(false);
  lcd.noBacklight();
  digitalWrite(LED_VERTE, HIGH);
  digitalWrite(LED_ROUGE, LOW);
}
 
void loop() {

  uint64_t u64_StartTick = Utils::GetMillis64();
  static uint64_t u64_LastRead = 0;
    if (gb_InitSuccess)
      {
          // Turn on the RF field for 100 ms then turn it off for one second (RF_OFF_INTERVAL) to safe battery
          if ((int)(u64_StartTick - u64_LastRead) < RF_OFF_INTERVAL)
              return;
      }else{
        
        InitReader(true); // flash red LED for 2.4 seconds
        
    }
    uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
    kCard k_Card;
    if (!ReadCard(uid, &k_Card))
    {
        if (k_Card.b_PN532_Error) InitReader(true);            
        
    }
    // No card present in the RF field
    if (k_Card.u8_UidLength == 0) 
    {
        gu64_LastID = 0;
    }else{

      String result = "";
      String jSessionId = "";
      while(true){
       if(client.connect(server, 80)){
        client.print("GET /desfire-ws/?result=");
        client.print(result);
        client.print("&numeroId=");
        client.print(ARDUINO_ID);
        client.println(" HTTP/1.1");  
        client.print("Host: ");
        client.println(server);
        client.print("Cookie: JSESSIONID=");
        client.println(jSessionId);
        client.println("Connection: close");
        client.println();

        result="";
        int nbChar = 0;
        boolean startRead = 0; 
        int currentParam = 0;
        char cmd[2];
        char param[64];
        String code = "";
        String sizeReturn = "";
        jSessionId = "";
        cmdSize=0;
        paramSize = 0;
        String msg = "";
        msgSize=0;
                                        
        while(client.connected()) {
          while (client.available()) {
            char c = client.read();
            Serial.print(c);
                   if (c == '{' && !startRead) {
                      startRead = 1;
                    } 
                    if(startRead) {
                      if(c == '"') {
                        currentParam++;
                      } else {
                        if(currentParam==3) {
                          code += c;
                        } else 
                        if(currentParam==7) {
                          cmd[cmdSize] = c;
                          cmdSize++;
                        } else 
                        if(currentParam==11) {
                          param[paramSize] = c;
                          paramSize++;
                        } else 
                        if(currentParam==15) {
                          sizeReturn += c;
                        } else
                        if(currentParam==19) {
                          jSessionId += c;
                        } else
                        if(currentParam==27) {
                          msg += c;
                          if(msgSize>15) break;
                        }                        
                      }
                      if(c == '}'){
                        startRead=0;
                        break;
                       }
                    }
                }
        }
          client.stop();   
          client.flush();
           int nbByteReturn = sizeReturn.toInt();
      
           byte u8_RecvBuf[nbByteReturn];
                      DESFireStatus e_Status;

           int returnStatus = sendApdu(cmd, cmdSize, param, paramSize, &e_Status, u8_RecvBuf, nbByteReturn);

 
           String resultData;
           for (uint32_t i=0; i < sizeof(u8_RecvBuf); i++){
                char s8_RecvBuf[2];
                sprintf(s8_RecvBuf, "%02X", u8_RecvBuf[i]);
                resultData+=s8_RecvBuf;
           }
           char resultStatus[2];
           sprintf(resultStatus, "%02X", e_Status);
           result = resultData+"91"+resultStatus; 
           Serial.println(result);
           lcd.backlight();
           lcd.clear();
           if((((String) resultStatus == "00" || (String) resultStatus == "AF") && returnStatus>-1) || code=="END"){
            if(code=="END"){

              signalSuccess();
              lcd.print("Ok");
              lcd.setCursor(0,1);
              lcd.print(msg);
              //Serial.println("Ok");
              //Serial.println(msg);
              delay(2000);
              lcd.clear();
              lcd.print(location);
              lcd.noBacklight();
              k_Card.u8_UidLength=0;
              break;
            }else{
              signalProcess();
              lcd.print("En cours");
              lcd.setCursor(0,1);
              lcd.print(msg);
            }
           }else{
            signalErreur();
            lcd.print("Erreur ");
            lcd.print(resultStatus);
            lcd.setCursor(0,1);
            lcd.print(msg);
            delay(2000);
            lcd.clear();
            lcd.print(location);
            lcd.noBacklight();            
            k_Card.u8_UidLength=0;
            break;  
           }
         }
      }
    }

    // Turn off the RF field to save battery
    // When the RF field is on,  the PN532 board consumes approx 110 mA.
    // When the RF field is off, the PN532 board consumes approx 18 mA.
    gi_PN532.SwitchOffRfField();
    u64_LastRead = Utils::GetMillis64();
    delay(1000);
    digitalWrite(LED_VERTE, LOW);
    digitalWrite(LED_ROUGE, LOW);
  
}

void InitReader(bool b_ShowError)
{

    do // pseudo loop (just used for aborting with break;)
    {
        gb_InitSuccess = false;
      
        // Reset the PN532
        gi_PN532.begin(); // delay > 400 ms
        byte IC, VersionHi, VersionLo, Flags;
        if (!gi_PN532.GetFirmwareVersion(&IC, &VersionHi, &VersionLo, &Flags))
            break;
        // Set the max number of retry attempts to read from a card.
        // This prevents us from waiting forever for a card, which is the default behaviour of the PN532.
        if (!gi_PN532.SetPassiveActivationRetries())
            break;
        
        // configure the PN532 to read RFID tags
        if (!gi_PN532.SamConfig())
            break;
    
        gb_InitSuccess = true;
    }
    while (false);  
}

bool ReadCard(byte u8_UID[8], kCard* pk_Card)
{
    memset(pk_Card, 0, sizeof(kCard));
  
    if (!gi_PN532.ReadPassiveTargetID(u8_UID, &pk_Card->u8_UidLength, &pk_Card->e_CardType))
    {
        pk_Card->b_PN532_Error = true;
        return false;
    }

    if (pk_Card->e_CardType == CARD_DesRandom) // The card is a Desfire card in random ID mode
    {
        #if USE_DESFIRE
            if (!AuthenticatePICC(&pk_Card->u8_KeyVersion))
                return false;
        
            // replace the random ID with the real UID
            if (!gi_PN532.GetRealCardID(u8_UID))
                return false;

            pk_Card->u8_UidLength = 7; // random ID is only 4 bytes
        #else
            //Utils::Print("Cards with random ID are not supported in Classic mode.\r\n");
            return false;    
        #endif
    }
    return true;
}

uint8_t* convertCharStarHexToInt(char* str, int strSize){
  uint8_t* strInt = (uint8_t*) malloc(strSize/2 * sizeof(uint8_t));
  for (uint32_t i=0; i < strSize; i+=2)
  {
    char byteChar[2];
    byteChar[0] = str[i];
    byteChar[1] = str[i+1];
    strInt[i/2] = (int) strtol(byteChar , NULL, 16);      
    //Serial.print((String) byteChar);
  }
  return strInt;
}

int sendApdu(char* cmdStr, int cmdSize, char* paramStr, int paramSize, DESFireStatus* e_Status, byte* u8_RecvBuf, int s32_RecvSize){
        uint8_t* cmdInt = convertCharStarHexToInt(cmdStr, cmdSize);
        TX_BUFFER(i_cmd, 1);
        i_cmd.AppendUint8(cmdInt[0]);  
        free(cmdInt);
        uint8_t* paramInt = convertCharStarHexToInt(paramStr, paramSize);
        TX_BUFFER(i_Params,  paramSize/2);
        for (uint32_t i=0; i <  paramSize/2; i++)
        {
            i_Params.AppendUint8(paramInt[i]);  
        }        
        free(paramInt);
        int s32_Read = gi_PN532.DataExchange(&i_cmd, &i_Params, u8_RecvBuf, s32_RecvSize, e_Status, MAC_None);
        return s32_Read;
}

void myTone(byte pin, uint16_t frequency, uint16_t duration)
{ // input parameters: Arduino pin number, frequency in Hz, duration in milliseconds
  unsigned long startTime=millis();
  unsigned long halfPeriod= 1000000L/frequency/2;
  pinMode(pin,OUTPUT);
  while (millis()-startTime< duration)
  {
    digitalWrite(BUZZER,HIGH);
    delayMicroseconds(halfPeriod);
    digitalWrite(BUZZER,LOW);
    delayMicroseconds(halfPeriod);
  }
  pinMode(pin,INPUT);
}


void signalErreur() {
  digitalWrite(LED_VERTE, LOW);
  digitalWrite(LED_ROUGE, HIGH);
  myTone(BUZZER, error, 50);
}

void signalProcess() {
  digitalWrite(LED_VERTE, HIGH);
  digitalWrite(LED_ROUGE, HIGH);
  myTone(BUZZER, process, 1);
}

void signalSuccess() {
  digitalWrite(LED_VERTE, HIGH);
  digitalWrite(LED_ROUGE, LOW);
  myTone(BUZZER, success, 50);
}
