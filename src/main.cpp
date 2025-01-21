/*
 * Author: João Pedro Martins Gomes
 * Date: 2025-01-21
 * Last Update: 2025-01-21
 */

#include <Arduino.h>
#include "NetworkClientSecure.h"
#include "PubSubClient.h"
#include "PPP.h"
#include "certificado.h"

// PPP

#define PPP_MODEM_APN "claro.com.br" // SUA APN
#define PPP_MODEM_PIN "0000"  // PIN DO MODEM, DEIXE 0000 CASO O MODEM ESTEJA DESBLOQUEADO

#define PPP_MODEM_TX      17 // PORTA TX PADRÃO
#define PPP_MODEM_RX      16 // PORTA RX PADRÃO
#define PPP_MODEM_RTS     -1 // NÃO ESTÁ SENDO UTILIZADO
#define PPP_MODEM_CTS     -1 // NÃO ESTÁ SENDO UTILIZADO
#define PPP_MODEM_FC      ESP_MODEM_FLOW_CONTROL_NONE // SEM CONTROLE DE FLUXO PADRÃO
#define PPP_MODEM_MODEL   PPP_MODEM_GENERIC // MODEM GENÉRICO

// Variáveis 

const char* user = ""; // USUÁRIO DO MQTT
const char* password = ""; // SENHA DO MQTT
const char* clientid = ""; // COLOQUE O SEU CLIENT_ID AQUI 
const char* pub_topic = ""; // TÓPICO DE PUBLICAÇÃO
const char* sub_topic = ""; // TÓPICO DE SUBSCRIÇÃO
const char* URL = ""; // URL DO BROKER 

// Declarações

bool dataMode = false; 
NetworkClientSecure client;
PubSubClient mqtt(client);

// put function declarations here:
int myFunction(int, int);
void onEvent(arduino_event_id_t event, arduino_event_info_t info);
void setClock();
void reconnect();
void callback(char* topic, byte* payload, unsigned int length);

void onEvent(arduino_event_id_t event, arduino_event_info_t info) {
  switch (event) {
    case ARDUINO_EVENT_PPP_START:        Serial.println("PPP Started"); break;
    case ARDUINO_EVENT_PPP_CONNECTED:    Serial.println("PPP Connected"); break;
    case ARDUINO_EVENT_PPP_GOT_IP:       Serial.println("PPP Got IP"); break;
    case ARDUINO_EVENT_PPP_LOST_IP:      Serial.println("PPP Lost IP"); break;
    case ARDUINO_EVENT_PPP_DISCONNECTED: Serial.println("PPP Disconnected"); break;
    case ARDUINO_EVENT_PPP_STOP:         Serial.println("PPP Stopped"); break;
    default:                             break;
  }
}

void setClock() {
  configTime(0, 0, "pool.ntp.org");

  Serial.print(F("Waiting for NTP time sync: "));
  time_t nowSecs = time(nullptr);
  while (nowSecs < 8 * 3600 * 2) {
    delay(500);
    Serial.print(F("."));
    yield();
    nowSecs = time(nullptr);
  }

  Serial.println();
  struct tm timeinfo;
  gmtime_r(&nowSecs, &timeinfo);
  Serial.print(F("Current time: "));
  Serial.print(asctime(&timeinfo));
}

void reconnect(){
  while(!mqtt.connected()){
    Serial.print("[MQTT] Conectando...");

    if(mqtt.connect(clientid)){
        Serial.print("[MQTT] Conectado!");
        mqtt.publish("esp32/youtube", "Um salve para a galera do canal da FAT no Youtube!");
        mqtt.subscribe("esp32/youtube");
    } else {
        Serial.print("[MQTT] Houve uma falha! erro =");
        Serial.print(mqtt.state());
        Serial.println(" ");
        delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length){
    Serial.print("Mensagem recebida: [");
    Serial.print(topic);
    Serial.print("]");

    char message[length];

    for(int i = 0; i < length; i++){
      message[i] = (char) payload[i];
    }

    Serial.println(message);
}

void setup() {
    Serial.begin(115200); // Inicia a porta serial UART0 a 115200 bps
    Network.onEvent(onEvent); // callback de rede 
    client.setCACert(rootCACertificate); // certificado de autenticação

    mqtt.setServer(URL, 8883);
    mqtt.setCallback(callback);

    PPP.setApn(PPP_MODEM_APN);
    PPP.setPins(PPP_MODEM_TX, PPP_MODEM_RX);
    PPP.begin(PPP_MODEM_MODEL, 2);

  bool attached = PPP.attached();
  if (!attached) {
    int i = 0;
    unsigned int s = millis();
    Serial.print("Aguardando conexão...");
    while (!attached && ((++i) < 600)) {
      Serial.print(".");
      delay(100);
      attached = PPP.attached();
    }
    Serial.print((millis() - s) / 1000.0, 1);
    Serial.println("s");
    attached = PPP.attached();
  }

    Serial.print("Attached: ");
    Serial.println(attached);
    Serial.print("State: ");
    Serial.println(PPP.radioState());
      if (attached) {
            Serial.print("Operator: ");
            Serial.println(PPP.operatorName());
            Serial.print("IMSI: ");
            Serial.println(PPP.IMSI());
            Serial.print("RSSI: ");
            Serial.println(PPP.RSSI());
        int ber = PPP.BER();
        if (ber > 0) {
            Serial.print("BER: ");
            Serial.println(ber);
            Serial.print("NetMode: ");
            Serial.println(PPP.networkMode());
        }

            Serial.println("Switching to data mode...");
            PPP.mode(ESP_MODEM_MODE_DATA);  // Data and Command mixed mode
        if (!PPP.waitStatusBits(ESP_NETIF_CONNECTED_BIT, 1000)) {
          Serial.println("Failed to connect to internet!");
        } else {
          Serial.println("Connected to internet!");
        }
    } else {
    Serial.println("Failed to connect to network!");
  }

  setClock();
}

void loop() {
  // put your main code here, to run repeatedly:
    if(!PPP.connected()){
      unsigned long _tg = millis();
      while(!PPP.connected()) {
        if (millis() > (_tg + 10000)) { PPP.mode(ESP_MODEM_MODE_CMUX); }
      }
    }

    if(!mqtt.connected()){
        reconnect();
    }

    mqtt.loop();
}