/*
    A simple library demo thatat reads an MLP191020 - 1-channel CT sesnor boards
    Based On:   EmonLib https://github.com/openenergymonitor/EmonLib
    Auther:     David Mottram
    Updated:    22nd September 2021
*/

#include <ESP8266WiFi.h>              // needed for EPS8266
#include <WiFiClient.h>               // WiFi client

#include <SPI.h>
#include <Wire.h>

void printWifiStatus();
void httpRequest();

/*for 1.3 display*/
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
/* Uncomment the initialize the I2C address , uncomment only one, If you get a totally blank screen try the other*/
#define i2c_Address 0x3c //initialize with the I2C addr 0x3C Typically eBay OLED's
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1   //   QT-PY / XIAO
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//This ProgramID is the name of the sketch and identifies what code is running on the D1 Mini
const char* ProgramID = "LMWA_d1_003";


//Wifi Stuff
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
const char *ssid =	"LMWA-PumpHouse";		// cannot be longer than 32 characters!
const char *pass =	"ds42396xcr5";		//
//const char *ssid =	"WiFiFoFum";		// cannot be longer than 32 characters!
//const char *pass =	"6316EarlyGlow";		//
WiFiClient client;
String wifistatustoprint;

//Tago.io server address and device token
char server[] = "api.tago.io";
int lastConnectionTime = 0;            // last time you connected to the server, in milliseconds
int currentMillis = 0;
int postingInterval = 10 * 1000; // delay between updates, in milliseconds
String Device_Token = "de213466-99fb-413a-9aad-5411de17c963"; //d1_002_pressure_sensor Default token
String pressure_string = "";

// I/O items
#define Network_LED 2

// library for the MLP191020 PCB
#define Cal_value 1500

// values for reporting
#define Voltage 120
#define CT_Cal 17.619
#define Min_Usable_Value 0.3
#define Reporting_Delay 500

// https://github.com/Mottramlabs/MQTT-Power-Sensor
#include <MLP191020.h>
// make an instance of MLP191020
MLP191020 My_PCB(Cal_value);


// Binary Sensor
#define BINARYPIN 13

double Value = 0;                     // result
float Amps = 0;
int status = 0;
String MotorStatus = "Off";

void setup() {

  Serial.begin(115200);
  while (!Serial) {
    ;                     // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("SETUP");
  Serial.println();

  pinMode(BINARYPIN, INPUT_PULLUP);

  //1.3" OLED Setup
  delay(250); // wait for the OLED to power up
  display.begin(i2c_Address, true); // Address 0x3C default
 //display.setContrast (0); // dim display
 
  display.display();
  delay(2000);

  // Clear the buffer.
  display.clearDisplay();

  // draw a single pixel
  display.drawPixel(10, 10, SH110X_WHITE);
  // Show the display buffer on the hardware.
  // NOTE: You _must_ call display after making any drawing commands
  // to make them visible on the display hardware!
  display.display();
  delay(2000);
  display.clearDisplay();

} // end of setup


void loop() {

  currentMillis = millis();

  //Wifi Stuff
  if (WiFi.status() != WL_CONNECTED) {
    
    //Write wifi connection to display
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 0);
    display.println("Booting Program ID:");
    display.println(ProgramID);
    display.setTextSize(1);
    display.println(" ");
    display.println("Connecting To WiFi:");
    display.println(ssid);
    display.println(" ");
    display.println("Wait for it......");
    display.display();

    //write wifi connection to serial
    Serial.print("Connecting to ");
    Serial.print(ssid);
    Serial.println("...");
    WiFi.setHostname(ProgramID);
    WiFi.begin(ssid, pass);

    //delay 8 seconds for effect
    delay(8000);

    if (WiFi.waitForConnectResult() != WL_CONNECTED){
      return;
    }

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 0);
    display.print("Pump Sensor\nPrgm ID: "); display.println(ProgramID);
    display.setTextSize(1);
    display.println(" ");
    display.println("Connected To WiFi:");
    display.println(ssid);
    display.println(" ");
    display.display();

    Serial.println("\n\nWiFi Connected! ");
    printWifiStatus();

  }

  if (WiFi.status() == WL_CONNECTED) {
    wifistatustoprint="Wifi Connected!";
  }else{
    wifistatustoprint="Womp, No Wifi!";
  }



  // read A/D values and store in value
  Value = My_PCB.power_sample();

  // calc Amps, zero the value if below usable value
  Amps = Value / CT_Cal;
