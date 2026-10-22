#include <SPI.h>
#include <LoRa.h>

// LORA PINS (Arduino Nano)
#define SS      10
#define RST     9
#define DIO0    2

// VARIABLES
int previousSeq = 0;

long totalReceived = 0;
long totalExpected = 0;

float pdr = 100.0;

// SETUP
void setup() {

  Serial.begin(115200);

  while (!Serial);

  Serial.println("========================================");
  Serial.println("LORA RECEIVER INITIALIZING...");
  Serial.println("========================================");

  // ---------------- LoRa Setup ----------------

  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(433E6)) {

    Serial.println("LoRa init failed!");

    while (1);
  }

  Serial.println("LoRa Receiver Ready");
  Serial.println();

  // CSV HEADER
  Serial.println("SEQ,LAT,LON,GPS_STATUS,TX_TIME,BATT,RSSI,SNR,PDR,RX_MS");
}

// LOOP
void loop() {

  int packetSize = LoRa.parsePacket();

  if (packetSize) {

    String receivedData = "";

    while (LoRa.available()) {
      receivedData += (char)LoRa.read();
    }

    // RSSI & SNR
    int rssi = LoRa.packetRssi();

    float snr = LoRa.packetSnr();

    // PARSE DATA
    String fields[6];

    int fieldIndex = 0;

    int startPos = 0;

    for (int i = 0; i < receivedData.length(); i++) {

      if (receivedData[i] == ',') {

        fields[fieldIndex] =
          receivedData.substring(startPos, i);

        fieldIndex++;

        startPos = i + 1;
      }
    }

    // Last field

    fields[fieldIndex] =
      receivedData.substring(startPos);

    // EXTRACT VALUES
    int seq = fields[0].toInt();

    String lat = fields[1];

    String lon = fields[2];

    String gpsStatus = fields[3];

    String txTime = fields[4];

    String batt = fields[5];


    // PDR CALCULATION
    if (previousSeq == 0) {

      totalExpected = 1;
      totalReceived = 1;
    }
    else {

      totalExpected += (seq - previousSeq);

      totalReceived++;
    }

    previousSeq = seq;

    pdr = ((float)totalReceived /
           (float)totalExpected) * 100.0;


    // RX TIMESTAMP
    unsigned long rxMillis = millis();

    // SERIAL OUTPUT (CSV FORMAT)


    Serial.print(seq);
    Serial.print(", ");

    Serial.print(lat);
    Serial.print(", ");

    Serial.print(lon);
    Serial.print(", ");

    Serial.print(gpsStatus);
    Serial.print(", ");

    Serial.print(txTime);
    Serial.print(", ");

    Serial.print(batt);
    Serial.print("%\n");

    // OPTIONAL PRETTY DEBUG OUTPUT

    Serial.println("----------------------------------------");

    Serial.print("SEQ: ");
    Serial.println(seq);

    Serial.print("LATITUDE: ");
    Serial.println(lat);

    Serial.print("LONGITUDE: ");
    Serial.println(lon);

    Serial.print("GPS STATUS: ");
    Serial.println(gpsStatus);

    Serial.print("TX TIME: ");
    Serial.println(txTime);

    Serial.print("BATTERY: ");
    Serial.print(batt);
    Serial.println("%");

    Serial.print("RSSI: ");
    Serial.print(rssi);
    Serial.println(" dBm");

    Serial.print("SNR: ");
    Serial.print(snr);
    Serial.println(" dB");

    Serial.print("PDR: ");
    Serial.print(pdr, 1);
    Serial.println("%");

    Serial.print("RX UPTIME (ms): ");
    Serial.println(rxMillis);

    Serial.println("----------------------------------------");
    Serial.println();
  }
}