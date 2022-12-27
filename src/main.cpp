#include <M5Atom.h>
#include "AtomMatrix.h"
#include "Toio.h"
#include "MMLParserToio.h"
#include "PlayToio.h"
#include "Wanderer.h"

#define MAX_TOIO 4
#define TOIO_COUNT 2

Toio toio;
std::vector<ToioCore*> toioCores;
ToioCore* core = nullptr;
int coreIndex = -1;
MMLParserToio mmlParser;
PlayToio playToio;
Wanderer wanderer;

bool isWandering = false;
bool lockFace = false;
bool isConnected = false;

void ChangePlayMode();
void onClashedWandering();
void endWandering();
void onSetMoter(int, int);

void coreDebugPrint(const char* msg) {
	Serial.printf("toio[%d] %s\n", coreIndex, msg);
}

uint32_t coreColors[MAX_TOIO] = { CRGB_CYAN, CRGB_MAGENTA, CRGB_GREEN, CRGB_YELLOW };

uint32_t getCoreColor() {
	return coreColors[coreIndex];
}

void setup() {
    M5.begin(true, true, true);
	M5.dis.setWidthHeight(5, 5);
	delay(100);
	M5.dis.fillpix(CRGB_GRAY);
	delay(500);
	M5.dis.clear();
}

void connectionChanged(bool state) {
	if (state == isConnected) return;
	isConnected = state;
	if (state) {
		drawLEDChar(LEDCHAR_OK_L, CRGB_CYAN);
		coreDebugPrint("connected!");
	} else {
		drawLEDChar(LEDCHAR_NG_L, CRGB_MAGENTA);
		coreDebugPrint("disconnected.");
	}
	delay(500);
	M5.dis.clear();
}

void onMotionCallback(ToioCoreMotionData motion) {
	if (motion.clash) {
		Serial.println("*** clashed!");
		if (isWandering) {
			onClashedWandering();
		}
	}
	if (motion.dtap) {
		Serial.println("*** double tapped!");
		ChangePlayMode();
	}
	if (motion.shake) {
		Serial.println("*** shaked!");
		if (isWandering) {
			endWandering();
		}
		if (isWandering || !mmlParser.isPlaying()) {
			core->playSoundEffect(1);
		}
	}
}

int scanToio() {
	Serial.println("scan toio ...");
	drawLEDChar(LEDCHAR_SCAN, CRGB_BLUE);
 	toioCores = toio.scan(3);
	size_t n = toioCores.size();
	if (n == 0) {
		Serial.println("toio not found.");
		drawLEDChar(LEDCHAR_NG_L, CRGB_RED);
		core = nullptr;
		coreIndex = -1;
	} else {
		for (int i = 0; i < n; i++) {
			coreIndex = i;
			coreDebugPrint(toioCores.at(i)->getName().c_str());
		}
		Serial.printf("%d toio(s) found!\n", n);
		core = toioCores.at(0);
		coreIndex = TOIO_COUNT - n;
		drawLEDChar(coreIndex + 1, getCoreColor());
		core->onConnection([](bool state) {
			connectionChanged(state);
		});
		core->onMotion([](ToioCoreMotionData motion) {
			onMotionCallback(motion);
		});

		core->setClashThreshold(2);
		mmlParser.setToioCore(core);
		playToio.setToioCore(core);
		playToio.setMotorCallback(onSetMoter);
	}
	return n;
}

bool connectToio() {
	coreDebugPrint("connecting ...");
	drawLEDChar(LEDCHAR_3DOTS, CRGB_YELLOW);
	bool state = core->connect();
	connectionChanged(state);
	return state;
}

void disConnectToio() {
	core->disconnect();
}

bool isToioReady() {
	return core && core->isConnected();
}

bool readPosition() {
	static uint16_t prevPosX;
	static uint16_t prevPosY;
	static uint16_t prevAngle;

	unsigned long t0 = millis();
	ToioCoreIDData data = core->getIDReaderData();
	unsigned long t1 = millis();
	if (data.type == ToioCoreIDTypePosition) {
		uint16_t posX = data.position.cubePosX;
		uint16_t posY = data.position.cubePosY;
		uint16_t angle = data.position.cubeAngleDegree;
		if (posX != prevPosX || posY != prevPosY || angle != prevAngle) {
			prevPosX = posX;
			prevPosY = posY;
			prevAngle = angle;
			Serial.printf("toio[%d] position(%d, %d) angle=%d (%d ms)\n", coreIndex, posX, posY, angle, t1 - t0);
		}
		return true;
	}
	return false;
}

