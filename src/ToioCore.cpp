/* ----------------------------------------------------------------
  ToioCore.cpp

  Copyright (c) 2020 Futomi Hatano. All right reserved.
  https://github.com/futomi

  Licensed under the MIT license.
  See LICENSE file in the project root for full license information.
  -------------------------------------------------------------- */
#include "ToioCore.h"

// ===============================================================
// ToioCore クラス
// ===============================================================

static bool g_event_connection_updated = false;
static BLEClient* g_current_client = nullptr;
static BLEClient* g_last_client = nullptr;

static bool g_event_battery_updated = false;
static uint8_t g_event_battery_level = 0;

static bool g_event_button_updated = false;
static bool g_event_button_state = false;

static bool g_event_motion_updated = false;
static ToioCoreMotionData g_event_motion_data = {0x00, 0x00, 0x00, 0x00};

static bool g_event_id_data_updated = false;
static ToioCoreIDData g_event_id_data;

// BLE 接続状態変化のコールバック
class ToioClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient* client) {
      g_current_client = client;
      g_last_client = client;
      g_event_connection_updated = true;
    }
    void onDisconnect(BLEClient* client) {
      g_current_client = nullptr;
      g_event_connection_updated = true;
    }
};

// ---------------------------------------------------------------
// コンストラクタ
// ---------------------------------------------------------------
ToioCore::ToioCore(BLEAdvertisedDevice& device) {
  this->_device = new BLEAdvertisedDevice(device);
  BLEClient* client = BLEDevice::createClient();
  this->_client = client;

  this->_onconnection = nullptr;
  this->_onbutton = nullptr;
  this->_onbattery = nullptr;
  this->_onmotion = nullptr;
  this->_on_id_reader = nullptr;

  client->setClientCallbacks(new ToioClientCallback());
}

// ---------------------------------------------------------------
// デストラクタ
// ---------------------------------------------------------------
ToioCore::~ToioCore() {
  delete this->_device;
}

// ---------------------------------------------------------------
// アドレスを取得
// ---------------------------------------------------------------
std::string ToioCore::getAddress() {
  std::string str = this->_device->getAddress().toString();
  return std::string(str);
}

// ---------------------------------------------------------------
// デバイス名を取得
// ---------------------------------------------------------------
std::string ToioCore::getName() {
  std::string str = this->_device->getName();
  return std::string(str);
}

