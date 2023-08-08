//https://github.com/UT2UH/M5Stack/tree/K46_FROG
//https://github.com/UT2UH/M5ez/tree/M5ez2
//https://github.com/UT2UH/ezTime/tree/K46
//https://github.com/sparkfun/SparkFun_u-blox_GNSS_Arduino_Library
#include <M5ez.h>
#include <ezTime.h>
#include "images.h"
#include <driver/adc.h>
#include <driver/rtc_io.h>
#include <Preferences.h>
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>
#include <RadioLib.h>

#define I2C_DEV_NUM     5 
#define PIN_IRQ_RTC     34
#define PIN_KEEP_PWR    23
#define GPIO_KEEP_PWR   GPIO_NUM_23
#define PIN_EXTINT      32

#define K46_V4
// Only one permitted for the module installed
//#define USE_RFM95
//#define USE_RFM98
//#define USE_LLCC68
//#define USE_SX1262
//#define USE_SX1268
#define USE_LORA127XF30
//#define USE_LORA126XF30
//#define USE_R68837DHAV2

//#define BAND_UHF
//#define BAND_915

#if defined (USE_RFM95)
  #define RADIO_INT   36
  #define RADIO_BUSY  39
  #define MAX_POWER   22
  #define RADIO_TYPE SX1276
  #ifdef BAND_UHF
    #undef BAND_UHF
  #endif
#elif defined (USE_RFM98)
  #define RADIO_INT   36
  #define RADIO_BUSY  39
  #define MAX_POWER   22
  #define RADIO_TYPE SX1278
  #ifndef BAND_UHF
    #define BAND_UHF
  #endif
#elif defined (USE_LORA127XF30)
  #define RADIO_PWR_EN  12
  #define RADIO_INT     36
  #define RADIO_BUSY    39
  #define MAX_POWER     13
  #if defined (BAND_UHF)
    #define RADIO_TYPE SX1278
  #else
    #define RADIO_TYPE SX1276
  #endif
#elif defined (USE_LLCC68)
  #define RADIO_INT   36
  #define RADIO_BUSY  39
  #define MAX_POWER   22
  #if defined (BAND_UHF)
    #define RADIO_TYPE SX1268
  #else
    #define RADIO_TYPE SX1262
  #endif
#elif defined (USE_SX1262) || defined (USE_SX1268)
  #define RADIO_INT   36
  #define RADIO_BUSY  39
  #define MAX_POWER   22
  #if defined (BAND_UHF)
    #define RADIO_TYPE SX1268
  #else
    #define RADIO_TYPE SX1262
  #endif
#elif defined (USE_LORA126XF30)
  #define RADIO_PWR_EN  12
  #define RADIO_INT     39
  #define RADIO_BUSY    36
  #define MAX_POWER     13 // Set lora.tx_power to 13 due to PA
  #if defined (BAND_UHF)
    #define RADIO_TYPE SX1268
  #else
    #define RADIO_TYPE SX1262
  #endif
#elif defined (USE_R68837DHAV2)
  #undef  BAND_915
  #ifndef BAND_UHF
    #define BAND_UHF
  #endif
  #define USE_SX1268
  #define SX126X_R6822DH
  #define RADIO_PWR_EN  12
  #define RADIO_INT     39
  #define RADIO_BUSY    36
  #define RADIO_RXEN     0
  #define RADIO_TXEN    -1
  #define E22_TXEN_CONNECTED_TO_DIO2 1
  #define MAX_POWER    (13) // Set lora.tx_power to 13 due to PA
  #define RADIO_TYPE SX1268
#endif

#if defined BAND_UHF
  #define K46_Pout (17)      // 19dBm is (unstable) max for RF98
  float K46_Freq = 449.9375;
  float K46_AntBW = 0.5;
  RTC_DATA_ATTR float _freqOffset  = 0.0;
#elif defined BAND_915
  #define K46_Pout (20)      // TODO set to 13dBm for debug, 23 dBm for production
  float K46_Freq = 917.719;
  float K46_AntBW = 0.5;
  RTC_DATA_ATTR float _freqOffset  = 0.0;
#else
  #define K46_Pout (20)      // TODO set to 13dBm for debug, 23 dBm for production
  float K46_Freq = 868.300;
  float K46_AntBW = 1.0;
  RTC_DATA_ATTR float _freqOffset  = 0.0;
#endif

  RADIO_TYPE radio = new Module(RADIO_NSS, RADIO_INT, RADIO_RST, RADIO_BUSY, SPI);

#define USER_SET  255

RTC_DATA_ATTR bool _hasRFM = false;
RTC_DATA_ATTR bool _hasMAG = false;
RTC_DATA_ATTR bool _hasAK  = false;
RTC_DATA_ATTR bool _hasQMI = false;
RTC_DATA_ATTR bool _hasGPS = false;
RTC_DATA_ATTR bool _hasRFT = false;
RTC_DATA_ATTR bool _hasNTC = false;
RTC_DATA_ATTR bool _hasADC = false;
RTC_DATA_ATTR bool _hasRTC = false;
RTC_DATA_ATTR bool _hasBM  = false;
RTC_DATA_ATTR bool _hasRV  = false;
RTC_DATA_ATTR bool _hasDS  = false;
RTC_DATA_ATTR bool _hasX68 = false;
RTC_DATA_ATTR bool _hasIMU = false;
RTC_DATA_ATTR bool _hasIP  = false;
RTC_DATA_ATTR bool _hasQMC = false;



