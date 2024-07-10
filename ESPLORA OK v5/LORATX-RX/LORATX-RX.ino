#include <heltec_unofficial.h>

#define HELTEC_POWER_BUTTON
#define FREQUENCY 866.3
#define BANDWIDTH 500
#define SPREADING_FACTOR 7

String rxdata;
volatile bool rxFlag = false;

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
  
  RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));

  Serial.begin(9600); // Inicializar comunicación serial
}

void loop() {
  heltec_loop();

  // Leer datos del puerto serial
  if (Serial.available()) {
    char receivedChar = Serial.read();
    if (receivedChar == '1') {
      heltec_led(100); // Encender LED integrado
      both.println("LED Encendido");
    } else if (receivedChar == '0') {
      heltec_led(0); // Apagar LED integrado
      both.println("LED Apagado");
    }
  }

  if (rxFlag) {
    rxFlag = false;
    radio.readData(rxdata);
    if (_radiolib_status == RADIOLIB_ERR_NONE) {
      both.printf("%s\n", rxdata.c_str());
      both.printf("  RSSI: %.2f dBm\n", radio.getRSSI());
      both.printf("  SNR: %.2f dB\n", radio.getSNR());

      // Copiar rxdata a un buffer modificable
      char buffer[100];
      rxdata.toCharArray(buffer, sizeof(buffer));

      // Descomponer los datos recibidos en los valores de los sensores
      int sensorValues[4];
      int index = 0;
      char *token = strtok(buffer, ",");
      while (token != NULL && index < 4) {
        sensorValues[index++] = atoi(token);
        token = strtok(NULL, ",");
      }

      // Verificar si se recibieron cuatro valores de sensor
      if (index == 4) {
        both.printf("Sensor 1: %d\n", sensorValues[0]);
        both.printf("Sensor 2: %d\n", sensorValues[1]);
        both.printf("Sensor 3: %d\n", sensorValues[2]);
        both.printf("Sensor 4: %d\n", sensorValues[3]);

        // Condiciones para encender o apagar el LED
        if (sensorValues[0] < 24 && sensorValues[1] > 40 && sensorValues[2] > 70 && sensorValues[3] > 50) {
          heltec_led(100); // Encender LED
          both.println("LED Encendido por condiciones de sensores");
        } else {
          heltec_led(0); // Apagar LED
          both.println("LED Apagado por condiciones de sensores");
        }
      } else {
        both.printf("Error: No se recibieron cuatro valores de sensor. Índice: %d\n", index);
      }
    }
    RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
  }
}

void rx() {
  rxFlag = true;
}
