//
// PlayToio.h
//

#include <ToioCore.h>

#include "PlayToio.h"

#define TIMEBASE 192

#define POS_OFFSET 250

PlayToio::PlayToio() :
	p(nullptr),
	tempo(120),
	isReadPosition(false),
	speedRatio(1.0f),
	pfnMotorCallback(nullptr) {
}

void PlayToio::play(const char* action) {
	p = pStart = action;
	prevTick = 0;
	loopNest = 0;
	isReadPosition = readStartPosition();
}

bool PlayToio::loop(unsigned long tick) {
	if (p && (prevTick == 0 || tick >= prevTick + duration)) {
		prevTick = tick;
		if (parse()) {
			p = nullptr;
			return false;
		}
	}
	return true;
}

bool PlayToio::readStartPosition() {
	ToioCoreIDData data = core->getIDReaderData();
	if (data.type == ToioCoreIDTypePosition) {
		prevX = data.position.cubePosX;
		prevY = data.position.cubePosY;
		prevAngle = data.position.cubeAngleDegree;
		Serial.printf("start position: x=%d, y=%d, angle=%d\n", prevX, prevY, prevAngle);
		return true;
	}
	return false;
}

bool PlayToio::parse() {
	int speedLeft = 0;
	int speedRight = 0;
	duration = 0;

	bool isTargeting = false;
	bool isRelativeXY = false;
	int targetX = UINT16_MAX;
	int targetY = UINT16_MAX;
	int targetAngle = UINT16_MAX;
	int maxSpeed = 0;

	for (;;) {
		skipSpace();
		char c = *p++;
		if (c == '\0') {
			p--;
			return true;
		}
		if (c >= 'a' && c <= 'z') {
			c -= 'a' - 'A';
		}
		if (c >= 'A' && c <= 'Z') {
			skipAlpha();
			skipSpace();
			int value = parseValue();
			switch (c) {
				case 'L': // Left motor
					speedLeft = value;
					break;
				case 'R': // Right motor
					speedRight = value;
					break;
				case 'F': // Forward
					speedLeft = speedRight = value;
					break;
				case 'B': // Backward
					speedLeft = speedRight = -value;
					break;
				case 'T': // Turn
					speedLeft = value;
					speedRight = -value;
					break;
				case 'M': // Motor
					if (isReadPosition) {
						speedLeft = speedRight = value;
					}
					break;
				case 'W': // Wait
					break;
				default:
					parseError();
					return true;
			}
		} else if (c >= '0' && c <= '9') {
			p--;
			duration = parseValue();
			break;
		} else if (c == '#') {
			int steps = parseSteps();
			duration = 60 * 1000 * steps / tempo / 48;
			break;
		} else if (c == '@') { // target mark
			c = *p++;
			if (c >= 'a' && c <= 'z') {
				c -= 'a' - 'A';
			}
			if (!isTargeting) {
				isTargeting = true;
				maxSpeed = max(abs(speedLeft), abs(speedRight));
			}
			int value = parseValue();
			switch (c) {
				case 'X':
					prevX = targetX = value + POS_OFFSET;
					break;
				case 'Y':
					prevY = targetY = value + POS_OFFSET;
					break;
				case 'A':
					prevAngle = targetAngle = value;
					break;
				case 'U':
					targetX = value;
					isRelativeXY = true;
					break;
				case 'V':
					targetY = value;
					isRelativeXY = true;
					break;
				case 'R':
					targetAngle = prevAngle + value;
					break;
				default:
					parseError();
					return true;
			}
		} else {
			switch (c) {
				case '[':
					if (!loopStart()) return true;
					break;
				case ']':
					if (!loopEnd()) return true;
					break;
				case ':':
					if (!loopSkip()) return true;
					break;
				default:
					parseError();
					return true;
			}
		}
	}
	if (duration && (speedLeft || speedRight)) {
		if (isTargeting && isReadPosition) {
			if (isRelativeXY) {
				if (targetX == UINT16_MAX) {
					targetX = 0;
				}
				if (targetY == UINT16_MAX) {
					targetY = 0;
				}
				float x = float(targetX);
				float y = float(targetY);
				float r = float(270 - prevAngle) * PI / 180.f;
				targetX = prevX + round(x * cos(r) - y * sin(r));
				targetY = prevY - round(x * sin(r) + y * cos(r));
				prevX = targetX;
				prevY = targetY;
			}
			if (targetAngle != UINT16_MAX) {
				if (targetAngle < 0) {
					targetAngle += 360;
				} else if (targetAngle > 360) {
					targetAngle -= 360;
				}
				prevAngle = targetAngle;
			} else {
				targetAngle = 0x05 << 13; // non-rotating mode
			}
			moveTo(targetX, targetY, targetAngle, maxSpeed, false);
		} else {
			controlMotor(speedLeft, speedRight);
		}
	}
	return false;
}

