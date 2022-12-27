//
// MMLParserToio.cpp
//

#include <ToioCore.h>

#include "MMLParserToio.h"

MMLParserToio::MMLParserToio() : totalTime(0), mmlCount(0), transpose(0), isLoop(false) {
	MMLParser();
	setMaxVelocity(255);
}

int MMLParserToio::playMML(MMLPTR mml, unsigned long tick, int repeatCount) {
	mmlCount = 0;
	return _playMML(mml, tick, repeatCount);
}

int MMLParserToio::_playMML(MMLPTR mml, unsigned long tick, bool isContinue, int repeatCount) {
	startTick = tick;
	totalTime = 0;
	velocity = maxVelocity;
	pBuffer = soundBuffer;
	*pBuffer++ = 3; //
	*pBuffer++ = repeatCount;
	pBuffer++;
	translate(mml, isContinue);
	soundBuffer[2] = translateCount;
	core->playSoundRaw(soundBuffer, translateCount * 3 + 3);
	totalTime *= repeatCount;
	Serial.printf("play MML #%d time=%d count=%d\n", mmlIndex, totalTime, translateCount);
	return totalTime;
}

void MMLParserToio::playMML(MMLPTR* _mml, int _mmlCount, unsigned long tick, bool _isLoop) {
	mml = _mml;
	mmlIndex = 0;
	mmlCount = _mmlCount;
	isLoop = _isLoop;
	_playMML(mml[0], tick);
}

void MMLParserToio::stopSound() {
	totalTime = 0;
	core->stopSound();
	Serial.println("stop sound");
}

bool MMLParserToio::loop(unsigned long tick) {
	if (totalTime != 0 && tick >= startTick + totalTime) {
		if (mmlIndex < mmlCount - 1) {
			_playMML(mml[++mmlIndex], tick, true);
		} else if (isLoop) {
			mmlIndex = 0;
			_playMML(mml[0], tick);
		} else {
			totalTime = 0;
		}
	}
	return isPlaying();
}

void MMLParserToio::callback2(int num, int velocity, int steps, int stepsGate) {
	if (translateCount >= MAX_OPERATION) return;
//	Serial.printf("%d, %d, %d, %d\n", num, velocity, steps, stepsGate);
	int prevTime = stepToTime(totalSteps);
	uint8_t len = (stepToTime(totalSteps + steps) - totalTime) / 10;
	totalTime += len * 10;
	*pBuffer++ = len;
 	*pBuffer++ = (uint8_t)(num + transpose);
	*pBuffer++ = velocity;
}
