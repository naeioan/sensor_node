#include "VirtualWire.h"
#include "DHT.h"
#include <Arduino.h>
#include "constants.h"
#include "cipher.h"

#define BUFSIZE    8 
#define TRANSPIN 3  //what pin to transmit on ( tx radio data pin)
#define DHTPIN 4     // what pin the DHT sensor is connected to


// Uncomment whatever type you're using!
//#define DHTTYPE DHT11   // DHT 11 
#define DHTTYPE DHT22   // DHT 22  (AM2302)
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

// Connect pin 1 (on the left) of the sensor to +5V
// Connect pin 2 of the sensor to whatever your DHTPIN is
// Connect pin 4 (on the right) of the sensor to GROUND
// Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  while (!Serial);  // required for Flora & Micro
  delay(500);
  Serial.begin(115200);
  //randomSeed(analogRead(A0));  //initialize the random number generator with
                               //a random read from an unused and floating analog port
  //initialize the virtual wire library
  vw_set_ptt_inverted(true); // Required for DR3100
  vw_set_tx_pin(TRANSPIN);
  vw_setup(2000);  //keep the data rate low for better reliability and range
  
  //Initialize the Sensor
  dht.begin();
  
}


int txMessage(char *msg){
    digitalWrite(13, true); // Flash an led to show transmitting
    vw_send((uint8_t *)msg, strlen(msg));
    vw_wait_tx(); // Wait until the whole message is gone
    digitalWrite(13, false);
}


void loop() {

  char msg_enc_snd[BUFSIZE+1]; // encrypted data to char
  uint8_t inputKey[] = {0x00, 0x01, 0x02, 0x03, 0x08, 0x09, 0x0a, 0x0b, 0x10, 0x11, 0x12, 0x13, 0x18, 0x19, 0x1a, 0x1b};
  uint8_t keys[SIMON_BLOCK_SIZE/16*SIMON_ROUNDS];
  uint8_t plainText[BUFSIZE+1]; //plaintext data

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  
// check if returns are valid, if they are NaN (not a number) then something went wrong!
  if (isnan(t) || isnan(h)) {
    Serial.println("Failed to read from DHT");
	} 
  else {
    Serial.print("Humidity: "); 
    Serial.print(h);
    Serial.print(" %\t");
    Serial.print("Temperature: "); 
    Serial.print(t);
    Serial.println(" *C");
    }
  

 //build the message
	plainText[0] = (uint8_t) h;
	plainText[1] = (uint8_t) t;
	//0 padding
	for(int i=2; i<8; i++){
		plainText[i] = (uint8_t) 0;
	}
	
	//rotate keys
	encryptKeySchedule(inputKey, keys);
	printArr(plainText,"PlainText: ");
	
	//encrypt
	encrypt(plainText, keys);
	printArr(plainText,"After encryption: ");

    Serial.print("Sending Message: ");
	// add encrypted message as a char string to buffer for sending
	/*for(int i=0;i<8;i++){
			strcpy(msg_enc_snd,(char)plainText[i]);
     if(i>0){
      strncat(msg_enc_snd,&(char)plainText[i],1);
      }
			//sprintf(msg_enc_snd + strlen(msg_enc_snd),"%c",(char)plainText[i]); // a better idea would be to not transmit the encrypted padding
			
		}
   */

   memcpy(msg_enc_snd,plainText,8);
   msg_enc_snd[8]='\0';
	//sprintf(msg_enc_snd + strlen(msg_enc_snd),"\0"); //do we need null termination ?
  //strcat(msg_enc_snd,"\0");
	//#TODO check if the msg_enc_snd string is created correctlly !!!
    Serial.println(msg_enc_snd);
	
    txMessage(msg_enc_snd);  //message will not be sent if there is an error
    
	delay(1000); //1s

 }



