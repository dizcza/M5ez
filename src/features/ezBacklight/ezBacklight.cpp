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
uint32_t ezBacklight::_BtnA_LastChg = 0;
uint32_t ezBacklight::_BtnB_LastChg = 0;
uint32_t ezBacklight::_BtnC_LastChg = 0;


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
	#if defined (ARDUINO_Piranha)	//K46
	    // Set up buttons back-light LED
		pinMode(BTN_BL, OUTPUT);
		digitalWrite(BTN_BL, HIGH);
		// Init the buttons back-light LED PWM
		ledcSetup(BTN_PWM_CHANNEL, 44100, 8);
		ledcAttachPin(BTN_BL, BTN_PWM_CHANNEL);
	#endif
	setBtnBrightness(_btn_brightness);
}

void ezBacklight::menu() {
	uint8_t start_lcd_brightness = _lcd_brightness;
	uint8_t start_btn_brightness = _btn_brightness;
	uint8_t start_inactivity = _inactivity;
	#if defined (UKRAINIAN)
		ezMenu blmenu("ЯСКР");
	#else
		ezMenu blmenu("Backlight settings");
	#endif
	blmenu.txtSmall();
	#if defined (ARDUINO_M5Stick_C)
		blmenu.buttons("##select##down#");
		blmenu.addItem("timeout | Inactivity timeout\t"  + (String)(_inactivity == NEVER ? "OFF" : (String)(_inactivity) + "s"));
		blmenu.addItem("bltft | Screen brightness\t" + (String)((uint16_t)_lcd_brightness * 10) + "%");
	#elif defined (UKRAINIAN)
		blmenu.buttons("up # Back|ВИЙТИ # select|ОБРАТИ # # down # ");
		blmenu.addItem("timeout | Таймаут бездії\t"  + (String)(_inactivity == NEVER ? "ВИМКН" : (String)(_inactivity) + "сек"));
		blmenu.addItem("bltft | Яскравість екрану\t" + (String)((uint16_t)_lcd_brightness * 10) + "%");
		#if defined (ARDUINO_ESP32_DEV) || defined (ARDUINO_D1_MINI32) || defined (ARDUINO_Piranha) //M35 or K36 or K46
		blmenu.addItem("blkbd | Яскравість кнопок\t" + (String)((uint16_t)(15 - _btn_brightness) * 10) + "%");
		#endif
	#else
		blmenu.buttons("up#Back#select##down#");
		blmenu.addItem("timeout | Inactivity timeout\t"  + (String)(_inactivity == NEVER ? "OFF" : (String)(_inactivity) + "s"));
		blmenu.addItem("bltft | Screen brightness\t" + (String)((uint16_t)_lcd_brightness * 10) + "%");
		#if defined (ARDUINO_ESP32_DEV) || defined (ARDUINO_D1_MINI32) || defined (ARDUINO_Piranha) //M35 or K36 or K46
		blmenu.addItem("blkbd | Buttons brightness\t" + (String)((uint16_t)(15 - _btn_brightness) * 10) + "%");
		#endif
	#endif
	while(true) {
		switch (blmenu.runOnce()) {
			case 1:
				{
					if (_inactivity >= 90) {
						_inactivity = 0;
					} else {
						_inactivity += 15;
					}
				#if defined (UKRAINIAN)
					blmenu.setCaption("timeout", "Тайм-аут бездії\t" + (String)(_inactivity == NEVER ? "ВИМКН" : (String)(_inactivity) + "сек"));
				#else
					blmenu.setCaption("timeout", "Inactivity timeout\t" + (String)(_inactivity == NEVER ? "OFF" : (String)(_inactivity) + "s"));
				#endif
				}
				break;
			case 2:	
				{
					#if defined (ARDUINO_M5Stick_C)
						ezProgressBar lcdbl ("LCD", "Screen brightness", " ##left##right#");
					#elif defined (UKRAINIAN)
						ezProgressBar lcdbl ("LCD", "Яскравість екрану", "left#OK#right");
					#else
						ezProgressBar lcdbl ("LCD", "Screen brightness", "left#OK#right");
					#endif
					while (true) {
						String b = ez.buttons.poll();
						if (b == "right" && _lcd_brightness < 0xA) _lcd_brightness++;
						if (_lcd_brightness > 0xA) _lcd_brightness = 0xA;
						if (b == "left"  && _lcd_brightness > 0x1) _lcd_brightness--;
						if (_lcd_brightness > 0xA) _lcd_brightness = 0;
						lcdbl.value((float)_lcd_brightness * 10);
						setLcdBrightness(_lcd_brightness);
						if (b == "OK") {
							#if defined (ARDUINO_M5Stick_C)
								blmenu.setCaption("bltft", "Screen brightness\t" + (String)((uint16_t)_lcd_brightness * 10) + "%");
							#elif defined (UKRAINIAN)
								blmenu.setCaption("bltft", "Яскравість екрану\t" + (String)((uint16_t)_lcd_brightness * 10) + "%");
							#else
								blmenu.setCaption("bltft", "Screen brightness\t" + (String)((uint16_t)_lcd_brightness * 10) + "%");
							#endif
							break;
						}
					}
				}
				break;
			#if defined (ARDUINO_Piranha) //K46
			case 3:
				{
					#if defined (UKRAINIAN)
						ezProgressBar kbdbl ("KBD", "Яскравість кнопок", "left#OK#right");
					#else
						ezProgressBar kbdbl ("KBD", "Buttons brightness", "left#OK#right");
					#endif
					while (true) {
						String b = ez.buttons.poll();
						if (b == "right" && _btn_brightness < 0xA) _btn_brightness++;
						if (b == "left"  && _btn_brightness > 0x0) _btn_brightness--;
						if (_btn_brightness > 0xA) _btn_brightness = 0;
						kbdbl.value((float)(_btn_brightness) * 10);
						setBtnBrightness(_btn_brightness);
						if (b == "OK"){
							#if defined (UKRAINIAN)
								blmenu.setCaption("blkbd", "Яскравість кнопок\t" + (String)((uint16_t)_btn_brightness * 10) + "%");
							#else
								blmenu.setCaption("blkbd", "Buttons brightness\t" + (String)((uint16_t)_btn_brightness * 10) + "%");
							#endif
							
							break;
						}
					}
				}
				break;
			#elif defined (ARDUINO_ESP32_DEV) || defined (ARDUINO_D1_MINI32) //M35 or K36
			case 3:
				{
					ezProgressBar kbdbl ("KBD", "Buttons brightness", "left#OK#right");
					while (true) {
						String b = ez.buttons.poll();
						if (b == "right" && _btn_brightness > 0xB) _btn_brightness--;
						if (b == "left"  && _btn_brightness < 0xF) _btn_brightness++;
						kbdbl.value((float)(0xF - _btn_brightness) * 10);
						setBtnBrightness(_btn_brightness);
						if (b == "OK"){
							blmenu.setCaption("blkbd", "Buttons brightness\t" + (String)((uint16_t)(15 - _btn_brightness) * 10) + "%");
							break;
						}
					}
				}
				break;
			#endif
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
		_inactivity = prefs.getUShort("inactivity", 0);
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
			m5.Lcd.writecommand(TFT_DISPOFF);
			m5.Lcd.writecommand(TFT_SLPIN);
			ez.yield();
			_BtnA_LastChg = M5.BtnA.lastChange();
			_BtnB_LastChg = M5.BtnB.lastChange();
			_BtnC_LastChg = M5.BtnC.lastChange();
		}
	}
	if (_backlight_off) {
		ez.yield();
		if (_BtnA_LastChg != M5.BtnA.lastChange() || _BtnB_LastChg != M5.BtnB.lastChange() || _BtnC_LastChg != M5.BtnC.lastChange()) {
			m5.Lcd.writecommand(TFT_SLPOUT);
			m5.Lcd.writecommand(TFT_DISPON);
			setLcdBrightness(_lcd_brightness);
			setBtnBrightness(_btn_brightness);
			activity();
			_backlight_off = false;	//set it here to turn LDO on in setLcdBrightness()
		}
	}
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
	activity();	
}

