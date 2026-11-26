# Smart Greenhouse – CoreIOT

**CO3037 – Internet of Things | Học kỳ 261**

Module CoreIOT cung cấp giao diện giám sát nhà kính, điều khiển máy bơm và quạt thông gió theo chế độ thủ công/tự động, cảnh báo môi trường và thông báo trong ứng dụng.

## 1. Phạm vi thực hiện

| Nhiệm vụ | Nội dung | Trạng thái |
|---|---|---|
| T11 | Dashboard giám sát và điều khiển | Đã kiểm thử trên simulator |
| T12 | RuleChain điều khiển tự động theo ngưỡng động | Đã kiểm thử trên simulator |
| T13 | Alarm và Notification | Đã kiểm thử thông báo trong ứng dụng |

## 2. Cấu trúc thư mục

```text
coreiot/
├── README.md
├── dashboard.json
├── rulechain.json
└── tests/
    ├── requirements.txt
    └── simulate_telemetry.py
```

- `dashboard.json`: cấu hình Smart Greenhouse (14 widget).
- `rulechain.json`: cấu hình Greenhouse_RuleChain (16 node, 17 liên kết).
- `tests/`: chương trình giả lập thiết bị MQTT và các thư viện Python cần thiết.

## 3. Kiến trúc hệ thống

```mermaid
flowchart LR
    G[Gateway / Python Simulator] -->|MQTT telemetry| D[Greenhouse_Gateway]
    D --> R[Greenhouse_RuleChain]
    R --> S[Save Timeseries]
    S --> H[Smart Greenhouse Dashboard]
    S --> A[Get Shared Attributes]
    H -->|Cập nhật Shared Attributes| D
    A --> C[Điều khiển tự động]
    A --> L[Create / Clear Alarm]
    C --> P[RPC Call Request]
    H -->|RPC thủ công| P
    P -->|setPump / setFan| G
    L --> H
    L --> N[Notification Center]
```

Logic tự động được thực hiện trên CoreIOT. Gateway nhận lệnh RPC, điều khiển thiết bị và gửi trạng thái thực tế về cloud. Simulator thay thế Gateway trong quá trình kiểm thử độc lập.

## 4. Triển khai trên CoreIOT

### 4.1. Import RuleChain

