#include "common.h"
#include "draw.h"
#include "hid.h"
#include "i2c.h"
#include "ntrcard.h"

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

struct menu_item {
    const char *name;
    void (*func)(void);
};

void arbitrary_menu(void);
void serial_menu(void);

static const struct menu_item main_menu[] = {
    {.name = "Arbitrary Commands", .func = arbitrary_menu},
    {.name = "Serial Recieve", .func = serial_menu},
};

static const unsigned menu_length = sizeof(main_menu)/sizeof(main_menu[0]);

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
        top_screen = (u8*)(*(uintptr_t*)0x23FFFE00);
        bottom_screen = (u8*)(*(uintptr_t*)0x23FFFE08);
    }

    Cart_Init();

    unsigned menu_index = 0;
    while (true) {
        for (unsigned draw_index = 0; draw_index < menu_length; ++draw_index) {
            DrawString(TOP_SCREEN, main_menu[draw_index].name, 10, 10 * (draw_index + 1),
                       (draw_index == menu_index) ? COLOR_RED : STD_COLOR_FONT, STD_COLOR_BG);
        }

        u32 pad_state = InputWait();
        if (pad_state & (BUTTON_START))
            break;
        if (pad_state & BUTTON_A && main_menu[menu_index].func != NULL) {
            ClearScreenFull(true, true);
            main_menu[menu_index].func();
            ClearScreenFull(true, true);
        }

        if (pad_state & BUTTON_DOWN) {
            ++menu_index;
            if (main_menu[menu_index].name == NULL) menu_index = 0;
        }
        if (pad_state & BUTTON_UP) {
            --menu_index;
            if (main_menu[menu_index].name == NULL) menu_index = menu_length - 1;
        }

    }

    PowerOff();
    return 0;
}
