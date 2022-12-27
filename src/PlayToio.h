//
// PlayToio.h
//

#pragma once

#include <stdint.h>

class ToioCore;

#define MAX_LOOP_NEST 4

struct PlayLoop {
	int16_t index;
	int16_t count;
};

class PlayToio {
public:
	PlayToio();
	void setToioCore(ToioCore* _core) { core = _core; }
	void play(const char* action);
	void stop();
	bool loop(unsigned long tick);
	bool isPlaying() { return p != nullptr; }
	void setTempo(int16_t _tempo) { tempo = _tempo; }
	void setSpeedRatio(float ratio = 1.0f) { speedRatio = ratio; }
	void setMotor(int speedLeft, int speedRight, int duration = 0);
	void moveTo(int x, int y, int angle, int maxSpeed, bool isSigned = true);
	void setMotorCallback(void (*cb)(int, int)) { pfnMotorCallback = cb; }

protected:
	ToioCore* core;
	const char* pStart;
	const char* p;
	unsigned long prevTick;
	int duration;
	int16_t tempo;
	PlayLoop playLoops[MAX_LOOP_NEST];
	int16_t loopNest;
	int16_t prevX;
	int16_t prevY;
	int16_t prevAngle;
	bool isReadPosition;
	float speedRatio;
	void(*pfnMotorCallback)(int, int);

	bool readStartPosition();
	bool parse();
	int parseValue();
	int parseSteps();
	void skipSpace();
	void skipAlpha();
	void parseError();
	bool loopStart();
	bool loopEnd();
	bool loopSkip();
	void controlMotor(int speedLeft, int speedRight);
};