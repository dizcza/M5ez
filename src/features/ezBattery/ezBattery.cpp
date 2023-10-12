#include <Preferences.h>
#include "../../M5ez.h"
#include "ezBattery.h"

#define BATTERY_CHARGING_OFF 	(255)

bool ezBattery::_canControl = false;
bool ezBattery::_on = false;
uint8_t ezBattery::_numChargingBars = BATTERY_CHARGING_OFF;

bool ezBattery::entry(uint8_t command, void* /* user */) {
	switch(command) {
		case FEATURE_MSG_PING:
			return true;
		case FEATURE_MSG_START:
			begin();
			return true;
		case FEATURE_MSG_STOP:
			if(_on) {
				_on = false;
				_refresh();
			}
			return true;
		case FEATURE_MSG_QUERY_ENABLED:
			return _on;
	}
	return false;
}

void ezBattery::begin() {
	//Wire.begin();
	_canControl = _isChargeControl();
	ezBattery::_readFlash();
	ez.settings.menuObj.addItem("Battery settings", ezBattery::menu);
	if (_on) {
		_refresh();
	}
}

void ezBattery::menu() {
	bool on_orig = _on;
	while(true) {
		ezMenu clockmenu("Battery Settings");
		clockmenu.txtSmall();
		clockmenu.buttons("up#Back#select##down#");
		clockmenu.addItem("on|Display battery\t" + (String)(_on ? "on" : "off"));
		switch (clockmenu.runOnce()) {
			case 1:
				_on = !_on;
				_refresh();
				break;
			case 0:
				if (_on != on_orig) {
					_writeFlash();
				}
				return;
		}
	}
}

uint32_t ezBattery::loop() {
	_adaptChargeMode();
	if (!_on) return 0;
	ez.header.draw("battery");
	return (_numChargingBars != BATTERY_CHARGING_OFF ? 1000000 : 5000000);	// 1s/5s
}

void ezBattery::_readFlash() {
	Preferences prefs;
	prefs.begin("M5ez", true);	// true: read-only
	_on = prefs.getBool("battery_icon_on", true);
	prefs.end();
}

void ezBattery::_writeFlash() {
	Preferences prefs;
	prefs.begin("M5ez");
	prefs.putBool("battery_icon_on", _on);
	prefs.end();
}

void ezBattery::_refresh() {
	if (_on) {
		ez.header.insert(RIGHTMOST, "battery", ez.theme->battery_bar_width + 2 * ez.theme->header_hmargin, ezBattery::_drawWidget);
		ez.addEvent(ezBattery::loop);
	} else {
		ez.header.remove("battery");
		ez.removeEvent(ezBattery::loop);
	}
}

void ezBattery::_drawWidget(uint16_t x, uint16_t w) {
	uint8_t currentBatteryLevel = _getTransformedBatteryLevel();
	if((_isChargeFull() == false) && (_isCharging() == true)) {
		if(_numChargingBars < currentBatteryLevel) {
			_numChargingBars++;
		} else {
			_numChargingBars = 0;
		}
	} else {
		_numChargingBars = BATTERY_CHARGING_OFF;
	}
	uint16_t left_offset = x + ez.theme->header_hmargin;
	uint8_t top = ez.theme->header_height / 10;
	uint8_t height = ez.theme->header_height * 0.8;
	m5.lcd.fillRoundRect(left_offset, top, ez.theme->battery_bar_width, height, ez.theme->battery_bar_gap, ez.theme->header_bgcolor);
	if (_isCharging()) {
		m5.lcd.drawRoundRect(left_offset, top, ez.theme->battery_bar_width, height, ez.theme->battery_bar_gap, TFT_RED);
	} else {
		m5.lcd.drawRoundRect(left_offset, top, ez.theme->battery_bar_width, height, ez.theme->battery_bar_gap, ez.theme->header_fgcolor);
	}
	uint8_t bar_width = (ez.theme->battery_bar_width - ez.theme->battery_bar_gap * 9) / 8.0;
	uint8_t bar_height = height - ez.theme->battery_bar_gap * 2;
	left_offset += ez.theme->battery_bar_gap;
	for (uint8_t n = 0; n < (_numChargingBars != BATTERY_CHARGING_OFF ? _numChargingBars : currentBatteryLevel); n++) {
		m5.lcd.fillRect(left_offset + n * (bar_width + ez.theme->battery_bar_gap), top + ez.theme->battery_bar_gap, 
			bar_width, bar_height, _getBatteryBarColor(currentBatteryLevel));
	}
}

bool ezBattery::_isChargeControl() {
	#if defined (ARDUINO_M5Stack_Core_ESP32)
		//Serial.print("Can control "); Serial.println(m5.Power.canControl(), DEC);
		return m5.Power.canControl();
	#elif defined (ARDUINO_M5STACK_Core2)
		return false;	// charging is automatic
	#elif defined (ARDUINO_M5Stick_C_Plus)
		return false;	// charging is automatic
	#elif defined (ARDUINO_M5Stick_C)
		return false;	// charging is automatic
	#elif defined (ARDUINO_ESP32_DEV)
		return true;
	#elif defined (ARDUINO_FROG_ESP32) || defined (ARDUINO_WESP32)	//K46v4 || K46v1
		return false;	// charging is automatic
	#else
		return false;	//placeholder for your device method
	#endif
}

void ezBattery::_adaptChargeMode() {
	// If power management not available, ignore the routine
	if(!_canControl) {
		return;
	}
	// Disable the charging if the battery is fully charged
	if(_isChargeFull()) {
		_setCharge(false);
	} else if (_getBatteryLevel() < 76) {
		_setCharge(true);
	}
	// Define the shutdown time at 64s
	_setLowPowerShutdownTime();
}

