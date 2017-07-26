#include "common.h"
#include "draw.h"
#include "hid.h"
#include "i2c.h"
#include "gamecart/card_ntr.c"
#include "gamecart/ndscard.h"

#define REG_NTRCARDMCNT    (*(vu16*)0x10164000)
#define REG_NTRCARDMDATA   (*(vu16*)0x10164002)
#define REG_NTRCARDROMCNT  (*(vu32*)0x10164004)
#define REG_NTRCARDCMD     ((vu8*)0x10164008)
#define REG_CARDCONF       (*(vu16*)0x1000000C)
#define REG_CARDCONF2      (*(vu8*)0x10000010)
#define REG_NTRCARDFIFO    (*(vu32*)0x1016401C)

#define NTRCARD_CR1_ENABLE  0x8000u
#define NTRCARD_CR1_IRQ     0x4000u
#define NTRCARD_SEC_SEED     (1u<<15)
#define NTRCARD_DATA_READY   (1u<<23)
#define NTRCARD_nRESET       (1u<<29)
#define NTRCARD_BUSY         (1u<<31)
#define REG_CTRCARDSECCNT  (*(vu32*)0x10004008)


static void ResetCartSlot(void)
{
    REG_CARDCONF2 = 0x0C;
    REG_CARDCONF &= ~3;

    if (REG_CARDCONF2 == 0xC) {
        while (REG_CARDCONF2 != 0);
    }

    if (REG_CARDCONF2 != 0)
        return;

    REG_CARDCONF2 = 0x4;
    while(REG_CARDCONF2 != 0x4);

    REG_CARDCONF2 = 0x8;
    while(REG_CARDCONF2 != 0x8);
}

static void SwitchToNTRCARD(void)
{
    REG_NTRCARDROMCNT = 0x20000000;
    REG_CARDCONF &= ~3;
    REG_CARDCONF &= ~0x100;
    REG_NTRCARDMCNT = NTRCARD_CR1_ENABLE;
}

void Cart_Init(void)
{
    ResetCartSlot(); //Seems to reset the cart slot?

    REG_CTRCARDSECCNT &= 0xFFFFFFFB;
    ioDelay(0x40000);

    SwitchToNTRCARD();
    ioDelay(0x40000);

    REG_NTRCARDROMCNT = 0;
    REG_NTRCARDMCNT &= 0xFF;
    ioDelay(0x40000);

    REG_NTRCARDMCNT |= (NTRCARD_CR1_ENABLE | NTRCARD_CR1_IRQ);
    REG_NTRCARDROMCNT = NTRCARD_nRESET | NTRCARD_SEC_SEED;
    while (REG_NTRCARDROMCNT & NTRCARD_BUSY);

    // Reset
    // I don't want to send extra commands.
    // NTR_CmdReset();
    // ioDelay(0x40000);
    // CartID = NTR_CmdGetCartId();
    //
    // // 3ds
    // if (CartID & 0x10000000) {
    //     u32 unknowna0_cmd[2] = { 0xA0000000, 0x00000000 };
    //     NTR_SendCommand(unknowna0_cmd, 0x4, 0, &A0_Response);
    //
    //     NTR_CmdEnter16ByteMode();
    //     SwitchToCTRCARD();
    //     ioDelay(0xF000);
    //
    //     REG_CTRCARDBLKCNT = 0;
    // }
}

void Reboot()
{
    i2cWriteRegister(I2C_DEV_MCU, 0x20, 1 << 2);
    while(true);
}


void PowerOff()
{
    i2cWriteRegister(I2C_DEV_MCU, 0x20, 1 << 0);
    while (true);
}

u8 *top_screen, *bottom_screen;

int main(int argc, char** argv)
{
    // Fetch the framebuffer addresses
    if(argc >= 2) {
        // newer entrypoints
        u8 **fb = (u8 **)(void *)argv[1];
        top_screen = fb[0];
        bottom_screen = fb[2];
    } else {
        // outdated entrypoints
        top_screen = (u8*)(*(u32*)0x23FFFE00);
        bottom_screen = (u8*)(*(u32*)0x23FFFE08);
    }


    char clear_text[64] = { 0 };
    bool use_top = true;

    int cursor_pos = 0;
    uint8_t command[8] = {0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
    uint_fast8_t value;

    memset(clear_text, (int) ' ', 16);

    Cart_Init();

    while( true ) {
        DrawStringF(10, 10, use_top, "%02X%02X%02X%02X%02X%02X%02X%02X",
                        command[0], command[1], command[2], command[3],
                        command[4], command[5], command[6], command[7]);
        DrawStringF(10, 20, use_top, clear_text);
        DrawStringF(10 + 8*cursor_pos, 20, use_top, "^");
        u32 pad_state = InputWait();
        if (pad_state & (BUTTON_B | BUTTON_START))
            break;
        if (pad_state & BUTTON_A)
            cardPolledTransfer(CARD_ACTIVATE, NULL, 0, command);
        if (pad_state & BUTTON_UP) {
            unsigned byte_addr = cursor_pos >> 1;
            unsigned shift = (cursor_pos & 1) ? 0 : 4;
            value = (command[byte_addr] >> shift) & 0xF;
            ++value;
            command[byte_addr] = (command[byte_addr] & (0xF0 >> shift)) |
                                 (value & 0x0F) << shift;
        }
        else if (pad_state & BUTTON_LEFT)
            cursor_pos = (cursor_pos > 0) ? cursor_pos - 1 : 0;
        else if (pad_state & BUTTON_DOWN) {
            unsigned byte_addr = cursor_pos >> 1;
            unsigned shift = (cursor_pos & 1) ? 0 : 4;
            value = (command[byte_addr] >> shift) & 0xF;
            --value;
            command[byte_addr] = (command[byte_addr] & (0xF0 >> shift)) |
                                 (value & 0x0F) << shift;
        }
        else if (pad_state & BUTTON_RIGHT)
            cursor_pos = (cursor_pos < 2*8-1) ? cursor_pos + 1 : 0;
    }

    PowerOff();
    return 0;
}
