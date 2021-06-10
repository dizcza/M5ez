#pragma once

// Coupling:
// Indicate activity to the ezBacklight feature by calling ez.tell("ezBacklight", FEATURE_MSG_PING, nullptr)

class ezBacklight {
	public:
		static bool entry(uint8_t command, void* user);
		static void begin();
		static void menu();
		static void inactivity(uint8_t half_minutes);
		static void activity();
		static uint32_t loop();
		static void defaults();
		static uint8_t getInactivity();
		static void setLcdBrightness(uint8_t brightness);
		static void setBtnBrightness(uint8_t brightness);
		static void wakeup();
	private:
		static uint8_t _lcd_brightness;
		static uint8_t _btn_brightness;
		static uint8_t _inactivity;
		static uint32_t _last_activity;
		static bool _backlight_off;
		static uint32_t _BtnA_LastChg;
		static uint32_t _BtnB_LastChg;
		static uint32_t _BtnC_LastChg;
};
