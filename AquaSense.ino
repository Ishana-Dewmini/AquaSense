#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <WiFiManager.h>

#define Screen_width 128
#define Screen_heigh 64
#define OLD_reset -1
#define Screen_Address 0x3C
#define SOUND_SPEED 0.034

Adafruit_SSD1306 display(Screen_width, Screen_heigh, &Wire, OLD_reset);


//Set Water Level Distance in CM
int emptyTankDistance = 120 ;  //Distance when tank is empty
int fullTankDistance =  35 ;  //Distance when tank is full

BLYNK_WRITE(V4) {
  emptyTankDistance = param.asInt();
}

BLYNK_WRITE(V3) {
  fullTankDistance = param.asInt();
}

//Set trigger value in percentage
int Low_triggerPer =   10 ;  //alarm will start when water level drop below triggerPer
int High_triggerPer =  95 ;  //alarm will start when water level exceed upper triggerPer

// Define connections to sensor
#define TRIGPIN    5  //D27
#define ECHOPIN    18  //D26
#define wifiLed    2   //D2
#define ButtonPin1 34  //D12
#define BuzzerPin  4  //D13
#define Led   15  //D14
#define screen 19

/* Fill-in your Template ID (only if using Blynk.Cloud) */


#define BLYNK_TEMPLATE_ID "TMPL6RhvwKUZN"
#define BLYNK_TEMPLATE_NAME "ESP32 Water Level"
#define BLYNK_AUTH_TOKEN "A-p6kvshW03bfzTJ1bV9rSYc7HfzkQD0"



//Change the virtual pins according the rooms
#define VPIN_BUTTON_1    V1 
#define VPIN_BUTTON_2    V2
#define VPIN_BUTTON_5    V5


char auth[] = BLYNK_AUTH_TOKEN;

bool break_happened =false;
long duration;
float distance;
int   waterLevelPer;
bool  toggleBuzzer = HIGH; //Define to remember the toggle state

BlynkTimer timer;

void checkBlynkStatus() { // called every 3 seconds by SimpleTimer

  bool isconnected = Blynk.connected();
  if (isconnected == false) {
    Serial.println("Blynk Not Connected");
    digitalWrite(wifiLed, LOW);
  }
  if (isconnected == true) {
    digitalWrite(wifiLed, HIGH);
    Serial.println("Blynk Connected");
  }
}

BLYNK_CONNECTED() {
  Blynk.syncVirtual(VPIN_BUTTON_1);
  Blynk.syncVirtual(VPIN_BUTTON_2);
  Blynk.syncVirtual(VPIN_BUTTON_5);
}


void print_line(String text, int text_size, int column, int row) {
  if (digitalRead(screen) == HIGH) {
    display.setTextSize(text_size);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(column, row);
    display.println(text);
    display.display();
  }
    
}


void measureDistance(){
  // Set the trigger pin LOW for 2uS
  digitalWrite(TRIGPIN, LOW);
  delayMicroseconds(2);
 
  // Set the trigger pin HIGH for 20us to send pulse
  digitalWrite(TRIGPIN, HIGH);
  delayMicroseconds(20);
 
  // Return the trigger pin to LOW
  digitalWrite(TRIGPIN, LOW);
 
 
  int distance = pulseIn(ECHOPIN,HIGH,26000);
  distance = distance/58;
  Serial.print("Distance: ");
  Serial.println(distance);  
  
  
  waterLevelPer = map((int)distance ,emptyTankDistance, fullTankDistance, 0, 100);
  if (waterLevelPer>100){
    waterLevelPer=100;
  }
  else if (waterLevelPer<0){
    waterLevelPer=0;
  }
  Blynk.virtualWrite(VPIN_BUTTON_1, waterLevelPer);
  Blynk.virtualWrite(VPIN_BUTTON_2, (String(distance) + " cm"));
  Blynk.virtualWrite(VPIN_BUTTON_5, (String(emptyTankDistance-distance) + " cm"));
   
  display.clearDisplay();
  print_line("Water Level is", 1, 25, 0);
  print_line(String(waterLevelPer)+" %", 2, 42, 15);
  print_line("Distance", 1, 42, 32);
  print_line(String(distance)+" cm", 2, 33, 50);

  // Print result to serial monitor
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");
  Serial.println(waterLevelPer);

  if (waterLevelPer < Low_triggerPer){
      if (break_happened==false){
        display.clearDisplay();
        ring_alarm("LOW"); 
      }    
  }
  else if (waterLevelPer > High_triggerPer){
      if (break_happened==false){
        display.clearDisplay();
        ring_alarm("HIGH"); 
      } 
  }
  else if (waterLevelPer<High_triggerPer && waterLevelPer>Low_triggerPer){
      break_happened =false;
  }
    
          
  
  
  // Delay before repeating measurement
  delay(100);
}

 
void ring_alarm(String state) {
  
  digitalWrite(Led, HIGH);
  print_line(String(waterLevelPer)+" %", 2, 42, 15);  
  print_line(state, 2, 42, 40);
  

  
  if (digitalRead(ButtonPin1) == LOW) {
        print_line(String(waterLevelPer)+" %", 2, 42, 15);
        print_line("Distance", 1, 42, 32);
        print_line(String(distance)+" cm", 2, 33, 50);
        delay(200);
        break_happened = true;
       
  }
  tone(BuzzerPin,523);
  delay(500);
  noTone(BuzzerPin);
  delay(2);
    
  


  digitalWrite(Led, LOW);
 // display.clearDisplay();
}

bool timeouted = false;
void connectWiFi(){
  display.clearDisplay();
  print_line("Connect to wifi", 1, 1, 0);
  WiFiManager wm;
  wm.resetSettings();
  wm.setConfigPortalTimeout(120); // set connection timeout to 2 minutes
  bool res = wm.autoConnect("AquaSense","password"); // password protected aphttp://192.168.4.1/

  if (res) {
    Serial.println("Connected to WiFi");
    display.clearDisplay();
    print_line("Connected", 2, 1, 0);
    delay(1000);
    return; // exit the function on successful connection
  } 
  else {
    Serial.println("Failed to connect to WiFi");
    display.clearDisplay();
    timeouted = true;
    print_line("Wifi connection failed", 1, 1, 0);
    delay(2);
    return; // exit the function on failure
  }
}



void setup() {
  Serial.begin(115200);

  if (!display.begin(SSD1306_SWITCHCAPVCC, Screen_Address)) {
    Serial.println(F("SSD1306 allocation failed."));
    for (;;);
  }
  display.display();
  delay(2000);
  
  display.clearDisplay();
  print_line("Welcome", 1,50 , 0);
  print_line("AquaSense", 2,10 , 20);
  delay(1000);
  
  // Set pinmodes for sensor connections
  pinMode(ECHOPIN, INPUT_PULLUP);
  pinMode(TRIGPIN, OUTPUT);
  pinMode(wifiLed, OUTPUT);
  pinMode(Led, OUTPUT);
  pinMode(BuzzerPin, OUTPUT);
  pinMode(ButtonPin1, INPUT);
  pinMode(screen,INPUT);

  digitalWrite(wifiLed, LOW);
  digitalWrite(Led, LOW);
  digitalWrite(BuzzerPin, LOW);

  //WiFi.begin(ssid, pass);
  timer.setInterval(2000L, checkBlynkStatus); // check if Blynk server is connected every 2 seconds
  Blynk.config(auth);
  delay(1000);

  
  
}

void loop() {
  if (WiFi.status() != WL_CONNECTED && timeouted == false) {
    connectWiFi();
  }
  measureDistance();
  Blynk.run();
  timer.run();  
}














  

  