void PlayToio::setMotor(int speedLeft, int speedRight, int _duration) {
	duration = _duration;
	controlMotor(speedLeft, speedRight);
}


void PlayToio::controlMotor(int speedLeft, int speedRight) {
	if (speedRatio != 1.0f) {
//			duration = round(float(duration) / speedRatio);
		speedLeft = round(float(speedLeft) * speedRatio);
		speedRight = round(float(speedRight) * speedRatio);
	}
	uint8_t lspeed = (uint8_t)speedLeft;
	uint8_t rspeed = (uint8_t)speedRight;
	bool ldir = true;
	bool rdir = true;
	if (speedLeft < 0) {
		ldir = false;
		lspeed = (uint8_t)-speedLeft;
	}
	if (speedRight < 0) {
		rdir = false;
		rspeed = (uint8_t)-speedRight;
	}
	core->controlMotor(ldir, lspeed, rdir, rspeed, duration);
	Serial.printf("motor: left=%d, right=%d, %dms\n", speedLeft, speedRight, duration);
	if (pfnMotorCallback) {
		(*pfnMotorCallback)(speedLeft, speedRight);
	}
}

void PlayToio::stop() {
	p = nullptr;
	core->controlMotor(true, 0, true, 0);
	Serial.printf("stop motor\n");
}

void PlayToio::moveTo(int x, int y, int angle, int maxSpeed, bool isSigned) {
	if (isSigned) {
		x += POS_OFFSET;
		y += POS_OFFSET;
	}
	core->moveToTarget(x, y, angle, maxSpeed);
	Serial.printf("move to: x=%d, y=%d, angle=%d, maxSpped=%d\n", x, y, angle, maxSpeed);
}

void PlayToio::parseError() {
	Serial.printf("PlayToio: parse error: '%s'\n", p - 1);
	p =  nullptr;
}

int PlayToio::parseValue() {
	int value = 0;
	bool minus = false;
	if (*p == '+') {
		p++;
	} else if (*p == '-') {
		minus = true;
		p++;
	}
	for (;;) {
		char c = *p;
		if (c < '0' || c > '9') break;
		if (value < 0) {
			value = 0;
		}
		value = value * 10 + (c - '0');
		p++;
	}
	if (minus) {
		value = -value;
	}
	return value;
}

int PlayToio::parseSteps() {
	int steps = 0;
	for (;;) {
		int len = parseValue();
		steps += TIMEBASE / len;
		while (*p == '.') {
			p++;
			steps += TIMEBASE / (len <<= 1);
		}
		if (*p == '^') { // tie
			p++;
			skipSpace();
			continue;
		}
		break;
	}
	return steps;
}

void PlayToio::skipSpace() {
	for (;;) {
		char c = *p;
		if ((c > '\0' && c <= ' ') || c == ';') {
			p++;
			continue;
		}
		break;
	}
}

void PlayToio::skipAlpha() {
	for (;;) {
		char c = *p;
		if (c >= 'A' && c <= 'Z' || c >= 'a' && c <= 'z') {
			p++;
			continue;
		}
		break;
	}
}

bool PlayToio::loopStart() {
	if (loopNest >= MAX_LOOP_NEST) return false;
	PlayLoop& playLoop = playLoops[loopNest];
	playLoop.index = p - pStart;
	playLoop.count = -1;
	loopNest++;
	return true;
}

bool PlayToio::loopEnd() {
	if (loopNest == 0) return false;
	int value = parseValue();
	PlayLoop& playLoop = playLoops[loopNest - 1];
	if (playLoop.count == -1) {
		playLoop.count = (value <= 0) ? 2 : value;
	}
	if (--playLoop.count > 0) {
		p = pStart + playLoop.index;
	} else {
		loopNest--;
	}
	return true;
}

bool PlayToio::loopSkip() {
	if (loopNest == 0) return false;
	PlayLoop& playLoop = playLoops[loopNest - 1];
	if (playLoop.count == 1) {
		char c;
		while ((c = *p)) {
			if (c == ']') {
				break;
			}
			p++;
		}
	}
	return true;
}

