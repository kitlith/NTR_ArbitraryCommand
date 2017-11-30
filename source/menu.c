#include "draw.h"
#include "hid.h"
#include "i2c.h"
#include "ntrcard.h"
#include "../minmea/minmea.h"

void arbitrary_menu(void) {
    char clear_text[64] = { 0 };
    uint8_t command[8] = {0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
    uint8_t response[4] = {0};
    uint8_t value;
    int cursor_pos = 0;

    memset(clear_text, (int) ' ', 16);
    while( true ) {
        DrawStringF(10, 10, true, "%02X%02X%02X%02X%02X%02X%02X%02X",
                        command[0], command[1], command[2], command[3],
                        command[4], command[5], command[6], command[7]);
        DrawStringF(10, 20, true, clear_text);
        DrawStringF(10 + 8*cursor_pos, 20, true, "^");
        DrawStringF(10, 40, true, "%02X%02X%02X%02X",
                    response[0], response[1], response[2], response[3]);

        u32 pad_state = InputWait();
        if (pad_state & (BUTTON_B))
            break;
        if (pad_state & BUTTON_A) {
            cardPolledTransfer(CARD_ACTIVATE | CARD_BLK_SIZE(7), (uint32_t*)response, 4, command);
        }

        if (pad_state & BUTTON_UP) {
            unsigned byte_addr = cursor_pos >> 1;
            unsigned shift = (cursor_pos & 1) ? 0 : 4;
            value = (command[byte_addr] >> shift) & 0xF;
            ++value;
            command[byte_addr] = (command[byte_addr] & (0xF0 >> shift)) |
                                 (value & 0x0F) << shift;
        }
        else if (pad_state & BUTTON_DOWN) {
            unsigned byte_addr = cursor_pos >> 1;
            unsigned shift = (cursor_pos & 1) ? 0 : 4;
            value = (command[byte_addr] >> shift) & 0xF;
            --value;
            command[byte_addr] = (command[byte_addr] & (0xF0 >> shift)) |
                                 (value & 0x0F) << shift;
        }

        else if (pad_state & BUTTON_LEFT)
            cursor_pos = (cursor_pos > 0) ? cursor_pos - 1 : 2*8-1;
        else if (pad_state & BUTTON_RIGHT)
            cursor_pos = (cursor_pos < 2*8-1) ? cursor_pos + 1 : 0;
    }
}

static const uint8_t cmdReadSerial[8] = {0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

void serial_menu(void) {
    unsigned charno = 1;
    unsigned lineno = 1;
    while (true) {
        if (CheckButton(BUTTON_B))
            break;

        uint8_t result[4];
        cardPolledTransfer(CARD_ACTIVATE | CARD_BLK_SIZE(7), (uint32_t *)result, 4, cmdReadSerial);
        // bool data_present = result[0] & 1;
        uint8_t data = result[3];
        if (data == 0x00 || data == 0xFF) continue;

        if (data == '\n' || data == '\r') {
            ++lineno;
            charno = 1;
            continue;
        }
        DrawCharacter(TOP_SCREEN, data, charno++ * 10, lineno * 10, STD_COLOR_FONT, STD_COLOR_BG);

    }
}

static int normalize(double *val) {
    int exponent = 0;
    double value = *val;

    while (value >= 1.0) {
        value /= 10.0;
        ++exponent;
    }

    while (value < 0.1) {
        value *= 10.0;
        --exponent;
    }
    *val = value;
    return exponent;
}

static char *ftoa_fixed(char *buffer, double value) {
    /* carry out a fixed conversion of a double value to a string, with a precision of 5 decimal digits.
     * Values with absolute values less than 0.000001 are rounded to 0.0
     * Note: this blindly assumes that the buffer will be large enough to hold the largest possible result.
     * The largest value we expect is an IEEE 754 double precision real, with maximum magnitude of approximately
     * e+308. The C standard requires an implementation to allow a single conversion to produce up to 512
     * characters, so that's what we really expect as the buffer size.
     */

    char *ret = buffer;

    int exponent = 0;
    int places = 0;
    static const int width = 4;

    if (value == 0.0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return ret;
    }

    if (value < 0.0) {
        *buffer++ = '-';
        value = -value;
    }

    exponent = normalize(&value);

    while (exponent > 0) {
        int digit = value * 10;
        *buffer++ = digit + '0';
        value = value * 10 - digit;
        ++places;
        --exponent;
    }

    if (places == 0)
        *buffer++ = '0';

    *buffer++ = '.';

    while (exponent < 0 && places < width) {
        *buffer++ = '0';
        --exponent;
        ++places;
    }

    while (places < width) {
        int digit = value * 10.0;
        *buffer++ = digit + '0';
        value = value * 10.0 - digit;
        ++places;
    }
    *buffer = '\0';

    return ret;
}

void gps_menu(void) {
    char sentence[0x200];
    char float1[0x200];
    char float2[0x200];
    int sentence_pos = 0;
    while (true) {
        if (CheckButton(BUTTON_B))
            break;

        uint8_t result[4];
        cardPolledTransfer(CARD_ACTIVATE | CARD_BLK_SIZE(7), (uint32_t *)result, 4, cmdReadSerial);
        char data = result[3];
        if (data == 0x00 || data == 0xFF) continue;

        sentence[sentence_pos++] = data;
        if (sentence_pos >= 0x200) {
            sentence_pos = 0; // Avoid stack overflow.
            continue;
        }

        if (data == '\n') {
            sentence[sentence_pos] = '\0';
            sentence_pos = 0;

            switch (minmea_sentence_id(sentence, false)) {
                case MINMEA_SENTENCE_RMC: {
                    struct minmea_sentence_rmc frame;
                    if (minmea_parse_rmc(&frame, sentence)) {
                        DrawStringF(10, 10, true, "Latitude: %s", ftoa_fixed(float1, minmea_tocoord(&frame.latitude)));
                        DrawStringF(10, 20, true, "Longitude: %s", ftoa_fixed(float1, minmea_tocoord(&frame.longitude)));
                        DrawStringF(10, 30, true, "Speed: %s", ftoa_fixed(float1, minmea_tofloat(&frame.speed)));

                        DrawStringF(10, 60, true, "Valid: %s", frame.valid ? "Yes" : "No");
                    }
                } break;

                case MINMEA_SENTENCE_GGA: {
                    struct minmea_sentence_gga frame;
                    if (minmea_parse_gga(&frame, sentence)) {
                        DrawStringF(10, 40, true, "Altitude: %s %c, Height: %s %c",
                            ftoa_fixed(float1, minmea_tofloat(&frame.altitude)), frame.altitude_units,
                            ftoa_fixed(float2, minmea_tofloat(&frame.height)), frame.height_units);
                    }
                } break;
                case MINMEA_SENTENCE_GSA:
                case MINMEA_SENTENCE_VTG:
                default: break;
            }

            DrawStringF(10,70,true, "Sentence recieved!");
        }

    }
}
