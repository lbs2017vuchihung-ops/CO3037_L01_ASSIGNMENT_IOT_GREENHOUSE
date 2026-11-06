# Hệ thống Giám sát và Điều khiển Khí hậu Nhà kính (Greenhouse Climate Monitoring & Control System)

![HCMUT](https://img.shields.io/badge/HCMUT-Đại_học_Bách_Khoa-0055A6?style=flat-square)
![Course](https://img.shields.io/badge/Course-CO3037_IoT-success?style=flat-square)
![PlatformIO](https://img.shields.io/badge/Firmware-PlatformIO_%7C_ESP32--S3-orange?style=flat-square)
![Python](https://img.shields.io/badge/Gateway-Python_3.10+-blue?style=flat-square)
![Docker](https://img.shields.io/badge/Local_Server-Docker_%7C_ThingsBoard-2496ED?style=flat-square)

Dự án Bài tập lớn môn học **Internet of Things (IoT)** – Học kỳ 261 (Năm học 2025–2026)  
**Khoa Khoa học và Kỹ thuật Máy tính – Trường Đại học Bách Khoa, ĐHQG-HCM**  
**Giảng viên hướng dẫn:** TS. Lê Trọng Nhân

---

## 1. Tổng quan hệ thống
Hệ thống giám sát vi khí hậu và tự động hóa tưới tiêu, thông gió dành cho mô hình nhà kính nông nghiệp công nghệ cao. Hệ thống kết hợp kiến trúc truyền thông không dây cục bộ độ trễ thấp (**ESP-NOW**), giao thức cảm biến công nghiệp (**Modbus RS485**), **IoT Gateway** tích hợp nhận diện sâu bệnh bằng **AI (YOLO/OpenCV)** và cơ chế đồng bộ song song lên nền tảng đám mây **CoreIOT** cùng máy chủ **ThingsBoard cục bộ (Docker)**.

### Các yếu tố công nghệ hiện thực
| Mã CN | Yếu tố công nghệ | Thành phần đảm nhiệm | Điểm đăng ký |
| :---: | :--- | :--- | :---: |
| **T02** | Cảm biến độ ẩm đất giao tiếp công nghiệp **Modbus RS485** | `node_sensor` (ESP32) | 1.0 |
| **T06** | Giao tiếp không dây nội bộ sử dụng giao thức **ESP-NOW** | `node_sensor` $\leftrightarrow$ `node_center` | 1.5 |
| **T07** | **Webserver nhúng** trên thiết bị (hiển thị & điều khiển qua LAN) | `node_center` (YoloUNO ESP32-S3) | 1.0 |
| **T08** | **IoT Gateway trên PC**: tổng hợp dữ liệu và phân phối lệnh điều khiển | PC Gateway (`gateway/main.py`) | 1.0 |
| **T09** | **AI trên IoT Gateway**: nhận diện sâu bệnh lá cây và gửi cảnh báo | PC Gateway (`ai_detector.py`) | 1.0 |
| **T10** | **Giao diện giám sát – điều khiển (UI)** trực quan trên IoT Gateway | PC Gateway (`dashboard_ui.py`) | 1.0 |
| **T11** | Thiết kế **Dashboard, Device Profile & Template** trên CoreIOT | Nền tảng CoreIOT | 1.0 |
| **T12** | Tự động hóa kịch bản tưới tiêu & thông gió bằng **RuleChain** | Nền tảng CoreIOT | 1.0 |
| **T13** | Gửi cảnh báo **Notification** khi chỉ số vi khí hậu vượt ngưỡng | Nền tảng CoreIOT | 0.5 |
| **N07** | Tự triển khai (**Deploy**) máy chủ **ThingsBoard Local** qua Docker | PC Gateway (`docker/`) | 1.0 (Cộng) |

---

## 2. Cấu trúc thư mục mã nguồn (Mục 6.1 Báo cáo)

```text
CO3037_L01_ASSIGNMENT_IOT_GREENHOUSE/
├── ai/                         # Dữ liệu, Jupyter Notebook huấn luyện và trọng số mô hình AI (T09)
│   ├── dataset/                # Tập dữ liệu ảnh lá cây (khỏe mạnh / sâu bệnh)
│   ├── models/                 # File trọng số mô hình đã export (.pt / .onnx)
│   └── train_model.ipynb       # Notebook huấn luyện và đánh giá ma trận nhầm lẫn
├── coreiot/                    # File cấu hình nền tảng CoreIOT (T11, T12, T13)
│   ├── dashboard.json          # File export giao diện Dashboard giám sát từ xa
│   └── rulechain.json          # File export kịch bản tự động hóa RuleChain
├── docker/                     # Triển khai máy chủ dự phòng cục bộ (N07)
│   ├── docker-compose.yml      # Cấu hình khởi chạy ThingsBoard CE & PostgreSQL
│   └── .env.example            # Biến môi trường mẫu cho Docker container
├── docs/                       # Tài liệu, sơ đồ khối kiến trúc (Chương 2) và hình ảnh minh chứng
├── firmware/                   # Mã nguồn vi điều khiển viết trên PlatformIO (C/C++)
│   ├── node_center/            # Node Trung tâm & Chấp hành (Mạch ESP32-S3 YoloUNO)
│   │   ├── include/            # Chứa config.example.h và giao diện web_ui.h (T07)
│   │   ├── src/main.cpp        # Xử lý ESP-NOW, Webserver, đóng cắt Relay và giao tiếp Gateway
│   │   └── platformio.ini      # Cấu hình môi trường biên dịch cho YoloUNO
│   └── node_sensor/            # Node Cảm biến tại luống cây (Mạch ESP32 phụ)
│       ├── include/            # Chứa config.example.h (Cấu hình địa chỉ MAC, chân RS485)
│       ├── src/main.cpp        # Đọc cảm biến Modbus RS485 (T02) và truyền gói tin ESP-NOW (T06)
│       └── platformio.ini      # Cấu hình môi trường biên dịch cho ESP32 Sensor
├── gateway/                    # Mã nguồn IoT Gateway chạy trên PC (Python)
│   ├── services/               # Các module xử lý: Serial/TCP, MQTT Client (T08), AI Detector (T09)
│   ├── ui/                     # Giao diện người dùng Desktop tại trạm quản lý (T10)
│   ├── config.example.yaml     # Cấu hình mẫu (Cổng COM, MQTT Broker, Access Token)
│   ├── main.py                 # Kịch bản thực thi chính của IoT Gateway
│   └── requirements.txt        # Danh sách thư viện Python cần cài đặt
└── README.md                   # Tài liệu hướng dẫn cài đặt và vận hành hệ thống