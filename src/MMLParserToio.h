//
// MMLParserToio.h
//

#pragma once

#include <functional>

#include "MMLParser.h"

#define MAX_OPERATION 59
#define MAX_SOUND_BUFFER (MAX_OPERATION * 3 + 3)

class ToioCore;

class MMLParserToio : MMLParser {
public:
	MMLParserToio();
	void setToioCore(ToioCore* _core) { core = _core; }
	int playMML(MMLPTR mml, unsigned long tick, int repeatCount = 1);
	void playMML(MMLPTR mml[], int mmlCount, unsigned long tick, bool isLoop = false);
	void stopSound();
	virtual bool isPlaying() { return totalTime != 0; }
	bool loop(unsigned long tick);
	void setTranspose(int _transpose = 0) { transpose = _transpose; }

protected:
	ToioCore* core;
	MMLPTR* mml;
	int mmlIndex;
	int mmlCount;
	uint8_t soundBuffer[MAX_SOUND_BUFFER];
	uint8_t* pBuffer;
	int totalTime;
	unsigned long startTick;
	int transpose;
	bool isLoop;

	int _playMML(MMLPTR mml, unsigned long tick, bool isContinue = false, int repeatCount = 1);
	virtual void callback2(int num, int velocity, int steps, int stepsGate);
};