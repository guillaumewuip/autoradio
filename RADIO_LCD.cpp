#include "Arduino.h"
#include "RADIO_LCD.h"
#include <LiquidCrystal.h>

//max width of the title string
#define TITLE_WIDTH             16
//max width of the station string
#define STATION_WIDTH           10
//number of spaces to wait at the end of a rotation
#define SPACE_END               2

//if rssi is below this, we say it's a bad reception
#define BAD_RECEPTION           15

#define MAX_VOLUME              15

//step between each backlight value
//real backlight value = value * BACKLIGHT_STEP
#define BACKLIGHT_STEP          17

//max backlight value
#define MAX_BACKLIGHT           15

//import _lcd from main file
extern LiquidCrystal _lcd;

/**
 * repeat()
 *
 * @function
 * @abstract    Repeat the character n times
 *
 * @param   {char}      c
 * @param   {uint8_t}   n
 *
 * @return  {Sring}     c repeated n times
 */
String repeat(char c, uint8_t n) {
    String result;
    for (uint8_t i=0; i < n; i ++) {
        result += c;
    }

    return result;
};

/**
 * center()
 *
 * @function
 * @abstract    Center the string
 * @discussion  Will add spaces before the string in order to center it
 *              in a `n` long screen
 *
 * @param   {String}    s       The string to center
 * @param   {uint8_t}   l       The screen width
 *
 * @result  {String}
 */
String center(String s, uint8_t l) {
    uint8_t size = s.length();
    if (size < l) {
        //add half of the offset before the string
        uint8_t offset = (l - size) / 2;
        s = repeat(' ', offset) + s;
    }

    return s;
};

/**
 * buildMessage()
 *
 * @abstract    An helper to build station or title strings
 * @discussion  It takes the good part of the message in order to make it
 *              "rotating"
 *
 * @param   {String}    message     The full message
 * @param   {uint8_t}   l           The screen width
 * @param   {&uint8_t}  &counter    The position in the message
 *
 * @return  {String}
 */
String buildMessage(String message, uint8_t l, uint8_t &counter) {

    uint8_t s = message.length();

    //if message if smaller than the screen width, reset counter to 0
    //and return the message as it is
    if (message.length() < l) {
        counter = 0;
    }
    //we have to take a part of the message
    else {
        //reset the counter to zero if we are at the end
        if (s - counter < SPACE_END) {
            counter = 0;
        }

        message = message.substring(counter, counter + l); //part of the message
        counter += 2; //next time, move to right
    }

    return message;
};

RADIO_LCD::RADIO_LCD(uint8_t pin) {
    _pin            = pin;
    _offsetStation  = 0;
    _offsetTitle    = 0;
};

void RADIO_LCD::init() {

    byte charHP[8] = { //HP
        B00000,
        B00010,
        B00110,
        B01110,
        B01110,
        B00110,
        B00010,
        B00000
    };

    byte charHPMute[8] = { //HP mute
        B00000,
        B01010,
        B00100,
        B01010,
        B00000,
        B11111,
        B01110,
        B00100
    };


    byte charSmileyHappy[8] = { //smiley happy
        B00000,
        B00000,
        B10001,
        B00000,
        B00000,
        B10001,
        B01110,
        B00000,
    };

    byte charSmileySad[8] = { //smiley sad
        B00000,
        B00000,
        B10001,
        B00000,
        B00000,
        B01110,
        B10001,
        B00000
    };

    byte charLight[8] = {
        B00000,
        B01110,
        B10001,
        B10001,
        B01010,
        B01110,
        B00100,
        B00000
    };

    byte charHeart[8] = {
        B00000,
        B00000,
        B01010,
        B11111,
        B11111,
        B01110,
        B00100,
        B00000
    };

    _lcd.begin(16,2); // initialize the _lcd.for 16 chars 2 lines, turn on backlight

    //add new custom characters
    _lcd.createChar(0, charHP);
    _lcd.createChar(1, charHPMute);
    _lcd.createChar(2, charSmileyHappy);
    _lcd.createChar(3, charSmileySad);
    _lcd.createChar(4, charLight);
    _lcd.createChar(5, charHeart);

    _lcd.home(); //go to (0,0)
    _lcd.clear();
};

