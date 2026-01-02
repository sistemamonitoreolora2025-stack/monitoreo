#include <SPI.h>
#include <LoRa.h>
#include <SD.h>
#include <WiFi.h>

// ================== PINES ==================
#define LORA_SS   18
#define LORA_RST  14
#define LORA_DIO0 26

#define SD_CS 13

// ================== WIFI ===================
const char* ssid = "TU_WIFI";
const char* password = "TU_PASSWORD";

// ================= THINGSPEAK ==============
String apiKey = "TU_WRITE_API_KEY";
const char* server = "api.thingspeak.com";

// ================= SPI SD ==================
SPIClass spiSD(VSPI);

WiFiClient client;

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println("\n=== HELTEC LORA + SD + CLOUD ===");

  // ---------- SD ----------
  spiSD.begin(5, 19, 27, SD_CS);
  if (!SD.begin(SD_CS, spiSD)) {
    Serial.println("SD no detectada");
  } else {
    Serial.println("SD lista");
  }

  // ---------- LoRa ----------
  SPI.begin(5, 19, 27, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(915E6)) {
    Serial.println("Error LoRa");
    while (1);
  }
  Serial.println("LoRa iniciado");

  // ---------- WiFi ----------
  WiFi.begin(ssid, password);
  Serial.print("Conectando WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi conectado");
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {

    String data = "";
    while (LoRa.available()) {
      data += (char)LoRa.read();
    }

    int rssi = LoRa.packetRssi();

    Serial.println("LoRa recibido:");
    Serial.println(data);
    Serial.print("RSSI: ");
    Serial.println(rssi);

    guardarSD(data, rssi);
    enviarThingSpeak(data);
  }
}

// ================= GUARDAR SD =================
void guardarSD(String msg, int rssi) {
  File f = SD.open("/lora_log.csv", FILE_APPEND);
  if (!f) {
    Serial.println("Error SD");
    return;
  }

  f.print(msg);
  f.print(",");
  f.println(rssi);
  f.close();

  Serial.println("Guardado en SD");
}

// ================= ENVIAR A THINGSPEAK =================
void enviarThingSpeak(String data) {

  // Ejemplo: T=24.5,H=63,L=320
  float T = extraerValor(data, "T");
  float H = extraerValor(data, "H");
  float L = extraerValor(data, "L");

  if (client.connect(server, 80)) {
    String url = "/update?api_key=" + apiKey +
                 "&field1=" + String(T) +
                 "&field2=" + String(H) +
                 "&field3=" + String(L);

    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + server + "\r\n" +
                 "Connection: close\r\n\r\n");

    Serial.println("☁️ Enviado a ThingSpeak");
  }
  client.stop();
}

// ================= PARSER SIMPLE =================
float extraerValor(String data, String key) {
  int i = data.indexOf(key + "=");
  if (i == -1) return 0;
  int f = data.indexOf(",", i);
  if (f == -1) f = data.length();
  return data.substring(i + 2, f).toFloat();
}
