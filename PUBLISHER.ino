#include <Servo.h> 
#include <SPI.h> 
#include <MFRC522.h> 
#include <ESP8266WiFi.h> 
#include <PubSubClient.h> 

// Update these with values suitable for your network. 
const char* ssid = "gelombang cinta"; 
const char* password = "rindukoe"; 
const char* mqtt_server = "broker.emqx.io";  // broker gratisan 

#define SS_PIN 4 
#define RST_PIN 5 
MFRC522 rfid(SS_PIN, RST_PIN); 
Servo myServo; 
int LED_G = D0; 
int LED_R = D8; 
int BUZZER = D3; 

unsigned long previousMillis = 0; 
const long interval = 1000;  // Waktu interval dalam milidetik 

byte lastUID[10]; 
bool isCardPresent = false; 
int failCount = 0; 
bool isBlocked = false; 

WiFiClient espClient; 
PubSubClient client(espClient); 
unsigned long lastMsg = 0; 
#define MSG_BUFFER_SIZE (50) 
char msg[MSG_BUFFER_SIZE]; 

void setup() { 
  Serial.begin(9600); 
  pinMode(LED_G, OUTPUT); 
  pinMode(LED_R, OUTPUT); 
  pinMode(BUZZER, OUTPUT);  // Inisialisasi pin buzzer 
  myServo.attach(2);        // Menggunakan pin D2 untuk servo 
  myServo.write(0); 
  SPI.begin(); 
  rfid.PCD_Init(); 
  memset(lastUID, 0, sizeof(lastUID)); 

  delay(10); 
  // We start by connecting to a WiFi network 
  Serial.println(); 
  Serial.print("Connecting to "); 
  Serial.println(ssid); 

  WiFi.mode(WIFI_STA); 
  WiFi.begin(ssid, password); 

  while (WiFi.status() != WL_CONNECTED) { 
    delay(500); 
    Serial.print("."); 
  } 

  Serial.println(""); 
  Serial.println("WiFi connected"); 
  Serial.println("IP address: "); 
  Serial.println(WiFi.localIP()); 

  client.setServer(mqtt_server, 1883); 
  client.setCallback(callback);  // Set the callback function 
} 

void reconnect() { 
  // Loop until we're reconnected 
  while (!client.connected()) { 
    Serial.print("Attempting MQTT connection..."); 
    // Create a random client ID 
    String clientId = "PAiotPublisher-"; 
    clientId += String(random(0xffff), HEX); 
    // Attempt to connect 
    if (client.connect(clientId.c_str())) { 
      Serial.println("connected"); 
      client.subscribe("PA/RFID");  // Subscribe to the topic 
    } else { 
      Serial.print("failed, rc="); 
      Serial.print(client.state()); 
      Serial.println(" try again in 5 seconds"); 
      // Wait 5 seconds before retrying 
      delay(5000); 
    } 
  } 
} 

void callback(char* topic, byte* payload, unsigned int length) { 
  Serial.print("Message arrived ["); 
  Serial.print(topic); 
  Serial.print("] "); 
  for (unsigned int i = 0; i < length; i++) { 
    Serial.print((char)payload[i]); 
  } 
  Serial.println(); 

  // Add your code here to handle incoming messages if needed 
} 

void loop() { 
  if (!client.connected()) { 
    reconnect(); 
  } 
  client.loop(); 

  unsigned long currentMillis = millis(); 
  if (currentMillis - previousMillis >= interval) { 
    previousMillis = currentMillis; 
    checkForCard(); 
  } 
} 

void checkForCard() { 
  if (!rfid.PICC_IsNewCardPresent()) { 
    isCardPresent = false; 
    return; 
  } 
  if (!rfid.PICC_ReadCardSerial()) { 
    isCardPresent = false; 
    return; 
  } 

  // Simpan UID kartu yang baru 
  bool isNewCard = memcmp(rfid.uid.uidByte, lastUID, rfid.uid.size) != 0; 

  // Jika kartu yang baru berbeda dengan yang terakhir, simpan UID baru 
  if (isNewCard || !isCardPresent) { 
    memcpy(lastUID, rfid.uid.uidByte, rfid.uid.size); 
    isCardPresent = true; 
    handleCard(); 
  } 
} 

void handleCard() { 
  Serial.print("UID tag : "); 
  String content = ""; 
  for (byte i = 0; i < rfid.uid.size; i++) { 
    Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0 " : " "); 
    Serial.print(rfid.uid.uidByte[i], DEC); 
    content.concat(String(rfid.uid.uidByte[i] < 0x10 ? "0" : " ")); 
    content.concat(String(rfid.uid.uidByte[i], DEC)); 
  } 
  Serial.println(); 
  Serial.print("Pesan : "); 
  content.toUpperCase(); 
  Serial.println(content); 
  client.publish("PA/RFID", content.c_str()); 

  // if (content.substring(1) == "195 178 139 166") { 
  //   Serial.println("Hudzaifah"); 
  //   Serial.println("---AKSES DITERIMA---"); 
  //   Serial.println(); 
  //   digitalWrite(LED_G,HIGH); 
  //   digitalWrite(BUZZER, LOW); // Matikan buzzer 
  //   myServo.write(180); 
  //   delay(3000); 
  //   myServo.write(0); 
  //   delay(1000); 
  //   digitalWrite(LED_G, LOW); 
  //   failCount = 0; // Reset fail count on successful access 
  //   isBlocked = false; // Unblock card after successful access 
  // } else { 
  //   failCount++; 
  //   Serial.println("Kartu Gagal"); 
  //   Serial.println("---AKSES DITOLAK---"); 

  //   if (failCount >= 4) { 
  //     // isBlocked = true; 
  //     Serial.println("---KARTU DIBLOKIR---"); 
  //     digitalWrite(LED_R, HIGH); 
  //     for (int i = 0; i < 4; i++) { 
  //       digitalWrite(BUZZER, HIGH); 
  //       delay(200); 
  //       digitalWrite(BUZZER, LOW); 
  //       delay(200); 
  //       digitalWrite(BUZZER, HIGH); 
  //       delay(200); 
  //       digitalWrite(BUZZER, LOW); 
  //       delay(200); 
  //     } 
  //     digitalWrite(LED_R, LOW); 
  //   } else { 
  //     digitalWrite(LED_R, HIGH); 
  //     digitalWrite(BUZZER, HIGH); // Hidupkan buzzer 
  //     delay(1000); 
  //     digitalWrite(LED_R, LOW); 
  //     digitalWrite(BUZZER, LOW); // Matikan buzzer 
  //   } 
  // } 
}