void RADIO_LCD::flash() {
    _lcd.backlight();
    delay(250);
    _lcd.noBacklight();
    delay(250);
    _lcd.backlight();
};

void RADIO_LCD::hello() {
    _lcd.clear();
    _lcd.setCursor(0, 1);
    _lcd.print(F("Hello "));
    _lcd.write(2); //smiley
};

String RADIO_LCD::_prepareFrequency(uint16_t &frequency) {
    //we tranform frequency to String
    //add a dot, and remove last 0
    String tmp = String(frequency);
    uint8_t l = tmp.length();

    //return _frequency;
    return tmp.substring(0, l - 2) + '.' + tmp[l-2];
};

String RADIO_LCD::_buildTitle(String &message) {

    if (message.length() >= TITLE_WIDTH) {
        return buildMessage(
            repeat(' ', TITLE_WIDTH) + message,
            TITLE_WIDTH,
            _offsetTitle
        );
    } else {
        //if message is smaller than the screen, center it
        return center(message, TITLE_WIDTH);
    }
};

void RADIO_LCD::setBacklight(uint8_t n) {
    if (n > MAX_BACKLIGHT) {
        n = MAX_BACKLIGHT;
    }
    if (n < 1) {
        n = 1;
    }
    _backlight = n;
    analogWrite(_pin, n * BACKLIGHT_STEP); //set backlight value
};

uint8_t RADIO_LCD::backlightUp() {
    if (_backlight < MAX_BACKLIGHT) {
        _backlight += 1;
        analogWrite(_pin, _backlight * BACKLIGHT_STEP); //set backlight value
    }
    return _backlight;
};

uint8_t RADIO_LCD::backlightDown() {
    if (_backlight > 1) { //don't power off the screen
        _backlight -= 1;
        analogWrite(_pin, _backlight * BACKLIGHT_STEP); //set backlight value
    }
    return _backlight;
};

void RADIO_LCD::_showTitleScreen(
        uint8_t &mode,
        String  &title,
        String  &station,
        String  &frequency,
        bool    &mute,
        uint8_t &volume,
        uint8_t &rssi,
        uint8_t &movingStation,
        uint8_t &indexPref
    ) {

    _lcd.clear();
    _lcd.home();
    //station top left corner
    if (title.length() == 0 || movingStation > 0) { //if no title or moving station,
                                             //show frequency here
        _lcd.print(buildMessage(frequency, STATION_WIDTH, _offsetStation));
    } else {
        _lcd.print(buildMessage(station, STATION_WIDTH, _offsetStation));
    }

    _lcd.setCursor(12, 0);
    if (rssi < BAD_RECEPTION) {
        _lcd.write(3); //smiley sad if bad reception
    } else {
       _lcd.print(' ');
    }

    if (mode == NORMAL_MODE) {
        if (mute) { //mute
            _lcd.print("  ");
            _lcd.write(1); //icon mute
        } else {    //volume
            _lcd.write((uint8_t)0); //icon sound
            if (volume < 10) {
                _lcd.print(0);
            }
            _lcd.print(volume);
        }
    } else if (mode == PREF_MODE) {
        if (indexPref == 0) {
            _lcd.setCursor(15, 0);
            _lcd.write(5); //heart icon
        } else {
            _lcd.write(5); //heart icon
            if (indexPref < 10) {
                _lcd.print(0);
            }
            _lcd.print(indexPref);
        }
    } else if (mode == BACKLIGHT_MODE) {
        _lcd.write(4); //light icon
        if (_backlight < 10) {
            _lcd.print(0);
        }
        _lcd.print(_backlight);
    } else if (mode == SEEK_MODE) {
        _lcd.setCursor(14, 0);
        _lcd.write(127); //arrows
        _lcd.write(126);
    } else if (mode == SEARCH_MODE) {
        _lcd.setCursor(14, 0);
        _lcd.print("<>");
    }

    _lcd.setCursor(0, 1); //title on second row
    if (title.length() == 0 || movingStation > 0) { //if no title or moving station
        if (station.length() == 0) {
            _lcd.print(_buildTitle(frequency)); //if nothing, show frequency
        } else {
            _lcd.print(_buildTitle(station)); //show station name here
        }
    } else {
        _lcd.print(_buildTitle(title));
    }

}