/* The string contains the names for your TZ in standard and Daylight Saving Time, 
 * as well as the starting and ending point for DST and the offset to UTC:
 * в останню неділю березня о 3 годині на 1 годину вперед
 * в останню неділю жовтня  о 4 годині на 1 годину назад
 */
#define LOCALTZ_POSIX  "EET-2EEST,M3.5.0/3,M10.5.0/4"    // Time in Kyiv
Timezone local;
tmElements_t tm;
time_t t;

#define MAIN_DECLARED

void disableLcdBl(void) {
  ez.backlight.setLcdBrightness(0);
  M5.Lcd.writecommand(0x28);  // ILI9341_DISPOFF
  M5.Lcd.writecommand(0x10);  // ILI9341_SLPIN
}

void enableLcdBl(void) {
  ez.backlight.setLcdBrightness(80);
}

void disableKbdBl(void) {
  ez.backlight.setBtnBrightness(0);
}

void enableKbdBl(void) {
  ez.backlight.setBtnBrightness(80);
}

#define GPIO_OUTPUT_PIN_SEL   (1ULL<<PIN_KEEP_PWR)

static void disablePwrBtnDeepSleep() {
  if(_hasIP) return;
  gpio_config_t io_conf;
  io_conf.intr_type = GPIO_INTR_DISABLE ;
  io_conf.mode = GPIO_MODE_OUTPUT;
  //bit mask of the pins that you want to set,e.g.GPIO23
  io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
  gpio_config(&io_conf);
  gpio_set_direction(GPIO_KEEP_PWR, GPIO_MODE_OUTPUT);
  gpio_set_level(GPIO_KEEP_PWR, HIGH);
  gpio_hold_en(GPIO_KEEP_PWR);
  gpio_deep_sleep_hold_en();
}

static void disablePwrBtn() {
  if(_hasIP) return;
  pinMode(PIN_KEEP_PWR, OUTPUT);
  digitalWrite(PIN_KEEP_PWR, HIGH);
}

static void enablePwrBtn() {
  if(_hasIP) return;
  pinMode(PIN_KEEP_PWR, OUTPUT);
  digitalWrite(PIN_KEEP_PWR, LOW);
}

//********** GNSS *************************//
// Create storage for the time pulse parameters
UBX_CFG_TP5_data_t timePulseParameters;

#define MAX_PAYLOAD_SIZE 384 // Override MAX_PAYLOAD_SIZE for getModuleInfo which can return up to 348 bytes
// Extend the class for getModuleInfo
class SFE_UBLOX_GPS_ADD : public SFE_UBLOX_GNSS
{
public:
    bool getModuleInfo(uint16_t maxWait = 1100); //Queries module, texts

    struct minfoStructure // Structure to hold the module info (uses 341 bytes of RAM)
    {
        char swVersion[30];
        char hwVersion[10];
        uint8_t extensionNo = 0;
        char extension[10][30];
    } minfo;
};
SFE_UBLOX_GPS_ADD GPS;

void wakeUpGps(void) {
  if (!_hasGPS) return;
  digitalWrite(PIN_EXTINT, LOW);
  delay(1000);
  digitalWrite(PIN_EXTINT, HIGH);
  delay(1000);
  digitalWrite(PIN_EXTINT, LOW);
  //;;Serial.println("GPS ON");
}

void disableGps(void) {
  if (!_hasGPS) return;
  GPS.powerOffWithInterrupt(0xFFFFFFFF, VAL_RXM_PMREQ_WAKEUPSOURCE_EXTINT0);
  //;;Serial.println("GPS OFF");
}

void moduleInfo(){
    //GPS.enableDebugging(); // Uncomment this line to enable debug messages

    // setPacketCfgPayloadSize tells the library how many bytes our customPayload can hold.
    // If we call it after the .begin, the library will attempt to resize the existing 256 byte payload buffer
    // by creating a new buffer, copying across the contents of the old buffer, and then delete the old buffer.
    // This uses a lot of RAM and causes the code to fail on the ATmega328P. (We are also allocating another 341 bytes for minfo.)
    // To keep the code ATmega328P compliant - don't call setPacketCfgPayloadSize after .begin. Call it here instead.    
    GPS.setPacketCfgPayloadSize(MAX_PAYLOAD_SIZE);

    Serial.print(F("Polling module info"));
    if (GPS.getModuleInfo(1100) == false) {  // Try to get the module info
        Serial.print(F("getModuleInfo failed! Freezing..."));
        while (1)
            ;
    }
    Serial.println();
    Serial.println();
    Serial.println(F("Module Info : "));
    Serial.print(F("Soft version: "));
    Serial.println(GPS.minfo.swVersion);
    Serial.print(F("Hard version: "));
    Serial.println(GPS.minfo.hwVersion);
    Serial.print(F("Extensions:"));
    Serial.println(GPS.minfo.extensionNo);
    for (int i = 0; i < GPS.minfo.extensionNo; i++){
        Serial.print("  ");
        Serial.println(GPS.minfo.extension[i]);
    }
    Serial.println();
    Serial.println(F("Done!"));
}

