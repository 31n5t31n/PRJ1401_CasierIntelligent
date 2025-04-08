import paho.mqtt.client as mqtt  # Bibliotheque pour gerer mqtt

MQTT_BROKER = "172.20.10.9" # l'IP de mon broker (mon ordi)
MQTT_TOPIC = "casier/ouverture" # le topic sur lequel je veux publier

# Base de donnees integree
badges_autorises = {
    "40F1ED56" : "2", # Badge NFC
    "44F77FA591490" : "1" # Ma carte UBS
}

def on_connect(client, userdata, flags, rc):
    print("✅ Connecté au broker MQTT !") # Confimrer si on est connecté au broker
    client.subscribe(MQTT_TOPIC) # On s'abonne à casier/ouverture

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
