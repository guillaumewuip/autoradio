
#include <Arduino.h>
#include <EEPROM.h>
#include <Wire.h>

//@see https://bitbucket.org/fmalpartida/new-liquidcrystal/downloads
#include <LiquidCrystal_I2C.h>

#include <radio.h>
#include <si4703.h>
#include <RDSParser.h>
#include "RADIO_LCD.h"

//@see http://playground.arduino.cc/Code/Buttons
#include <buttons.h>

//@see https://github.com/guillaumewuip/clioSteeringWheelCmd
#include <CLIO_STEERING_WHEEL_CMD.h>

#define BMode_pin   7
#define BUp_pin     6
#define BDown_pin   5
#define BTimer      700
#define BRefresh    500

#define FIX_BAND  RADIO_BAND_FM
#define MAXVOLUME 15

//interval between two lcd show (nb of loop)
#define REFRESH_INTERVAL 175

//number of loop to wait before going back to title screen
#define INTERVAL_SCREEN  1000

//if station changes quicker that every 8 sec
//it's a "moving" station
#define INTERVAL_MOVING_STATION 8000

//EEPROM addresses
#define LIGHT_ADDR   0
#define VOLUME_ADDR  1
#define MUTE_ADDR    2
#define MODE_ADDR    3
#define FREQ_ADDR    4  //uint16 on 2 byte here
#define PREF_ADDR    6  //start of the preferences memory
#define PREF_LENTGH  15 //how many preferences ?

//default values if no EEPROM
#define LIGHT_DEFAULT   8
#define VOLUME_DEFAULT  8
#define MUTE_DEFAULT    0
#define FREQ_DEFAULT    8780

//Clio controller intervals
#define INTERVAL_CTRL_PRESS    20
#define INTERVAL_CTRL_HOLD     250

#define LCD_BACKLIGHT 3

SI4703 radio;
RDSParser rds;
RADIO_LCD lcd(LCD_BACKLIGHT);

uint8_t  ctrl_inputs[3]  = {0, 1, 2}; //analog A0, A1, A2
uint8_t  ctrl_outputs[3] = {12, 11, 10}; //digital D12, D11, D10
WHEEL_CMD controller(ctrl_inputs, ctrl_outputs);

Button BMode;
Button BUp;
Button BDown;

uint8_t mode;
uint8_t screen = TITLE_SCREEN;

uint16_t preferences[PREF_LENTGH];

RADIO_FREQ maxFrequency  = 20000; //10800
RADIO_FREQ minFrequency  = 8000;  //8750
uint8_t    stepFrequency = 10;

String stationRDS;
String titleRDS;
uint8_t _movingStation = 0;
unsigned long _lastUpdateStation = millis();

uint8_t  refreshCounter = 0;
uint16_t screenCounter  = 0;

/**
 * RDS_process()
 *
 * @function
 * @abstract    RDS info callback
 */
void RDS_process(uint16_t block1, uint16_t block2, uint16_t block3,
        uint16_t block4) {
    rds.processData(block1, block2, block3, block4);
}

/**
 * DisplayServiceName()
 *
 * @function
 * @abstract    Receive the RDS station name and check if it's a "moving"
 *              station
 *
 * @param   {char *}    name
 */
void DisplayServiceName(char *name) {
    Serial.print(F("RDS:"));
    Serial.println(name);

    //if the station has changed before the INTERVAL_MOVING_STATION interval
    if (String(name) != stationRDS &&
            millis() - _lastUpdateStation < INTERVAL_MOVING_STATION) {
        _movingStation = 2; //it's a moving station
    } else {
        if (_movingStation > 0) {
            _movingStation--; //goes to 1, then 0 in order to wait if late message
        }
    }

    stationRDS = name;
    _lastUpdateStation = millis(); //save time
}

/**
 * DisplayText()
 *
 * @function
 * @abstract    Receive the RDS second stream (usually the title of the track)
 *
 * @param   {char *}    text
 */
void DisplayText(char *text) {
    Serial.print(F("RDS-text:"));
    Serial.println(text);

    titleRDS = text;
}

void changeScreen(uint8_t s) {
    screen = s;
    screenCounter = 0;
};

String changeFrequency(int frequency) {
    String command;

    if (frequency > maxFrequency) {
        frequency = minFrequency;
    } else if (frequency < minFrequency) {
        frequency = maxFrequency;
    }

    radio.setFrequency(frequency);
    radio.clearRDS();
    writeInt16EEPROM(FREQ_ADDR, frequency + 1);

    command = F("Frequency :");
    command += frequency;
    return command;
};

