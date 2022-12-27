//
// TOFSensor.hpp
//
#pragma once

#define ADAFRUIT

//#include <M5Stack.h>
//#include <M5StickC.h>
#include <M5Atom.h>

#ifdef ADAFRUIT
  #include <Adafruit_VL53L0X.h>
  #define VL53L0X_CLASS Adafruit_VL53L0X
#else // Pololu
  #include "VL53L0X.h"
  #define VL53L0X_CLASS VL53L0X
#endif

#define TOF_ERROR -1
#define TOF_INFINITY 8190

//
// TOFSensor class
//
class TOFSensor : VL53L0X_CLASS {
public:
	TOFSensor() : isReady(false), distance(0) {}

	bool begin(float _filterGain = 1.0, bool _isDebug = false) {
		filterGain = _filterGain;
		isDebug = _isDebug;
#ifdef ADAFRUIT
		return isReady = Adafruit_VL53L0X::begin(VL53L0X_I2C_ADDR, isDebug);
#else // Pololu
		isReady = init();
		setTimeout(500);
		return isReady;
#endif
	}

	bool isAvailable() {
		Wire.beginTransmission(VL53L0X_I2C_ADDR);
		return (Wire.endTransmission() == 0);
	}

	int getDistance() {
		int d = getDistanceRaw();
		if (d <= 0 || d >= TOF_INFINITY) {
			return d;
		}
		if (filterGain == 1.0) {
			distance = d;
		} else {
			distance += (d - distance) * filterGain;
		}
		return distance;
	}

	bool isReady;
	int distance;
	float filterGain;

private:
	int getDistanceRaw() {
#ifdef ADAFRUIT
		VL53L0X_RangingMeasurementData_t measure;
		getSingleRangingMeasurement(&measure, isDebug);
		if (measure.RangeStatus == 4) {  // phase failures have incorrect data
			return TOF_ERROR;
		}
		return measure.RangeMilliMeter;
#else // Pololu
		int d = readRangeSingleMillimeters();
		if (timeoutOccurred()) {
			return TOF_ERROR;
		}
		return d;
#endif
	}

	bool isDebug;
};
