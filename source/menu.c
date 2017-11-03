#include "draw.h"
#include "hid.h"
#include "i2c.h"
#include "ntrcard.h"

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
        if (data != 0x00 && data != 0xFF) {
            if (data == '\n' || data == '\r') {
                ++lineno;
                charno = 1;
                continue;
            }
            DrawCharacter(TOP_SCREEN, data, charno++ * 10, lineno * 10, STD_COLOR_FONT, STD_COLOR_BG);
        }
    }
}