// ---------------------------------------------------------------
// 接続
// ---------------------------------------------------------------
bool ToioCore::connect() {
  if (this->isConnected()) {
    return true;
  }

  // 接続
  bool connected = this->_client->connect(this->_device);
  if (!connected) {
    return false;
  }

  g_event_battery_updated = false;
  g_event_battery_level = 0;

  g_event_button_updated = false;
  g_event_button_state = false;

  // Service を取得
  BLERemoteService* service = this->_client->getService(this->_TOIO_SERVICE_UUID);
  if (service == nullptr) {
    Serial.print("Failed to find the service: UUID=" + String(this->_TOIO_SERVICE_UUID));
    this->disconnect();
    return false;
  }

  // バッテリーの Characteristic を取得
  this->_char_battery = service->getCharacteristic(this->_TOIO_CHAR_UUID_BATT);
  if (this->_char_battery == nullptr) {
    Serial.print("Failed to find the characteristic for the battery: UUID=" + String(this->_TOIO_CHAR_UUID_BATT));
    this->disconnect();
    return false;
  }

  // ライト (LED) の Characteristic を取得
  this->_char_light = service->getCharacteristic(this->_TOIO_CHAR_UUID_LIGHT);
  if (this->_char_light == nullptr) {
    Serial.print("Failed to find the characteristic for the light: UUID=" + String(this->_TOIO_CHAR_UUID_LIGHT));
    this->disconnect();
    return false;
  }

  // サウンドの Characteristic を取得
  this->_char_sound = service->getCharacteristic(this->_TOIO_CHAR_UUID_SOUND);
  if (this->_char_sound == nullptr) {
    Serial.print("Failed to find the characteristic for the sound: UUID=" + String(this->_TOIO_CHAR_UUID_SOUND));
    this->disconnect();
    return false;
  }

  // ボタンの Characteristic を取得
  this->_char_button = service->getCharacteristic(this->_TOIO_CHAR_UUID_BUTTON);
  if (this->_char_button == nullptr) {
    Serial.print("Failed to find the characteristic for the button: UUID=" + String(this->_TOIO_CHAR_UUID_BUTTON));
    this->disconnect();
    return false;
  }

  // モーションセンサーの Characteristic を取得
  this->_char_motion = service->getCharacteristic(this->_TOIO_CHAR_UUID_MOTION);
  if (this->_char_motion == nullptr) {
    Serial.print("Failed to find the characteristic for the motion sensor: UUID=" + String(this->_TOIO_CHAR_UUID_MOTION));
    this->disconnect();
    return false;
  }

  // 設定の Characteristic を取得
  this->_char_conf = service->getCharacteristic(this->_TOIO_CHAR_UUID_CONF);
  if (this->_char_conf == nullptr) {
    Serial.print("Failed to find the characteristic for the configuration: UUID=" + String(this->_TOIO_CHAR_UUID_CONF));
    this->disconnect();
    return false;
  }

  // モーターの Characteristic を取得
  this->_char_motor = service->getCharacteristic(this->_TOIO_CHAR_UUID_MOTOR);
  if (this->_char_motor == nullptr) {
    Serial.print("Failed to find the characteristic for the motors: UUID=" + String(this->_TOIO_CHAR_UUID_MOTOR));
    this->disconnect();
    return false;
  }

  // IDリーダーの Characteristic を取得
  this->_char_id_reader = service->getCharacteristic(this->_TOIO_CHAR_UUID_ID_READER);
  if (this->_char_id_reader == nullptr) {
    Serial.print("Failed to find the characteristic for the id reader: UUID=" + String(this->_TOIO_CHAR_UUID_ID_READER));
    this->disconnect();
    return false;
  }

  // バッテリーイベントのコールバックをセット
  this->_char_battery->registerForNotify([](BLERemoteCharacteristic * rchar, uint8_t* data, size_t len, bool is_notify) {
    if (!g_current_client) {
      return;
    }
    if (len != 1) {
      return;
    }
    uint8_t level = data[0];
    g_event_battery_level = level;
    g_event_battery_updated = true;
  });

  // ボタンイベントのコールバックをセット
  this->_char_button->registerForNotify([](BLERemoteCharacteristic * rchar, uint8_t* data, size_t len, bool is_notify) {
    if (!g_current_client) {
      return;
    }
    if (len != 2) {
      return;
    }
    if (data[0] != 0x01) {
      return;
    }
    bool state = (data[1] == 0x80) ? true : false;
    g_event_button_state = state;
    g_event_button_updated = true;
  });


  // モーションセンサーイベントのコールバックをセット
  this->_char_motion->registerForNotify([](BLERemoteCharacteristic * rchar, uint8_t* data, size_t len, bool is_notify) {
    if (!g_current_client) {
      return;
    }
    if (len != 6) {
      return;
    }
    g_event_motion_data.flat = data[1];
    g_event_motion_data.clash = data[2];
    g_event_motion_data.dtap = data[3];
    g_event_motion_data.attitude = data[4];
    g_event_motion_data.shake = data[5];
    g_event_motion_updated = true;
  });

  // IDリーダーイベントのコールバックをセット
  this->_char_id_reader->registerForNotify([](BLERemoteCharacteristic * rchar, uint8_t* data, size_t len, bool is_notify) {
    if (!g_current_client) {
      return;
    }
    if (len > 0 && data[0] == 0x03 || data[0] == 0x04) {
      // no data
      g_event_id_data.type = ToioCoreIDTypeNone;
      g_event_id_data_updated = true;
      return;
    }

    if (!ToioCore::_convertBLEBytesToIDData(data, len, g_event_id_data)) {
      // wrong data
      return;
    }
    g_event_id_data_updated = true;
  });

  // 1000 ミリ秒待つ
  this->_wait(1000);
  return true;
}

// ---------------------------------------------------------------
// 切断
// ---------------------------------------------------------------
void ToioCore::disconnect() {
  this->_client->disconnect();
}

// ---------------------------------------------------------------
// 接続状態を返す
// ---------------------------------------------------------------
bool ToioCore::isConnected() {
  return (g_current_client && g_current_client == this->_client) ? true : false;
}

// ---------------------------------------------------------------
// 接続状態イベントのコールバックをセット
// ---------------------------------------------------------------
void ToioCore::onConnection(OnConnectionCallback cb) {
  this->_onconnection = cb;
}