bool SFE_UBLOX_GPS_ADD::getModuleInfo(uint16_t maxWait) {
    GPS.minfo.hwVersion[0] = 0;
    GPS.minfo.swVersion[0] = 0;
    for (int i = 0; i < 10; i++)
        GPS.minfo.extension[i][0] = 0;
    GPS.minfo.extensionNo = 0;

    // Let's create our custom packet
    uint8_t customPayload[MAX_PAYLOAD_SIZE]; // This array holds the payload data bytes

    // setPacketCfgPayloadSize tells the library how many bytes our customPayload can hold.
    // If we call it here, after the .begin, the library will attempt to resize the existing 256 byte payload buffer
    // by creating a new buffer, copying across the contents of the old buffer, and then delete the old buffer.
    // This uses a lot of RAM and causes the code to fail on the ATmega328P. (We are also allocating another 341 bytes for minfo.)
    // To keep the code ATmega328P compliant - don't call setPacketCfgPayloadSize here. Call it before .begin instead.
    //GPS.setPacketCfgPayloadSize(MAX_PAYLOAD_SIZE);

    // The next line creates and initialises the packet information which wraps around the payload
    ubxPacket customCfg = {0, 0, 0, 0, 0, customPayload, 0, 0, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED};

    // The structure of ubxPacket is:
    // uint8_t cls           : The message Class
    // uint8_t id            : The message ID
    // uint16_t len          : Length of the payload. Does not include cls, id, or checksum bytes
    // uint16_t counter      : Keeps track of number of overall bytes received. Some responses are larger than 255 bytes.
    // uint16_t startingSpot : The counter value needed to go past before we begin recording into payload array
    // uint8_t *payload      : The payload
    // uint8_t checksumA     : Given to us by the module. Checked against the rolling calculated A/B checksums.
    // uint8_t checksumB
    // sfe_ublox_packet_validity_e valid            : Goes from NOT_DEFINED to VALID or NOT_VALID when checksum is checked
    // sfe_ublox_packet_validity_e classAndIDmatch  : Goes from NOT_DEFINED to VALID or NOT_VALID when the Class and ID match the requestedClass and requestedID

    // sendCommand will return:
    // SFE_UBLOX_STATUS_DATA_RECEIVED if the data we requested was read / polled successfully
    // SFE_UBLOX_STATUS_DATA_SENT     if the data we sent was writted successfully (ACK'd)
    // Other values indicate errors. Please see the sfe_ublox_status_e enum for further details.

    // Referring to the u-blox M8 Receiver Description and Protocol Specification we see that
    // the module information can be read using the UBX-MON-VER message. So let's load our
    // custom packet with the correct information so we can read (poll / get) the module information.

    customCfg.cls = UBX_CLASS_MON; // This is the message Class
    customCfg.id = UBX_MON_VER;    // This is the message ID
    customCfg.len = 0;             // Setting the len (length) to zero let's us poll the current settings
    customCfg.startingSpot = 0;    // Always set the startingSpot to zero (unless you really know what you are doing)

    // Now let's send the command. The module info is returned in customPayload
    if (sendCommand(&customCfg, maxWait) != SFE_UBLOX_STATUS_DATA_RECEIVED)
        return (false); //If command send fails then bail
    // Now let's extract the module info from customPayload
    uint16_t position = 0;
    for (int i = 0; i < 30; i++){
        minfo.swVersion[i] = customPayload[position];
        position++;
    }
    for (int i = 0; i < 10; i++){
        minfo.hwVersion[i] = customPayload[position];
        position++;
    }
    while (customCfg.len >= position + 30) {
        for (int i = 0; i < 30; i++){
            minfo.extension[minfo.extensionNo][i] = customPayload[position];
            position++;
        }
        minfo.extensionNo++;
        if (minfo.extensionNo > 9)
            break;
    }
    return (true); //Success!
}


// save transmission states between loops
int transmissionState = RADIOLIB_ERR_NONE;

// flag to indicate transmission or reception state
bool transmitFlag = false;

// disable interrupt when it's not needed
volatile bool enableInterrupt = true;

// flag to indicate that a packet was sent or received
volatile bool operationDone = false;

volatile bool _flagPendingMsg = false;

// this function is called when a complete packet
// is transmitted or received by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
ICACHE_RAM_ATTR void setFlag(void) {
  if(!enableInterrupt) return;
  // we sent or received  packet, set the flag
  operationDone = true;
}
uint8_t _rxPktNum = 0;
String sMainMenu = (String)("K62");

uint32_t msgRxTxLoop() {
  ezMenu* curMenu = M5ez::getCurrentMenu();
  String sMenuTitle = curMenu->getTitle();
  // check if the previous operation finished
  if(operationDone) {
    // reset flag
    operationDone = false;

    if(transmitFlag) {
      // the previous operation was transmission
      // clean up after transmission is finished
      // this will ensure transmitter is disabled,
      // RF switch is powered down etc.
      radio.finishTransmit();
      
      // listen for response
      // print the result
      if (transmissionState == RADIOLIB_ERR_NONE) {
        // packet was successfully sent
        Serial.println(F("transmission finished!"));
      } else {
//        Serial.print(F("failed, code "));
//        Serial.println(transmissionState);
        if(sMenuTitle == sMainMenu){
          //we are in main menu
          curMenu->setCaption("txPacket", "Передати пакет\t" + String(transmissionState));
        }
      }

      // listen for response
      radio.startReceive();
      transmitFlag = false;

    } else {
      // the previous operation was reception
      // print data and send another packet
      byte byteArr[251];
      int state = radio.readData(byteArr, 251);

      if (state == RADIOLIB_ERR_NONE) {
        _rxPktNum++;
        // packet was successfully received
        //wake up screen BEFORE! drawing to it
        ez.backlight.wakeup();

        if(sMenuTitle == sMainMenu){
          //we are in main menu
          curMenu->setCaption("rxPktNum", "Прийнято пакетів\t" + String(_rxPktNum));
          curMenu->setCaption("lastRSSI", "RSSI\t" + String(radio.getRSSI()) + " dBm");
          curMenu->setCaption("lastSNR", "SNR\t" + String(radio.getSNR()) + " dB");
          // print data of the packet
//          Serial.print(F("[SX1262] Data:\t\t"));
//          Serial.println(str);
        }
      } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
        // packet was received, but is malformed
        curMenu->setCaption("rxPktNum", "Прийнято пакетів\t CRC");
      } else {
        // some other error occurred
        curMenu->setCaption("rxPktNum", "Прийнято пакетів\t Err");
      }
    }
  }
  if(_flagPendingMsg){
    _flagPendingMsg = false;
    byte byteArr[] = {0x01, 0x23, 0x45, 0x67,
                      0x89, 0xAB, 0xCD, 0xEF};
    transmissionState = radio.startTransmit(byteArr, 8);
    transmitFlag = true;
  }
  return 1000;
}


