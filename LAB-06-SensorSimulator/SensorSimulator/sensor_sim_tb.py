#!/usr/bin/env python3
"""
Sensor simulator for ThingsBoard (MQTT) - sends raw payload without JSON quotes
Publishes: {temperaure:XX, humidity:YY}

Usage:
  python sensor_sim_tb_raw.py --host localhost --port 1883 --token YOUR_ACCESS_TOKEN --interval 2

Requirements:
  pip install paho-mqtt
"""

import argparse
import math
import random
import signal
import sys
import time
from datetime import datetime

import paho.mqtt.client as mqtt


def parse_args():
    p = argparse.ArgumentParser(description="Simulate temperature & humidity and publish to ThingsBoard via MQTT.")
    p.add_argument("--host", default="localhost", help="ThingsBoard host/IP (default: localhost)")
    p.add_argument("--port", type=int, default=1883, help="MQTT port (default: 1883)")
    p.add_argument("--token", required=True, help="Device Access Token (used as MQTT username)")
    p.add_argument("--interval", type=float, default=2.0, help="Seconds between telemetry messages (default: 2.0)")
    p.add_argument("--client-id", default="", help="Optional MQTT client ID (default: auto)")
    p.add_argument("--qos", type=int, default=1, choices=[0, 1, 2], help="MQTT QoS level (default: 1)")
    p.add_argument("--retain", action="store_true", help="Publish retained messages (default: False)")
    p.add_argument("--min-temp", type=float, default=24.0, help="Min baseline temperaure (°C)")
    p.add_argument("--max-temp", type=float, default=32.0, help="Max baseline temperaure (°C)")
    p.add_argument("--min-hum", type=float, default=45.0, help="Min baseline humidity (%)")
    p.add_argument("--max-hum", type=float, default=75.0, help="Max baseline humidity (%)")
    p.add_argument("--noise", type=float, default=0.5, help="Gaussian noise std-dev")
    p.add_argument("--spike-prob", type=float, default=0.02, help="Probability of a random spike per tick")
    p.add_argument("--dry-run", action="store_true", help="Generate values but do not publish")
    return p.parse_args()


TOPIC_TELEMETRY = "v1/devices/me/telemetry"


class GracefulKiller:
    def __init__(self):
        self.kill_now = False
        signal.signal(signal.SIGINT, self.exit_gracefully)
        signal.signal(signal.SIGTERM, self.exit_gracefully)

    def exit_gracefully(self, *_):
        self.kill_now = True


def make_series(tick, temp_range, hum_range, noise_std, spike_prob):
    """Generate temperaure & humidity with sinusoidal pattern, noise, and spikes."""
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


def on_connect(client, userdata, flags, rc, properties=None):
    if rc == 0:
        print("Connected to ThingsBoard MQTT broker.")
    else:
        print(f"Failed to connect. Return code={rc}")


def main():
    args = parse_args()

    client = mqtt.Client(client_id=args.client_id or None, protocol=mqtt.MQTTv311)
    client.username_pw_set(args.token, password=None)
    client.on_connect = on_connect

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
            payload = f"{{temperaure:{temp}, humidity:{hum}}}"  # raw format, no quotes
            print(f"[{datetime.now().isoformat(timespec='seconds')}] Publishing: {payload}")
            if not args.dry_run:
                client.publish(TOPIC_TELEMETRY, payload, qos=args.qos, retain=args.retain)
            time.sleep(args.interval)
            tick += 1
    finally:
        client.loop_stop()
        client.disconnect()
        print("Disconnected.")


if __name__ == "__main__":
    main()