// ---------------------------------------------------------------
// サウンド再生開始 (生データ指定)
// ---------------------------------------------------------------
void ToioCore::playSoundRaw(uint8_t* data, size_t length) {
  if (!this->isConnected()) {
    return;
  }
  this->_char_sound->writeValue(data, length, true);
}

// ---------------------------------------------------------------
// サウンド再生開始 (効果音)
// ---------------------------------------------------------------
void ToioCore::playSoundEffect(uint8_t sound_id, uint8_t volume) {
  if (!this->isConnected()) {
    return;
  }
  uint8_t data[3] = {0x02, sound_id, volume};
  this->_char_sound->writeValue(data, 3, true);
}

// ---------------------------------------------------------------
// サウンド再生停止
// ---------------------------------------------------------------
void ToioCore::stopSound() {
  if (!this->isConnected()) {
    return;
  }
  uint8_t data[1] = {0x01};
  this->_char_sound->writeValue(data, 1, true);
}

// ---------------------------------------------------------------
// LED 点灯
// ---------------------------------------------------------------
void ToioCore::turnOnLed(uint8_t r, uint8_t g, uint8_t b) {
  if (!this->isConnected()) {
    return;
  }
  uint8_t data[7] = {
    0x03, // 制御の種類 (点灯・消灯)
    0x00, // ランプを制御する時間 (ミリ秒)
    0x01, // 制御するランプの数 (0x01 固定)
    0x01, // 制御するランプの ID (0x01 固定)
    r,    // ランプの Red の値
    g,    // ランプの Green の値
    b     // ランプの Blue の値
  };
  this->_char_light->writeValue(data, 7);
}

// ---------------------------------------------------------------
// LED 消灯
// ---------------------------------------------------------------
void ToioCore::turnOffLed() {
  this->turnOnLed(0x00, 0x00, 0x00);
}

// ---------------------------------------------------------------
// バッテリーレベルを取得
// ---------------------------------------------------------------
uint8_t ToioCore::getBatteryLevel() {
  if (!this->isConnected()) {
    return 0;
  }
  std::string data = this->_char_battery->readValue();
  if (data.size() != 1) {
    return 0;
  }
  uint8_t level = data[0];
  return level;
}

// ---------------------------------------------------------------
// バッテリーイベントのコールバックをセット
// ---------------------------------------------------------------
void ToioCore::onBattery(OnBatteryCallback cb) {
  this->_onbattery = cb;
}

// ---------------------------------------------------------------
// ボタンの状態を取得
// ---------------------------------------------------------------
bool ToioCore::getButtonState() {
  if (!this->isConnected()) {
    return false;
  }
  std::string data = this->_char_button->readValue();
  if (data.size() != 2) {
    return false;
  }
  bool state = (data[1] == 0x80) ? true : false;
  return state;
}

// ---------------------------------------------------------------
// ボタンイベントのコールバックをセット
// ---------------------------------------------------------------
void ToioCore::onButton(OnButtonCallback cb) {
  this->_onbutton = cb;
}

// ---------------------------------------------------------------
// モーションセンサーの状態を取得
// ---------------------------------------------------------------
ToioCoreMotionData ToioCore::getMotion() {
  ToioCoreMotionData res = {0x00, 0x00, 0x00 , 0x00, 0x00};
  if (!this->isConnected()) {
    return res;
  }
  std::string data = this->_char_motion->readValue();
  if (data.size() != 6) {
    return res;
  }
  res.flat = data[1];
  res.clash = data[2];
  res.dtap = data[3];
  res.attitude = data[4];
  res.shake = data[5];
  return res;
}

// ---------------------------------------------------------------
// モーションセンサーのコールバックをセット
// ---------------------------------------------------------------
void ToioCore::onMotion(OnMotionCallback cb) {
  this->_onmotion = cb;
}

// ---------------------------------------------------------------
// BLE プロトコルバージョン取得
// ---------------------------------------------------------------
std::string ToioCore::getBleProtocolVersion() {
  std::string empty_data = "";
  if (!this->isConnected()) {
    return empty_data;
  }
  uint8_t sdata[2] = {0x01, 0x00};
  this->_char_conf->writeValue(sdata, 2, true);
  this->_wait(2000);
  std::string rdata = this->_char_conf->readValue();
  if (rdata.size() >= 3 || rdata[0] == 0x81) {
    std::string ver = rdata.substr(2, rdata.size() - 2);
    return ver;
  } else {
    return empty_data;
  }
}

