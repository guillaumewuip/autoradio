/**
 * indexOfArray()
 *
 * @function
 * @abstract    Return index of element or -1
 *
 * @param   {uint16_t[]}  array
 * @param   {int}         length        Array's length
 * @param   {uint16_t}    element       What to look for in array
 *
 * @return {uint8_t}
 */
uint8_t indexOfArray(uint16_t array[], int length, uint16_t element) {
    for (int i = 0; i < length; i++) {
        if (array[i] == element) {
            return i;
        }
    }
    return -1;
};

/**
 * readInt16EEPROM()
 *
 * @function
 * @abstract    Read two byte in EEPROM and return the uint16_t
 *              value that goes with
 * @discussion  EEPROM.get() and EEPROM.put() seem to not really like uint16_t
 *              so here's a custom function for that.
 *
 * @param   {int}   addr    Address in EEPROM where to look for the first byte
 *                          (assuming the second is following)
 *
 * @return  {int16_t}
 */
uint16_t readInt16EEPROM(int addr) {
    uint8_t part1;
    uint8_t part2;

    EEPROM.get(addr, part1);
    EEPROM.get(addr + 1, part2);
    return (part1 << 8) + part2;
}

/**
 * writeInt16EEPROM()
 *
 * @function
 * @abstract    Write an uint16_t in two EEPROM bytes
 * @discussion  EEPROM.get() and EEPROM.put() seem to not really like uint16_t
 *              so here's a custom function for that.
 *
 * @param   {int}       addr    Where to write the first byte (assuming the second
 *                              is following)
 * @param   {uint16_t}  item    What to write
 */
void writeInt16EEPROM(int addr, uint16_t item) {
    //cut the uint16_t into two
    uint8_t part1 = item >> 8;
    uint8_t part2 = item & 0b0000000011111111;
    //write
    EEPROM.update(addr, part1);
    EEPROM.update(addr + 1, part2);
};

