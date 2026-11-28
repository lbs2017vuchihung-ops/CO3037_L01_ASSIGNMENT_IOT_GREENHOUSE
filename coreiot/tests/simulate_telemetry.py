import os
import json
import time
import random
import threading
import paho.mqtt.client as mqtt

# MQTT configuration
MQTT_HOST = os.getenv("MQTT_HOST", "app.coreiot.io")
MQTT_PORT = int(os.getenv("MQTT_PORT", "1883"))
ACCESS_TOKEN = os.getenv("COREIOT_ACCESS_TOKEN")

# MQTT topics
TELEMETRY_TOPIC = "v1/devices/me/telemetry"
RPC_REQUEST_TOPIC = "v1/devices/me/rpc/request/+"
RPC_RESPONSE_TOPIC = "v1/devices/me/rpc/response/"
ATTRIBUTE_TOPIC = "v1/devices/me/attributes"
ATTRIBUTE_REQUEST_TOPIC = "v1/devices/me/attributes/request/"
ATTRIBUTE_RESPONSE_TOPIC = "v1/devices/me/attributes/response/+"

# Timing configuration
SEND_INTERVAL = 5
PUBLISH_TIMEOUT = 15

connected = threading.Event()

# Device state: pump_state and led_color are derived from pump_level.
device_state = {
    "pump_level": 0,
    "fan_state": False
}

LED_COLORS = {
    0: "GREEN",
    1: "YELLOW",
    2: "RED"
}

packet_id = 0

# Shared attributes configuration
# Automatic control is handled by CoreIOT RuleChain, not the simulator.
device_config = {
    "auto_mode": None,
    "soil_threshold": None,
    "temp_threshold": None
}


def request_shared_attributes(client):
    payload = {
        "sharedKeys": ",".join(device_config.keys())
    }

    client.publish(
        ATTRIBUTE_REQUEST_TOPIC + "1",
        json.dumps(payload),
        qos=1
    )

    print("[CONFIG] Requested shared attributes")


def update_config(attributes):
    if not isinstance(attributes, dict):
        return

    for key, value in attributes.items():
        if key in device_config:
            device_config[key] = value
            print(f"[CONFIG] {key} = {value}")


def on_connect(client, userdata, flags, reason_code, properties):
    if reason_code == 0:
        print("[MQTT] Connected successfully")

        client.subscribe([
            (RPC_REQUEST_TOPIC, 1),
            (ATTRIBUTE_TOPIC, 1),
            (ATTRIBUTE_RESPONSE_TOPIC, 1)
        ])

        connected.set()

    else:
        print(f"[MQTT] Connection failed: {reason_code}")


def on_subscribe(client, userdata, mid, reason_codes, properties):
    if any(code.is_failure for code in reason_codes):
        print("[MQTT] Subscription failed")
        return

    print("[MQTT] Subscribed to RPC requests")
    print("[MQTT] Subscribed to shared attributes")

    request_shared_attributes(client)


def on_disconnect(client, userdata, flags, reason_code, properties):
    connected.clear()
    print(f"[MQTT] Disconnected: {reason_code}")


def handle_rpc(client, topic, request):
    request_id = topic.split("/")[-1]

    method = request.get("method")
    params = request.get("params")

    print(f"[RPC] Received: {method}, params={params}")

    if method == "setPump" and isinstance(params, bool):
        device_state["pump_level"] = 2 if params else 0
        response = device_state["pump_level"] > 0

    elif method == "setPumpLevel" and type(params) is int:
        if params in LED_COLORS:
            device_state["pump_level"] = params
            response = device_state["pump_level"]
        else:
            response = {
                "error": "Invalid pump level"
            }

    elif method == "setFan" and isinstance(params, bool):
        device_state["fan_state"] = params
        response = device_state["fan_state"]

    elif method == "getPump":
        response = device_state["pump_level"] > 0

    elif method == "getFan":
        response = device_state["fan_state"]

    else:
        response = {
            "error": "Invalid RPC request"
        }

    client.publish(
        RPC_RESPONSE_TOPIC + request_id,
        json.dumps(response),
        qos=1
    )

    print(f"[RPC] Response: {response}")


def on_message(client, userdata, message):
    try:
        payload = json.loads(message.payload.decode())
        topic = message.topic

        # Handle shared attributes updates
        if topic == ATTRIBUTE_TOPIC:
            update_config(payload)
            return

        # Handle initial shared attributes response
        if topic.startswith(
            "v1/devices/me/attributes/response/"
        ):
            attributes = payload.get("shared", {})
            update_config(attributes)
            return

        # Handle RPC requests
        if topic.startswith(
            "v1/devices/me/rpc/request/"
        ):
            handle_rpc(client, topic, payload)
            return

    except Exception as e:
        print(f"[MESSAGE ERROR] {e}")


def generate_telemetry():
    global packet_id

    packet_id += 1
    pump_level = device_state["pump_level"]

    # Simulate an ideal ESP-NOW link: rx_count equals packet_id.
    # Sensor values are synthetic and are not agronomic thresholds.
    return {
        "packet_id": packet_id,
        "rx_count": packet_id,
        "temp": round(random.uniform(25, 40), 1),
        "hum_air": round(random.uniform(50, 85), 1),
        "hum_soil": random.randint(25, 70),
        "light": random.randint(0, 100),
        "nitrogen": random.randint(20, 100),
        "phosphorus": random.randint(20, 100),
        "potassium": random.randint(20, 100),
        "pump_level": pump_level,
        "pump_state": pump_level > 0,
        "fan_state": device_state["fan_state"],
        "led_color": LED_COLORS[pump_level]
    }


def main():
    if not ACCESS_TOKEN:
        print("[ERROR] Missing COREIOT_ACCESS_TOKEN")
        return

    client = mqtt.Client(
        mqtt.CallbackAPIVersion.VERSION2
    )

    client.username_pw_set(ACCESS_TOKEN)

    client.reconnect_delay_set(
        min_delay=1,
        max_delay=30
    )

    client.on_connect = on_connect
    client.on_disconnect = on_disconnect
    client.on_subscribe = on_subscribe
    client.on_message = on_message

    try:
        client.connect(MQTT_HOST, MQTT_PORT, 60)
        client.loop_start()

        if not connected.wait(timeout=15):
            print("[ERROR] MQTT connection timeout")
            return

        while True:
            if not connected.is_set():
                time.sleep(1)
                continue

            telemetry = generate_telemetry()
            payload = json.dumps(telemetry)

            result = client.publish(
                TELEMETRY_TOPIC,
                payload,
                qos=1
            )

            if result.rc == mqtt.MQTT_ERR_SUCCESS:
                result.wait_for_publish(
                    timeout=PUBLISH_TIMEOUT
                )

                if result.is_published():
                    print("[SEND]", payload)
                else:
                    print("[ERROR] Publish timeout")

            else:
                print("[ERROR] Publish failed")

            time.sleep(SEND_INTERVAL)

    except KeyboardInterrupt:
        print("\n[MQTT] Stopping simulator")

    except Exception as e:
        print(f"[ERROR] {e}")

    finally:
        client.disconnect()
        client.loop_stop()


if __name__ == "__main__":
    main()
