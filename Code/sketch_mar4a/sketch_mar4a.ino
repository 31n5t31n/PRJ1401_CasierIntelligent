#include <Wire.h>
#include <Adafruit_PN532.h>
#include <WiFi.h>
#include <PubSubClient.h>

// On defini les broches 
#define RELAY_PIN 16  // Broche du relais
#define IRQ_PIN 2     // Broche IRQ (optionnelle)
#define RESET_PIN 3   // Broche RESET (optionnelle)

// Initialisation du module NFC en I2C
Adafruit_PN532 nfc(IRQ_PIN, RESET_PIN, &Wire);

// Wi-Fi et MQTT
const char* ssid = "MaamSaaliwu";  // Mon Wi-FI 
const char* password = "xelcom313";
const char* mqtt_server = "172.20.10.9";
const char* mqtt_topic = "casier/ouverture";

WiFiClient espClient;
PubSubClient client(espClient);

String dernierBadge = "";  // Stocke le dernier badge détecté pour éviter les doublons

void setup() {
    Serial.begin(115200);
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW); // Relais désactivé au départ

    // Connexion WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\n✅ WiFi connecté !");

    // Connexion MQTT
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
    reconnect();

    // Initialisation du lecteur NFC
    Wire.begin();
    nfc.begin();
    Serial.println("🔍 Vérification de la connexion avec le module NFC...");

    uint32_t versiondata = nfc.getFirmwareVersion();
    if (!versiondata) {
        Serial.println("❌ Impossible d'obtenir la version du firmware. Vérifie le câblage !");
        while (1); // Bloque le programme ici
    }
    
    Serial.println("✅ Module NFC détecté !");
    Serial.print("📡 Version du firmware : 0x"); Serial.println(versiondata, HEX);
    
    nfc.SAMConfig(); // Active le mode de lecture continue
    Serial.println("🎯 Lecteur NFC prêt !");
}

void reconnect() {
    while (!client.connected()) {
        Serial.print("🔄 Connexion au broker MQTT...");
        if (client.connect("ESP32Client")) {
            Serial.println("✅ Connecté !");
            client.subscribe(mqtt_topic);
        } else {
            Serial.print("❌ Erreur : ");
            Serial.println(client.state());
            delay(5000);
        }
    }
}

// Callback MQTT pour gérer les messages du serveur
bool casierOuvert = false;

void callback(char* topic, byte* payload, unsigned int length) {
    String message = "";
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }

    Serial.println("📩 Message reçu : " + message);

    if (message.startsWith("CASIER:")) {
        if (!casierOuvert) {
            String casier = message.substring(7);
            Serial.println("✅ Accès autorisé, ouverture du casier " + casier);
            ouvrirCasier();
            casierOuvert = true; // ouvert
            delay(5000);         // attente pour éviter une redetection immédiate
            casierOuvert = false;
        }
    } else if (message == "REFUS") {
        Serial.println("❌ Accès refusé !");
    }
}


// Lecture d'un badge NFC
String badgeValide() {
    uint8_t success;
    uint8_t uid[7];  
    uint8_t uidLength;

    success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 1000);

    if (!success) {
        return "";  // Aucun badge détecté
    }

    String badgeID = "";
    for (byte i = 0; i < uidLength; i++) {
        badgeID += String(uid[i], HEX);
    }
    badgeID.toUpperCase();

    Serial.print("🔹 Badge détecté : ");
    Serial.println(badgeID);

    return badgeID;
}

// Ouverture du casier via le relais, on l'ouvre pendant 5s
void ouvrirCasier() {
    Serial.println("🔓 Ouverture du casier !");
    digitalWrite(RELAY_PIN, HIGH);
    delay(5000);
    digitalWrite(RELAY_PIN, LOW);
}

// Vérification de la connexion WiFi et MQTT
void verifierConnexion() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("🔴 Wi-Fi déconnecté ! Tentative de reconnexion...");
        WiFi.disconnect();
        WiFi.reconnect();
        delay(5000);
    }

    if (!client.connected()) {
        Serial.println("🔴 MQTT déconnecté ! Tentative de reconnexion...");
        reconnect();
    }
}

void loop() {
    verifierConnexion();

    String badgeID = badgeValide();
    if (badgeID != "" && badgeID != dernierBadge) {
        Serial.println("📤 Envoi de l'UID au serveur MQTT.");
        client.publish(mqtt_topic, badgeID.c_str(), false);
        dernierBadge = badgeID;
    }

    client.loop();
}
