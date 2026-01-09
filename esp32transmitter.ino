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

typedef struct struct_message{
  int throttle;
  int steering;
  int max_speed;
  int trim;
} struct_message;

struct_message myData; 

esp_now_peer_info_t peerInfo;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void setup(){
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

int AVGanalogRead(int pot){ //HÀM LỌC TRUNG BÌNH NHƯNG KHÔNG HOẠT ĐỘNG LẮM, có gì anh góp ý 
  long sum = 0;
  const int samp = 10;
  for(int i=0; i < samp; i++){
    sum += analogRead(pot);
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

  //BAOGAY();

  oled.display();

  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
}
