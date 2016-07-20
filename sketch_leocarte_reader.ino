String ARDUINO_ID = "esup-nfc-tag-arduino-num-1";

#include <LiquidCrystal_I2C.h>


#include <SPI.h>
#include <Ethernet.h>

#include <Wire.h>
#include <Adafruit_PN532.h>

// If using the breakout with SPI, define the pins for SPI communication.
#define PN532_SCK  (2)
#define PN532_MOSI (3)
#define PN532_SS   (4)
#define PN532_MISO (5)

// If using the breakout or shield with I2C, define just the pins connected
// to the IRQ and reset lines.  Use the values below (2, 3) for the shield!
#define PN532_IRQ   (2)
#define PN532_RESET (3)  // Not connected by default on the NFC Shield

// Uncomment just _one_ line below depending on how your breakout or shield
// is connected to the Arduino:

// Use this line for a breakout with a software SPI connection (recommended):
Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);

// Use this line for a breakout with a hardware SPI connection.  Note that
// the PN532 SCK, MOSI, and MISO pins need to be connected to the Arduino's
// hardware SPI SCK, MOSI, and MISO pins.  On an Arduino Uno these are
// SCK = 13, MOSI = 11, MISO = 12.  The SS line can be any digital IO pin.
//Adafruit_PN532 nfc(PN532_SS);

// Or use this line for a breakout or shield with an I2C connection:
//Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);

// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = { 0xDE, 0xDE, 0xDE, 0xEF, 0xEF, 0xEF };

char server[] = "esup-nfc-tag.univ-ville.fr";    

// Set the static IP address to use if the DHCP fails to assign
IPAddress ip(192, 168, 0, 1);

// Initialize the Ethernet client library
// with the IP address and port of the server
// that you want to connect to (port 80 is default for HTTP):
EthernetClient client;

const int LED_ROUGE = 8; 
const int LED_VERTE = 9; 
const int BUZZER = 6;

const int numTones = 2;
const int tonesOK[] = {880, 1760};
const int tonesKO[] = {988, 784};
const int tonesRETRY[] = {300, 300};

String uidStringLast = "";

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address

void setup() {

  pinMode(LED_ROUGE, OUTPUT); 
  pinMode(LED_VERTE, OUTPUT); 
  pinMode(BUZZER, OUTPUT);

  digitalWrite(LED_VERTE, LOW);
  digitalWrite(LED_ROUGE, HIGH);

  lcd.begin(16,2);   // initialize the lcd for 16 chars 2 lines, turn on backlight
  lcd.setCursor(0,0); //Start at character 4 on line 0
  
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }



   lcd.print("DHCP ....");

  // start the Ethernet connection:
  while (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP\n");
    // try to congifure using IP address instead of DHCP:
    // Ethernet.begin(mac, ip);
    lcd.setCursor(0, 1);
    lcd.print("DHCP failed - reset ...");
    signalErreur();
    delay(500);
    softReset();
  }
  
  lcd.setCursor(0,0);
  lcd.print("TRY HTTP ...");
  if (client.connect(server, 80)) {
      client.stop();
      client.flush();
  }

  lcd.setCursor(0,0);
  lcd.print("NFC part ...");

  nfc.begin();

Serial.print("dav");

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }
  // Got ok data, print it out!
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX); 
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC); 
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
  
  // configure board to read RFID tags
  nfc.SAMConfig();
  
  Serial.println("Waiting for an ISO14443A Card ...");
  digitalWrite(LED_ROUGE, LOW);

  delay(300);
  digitalWrite(LED_ROUGE, HIGH);
  digitalWrite(LED_VERTE, HIGH);
  delay(500);
  digitalWrite(LED_ROUGE, LOW);
  digitalWrite(LED_VERTE, LOW);

  lcd.setCursor(0,0);
  lcd.print("READY");
}