// ---------------------------------------------------------------
// 水平検出のしきい値設定
// ---------------------------------------------------------------
void ToioCore::setFlatThreshold(uint8_t deg) {
  if (!this->isConnected()) {
    return;
  }
  if (deg < 1) {
    deg = 1;
  }
  if (deg > 45) {
    deg = 45;
  }
  uint8_t data[3] = {0x05, 0x00, deg};
  this->_char_sound->writeValue(data, 3, true);
}

// ---------------------------------------------------------------
// 衝突検出のしきい値設定
// ---------------------------------------------------------------
void ToioCore::setClashThreshold(uint8_t level) {
  if (!this->isConnected()) {
    return;
  }
  if (level < 1) {
    level = 1;
  }
  if (level > 10) {
    level = 10;
  }
  uint8_t data[3] = {0x06, 0x00, level};
  this->_char_sound->writeValue(data, 3, true);
}

// ---------------------------------------------------------------
// ダブルタップ検出の時間間隔の設定
// ---------------------------------------------------------------
void ToioCore::setDtapThreshold(uint8_t level) {
  if (!this->isConnected()) {
    return;
  }
  if (level < 1) {
    level = 1;
  }
  if (level > 7) {
    level = 7;
  }
  uint8_t data[3] = {0x17, 0x00, level};
  this->_char_sound->writeValue(data, 3, true);
}

// ---------------------------------------------------------------
// モーター制御 (引数の値をそのまま送信するローレベルのメソッド)
// ---------------------------------------------------------------
void ToioCore::controlMotor(bool ldir, uint8_t lspeed, bool rdir, uint8_t rspeed, uint16_t duration) {
  if (!this->isConnected()) {
    return;
  }
  uint8_t ldir_data = ldir ? 0x01 : 0x02;
  uint8_t rdir_data = rdir ? 0x01 : 0x02;
  uint8_t dur_data = (float)duration / 10;
  uint8_t data[8] = {0x02, 0x01, ldir_data, lspeed, 0x02, rdir_data, rspeed, dur_data};
  size_t len = 8;
  if (duration == 0) {
	data[0] = 0x01;
	len = 7;
  }
  this->_char_motor->writeValue(data, len, true);
}

// ---------------------------------------------------------------
// 目標指定付きモーター制御
// ---------------------------------------------------------------

void ToioCore::moveToTarget(uint16_t x, uint16_t y, uint16_t angle, uint8_t max_speed, uint8_t move_type, uint8_t accel_type, uint8_t timeout) {
  if (!this->isConnected()) {
    return;
  }
  uint8_t data[13] = {0x03, 0x00, timeout, move_type, max_speed, accel_type, 0x00,
  				 	  lowByte(x), highByte(x), lowByte(y), highByte(y), lowByte(angle), highByte(angle)};
  this->_char_motor->writeValue(data, 13, true);
}

// ---------------------------------------------------------------
// 運転 (モーター制御をスロットルとステアリング操作に置き換える)
// - throttle : -100 ～ +100
// - handle   : -100 ～ +100
// ---------------------------------------------------------------
void ToioCore::drive(int8_t throttle, int8_t steering) {
  if (!this->isConnected()) {
    return;
  }
  uint8_t dir = 0x01;
  if (throttle < 0) {
    dir = 0x02;
  }
  throttle = abs(throttle);
  if(throttle > 100) {
  	  throttle = 100;
  }

  if (steering < -100) {
    steering = -100;
  } else if (steering > 100) {
    steering = 100;
  }

  double speed = 115.0 * (double)throttle / 100.0;

  // steering から左右のモーターのスピードを確定する
  double lspeed = speed;
  double rspeed = speed;
  if (steering < 0) {
    // 左に旋回を意味するので左のモーターを減速する
    lspeed = speed * (100 - abs(steering)) / 100.0;
  } else if (steering > 0) {
    // 右に旋回を意味するので右のモーターを減速する
    rspeed = speed * (100 - abs(steering)) / 100.0;
  }

  uint8_t data[7] = {0x01, 0x01, dir, (uint8_t)lspeed, 0x02, dir, (uint8_t)rspeed};
  this->_char_motor->writeValue(data, 7, true);
}

// ---------------------------------------------------------------
// ID Readerの読み取り結果を取得
// ---------------------------------------------------------------
ToioCoreIDData ToioCore::getIDReaderData() {
  ToioCoreIDData res;
  if (!this->isConnected()) {
    return res;
  }
  std::string data = this->_char_id_reader->readValue();
  if (data.size() > 0 && data[0] == 0x03 || data[0] == 0x04) {
    // no data
    return res;
  }
  if (!_convertBLEBytesToIDData((const uint8_t *) data.c_str(), data.size(), res)) {
    Serial.println("id data is wrong. type=" + String((int)data[0])+ " length=" + String(data.size()));
    return res;
  }
  return res;
}