void setup() {
  //timerModeLoop();
  SPI.begin(RADIO_SCK, RADIO_MISO, RADIO_MOSI, RADIO_NSS);
  M5.Lcd.writecommand(0x29); // ILI9341_DISPON
  M5.Lcd.writecommand(0x11); // ILI9341_SLPOUT
  #include <themes/monoAmber.h>
  #include <themes/monoDark.h>
  #include <themes/monoDay.h>
  #include <themes/monoNight.h>
  #include <themes/monoDayBtn.h>
  disablePwrBtn();
  ezt::setDebug(INFO);
  ez.begin();
  ezt::setInterval(0);  //stops NTP updates
  local.setPosix(LOCALTZ_POSIX);
  local.setDefault();
  
  ez.canvas.font(&FreeMono9pt7b);
  ez.header.show("ТЕСТ");         
  ez.canvas.lmargin(10);
  //;;ez.canvas.print("WakeupReason: "); ez.canvas.println(wakeupReason, DEC);
  ez.canvas.println("Знайдені I2C пристрої:");
  
  uint8_t i2cFound = 0;
  for(uint8_t i2cAdr = 0; i2cAdr < 127; i2cAdr++ ) {
    Wire.beginTransmission(i2cAdr);
    if(Wire.endTransmission() == 0 ) {
      ez.canvas.print(i2cAdr,HEX); ez.canvas.print(" ");
      if(i2cAdr == 0x0C){
        _hasMAG = true;
        _hasAK  = true;  //AK8963
      }else if(i2cAdr == 0x1C){
        _hasMAG = true;
        _hasQMC = true;  //QMC6310U
      }else if(i2cAdr == 0x42){
        _hasGPS = true;  //U-blox
      }else if(i2cAdr == 0x48){
        _hasRFT = true;  //MCP9802A0T-M/OT
        _hasNTC = false; //Termistor
      }else if(i2cAdr == 0x4D){
        _hasADC = true;  //MCP3021A5T-E/OT
      }else if(i2cAdr == 0x51){
        _hasBM  = true;   //BM8563
        _hasRTC = true;
      }else if(i2cAdr == 0x52){
        _hasRV  = true;   //RV-3028-C7
        _hasRTC = true;
      }else if(i2cAdr == 0x68){
        _hasX68 = true;  //MPU6886 in M5 or DS3231?
      }else if(i2cAdr == 0x69){
        _hasIMU = true;  //MPU9250 in K46
        if (_hasX68) {
          _hasDS  = true;   //DS3231SN#
          _hasRTC = true;
        }
      }else if(i2cAdr == 0x6A){
        _hasQMI = true;  //QMI8658C
      }else if(i2cAdr == 0x6B){
        _hasQMI = true;  //QMI8658C
      }
      i2cFound++;
    }    
    delay(10);
  }
  ez.canvas.println();

  // initialize SX1262 with some settings
  // carrier frequency:           868.0 MHz
  // bandwidth:                   125.0 kHz
  // spreading factor:            9 //512
  // coding rate:                 8
  // sync word:                   0x12 (privat network)
  // output power:                2 dBm
  // preamble length:             20 symbols
  int state = radio.begin(K46_Freq, 125.0, 10, 8, 0x12, 2, 8);
  if (state == RADIOLIB_ERR_NONE) {
    ez.canvas.println("Радіо ініціалізоване");
  } else {
    ez.canvas.println("Радіо не ініціалізується");
  }
  #if defined (BAND_915)
    radio.setTCXO(2.7);
  #endif
  #if defined (USE_RFM95) || defined (USE_RFM98) || defined (USE_LORA127XF30)
    radio.setDio0Action(setFlag, RISING);
  #else
    radio.setDio1Action(setFlag);
  #endif
  state = radio.startReceive();
  if (state == RADIOLIB_ERR_NONE) {
    ez.canvas.println("Прийом почато");
  } else {
    ez.canvas.println("Прийом не почато");
  }

  if(_hasGPS){
    pinMode(PIN_EXTINT, OUTPUT);
    digitalWrite(PIN_EXTINT, LOW);
    // setPacketCfgPayloadSize tells the library how many bytes our customPayload can hold.
    // If we call it after the .begin, the library will attempt to resize the existing 256 byte payload buffer
    // by creating a new buffer, copying across the contents of the old buffer, and then delete the old buffer.
    // This uses a lot of RAM and causes the code to fail on the ATmega328P. (We are also allocating another 341 bytes for minfo.)
    // To keep the code ATmega328P compliant - don't call setPacketCfgPayloadSize after .begin. Call it here instead.    
    GPS.setPacketCfgPayloadSize(MAX_PAYLOAD_SIZE);
    GPS.begin();
    GPS.setI2COutput(COM_TYPE_UBX);
    GPS.setNavigationFrequency(2);  //Produce two solutions per second
    //moduleInfo();
    GPS.getTimePulseParameters(&timePulseParameters);
    //;;Serial.print(F("UBX_CFG_TP5 version: ")); Serial.println(timePulseParameters.version);
    timePulseParameters.tpIdx = 0; // Select the TIMEPULSE pin
    // We can configure the time pulse pin to produce a defined frequency or period
    // Here is how to set the period:
    // Let's say that we want our 1 pulse every second to be as accurate as possible. So, let's tell the module
    // to generate no signal while it is _locking_ to GNSS time. We want the signal to start only when the module is
    // _locked_ to GNSS time.
    timePulseParameters.freqPeriod = 0;                 // Set the frequency/period to zero
    timePulseParameters.pulseLenRatio = 0;              // Set the pulse ratio to zero  
    // When the module is _locked_ to GNSS time, make it generate a 0.25 second pulse every second
    // (Although the period can be a maximum of 2^32 microseconds (over one hour), the upper limit appears to be around 33 seconds)
    timePulseParameters.freqPeriodLock = 1000000;       // Set the period to 1,000,000 us
    timePulseParameters.pulseLenRatioLock = 100000;     // Set the pulse length to 100,000 us
    timePulseParameters.flags.bits.active = 1;          // Make sure the active flag is set to enable the time pulse. (Set to 0 to disable.)
    timePulseParameters.flags.bits.lockedOtherSet = 1;  // Tell the module to use freqPeriod while locking and freqPeriodLock when locked to GNSS time
    timePulseParameters.flags.bits.isFreq = 0;          // Tell the module that we want to set the period (not the frequency)
    timePulseParameters.flags.bits.isLength = 1;        // Tell the module that pulseLenRatio is a length (in us) - not a duty cycle
    timePulseParameters.flags.bits.polarity = 1;        // Tell the module that we want the rising edge at the top of second. (Set to 0 for falling edge.)
    // Now set the time pulse parameters
    GPS.setTimePulseParameters(&timePulseParameters);
    if (GPS.setTimePulseParameters(&timePulseParameters) == false){
      //;;Serial.println(F("setTimePulseParameters failed!"));
    } else {
      //;;Serial.println(F("Success!"));
    }
    // Finally, save the time pulse parameters in battery-backed memory so the pulse will automatically restart at power on
    GPS.saveConfigSelective(VAL_CFG_SUBSEC_NAVCONF); // Save the configuration
    GPS.saveConfiguration(); //Uncomment this line to save the current settings to flash and BBR
    GPS.powerSaveMode(); // Defaults to true
    GPS.powerOffWithInterrupt(0, VAL_RXM_PMREQ_WAKEUPSOURCE_EXTINT0);       
    ez.canvas.println("GPS для точного часу");
  } else {
    ez.canvas.println("Встановіть точний час!");
  }
  ez.addEvent(msgRxTxLoop);
}

