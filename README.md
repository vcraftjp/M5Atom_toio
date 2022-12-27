# M5Atom_toio
[ATOM Mate for toio](https://www.switch-science.com/products/8500) Fork Dance & TOF sensor demo<br>
Youtube [Fork Dance](https://www.youtube.com/watch?v=xxcl3fl0vaM),  [TOF sensor](https://www.youtube.com/watch?v=3qvqGaXkXsg)
![git_toio](https://user-images.githubusercontent.com/46808493/209662553-caa17e44-635b-47bc-b975-7f9b49ed55db.jpg)

## How to play
- Power on **toio** #1,#2.
- Press ATOM Matrix LED button to scan and connect **toio** via BLE. (If the scan finds two **toio**, #1. then #2)
- Press the button to start/stop dancing.
- Double tap **toio** to change dancing/wandering mode.

## Feature
- Play melody with [MML Parser](https://github.com/vcraftjp/MML-Parser).
```C
const char* music_dance[] = {
    "L8 e.d16 c.<b16>c.d16c.<g16e.f16 g.a16g.f#16gr>c.d16erere.d16c.d16 e4d16r.d16r.",
    "   e.d16 c.<b16>c.d16c.<g16e.f16 g.a16g.f#16gr>c.d16ergrg.e16c.d16 erdrcrr4",
    "   egregrgregregr4.farfararfarfar",
    "   a.b16>crcr<grgrererdrc.d16 ergrg.e16c.d16 erdrcr"
};
```
- Dancing with MML-like language. (PlayToio class)
```C
const char* action_dance = "W#4 [F30#2 W#2 B30#2 W#2 T39 #1.: W#2] T26 @R90 #4 W#4"
                           "[L26 R40 #8. : W#4^32]8 W16 T-22 M40 @R90 #4 F30#2 W#2 B30#2 W#2 T39 #1.";
```

## Dependent libraries
- [futomi/M5StackToio](https://github.com/futomi/M5StackToio)
- [mhama/added ID reader methods pull request](https://github.com/futomi/M5StackToio/pull/1)
- [adafruit/Adafruit_VL53L0X](https://github.com/adafruit/Adafruit_VL53L0X)

Added functions and modified code to the [ToioCore class](https://github.com/futomi/M5StackToio/blob/master/src/ToioCore.h).

```C
    // 目標指定付きモーター制御
    void moveToTarget(uint16_t x, uint16_t y, uint16_t angle, uint8_t max_speed, uint8_t move_type = 0, uint8_t accel_type = 0, uint8_t timeout = 5);
```

```C
void ToioCore::controlMotor(bool ldir, uint8_t lspeed, bool rdir, uint8_t rspeed, uint16_t duration) {
	...
  if (duration == 0) {
	data[0] = 0x01;
	len = 7;
  }
  this->_char_motor->writeValue(data, len, true);
}
```

```C
struct ToioCoreMotionData {
  bool flat;
  bool clash;
  bool dtap;
  uint8_t attitude;
  uint8_t shake; // <--
};

ToioCoreMotionData ToioCore::getMotion() {
	...
  std::string data = this->_char_motion->readValue();
  if (data.size() != 6) { // 5 -> 6
    return res;
  }
  res.flat = data[1];
  res.clash = data[2];
  res.dtap = data[3];
  res.attitude = data[4];
  res.shake = data[5]; // <--
  return res;
}
```
