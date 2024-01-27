import paho.mqtt.client as mqtt
import pymongo
from datetime import datetime

# MongoDB configuration
mongo_client = pymongo.MongoClient("mongodb://localhost:27017/")
db_name = "smart_boat_data"
collection_name = "telemetry_data"
db = mongo_client[db_name]
collection = db[collection_name]

# MQTT configuration
mqtt_broker_address = '34.133.5.85'
mqtt_topic_prefix = 'smart-boat'

def on_message(client, userdata, message):
    payload = message.payload.decode('utf-8')
    print(f'Received message on topic {message.topic}: {payload}')

    # Convert MQTT timestamp to datetime
    timestamp = datetime.utcnow()  # Use current UTC time
    datetime_obj = timestamp.strftime("%Y-%m-%dT%H:%M:%S.%fZ")

    # Process the payload and insert into MongoDB with proper timestamp
    document = {
        'timestamp': datetime_obj,
        'topic': message.topic,
        'data': payload
    }
    collection.insert_one(document)
    print('Data ingested into MongoDB')

client = mqtt.Client()
client.on_message = on_message

# Connect to MQTT broker
client.connect(mqtt_broker_address, 1883, 60)

# Subscribe to MQTT topics
topics = ['infrared', 'dht11', 'rain']  # Add more topics as needed
for topic in topics:
    full_topic = f'{mqtt_topic_prefix}/{topic}'
    client.subscribe(full_topic)

# Start the MQTT loop
client.loop_forever()
