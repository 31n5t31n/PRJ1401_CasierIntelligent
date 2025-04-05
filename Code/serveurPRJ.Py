import paho.mqtt.client as mqtt

MQTT_BROKER = "172.20.10.9"
MQTT_TOPIC = "casier/ouverture"

# Liste des badges autorisés
badges_autorises = {
    "A1B2C3D4": "3",   # Badge 1 ouvre le casier 1
    "12345678": "2",   # Badge 2 ouvre le casier 2
    "40F1ED56" : "4", 
    "44F77FA591490" : "1"
}

def on_connect(client, userdata, flags, rc):
    print("✅ Connecté au broker MQTT !")
    client.subscribe(MQTT_TOPIC)

def on_message(client, userdata, msg):
    badge_recu = msg.payload.decode()
    print(f"📩 Badge reçu : {badge_recu}")

    if badge_recu in badges_autorises:
        casier = badges_autorises[badge_recu]
        print(f"✅ Badge autorisé. Ouverture du casier {casier}")
        client.publish(MQTT_TOPIC, f"CASIER:{casier}")
    elif "CASIER" not in badge_recu:  # S'assure que ce n'est pas une réponse en chaîne
        print("❌ Accès refusé !")
        client.publish(MQTT_TOPIC, "REFUS")



client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

client.connect(MQTT_BROKER, 1883, 60)
client.loop_forever()
