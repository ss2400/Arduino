/**************************************************************************
 This is an example for our Monochrome OLEDs based on SSD1306 drivers

 Pick one up today in the adafruit shop!
 ------> http://www.adafruit.com/category/63_98

 This example is for a 128x64 pixel display using I2C to communicate
 3 pins are required to interface (two I2C and one reset).

 Adafruit invests time and resources providing this open
 source code, please support Adafruit and open-source
 hardware by purchasing products from Adafruit!

 Written by Limor Fried/Ladyada for Adafruit Industries,
 with contributions from the open source community.
 BSD license, check license.txt for more information
 All text above, and the splash screen below must be
 included in any redistribution.
 **************************************************************************/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <HX711.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define LOGO_HEIGHT   16
#define LOGO_WIDTH    16

static const unsigned char PROGMEM logo_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000 };

const int LOADCELL_DOUT_PIN = A2;
const int LOADCELL_SCK_PIN = A1;
const long LOADCELL_DIVIDER = -2230;

HX711 scale;

void setup() {
  Serial.begin(9600);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3c)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  //display.display();
  
  Serial.println("Type 'c' to enter calibration mode");
  Serial.println("Initializing the scale");
  
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(LOADCELL_DIVIDER);
  scale.tare();	 // reset the scale to zero

  delay(2000); // Pause for 2 seconds
  
  // Clear the buffer
  display.clearDisplay();
}

void loop() {
  
   // Read serial if ready
  if(Serial.available()) {
    if(Serial.read() == 'c')
      calmode();
  }
  
  // Read scale if ready
  if (scale.is_ready()) {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    display.println(F("Weight"));
    display.setTextSize(3);
    display.print(scale.get_units(),1);
    display.setTextSize(2);
    display.println(F(" Kg"));
    display.display();
    //Serial.print("HX711 reading: ");
    //Serial.println(scale.get_units(),1);
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
    Serial.print(" kg"); //Change this to kg and re-adjust the calibration factor if you follow SI units like a sane person
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
   }
  } while (temp != 'e');
  scale.tare();  //Reset the scale to 0
}
