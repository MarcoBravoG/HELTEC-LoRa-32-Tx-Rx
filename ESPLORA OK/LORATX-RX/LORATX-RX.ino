/**
 * Enviar y recibir paquetes con modulación LoRa con un número de secuencia, mostrando
 * el RSSI y el SNR para los paquetes recibidos en la pequeña pantalla.
 *
 * Nota: Aunque esto envía y recibe utilizando modulación LoRa, no usa LoRaWAN.
 * Para eso, vea el ejemplo LoRaWAN_TTN.
 *
 * Funciona en el stick, pero la salida en la pantalla se corta.
*/

// Convierte el botón 'PRG' en el botón de encendido, una pulsación larga apaga
#define HELTEC_POWER_BUTTON   // debe estar antes de "#include <heltec_unofficial.h>"
#include <heltec_unofficial.h>

// Pausa entre los paquetes transmitidos en segundos.
// Establecer en cero para solo transmitir un paquete al presionar el botón del usuario
// No excederá el ciclo de trabajo del 1%, incluso si se establece un valor más bajo.
#define PAUSE               30

// Frecuencia en MHz. Mantenga el punto decimal para designar float.
// Verifique sus propias reglas y regulaciones para ver qué es legal en su área.
#define FREQUENCY           866.3       // para Europa
// #define FREQUENCY           905.2       // para EE.UU.

// Ancho de banda de LoRa. Mantenga el punto decimal para designar float.
// Los valores permitidos son 7.8, 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125.0, 250.0 y 500.0 kHz.
#define BANDWIDTH           250.0

// Número de 5 a 12. Un número mayor significa más lento pero mayor "ganancia de procesamiento",
// lo que significa (en resumen) mayor alcance y más robustez contra interferencias.
#define SPREADING_FACTOR    9

// Potencia de transmisión en dBm. 0 dBm = 1 mW, suficiente para pruebas en mesa.
// Este valor puede establecerse entre -9 dBm (0.125 mW) y 22 dBm (158 mW). Nota: la ERP máxima
// (que es lo que irradia su antena de manera máxima) en la banda ISM de la UE es de 25 mW, y 
// transmitir sin una antena puede dañar su hardware.
#define TRANSMIT_POWER      0

String rxdata;                 // Variable para almacenar datos recibidos
volatile bool rxFlag = false;  // Bandera para indicar la recepción de un paquete
long counter = 0;              // Contador de paquetes transmitidos
uint64_t last_tx = 0;          // Tiempo del último envío
uint64_t tx_time;              // Tiempo de transmisión
uint64_t minimum_pause;        // Pausa mínima entre transmisiones

void setup() {
  heltec_setup();   // Configurar la pantalla y LED del dispositivo Heltec
  both.println("Radio init");   // Mostrar mensaje de inicialización en pantalla y Serial
  RADIOLIB_OR_HALT(radio.begin());  // Iniciar el módulo de radio y verificar errores
  // Configurar la función de callback para manejar la recepción de paquetes
  radio.setDio1Action(rx);
  // Configurar los parámetros de la radio
  both.printf("Frequency: %.2f MHz\n", FREQUENCY);   // Mostrar la frecuencia configurada
  RADIOLIB_OR_HALT(radio.setFrequency(FREQUENCY));  // Configurar la frecuencia en el módulo de radio
  both.printf("Bandwidth: %.1f kHz\n", BANDWIDTH);   // Mostrar el ancho de banda configurado
  RADIOLIB_OR_HALT(radio.setBandwidth(BANDWIDTH));   // Configurar el ancho de banda en el módulo de radio
  both.printf("Spreading Factor: %i\n", SPREADING_FACTOR);  // Mostrar el factor de expansión configurado
  RADIOLIB_OR_HALT(radio.setSpreadingFactor(SPREADING_FACTOR));  // Configurar el factor de expansión
  both.printf("TX power: %i dBm\n", TRANSMIT_POWER);  // Mostrar la potencia de transmisión configurada
  RADIOLIB_OR_HALT(radio.setOutputPower(TRANSMIT_POWER));  // Configurar la potencia de transmisión
  // Iniciar la recepción de paquetes
  RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
}

void loop() {
  heltec_loop();   // Actualizar la pantalla y LED del dispositivo Heltec
  
  bool tx_legal = millis() > last_tx + minimum_pause;   // Verificar si es legal transmitir ahora
  
  // Transmitir un paquete cada PAUSE segundos o cuando se presiona el botón
  if ((PAUSE && tx_legal && millis() - last_tx > (PAUSE * 1000)) || button.isSingleClick()) {
    // En caso de pulsar el botón, informar al usuario que espere
    if (!tx_legal) {
      both.printf("Legal limit, wait %i sec.\n", (int)((minimum_pause - (millis() - last_tx)) / 1000) + 1);
      return;
    }
    both.printf("TX [%s] ", String(counter).c_str());   // Mostrar el número de paquete a transmitir
    radio.clearDio1Action();   // Limpiar la acción del pin DIO1
    heltec_led(50);   // Encender el LED al 50% de brillo
    tx_time = millis();   // Registrar el tiempo de inicio de la transmisión
    RADIOLIB(radio.transmit(String(counter++).c_str()));   // Transmitir el número de paquete actualizado
    tx_time = millis() - tx_time;   // Calcular el tiempo de transmisión
    heltec_led(0);   // Apagar el LED
    if (_radiolib_status == RADIOLIB_ERR_NONE) {   // Verificar si la transmisión fue exitosa
      both.printf("OK (%i ms)\n", (int)tx_time);   // Mostrar mensaje de éxito y tiempo de transmisión
    } else {
      both.printf("fail (%i)\n", _radiolib_status);   // Mostrar mensaje de falla y código de error
    }
    // Establecer el tiempo mínimo de pausa basado en el tiempo de transmisión para cumplir con el ciclo de trabajo máximo
    minimum_pause = tx_time * 100;
    last_tx = millis();   // Actualizar el tiempo del último envío
    radio.setDio1Action(rx);   // Restablecer la acción del pin DIO1 para manejar recepciones
    RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));   // Volver a iniciar la recepción
  }

  // Si se recibió un paquete, mostrarlo y también el RSSI y SNR
  if (rxFlag) {
    rxFlag = false;   // Restablecer la bandera de recepción
    radio.readData(rxdata);   // Leer los datos recibidos
    if (_radiolib_status == RADIOLIB_ERR_NONE) {   // Verificar si la lectura fue exitosa
      both.printf("RX [%s]\n", rxdata.c_str());   // Mostrar los datos recibidos
      Serial.printf("RX [%s]\n", rxdata.c_str());
      both.printf("  RSSI: %.2f dBm\n", radio.getRSSI());   // Mostrar el RSSI de la señal recibida
      both.printf("  SNR: %.2f dB\n", radio.getSNR());   // Mostrar el SNR de la señal recibida
    }
    RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));   // Reiniciar la recepción
  }
}

// No se pueden hacer cosas de Serial o pantalla aquí, toma demasiado tiempo para la interrupción
void rx() {
  rxFlag = true;   // Establecer la bandera de recepción cuando se llama esta función
}
