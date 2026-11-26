
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

# Device state
device_state = {
    "pumpState": False,
    "fanState": False
}

# Shared attributes configuration
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
        device_state["pumpState"] = params
        response = device_state["pumpState"]

    elif method == "setFan" and isinstance(params, bool):
        device_state["fanState"] = params
        response = device_state["fanState"]

    elif method == "getPump":
        response = device_state["pumpState"]

    elif method == "getFan":
        response = device_state["fanState"]

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
    return {
        "soilMoisture": round(random.uniform(25, 70), 1),
        "soilTemp": round(random.uniform(24, 32), 1),
        "airTemp": round(random.uniform(25, 40), 1),
        "airHumidity": round(random.uniform(50, 85), 1),
        "pumpState": device_state["pumpState"],
        "fanState": device_state["fanState"]
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
