
/**
 * resetMemory()
 *
 * @function
 * @abstract    Reset EEPROM memory used by the project
 */
void resetMemory() {
    //reset first adresses
    EEPROM.write(LIGHT_ADDR, 0);
    EEPROM.write(VOLUME_ADDR, 0);
    EEPROM.write(MUTE_ADDR, 0);
    writeInt16EEPROM(FREQ_ADDR, 0);

    //go over preferences addresses and erase them with 0
    for(int addr=PREF_ADDR; addr < PREF_ADDR + PREF_LENTGH * 2; addr += 2) {
        writeInt16EEPROM(addr, 0);
    }
};

/**
 * loadPreferences()
 *
 * @function
 * @abstract    Load preferences from EEPROM to `preferences` array
 */
void loadPreferences() {
    uint8_t i = 0; //index of pref in `preference`
    //go over EEPROM addresses
    for(int addr=PREF_ADDR; addr < PREF_ADDR + PREF_LENTGH * 2; addr += 2) {

        //save the pref
        preferences[i] = readInt16EEPROM(addr);

        Serial.print(F("Pref "));
        Serial.print(i);
        Serial.print(F(" : "));
        Serial.println(preferences[i]);
        i++;
    }
};

/**
 * savePreference()
 *
 * @function
 * @abstract    Save preference at the end of the preferences area in EEPROM
 *
 * @return  {int8_t}    index in `preferences` array or -1 if no more space
 */
int8_t savePreference(uint16_t item) {
    if (preferences[PREF_LENTGH] > 0) { //preferences array is full
                                        //so is EEPROM area
        return -1;

    //it's ok, save pref in array and on EEPROM
    } else {
        int i = 0;
        while(preferences[i] > 0) { //find the first empty pref
            i++;
        }
        //save item here and write it to EEPROM
        preferences[i] = item;
        writeInt16EEPROM(PREF_ADDR + i * 2, item);
        return i;
    }
};

/**
 * removePreference()
 *
 * @function
 * @abstract    Remove this preference
 *
 * @param   {int}   index in `preferences` array
 */
void removePreference(int index) {
    //check if index is ok first
    if (index > 0 && index < PREF_LENTGH) {
        //we need to shift preferences to left
        int addr = PREF_ADDR + index * 2; //addr of the pref on EEPROM
        int i = index + 1; //index of the next pref in `preferences`
        while(preferences[i] > 0) {
            //switch place
            preferences[i-1] = preferences[i];
            writeInt16EEPROM(addr, preferences[i]);
            addr += 2;
        }

        //erase last place
        preferences[i] = 0;
        writeInt16EEPROM(addr, 0);
    }
};
