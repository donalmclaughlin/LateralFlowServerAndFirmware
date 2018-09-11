// This program can only work on OV5640_MINI_5MP_PLUS platform.
// The program will do the following tasks:
//      1. Set the camera to JPEG output mode.
//      2. Read data from Serial port and deal with it
//      3. If receive 0x00-0x08,the resolution will be changed.
//      4. If receive 0x10,camera will capture a JPEG photo and buffer the image to FIFO. Then write datas to Serial port.
//      5. If receive 0x30,camera will capture a BMP  photo and buffer the image to FIFO.Then write datas to Serial port.
//      7. If receive 0x11 ,set camera to JPEG output mode.
//      8. If receive 0x31 ,set camera to BMP  output mode.
// This program requires the ArduCAM V4.0.0 (or later) library and ArduCAM_Mini_5MP_Plus
// and use Arduino IDE 1.6.8 compiler or above.

#include <Wire.h>
#include <ArduCAM.h>
#include <SPI.h>
#include "memorysaver.h"

#if !(defined (OV5640_MINI_5MP_PLUS))
#error Please select the hardware platform and camera module in the ../libraries/ArduCAM/memorysaver.h file
#endif

#define BMPIMAGEOFFSET 66
const char bmp_header[BMPIMAGEOFFSET] PROGMEM =
{
  0x42, 0x4D, 0x36, 0x58, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x42, 0x00, 0x00, 0x00, 0x28, 0x00,
  0x00, 0x00, 0x40, 0x01, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x01, 0x00, 0x10, 0x00, 0x03, 0x00,
  0x00, 0x00, 0x00, 0x58, 0x02, 0x00, 0xC4, 0x0E, 0x00, 0x00, 0xC4, 0x0E, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x00, 0x00, 0xE0, 0x07, 0x00, 0x00, 0x1F, 0x00,
  0x00, 0x00
};

// set pin 7 as the slave select for the digital pot:
const int CS = 7;
bool is_header = false;
int mode = 0;
uint8_t start_capture = 0;

#if defined (OV5640_MINI_5MP_PLUS)
  ArduCAM myCAM(OV5640, CS);
#endif

uint8_t read_fifo_burst(ArduCAM myCAM);

void setup() {
  // put your setup code here, to run once:
  uint8_t vid, pid;
  uint8_t temp;
  #if defined(__SAM3X8E__)
    Wire1.begin();
    Serial.begin(115200);
  #else
    Wire.begin();
    Serial.begin(921600);
  #endif
  Serial.println(F("ACK CMD ArduCAM Start!"));

  // set the CS as an output:
    pinMode(CS, OUTPUT);
  
  // initialize SPI:
    SPI.begin();

  while(1){
    //Check if the ArduCAM SPI bus is OK
      myCAM.write_reg(ARDUCHIP_TEST1, 0x55);
      temp = myCAM.read_reg(ARDUCHIP_TEST1);
      if(temp != 0x55){
        Serial.println(F("ACK CMD SPI interface Error!"));
        delay(1000);continue;
        }else{
        Serial.println(F("ACK CMD SPI interface OK."));break;
        }
  }

  #if defined (OV5640_MINI_5MP_PLUS)
    while(1){
      //Check if the camera module type is OV5640
        myCAM.rdSensorReg16_8(OV5640_CHIPID_HIGH, &vid);
        myCAM.rdSensorReg16_8(OV5640_CHIPID_LOW, &pid);
        if ((vid != 0x56) || (pid != 0x40)){
          Serial.println(F("ACK CMD Can't find OV5640 module!"));
          delay(1000); continue;
        }else{
          Serial.println(F("ACK CMD OV5640 detected."));break;      
        }
    }
  #endif
  
  //Change to JPEG capture mode
    myCAM.set_format(JPEG);
    myCAM.InitCAM();
    myCAM.set_bit(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);
    myCAM.clear_fifo_flag();
    myCAM.write_reg(ARDUCHIP_FRAMES, 0x00);
}

