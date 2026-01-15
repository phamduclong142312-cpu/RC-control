#include "WiFi.h"
#include <esp_now.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128  // định nghĩa chiều x màn oled
#define SCREEN_HEIGHT 64  // định nghĩa chiều y màn oled
#define OLED_ADDR 0x3C

Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

uint8_t broadcastAddress[] = {0x28, 0x37, 0x2F, 0x86, 0x8A, 0x94};

#define potT 15 // throttle
#define potS 16 // steering
#define potM 17 // max speed
#define potTr 18 // trim steering
#define button 41

typedef struct struct_message{
  int throttle;
  int steering;
  int max_speed;
  int trim;
  bool state;
} struct_message;

struct_message myData; 

typedef struct struct_message2{
  int DfC;
  int DfR;
  int DfL;
  int SL;
  int SR;
} struct_message2;

struct_message2 DataReceived; 

esp_now_peer_info_t peerInfo;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void setup(){
  pinMode(button, INPUT_PULLUP);
  Serial.begin(115200);
  WiFi.mode(WIFI_MODE_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_send_cb(OnDataSent);

  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

  Wire.begin(5, 4);
  if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("Khong tim thay OLED");
    while (true);
  }
  oled.setTextColor(SSD1306_WHITE); //pixel hiện sẽ có màu trắng
  oled.clearDisplay(); // xóa hết màn hình
}

int AVGanalogRead(int pot){
  long sum = 0;
  const int samp = 64;
  for(int i=0; i < samp; i++){
    sum += analogRead(pot);
    delayMicroseconds(200);
  }
  return sum/samp;
}

void THROTTLE(){
  int value = AVGanalogRead(potT);
  myData.throttle = constrain(map(value, 0, 4095, -255, 255),-255,255);
}

void STEERING(){
  int value1 = AVGanalogRead(potS);
  myData.steering = constrain(map(value1, 0, 4095, 0 ,180),0,180);
}

void MAX_SPEED(){
  int value2 = AVGanalogRead(potM);
  myData.max_speed = constrain(map(value2, 0, 4095, 0, 255),0,255);
}

void TRIM_SERVO(){
  int value3 = AVGanalogRead(potTr);
  myData.trim = constrain(map(value3, 0, 4095, 0, 180),0,180); 
}

int x=0;
void BAOGAY(){
  oled.setTextSize(1);
  oled.setCursor(x,47);
  oled.print("Bao gay vcl");
  x+=2;
  if(x>SCREEN_WIDTH) x=0;
}

void loop(){
  oled.clearDisplay();
  oled.setTextSize(1); 
  myData.state = digitalRead(button);

  if(myData.state == HIGH){
    THROTTLE();
    oled.setCursor(3,3);
    oled.print("THROTTLE: ");
    oled.print(myData.throttle);

    STEERING();
    oled.setCursor(3,14);
    oled.print("STEERING ANGLE: ");
    oled.print(myData.steering);

    MAX_SPEED();
    oled.setCursor(3,25);
    oled.print("MAX SPEED: ");
    oled.print(myData.max_speed);

  TRIM_SERVO();
  oled.setCursor(3,36);
  oled.print("TRIM SERVO: ");
  oled.print(myData.trim);
  }
  else{
    oled.setCursor(1,1);
    oled.print("Mode: Automotive");

    oled.setCursor(1,11);
    oled.print("TT: ");
    oled.print(DataReceived.DfC);

    oled.setCursor(1,21);
    oled.print("TR: ");
    oled.print(DataReceived.DfR);

    oled.setCursor(1,31);
    oled.print("TL: ");
    oled.print(DataReceived.DfL);

    oled.setCursor(1,41);
    oled.print("SL: ");
    oled.print(DataReceived.SL);

    oled.setCursor(1,51);
    oled.print("SR: ");
    oled.print(DataReceived.SR);

  }
  //BAOGAY();

  oled.display();

  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
}
