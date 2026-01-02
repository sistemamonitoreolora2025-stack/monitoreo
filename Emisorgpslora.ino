#include <TinyGPSPlus.h>
#include <HardwareSerial.h>
#include <SPI.h>
#include <LoRa.h>
// ================= LORA =================
#define LORA_SS   18
#define LORA_RST  14
#define LORA_DIO0 26

// ================= GPS ==================
#define GPS_RX 16   // RX ESP32 <- TX GPS
#define GPS_TX 17   // TX ESP32 -> RX GPS

HardwareSerial gpsSerial(2);
TinyGPSPlus gps;

// ================= TIMING =================
unsigned long lastSend = 0;
const unsigned long interval = 10000; // enviar cada 10 s

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println("=== EMISOR GPS LORA ===");

  // ---------- GPS ----------
  gpsSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
  Serial.println("GPS iniciado");

  // ---------- LORA ----------
  SPI.begin(5, 19, 27, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(915E6)) {
    Serial.println("Error LoRa");
    while (1);
  }

  Serial.println("LoRa OK");
}

void loop() {
  // Leer datos GPS
  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  // Enviar solo si hay fix
  if (gps.location.isValid() && millis() - lastSend > interval) {

    float lat = gps.location.lat();
    float lon = gps.location.lng();
    int sats = gps.satellites.value();

    String payload = "LAT=" + String(lat, 6) +
                     ",LON=" + String(lon, 6) +
                     ",SAT=" + String(sats);

    LoRa.beginPacket();
    LoRa.print(payload);
    LoRa.endPacket();

    Serial.println("Enviado:");
    Serial.println(payload);

    lastSend = millis();
  }
}
