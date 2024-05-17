#define BLYNK_TEMPLATE_ID "TMPL6tyzY65kZ"
#define BLYNK_TEMPLATE_NAME "ProjekIotT"

#include <Servo.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <BlynkSimpleEsp8266.h> // Tambahkan library Blynk

// Update ini dengan nilai yang sesuai untuk jaringan Anda.
const char* ssid = "amin";
const char* password = "12345678";
const char* mqtt_server = "broker.emqx.io";  // broker gratisan

#define SS_PIN 4
#define RST_PIN 5
MFRC522 rfid(SS_PIN, RST_PIN);
Servo myServo;
int LED_G = D0;
int LED_R = D8;
int BUZZER = D3;  // Pin buzzer

char auth[] = "OaGIV5Pdntxyyh9yqaoRraELABm9FdGm"; // Masukkan token autentikasi Blynk

unsigned long previousMillis = 0;
const long interval = 1000;  // Waktu interval dalam milidetik

byte lastUID[10];
bool isCardPresent = false;
int failCount = 0;
bool isBlocked = false;

WiFiClient espClient;
PubSubClient client(espClient);

bool buzzerEnabled = true;  // Flag untuk menyimpan status buzzer

BLYNK_WRITE(V0) {  // Pin virtual V0 untuk kontrol buzzer
  buzzerEnabled = param.asInt();  // Update status buzzer berdasarkan switch
}

void setup() {
  Serial.begin(9600);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_R, OUTPUT);
  pinMode(BUZZER, OUTPUT);  // Inisialisasi pin buzzer
  myServo.attach(2);        // Menggunakan pin D2 untuk servo
  myServo.write(0);
  SPI.begin();
  rfid.PCD_Init();
  memset(lastUID, 0, sizeof(lastUID));

  Serial.println();
  Serial.print("Menghubungkan ke ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi terhubung");
  Serial.println("Alamat IP: ");
  Serial.println(WiFi.localIP());

  // Koneksi ke server Blynk
  Blynk.begin(auth, ssid, password);

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Pesan diterima [");
  Serial.print(topic);
  Serial.print("] ");
  String user = "";  // variabel untuk menyimpan data yang berbentuk array char
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    user += (char)payload[i];  // menyimpan kumpulan char ke dalam string
  }
  if (user.substring(1) == "195 178 139 166") {  // pengkondisian
    Serial.println("Hudzaifah");
    Serial.println("---AKSES DITERIMA---");
    Serial.println();
    digitalWrite(LED_G, HIGH);
    Blynk.virtualWrite(V1, HIGH);
    digitalWrite(BUZZER, LOW);  // Matikan buzzer
    myServo.write(180);
    delay(3000);
    myServo.write(0);
    delay(1000);
    digitalWrite(LED_G, LOW);
    failCount = 0;      // Reset fail count pada akses yang berhasil
    isBlocked = false;  // Unblock kartu setelah akses yang berhasil

    // Kontrol tampilan di Blynk
    delay(1000);
    Blynk.virtualWrite(V1, LOW);
    Blynk.virtualWrite(V2, LOW);  // Lampu merah mati
    Blynk.virtualWrite(V3, LOW);  // Lampu merah saat terblokir mati
  } else {
    failCount++;
    Serial.println("Kartu Gagal");
    Serial.println("---AKSES DITOLAK---");

    if (failCount >= 4) {
      isBlocked = true;
      Serial.println("---KARTU DIBLOKIR---");
      digitalWrite(LED_R, HIGH);
      Blynk.virtualWrite(V3, HIGH);
      if (buzzerEnabled) {  // Periksa jika buzzer diaktifkan
        for (int i = 0; i < 4; i++) {
          digitalWrite(BUZZER, HIGH);
          delay(200);
          digitalWrite(BUZZER, LOW);
          delay(200);
          digitalWrite(BUZZER, HIGH);
          delay(200);
          digitalWrite(BUZZER, LOW);
          delay(200);
        }
      }
      digitalWrite(LED_R, LOW);

      // Kontrol tampilan di Blynk
      Blynk.virtualWrite(V1, LOW);  // Lampu hijau mati
      Blynk.virtualWrite(V2, LOW); // Lampu merah menyala
      
      delay(500); // Lampu merah saat terblokir menyala
      Blynk.virtualWrite(V2, LOW);
      Blynk.virtualWrite(V3, LOW);
    } else {
      digitalWrite(LED_R, HIGH);
      if (buzzerEnabled) {  // Periksa jika buzzer diaktifkan
        digitalWrite(BUZZER, HIGH);
      }
      Blynk.virtualWrite(V2, HIGH);  // Hidupkan buzzer
      delay(1000);
      digitalWrite(LED_R, LOW);
      digitalWrite(BUZZER, LOW);  // Matikan buzzer

      // Kontrol tampilan di Blynk
      Blynk.virtualWrite(V1, LOW);  // Lampu hijau mati
      delay(1000);
      Blynk.virtualWrite(V2, LOW);
      Blynk.virtualWrite(V3, LOW);  // Lampu merah saat terblokir mati
    }
  }
}

void reconnect() {
  // Loop hingga kita terhubung kembali
  while (!client.connected()) {
    Serial.print("Mencoba koneksi MQTT...");
    // Buat ID klien acak
    String clientId = "PAiotSubscriber";
    clientId += String(random(0xffff), HEX);
    // Coba untuk terhubung
    if (client.connect(clientId.c_str())) {
      Serial.println("terhubung");
      client.subscribe("PA/RFID");
    } else {
      Serial.print("gagal, rc=");
      Serial.print(client.state());
      Serial.println(" coba lagi dalam 5 detik");
      // Tunggu 5 detik sebelum mencoba lagi
      delay(5000);
    }
  }
}

void loop() {
  // Blynk
  Blynk.run();

  if (!client.connected()) {
    reconnect();
  }

  client.loop();
}
