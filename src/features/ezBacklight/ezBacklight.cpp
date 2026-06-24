#include <Preferences.h>
#include "../../M5ez.h"
#include "ezBacklight.h"

#define NEVER		0
#define USER_SET	255
#define BTN_PWM_CHANNEL 6 // LEDC_CHANNEL_6

uint8_t ezBacklight::_lcd_brightness;
uint8_t ezBacklight::_btn_brightness;
uint8_t ezBacklight::_inactivity;
uint32_t ezBacklight::_last_activity;
bool ezBacklight::_backlight_off = false;


bool ezBacklight::entry(uint8_t command, void* /* user */) {
	switch(command) {
		case FEATURE_MSG_PING:
			activity();
			return true;
		case FEATURE_MSG_START:
			begin();
			return true;
	}
	return false;
}

void ezBacklight::begin() {
	ez.addEvent(ezBacklight::loop);
	ez.settings.menuObj.addItem("Backlight settings", ezBacklight::menu);
	Preferences prefs;
	prefs.begin("M5ez", true);	// read-only
	_lcd_brightness = prefs.getUChar("lcd_brightness", ez.theme->lcd_brightness_default);
	_btn_brightness = prefs.getUChar("btn_brightness", ez.theme->btn_brightness_default);
	_inactivity = prefs.getUChar("inactivity", 15);
	prefs.end();
	ez.backlight.inactivity(_inactivity);
	setLcdBrightness(_lcd_brightness);
	setBtnBrightness(_btn_brightness);
}

void ezBacklight::menu() {
	uint8_t start_lcd_brightness = _lcd_brightness;
	uint8_t start_btn_brightness = _btn_brightness;
	uint8_t start_inactivity = _inactivity;
	ezMenu blmenu("ЯСКР");
	blmenu.txtSmall();
	blmenu.buttons("up # Back|ВИЙТИ # select|ОБРАТИ # # down # ");
	blmenu.addItem("timeout | Таймаут бездії\t"  + (String)(_inactivity == NEVER ? "ВИМКН" : (String)(_inactivity) + "сек"));
	blmenu.addItem("bltft | Підсвітка екрану\t" + (_lcd_brightness > 0) ? "ТАК" : "НІ");
	while(true) {
		switch (blmenu.runOnce()) {
			case 1:
				{
					if (_inactivity >= 90) {
						_inactivity = 0;
					} else {
						_inactivity += 15;
					}
					blmenu.setCaption("timeout", "Тайм-аут бездії\t" + (String)(_inactivity == NEVER ? "ВИМКН" : (String)(_inactivity) + "сек"));
				}
				break;
			case 2:
				{
					ezProgressBar lcdbl ("LCD", "Підсвітка екрану", "left#OK#right");
					while (true) {
						String b = ez.buttons.poll();
						if (b == "right") _lcd_brightness = 1;
						if (b == "left") _lcd_brightness = 0;
						lcdbl.value(_lcd_brightness > 0 ? 100 : 0);
						setLcdBrightness(_lcd_brightness);
						if (b == "OK") {
							blmenu.setCaption("bltft", "Підсвітка екрану\t" + (_lcd_brightness > 0) ? "ТАК" : "НІ");
							break;
						}
					}
				}
				break;
			case 0:
				if (_lcd_brightness != start_lcd_brightness || _btn_brightness != start_btn_brightness || _inactivity != start_inactivity) {
					Preferences prefs;
					prefs.begin("M5ez");
					prefs.putUChar("lcd_brightness", _lcd_brightness);
					prefs.putUChar("btn_brightness", _btn_brightness);
					prefs.putUChar("inactivity", _inactivity);
					prefs.end();
				}
				return;
			//
		}
	}
}

void ezBacklight::inactivity(uint8_t half_minutes) {
	if (half_minutes == USER_SET) {
		Preferences prefs;
		prefs.begin("M5ez", true);
		_inactivity = prefs.getUShort("inactivity", 15);
		prefs.end();
	} else {
		_inactivity = half_minutes;
	}
}

void ezBacklight::activity() {
	_last_activity = millis();
}

uint32_t ezBacklight::loop() {
	if (!_backlight_off && _inactivity) {
		if (millis() > _last_activity + 1000 * _inactivity) {
			_backlight_off = true;	//set it here to turn LDO off on brightness==0
			setBtnBrightness(0);
			setLcdBrightness(0);
			M5.Lcd.writecommand(TFT_DISPOFF);
			M5.Lcd.writecommand(TFT_SLPIN);
			ez.yield();
		}
	}
	ez.yield();
	return 1000000;	//1s
}
	
void ezBacklight::defaults() {
	_lcd_brightness = ez.theme->lcd_brightness_default;
	_btn_brightness = ez.theme->btn_brightness_default;
	setLcdBrightness(_lcd_brightness);
	setBtnBrightness(_btn_brightness);

	Preferences prefs;
	prefs.begin("M5ez");
	prefs.putUChar("lcd_brightness", _lcd_brightness);
	prefs.putUChar("btn_brightness", _btn_brightness);
	prefs.end();	
	_backlight_off = false;	
	_last_activity = millis();
}

uint8_t ezBacklight::getInactivity(){
	return _inactivity;
}

bool ezBacklight::getBacklightOff(){
	return _backlight_off;
}

void ezBacklight::setLcdBrightness(uint8_t brightness) {

}

void ezBacklight::setBtnBrightness(uint8_t btnBrightness) {
}

void ezBacklight::wakeup() {
	if(_backlight_off){
		M5.Lcd.writecommand(TFT_DISPON);
		M5.Lcd.writecommand(TFT_SLPOUT);
		setBtnBrightness(_btn_brightness);
		setLcdBrightness(_lcd_brightness);
		_backlight_off = false;
	}
	_last_activity = millis();
}
