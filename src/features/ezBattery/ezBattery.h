#pragma once

class ezBattery {
	public:
		static bool entry(uint8_t command, void* user);
		static void begin();
		static void menu();
		static uint32_t loop();
	private:
		static void _readFlash();
		static void _writeFlash();
		static void _adaptChargeMode();
		static bool _isChargeControl();
		static bool _isCharging();
		static bool _isChargeFull();
		static uint8_t _getBatteryLevel();
		static uint8_t _getTransformedBatteryLevel();
		static uint32_t _getBatteryBarColor(uint8_t batteryLevel);
		static void _setCharge(bool enable);
		static void _setLowPowerShutdownTime();
		static bool _on;
		static void _refresh();
		static void _drawWidget(uint16_t x, uint16_t w);
		static uint8_t _numChargingBars;
		static bool _canControl;
};