void loop() {
  events();
  ezMenu mainmenu(sMainMenu);
  mainmenu.txtSmall();
  mainmenu.addItem("rxPktNum | Прийнято пакетів\t" + String(_rxPktNum), NULL);
  mainmenu.addItem("lastRSSI | RSSI\t");
  mainmenu.addItem("lastSNR | SNR\t");
  mainmenu.addItem("tx | Передати пакет", NULL, mngTransmit);
  mainmenu.addItem("Flexible text menus", mainmenu_menus);
  mainmenu.addItem("Event updated menus", NULL, mainmenu_settings);
  mainmenu.addItem("Image menus", mainmenu_image);
  mainmenu.addItem("Neat messages", mainmenu_msgs);
  mainmenu.addItem("Multi-function buttons", mainmenu_buttons);
  mainmenu.addItem("3-button text entry", mainmenu_entry);
  mainmenu.addItem("Built-in wifi & other settings", ez.settings.menu);
  mainmenu.addItem("Updates via https", mainmenu_ota);
  mainmenu.addItem("Налаштувати підсвітку", ez.backlight.menu);
  mainmenu.addItem("Вибрати режим екрану", ez.theme->menu);
  mainmenu.addItem("Розблокувати вимкнення", enablePwrBtn);
  mainmenu.upOnFirst("last|up");
  mainmenu.downOnLast("first|down");
  mainmenu.run();
}

void mainmenu_menus() { 
  ezMenu submenu("This is a sub menu");
  submenu.txtSmall();
  submenu.buttons("up#Back#select##down#");
  submenu.addItem("You can make small menus");
  submenu.addItem("Or big ones");
  submenu.addItem("(Like the Main menu)");
  submenu.addItem("In this menu most options");
  submenu.addItem("Do absolutely nothing");
  submenu.addItem("They are only here");
  submenu.addItem("To demonstrate that menus");
  submenu.addItem("Can run off the screen");
  submenu.addItem("And will just scroll");
  submenu.addItem("And scroll");
  submenu.addItem("And Scroll");
  submenu.addItem("And Scroll even more");   
  submenu.addItem("more | Learn more about menus", submenu_more);
  submenu.addItem("Exit | Go back to main menu");
  submenu.run();
}

enum ez_switch_t {
  SW_NORTH,
  SW_EAST,
  SW_SOUTH,
  SW_WEST
};

