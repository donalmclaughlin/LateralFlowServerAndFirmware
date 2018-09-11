/*
Photon Particle firmware for the Quick Check Lateral Flow Immunoassay.
This firmware is used in conjuction with:
    AWS EC2 Instance IP:    54.85.56.7
    AWS EC2 Instance Port:  5550
    Arducam Camera Module:  OV5640 5MP MINI PLUS
    Image Resolution:       2592x1944px
Images are captured and are sent in TCP packets to a recipient Node.js script running in AWS.
Due to the Photon's memory capacity, one image must be sent in multipled TCP packets.
Open-source camera libraries and example scripts are published by www.arducam.com.

@modified   28/08/18
@author:     Donal McLaughlin
*/

#include "ArduCAM.h"
#include "memorysaver.h"
SYSTEM_THREAD(ENABLED);

#define VERSION_SLUG "7n"

//Connect to TCP Client Server on PC
TCPClient client;

//Elastic IP for EC2 instance containing Node.js server
#define SERVER_ADDRESS "54.85.56.7"
//Node.js listens at port 5550
#define SERVER_TCP_PORT 5550
//1024
#define TX_BUFFER_MAX 4096

uint8_t buffer[TX_BUFFER_MAX + 1];
int tx_buffer_index = 0;
int led = D7;           //Onboard led
int ledouter = A0;      //LED outer, on if image capturing is in progress
int button = D4;        //Push Button Switch
int illled1 = D3;       //Illumination LED 1
int illled2 = D2;       //Illumination LED 2
int conled = A1;        //Connection LED - On if cant connect to TCP server
int buttonpush = 0;     //Button switch - 0 when off - 1 when pushed

// set pin A2 as the slave select for the ArduCAM shield
const int SPI_CS = A2;
// camera module used: OV5640
ArduCAM myCAM(OV5640, SPI_CS);

//----------------------------------------------SETUP----------------------------------------------//
void setup()
{
    // Set pin modes for all the LEDs and PBS
    pinMode(led, OUTPUT);
    pinMode(ledouter, OUTPUT);
    pinMode(illled1, OUTPUT);
    pinMode(illled2,OUTPUT);
    pinMode(conled, OUTPUT);
    pinMode(button, INPUT_PULLUP);

    Particle.publish("status", "Hello, Version: " + String(VERSION_SLUG));
    delay(1000);

    uint8_t vid,pid;
    uint8_t temp;

    //Set baseline frequency for communitcation between I2C slave devices
    Wire.setSpeed(CLOCK_SPEED_100KHZ);
    Wire.begin();

    Serial.begin(115200);
    Serial.println("ArduCAM camera starting!");

    // set the SPI_CS as an output:
    pinMode(SPI_CS, OUTPUT);

    // Initialise Serial Peripheral Interface (SPI)
    //Outputs:  SCK, MOSI
    //Pull low: SCK, MOSI
    SPI.begin();


    while(1) {
        //Check for CMOS sensor, events publushed to particle cloud and console
        Particle.publish("status", "checking for camera");
        Serial.println("Checking for camera...");   

        //Check if the ArduCAM SPI bus is OK
        myCAM.write_reg(ARDUCHIP_TEST1, 0x55);
        temp = myCAM.read_reg(ARDUCHIP_TEST1);
        if(temp != 0x55){
            Serial.println("SPI interface Error!");
            Serial.println("myCam.read_reg: " + String(temp));
            delay(5000);
            }
        else {
            break;
            }
        Particle.process();
        }

        Particle.publish("status", "Camera found.");


    while(1){
        //Check if the camera module type is OV5640
        myCAM.rdSensorReg16_8(OV5640_CHIPID_HIGH, &vid);
        myCAM.rdSensorReg16_8(OV5640_CHIPID_LOW, &pid);
       
            Serial.println(F("OV5640 detected."));
            Particle.publish("status", "OV5640 detected: " + String::format("%d:%d", vid, pid));
            break;
        }


    Serial.println("Camera found, initializing...");


    //Change MCU mode of the sensor (RGB or JPEG MODE)
    myCAM.set_format(JPEG);
    delay(100);
    //initialise hardware information of the photon (SPI chip select port and
    //image sensor slave address)
    myCAM.InitCAM();
    delay(100);
    // Close auto exposure mode
    uint8_t _x3503;
    myCAM.wrSensorReg16_8(0x5001,_x3503|0x01);	 
    //Set exposure value 
    myCAM.wrSensorReg16_8(0x3500,0x00);
    myCAM.wrSensorReg16_8(0x3501,0x79);
    myCAM.wrSensorReg16_8(0x3502,0xe0);

    myCAM.set_bit(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);
    delay(100);
    //Clear flag after one image is buffed to photon memory
    myCAM.clear_fifo_flag();
    delay(100);
    
    myCAM.write_reg(ARDUCHIP_FRAMES,0x00);
    delay(100);
    //Set JPEG resolution
    myCAM.OV5640_set_JPEG_size(OV5640_320x240);

    // wait 1 second
    delay(1000);

    //Connect to Node.js server in AWS EC2 instance
    client.connect(SERVER_ADDRESS, SERVER_TCP_PORT);
}