//  float Amps = Value;

  // if below min usable value then zero
  if (Value < Min_Usable_Value) {
    Amps = 0;
  } // end if

  //int Watts = Amps * Voltage;

  status = digitalRead(BINARYPIN);
  Serial.print("BINARYPIN Status: ");
  Serial.println(status);
  if (status == HIGH)
  {
    Serial.println("BINARY SENSOR OPEN");
    MotorStatus = "Off";
  }
  else
  {
    Serial.println("BINARY SENSOR CLOSED");
    MotorStatus = "On";
  }

  // display report
  Serial.print("Value: "); Serial.println(Value); 
  Serial.print("Amps: "); Serial.println(Amps); 
  Serial.print("Binary: "); Serial.println(status);
  Serial.println("");

  // flash the LED
  digitalWrite(Network_LED, HIGH); delay(10); digitalWrite(Network_LED, LOW);
  display.clearDisplay();

  // text display tests
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.println("Power Sensor");
  display.print("Prgm ID: "); display.println(ProgramID);
  display.print("Expected Voltage:"); display.println(Voltage);
  //display.print("Raw:"); display.println(Value);
  display.print("Amps:"); display.println(Amps);
  display.print("Open(1) Closed(0): "); display.println(status);
  display.print("MotorStatus: "); display.println(MotorStatus);
//  display.println(wifistatustoprint);
  display.print("SSID:");
  display.println(ssid);
  display.print("IP:");
  display.println(WiFi.localIP());
  // write the buffer to the display
  display.display();
  display.clearDisplay();
  delay(Reporting_Delay);




    // if upload interval has passed since your last connection,
  // then connect again and send data to tago.io
  Serial.print("currentMillis: "); Serial.println(currentMillis, 0);
  Serial.print("lastConnectionTime: "); Serial.println(lastConnectionTime, 0);
  Serial.print("PostingInterval: "); Serial.println(postingInterval, 0);
  if (currentMillis - lastConnectionTime > postingInterval) {
    Serial.println("Time to post to tago.io!");
    // then, send data to Tago
    httpRequest();
  }
  Serial.println();
  Serial.println();

} // end of loop


// this method makes a HTTP connection to tago.io
void httpRequest() {

  Serial.print("Sending this Amperage: ");
  Serial.println(Amps);

    // close any connection before send a new request.
    // This will free the socket on the WiFi shield
    client.stop();

    Serial.println("Starting connection to server for Pressure...");
    // if you get a connection, report back via serial:
    String PostAmps = String("{\"variable\":\"amps\", \"value\":") + String(Amps)+ String(",\"unit\":\"Amps\"}");
    String Dev_token = String("Device-Token: ")+ String(Device_Token);
    if (client.connect(server,80)) {                      // we will use non-secured connnection (HTTP) for tests
    Serial.println("Connected to server");
    // Make a HTTP request:
    client.println("POST /data? HTTP/1.1");
    client.println("Host: api.tago.io");
    client.println("_ssl: false");                        // for non-secured connection, use this option "_ssl: false"
    client.println(Dev_token);
    client.println("Content-Type: application/json");
    client.print("Content-Length: ");
    client.println(PostAmps.length());
    client.println();
    client.println(PostAmps);
    Serial.println("Amps sent!\n");
    }  else {
      // if you couldn't make a connection:
      Serial.println("Server connection failed.");
    }

    client.stop();


  Serial.print("Sending this status: ");
  Serial.println(status);

    // close any connection before send a new request.
    // This will free the socket on the WiFi shield
    client.stop();

    Serial.println("Starting connection to server for data upload...");
    // if you get a connection, report back via serial:
    String PostStatus = String("{\"variable\":\"status\", \"value\":") + String(status)+ String(",\"unit\":\"status\"}");
    Dev_token = String("Device-Token: ")+ String(Device_Token);
    if (client.connect(server,80)) {                      // we will use non-secured connnection (HTTP) for tests
    Serial.println("Connected to server");
    // Make a HTTP request:
    client.println("POST /data? HTTP/1.1");
    client.println("Host: api.tago.io");
    client.println("_ssl: false");                        // for non-secured connection, use this option "_ssl: false"
    client.println(Dev_token);
    client.println("Content-Type: application/json");
    client.print("Content-Length: ");
    client.println(PostStatus.length());
    client.println();
    client.println(PostStatus);
    Serial.println("Status sent!\n");
    }  else {
      // if you couldn't make a connection:
      Serial.println("Server connection failed.");
    }

    client.stop();

    // note the time that the connection was made:
    lastConnectionTime = currentMillis;
}

//this method prints wifi network details
void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  Serial.print("Hostname: ");
  Serial.println(WiFi.getHostname());

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
  Serial.println("");
}