// ---------------------------------------------------------------
// ID Readerのコールバックをセット
// ---------------------------------------------------------------
void ToioCore::onIDReaderData(OnIDDataCallback cb) {
  this->_on_id_reader = cb;
}

// ---------------------------------------------------------------
// Toio.cpp から呼ばれる (.ino からは直接呼ばない)
// ---------------------------------------------------------------
void ToioCore::_loop() {
  // 接続状態イベント
  if (g_event_connection_updated) {
    if (g_last_client == this->_client) {
      bool event_connection_state = false;
      if (g_current_client) {
        event_connection_state = true;
      } else {
        g_last_client = nullptr;
      }
      if (this->_onconnection) {
        this->_onconnection(event_connection_state);
      }
      g_event_connection_updated = false;
    }
  }

  // 未接続または接続中の ToioCore が自分自身でないなら、以降の処理は不要なので終了
  if (!this->isConnected() || g_current_client != this->_client) {
    return;
  }

  // バッテリーイベント
  if (g_event_battery_updated) {
    if (this->_onbattery) {
      this->_onbattery(g_event_battery_level);
      g_event_battery_updated = false;
    }
  }

  // ボタンイベント
  if (g_event_button_updated) {
    if (this->_onbutton) {
      this->_onbutton(g_event_button_state);
      g_event_button_updated = false;
    }
  }

  // モーションセンサーイベント
  if (g_event_motion_updated) {
    if (this->_onmotion) {
      this->_onmotion(g_event_motion_data);
      g_event_motion_updated = false;
    }
  }

  // ID読み取りセンサーイベント
  if (g_event_id_data_updated) {
    if (this->_on_id_reader) {
      this->_on_id_reader(g_event_id_data);
    }
    g_event_id_data_updated = false;
  }
}

// ---------------------------------------------------------------
// 指定のミリ秒数だけ待つ
// ---------------------------------------------------------------
void ToioCore::_wait(const unsigned long msec) {
  unsigned long now = millis();
  while (millis() < now + msec) {
    yield();
  }
}

// ---------------------------------------------------------------
// ID ReaderのデータをBLEのbyte配列から取り出す
// ---------------------------------------------------------------
bool ToioCore::_convertBLEBytesToIDData(const uint8_t *bytes, int length, ToioCoreIDData & id_data) {
    if (bytes[0] == 0x01) { // Position ID
      if (length != 13) {
        id_data.type = ToioCoreIDTypeNone;
        return false;
      }
      id_data.type = ToioCoreIDTypePosition;
      _convertBLEBytesToPositionIDData(bytes, id_data.position);
      return true;
    }
    if (bytes[0] == 0x02) { // Standard ID
      if (length != 7) {
        id_data.type = ToioCoreIDTypeNone;
        return false;
      }
      id_data.type = ToioCoreIDTypeStandard;
      _convertBLEBytesToStandardIDData(bytes, id_data.standard);
      return true;
    }

    id_data.type = ToioCoreIDTypeNone;
    return false;
}

// ---------------------------------------------------------------
// Position IDのデータをBLEのbyte配列から取り出す
// ---------------------------------------------------------------
void ToioCore::_convertBLEBytesToPositionIDData(const uint8_t *bytes, ToioCorePositionIDData & pos_data)
{
  pos_data.cubePosX = *(uint16_t *)&bytes[1];
  pos_data.cubePosY = *(uint16_t *)&bytes[3];
  pos_data.cubeAngleDegree = *(uint16_t *)&bytes[5];
  pos_data.sensorPosX = *(uint16_t *)&bytes[7];
  pos_data.sensorPosY = *(uint16_t *)&bytes[9];
  pos_data.sensorAngleDegree = *(uint16_t *)&bytes[11];
}

// ---------------------------------------------------------------
// Standard IDのデータをBLEのbyte配列から取り出す
// ---------------------------------------------------------------
void ToioCore::_convertBLEBytesToStandardIDData(const uint8_t *bytes, ToioCoreStandardIDData & std_data)
{
  std_data.standardID = *(uint32_t *)&bytes[1];
  std_data.cubeAngleDegree = *(uint16_t *)&bytes[5];
}