//----------------------------------------------LOOP----------------------------------------------//

void loop()
{
	//Cannot connect to client: Blue LED turns on and message sent to Particle cloud
	//Repeated until TCP server is found
    if (!client.connected()) {
        //client.stop();
        Particle.publish("status", "Reconnect to TCP Server...");
        if (!client.connect(SERVER_ADDRESS, SERVER_TCP_PORT)) {
            digitalWrite(conled, HIGH);
            delay(1000);
            return;
        }
    }
    digitalWrite(conled, LOW);
int buttonState = digitalRead(button);
int imgcount = 0;
if (buttonState == LOW){
    buttonpush = 0;
    digitalWrite(ledouter, HIGH);
    analogWrite(illled1, 1);
    analogWrite(illled2, 1);

    Particle.publish("status", "Taking a picture...");
    Serial.println("Taking a picture...");
   digitalWrite(led, HIGH);

//CHANGE IMAGE RESOLUTION-------------------------------------------------
	//myCAM.OV5640_set_JPEG_size(OV5640_320x240);   //IMAGE RESOLUTION: 320x240 works
    //myCAM.OV5640_set_JPEG_size(OV5640_1600x1200); //IMAGE RESOLUTION: 1600x1200 not working
    //myCAM.OV5640_set_JPEG_size(OV5640_1280x960);  // IMAGE RESOLUTION: 1280x960 not working
    //myCAM.OV5640_set_JPEG_size(OV5640_640x480);    // IMAGE RESOLUTION: 640x480 not working

	//Set image resolution at max resolution at: 2592x1944
    myCAM.OV5640_set_JPEG_size(OV5640_2592x1944); //works
//------------------------------------------------------------------------
    delay(100);

    myCAM.flush_fifo();
    delay(100);

    myCAM.clear_fifo_flag();
    delay(100);
    //Capture and Store frame data on 8MB frame buffer
    myCAM.start_capture();
    delay(100);


    unsigned long start_time = millis(),
                  last_publish = millis();


/*
  wait for the photo to be done
*/
    while(!myCAM.get_bit(ARDUCHIP_TRIG , CAP_DONE_MASK)) {
        Particle.process();
        delay(10);

        unsigned long now = millis();
        if ((now - last_publish) > 1000) {
            Particle.publish("status", "waiting for photo " + String(now-start_time));
            last_publish = now;
        }

        if ((now-start_time) > 30000) {
            Particle.publish("status", "bailing...");
            break;
        }
    }
    delay(100);

    int length = myCAM.read_fifo_length();
    Particle.publish("status", "Image size is " + String(length));
    Serial.println("Image size is " + String(length));

    uint8_t temp = 0xff, temp_last = 0;
    int bytesRead = 0;

    if(myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK))
    {
        delay(100);
        Serial.println(F("Capture Done."));
        Particle.publish("status", "Capture done");

        //read multiple bytes out of the frame buffer
        myCAM.set_fifo_burst();

        if(length < 430000){
            Particle.publish("status", "No strip detected");
             digitalWrite(conled, HIGH);
             delay(1000)
             digitalWrite(conled, LOW);
             delay(1000)
             digitalWrite(conled, HIGH);
             delay(1000)
             digitalWrite(conled, LOW);
            break;
        }

        tx_buffer_index = 0;
        temp = 0;


        while( (temp != 0xD9) | (temp_last != 0xFF) )
        {
            temp_last = temp;// set last temp
            temp = myCAM.read_fifo();// read data
            bytesRead++;


            buffer[tx_buffer_index++] = temp;//
            //index >= 4096
            if (tx_buffer_index >= TX_BUFFER_MAX) {
              //client.write(buffer array, length of buffer);
                client.write(buffer, tx_buffer_index);
                Particle.publish("Writing to server");
                
                tx_buffer_index = 0;
                
                Particle.process();
                
            }
            // If the image is larger than 2MB, stop sending to the server
            if (bytesRead > 2048000) {
                break;
            }
        }


        if (tx_buffer_index != 0) {
            client.write(buffer, tx_buffer_index);
            Particle.publish("Writing to server (small)");
        }

        //Clear the capture done flag
        myCAM.clear_fifo_flag();

        Serial.println(F("End of Photo"));
    }

	
	//Photo capture finish: turn off Green LED and illumination LEDs
    digitalWrite(ledouter, LOW);
    digitalWrite(led, LOW);
    analogWrite(illled1, 0);
    analogWrite(illled2, 0);
    buttonState = HIGH;

    }else{
        digitalWrite(ledouter, LOW);
    }
}
