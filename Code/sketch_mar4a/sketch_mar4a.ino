#include <Wire.h>
#include <Adafruit_PN532.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Broches Relais
#define RELAY_PIN_1 16
#define RELAY_PIN_2 17

// NFC
#define IRQ_PIN 2
#define RESET_PIN 3
Adafruit_PN532 nfc(IRQ_PIN, RESET_PIN, &Wire);

// Wi-Fi & MQTT
const char* ssid = "MaamSaaliwu";
const char* password = "xelcom313";
const char* mqtt_server = "172.20.10.9";
const char* mqtt_topic = "casier/ouverture";

WiFiClient espClient;
PubSubClient client(espClient);
String dernierBadge = "";
bool casierOuvert = false;

void afficherMessage(String ligne1, String ligne2 = "") {
  display.clearDisplay();
  display.setTextSize(1.75);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(ligne1);
  if (ligne2 != "") {
    display.println(ligne2);
  }
  display.setCursor(0, 40);
  display.println("Kasse / Diallo");
  display.setCursor(0, 50);
  display.println("Casier Intelligent");
  display.display();
}

void setup() {
  Serial.begin(115200); 
  pinMode(RELAY_PIN_1, OUTPUT);
  pinMode(RELAY_PIN_2, OUTPUT);
  digitalWrite(RELAY_PIN_1, LOW);
  digitalWrite(RELAY_PIN_2, LOW);

  // Init écran OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Ecran OLED non détecté !"));
    while (true);
  }
  afficherMessage("Demarrage...");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  afficherMessage("WiFi connecte", WiFi.localIP().toString());

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  reconnect();

  Wire.begin();
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    afficherMessage("NFC erreur", "Verifie cablage");
    while (1);
  }

  nfc.SAMConfig();
  afficherMessage("NFC pret !");
}

void reconnect() {
  while (!client.connected()) {
    afficherMessage("Connexion MQTT...");
    if (client.connect("ESP32Client")) {
      afficherMessage("MQTT Connecte");
      client.subscribe(mqtt_topic);
    } else {
      delay(3000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.println("Message reçu : " + message);
  if (message.startsWith("CASIER:")) {
    if (!casierOuvert) {
      int numero = message.substring(7).toInt();
      afficherMessage("Acces autorise", "Ouverture casier " + String(numero));
      ouvrirCasier(numero);
      casierOuvert = true;
      delay(5000);
      casierOuvert = false;
    }
  } else if (message == "REFUS") {
    afficherMessage("Acces refuse !");
  }
}

String badgeValide() {
  uint8_t success, uid[7], uidLength;
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 1000);
  if (!success) return "";
  
  String badgeID = "";
  for (byte i = 0; i < uidLength; i++) {
    badgeID += String(uid[i], HEX);
  }
  badgeID.toUpperCase();

  afficherMessage("Badge detecte", badgeID);
  return badgeID;
}

void ouvrirCasier(int numero) {
  int pin = (numero == 1) ? RELAY_PIN_1 : (numero == 2) ? RELAY_PIN_2 : -1;
  if (pin == -1) {
    afficherMessage("Casier invalide");
    return;
  }
  digitalWrite(pin, HIGH);
  delay(5000);
  digitalWrite(pin, LOW);
}

void verifierConnexion() {
  if (WiFi.status() != WL_CONNECTED) {
    afficherMessage("WiFi perdu !");
    WiFi.disconnect();
    WiFi.reconnect();
    delay(5000);
  }
  if (!client.connected()) {
    afficherMessage("MQTT perdu !");
    reconnect();
  }
}

void loop() {
  verifierConnexion();

  String badgeID = badgeValide();
  if (badgeID != "" && badgeID != dernierBadge && badgeID != "REFUS") {
    afficherMessage("Envoi UID...", badgeID);
    client.publish(mqtt_topic, badgeID.c_str(), false);
    dernierBadge = badgeID;
  }

  client.loop();
}
