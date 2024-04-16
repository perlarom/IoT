#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Stepper.h>

const char *ssid = "Internet xd";
const char *password = "hola12201";
const char *serverUrl = "http://www.accesotec.com.mx/datos-distancia";
const char *apiUrl = "http://www.accesotec.com.mx/api/alumnos";

#define PIN_SS 5
#define PIN_RST 22  
#define PIN_LED_VERDE 2  
#define PIN_LED_ROJO 21  

MFRC522 mfrc522(PIN_SS, PIN_RST);   
WiFiClient client;

// Definición de los pines para el motor paso a paso
const int stepsPerRev = 1200;
const int motorSpeed = 20;
const int motorPin1 = 26;
const int motorPin2 = 25;
const int motorPin3 = 33;
const int motorPin4 = 32;

// Inicialización del objeto stepper
Stepper stepper(stepsPerRev, motorPin1, motorPin2, motorPin3, motorPin4);

const int delayTime = 500;

void setup() {
  Serial.begin(115200);  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando a Wi-Fi...");
  }

  Serial.println("Conectado a Wi-Fi!");

  SPI.begin();      
  mfrc522.PCD_Init();   
  Serial.println("Aproxime su tarjeta al lector...");
  Serial.println();

  pinMode(PIN_LED_VERDE, OUTPUT);  
  pinMode(PIN_LED_ROJO, OUTPUT);  

  // Establecer la velocidad del motor
  stepper.setSpeed(motorSpeed);
}

void loop() {
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  Serial.print("UID de la tarjeta: ");
  String contenido = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    contenido.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    contenido.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  Serial.println();

  contenido.toUpperCase();

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    // Enviar UID al primer servidor
    http.begin(client, serverUrl);
    http.addHeader("Content-Type", "application/json");

    StaticJsonDocument<200> jsonDoc;
    jsonDoc["uid"] = contenido;

    String jsonStr;
    serializeJson(jsonDoc, jsonStr);

    int httpResponseCode = http.POST(jsonStr);
    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
    } else {
      Serial.print("HTTP Request failed. Error code: ");
      Serial.println(httpResponseCode);
    }
    http.end();

    // Realizar solicitud GET a la API de alumnos
    http.begin(client, apiUrl);
    int httpResponseCode2 = http.GET();

    if (httpResponseCode2 > 0) {
      DynamicJsonDocument jsonDoc2(1024);
      DeserializationError error = deserializeJson(jsonDoc2, http.getStream());

      if (!error) {
        JsonArray alumnos = jsonDoc2.as<JsonArray>();
        bool encontrado = false;

        for (JsonObject alumno : alumnos) {
          String idTarjeta = alumno["idtarjeta"].as<String>();
          idTarjeta.toUpperCase();

          if (idTarjeta == contenido) {
            encontrado = true;
            break;
          }
        }

        if (encontrado) {
          digitalWrite(PIN_LED_VERDE, HIGH);
          digitalWrite(PIN_LED_ROJO, LOW);
          stepper.step(stepsPerRev); // Activar motor
          delay(delayTime);  
          digitalWrite(PIN_LED_VERDE, LOW);
          stepper.step(-stepsPerRev); // Girar en sentido contrario para volver a la posición inicial
        } else {
          digitalWrite(PIN_LED_VERDE, LOW);
          digitalWrite(PIN_LED_ROJO, HIGH);
          delay(5000);  
          digitalWrite(PIN_LED_ROJO, LOW);
        }
      } else {
        Serial.print("Error al analizar JSON: ");
        Serial.println(error.c_str());
      }
    } else {
      Serial.print("Error al realizar la solicitud HTTP: ");
      Serial.println(httpResponseCode2);
    }
    http.end();
  }

  delay(3000);  
}
