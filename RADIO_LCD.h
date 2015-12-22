#include "Arduino.h"

//modes
#define NORMAL_MODE         0
#define PREF_MODE           1
#define SEEK_MODE           2
#define SEARCH_MODE         3
#define BACKLIGHT_MODE      4
#define MAX_MODE            4

//screens
#define TITLE_SCREEN        0
#define VOLUME_SCREEN       1
#define INFO_SCREEN         3
#define SEARCH_SCREEN_UP    4
#define SEARCH_SCREEN_DOWN  5
#define LIGHT_SCREEN        6

/**
 * @class
 * @abstract    The class to manage the display
 */
class RADIO_LCD {
    public:
        /**
         * RADIO_LCD()
         *
         * @method
         *
         * @param   {uint8_t}   pin     Adress of the lcd led backlight
         */
        RADIO_LCD(uint8_t pin);

        /**
         * init()
         *
         * @method
         * @abstract    Init the lcd
         * @discussion  Add custom characters, init, etc.
         */
        void init();

        /**
         * flash()
         *
         * @method
         * @abstract    Blink the lcd once
         */
        void flash();

        /**
         * hello()
         *
         * @method
         * @abstract    Display hello message.
         */
        void hello();

        /**
         * show()
         *
         * @method
         * @abstract    Method to call in order to update the screen
         *
         * @param   {uint8_t}   mode
         * @param   {uint8_t}   screen
         * @param   {bool}      active          Radio state
         * @param   {bool}      mute
         * @param   {uint8_t}   rssi            RSSI value
         * @param   {uint8_t}   volume
         * @param   {uint8_t}   frequency
         * @param   {String}    station         Message on RDS stream
         * @param   {String}    title           Message on RDS title stream
         * @param   {uint8_t}   movingStation   If the station is "moving".
         * @param   {uint8_t}   indexPref       Index of the preference if
         *                                      frequency is in prefs
         */
        void show(
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
        );

        /**
         * setBacklight()
         *
         * @method
         * @abstract    Set the backlight value
         *
         * @param   {uint8_t}   n   in [0, 15]
         */
        void setBacklight(uint8_t n);

        /**
         * backlightUp()
         *
         * @method
         * @abstract    Increase backlight
         *
         * @return   {uint8_t}      Backlight value
         */
        uint8_t backlightUp();

        /**
         * backlightDown()
         *
         * @method
         * @abstract    Decrease backlight
         *
         * @return   {uint8_t}      Backlight value
         */
        uint8_t backlightDown();

    private:
        uint8_t _pin;       //lcd backlight pin
        uint8_t _backlight; //lcd backlight value

        //offset station and title values for rotating texts
        uint8_t  _offsetStation;
        uint8_t  _offsetTitle;

        bool _rotatingStation;  //true if the station is rotating

        unsigned long _lastScreeChange; //last time the screen option was setted

        /**
         * _prepareFrequency()
         *
         * @private
         * @method
         * @abstract    Transform frequency to string, remove last 0, add dot
         *
         * @param   {uint16_t}  frequency
         */
        String _prepareFrequency(uint16_t &frequency);

        /**
         * _buildTitle()
         *
         * @private
         * @method
         * @abstract    Build the title string.
         *
         * @param   {String}    message
         */
        String _buildTitle(String &message);

        /**
         * _showTitleScreen()
         *
         * @private
         * @method
         *
         * @param   {String}    title
         * @param   {String}    station
         * @param   {String}    frequency
         * @param   {bool}      mute
         * @param   {uint8_t}   rssi
         * @param   {uint8_t}   volume
         * @param   {uint8_t}   movingStation
         * @param   {uint8_t}   indexPref
         */
        void _showTitleScreen(
            uint8_t &mode,
            String  &title,
            String  &station,
            String  &frequency,
            bool    &mute,
            uint8_t &volume,
            uint8_t &rssi,
            uint8_t &movingStation,
            uint8_t &indexPref
        );

        /**
         * _showVolumeScreen()
         *
         * @private
         * @method
         *
         * @param   {bool}      mute
         * @param   {uint8_t}   volume
         */
        void _showVolumeScreen(
            bool    &mute,
            uint8_t &volume
        );

        /**
         * _showInfoScreen()
         *
         * @private
         * @method
         *
         * @param   {String}    frequency
         * @param   {String}    station
         * @param   {uint8_t}   rssi
         */
        void _showInfoScreen(
            String  &frequency,
            String  &station,
            uint8_t &rssi
        );

        /**
         * _showSearchScreen()
         *
         * @private
         * @method
         *
         * @param   {bool}      up          Search up or down ?
         * @param   {String}    frequency
         */
        void _showSearchScreen(
            bool   up,
            String &frequency
        );

        /**
         * _showLightScreen()
         *
         * @private
         * @method
         */
        void _showLightScreen();
};
