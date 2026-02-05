/*********************************************************************

 Mill tachometer

 SSD1306Ascii can be installed using the library manager.  Newer untagged
 versions may be available on GitHub.
 https://github.com/greiman/SSD1306Ascii

*********************************************************************/

#include <Wire.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"
#include <avr/io.h>
#include <avr/interrupt.h>

#define I2C_ADDRESS 0x3C      // 0X3C+SA0 - 0x3C or 0x3D
#define RST_PIN -1            // Define proper RST_PIN if required.
#define MillCS PD2            // Chip select, active low
#define MillCLK PD3           // Clock
#define MillDATA PD4          // Data
#define MillCLK_INTERRUPT 1   // Interrupt
#define PACKET_BITS_HEADER 36 // For newer mills there is a 36 bit header.
#define PACKET_BITS 68
#define PACKET_BITS_COUNT PACKET_BITS+PACKET_BITS_HEADER

SSD1306AsciiWire oled;
volatile uint8_t packet_bits[PACKET_BITS_COUNT];
volatile uint8_t packet_bits_pos;

void setup()   {                
  Serial.begin(9600);
  
  Wire.begin();
  Wire.setClock(400000L);
  
  pinMode(MillCS, INPUT);
  pinMode(MillCLK, INPUT);
  pinMode(MillDATA, INPUT);
  
  // Display setup
  #if RST_PIN >= 0
    oled.begin(&Adafruit128x64, I2C_ADDRESS, RST_PIN);
  #else // RST_PIN >= 0
    oled.begin(&Adafruit128x64, I2C_ADDRESS);
  #endif // RST_PIN >= 0

  // Clear the buffer
  oled.clear();
  oled.home();
  oled.setFont(Adafruit5x7);
  oled.set2X();
  oled.println("Waiting");
  Serial.println("Waiting");
          
  //Trigger on falling edge of INT1
  EICRA |= (1<<ISC11);
  EICRA &= ~(1<<ISC10);
  
  // Disable timer0
  TIMSK0 &= ~(1<<TOIE0);
}

void loop() {
  int rpm;
  
  // Enable Interrupt
  EIMSK |= (1 << INT1);

  if (packet_bits_pos >= PACKET_BITS_COUNT) {
    // Disable Interrupt
    EIMSK &= ~(1 << INT1);
    
    rpm = get_rpm();
    oled.home();
    oled.setFont(Adafruit5x7);
    oled.set2X();
     
    if (rpm == -1) {
      delay(1);
    }
    else if (rpm <= 10) {
      Serial.println("Stopped");
      oled.println("Stopped");
    } 
    else {
      Serial.print(rpm, DEC);
      Serial.println(" rpm");
      oled.print(" RPM");
      oled.clearToEOL();
      oled.println(F(""));
    }
    packet_bits_pos = 0;
    oled.setFont(lcdnums14x24);
    oled.set2X(); 
    oled.setRow(2); // 8 pixel row
    oled.print(rpm);
    oled.clearToEOL();
    oled.println(F(""));
  }
  else if (packet_bits_pos > PACKET_BITS_COUNT) {
    Serial.println("Exceeded length");
    packet_bits_pos = 0;
  }
}

//----------------------------------------------------------------------------------------------------
// Assumes 68 bits were received properly. Returns the spindle speed or -1 if the values are absurd.
int get_rpm()
{
  int temp, ret = 0;
  if (build_address(0) != 0xA0) {
    return -1;
  }
  temp = get_digit_from_data(build_data(8));
  if (temp == -1) {
    return -1;
  }
  ret += temp*1000;

  if (build_address(17) != 0xA1) {
    return -1;
  } 
  temp = get_digit_from_data(build_data(25));
  if (temp == -1) {
    return -1;
  }
  ret += temp*100;  
  
  if (build_address(34) != 0xA2) {
    return -1;
  } 
  temp = get_digit_from_data(build_data(42));
  if (temp == -1) {
    return -1;
  }
  ret += temp*10;

  if (build_address(51) != 0xA3) {
    return -1;
  }
  temp = build_data(59);
  if (temp != 0x20) {
    return -1; 
  }
  
  return ret;
}

// Address is 8 bits long
uint8_t build_address(uint8_t start_address)
{
  uint8_t ret = 0x1;
  uint8_t i;

  if (PACKET_BITS_COUNT != PACKET_BITS) {
    // Compensate for header
    start_address += PACKET_BITS_HEADER;
  }

  for (i = start_address; i < start_address + 8; i++) {
    ret = (ret << 1) ^ ((packet_bits[i] & B00010000) ? 1 : 0);
  }
  return ret;
}

// Data is 9 bits long
uint16_t build_data(uint8_t start_address)
{
  uint16_t ret = 0;
  uint8_t i;

  if (PACKET_BITS_COUNT != PACKET_BITS) {
    // Compensate for header
    start_address += PACKET_BITS_HEADER;
  }

  for (i = start_address; i < start_address + 9; i++) {
    ret = (ret << 1) ^ ((packet_bits[i] & B00010000) ? 1 : 0);
  }
  return ret;
}

// Byte conversion
int get_digit_from_data(uint16_t data)
{
  uint16_t segments = (data & 0xFE) >> 1;
  int ret = 0;
  switch(segments) {
  case 0x7D: ret = 0; break;
  case 0x05: ret = 1; break;
  case 0x6B: ret = 2; break;
  case 0x4F: ret = 3; break;
  case 0x17: ret = 4; break;
  case 0x5E: ret = 5; break;
  case 0x7E: ret = 6; break;
  case 0x0D: ret = 7; break;
  case 0x7F: ret = 8; break;
  case 0x5F: ret = 9; break;
  default:  ret = -1; break;
  }
  return ret;
}

// Returns 1 if stopped, 0 otherwise.
uint8_t spindle_stopped(uint16_t data)
{
  return data & 0x1;
}

// Interupt vector
SIGNAL(INT1_vect)
{
  packet_bits[packet_bits_pos] = PIND;
  packet_bits_pos++;
}
