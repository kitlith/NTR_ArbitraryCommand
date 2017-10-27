#pragma once

#include "gamecart/card_ntr.h"
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

void ResetCartSlot(void);
void SwitchToNTRCARD(void);
void Cart_Init(void);