enum tmEntered_t {
  TM_DAY    = 0,
  TM_MONTH  = 1,
  TM_YEAR   = 2,
  TM_HOUR   = 3,
  TM_MINUTE = 4
};
  
time_t alarmTime = 0;
uint16_t updatedData = 0;
uint8_t days[12] = {31,29,31,30,31,30,31,31,30,31,30,31};
ez_switch_t rotarySwitch = SW_NORTH;
uint8_t timeOutVar = 0;
static bool toggleSwitch = false;
bool extraControl = false;
String strRotary[4]   = {"North", "East", "South", "West"};

bool mngAlarmTime(ezMenu* callingMenu) {
  tmElements_t tmOnScreen;
  uint16_t highlight_color = TFT_RED;
  String disp_val;
  uint8_t elementSet = TM_DAY;

  //production time as start point
  tmOnScreen.Hour   = 12;
  tmOnScreen.Minute = 0;
  tmOnScreen.Day    = 1;
  tmOnScreen.Month  = 3;
  tmOnScreen.Year   = 21;

  elementSet  = TM_DAY;
  disp_val = "Enter alarm time in | DD.MM.YY HH:MM format";
  while (true) {     
    ez.msgBox("TIME", disp_val, " - # -- # select | > # OK # + # ++ # # # Back ", false, &FreeMono12pt7b);
    ez.canvas.font(&FreeMonoBold18pt7b);
    M5.lcd.fillRect (0, ez.canvas.bottom() - 40, TFT_W, 40, ez.theme->background); 
    ez.canvas.pos(13, ez.canvas.bottom() - 40);
    ez.canvas.color(elementSet == TM_DAY    ? highlight_color : ez.theme->msg_color);            
    ez.canvas.print(zeropad((uint32_t)tmOnScreen.Day, 2));
    ez.canvas.print(".");
    ez.canvas.color(elementSet == TM_MONTH  ? highlight_color : ez.theme->msg_color);            
    ez.canvas.print(zeropad((uint32_t)tmOnScreen.Month, 2));
    ez.canvas.color(ez.theme->msg_color);        
    ez.canvas.print(".");      
    ez.canvas.color(elementSet == TM_YEAR   ? highlight_color : ez.theme->msg_color);
    ez.canvas.print(zeropad((uint32_t)tmOnScreen.Year, 2));
    ez.canvas.color(ez.theme->msg_color);        
    ez.canvas.print(" ");
    ez.canvas.color(elementSet == TM_HOUR   ? highlight_color : ez.theme->msg_color);  
    ez.canvas.print(zeropad((uint32_t)tmOnScreen.Hour, 2));
    ez.canvas.color(ez.theme->msg_color);        
    ez.canvas.print(":");
    ez.canvas.color(elementSet == TM_MINUTE ? highlight_color : ez.theme->msg_color);          
    ez.canvas.print(zeropad((uint32_t)tmOnScreen.Minute, 2));
    String b = ez.buttons.wait();
    if (b == "select") elementSet == 4 ? elementSet-=4 : elementSet++ ; 
    switch (elementSet) {
      case TM_DAY:
        if (tmOnScreen.Day > days[tmOnScreen.Month-1]) tmOnScreen.Day = days[tmOnScreen.Month-1];
        if (b == "-" && tmOnScreen.Day >  1) tmOnScreen.Day--;
        if (b == "+" && tmOnScreen.Day < days[tmOnScreen.Month-1]) tmOnScreen.Day++;
        if (b == "--") {
          if (tmOnScreen.Day < 10) {
            tmOnScreen.Day = 0;
          } else {
            tmOnScreen.Day -= 10;
          }
        }
        if (b == "++") {
          if (tmOnScreen.Day > 20) {
            tmOnScreen.Day = days[tmOnScreen.Month-1];
          } else {
            tmOnScreen.Day += 10;
          }
        }
        break;
      case TM_MONTH: 
        if (((b == "-") || (b == "--")) && (tmOnScreen.Month >  1)) tmOnScreen.Month--;
        if (((b == "+") || (b == "++")) && (tmOnScreen.Month < 12)) tmOnScreen.Month++;
        if (tmOnScreen.Day > days[tmOnScreen.Month-1]) tmOnScreen.Day = days[tmOnScreen.Month-1];
        break;
      case TM_YEAR: 
        if (((b == "-") || (b == "--")) && (tmOnScreen.Year > 20)) tmOnScreen.Year--;
        if (((b == "+") || (b == "++")) && (tmOnScreen.Year < 35)) tmOnScreen.Year++;
        break; 
      case TM_HOUR: 
        if ((b == "-") && (tmOnScreen.Hour >  0)) tmOnScreen.Hour--;
        if ((b == "+") && (tmOnScreen.Hour < 23)) tmOnScreen.Hour++;
        if (b == "--") {
          if (tmOnScreen.Hour < 3) {
            tmOnScreen.Hour = 0;
          } else {
            tmOnScreen.Hour -= 3;
          }
        }
        if (b == "++") {
          if (tmOnScreen.Hour > 21) {
            tmOnScreen.Hour = 0;
          } else {
            tmOnScreen.Hour += 3;
          }
        }
        break;
      case TM_MINUTE: 
        if (b == "-") {
          tmOnScreen.Minute--;
          if(tmOnScreen.Minute == 255)tmOnScreen.Minute = 59;
        }
        if (b == "+") {
          tmOnScreen.Minute++;
          if(tmOnScreen.Minute == 60)tmOnScreen.Minute = 0;
        }
        if (b == "--") {
          if (tmOnScreen.Minute < 10) {
            tmOnScreen.Minute = 0;
          } else {
            tmOnScreen.Minute -= 10;
          }
        }
        if (b == "++") {
          if (tmOnScreen.Minute > 50) {
            tmOnScreen.Minute = 59;
          } else {
            tmOnScreen.Minute += 10;
          }
        }
        break;          
    }
    if (b == "OK") {
      tmOnScreen.Year += 30;
      tmOnScreen.Second = 00;
      time_t timeEntered = makeTime(tmOnScreen);
      
      if (timeStatus() == timeNotSet) {
        // timeNotSet - We are enering system time
        // store entered time somewhere
        //setRtcEpoch(timeEntered);
        UTC.setTime(timeEntered);
        ez.header.draw("clock");
        callingMenu->setCaption("settime", "System time set\t" + UTC.dateTime(timeEntered, "d.m.y H:i"));
      } else {
        // timeSet || timeNeedSync
        UTC.setTime(timeEntered);
        ez.header.draw("clock");
        callingMenu->setCaption("settime", "Alarm time set\t"  + UTC.dateTime(timeEntered, "d.m.y H:i"));
        break;
      }
      break;
    } else if (b == "Back") {
      break;
    }
  }
  return true;
}

