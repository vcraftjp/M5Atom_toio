/* ----------------------------------------------------------------
  Toio.h

  Copyright (c) 2020 Futomi Hatano. All right reserved.
  https://github.com/futomi

  Licensed under the MIT license.
  See LICENSE file in the project root for full license information.
  -------------------------------------------------------------- */
#ifndef Toio_h
#define Toio_h

#include <Arduino.h>
#include <string>
#include <map>
#include <vector>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include "ToioCore.h"

// ---------------------------------------------------------------
// Toio クラス
// ---------------------------------------------------------------
class Toio {
  private:
    // BLE Scan Interval (ミリ秒)
    const int _BLE_SCAN_INTERVAL = 100;

    // BLE Scan Window (ミリ秒)
    const int _BLE_SCAN_WINDOW = 99;

    // toio のプライマリサービス UUID (スキャンのフィルタリングに使う)
    const char* _TOIO_SERVICE_UUID = "10b20100-5b3b-4571-9508-cf3efcd7bbae";

    // 発見済みの toio (ToioCore オブジェクト) の map (キーはアドレス)
    std::map<std::string, ToioCore*> _devices;

  public:
    // コンストラクタ
    Toio();

    // toio をスキャン
    std::vector<ToioCore*> scan(uint8_t duration);

    // .ino の loop() 内で呼び出す
    void loop();
};

#endif
