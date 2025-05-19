#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// ✅ UUID 설정
#define SERVICE_UUID "91bad492-b950-4226-aa2b-4ede9fa42f59"
#define TEMP_CHARACTERISTIC_UUID "cba1d466-344c-4be3-ab3f-189f80dd7518"
#define HUM_CHARACTERISTIC_UUID "ca73b3ba-39f6-4ab3-91ae-186dc9577d99"

// ✅ BLE Server Name
#define bleServerName "DHT11_ESP32"

static BLEAddress *pServerAddress;
static bool doConnect = false;
static bool connected = false;
static BLERemoteCharacteristic* tempCharacteristic;
static BLERemoteCharacteristic* humCharacteristic;

// ✅ 문자열 안전 변환 함수
String parseDataToString(uint8_t* data, size_t length) {
  String result = "";
  for (size_t i = 0; i < length; i++) {
    result += (char)data[i];
  }
  return result;
}

// ✅ 콜백 설정
static void temperatureNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  Serial.print("🌡️ Temperature: ");
  Serial.println(parseDataToString(pData, length));
}

static void humidityNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  Serial.print("💧 Humidity: ");
  Serial.println(parseDataToString(pData, length));
}

// ✅ 광고 탐색 시 콜백
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.getName() == bleServerName) {
      Serial.println("✔️ Found DHT11_ESP32! Connecting...");
      pServerAddress = new BLEAddress(advertisedDevice.getAddress());
      doConnect = true;
      advertisedDevice.getScan()->stop();
    }
  }
};

void setup() {
  Serial.begin(9600);
  Serial.println("Starting BLE Client...");

  // ✅ BLE 초기화
  BLEDevice::init("");

  // ✅ BLE 스캔 시작
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->start(30);  // 30초 동안 스캔
}

void loop() {
  if (doConnect) {
    BLEClient* pClient = BLEDevice::createClient();
    Serial.println("🔗 Connecting to server...");
    pClient->connect(*pServerAddress);

    // ✅ Service 연결
    BLERemoteService* pRemoteService = pClient->getService(SERVICE_UUID);
    if (pRemoteService == nullptr) {
      Serial.println("❌ Failed to find our service UUID.");
      pClient->disconnect();
      return;
    }

    // ✅ Characteristics 연결
    tempCharacteristic = pRemoteService->getCharacteristic(TEMP_CHARACTERISTIC_UUID);
    humCharacteristic = pRemoteService->getCharacteristic(HUM_CHARACTERISTIC_UUID);

    if (tempCharacteristic == nullptr || humCharacteristic == nullptr) {
      Serial.println("❌ Failed to find our characteristics UUID.");
      pClient->disconnect();
      return;
    }

    Serial.println("✔️ Successfully connected to the server!");

    // ✅ Notify 활성화
    tempCharacteristic->registerForNotify(temperatureNotifyCallback);
    humCharacteristic->registerForNotify(humidityNotifyCallback);

    connected = true;
    doConnect = false;
  }

  delay(1000); // 1초마다 체크
}