String moveFrequency(int add) {
    String command;
    RADIO_FREQ frequency = radio.getFrequency();

    frequency += add;
    if (frequency > maxFrequency) {
        frequency = minFrequency;
    } else if (frequency < minFrequency) {
        frequency = maxFrequency;
    }

    return changeFrequency(frequency);
};

String upMode() {
    String command;
    mode += 1;
    if (mode > MAX_MODE) {
        mode = 0;
    }
    command = F("Change mode to :");
    command += mode;
    changeScreen(NORMAL_MODE);
    EEPROM.update(MODE_ADDR, mode + 1);

    return command;
};

String resetMode() {
    mode = NORMAL_MODE;
    changeScreen(NORMAL_MODE);

    return F("Reset mode");
};

String changeMute() {
    String command;
    bool mute = !radio.getMute();
    command = F("Mute ");
    command += mute;
    radio.setMute(mute);
    changeScreen(VOLUME_SCREEN);
    EEPROM.update(MUTE_ADDR, mute + 1);

    return command;
};

String seekUp() {
    String command = F("Seek up");
    screen = SEARCH_SCREEN_UP;
    radio.seekUp();

    return command;
};

String seekDown() {
    String command = F("Seek down");
    screen = SEARCH_SCREEN_DOWN;
    radio.seekDown();

    return command;
};

String prefUp() {
    String command = F("Preference up");
    int8_t currentPref = indexOfArray(preferences, PREF_LENTGH, radio.getFrequency());

    //if we're not on a pref currently
    if (currentPref == -1 && preferences[0] > 0) {
        changeFrequency(preferences[0]);
    //if we're on a pref already
    } else if (currentPref > -1) {
        //there is a pref after
        if (currentPref + 1 < PREF_LENTGH
                && preferences[currentPref + 1] > 0) {

            changeFrequency(preferences[currentPref + 1]);
        } else {
            changeFrequency(preferences[0]); //reset
        }
    }

    return command;
};

String prefDown() {
    String command = F("Preference down");
    int8_t currentPref = indexOfArray(preferences, PREF_LENTGH, radio.getFrequency());

    //if we're not on a pref currently
    if (currentPref == -1 && preferences[0] > 0) {
        changeFrequency(preferences[0]);

    //if we're on a pref already
    } else if (currentPref > -1) {
        //there is a pref before
        if (currentPref - 1 >= 0 && preferences[currentPref - 1] > 0) {
            changeFrequency(preferences[currentPref - 1]);
        } else {
            uint8_t i = PREF_LENTGH - 1;
            while (preferences[i] == 0) {
                i--;
            }
            changeFrequency(preferences[i]);
        }
    }

    return command;
};

String backlightUp() {
    String command = F("Backlight +1");
    uint8_t light = lcd.backlightUp();
    changeScreen(LIGHT_SCREEN);
    EEPROM.update(LIGHT_ADDR, light + 1);

    return command;
};

String backlightDown() {
    String command = F("Backlight -1");
    uint8_t light = lcd.backlightDown();

    changeScreen(LIGHT_SCREEN);
    EEPROM.update(LIGHT_ADDR, light + 1);

    return command;
};

String volUp() {
    String command = F("Volume +1");
    uint8_t volume = radio.getVolume();
    if (volume < MAXVOLUME) {
        volume += 1;
    }

    radio.setVolume(volume);
    changeScreen(VOLUME_SCREEN);
    EEPROM.update(VOLUME_ADDR, volume + 1);

    return command;
};

String volDown() {
    String command = F("Volume -1");
    uint8_t volume = radio.getVolume();
    if (volume > 0) {
        volume -= 1;
    }

    radio.setVolume(volume);
    changeScreen(VOLUME_SCREEN);
    EEPROM.update(VOLUME_ADDR, volume + 1);

    return command;
};

String up() {
    String command;

    if (mode == NORMAL_MODE) {
        command = volUp();
    } else if (mode == BACKLIGHT_MODE) {
        command = backlightUp();
    } else if (mode == PREF_MODE) {
        command = prefUp();
    } else if (mode == SEEK_MODE) {
        command = seekUp();
    } else if (mode == SEARCH_MODE) {
        command = moveFrequency(stepFrequency);
    }

    return command;
};

String down() {
    String command;

    if (mode == NORMAL_MODE) {
        command = volDown();
    } else if (mode == BACKLIGHT_MODE) {
        command = backlightDown();
    } else if (mode == PREF_MODE) {
        command = prefDown();
    } else if (mode == SEEK_MODE) {
        command = seekDown();
    } else if (mode == SEARCH_MODE) {
        command = moveFrequency(-stepFrequency);
    }

    return command;
};

