#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <TimeLib.h>

const char* ssid = "Tenda_91B830";      // 您的WiFi网络名称
const char* password = "qq1752019978";  // 您的WiFi网络密码

WiFiUDP udp;
const char* ntpServerName = "pool.ntp.org";
const int timeZone = 8;  // 东八区
unsigned int localPort = 8888;  // 本地端口

const int mosPin = 2;       // MOS管连接到ESP8266的D2引脚
const unsigned long activationTime = 7 * 3600; // 早上7点的时间（单位：秒）
const unsigned long activationDuration = 60; // 酒精闹钟打开的持续时间（单位：秒）

#define NTP_PACKET_SIZE 48
byte packetBuffer[NTP_PACKET_SIZE];

void setup() {
  pinMode(mosPin, OUTPUT);

  // 连接到WiFi网络
  Serial.begin(115200);
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // 启动UDP
  udp.begin(localPort);
  Serial.println("UDP started");
}

void loop() {
  // 获取当前时间
  time_t epochTime = getNtpTime();
  if (epochTime != 0) {
    struct tm *currentTime = localtime(&epochTime);
    int currentHour = currentTime->tm_hour;
    
    // 如果当前时间是早上7点，则打开酒精闹钟
    if (currentHour == 7) {
      digitalWrite(mosPin, HIGH); // 打开MOS管
      delay(activationDuration * 1000); // 等待一分钟
      digitalWrite(mosPin, LOW); // 关闭MOS管
    }
  }
  delay(1000); // 等待1秒钟
}

time_t getNtpTime() {
  IPAddress ntpServerIP;
  while (udp.parsePacket() > 0); // 清空缓冲区
  Serial.println("Transmit NTP Request");
  if (WiFi.hostByName(ntpServerName, ntpServerIP)) {
    Serial.println("NTP Server IP Address:");
    Serial.println(ntpServerIP);
    sendNTPPacket(ntpServerIP);
    uint32_t beginWait = millis();
    while (millis() - beginWait < 1500) {
      int size = udp.parsePacket();
      if (size >= NTP_PACKET_SIZE) {
        Serial.println("Receive NTP Response");
        udp.read(packetBuffer, NTP_PACKET_SIZE);
        unsigned long secsSince1900;
        secsSince1900 = (unsigned long)packetBuffer[40] << 24;
        secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
        secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
        secsSince1900 |= (unsigned long)packetBuffer[43];
        return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
      }
    }
    Serial.println("No NTP Response :-(");
    return 0;
  }
  else {
    Serial.println("DNS Lookup failed. Rebooting...");
    ESP.restart();
    return 0;
  }
}

void sendNTPPacket(IPAddress &address) {
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11100011;
  packetBuffer[1] = 0;
  packetBuffer[2] = 6;
  packetBuffer[3] = 0xEC;
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  udp.beginPacket(address, 123);
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}
