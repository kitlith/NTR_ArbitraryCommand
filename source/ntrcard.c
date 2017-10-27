#include "ntrcard.h"

void ResetCartSlot(void)
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

void SwitchToNTRCARD(void)
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