String prefRemoveOrSave() {
    String command;
    RADIO_FREQ frequency = radio.getFrequency();
    int8_t index = indexOfArray(preferences, PREF_LENTGH, radio.getFrequency());
    if (index == -1) { //add in preferences
        command = F("Save preference");
        savePreference(frequency);
    } else { //remove from preferences
        command = F("Remove preference");
        removePreference(index);
    }

    return command;
};

void listenSerialCommand() {

    char cmd;
    String message;
    String command;

    while (Serial.available() > 0) {
        cmd = Serial.read();

        message = F("Command : ");
        message += cmd;
        Serial.println(message);

        //Mode
        if (cmd == 'm') {
            command = upMode();
        }

        //Volume
        else if (cmd == '+') {
            command = up();
        } else if (cmd == '-') {
            command = down();
        }

        //Station
        else if (cmd == 'n') {
            command = moveFrequency(stepFrequency);
        } else if (cmd == 'p') {
            command = moveFrequency(-stepFrequency);
        } else if (cmd == 'N') {
            command = moveFrequency(100);
        } else if (cmd == 'P') {
            command = moveFrequency(-100);
        }

        //Mute
        else if (cmd == 'M') {
            command = changeMute();
        }

        //Seek
        else if (cmd == '>') {
            command = seekUp();
        } else if (cmd == '<') {
            command = seekDown();
        }

        else if (cmd == 's') {
            command = prefRemoveOrSave();
        }

        //Info
        else if (cmd == 'i') {
            char s[12];
            radio.formatFrequency(s, sizeof(s));
            Serial.print(F("Station:"));
            Serial.println(s);

            Serial.print(F("Radio:"));
            radio.debugRadioInfo();

            Serial.print(F("Audio:"));
            radio.debugAudioInfo();

            changeScreen(INFO_SCREEN);
        }

        //reset memory
        else if (cmd == 'r') {
            command = F("Reset memory");
            resetMemory();
            changeScreen(TITLE_SCREEN);
        }

        //debug preferences on Serial
        else if (cmd == 'f') {
            for (uint8_t i=0; i < PREF_LENTGH; i++) {
                Serial.print(F("Preference "));
                Serial.print(i);
                Serial.print(F(" : "));
                Serial.println(preferences[i]);
            }
        }

        Serial.println(command);
    }
}

void listenControllerCommand() {

    String command;

    uint8_t btn_vol_up = controller.getButton(BTN_VOL_UP);
    uint8_t btn_vol_down = controller.getButton(BTN_VOL_DOWN);

    uint8_t btn_source_right = controller.getButton(BTN_SOURCE_RIGHT);
    uint8_t btn_source_left = controller.getButton(BTN_SOURCE_LEFT);

    uint8_t btn_pref = controller.getButton(BTN_MODE);

    uint8_t wheel = controller.getWheel();

    if (btn_vol_up == PRESSED && btn_vol_down == NO_NEWS) {
        command = volUp();
    }
    if (btn_vol_down == PRESSED && btn_vol_up == NO_NEWS) {
        command = volDown();
    }

    if (controller.getButton(BTN_MUTE) == PRESSED) {
        command = changeMute();
    }

    if (btn_source_right == HOLD || btn_source_left == HOLD) {
        command = prefRemoveOrSave();
    } else {
        if (btn_source_right == PRESSED) {
            command = prefUp();
        } else if (btn_source_left == PRESSED) {
            command = prefDown();
        }

    }

    if(btn_pref == PRESSED) {
        command = prefRemoveOrSave();
    }

    if (wheel == WHEEL_UP) {
        command = seekUp();
    } else if (wheel == WHEEL_DOWN) {
        command = seekDown();
    }

    if (command.length() > 0) {
        Serial.print("Controller cmd : ");
        Serial.println(command);
    }

};

void initButtons() {
    BMode.assign(BMode_pin);
    BMode.setMode(OneShotTimer);
    BMode.setTimer(BTimer);
    BMode.setRefresh(BRefresh);

    BUp.assign(BUp_pin);
    BUp.setMode(OneShotTimer);
    BUp.setTimer(BTimer);
    BUp.setRefresh(BRefresh);

    BDown.assign(BDown_pin);
    BDown.setMode(OneShotTimer);
    BDown.setTimer(BTimer);
    BDown.setRefresh(BRefresh);
};

