import json
import time
import threading
from pathlib import Path
import yaml
import serial
import paho.mqtt.client as mqtt


def load_config() -> dict:
    base_dir = Path(__file__).resolve().parent
    config_path = base_dir / "config.yaml"
    if not config_path.exists():
        raise FileNotFoundError(
            "Không tìm thấy gateway/config.yaml! Hãy tạo file này và điền cổng COM + Access Token."
        )
    with open(config_path, "r", encoding="utf-8") as f:
        return yaml.safe_load(f)


class FarmLabGateway:
    def __init__(self, cfg: dict):
        self.cfg = cfg
        self.ser = None
        self.serial_lock = threading.Lock()
        self.running = True

        self.mqtt_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
        self.mqtt_client.username_pw_set(cfg["coreiot"]["access_token"])
        self.mqtt_client.on_connect = self._on_mqtt_connect
        self.mqtt_client.on_message = self._on_mqtt_message
        self.mqtt_client.on_disconnect = self._on_mqtt_disconnect

    def _on_mqtt_connect(self, client, userdata, flags, reason_code, properties):
        if reason_code == 0:
            print(
                f"[CoreIOT] Đã kết nối thành công tới {self.cfg['coreiot']['host']}")
            rpc_topic = self.cfg["coreiot"]["rpc_request_topic"]
            client.subscribe(rpc_topic)
            print(f"[CoreIOT] Đang lắng nghe lệnh RPC: {rpc_topic}")
        else:
            print(f"[CoreIOT] Kết nối thất bại, mã lỗi: {reason_code}")

    def _on_mqtt_disconnect(self, client, userdata, flags, reason_code, properties):
        if self.running:
            print(
                f"[CoreIOT] Mất kết nối MQTT (code={reason_code}). Đang tự động kết nối lại...")

    def _on_mqtt_message(self, client, userdata, msg):
        """Nhận lệnh RPC từ CoreIOT -> Đóng gói JSON gửi ngược xuống Serial cho Node Center."""
        try:
            payload = json.loads(msg.payload.decode("utf-8"))
            method = payload.get("method", "")
            params = payload.get("params")
            print(
                f"\n[CoreIOT RPC -> Gateway] Method: {method} | Params: {params}")

            downlink_cmd = None
            if method == "setPumpLevel":
                level = max(0, min(2, int(params)))
                downlink_cmd = {"pump_level": level, "auto": False}
            elif method == "setPump":
                is_on = bool(params)
                downlink_cmd = {"pump_level": 2 if is_on else 0, "auto": False}
            elif method == "setAutoMode":
                downlink_cmd = {"auto": bool(params)}

            if downlink_cmd is not None:
                self.send_command_to_center(downlink_cmd)

                req_id = msg.topic.split("/")[-1]
                resp_topic = f"v1/devices/me/rpc/response/{req_id}"
                client.publish(resp_topic, json.dumps(
                    {"status": "OK", **downlink_cmd}))

        except Exception as e:
            print(f"[Lỗi xử lý RPC] {e}")

    def send_command_to_center(self, cmd_dict: dict):
        """Gửi chuỗi JSON lệnh điều khiển xuống YoloUNO Center qua cổng Serial."""
        raw_str = json.dumps(cmd_dict) + "\n"
        with self.serial_lock:
            if self.ser and self.ser.is_open:
                self.ser.write(raw_str.encode("utf-8"))
                self.ser.flush()
                print(
                    f"[Gateway -> Serial] Đã gửi lệnh xuống Center: {raw_str.strip()}")
            else:
                print("[Gateway -> Serial] Cổng COM chưa mở, không gửi được lệnh!")

    def connect_serial(self):
        port = self.cfg["serial"]["port"]
        baud = int(self.cfg["serial"].get("baudrate", 115200))
        timeout = float(self.cfg["serial"].get("timeout", 1.0))

        while self.running:
            try:
                self.ser = serial.Serial(port, baud, timeout=timeout)
                time.sleep(1.5)
                print(f"[Serial] Đã mở cổng {port} @ {baud} bps")
                return
            except serial.SerialException as e:
                print(f"[Serial] Chưa mở được {port} ({e}). Thử lại sau 3s...")
                time.sleep(3)

    def start(self):
        self.mqtt_client.connect(
            self.cfg["coreiot"]["host"],
            int(self.cfg["coreiot"].get("port", 1883)),
            keepalive=60
        )
        self.mqtt_client.loop_start()

        self.connect_serial()
        telemetry_topic = self.cfg["coreiot"]["telemetry_topic"]

        print("[Gateway] Đang chạy... Nhấn Ctrl+C để dừng.\n")
        try:
            while self.running:
                try:
                    if self.ser and self.ser.in_waiting > 0:
                        line = self.ser.readline().decode("utf-8", errors="ignore").strip()
                        if not line:
                            continue

                        if line.startswith("{") and line.endswith("}"):
                            data = json.loads(line)
                            payload_str = json.dumps(data)
                            info = self.mqtt_client.publish(
                                telemetry_topic, payload_str, qos=1)
                            if info.rc == mqtt.MQTT_ERR_SUCCESS:
                                print(
                                    f"[Serial -> CoreIOT] PKT #{data.get('packet_id')}: {payload_str}")
                        else:
                            print(f"[Center Log] {line}")
                    else:
                        time.sleep(0.02)

                except (serial.SerialException, OSError):
                    print("[Serial] Mất kết nối cổng COM! Đang kết nối lại...")
                    self.connect_serial()
                except json.JSONDecodeError:
                    pass

        except KeyboardInterrupt:
            print("\n[Gateway] Đang dừng chương trình...")
        finally:
            self.running = False
            self.mqtt_client.loop_stop()
            self.mqtt_client.disconnect()
            if self.ser and self.ser.is_open:
                self.ser.close()


if __name__ == "__main__":
    config = load_config()
    gateway = FarmLabGateway(config)
    gateway.start()
