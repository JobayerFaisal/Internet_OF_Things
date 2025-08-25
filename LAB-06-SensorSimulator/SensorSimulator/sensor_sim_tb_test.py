# Requires: pip install paho-mqtt
# Works with Paho MQTT v2.x callback API and sends valid JSON to ThingsBoard.

import json
import time
from paho.mqtt.client import Client, CallbackAPIVersion, MQTTv311

ACCESS_TOKEN = "4ewGT4aULbexYYxiXGdB"   # ThingsBoard device token (MQTT username)
HOST = "localhost"                   # Your TB host/IP
PORT = 1883
TOPIC = "v1/devices/me/telemetry"    # TB telemetry topic

# ----- Paho v2.x callback signatures -----
def on_connect(client, userdata, flags, reason_code, properties):
    # flags is an object; flags.session_present indicates session state
    print(f"[on_connect] reason_code={reason_code}, session_present={getattr(flags, 'session_present', None)}")

def on_disconnect(client, userdata, flags, reason_code, properties):
    print(f"[on_disconnect] reason_code={reason_code}, flags={flags}")

def on_publish(client, userdata, mid, reason_codes, properties):
    # reason_codes is a list (empty for MQTT v3) under API v2
    pass

def main():
    client = Client(
        CallbackAPIVersion.VERSION2,
        client_id="python_tb_pub",
        protocol=MQTTv311
    )
    client.username_pw_set(ACCESS_TOKEN)  # username = device token; password empty
    client.on_connect = on_connect
    client.on_disconnect = on_disconnect
    client.on_publish = on_publish
    # Optional: enable internal client logging to stdout
    # client.enable_logger()

    client.connect(HOST, PORT, keepalive=60)
    client.loop_start()

    try:
        while True:
            payload = {"temperaure": 25.0, "humidity": 60.0}  # valid JSON, key name per request
            msg = json.dumps(payload)
            info = client.publish(TOPIC, msg, qos=1, retain=False)
            info.wait_for_publish()
            print("Published:", msg)
            time.sleep(2)
    except KeyboardInterrupt:
        pass
    finally:
        client.loop_stop()
        client.disconnect()

if __name__ == "__main__":
    main()