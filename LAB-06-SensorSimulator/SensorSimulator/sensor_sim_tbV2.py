# Update the script to print response codes from on_publish and also rc from publish()

"""
Sensor simulator for ThingsBoard (MQTT) - Paho v2.x + valid JSON
Publishes: {"temp": XX, "humidity": YY}

Adds: prints publish rc and on_publish reason codes.

Usage (Windows PowerShell):
  python sensor_sim_tbV2_fixed_rc.py --host localhost --port 1883 --interval 2 [--with-ts] [--dry-run]

Requirements:
  pip install paho-mqtt
"""

import argparse
import json
import math
import random
import signal
import sys
import time
from datetime import datetime

import paho.mqtt.client as mqtt

# >>> Put your ThingsBoard DEVICE ACCESS TOKEN here (MQTT username) <<<
ACCESS_TOKEN = "4ewGT4aULbexYYxiXGdB"

TOPIC_TELEMETRY = "v1/devices/me/telemetry"


def parse_args():
    p = argparse.ArgumentParser(description="Simulate temp & humidity and publish to ThingsBoard via MQTT.")
    p.add_argument("--host", default="localhost", help="ThingsBoard host/IP (default: localhost)")
    p.add_argument("--port", type=int, default=1883, help="MQTT port (default: 1883)")
    p.add_argument("--interval", type=float, default=2.0, help="Seconds between telemetry messages (default: 2.0)")
    p.add_argument("--client-id", default="", help="Optional MQTT client ID (default: auto)")
    p.add_argument("--qos", type=int, default=1, choices=[0, 1, 2], help="MQTT QoS level (default: 1)")
    p.add_argument("--retain", action="store_true", help="Publish retained messages (default: False)")
    p.add_argument("--min-temp", type=float, default=24.0, help="Min baseline temp (°C)")
    p.add_argument("--max-temp", type=float, default=32.0, help="Max baseline temp (°C)")
    p.add_argument("--min-hum", type=float, default=45.0, help="Min baseline humidity (%)")
    p.add_argument("--max-hum", type=float, default=75.0, help="Max baseline humidity (%)")
    p.add_argument("--noise", type=float, default=0.5, help="Gaussian noise std-dev")
    p.add_argument("--spike-prob", type=float, default=0.02, help="Probability of a random spike per tick")
    p.add_argument("--with-ts", action="store_true", help="Send explicit timestamped telemetry [{ts, values}]")
    p.add_argument("--dry-run", action="store_true", help="Generate values but do not publish")
    return p.parse_args()


class GracefulKiller:
    def __init__(self):
        self.kill_now = False
        signal.signal(signal.SIGINT, self.exit_gracefully)
        signal.signal(signal.SIGTERM, self.exit_gracefully)

    def exit_gracefully(self, *_):
        self.kill_now = True


def make_series(tick, temp_range, hum_range, noise_std, spike_prob):
    """Generate temp & humidity with sinusoidal pattern, noise, and spikes."""
    day_period = 60.0
    temp_center = (temp_range[0] + temp_range[1]) / 2.0
    temp_amp = (temp_range[1] - temp_range[0]) / 2.0
    hum_center = (hum_range[0] + hum_range[1]) / 2.0
    hum_amp = (hum_range[1] - hum_range[0]) / 2.0

    temp = temp_center + temp_amp * math.sin(2 * math.pi * (tick % day_period) / day_period)
    hum = hum_center + hum_amp * math.cos(2 * math.pi * (tick % (day_period * 1.2)) / (day_period * 1.2))

    temp += random.gauss(0, noise_std)
    hum += random.gauss(0, noise_std * 2)

    if random.random() < spike_prob:
        temp += random.choice([-1, 1]) * random.uniform(1.0, 3.0)
    if random.random() < spike_prob:
        hum += random.choice([-1, 1]) * random.uniform(3.0, 8.0)

    temp = max(temp_range[0] - 2, min(temp, temp_range[1] + 2))
    hum = max(0.0, min(hum, 100.0))

    return round(temp, 2), round(hum, 2)


# ----- Paho v2.x callback signatures -----
def on_connect(client, userdata, flags, reason_code, properties):
    print(f"[on_connect] reason_code={reason_code}, session_present={getattr(flags, 'session_present', None)}")


def on_disconnect(client, userdata, flags, reason_code, properties):
    print(f"[on_disconnect] reason_code={reason_code}")


def on_publish(client, userdata, mid, reason_codes, properties):
    # reason_codes: list for MQTT v5; None/[] for MQTT v3.1.1
    if not reason_codes:
        print(f"[on_publish] mid={mid}, reason_codes=None (MQTT 3.1.1)")
    else:
        try:
            rc_text = ",".join(str(rc) for rc in reason_codes)
        except Exception:
            rc_text = str(reason_codes)
        print(f"[on_publish] mid={mid}, reason_codes={rc_text}")


def main():
    args = parse_args()

    client = mqtt.Client(
        mqtt.CallbackAPIVersion.VERSION2,
        client_id=args.client_id or None,
        protocol=mqtt.MQTTv311  # Keep v3.1.1; switch to MQTTv5 if you need per-publish reason codes
    )
    client.username_pw_set(ACCESS_TOKEN, password=None)
    client.on_connect = on_connect
    client.on_disconnect = on_disconnect
    client.on_publish = on_publish

    try:
        client.connect(args.host, args.port, keepalive=60)
    except Exception as e:
        print(f"Connection failed: {e}")
        sys.exit(1)

    client.loop_start()
    killer = GracefulKiller()
    tick = 0
    try:
        while not killer.kill_now:
            temp, hum = make_series(
                tick,
                (args.min_temp, args.max_temp),
                (args.min_hum, args.max_hum),
                args.noise,
                args.spike_prob,
            )
            if args.with_ts:
                ts_ms = int(time.time() * 1000)
                payload = json.dumps([{"ts": ts_ms, "values": {"temp": temp, "humidity": hum}}])
            else:
                payload = json.dumps({"temp": temp, "humidity": hum})

            print(f"[{datetime.now().isoformat(timespec='seconds')}] Publishing: {payload}")
            if not getattr(args, "dry_run", False):
                info = client.publish(TOPIC_TELEMETRY, payload, qos=args.qos, retain=args.retain)
                # rc is the client-side return code for sending the PUBLISH
                print(f"[publish] mid={info.mid}, rc={info.rc}")
                info.wait_for_publish()
            time.sleep(args.interval)
            tick += 1
    finally:
        client.loop_stop()
        client.disconnect()
        print("Disconnected.")


if __name__ == "__main__":
    main()