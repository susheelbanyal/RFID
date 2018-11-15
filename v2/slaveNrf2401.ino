#define DEBUG
#include <Wire.h>
#include <SPI.h>
#include <RF24.h> //http://maniacbug.github.com/RF24
#include <printf.h>
RF24 radio(9, 10);
void setup() {
  Wire.begin(8); // join i2c bus with address #4
  SPI.begin(); // Init SPI bus
  Wire.onReceive(receiveEvent); // register event
  Serial.begin(9600);  // initialize serial communication
#ifdef DEBUG
  // Initialise  serial communications channel with PC
  printf_begin();
  Serial.println(F("Serial communication started"));
#endif

  // initialize the NRF2401 Module
  radio.begin();
  radio.setPALevel(RF24_PA_MIN);
  radio.setChannel(0x76);
  radio.openWritingPipe(0xF0F0F0F0E1LL); 
  radio.enableDynamicPayloads();
  radio.powerUp();
  //radio.setDataRate(RF24_250KBPS);
  radio.printDetails();
  Serial.println(F("--- END STUFF ---"));

}

void loop () {
  delay(100);
}
void receiveEvent(int howMany)
{
  String data;
  String EmpId;
  String productId;
  boolean checkEmp = false;
  while (Wire.available() > 0) // loop through all but the last
  {
    char receivedData = Wire.read(); // receive byte as a character
    data.concat(receivedData);
  }

  for (int x = 0; x < data.length(); x++) {
    if (data[x] == '-') {
      checkEmp = true;
    } else {
      if (checkEmp) {
        EmpId.concat(data[x]);
      } else {
        productId.concat(data[x]);
      }
    }
  }

  Serial.println("=====EmpId====");
  Serial.println(EmpId);
  Serial.println("=====productId====");
  Serial.println(productId);
  delay(500);
  sendData(productId);
  
}


void sendData(String data) {
  // Length (with one extra character for the null terminator)
  int str_len = data.length() + 1;
  // Prepare the character array (the buffer)
  char char_array[str_len];
  // Copy it over
  Serial.println(data);
  Serial.println("=====start sending nrf====");
  data.toCharArray(char_array, str_len);
  
  bool s = radio.write(&char_array, sizeof(char_array));
  Serial.println(s);
  Serial.println("=====end sending nrf====");
  radio.powerDown();
  radio.powerUp();
}