void RADIO_LCD::_showVolumeScreen(bool &mute, uint8_t &volume) {

    _lcd.clear();
    _lcd.home();

    if (mute) { //if mute, show mute icon
        _lcd.setCursor(7, 1);
        _lcd.write(1);
    } else { //else show volume
        //show volume ratio on first row
        String message;
        message += volume;
        message += '/';
        message += MAX_VOLUME;
        _lcd.print(center(message, TITLE_WIDTH));

        //show range on second row
        _lcd.setCursor(0, 1);
        _lcd.write((uint8_t)0); //sound icon
        for (uint8_t i=0; i < volume; i++) {
            _lcd.write(255); //block character
        }
    }
};

void RADIO_LCD::_showInfoScreen(
        String  &frequency,
        String  &station,
        uint8_t &rssi
    ) {

    _lcd.clear();
    _lcd.home();

    //first row, top left corner, show frequency
    _lcd.print(frequency);

    //first row, top right corner, show rssi value
    _lcd.setCursor(9, 0);
    _lcd.print(F("RSSI:"));
    _lcd.print(rssi);

    //second row, show station name
    _lcd.setCursor(0, 1);
    if (station.length() == 0) {
        _lcd.print('?');
    } else {
        _lcd.print(buildMessage(station, TITLE_WIDTH, _offsetStation));
    }
};

void RADIO_LCD::_showSearchScreen(
        bool   up,
        String &frequency
    ) {

    _lcd.clear();
    _lcd.home();

    _lcd.setCursor(0, 1); //second row
    String message;
    if (up) {
        message = "  " + frequency;
        message += " >";
    } else {
        message = "< " + frequency;
        message += "  ";
    }
    _lcd.print(center(message, TITLE_WIDTH));
};

void RADIO_LCD::_showLightScreen() {

    _lcd.clear();
    _lcd.home();

    //first row, show backlight ratio
    String message;
    message += _backlight;
    message += '/';
    message += MAX_BACKLIGHT;
    _lcd.print(center(message, TITLE_WIDTH));

    //second row, show backlight range
    _lcd.setCursor(0, 1);
    _lcd.write(4); //light icon
    for (uint8_t i=0; i < _backlight; i++) {
        _lcd.write(255); //block character
    }
};

void RADIO_LCD::show(
        uint8_t  &mode,
        uint8_t  &screen,
        bool     active,
        bool     mute,
        uint8_t  &rssi,
        uint8_t  volume,
        uint16_t frequency,
        String   station,
        String   title,
        uint8_t  &movingStation,
        uint8_t  indexPref
    ) {

    if (!active) { //smiley sad if something wrong
        _lcd.setCursor(0, 1);
        _lcd.write(3);
        return;
    }

    String _frequency = _prepareFrequency(frequency);
    title.trim();
    station.trim();

    //Title screen
    if (screen == TITLE_SCREEN) {
        _showTitleScreen(mode, title, station, _frequency, mute, volume,
                rssi, movingStation, indexPref);
    }

    //Volume screen
    else if (screen == VOLUME_SCREEN) {
        _showVolumeScreen(mute, volume);
    }

    //Info screen
    else if (screen == INFO_SCREEN) {
        _showInfoScreen(_frequency, station, rssi);
    }

    //Search screens
    else if (screen == SEARCH_SCREEN_UP) {
        _showSearchScreen(true, _frequency);
    }
    else if (screen == SEARCH_SCREEN_DOWN) {
        _showSearchScreen(false, _frequency);
    }

    //Light screen
    else if (screen == LIGHT_SCREEN) {
        _showLightScreen();
    }
};