#define TEMPO 160
#define TEMPO_FAST 200
#define TEMPO_SLOW 120

const char* music_dance[] = {
	"L8 e.d16 c.<b16>c.d16c.<g16e.f16 g.a16g.f#16gr>c.d16erere.d16c.d16 e4d16r.d16r.",
	"   e.d16 c.<b16>c.d16c.<g16e.f16 g.a16g.f#16gr>c.d16ergrg.e16c.d16 erdrcrr4",
	"   egregrgregregr4.farfararfarfar",
	"   a.b16>crcr<grgrererdrc.d16 ergrg.e16c.d16 erdrcr"
};

const char* music_wander1[] = {
	"L8 [e.r16e.r16e.d16e.f16 g.r16gr16g16e4r4 d.r16d.e16f4e.d16 : e.d16e.f16g4r4]c.r16cr16c32r32c4r4",
	"    a.r16a.b16>c.<b16>c.<a16 g.r16gr16g16e4r4 d.r16d.e16fr16f16e.d16 e.d16e.f16g4r4",
	"    a.r16a.b16>c.<b16>c.<a16 g.r16gr16g16e4r4 d.r16d.e16fr16f16e.d16 c.r16cr16c32r32c4r4"
};

const char* music_wander2[] = {
	"L4 [[e<ab8>c8d] e<a>fe8d8 : cder]c<bar>",
	"   [g8.r16g8.r16g.f8 edc:r Q7d8d8d8d8Q8gf e2.r]d edc<ba2.r",
	"   [O3 Q7b8b8b8b8>ddQ8c8d8e2r : Q7d8d8d8d8Q8gf e2.r] e8<b8>c8d8c<b a2.r"
};

const char* action_connect = "T30 300 T-30 300";
const char* action_dance = "W#4 [F30#2 W#2 B30#2 W#2 T39 #1.: W#2] T26 @R90 #4 W#4"
						   "[L26 R40 #8. : W#4^32]8 W16 T-22 M40 @R90 #4 F30#2 W#2 B30#2 W#2 T39 #1.";

const char* action_test = "M30 @X0 @Y0 @A270 2000 [M30 @V50 @R90 1500]4";

bool fastDance = false;

void startDance() {
	Serial.println("\n--- start dancing");
	lockFace = false;
	int tempo = fastDance ? TEMPO_FAST : TEMPO;
	MMLParser::setTempo(tempo);
	mmlParser.setTranspose(fastDance ? 5 : 0);
	mmlParser.playMML(music_dance, 4, millis());
	playToio.setTempo(tempo);
	playToio.setSpeedRatio(fastDance ? (float(TEMPO_FAST) / TEMPO) : 1.0f);
	playToio.play(action_dance);
}

void endDance() {
	mmlParser.stopSound();
	playToio.stop();
	fastDance = false;
	M5.dis.clear();
	Serial.println("--- end dancing");
}

void ChangePlayMode() {
	if (!isToioReady() || mmlParser.isPlaying() || playToio.isPlaying()) return;
	if (!isWandering) {
		if (wanderer.isReady()) {
			drawLEDChar(LEDCHAR_UP, CRGB_CYAN);
			isWandering = true;
			core->playSoundEffect(5); // Mat out
			Serial.println("change to wandering mode");
		} else {
			drawLEDChar(LEDCHAR_NG_S, CRGB_MAGENTA);
		}
		delay(500);
		M5.dis.clear();
	} else {
		drawLEDChar(LEDCHAR_OK_L, CRGB_CYAN);
		isWandering = false;
		core->playSoundEffect(4); // Mat in
		delay(500);
		M5.dis.clear();
		Serial.println("change to dancing mode");
	}
}

void startWandering() {
	Serial.println("\n--- start wandering");
	lockFace = false;
	wanderer.start();
	MMLParser::setTempo(coreIndex == 1 ? TEMPO_SLOW : TEMPO);
	mmlParser.playMML(coreIndex == 1 ? music_wander2 : music_wander1, 3, millis(), true);
}

void endWandering() {
	mmlParser.stopSound();
	wanderer.end();
	Serial.println("--- end wandering");
}

void onClashedWandering() {
	if (wanderer.isForwarding()) {
		wanderer.pause(true);
	}
}

enum class MotorMode {
	Stop, Forward, Backward, TurnLeft, TurnRight
};

