#include <esp_now.h>
#include <WiFi.h>
#include <esp_sleep.h>

#define BOTON 3
#define LED 8

uint8_t macEsclavo[] = {
  0x6C, 0xC8, 0x40, 0x59, 0x32, 0x38
};

bool mensaje = true;
bool enviado = false;

// Callback de envío para ESP32-C3 / Arduino-ESP32 actual
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Estado: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Exito" : "Error");

  if (status == ESP_NOW_SEND_SUCCESS) {
    enviado = true;
    Serial.println("Si se envio jeje");
  }
}

void setup() {

  Serial.begin(115200);
  delay(100);

  pinMode(BOTON, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);

  // Revisar causa del despertar
  esp_sleep_wakeup_cause_t causa = esp_sleep_get_wakeup_cause();

  if (causa == ESP_SLEEP_WAKEUP_GPIO) {

    Serial.println(">>> Desperte por boton");

    // Configurar WiFi para ESP-NOW
    WiFi.mode(WIFI_STA);

    if (esp_now_init() != ESP_OK) {
      Serial.println("Error iniciando ESP-NOW");
      digitalWrite(LED, LOW);
      delay(1000);
      goto dormir;
    }

    // Registrar callback
    esp_now_register_send_cb(OnDataSent);

    // Configurar dispositivo esclavo
    esp_now_peer_info_t peerInfo = {};

    memcpy(peerInfo.peer_addr, macEsclavo, 6);

    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      Serial.println("Error agregando peer");
      goto dormir;
    }

    Serial.println("Enviando mensaje...");

    esp_err_t resultado = esp_now_send(
      macEsclavo,
      (uint8_t *)&mensaje,
      sizeof(mensaje));

    if (resultado == ESP_OK) {
      Serial.println("Mensaje enviado");

    } else {
      Serial.println("Error enviando mensaje");
    }

    // Dar tiempo al callback de ESP-NOW
    delay(300);
  } else {
    Serial.println("Inicio normal -> esperando boton");
  }

dormir:

  if (enviado == true) {
    digitalWrite(LED, LOW);
    delay(4000);
    digitalWrite(LED, HIGH);
    delay(1000);
  }

  digitalWrite(LED, LOW);
  delay(1000);
  digitalWrite(LED, HIGH);
  delay(1000);
  Serial.println("Durmiendo...");

  /*
     ESP32-C3:
     Despertar cuando GPIO3 pase a LOW.

     El boton debe conectar GPIO3 con GND.
  */
  esp_deep_sleep_enable_gpio_wakeup(
    (1ULL << BOTON),
    ESP_GPIO_WAKEUP_GPIO_LOW);

  delay(100);

  esp_deep_sleep_start();
}

void loop() {
  // Nunca llega aquí porque usamos deep sleep
}