void testButtons() {
    switch(BMode.check()) {
        case ON:
            Serial.println(F("BMode ON"));
            upMode();
            break;
        case Hold:
            Serial.println(F("BMode Hold"));
            resetMode();
            break;
        default:
            break;
    }

    switch(BUp.check()) {
        case ON:
            Serial.println(F("BUp ON"));
            up();
            break;
        case Hold:
            Serial.println(F("BUp Hold"));
            prefRemoveOrSave();
            break;
        default:
            break;
    }

    switch(BDown.check()) {
        case ON:
            Serial.println(F("BDown ON"));
            down();
            break;
        case Hold:
            Serial.println(F("BDown Hold"));
            prefRemoveOrSave();
            break;
        default:
            break;
    }
};

void setup() {

    Serial.begin(9600);

    Serial.println(F("Hello world"));
    pinMode(LCD_BACKLIGHT, OUTPUT); //lcd backlight led

    initButtons();

    if (BMode.check() == ON) {
        Serial.println(F("Reset memory at start"));
        resetMemory();
    }

    controller.setIntervalPress(INTERVAL_CTRL_PRESS);
    controller.setIntervalHold(INTERVAL_CTRL_HOLD);

    Serial.println(F("Hello world"));

    mode = EEPROM.read(MODE_ADDR);
    uint8_t light     = EEPROM.read(LIGHT_ADDR);
    uint8_t volume    = EEPROM.read(VOLUME_ADDR);
    uint8_t mute      = EEPROM.read(MUTE_ADDR);
    RADIO_FREQ frequency = readInt16EEPROM(FREQ_ADDR);
    Serial.print(F("Frequency"));
    Serial.println(frequency);

    if (light == 0 or light > 15) {
        light = LIGHT_DEFAULT + 1;
        EEPROM.update(LIGHT_ADDR, light);
    }
    if (volume == 0 or volume > 15) {
        volume = VOLUME_DEFAULT + 1;
        EEPROM.update(VOLUME_ADDR, volume);
    }
    if (mute == 0 or mute > 2) {
        mute = MUTE_DEFAULT + 1;
        EEPROM.update(MUTE_ADDR, mute);
    }
    if (mode == 0 or mode > MAX_MODE + 1) {
        mode = NORMAL_MODE + 1;
        EEPROM.update(MODE_ADDR, mode);
    }
    if (frequency == 0) {
        frequency = FREQ_DEFAULT + 1;
        EEPROM.put(FREQ_ADDR, frequency);
        writeInt16EEPROM(FREQ_ADDR, frequency);
    }

    light--;
    volume--;
    mute--;
    frequency--;
    mode--;

    loadPreferences();

    lcd.init();
    lcd.setBacklight(light);
    lcd.flash();
    lcd.hello();

    delay(1000);

    radio.debugEnable();
    radio.init();

    // Set all radio setting to the fixed values.
    radio.setBandFrequency(FIX_BAND, frequency);

    stepFrequency = radio.getFrequencyStep();
    minFrequency  = radio.getMinFrequency();
    maxFrequency  = radio.getMaxFrequency();

    Serial.print(F("Frequency step : "));
    Serial.println(stepFrequency);
    Serial.print(F("Frequency min/max : "));
    Serial.print(minFrequency);

    Serial.println(maxFrequency);

    radio.setVolume(volume);
    radio.setMono(false);
    radio.setMute(mute);

    //radio.debugStatus(); // the current values of all registers
    radio.attachReceiveRDS(RDS_process);

    rds.attachServicenNameCallback(DisplayServiceName);
    rds.attachTextCallback(DisplayText);

    Serial.println(F("End setup"));
}

void loop() {

    testButtons();
    controller.update();

    listenSerialCommand();
    listenControllerCommand();

    radio.checkRDS();

    if (refreshCounter > REFRESH_INTERVAL) {
        RADIO_INFO infos;
        radio.getRadioInfo(&infos);

        RADIO_FREQ frequency = radio.getFrequency();

        lcd.show(
            mode,
            screen,
            infos.active,
            radio.getMute(),
            infos.rssi,
            radio.getVolume(),
            frequency,
            stationRDS,
            titleRDS,
            _movingStation,
            indexOfArray(preferences, PREF_LENTGH, frequency) + 1
        );

        refreshCounter = 0;
    }

    //if we've passed INTERVAL_MOVING_STATION time
    if (_movingStation > 0 && millis() - _lastUpdateStation > INTERVAL_MOVING_STATION) {
        _movingStation--; //goes to 1, then 0 in order to wait if late message
    }

    //go back to TITLE_SCREEN after x seconds
    if (screen != TITLE_SCREEN
            and screenCounter > INTERVAL_SCREEN) {
        Serial.println(F("reset screen"));
        changeScreen(TITLE_SCREEN);
    }

    refreshCounter += 1;
    screenCounter += 1;

}
