//
// Wanderer.cpp
//

#include <M5Atom.h>

#include "Wanderer.h"
#include "PlayToio.h"
#include "AtomMatrix.h"

#define TURN_SPEED 10
#define TURN90_TIME 750
#define TURN180_TIME (TURN90_TIME * 2)
#define TURN45_TIME (TURN90_TIME / 2)
#define TURN15_TIME (TURN90_TIME / 6)
#define PAUSE_TIME 2000
#define BACK_TIME 1000
#define FORWARD_SPEED 30
#define BACK_SPEED 10
#define MAX_DISTANCE 300
#define STOP_DISTANCE 30

extern PlayToio playToio;
extern bool lockFace;

enum class Action {
	None, Stop, Paused, Forward, Backward, Turn, Scanning, TurnTo
};

Wanderer::Wanderer() : initialized(0) {
}

bool Wanderer::isReady() {
	if (initialized != 0) {
		return initialized > 0;
	}
	initialized = -1;
	if (!tof.isAvailable()) {
		Serial.println("VL53L0X not found");
		return false;
	} else if (!tof.begin(1.0)) {
		Serial.println("Failed to boot VL53L0X");
		return false;
	}
	initialized = 1;
	Serial.println("VL53L0X Ready");
	return true;
}

void Wanderer::start() {
	startTick = millis();
	action = Action::Turn;
	nextAction = false;
	isScanRight = true;
	angle = 0;
	duration = TURN90_TIME;
	turn(false);
}
void Wanderer::end() {
	playToio.stop();
	M5.dis.clear();
	action = Action::None;
}

void Wanderer::pause(bool clashed) {
	duration = PAUSE_TIME;
	playToio.stop();
	drawLEDChar(clashed ? LEDCHAR_EXCLAMATION : LEDCHAR_PAUSE, CRGB_YELLOW);
	action = Action::Paused;
}

bool Wanderer::isPaused() {
	return action <= Action::Paused;
}

bool Wanderer::isForwarding() {
	return action == Action::Forward;
}

void Wanderer::turn(bool isRight) {
	if (isRight) {
		playToio.setMotor(TURN_SPEED, -TURN_SPEED, duration);
	} else {
		playToio.setMotor(-TURN_SPEED, TURN_SPEED, duration);
	}
	int r = duration * 180 / TURN180_TIME;
	if (!isRight) {
		r = -r;
	}
	angle += r;
	Serial.printf("turn=%d (angle=%d)\n", r, angle);
	unsigned long t = millis();
}

void Wanderer::loop(unsigned long tick) {
	if (action == Action::None) return;
	if (!nextAction) {
		if (tick < startTick + duration || duration == 0) {
			if (action == Action::Scanning) {
				int d = tof.getDistance();
				// Serial.printf("d=%d\n", d);
				if (d >= 0 && d < minDistance) {
					minDistance = d;
					targetTick = tick;
				}
			} else if (action == Action::Forward) {
				int d = tof.getDistance();
				// Serial.printf("d=%d\n", d);
				if (d >= 0 && d < STOP_DISTANCE) {
					pause(false);
				}
			}
		} else {
			nextAction = true; // one loop delayed
			return;
		}
	} else {
		nextAction = false;
		startTick = tick;
		lockFace = false;
		switch (action) {
			case Action::Turn:
				action = Action::Scanning;
				duration = TURN180_TIME;
				minDistance = MAX_DISTANCE;
				turn(isScanRight);
				break;
			case Action::Scanning:
				if (minDistance == MAX_DISTANCE) {
					duration = random(TURN180_TIME - TURN15_TIME * 2) + TURN15_TIME;
					drawLEDChar(LEDCHAR_QUESTION, CRGB_YELLOW);
					lockFace = true;
				} else {
					duration = tick - targetTick;
				}
				action = Action::TurnTo;
				Serial.printf("minDistance=%d, (%dms)\n", minDistance, duration);
				turn(!isScanRight);
				break;
			case Action::TurnTo:
				action = Action::Forward;
				duration = 0;
				playToio.setMotor(FORWARD_SPEED, FORWARD_SPEED);
				break;
			case Action::Paused:
				action = Action::Backward;
				duration = BACK_TIME;
				playToio.setMotor(-BACK_SPEED, -BACK_SPEED);
				break;
			case Action::Backward:
				action = Action::Turn;
				duration = TURN45_TIME;
				isScanRight = angle <= 0;
				turn(isScanRight);
				break;
		}
	}
}
