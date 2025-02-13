#include <Preferences.h>
#include "../../M5ez.h"
#include "ezClock.h"


Timezone ezClock::tz;
bool ezClock::_on;
bool ezClock::_posix_on;
String ezClock::_timezone;
String ezClock::_posix;
bool ezClock::_clock12;
bool ezClock::_am_pm;
bool ezClock::_date_on;
String ezClock::_datetime;
bool ezClock::_starting = true;

bool ezClock::entry(uint8_t command, void* /* user */) {
	switch(command) {
		case FEATURE_MSG_PING:
			return true;
		case FEATURE_MSG_START:
			begin();
			return true;
		case FEATURE_MSG_STOP:
			_on = false;
			return true;
		case FEATURE_MSG_QUERY_ENABLED:
			return _on;
		case FEATURE_MSG_CLOCK_EVENTS:
			events();	// ezTime.events(), updated from M5ez::yield()
			return true;
	}
	return false;
}

void ezClock::begin() {
	Preferences prefs;
	prefs.begin("M5ez", true);	// read-only
	_on = prefs.getBool("clock_on", true);
	_posix_on = prefs.getBool("posix_on", true);
	_posix = prefs.getString("posix", TZ_POSIX);
	_timezone = prefs.getString("timezone", "GeoIP");
	_date_on = prefs.getBool("date_on", true);
	_clock12 = prefs.getBool("clock12", false);
	_am_pm = prefs.getBool("ampm", false);
	prefs.end();
	ez.settings.menuObj.addItem("Clock settings", ezClock::menu);
	ez.addEvent(ezClock::loop);
	restart();
}

void ezClock::restart() {
	ez.header.remove("clock");
	uint8_t length;
	if (_on) {
		if (_clock12) {
			if (_am_pm) {
				_datetime = _date_on ? "m/d/Y g:ia" : "g:ia";
				length = _date_on ? 19 : 8;
			} else {
				_datetime = _date_on ? "d.m.y g:i" : "g:i";
				length = _date_on ? 15 : 6;
			}
		} else {
			_datetime = _date_on ? "d.m.y H:i" : "H:i";
			length = _date_on ? 15 : 6;
		}
		ez.setFont(ez.theme->clock_font);
		uint8_t width = length * m5.lcd.textWidth("5") + ez.theme->header_hmargin * 2;
		ez.header.insert(ez.header.position("battery"), "clock", width, ezClock::draw);
	}
}

void ezClock::menu() {
	bool on_orig = _on;
	bool posix_on_orig = _posix_on;
	bool date_on_orig = _date_on;
	bool clock12_orig = _clock12;
	bool am_pm_orig = _am_pm;
	String tz_orig = _timezone;
	String tz_posix = _posix;
	while(true) {
		ezMenu clockmenu("Clock Settings");
		clockmenu.txtSmall();
		clockmenu.buttons("up#Back#select##down#");
		clockmenu.addItem("on|Display clock\t" + (String)(_on ? "on" : "off"));
		if (_on) {
			clockmenu.addItem("tzfrom|Timezone from\t" + (String)(_posix_on ? "POSIX" : "Network"));
			clockmenu.addItem("tz|Timezone\t" + _timezone);
			clockmenu.addItem("posix|Timezone POSIX \t" + _posix);
			clockmenu.addItem("date|Show date\t" + (String)(_date_on ? "on" : "off"));
			clockmenu.addItem("1224|12/24 hour\t" + (String)(_clock12 ? "12" : "24"));
			if (_clock12) {
				clockmenu.addItem("ampm|am/pm indicator\t" + (String)(_am_pm ? "on" : "off"));
			}
		}
		switch (clockmenu.runOnce()) {
			case 1:
				_on = !_on;
				ezClock::restart();
				break;
			case 2:
				_posix_on = !_posix_on;
				ezClock::restart();
				break;
			case 3:
				_timezone = ez.textInput("Enter timezone");
				if (_timezone == "") _timezone = "GeoIP";
#ifdef EZTIME_NETWORK_ENABLE
				if (tz.setLocation(_timezone)) _timezone = tz.getOlsen();
#endif  // EZTIME_NETWORK_ENABLE
				break;
			case 4:
				_posix = ez.textInput("Enter POSIX");
				if (_posix == "") _posix = TZ_POSIX;
				ezClock::restart();
				break;
			case 5:
				_date_on = !_date_on;
				ezClock::restart();
				break;
			case 6:
				_clock12 = !_clock12;
				ezClock::restart();
				break;
			case 7:
				_am_pm = !_am_pm;
				ezClock::restart();
				break;
			case 0:
				if (_am_pm != am_pm_orig || _clock12 != clock12_orig || _on != on_orig || _posix_on != posix_on_orig || _timezone != tz_orig || _posix != tz_posix || _date_on != date_on_orig) {
					_writePrefs();
				}
				return;
		}
	}
}

uint32_t ezClock::loop() {
	ezt::events();
	if (_starting && timeStatus() != timeNotSet) {
		_starting = false;
		if(_posix_on) {
			tz.setPosix(_posix);
		} else {
#ifdef EZTIME_NETWORK_ENABLE
			if (tz.setLocation(_timezone)) {
				if (tz.getOlsen() != _timezone) {
					_timezone = tz.getOlsen();
					_writePrefs();
				}
			}
#endif  // EZTIME_NETWORK_ENABLE
		}
		ez.header.draw("clock");
	} else {
		if (_on && ezt::minuteChanged()) ez.header.draw("clock");
	}
	return 250000;	//250ms
}

void ezClock::draw(uint16_t x, uint16_t w) {
	if (_starting) return;
	m5.lcd.fillRect(x, 0, w, ez.theme->header_height, ez.theme->header_bgcolor);
	ez.setFont(ez.theme->clock_font);
	m5.lcd.setTextColor(ez.theme->header_fgcolor);
	m5.lcd.setTextDatum(TL_DATUM);
	m5.lcd.drawString(tz.dateTime(_datetime), x + ez.theme->header_hmargin, ez.theme->header_tmargin + 2);
}

void ezClock::_writePrefs() {
	Preferences prefs;
	prefs.begin("M5ez");
	prefs.putBool("clock_on", _on);
	prefs.putBool("posix_on", _posix_on);
	prefs.putString("posix", _posix);
	prefs.putString("timezone", _timezone);
	prefs.putBool("date_on", _date_on);
	prefs.putBool("clock12", _clock12);
	prefs.putBool("ampm", _am_pm);
	prefs.end();
}

bool ezClock::waitForSync(const uint16_t timeout /* = 0 */) {
	unsigned long start = millis();
	ez.msgBox("Clock sync", "Waiting for clock synchronisation", "", false);
	while (ezt::timeStatus() != timeSet) {
		if ( timeout && (millis() - start) / 1000 > timeout ) return false;
		delay(25);
		ez.yield();
	}
	return true;
}
