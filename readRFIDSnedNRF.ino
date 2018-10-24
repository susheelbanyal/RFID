#define DEBUG

// LIBERARIES
#include <SPI.h>
#include <MFRC522.h>
#include <RF24.h>
#include <printf.h>

//#define RST_PIN         8          // Configurable, see typical pin layout above
//#define SS_PIN          2         // Configurable, see typical pin layout above

int RedLED = 4;// no authorize to scan
int GreenLED = 5; //ready to scan
int BlueLED = 6; //ready to scan

const byte numReaders = 2;
const byte ssPins[] = {2, 3};
const byte resetPin = 8;


MFRC522 mfrc522[numReaders];
//MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

bool rfid_tag_present_prev = false;
bool rfid_tag_present = false;
int _rfid_error_counter = 0;
bool _tag_found = false;


String EmployeeRFIDCode = "289708846";//"3802552960"; //3779262765
unsigned long delayStart = 0;
unsigned long AuthTime = 10000;
bool authorizeTimeFlag = false;
bool IsEmp = false;
/**
   Configuraton for NRF24l01 sensor
*/
// create RF24 radio object using selected CE and CSN pins
RF24 radio(9, 10);

/**
   Initialization
*/
void setup() {
#ifdef DEBUG
  // Initialise  serial communications channel with PC
  Serial.begin(9600);
  printf_begin();
  Serial.println(F("Serial communication started"));
#endif

  SPI.begin();      // Init SPI bus
  delayStart = millis();
  authorizeTimeFlag = false;

  pinMode(RedLED, OUTPUT);
  pinMode(GreenLED, OUTPUT);
  pinMode(BlueLED, OUTPUT);

  //digitalWrite(GreenLED, LOW);
  //digitalWrite(RedLED, HIGH);
  ledOnOff("block");

  // Initialize RFID readers
  // Note that SPI pins on the reader must always be connected to certain
  // Arduino Pins (on an Uno, MOSI=> pin11, MISO=> pin12, SCK=>pin13
  // The Slave Select(SS) pin and reset pin can be assigned to any pin
  //mfrc522.PCD_Init(SS_PIN, RST_PIN);    // Initialize MFRC522 Hardware For single RFID

  for (uint8_t i = 0; i < numReaders; i++) {
    mfrc522[i].PCD_Init(ssPins[i], resetPin);
    Serial.print(F("Reader #"));
    Serial.print(F(" initialised on pin "));
    Serial.print(String(ssPins[i]));
    Serial.print(F(". Antenna strength: "));
    Serial.print(mfrc522[i].PCD_GetAntennaGain());
    Serial.print(F(". Version : "));
    mfrc522[i].PCD_DumpVersionToSerial();
  }
  // initialize the NRF2401 Module
  radio.begin();
  radio.setPALevel(RF24_PA_MIN);
  radio.setChannel(0x76);
  radio.openWritingPipe(0xF0F0F0F0E1LL);
  radio.enableDynamicPayloads();
  radio.powerUp();
  radio.setDataRate(RF24_250KBPS);
  radio.printDetails();
  Serial.print(F("--- END STUFF ---"));

  //mfrc522.PCD_Init();   // Init MFRC522 Dont init here as we have 2 readers now
}

void loop() {
  rfid_tag_present_prev = rfid_tag_present;

  _rfid_error_counter += 1;
  if (_rfid_error_counter > 2) {
    _tag_found = false;
  }

  // Detect Tag without looking for collisions
  byte bufferATQA[2];
  byte bufferSize = sizeof(bufferATQA);
  String readRFID = "";

  for (uint8_t i = 0; i < numReaders; i++) {
    mfrc522[i].PCD_Init();
    // Reset baud rates
    mfrc522[i].PCD_WriteRegister(mfrc522[i].TxModeReg, 0x00);
    mfrc522[i].PCD_WriteRegister(mfrc522[i].RxModeReg, 0x00);
    // Reset ModWidthReg
    mfrc522[i].PCD_WriteRegister(mfrc522[i].ModWidthReg, 0x26);

    MFRC522::StatusCode result = mfrc522[i].PICC_RequestA(bufferATQA, &bufferSize);

    if (result == mfrc522[i].STATUS_OK) {
      if ( ! mfrc522[i].PICC_ReadCardSerial()) { //Since a PICC placed get Serial and continue
        return;
      }
      _rfid_error_counter = 0;
      _tag_found = true;

      readRFID = dump_byte_array(mfrc522[i].uid.uidByte, mfrc522[i].uid.size);

      // start the timer 
      delayStart = millis(); // again set the timer to now

      // Check if RFID is from Employee
      if (readRFID == EmployeeRFIDCode) {
        // here we got the EMP RFID CODE
        IsEmp = true;
        authorizeTimeFlag = true; // start the timer
        ledOnOff("authorize");// green light on
      }
    }
    rfid_tag_present = _tag_found; // not emp tag

  }//MFRC522 for loop end

  if (authorizeTimeFlag && ((millis() - delayStart) <= AuthTime)) { // still there is time
    if (readRFID != EmployeeRFIDCode) { // allow others card scan
      // rising edge
      if (rfid_tag_present && !rfid_tag_present_prev) {
        Serial.println("Tag found");
        Serial.println(F("Card UID: "));
        Serial.println(readRFID);
        Serial.println(sizeof(readRFID));
        Serial.println();
        ledOnOff("reading");
        sendData(readRFID);
      }
    }
  } else {
    authorizeTimeFlag = false; // Set false timer again
    ledOnOff("block");// red light on

  }

  // falling edge should not run for Emp
  if (!rfid_tag_present && rfid_tag_present_prev) {
    Serial.println("Tag gone");
    if (authorizeTimeFlag && ((millis() - delayStart) <= AuthTime)) { //still have time
      ledOnOff("authorize");
      sendData("stop");// still have time to send the stop signal
    } else {
      ledOnOff("block");
    }
    
  }

}// loop fun

void ledOnOff(String status) {
  if (status == "authorize") { // authorize to scan card
    digitalWrite(GreenLED, HIGH);
    digitalWrite(RedLED, LOW);
    digitalWrite(BlueLED, LOW);
  } else if (status == "reading") { //reading tag
    digitalWrite(BlueLED, HIGH);
    digitalWrite(GreenLED, LOW);
    digitalWrite(RedLED, LOW);
  } else { //block [dont allow to scan]
    digitalWrite(BlueLED, LOW);
    digitalWrite(GreenLED, LOW);
    digitalWrite(RedLED, HIGH);
  }

}

void sendData(String data) {
  // Length (with one extra character for the null terminator)
  int str_len = data.length() + 1;

  // Prepare the character array (the buffer)
  char char_array[str_len];

  // Copy it over
  data.toCharArray(char_array, str_len);
  radio.write(&char_array, sizeof(char_array));
  radio.powerDown();
  radio.powerUp();
}


String dump_byte_array(byte *buffer, byte bufferSize) {
  //   String s;
  unsigned long uiddec = 0;
  //    unsigned long temp;
  char uid[8];
  for (byte m = (bufferSize > 4 ? (bufferSize - 4) : 0); m < bufferSize; m++) { //берем только последние 4 байта и переводим в десятичную систему
    unsigned long p = 1;
    for (int k = 0; k < bufferSize - m - 1; k++) {
      p = p * 256;
    }
    uiddec += p * buffer[m];
    //   s = s + (buffer[m] < 0x10 ? "0" : "");
    //   s = s + String(buffer[m], HEX);
  }
  //    s.toCharArray(uid, 8);
  return String(uiddec);
}