bool mngTransmit(ezMenu* callingMenu) {
    if(_flagPendingMsg) return true;
    _flagPendingMsg = true;
    return true;
}

bool mngToggle(ezMenu* callingMenu) {
    toggleSwitch = !toggleSwitch;
    callingMenu->setCaption("toggle", "Toggle something\t" + String(toggleSwitch ? "On" : "Off"));
    return true;
}

bool mngRotary(ezMenu* callingMenu) {
    uint8_t switchVar = (uint8_t)rotarySwitch;
    switchVar += 1;
    switchVar %= 4;
    rotarySwitch = (ez_switch_t)switchVar;
    callingMenu->setCaption("rotary", "Rotary Switch\t" + strRotary[switchVar]);
    return true;
}

bool mngTimeOut(ezMenu* callingMenu){
    timeOutVar += 15;
    timeOutVar %= 75;
    callingMenu->setCaption("timeout", "Show disabled state\t" + (String)(!timeOutVar ? "Off" : (String)(timeOutVar) + "s"));
    return true;
}

int64_t timeOnEntry, timeOnEntryOld;
uint32_t period;
uint32_t secondsCounter = 0;

uint32_t eventLoop() {
    //get some updated data
    
    //for example period between loop reentries in us
    timeOnEntry = esp_timer_get_time();
    period = timeOnEntry - timeOnEntryOld;
    timeOnEntryOld = timeOnEntry;
    secondsCounter++;
    
    //wake up screen on some event
    if(secondsCounter >= 100){
      secondsCounter = 0;
      ez.backlight.wakeup();
    }
    
    //show updated data in an item of the active menue 
    ezMenu* curMenu = M5ez::getCurrentMenu();
    if (curMenu->getTitle() == "Control") {
        curMenu->setCaption("update", "Loop period\t" + String(period) + "us");
        curMenu->setCaption("counter", "Seconds from start\t" + String(secondsCounter));
    }
    ez.yield();
    return 1000000; //1s
}

bool loopStarted = false;

bool mngEventLoop(ezMenu* callingMenu) {
    callingMenu->addItem("counter | Seconds from start\t" + String(secondsCounter));
    if (!loopStarted) {
        ez.addEvent(eventLoop);
        loopStarted = true;
    }
    return true;
}


bool mainmenu_settings(ezMenu* callingMenu) {
  ezMenu controlmenu("Control");
  controlmenu.txtSmall();
  controlmenu.buttons("up#Back#select##down#");
  controlmenu.addItem("update | Start reading data\t", NULL, mngEventLoop); //to add event eventLoop() once
  controlmenu.addItem("toggle | Toggle something\t" + String(toggleSwitch ? "On" : "Off"), NULL, mngToggle);
  controlmenu.addItem("rotary | Rotary Switch\t" + strRotary[(uint8_t)rotarySwitch], NULL, mngRotary);
  controlmenu.addItem("timeout | Show disabled state\t" + (String)(!timeOutVar ? "Off" : (String)(timeOutVar) + "s"), NULL, mngTimeOut);
  controlmenu.addItem("settime | Set some time\t" + (String)(!alarmTime ? "dd.mm.yy hh:mm" : UTC.dateTime(alarmTime, "d.m.y H:i")), NULL, mngAlarmTime);
  controlmenu.run();
  //on Back/Exit/Done
  ez.removeEvent(eventLoop);    //if event is not needed any more
  loopStarted = false; //allows to restart eventLoop
  secondsCounter = 0;
  controlmenu.deleteItem("counter");
  return true;
}

void submenu_more() {
  ez.header.show("A simple menu in code...");
  ez.canvas.lmargin(10);
  ez.canvas.println("");
  ez.canvas.println("ezMenu menu(\"Main menu\");");
  ez.canvas.println("menu.addItem(\"Option 1\");");
  ez.canvas.println("menu.addItem(\"Option 2\");");
  ez.canvas.println("menu.addItem(\"Option 3\");");
  ez.canvas.println("while ( menu.run() ) {");
  ez.canvas.println("  if (menu.pick == 1) {");
  ez.canvas.println("    ez.msgBox (\"One!\");");
  ez.canvas.println("  }");
  ez.canvas.println("}");
  ez.buttons.wait("OK");
  
  ezMenu fontmenu("Menus can change looks");
  fontmenu.txtFont(&Satisfy_24);
  fontmenu.addItem("Menus can use");
  fontmenu.addItem("Various Fonts");
  fontmenu.runOnce();
  
  ezMenu delmenu("Menus are dynamic");
  delmenu.txtSmall();
  delmenu.addItem("You can delete items");
  delmenu.addItem("While the menu runs");
  delmenu.addItem("Delete me!");
  delmenu.addItem("Delete me!");
  delmenu.addItem("Delete me!");
  delmenu.addItem("Exit | Go back" );
  while (delmenu.runOnce()) {
    if (delmenu.pickName() == "Delete me!") {
      delmenu.deleteItem(delmenu.pick());
    }
  }
}