uint8_t ezBacklight::getInactivity(){
	return _inactivity;
}

#if defined (ARDUINO_M5Stack_Core_ESP32) || defined (ARDUINO_M5STACK_FIRE) || defined (ARDUINO_LOLIN_D32_PRO) || defined (ARDUINO_Piranha) //TTGO T4 v1.3, K46
	void ezBacklight::setLcdBrightness(uint8_t lcdBrightness) { m5.Lcd.setBrightness((uint8_t)(lcdBrightness * 2.55)); }
#elif defined (ARDUINO_M5Stick_C)
	void ezBacklight::setLcdBrightness(uint8_t lcdBrightness) {
		uint8_t brightness = (lcdBrightness * 10) / 13;
		m5.Axp.ScreenBreath(brightness + 7);
		if(brightness == 0){
			if(_backlight_off) {
				m5.Axp.SetLDO2(false);	//turn LCD_BL LDO completely off to save battery
				return;
			}
		} else if(_backlight_off) {
			m5.Axp.SetLDO2(true);	//turn LCD_BL LDO on
		}
	}
#elif defined (ARDUINO_M5STACK_Core2)
	void ezBacklight::setLcdBrightness(uint8_t lcdBrightness) { m5.Axp.SetLcdVoltage(lcdBrightness * 80 + 2500); }
#elif defined (ARDUINO_ESP32_DEV) || defined (ARDUINO_D1_MINI32)	//M35, K36 under M5StX only
	void ezBacklight::setLcdBrightness(uint8_t lcdBrightness) { m5.Ioe.setLcdBrightness(lcdBrightness, LOW); }
#endif


#if defined (_M5STX_CORE_)
	#if defined (ARDUINO_ESP32_DEV) || defined (ARDUINO_D1_MINI32)	//M35, K36 under M5StX only
		void ezBacklight::setBtnBrightness(uint8_t btnBrightness) { m5.Ioe.setBtnBrightness(btnBrightness, HIGH); }
	#elif defined (ARDUINO_Piranha)
		void ezBacklight::setBtnBrightness(uint8_t btnBrightness) { 
			ledcWrite(BTN_PWM_CHANNEL, btnBrightness * 2.55); // brightness 0-255
		}
	#else
		void ezBacklight::setBtnBrightness(uint8_t btnBrightness) {}
	#endif
#else
	void ezBacklight::setBtnBrightness(uint8_t btnBrightness) {}
#endif

void ezBacklight::wakeup() {
	if(_backlight_off){
		m5.Lcd.writecommand(TFT_DISPON);
		m5.Lcd.writecommand(TFT_SLPOUT);
		setBtnBrightness(_btn_brightness);
		setLcdBrightness(_lcd_brightness);
		_backlight_off = false;
	}
	activity();
}
