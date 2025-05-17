#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <DHT.h>

// ✅ DHT11 설정
#define DHTPIN 14
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// ✅ BLE 설정
#define bleServerName "DHT11_ESP32"
#define SERVICE_UUID "91bad492-b950-4226-aa2b-4ede9fa42f59"
#define TEMP_CHARACTERISTIC_UUID "cba1d466-344c-4be3-ab3f-189f80dd7518"
#define HUM_CHARACTERISTIC_UUID "ca73b3ba-39f6-4ab3-91ae-186dc9577d99"

BLECharacteristic tempCharacteristic(TEMP_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_NOTIFY);
BLECharacteristic humCharacteristic(HUM_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_NOTIFY);
BLE2902 temp2902;
BLE2902 hum2902;

float temp, hum;
bool deviceConnected = false;

// 타이머
unsigned long lastTime = 0;
unsigned long timerDelay = 10000;  // 10초 간격

// ✅ 콜백 클래스
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) override {
    deviceConnected = true;
    Serial.println("✅ Client Connected!");
  }

  void onDisconnect(BLEServer* pServer) override {
    deviceConnected = false;
    Serial.println("🔌 Client Disconnected!");
  }
};

void setup() {
  Serial.begin(9600);
  Serial.println("🔧 Serial Monitor Activated...");
  dht.begin();
  Serial.println("🌡️ DHT11 Sensor Initialized...");

  // BLE 초기화
  BLEDevice::init(bleServerName);
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *bmeService = pServer->createService(SERVICE_UUID);

  // 온도 특성
  tempCharacteristic.addDescriptor(&temp2902);
  temp2902.setNotifications(true);
  bmeService->addCharacteristic(&tempCharacteristic);

  // 습도 특성
  humCharacteristic.addDescriptor(&hum2902);
  hum2902.setNotifications(true);
  bmeService->addCharacteristic(&humCharacteristic);

  // 서비스 시작
  bmeService->start();

  // 광고 시작
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  pAdvertising->setMinInterval(0x20);
  pAdvertising->setMaxInterval(0x40);
  pAdvertising->start();

  Serial.println("📡 Waiting for a client connection...");
}

void loop() {
  if (deviceConnected && (millis() - lastTime > timerDelay)) {
    temp = dht.readTemperature();
    hum = dht.readHumidity();

    if (isnan(temp) || isnan(hum)) {
      Serial.println("❌ Failed to read from DHT sensor!");
      return;
    }

    // 온도 전송
    char tempStr[10];
    dtostrf(temp, 6, 2, tempStr);
    tempCharacteristic.setValue(tempStr);
    tempCharacteristic.notify();
    Serial.print("🌡️ Temperature: ");
    Serial.print(temp);
    Serial.println(" ºC");

    // 습도 전송
    char humStr[10];
    dtostrf(hum, 6, 2, humStr);
    humCharacteristic.setValue(humStr);
    humCharacteristic.notify();
    Serial.print("💧 Humidity: ");
    Serial.print(hum);
    Serial.println(" %");

    lastTime = millis();
  }
}