void mainmenu_image() {
  ezMenu images;
  images.imgBackground(TFT_BLACK);
  images.imgFromTop(40);
  images.imgCaptionColor(TFT_WHITE);
  images.addItem(sysinfo_jpg, "System Information", sysInfo);
  images.addItem(wifi_jpg, "WiFi Settings", ez.wifi.menu);
  images.addItem(about_jpg, "About M5ez", aboutM5ez);
  images.addItem(sleep_jpg, "Power Off", powerOff);
  images.addItem(return_jpg, "Back");
  images.run();
}
    
void mainmenu_msgs() {
  String cr = (String)char(13);
  ez.msgBox("You can show messages", "ez.msgBox shows text");
  ez.msgBox("Looking the way you want", "In any font !", "OK", true, &FreeSerifBold24pt7b, TFT_RED);
  ez.msgBox("More ez.msgBox", "Even multi-line messages where everything lines up and is kept in the middle of the screen");
  ez.msgBox("Questions, questions...", "But can it also show any buttons you want?", "No # # Yes"); 
  ez.textBox("And there's ez.textBox", "To present or compose longer word-wrapped texts, you can use the ez.textBox function." + cr + cr + "M5ez (pronounced \"M5 easy\") is a complete interface builder library for the M5Stack ESP32 system. It allows even novice programmers to create good looking interfaces. It comes with menus as text or as images, message boxes, very flexible button setup (including different length presses and multi-button functions), 3-button text input (you have to see it to believe it) and built-in Wifi support. Now you can concentrate on what your program does, and let M5ez worry about everything else.", true);
}
    
void mainmenu_buttons() {
  ez.header.show("Simple buttons...");
  ez.canvas.font(&FreeSans12pt7b);
  ez.canvas.lmargin(20);
  ez.canvas.println("");      
  ez.canvas.println("You can have three buttons");
  ez.canvas.println("with defined funtions.");
  ez.buttons.show("One # Two # Done");
  printButton();
  ez.canvas.clear();
  ez.header.show("More functions...");
  ez.canvas.println("");      
  ez.canvas.println("But why stop there?");
  ez.canvas.println("If you press a little longer");
  ez.canvas.println("You access the functions");
  ez.canvas.println("printed in cyan.");
  ez.buttons.show("One # Two # Three # Four # Done #");
  printButton();
  ez.canvas.clear();
  ez.header.show("Two keys ...");
  ez.canvas.y(ez.canvas.top() + 10);
  ez.canvas.println("It gets even better...");
  ez.canvas.println("The purple bar shows the");
  ez.canvas.println("functions for key combis.");
  ez.canvas.println("See if you can work it out...");
  ez.buttons.show("One # Two # Three # Four # Five # Six # Seven # Eight # Done");
  printButton();
}

void printButton(){
  while (true) {
  String btnpressed = ez.buttons.poll();
  if (btnpressed == "Done") break;
  if (btnpressed != "") {
    m5.lcd.fillRect (0, ez.canvas.bottom() - 45, TFT_W, 40, ez.theme->background); 
    ez.canvas.pos(20, ez.canvas.bottom() - 45);
    ez.canvas.color(TFT_RED);
    ez.canvas.font(&FreeSansBold18pt7b);
    ez.canvas.print(btnpressed);
    ez.canvas.font(&FreeSans12pt7b);
    ez.canvas.color(TFT_BLACK);
  }
  }
}

void mainmenu_entry() {
  if (ez.msgBox("We're gonna enter text ... !", "Have you learned to use the buttons? Go there first if you haven't been there. Or hit 'Go' to see if you can enter your name.", "Back # # Go") == "Go") {
    String your_name = ez.textInput();
    ez.msgBox("Pfew...", "Hi " + your_name + "! | | Now that was a pain! But it is good enough for entering, say, a WPA key, or don't you think?");
    ez.msgBox("Don't worry", "(You do get better with practice...)");
  }
}

void mainmenu_ota() {
  if (ez.msgBox("Get OTA_https demo", "This will replace the demo with a program that can then load the demo program again.", "Cancel#OK#") == "OK") {
    ezProgressBar progress_bar("OTA update in progress", "Downloading ...", "Abort");
    #include "raw_githubusercontent_com.h" // the root certificate is now in const char * root_cert
    if (ez.wifi.update("https://raw.githubusercontent.com/M5ez/M5ez/master/compiled_binaries/OTA_https.bin", root_cert, &progress_bar)) {
      ez.msgBox("Over The Air updater", "OTA download successful. Reboot to new firmware", "Reboot");
      ESP.restart();
    } else {
      ez.msgBox("OTA error", ez.wifi.updateError(), "OK");
    }
  }
}

void powerOff() {  }

void aboutM5ez() {
  ez.msgBox("About M5ez", "M5ez was written by | Rop Gonggrijp | | https://github.com/M5ez/M5ez");
}
