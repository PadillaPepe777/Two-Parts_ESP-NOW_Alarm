#include <esp_now.h>
#include <WiFi.h>

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#define ALARM_START 26
#define Buzzer 23
//SDA 21 SCL 22
#define uS_TO_S_FACTOR 1000000ULL      

bool datoRecibido = false;
bool primerBoot=false;

RTC_DATA_ATTR bool Sonido = false;
RTC_DATA_ATTR uint64_t tiempoObjetivo_us = 0;
RTC_DATA_ATTR int Horas=7;
RTC_DATA_ATTR int Minutos=30;

// Recibe info + payload
void OnDataRecv(const esp_now_recv_info_t * recv_info,
                const uint8_t *incomingData, int len) 
{
  memcpy(&datoRecibido, incomingData, sizeof(datoRecibido));

  if (datoRecibido) {
    Serial.println("¡Recibido!");
  }
}

#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//COsas del encoder 
#define EncButton 14
#define OUTA 32 //CLK
#define OUTB 25 //DT

int cambio=0;

//Tiempo a dormir
#define uS_TO_S_FACTOR 1000000ULL 
uint64_t TIME_TO_SLEEP = 60;    
bool ModoHora=false;

bool ultimoCLK = true;

String Arriba, Abajo;

esp_sleep_wakeup_cause_t wakeup_reason;

void setup() {
  Serial.begin(115200);
  delay(1000);

  wakeup_reason = esp_sleep_get_wakeup_cause();
  switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_TIMER:
      Serial.println("Desperté por TIMER (alarma)");
      Sonido = true;   // 👈 activa la alarma
      break;

    case ESP_SLEEP_WAKEUP_EXT0:
      Serial.println("Desperté por BOTÓN / ENCODER");
      // NO activar alarma
      break;

    case ESP_SLEEP_WAKEUP_UNDEFINED:
    default:
      Serial.println("Arranque normal");
      primerBoot=true;
      break;
  }

  Wire.begin();
  
if (!display.begin(SCREEN_ADDRESS, true)) {
  Serial.println("SH1106 no detectado");
  while (1);
}

delay(100);
display.clearDisplay();
display.display();

  pinMode(ALARM_START, INPUT_PULLUP);
  pinMode(Buzzer, OUTPUT);

delay(100);
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error iniciando ESP-NOW");
    return;
  }

  // Nuevo callback obligatorio
  esp_now_register_recv_cb(OnDataRecv);

  //esp_sleep_enable_timer_wakeup((uint64_t)TIME_TO_SLEEP * uS_TO_S_FACTOR);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_26,0);

  pinMode(OUTA, INPUT);
  pinMode(OUTB, INPUT);
  pinMode(EncButton, INPUT_PULLUP);

  if(primerBoot)
  {    
    Arriba="Hola!";
    Abajo="dormire...";
    displayText();
    Serial.println("Primer Encendido, a dormir...");
    delay(3500);
    display.clearDisplay();
    display.display();
    Serial.flush();
    esp_deep_sleep_start();
  }
  else
  Serial.println("Esclavo listo, esperando mensajes...");
}

void loop() {

  //Ya que pongamos sleep normal, agregar case para que entre aqui por la razon wake up correcta
  //Cuidar no interferir si se despierta antes por el encoder
if(wakeup_reason==ESP_SLEEP_WAKEUP_TIMER)
{
  if (Sonido) {
    Serial.println("Suena la bocina");
    Arriba="Buenas!";
    Abajo="DESPIERTA";
    displayText();
    delay(1000);
    digitalWrite(Buzzer, HIGH);

    // Espera a que llegue el mensaje para apagar
    while (1) {
      if (datoRecibido == true) {
        digitalWrite(Buzzer, LOW);
        Sonido = false;
        Serial.println("Apagando la bocina");
        datoRecibido = false;
        break;
      }
      delay(100);
    }
  }
  Arriba="Ten un";
  Abajo="Buen Dia";
  displayText();
  delay(3000);
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
  delay(100);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_26,0);
  delay(100);
  display.clearDisplay();
  display.display();
  Serial.flush();
  delay(100);
  esp_deep_sleep_start();
}

    Arriba="Hola!";
    Abajo=" :D ";
    displayText();

  if(!digitalRead(EncButton) && !digitalRead(ALARM_START))
  {
    Serial.println("Por ahora no alarma...");
    Arriba="Estare";
    Abajo=" Esperando ";
    displayText();
    delay(3000);
    display.clearDisplay();
    display.display();
    Serial.flush();
    esp_deep_sleep_start();
  }

  if(!digitalRead(EncButton))
  {
    delay(20);
    unsigned long tiempoActual=millis();
    cambio=leerEncoder();
    Serial.println(cambio);

    normalTime();
    displaySetting();

    while(millis()-tiempoActual<3000)
    {
      cambio=leerEncoder();

      if(!digitalRead(EncButton))
      {
      ModoHora=!ModoHora;
      delay(50);
      }

      if(cambio!=0)
      { 
        sumar();

        tiempoActual=millis();

        normalTime();
        displaySetting();
      }

    }
    
    Serial.println(String(Horas)+":"+String(Minutos));

    TIME_TO_SLEEP=(((Horas*60)+(Minutos))*60);
    esp_sleep_enable_timer_wakeup((uint64_t)TIME_TO_SLEEP * uS_TO_S_FACTOR);

    display.clearDisplay();
    display.display();

  }

  // Botón que activa la alarma
  if (!digitalRead(ALARM_START)) {
    Sonido = true;
    Serial.println("Going to sleep now");
    Arriba="A dormir";
    Abajo="ZZZ";
    displayText();
    delay(3000);
    display.clearDisplay();
    display.display();
    Serial.flush();
    esp_deep_sleep_start();
  }

  delay(100);
}


void displaySetting()
{
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0,0);
  display.println("TIEMPO DE ALARMA:");
  display.setCursor(0,32);
  if(Minutos/10.0>=1.0)
  display.println(String(Horas)+":"+String(Minutos));
  else
  display.println(String(Horas)+":0"+String(Minutos));
  display.display();
}

void normalTime(){
  if(Minutos>=60)
  {
    Horas++;
    Minutos=Minutos-60;
  }

  if(Minutos<0 && Horas>=1)
  {
    Minutos=59;
    Horas--;
  }

  if(Horas<0)
  Horas=0;
  if(Horas>23)
  Horas=23;
          
  if(Minutos<=0 && Horas<=0)
  Minutos=1;
  }

  void sumar(){
    if(ModoHora)
    Horas=Horas+cambio;
    else
    Minutos=Minutos+cambio;
  }

    void restar(){
    if(ModoHora)
    Horas--;
    else
    Minutos--;
  }

  void displayText()
{
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(10,0);
  display.println(Arriba);
  display.setCursor(10,32);
  display.println(Abajo);
  display.display();
}

int leerEncoder()
{

  int clk = digitalRead(OUTA);

  if(clk != ultimoCLK)
  {

      ultimoCLK = clk;
      if(digitalRead(OUTB)!=clk)
        return -1;
      else
        return 1;
    
  }
  ultimoCLK = clk;
  return 0;
}