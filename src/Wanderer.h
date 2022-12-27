//
// Wanderer.h
//

#include "TOFSensor.hpp"

#pragma once

enum class Action;

class Wanderer {
public:
	Wanderer();
	bool isReady();
	void start();
	void end();
	void pause(bool clashed);
	bool isPaused();
	bool isForwarding();
	void loop(unsigned long tick);

protected:
	TOFSensor tof;
	int initialized;

	unsigned long startTick;
	unsigned long targetTick;
	bool nextAction;
	Action action;
	int minDistance;
	int duration;
	bool isScanRight;
	int angle;

	void turn(bool isRight);
};
