#include <heltec_unofficial.h>

#define HELTEC_POWER_BUTTON
#define PAUSE 30
#define FREQUENCY 866.3
#define BANDWIDTH 500
#define SPREADING_FACTOR 7
#define TRANSMIT_POWER 0

String rxdata;
volatile bool rxFlag = false;
long counter = 0;
uint64_t last_tx = 0;
uint64_t tx_time;
uint64_t minimum_pause;

void setup() {
  heltec_setup();
  both.println("Radio init");
  RADIOLIB_OR_HALT(radio.begin());
  
  radio.setDio1Action(rx);
  both.printf("Frequency: %.2f MHz\n", FREQUENCY);
  RADIOLIB_OR_HALT(radio.setFrequency(FREQUENCY));
  both.printf("Bandwidth: %.1f kHz\n", BANDWIDTH);
  RADIOLIB_OR_HALT(radio.setBandwidth(BANDWIDTH));
  both.printf("Spreading Factor: %i\n", SPREADING_FACTOR);
  RADIOLIB_OR_HALT(radio.setSpreadingFactor(SPREADING_FACTOR));
  both.printf("TX power: %i dBm\n", TRANSMIT_POWER);
  RADIOLIB_OR_HALT(radio.setOutputPower(TRANSMIT_POWER));
  
  RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
}

void loop() {
  heltec_loop();
  
  bool tx_legal = millis() > last_tx + minimum_pause;

  if ((PAUSE && tx_legal && millis() - last_tx > (PAUSE * 100)) || button.isSingleClick()) {
    if (!tx_legal) {
      both.printf("Legal limit, wait %i sec.\n", (int)((minimum_pause - (millis() - last_tx)) / 1000) + 1);
      return;
    }

    int sensorValue1 = analogRead(A0); // Leer valor del primer sensor
    int sensorValue2 = analogRead(A1); // Leer valor del segundo sensor
    int sensorValue3 = analogRead(A2); // Leer valor del tercer sensor
    
    String sensorData = String(sensorValue1) + "," + String(sensorValue2) + "," + String(sensorValue3);
    
    both.printf("TX [%s] ", sensorData.c_str());
    
    radio.clearDio1Action();
    heltec_led(50);
    tx_time = millis();
    RADIOLIB(radio.transmit(sensorData.c_str()));
    tx_time = millis() - tx_time;
    heltec_led(0);

    if (_radiolib_status == RADIOLIB_ERR_NONE) {
      both.printf("OK (%i ms)\n", (int)tx_time);
    } else {
      both.printf("fail (%i)\n", _radiolib_status);
    }

    minimum_pause = tx_time * 100;
    last_tx = millis();
    radio.setDio1Action(rx);
    RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
  }

  if (rxFlag) {
    rxFlag = false;
    radio.readData(rxdata);
    if (_radiolib_status == RADIOLIB_ERR_NONE) {
      both.printf("RX [%s]\n", rxdata.c_str());
      both.printf("  RSSI: %.2f dBm\n", radio.getRSSI());
      both.printf("  SNR: %.2f dB\n", radio.getSNR());
    }
    RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
  }
}

void rx() {
  rxFlag = true;
}