MotorMode motorMode = MotorMode::Stop;
unsigned long animateTick;
int shiftIndex;

void onSetMoter(int speedLeft, int speedRight) {
	Serial.printf("onSetMotor: %d, %d\n", speedLeft, speedRight);
	animateTick = millis();
	shiftIndex = 0;
	if (speedLeft == 0 && speedRight == 0) {
		motorMode = MotorMode::Stop;
		M5.dis.clear();
	} else if (speedLeft > 0 && speedRight > 0) {
		motorMode = MotorMode::Forward;
	} else if (speedLeft < 0 && speedRight < 0) {
		motorMode = MotorMode::Backward;
	} else if (speedLeft > 0 && speedRight <= 0) {
		motorMode = MotorMode::TurnLeft;
	} else {
		motorMode = MotorMode::TurnRight;
	}
}

void loopDrawFace(unsigned long tick) {
	if (isWandering && wanderer.isPaused() || lockFace) return;
	int period = 60 * 1000 / MMLParser::getTempo() / 4;
	if (tick - animateTick >= period) {
		animateTick = tick;
		int animCount = (motorMode == MotorMode::Forward || motorMode == MotorMode::Backward) ? 5 : 3;
		if (++shiftIndex >= animCount) {
			shiftIndex = 0;
		}
		switch (motorMode) {
			case MotorMode::Stop:
				M5.dis.clear();
				break;
			case MotorMode::Forward:
				drawLEDChar(LEDCHAR_FORWARD, getCoreColor(), 0, shiftIndex);
				break;
			case MotorMode::Backward:
				drawLEDChar(LEDCHAR_BACKWARD, getCoreColor(), 0, -shiftIndex);
				break;
			case MotorMode::TurnLeft:
				drawLEDChar(LEDCHAR_ROTATE + (2 - shiftIndex), getCoreColor());
				break;
			case MotorMode::TurnRight:
				drawLEDChar(LEDCHAR_ROTATE + shiftIndex, getCoreColor());
				break;
		}
	}
}

#define START_POS 56
#define START_SPEED 60
#define END_SPEED 20

int16_t startPositions[MAX_TOIO][3] = { // X, Y, Angle
	{ -START_POS, 0, 0 },
	{ START_POS, 0, 180 },
	{ 0, START_POS, 270 },
	{ 0, -START_POS, 90 },
};

#define DELAY 20

void loop() {
	static unsigned long prevTick = 0;

	if (M5.Btn.wasPressed()) {
		Serial.println("button pressed");

		if (isToioReady()) {
			if (isWandering) {
				if (mmlParser.isPlaying()) {
					endWandering();
				} else {
					delay(500);
					if (wanderer.isReady()) {
						startWandering();
					} else {
						drawLEDChar(LEDCHAR_NG_S, CRGB_MAGENTA);
						delay(500);
						M5.dis.clear();
					}
				}
			} else {
				if (mmlParser.isPlaying()) {
					endDance();
				} else {
					delay(500);
					startDance();
				}
			}
		} else {
			int n = toioCores.size();
			if (n == 0) {
				delay(500);
				n = scanToio();
			}
			delay(500);
			M5.dis.clear();
			if (n > 0) {
				if (connectToio()) {
					if (readPosition()) {
						int16_t* startPos = startPositions[coreIndex];
						playToio.moveTo(startPos[0], startPos[1], startPos[2], START_SPEED);
					} else {
						playToio.play(action_connect);
					}
				}
			}
		}
	}
	if (mmlParser.isPlaying()) {
		unsigned long tick = millis();

		loopDrawFace(tick);

		if (isWandering) {
			wanderer.loop(tick);
		}

		if (!mmlParser.loop(tick)) {
			if (!isWandering) {
				Serial.println("music finished");
				if (!fastDance) {
					fastDance = true;
					startDance();
				} else {
					fastDance = false;
					M5.dis.clear();
					if (readPosition()) {
						int16_t* startPos = startPositions[coreIndex];
						playToio.moveTo(startPos[0], startPos[1], startPos[2], END_SPEED);
					}
				}
			}
		}
	}
	if (isToioReady()) {
		// readPosition();
		toio.loop();
		playToio.loop(millis());
	}

	unsigned long tick = millis();
	unsigned long t = tick - prevTick;
	if (t < DELAY) {
		delay(DELAY - t);
	}
	prevTick = tick;
	M5.update();
}