void ezBattery::_setCharge(bool enable) {
	#if defined (ARDUINO_M5Stack_Core_ESP32)
		m5.Power.setCharge(enable);
	#elif defined (ARDUINO_M5STACK_Core2)
		;	// can be done using bit 7 of REG 0x33
	#elif defined (ARDUINO_M5Stick_C_Plus)
		;	// can be done using bit 7 of REG 0x33
	#elif defined (ARDUINO_M5Stick_C)
		;	// can be done using bit 7 of REG 0x33
	#elif defined (ARDUINO_ESP32_DEV)
		;	//placeholder for your device method
	#elif defined (ARDUINO_FROG_ESP32) || defined (ARDUINO_WESP32)	//K46v4 || K46v1
		;
	#else
		;	//placeholder for your device method
	#endif
}

void ezBattery::_setLowPowerShutdownTime() {
	#if defined (ARDUINO_M5Stack_Core_ESP32)
		m5.Power.setLowPowerShutdownTime(M5.Power.ShutdownTime::SHUTDOWN_64S);
	#elif defined (ARDUINO_M5STACK_Core2)
		;	//placeholder for your device method
	#elif defined (ARDUINO_M5Stick_C_Plus)
		;	//placeholder for your device method
	#elif defined (ARDUINO_M5Stick_C)
		;	//placeholder for your device method
	#elif defined (ARDUINO_ESP32_DEV)
		;	//placeholder for your device method
	#elif defined (ARDUINO_FROG_ESP32) || defined (ARDUINO_WESP32)	//K46v4 || K46v1
		;
	#else
		;	//placeholder for your device method
	#endif
}

uint8_t ezBattery::_getBatteryLevel() {
	#if defined (ARDUINO_M5Stack_Core_ESP32)
		return m5.Power.getBatteryLevel();
	#elif defined (ARDUINO_M5STACK_Core2) || defined (ARDUINO_M5Stick_C_Plus) || defined (ARDUINO_M5Stick_C)
		float vBat = m5.Axp.GetBatVoltage();
		if(vBat >= 4.17f ) return 100;
		if(vBat >= 4.1f )  return 90;
		if(vBat >= 4.0f )  return 80;
		if(vBat >= 3.9f )  return 60;
		if(vBat >= 3.8f )  return 40;
		if(vBat >= 3.75f ) return 30;
		if(vBat >= 3.7f )  return 20;
		if(vBat >= 3.65f ) return 13;
		return 0;
	#elif defined (ARDUINO_ESP32_DEV)
		return 50;	//placeholder for your device method
	#elif defined (ARDUINO_FROG_ESP32) || defined (ARDUINO_WESP32)	//K46v4 || K46v1
		return m5.Bat.getBatteryLevel();
	#else
		return 50;	//placeholder for your device method
	#endif
}

bool ezBattery::_isChargeFull() {
	#if defined (ARDUINO_M5Stack_Core_ESP32)
		return m5.Power.isChargeFull();
	#elif defined (ARDUINO_M5STACK_Core2)
		return (m5.Axp.GetBatVoltage() >= 4.17f ? true : false);
	#elif defined (ARDUINO_M5Stick_C_Plus)
		return (m5.Axp.GetBatVoltage() >= 4.17f ? true : false);
	#elif defined (ARDUINO_M5Stick_C)
		return (m5.Axp.GetBatVoltage() >= 4.17f ? true : false);
	#elif defined (ARDUINO_ESP32_DEV)
		return false;	//placeholder for your device method
	#elif defined (ARDUINO_FROG_ESP32) || defined (ARDUINO_WESP32)	//K46v4 || K46v1
		return m5.Bat.isChargeFull();
	#else
		return false;	//placeholder for your device method
	#endif		
}

bool ezBattery::_isCharging() {
	#if defined (ARDUINO_M5Stack_Core_ESP32)
		return m5.Power.isCharging();
	#elif defined (ARDUINO_M5STACK_Core2)
		return m5.Axp.isCharging();
	#elif defined (ARDUINO_M5Stick_C_Plus) || defined (ARDUINO_M5Stick_C)
		// No builtin method - can be done this way?
		// uint32_t coulombNow = m5.Axp.GetCoulombchargeData();
		// if(coulombNow > _batPrevCoulomb){
		// 	_batPrevCoulomb = coulombNow;
		// 	return true;
		// }			
	#elif defined (ARDUINO_ESP32_DEV)
		return false;	//placeholder for your device method
	#elif defined (ARDUINO_FROG_ESP32) || defined (ARDUINO_WESP32)	//K46v4 || K46v1
		return m5.Bat.isCharging();
	#else
		return false;
	#endif
}

//Transform the M5Stack built in battery level into an internal format.
// From [100, 75, 50, 25, 0] to [8, 6, 4, 2, 0]
uint8_t ezBattery::_getTransformedBatteryLevel()
{
	return (uint8_t)((float)_getBatteryLevel() / 12.5);
}

//Return the theme based battery bar color according to its level
uint32_t ezBattery::_getBatteryBarColor(uint8_t batteryLevel)
{
	switch (batteryLevel) {
		case 0:
		case 1:
			return ez.theme->battery_0_fgcolor;				
		case 2:
		case 3:
			return ez.theme->battery_25_fgcolor;				
		case 4:
		case 5:
			return ez.theme->battery_50_fgcolor;				
		case 6:
		case 7:
			return ez.theme->battery_75_fgcolor;				
		case 8:
			return ez.theme->battery_100_fgcolor;
		default:
			return ez.theme->header_fgcolor;	
	}
}
