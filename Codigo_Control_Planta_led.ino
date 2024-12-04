#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// Configuración del DHT11
#define DHTPIN 4  // Pin GPIO del sensor DHT11
#define DHTTYPE DHT11

const int ledPin = 5; // GPIO2 conectado al LED
//const float umbralTemperatura = 25.0; // Umbral para encender el "ventilador"

DHT dht(DHTPIN, DHTTYPE);

// Dirección I2C del módulo LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Configuración WiFi
const char* ssid = "test";        
const char* password = "test1234"; 

// Configuración MQTT
const char* mqttServer = "docker.coldspace.cl"; // Dirección IP o dominio del broker MQTT
const int mqttPort = 1883;               // Puerto del broker MQTT
const char* mqttUser = "USUARIO_MQTT";   // Usuario MQTT (opcional)
const char* mqttPassword = "CONTRASEÑA"; // Contraseña MQTT (opcional)
const char* topicHumedadSuelo = "planta/humedadSuelo";
const char* topicHumedadAire = "planta/humedadAire";
const char* topicTemperatura = "planta/te0mperatura";

WiFiClient espClient;
PubSubClient client(espClient);

// Configuración del sensor de humedad del suelo
int analogPin = 35;
int valorMinimo = 0;    // Ajusta según el sensor
int valorMaximo = 4095; // Ajusta según el sensor

void setup() {
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  Serial.begin(1200);
  
  lcd.init();
  lcd.backlight();
  dht.begin();

  // Conexión WiFi
  WiFi.begin(ssid, password);
  lcd.setCursor(0, 0);
  lcd.print("Conectando WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  lcd.clear();
  lcd.print("WiFi conectado");
  delay(1000);

  // Configuración MQTT
  client.setServer(mqttServer, mqttPort);
  conectarMQTT();
}

void conectarMQTT() {
  while (!client.connected()) {
    Serial.println("Conectando a MQTT...");
    if (client.connect("ESP32", mqttUser, mqttPassword)) {
      Serial.println("Conectado a MQTT");
    } else {
      Serial.print("Falló conexión: ");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    conectarMQTT();
  }
  client.loop();

  // Leer valores de los sensores
  float humedadSuelo = map(analogRead(analogPin), valorMinimo, valorMaximo, 0, 100);
  humedadSuelo = constrain(humedadSuelo, 0, 100);

  float humedadAire = dht.readHumidity();
  float temperatura = dht.readTemperature();


  if (!isnan(humedadAire) && !isnan(temperatura)) {
    // Enviar datos por MQTT
    char mensaje[10];

    //datos publicados
    dtostrf(humedadSuelo, 4, 2, mensaje);
    client.publish(topicHumedadSuelo, mensaje);

    dtostrf(humedadAire, 4, 2, mensaje);
    client.publish(topicHumedadAire, mensaje);

    dtostrf(temperatura, 4, 2, mensaje);
    client.publish(topicTemperatura, mensaje);

    //datos recibidos
    dtostrf(temperatura, 4, 2, mensaje);
    client.subscribe("esp32/led");

     dtostrf(temperatura, 4, 2, mensaje);
    client.subscribe("esp32/led/low");

    
    if( client.subscribe("esp32/led")) {
      digitalWrite(ledPin, HIGH);
    }
      
    if (client.subscribe("esp32/led/low")){
      digitalWrite(ledPin, LOW);
    }


    // Mostrar en LCD
    lcd.setCursor(0, 0);
    lcd.print("T:");
    lcd.print(temperatura);
    lcd.print("C H:");
    lcd.print(humedadAire);
    lcd.print("%");

    lcd.setCursor(0, 1);
    lcd.print("H.Suelo:");
    lcd.print(humedadSuelo);
    lcd.print("%");
  } else {
    Serial.println("Error al leer DHT11");
  }

  delay(5000); // Cada 5 segundos
}