void loop() {

  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)

  lcd.clear();
  lcd.off();
    
  // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
  // 'uid' will be populated with the UID, and uidLength will indicate
  // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

  
  if (success) {

    lcd.on();
    
    // Display some basic information about the card
    Serial.println("Found an ISO14443A card");
    Serial.print("  UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
    Serial.print("  UID Value: ");
    nfc.PrintHex(uid, uidLength);
    Serial.println("");

    String uidString = PrintHexWithLeadingZeroes(uid, uidLength);
            
    if (uidStringLast!=uidString) {
      
      lcd.setCursor(0,0); 
          
      if(client.connect(server, 80)) {
        
        digitalWrite(LED_VERTE, HIGH);

        Serial.println("connected");
        // Make a HTTP request:
        String reqChar = String("GET /csn-ws?csn=" + uidString + "&arduinoId=" + ARDUINO_ID + " HTTP/1.1");
        client.println(reqChar);
        Serial.println(reqChar);
        client.println("Host: esup-nfc-tag.univ-ville.fr");
        client.println("Connection: close");
        client.println();

        String line = "";
        int lineNb = 0;
        boolean startRead = false; 
        boolean erreur = true; 

        while(client.connected())
          {
                while (client.available()) {
                  char c = client.read();
                  Serial.print(c);
                  if (c == '<' ) { //'<' is our begining character
                    startRead = true; //Ready to start reading the part 
                  } else if(startRead){
                    if(c != '>'){ //'>' is our ending character
                      if(c == '\n'){ 
                        if(lineNb==0 && line.equals("OK")) {
                          erreur = false; 
                        } 
                        if(lineNb>0)  {
                          lcd.setCursor(0, lineNb-1);
                          lcd.print(line + "             ");
                        }
                        line = "";
                        lineNb++;
                      } else {
                        line = line + c;
                      }
                    } else {
                      //got what we need here! We can disconnect now
                      lcd.setCursor(0, lineNb);
                      lcd.print(line  + "              ");
                      startRead = false;
                    }
                  }
                }
          }


          client.stop();
          client.flush();
          Serial.println();
          Serial.println("disconnecting OK.");
          
          if(erreur) {
            signalErreur();
          } else {
            signalSuccess();
          }
          
          delay(3000);
          lcd.clear();
          digitalWrite(LED_VERTE, LOW);
          digitalWrite(LED_ROUGE, LOW);
          
          uidStringLast = uidString;
         
      } else {
        // if you didn't get a connection to the server:
        Serial.println("connection failed");
        
        lcd.print("Pb connection");
        signalRetry();
        
        delay(3000);
        lcd.clear();
      }
      
     
      
    } else {

         lcd.print("Changez de carte");
         signalRetry();
      
         delay(3000);
         lcd.clear();
         digitalWrite(LED_VERTE, LOW);
         digitalWrite(LED_ROUGE, LOW);
    }
   
  }

}

// co de issu de http://forum.arduino.cc/index.php?topic=38107.0
String PrintHexWithLeadingZeroes(uint8_t *data, uint8_t length) // prints 8-bit data in hex
{
 char tmp[length*2+1];
 byte first ;
 int j=0;
 for (uint8_t i=0; i<length; i++) 
 {
   first = (data[i] >> 4) | 48;
   if (first > 57) tmp[j] = first + (byte)39;
   else tmp[j] = first ;
   j++;

   first = (data[i] & 0x0F) | 48;
   if (first > 57) tmp[j] = first + (byte)39; 
   else tmp[j] = first;
   j++;
 }
 tmp[length*2] = 0;
 return String(tmp);
}

void softReset() {
asm volatile ("  jmp 0");
}


void signalErreur() {
  digitalWrite(LED_VERTE, LOW);
  digitalWrite(LED_ROUGE, HIGH);
  for (int i = 0; i < numTones; i++) {
     tone(BUZZER, tonesKO[i]);
     delay(200);
   }
   noTone(BUZZER);
}

void signalSuccess() {
   for (int i = 0; i < numTones; i++) {
     tone(BUZZER, tonesOK[i]);
     delay(200);       
   }
   noTone(BUZZER);
}


void signalRetry() {
  digitalWrite(LED_VERTE, LOW);
  digitalWrite(LED_ROUGE, HIGH);
  for (int i = 0; i < numTones; i++) {
     tone(BUZZER, tonesRETRY[i]);
     delay(400);
   }
   noTone(BUZZER);
}