1. Đăng nhập [CoreIOT](https://app.coreiot.io/) bằng tài khoản có quyền quản trị tenant.
2. Vào **Rule chains**, chọn chức năng Import rule chain và tải `rulechain.json`.
3. Kiểm tra RuleChain có tên `Greenhouse_RuleChain`.
4. Giữ RuleChain này là non-root. Không thay đổi Root RuleChain của tenant.

### 4.2. Tạo Device Profile và Device

1. Vào **Profiles → Device profiles**, tạo `Greenhouse_Profile`.
2. Chọn **Default rule chain** là `Greenhouse_RuleChain` và lưu.
3. Vào **Entities → Devices**, tạo `Greenhouse_Gateway` và chọn profile vừa tạo.
4. Sử dụng thiết bị MQTT thông thường (không bật Is gateway cho cấu hình giả lập này).
5. Trong Device Details hoặc Manage credentials, sao chép Access Token để sử dụng khi kết nối MQTT.

Nếu các đối tượng đã tồn tại, kiểm tra và tái sử dụng thay vì tạo bản sao không cần thiết.

### 4.3. Khởi tạo Shared Attributes

Vào **Entities → Devices → Greenhouse_Gateway → Attributes → Shared attributes**, tạo ba thuộc tính:

| Key | Kiểu dữ liệu | Giá trị khởi tạo đề xuất | Ý nghĩa |
|---|---|---|---|
| `auto_mode` | Boolean | `true` | Bật chế độ tự động |
| `soil_threshold` | Integer | `40` | Ngưỡng độ ẩm đất (%) |
| `temp_threshold` | Integer | `35` | Ngưỡng nhiệt độ không khí (°C) |

Nhập đúng kiểu dữ liệu, không nhập Boolean dưới dạng chuỗi. Khi chạy với phần cứng thật lần đầu, nên chọn `auto_mode = false` cho đến khi xác nhận trạng thái relay và cảm biến an toàn.

Các giá trị trên là cấu hình khởi tạo, không phải ngưỡng hard-code trong RuleChain. Hai file JSON không lưu giá trị Shared Attributes thực tế của Device. Node Get Shared Attributes yêu cầu đủ cả ba key.

### 4.4. Import Dashboard

1. Vào **Dashboards**, chọn Import dashboard và tải `dashboard.json`.
2. Mở `Smart Greenhouse`, vào Edit mode và mở Entity aliases.
3. Chỉnh alias `Greenhouse` thành Single entity → Device → `Greenhouse_Gateway` của tài khoản đang triển khai.
4. Apply và Save Dashboard.

**Lưu ý:** JSON export tham chiếu UUID của Device gốc, nên phải kiểm tra và gán lại alias khi import sang môi trường khác.

### 4.5. Notification

Mở **Notification center → Rules** để kiểm tra cấu hình thông báo cho sự kiện Alarm created và Alarm cleared. Bảo đảm người dùng cần nhận thông báo thuộc nhóm người nhận được cấu hình và kênh Web/in-app được bật. Nếu tài khoản đã có rule phù hợp, tái sử dụng để tránh gửi trùng. Nếu chưa có, tạo rule và template phù hợp với hai loại Alarm ở mục 7.

Cấu hình Notification của tenant không nằm trong hai file JSON. Tại môi trường đã kiểm thử, thông báo tạo và kết thúc Alarm xuất hiện ở biểu tượng chuông. Email/SMS chưa được kiểm thử.

## 5. Quy ước giao tiếp MQTT

### 5.1. Kết nối

| Thông số | Giá trị dùng khi giả lập |
|---|---|
| Broker | `app.coreiot.io` |
| Port | `1883` |
| Username | Access Token của `Greenhouse_Gateway` |
| Password | Không cần |
| QoS | 1 |
| Chu kỳ telemetry | 5 giây |

Không đưa Access Token vào mã nguồn, ảnh minh chứng hoặc GitHub. Port 1883 không mã hóa; chỉ dùng cấu hình này cho môi trường kiểm thử đã thiết lập. Khi triển khai thực tế, ưu tiên kết nối TLS theo cấu hình broker được cung cấp.

### 5.2. Telemetry

Publish topic: `v1/devices/me/telemetry`.

| Key | Kiểu dữ liệu | Đơn vị / ý nghĩa |
|---|---|---|
| `airTemp` | Number | Nhiệt độ không khí (°C) |
| `soilTemp` | Number | Nhiệt độ đất (°C) |
| `airHumidity` | Number | Độ ẩm không khí (%) |
| `soilMoisture` | Number | Độ ẩm đất (%) |
| `pumpState` | Boolean | Trạng thái thực tế máy bơm |
| `fanState` | Boolean | Trạng thái thực tế quạt |

Ví dụ payload:

```json
{
  "airTemp": 32.5,
  "soilTemp": 28.3,
  "airHumidity": 65.8,
  "soilMoisture": 42.6,
  "pumpState": false,
  "fanState": true
}
```

Gateway cần báo đủ các key, đặc biệt là hai trạng thái Boolean để RuleChain không gửi lệnh khi thiết bị đã ở đúng trạng thái mong muốn.

### 5.3. Server-side RPC

| Topic | Hướng |
|---|---|
| `v1/devices/me/rpc/request/+` | Gateway subscribe |
| `v1/devices/me/rpc/response/<requestId>` | Gateway publish phản hồi |

| Method | Params | Chức năng |
|---|---|---|
| `setPump` | Boolean | Đặt trạng thái máy bơm |
| `setFan` | Boolean | Đặt trạng thái quạt |
| `getPump` | Không cần | Đọc trạng thái bơm (simulator hỗ trợ) |
| `getFan` | Không cần | Đọc trạng thái quạt (simulator hỗ trợ) |

Ví dụ yêu cầu bật máy bơm:

```json
{"method": "setPump", "params": true}
```

Gateway phản hồi trạng thái Boolean trên topic response có cùng request ID và báo lại trạng thái thực tế trong telemetry tiếp theo. Không dùng chuỗi `"true"` hoặc `"false"` thay cho Boolean trong RPC params.

### 5.4. Shared Attributes qua MQTT

| Topic | Chức năng |
|---|---|
| `v1/devices/me/attributes` | Subscribe cập nhật thuộc tính |
| `v1/devices/me/attributes/request/1` | Yêu cầu cấu hình ban đầu |
| `v1/devices/me/attributes/response/+` | Subscribe phản hồi cấu hình |

Ví dụ payload yêu cầu cấu hình:

```json
{"sharedKeys": "auto_mode,soil_threshold,temp_threshold"}
```

Simulator đọc và ghi log cấu hình; logic quyết định tự động vẫn thuộc RuleChain.

## 6. Dashboard và điều khiển tự động

### 6.1. Dashboard (T11)

| Thành phần | Số lượng | Chức năng |
|---|---:|---|
| Gauge | 4 | Hiển thị cảm biến với một chữ số thập phân |
| Time-series chart | 2 | Biểu đồ nhiệt độ và độ ẩm, 1 phút gần nhất |
| Single Switch | 3 | Điều khiển bơm, quạt và chế độ tự động |
| Slider | 2 | Điều chỉnh ngưỡng đất (0–100%) và nhiệt độ (0–50°C) |
| Alarms Table | 1 | Lịch sử cảnh báo 24 giờ gần nhất |
| HTML Card | 2 | Tiêu đề và hướng dẫn sử dụng |

Khi `auto_mode = true`, Disabled state của hai Switch bơm/quạt đọc Shared Attribute `auto_mode` để khóa thao tác nhưng vẫn hiển thị trạng thái telemetry. Khi `auto_mode = false`, người dùng có thể điều khiển thủ công.

### 6.2. RuleChain thông minh (T12)

```mermaid
flowchart TD
    I[Input] --> M[Message Type Switch]
    M -->|Post telemetry| S[Save Timeseries]
    M -->|Post attributes| A[Save Client Attributes]
    M -->|RPC Request to Device| R[RPC Call Request]
    S -->|Success| G[Get Shared Attributes]
    G -->|Success| C[Check Auto Mode]
    C -->|True| P[Check Pump State]
    C -->|True| F[Check Fan State]
    P -->|True| BP[Build Pump RPC]
    F -->|True| BF[Build Fan RPC]
    BP -->|Success| R
    BF -->|Success| R
    G -->|Success| T[Check High Temperature]
    G -->|Success| L[Check Low Soil Moisture]
    T -->|True| CT[Create High Temperature Alarm]
    T -->|False| XT[Clear High Temperature Alarm]
    L -->|True| CL[Create Low Soil Moisture Alarm]
    L -->|False| XL[Clear Low Soil Moisture Alarm]
```

| Thiết bị | Điều kiện bật | Điều kiện tắt |
|---|---|---|
| Máy bơm | `soilMoisture < soil_threshold` | `soilMoisture >= soil_threshold` |
| Quạt | `airTemp > temp_threshold` | `airTemp <= temp_threshold` |

Node Get Shared Attributes đưa cấu hình vào metadata dưới dạng chuỗi với tiền tố `shared_`. Script JavaScript chuyển ngưỡng bằng `Number()` và kiểm tra `shared_auto_mode === "true"`. Hai nhánh điều khiển hoạt động độc lập. RPC chỉ được tạo khi trạng thái telemetry khác trạng thái mong muốn.

## 7. Alarm và Notification (T13)

| Alarm type | Severity | Điều kiện tạo | Điều kiện kết thúc |
|---|---|---|---|
| `High Temperature` | Major | `airTemp > temp_threshold` | `airTemp <= temp_threshold` |
| `Low Soil Moisture` | Warning | `soilMoisture < soil_threshold` | `soilMoisture >= soil_threshold` |

Hai nhánh Alarm nhận message từ Get Shared Attributes, độc lập với Check Auto Mode. Vì vậy, cảnh báo vẫn hoạt động trong chế độ thủ công. Alarm Details lưu giá trị cảm biến, ngưỡng và nội dung cảnh báo; Clear Alarm bổ sung thông tin lúc kết thúc.

- Active: điều kiện cảnh báo đang tồn tại.
- Cleared: cảnh báo đã kết thúc, lịch sử vẫn được giữ.
- Acknowledged: người dùng đã xác nhận cảnh báo.
- Unacknowledged: người dùng chưa xác nhận cảnh báo.

Thông báo trong ứng dụng được xử lý bằng Notification Center của CoreIOT, không thêm node gửi thông báo riêng vào RuleChain.

## 8. Chạy simulator

Yêu cầu: Python 3, pip, kết nối Internet và Access Token hợp lệ. Mở terminal tại thư mục gốc repository.

Windows CMD:

```cmd
python -m pip install -r coreiot/tests/requirements.txt
set "COREIOT_ACCESS_TOKEN=YOUR_ACCESS_TOKEN"
python coreiot/tests/simulate_telemetry.py
```

PowerShell:

```powershell
python -m pip install -r coreiot/tests/requirements.txt
$env:COREIOT_ACCESS_TOKEN = "YOUR_ACCESS_TOKEN"
python coreiot/tests/simulate_telemetry.py
```

Thay `YOUR_ACCESS_TOKEN` bằng token thật trong terminal, không lưu vào source. Biến môi trường chỉ có hiệu lực trong phiên terminal tương ứng. Dừng simulator bằng Ctrl+C. Không chạy simulator và Gateway thật đồng thời trên cùng Device vì có thể gây xung đột telemetry và trạng thái.

Ví dụ log mong đợi:

```text
[MQTT] Connected successfully
[CONFIG] auto_mode = True
[CONFIG] soil_threshold = 40
[CONFIG] temp_threshold = 35
[SEND] ...
[RPC] Received: setPump, params=True
[RPC] Response: True
```

## 9. Kiểm thử và kết quả

Simulator tạo độ ẩm đất trong khoảng 25–70% và nhiệt độ không khí trong khoảng 25–40°C. Có thể đặt ngưỡng ngoài các khoảng này để kiểm thử ổn định.

| Thao tác trong chế độ tự động | Kết quả mong đợi |
|---|---|
| Ngưỡng đất 80% | Bơm bật; Low Soil Moisture Active |
| Ngưỡng đất 10% | Bơm tắt; Low Soil Moisture Cleared nếu đang Active |
| Ngưỡng nhiệt độ 18°C | Quạt bật; High Temperature Active |
| Ngưỡng nhiệt độ 45°C | Quạt tắt; High Temperature Cleared nếu đang Active |
| Tắt chế độ tự động | Mở khóa Switch; cảnh báo vẫn được xử lý |

Các bài kiểm thử riêng lẻ về telemetry, RPC, Shared Attributes, Filter, Transformation, điều khiển tự động, khóa Switch, Create/Clear Alarm và Notification trong ứng dụng đã được thực hiện thành công trên simulator theo quá trình phát triển. Đợt kiểm thử tích hợp cuối TC44–TC51 được bỏ qua; chưa xác nhận tích hợp với Gateway và phần cứng thật.

Sau khi kiểm thử giả lập, có thể đưa ngưỡng về 40% và 35°C.

## 10. Xử lý lỗi thường gặp

| Hiện tượng | Cách kiểm tra |
|---|---|
| Missing COREIOT_ACCESS_TOKEN | Khai báo lại biến môi trường trong terminal đang chạy |
| Dashboard không có dữ liệu | Kiểm tra token, MQTT, Device Profile và Entity Alias |
| Debug Events không xuất hiện | Bật All messages, lưu RuleChain, kiểm tra đường nối, Refresh Events |
| Events phản hồi chậm | Đồng bộ thời gian máy tính (Windows Date & time → Sync now), kiểm tra chu kỳ telemetry và kết nối |
| Script không hoạt động | Chọn JavaScript thay vì TBEL; kiểm tra Data/Metadata của Event OUT |
| Publish timeout | Kiểm tra mạng và MQTT PUBACK; timeout không tự chứng minh mất bản tin |
| Không nhận Notification | Kiểm tra Notification Rules, template, người nhận và kênh Web |

Tắt Debug khi không cần kiểm thử để hạn chế ghi log.

## 11. Giới hạn và lưu ý tích hợp

- Khóa điều khiển hiện tại nằm ở Dashboard, chưa chặn RPC thủ công từ mọi nguồn bên ngoài. Disabled state mặc định là false nếu chưa lấy được cấu hình.
- RuleChain chưa áp dụng hysteresis hoặc thời gian giữ trạng thái tối thiểu; cảm biến dao động quanh ngưỡng có thể khiến thiết bị bật/tắt thường xuyên. Cần đánh giá trước khi điều khiển phần cứng thật.
- Simulator lưu trạng thái thiết bị trong RAM; khởi động lại có thể làm trạng thái về false. Gateway thật phải báo trạng thái thực tế.
- Cần kiểm tra khả năng tương thích dữ liệu MQTT, điều khiển relay và phản hồi RPC khi tích hợp với Gateway thật.
- Device, Device Profile, Shared Attributes và Notification Rules phải được thiết lập riêng khi tái triển khai.

