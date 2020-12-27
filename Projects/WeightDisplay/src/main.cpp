/**************************************************************************
 Arduino Nano clone based scale
 ATmega328P (Old Bootloader)
 **************************************************************************/
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <SSD1306Ascii.h>
#include <SSD1306AsciiWire.h>
#include <HX711.h>
#include <EEPROM.h>

// 0X3C+SA0 - 0x3C or 0x3D
#define I2C_ADDRESS 0x3C

// Define proper RST_PIN if required.
#define RST_PIN -1

const int LOADCELL_DOUT_PIN = A2;
const int LOADCELL_SCK_PIN = A1;
long LOADCELL_DIVIDER; // Calibration
//const long LOADCELL_DIVIDER = -2230; // Calibration for Kg
//const long LOADCELL_DIVIDER = -2230; // Calibration for Lbs

const int eeAddress = 0;

HX711 scale;
SSD1306AsciiWire oled;

void calmode(void);

void setup() {
  Serial.begin(9600);
  
  Wire.begin();
  Wire.setClock(400000L);
  
  // Display setup
  #if RST_PIN >= 0
    oled.begin(&Adafruit128x64, I2C_ADDRESS, RST_PIN);
  #else // RST_PIN >= 0
    oled.begin(&Adafruit128x64, I2C_ADDRESS);
  #endif // RST_PIN >= 0

  // Fetch eeprom value
  EEPROM.get(eeAddress, LOADCELL_DIVIDER);

  Serial.println("Type 'c' to enter calibration mode");
  Serial.println("Initializing the scale");
  Serial.print("Calibration factor: ");
  Serial.println(LOADCELL_DIVIDER);
  
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(LOADCELL_DIVIDER);
  scale.tare();	 // reset the scale to zero

  delay(2000); // Pause for 2 seconds
  
  // Clear the buffer
  oled.clear();}

void loop() {
  // Read serial if ready
  if(Serial.available()) {
    if(Serial.read() == 'c')
      calmode();
  }
  
  // Read scale if ready
  if (scale.is_ready()) {
    oled.home();
    oled.setFont(Adafruit5x7);
    oled.set2X();
    oled.println(F("Weight"));
    
    //oled.setFont(fixednums8x16);
    oled.setFont(lcdnums14x24);
    oled.set1X();
    oled.setRow(3); // 8 pixel row
    oled.print(scale.get_units(),1);
    //oled.print(F("4999.9"));
    oled.clearToEOL();
    oled.println(F(""));
        
    oled.setFont(Adafruit5x7);
    oled.set1X();
    oled.setCursor(105,7); // col pixels x 8 pixel row
    oled.println(F("Lbs"));

    Serial.print("HX711 reading: ");
    Serial.println(scale.get_units(),1);
  } else {
    Serial.println("HX711 not found.");
  }

  delay(1000);
}

void calmode(void) {
  char temp = ' ';
  long calibration_factor = LOADCELL_DIVIDER;
  
  Serial.println("HX711 calibration sketch");
  Serial.println("Remove all weight from scale");
  Serial.println("After readings begin, place known weight on scale");
  Serial.println("Press + or a to increase calibration factor");
  Serial.println("Press - or z to decrease calibration factor");
  Serial.println("Press w to write value to EEPROM");
  Serial.println("Press q to quit");
    
  scale.set_scale();
  scale.tare();  //Reset the scale to 0

  //long zero_factor = scale.read_average(); //Get a baseline reading
  //Serial.print("Zero factor: "); //This can be used to remove the need to tare the scale. Useful in permanent scale projects.
  //Serial.println(zero_factor);

  // Loop until time to exit
  do {
    scale.set_scale(calibration_factor); //Adjust to this calibration factor
    Serial.print("Reading: ");
    Serial.print(scale.get_units(), 1);
    Serial.print(" Lbs"); //Change this to kg and re-adjust the calibration factor if you follow SI units like a sane person
    Serial.print(" calibration_factor: ");
    Serial.print(calibration_factor);
    Serial.println();

    if(Serial.available())
    {
      temp = Serial.read();
      if(temp == '+' || temp == 'a')
        calibration_factor += 10;
      else if(temp == '-' || temp == 'z')
        calibration_factor -= 10;
      else if(temp == 'r')
        calibration_factor = -2230;
      else if(temp == 'w') {
        EEPROM.put(eeAddress, calibration_factor);
        Serial.println("calibration written!");
        }
     }

  } while (temp != 'q');
  scale.tare();  //Reset the scale to 0
}