void loop() {
  uint8_t temp= 0xff, temp_last =0;
  bool is_header = false;

  if (Serial.available()){
    temp = Serial.read();
    switch (temp)
        
      {        
          case 0:  myCAM.OV5640_set_JPEG_size(OV5640_320x240);delay(1000);
                   Serial.println(F("ACK CMD switch to OV5640_320x240"));
                   temp=0xff;
                   break;
      
          case 1:  myCAM.OV5640_set_JPEG_size(OV5640_352x288);delay(1000);
                   Serial.println(F("ACK CMD switch to OV5640_352x288"));
                   temp=0xff;
                   break;
          
          case 2:  myCAM.OV5640_set_JPEG_size(OV5640_640x480);delay(1000);
                   Serial.println(F("ACK CMD switch to OV5640_640x480"));
                   temp=0xff;
                   break;
    
          case 3:  myCAM.OV5640_set_JPEG_size(OV5640_800x480);delay(1000);
                   Serial.println(F("ACK CMD switch to OV5640_800x480"));
                   temp=0xff;
                   break;

          case 4:  myCAM.OV5640_set_JPEG_size(OV5640_1024x768);delay(1000);
                   Serial.println(F("ACK CMD switch to OV5640_1024x768"));
                   temp=0xff;
                   break;
                   
          case 5:  myCAM.OV5640_set_JPEG_size(OV5640_1280x960);delay(1000);
                   Serial.println(F("ACK CMD switch to OV5640_1280x960"));
                   temp=0xff;
                   break;
          
          case 6:  myCAM.OV5640_set_JPEG_size(OV5640_1600x1200);delay(1000);
                   Serial.println(F("ACK CMD switch to OV5640_1600x1200"));
                   temp=0xff;
                   break;
          
          case 7:  myCAM.OV5640_set_JPEG_size(OV5640_2048x1536);delay(1000);
                   Serial.println(F("ACK CMD switch to OV5640_2048x1536"));
                   temp=0xff;
                   break;
                     
          case 8:  myCAM.OV5640_set_JPEG_size(OV5640_2592x1944);delay(1000);
                   Serial.println(F("ACK CMD switch to OV5640_2592x1944"));
                   temp=0xff;
                   break;
    
          case 0x10:
                  mode = 1;
                  start_capture = 1;
                  Serial.println(F("ACK CMD CAM start single shoot."));
                  break;

          case 0x11:
                   myCAM.set_format(JPEG);
                   myCAM.InitCAM();
                   myCAM.set_bit(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);
                   break; 

         
      }
  }


  if (mode == 1){
    
      if (start_capture == 1){
          myCAM.flush_fifo();
          myCAM.clear_fifo_flag();
          myCAM.start_capture();
          start_capture = 0;
      }
  
      if (myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK)){
          Serial.println(F("CAM Capture Done."));
          read_fifo_burst(myCAM);
          myCAM.clear_fifo_flag();
      }
  }

}

uint8_t read_fifo_burst(ArduCAM myCAM)
{
uint8_t temp = 0, temp_last = 0;
uint32_t length = 0; 
length = myCAM.read_fifo_length();
Serial.println(length, DEC);
if (length >= MAX_FIFO_SIZE) //512 kb
{
  Serial.println(F("ACK CMD Over size."));
  return 0;
}
if (length == 0 ) //0 kb
{
  Serial.println(F("ACK CMD Size is 0."));
  return 0;
}
myCAM.CS_LOW();
myCAM.set_fifo_burst();//Set fifo burst mode
while ( length-- )
{
  temp_last = temp;
  temp =  SPI.transfer(0x00);
  if (is_header == true)
  {
    Serial.write(temp);
  }
  else if ((temp == 0xD8) & (temp_last == 0xFF))
  {
    is_header = true;
    Serial.println(F("ACK IMG"));
    Serial.write(temp_last);
    Serial.write(temp);
  }
  if ( (temp == 0xD9) && (temp_last == 0xFF) ) //If find the end ,break while,
  break;
  delayMicroseconds(15);
}
myCAM.CS_HIGH();
is_header = false;
return 1;
}
