#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include "usbplayback/menu.h"
#include "usbplayback/usbplayback.h"
#include "main.h"
#include "fatfs.h"
#include "ssd1306/ssd1306.h"
#include "usbd_cdc_if.h"
#include "n64.h"
#include "TASRun.h"
#include "stm32f4xx_it.h"
#include "serial_interface.h"

extern uint32_t readcount;

MenuType CurrentMenu;

int16_t cursorPos = 0;
int16_t displayPos = 0;
bool USBok = 0;
extern uint8_t firstLatch;

char currentFilename[256];

FATFS TASDrive;

unsigned long stepNextThink = 0;

extern uint32_t menuNextThink;

// Input menu event handlers
void Menu_Enter() {

	switch (CurrentMenu) {

	case MENUTYPE_BROWSER:
		USB_Start_Tas(&currentFilename[0]);
		CurrentMenu = MENUTYPE_TASINPUTS;
		break;
	case MENUTYPE_TASINPUTS:
	case MENUTYPE_TASSTATS:
		USB_Stop_TAS();
		CurrentMenu = MENUTYPE_BROWSER;
		break;
	}
}

void Menu_Up() {
	if (CurrentMenu == MENUTYPE_BROWSER) {
		if (cursorPos > 0)
			cursorPos--;
	}
}

void Menu_Down() {
	if (CurrentMenu == MENUTYPE_BROWSER) {
		cursorPos++;
	}
}

void Menu_HoldUp() {
	if (uwTick > stepNextThink) {
		Menu_Up();
		stepNextThink = uwTick + HOLDDELAY;
	}
}

void Menu_HoldDown() {
	if (uwTick > stepNextThink) {
		Menu_Down();
		stepNextThink = uwTick + HOLDDELAY;
	}
}

void Menu_Settings(){
	if (CurrentMenu == MENUTYPE_TASINPUTS)
		CurrentMenu = MENUTYPE_TASSTATS;
	else if (CurrentMenu == MENUTYPE_TASSTATS)
		CurrentMenu = MENUTYPE_TASINPUTS;
}


void Menu_Display() {
	static char temp[23];
	static FRESULT res;
	static DIR dir;
	static char path[2] = "/";
	static FILINFO fno;

	unsigned char lineNo = 0;

	switch (CurrentMenu) {
	case MENUTYPE_BROWSER:

		// if USB host initiated run, switch menu
		if (tasrun->initialized){
			CurrentMenu = MENUTYPE_TASINPUTS;
			break;
		}

		if (USBok) {
			res = f_opendir(&dir, &path[0]);

			ssd1306_Fill(Black);
			if (res == FR_OK) {
				if (cursorPos > (displayPos + 7)) {
					displayPos = cursorPos - 7;
				} else if (cursorPos < displayPos) {
					displayPos = cursorPos;
				}

				//Iterate through every directory entry till we get to the ones we need
				// Perhaps could cache this but would require a lot of RAM
				for (uint16_t cnt = 0; cnt < displayPos + DISPLAYLINES; cnt++) {

					res = f_readdir(&dir, &fno);

					// Scroll backwards if we've reached the end of the folder
					if (res != FR_OK || fno.fname[0] == 0) {
						if (displayPos > 0) {
							displayPos--;
							cursorPos--;
						}
						if (cursorPos > cnt - 1)
							cursorPos = cnt - 1;

						break;
					}

					// Skip directories
					if (fno.fattrib & AM_DIR){
						cnt--;
						continue;
					}
					if (cnt >= displayPos) {
						SSD1306_COLOR textColor = White;
						if (cnt == cursorPos) {
							textColor = Black;
							strcpy(currentFilename, fno.fname[0] == '\0' ? fno.altname : fno.fname);
						}
						ssd1306_SetCursor(0, lineNo * 8);
						ssd1306_WriteString(fno.fname[0] == '\0' ? fno.altname : fno.fname, Font_6x8, textColor);
						lineNo++;
					}
				}
				ssd1306_UpdateScreen();
			}
		} else {
			res = f_mount(&TASDrive, (TCHAR const*) USBHPath, 1);
			if (res == FR_OK) {
				USBok = 1;

			} else {
				ssd1306_Fill(Black);
				ssd1306_SetCursor(0, 0);
				sprintf(temp, "No USB");
				ssd1306_WriteString(temp, Font_16x26, White);
				ssd1306_UpdateScreen();
			}
		}
		break;

	case MENUTYPE_TASINPUTS:

		if (!tasrun->initialized) {
			CurrentMenu = MENUTYPE_BROWSER;
			break;
		}

		ssd1306_Fill(Black);

		RunDataArray *ct = tasrun->current;

		switch (tasrun->console) {

		case CONSOLE_NES:

			ssd1306_SetCursor(0, 25);
			ssd1306_WriteString("L", Font_16x26, ct[0][0]->nes_data.left ? Black : White);
			ssd1306_SetCursor(16, 12);
			ssd1306_WriteString("U", Font_16x26, ct[0][0]->nes_data.up ? Black : White);
			ssd1306_SetCursor(16, 38);
			ssd1306_WriteString("D", Font_16x26, ct[0][0]->nes_data.down ? Black : White);
			ssd1306_SetCursor(32, 25);
			ssd1306_WriteString("R", Font_16x26, ct[0][0]->nes_data.right ? Black : White);
			ssd1306_SetCursor(56, 25);
			ssd1306_WriteString("s", Font_16x26, ct[0][0]->nes_data.select ? Black : White);
			ssd1306_SetCursor(72, 25);
			ssd1306_WriteString("S", Font_16x26, ct[0][0]->nes_data.start ? Black : White);
			ssd1306_SetCursor(96, 25);
			ssd1306_WriteString("B", Font_16x26, ct[0][0]->nes_data.b ? Black : White);
			ssd1306_SetCursor(112, 25);
			ssd1306_WriteString("A", Font_16x26, ct[0][0]->nes_data.a ? Black : White);
			break;

		case CONSOLE_SNES:

			ssd1306_SetCursor(0, 25);
			ssd1306_WriteString("L", Font_16x26, ct[0][0]->snes_data.left ? Black : White);
			ssd1306_SetCursor(16, 12);
			ssd1306_WriteString("U", Font_16x26, ct[0][0]->snes_data.up ? Black : White);
			ssd1306_SetCursor(16, 38);
			ssd1306_WriteString("D", Font_16x26, ct[0][0]->snes_data.down ? Black : White);
			ssd1306_SetCursor(32, 25);
			ssd1306_WriteString("R", Font_16x26, ct[0][0]->snes_data.right ? Black : White);
			ssd1306_SetCursor(48, 25);
			ssd1306_WriteString("s", Font_16x26, ct[0][0]->snes_data.select ? Black : White);
			ssd1306_SetCursor(64, 25);
			ssd1306_WriteString("S", Font_16x26, ct[0][0]->snes_data.start ? Black : White);
			ssd1306_SetCursor(80, 25);
			ssd1306_WriteString("Y", Font_16x26, ct[0][0]->snes_data.y ? Black : White);
			ssd1306_SetCursor(96, 12);
			ssd1306_WriteString("X", Font_16x26, ct[0][0]->snes_data.x ? Black : White);
			ssd1306_SetCursor(112, 25);
			ssd1306_WriteString("A", Font_16x26, ct[0][0]->snes_data.a ? Black : White);
			ssd1306_SetCursor(96, 38);
			ssd1306_WriteString("B", Font_16x26, ct[0][0]->snes_data.b ? Black : White);
			ssd1306_SetCursor(0, 0);
			ssd1306_WriteString("(", Font_16x26, ct[0][0]->snes_data.l ? Black : White);
			ssd1306_SetCursor(112, 0);
			ssd1306_WriteString(")", Font_16x26, ct[0][0]->snes_data.r ? Black : White);
			break;

		case CONSOLE_GEN:

			ssd1306_SetCursor(0, 25);
			ssd1306_WriteString("L", Font_16x26, ct[0][0]->gen_data.left ? Black : White);
			ssd1306_SetCursor(16, 12);
			ssd1306_WriteString("U", Font_16x26, ct[0][0]->gen_data.up ? Black : White);
			ssd1306_SetCursor(16, 38);
			ssd1306_WriteString("D", Font_16x26, ct[0][0]->gen_data.down ? Black : White);
			ssd1306_SetCursor(32, 25);
			ssd1306_WriteString("R", Font_16x26, ct[0][0]->gen_data.right ? Black : White);
			ssd1306_SetCursor(56, 25);
			ssd1306_WriteString("S", Font_16x26, ct[0][0]->gen_data.start ? Black : White);
			ssd1306_SetCursor(72, 25);
			ssd1306_WriteString("A", Font_16x26, ct[0][0]->gen_data.a ? Black : White);
			ssd1306_SetCursor(96, 25);
			ssd1306_WriteString("B", Font_16x26, ct[0][0]->gen_data.b ? Black : White);
			ssd1306_SetCursor(112, 25);
			ssd1306_WriteString("C", Font_16x26, ct[0][0]->gen_data.c ? Black : White);

			break;

		case CONSOLE_GC:

			ssd1306_DrawRectangle(0, 15, 30, 45, White);
			ssd1306_DrawCircle(15, 30, 11, White);
			ssd1306_DrawPixel(15 + (ct[0][0]->gc_data.a_x_axis - 128) / 13 * 1.5, 30 - (ct[0][0]->gc_data.a_y_axis - 128) / 13 * 1.5, White);
			ssd1306_DrawRectangle(25 * 1.5, 15, 45 * 1.5, 45, White);
			ssd1306_DrawCircle(35 * 1.5, 30, 11, White);
			ssd1306_DrawPixel(35 * 1.5 + (ct[0][0]->gc_data.c_x_axis - 128) / 13 * 1.5, 30 - (ct[0][0]->gc_data.c_y_axis - 128) / 13 * 1.5, White);
			ssd1306_Line(0, 5 * 1.5, ct[0][0]->gc_data.l_trigger / 12.8 * 1.5, 5 * 1.5, White);
			ssd1306_Line(0, 5 * 1.5 + 1, 30 - 1, 5 * 1.5 + 1, ct[0][0]->gc_data.l ? White : Black);
			ssd1306_Line(45 * 1.5 - ct[0][0]->gc_data.r_trigger / 12.8 * 1.5, 5 * 1.5, 45 * 1.5, 5 * 1.5, White);
			ssd1306_Line(25 * 1.5 + 1, 5 * 1.5 + 1, 45 * 1.5, 5 * 1.5 + 1, ct[0][0]->gc_data.r ? White : Black);
			ssd1306_SetCursor(84, 0);
			ssd1306_WriteChar('U', Font_7x10, ct[0][0]->gc_data.d_up ? Black : White);
			ssd1306_SetCursor(75, 12);
			ssd1306_WriteChar('L', Font_7x10, ct[0][0]->gc_data.d_left ? Black : White);
			ssd1306_SetCursor(93, 12);
			ssd1306_WriteChar('R', Font_7x10, ct[0][0]->gc_data.d_right ? Black : White);
			ssd1306_SetCursor(84, 24);
			ssd1306_WriteChar('D', Font_7x10, ct[0][0]->gc_data.d_down ? Black : White);
			ssd1306_SetCursor(73 * 1.5, 15 * 1.5);
			ssd1306_WriteChar('A', Font_7x10, ct[0][0]->gc_data.a ? Black : White);
			ssd1306_SetCursor(67 * 1.5, 23 * 1.5);
			ssd1306_WriteChar('B', Font_7x10, ct[0][0]->gc_data.b ? Black : White);
			ssd1306_SetCursor(79 * 1.5, 15 * 1.5);
			ssd1306_WriteChar('X', Font_7x10, ct[0][0]->gc_data.x ? Black : White);
			ssd1306_SetCursor(73 * 1.5, 8 * 1.5);
			ssd1306_WriteChar('Y', Font_7x10, ct[0][0]->gc_data.y ? Black : White);
			ssd1306_SetCursor(79 * 1.5, 8 * 1.5);
			ssd1306_WriteChar('Z', Font_7x10, ct[0][0]->gc_data.z ? Black : White);
			ssd1306_SetCursor(56 * 1.5, 27 * 1.5);
			ssd1306_WriteChar('S', Font_7x10, ct[0][0]->gc_data.start ? Black : White);

			break;

		case CONSOLE_N64:

			ssd1306_DrawRectangle(54, 22, 74, 42, White);
			ssd1306_DrawCircle(64, 32, 8, White);
			ssd1306_DrawPixel(64 + (ct[0][0]->n64_data.x_axis - 128) / 13, 32 - (ct[0][0]->n64_data.y_axis - 128) / 13, White);
			ssd1306_SetCursor(61, 46);
			ssd1306_WriteChar('z', Font_6x8, ct[0][0]->n64_data.z ? Black : White);
			ssd1306_SetCursor(61, 10);
			ssd1306_WriteChar('s', Font_6x8, ct[0][0]->n64_data.start ? Black : White);
			ssd1306_SetCursor(28, 28);
			ssd1306_WriteChar('L', Font_6x8, ct[0][0]->n64_data.left ? Black : White);
			ssd1306_SetCursor(40, 28);
			ssd1306_WriteChar('R', Font_6x8, ct[0][0]->n64_data.right ? Black : White);
			ssd1306_SetCursor(34, 20);
			ssd1306_WriteChar('U', Font_6x8, ct[0][0]->n64_data.up ? Black : White);
			ssd1306_SetCursor(34, 36);
			ssd1306_WriteChar('D', Font_6x8, ct[0][0]->n64_data.down ? Black : White);
			ssd1306_SetCursor(50, 10);
			ssd1306_WriteChar('L', Font_6x8, ct[0][0]->n64_data.l ? Black : White);
			ssd1306_SetCursor(72, 10);
			ssd1306_WriteChar('R', Font_6x8, ct[0][0]->n64_data.r ? Black : White);
			ssd1306_SetCursor(88, 36);
			ssd1306_WriteChar('A', Font_6x8, ct[0][0]->n64_data.a ? Black : White);
			ssd1306_SetCursor(82, 28);
			ssd1306_WriteChar('B', Font_6x8, ct[0][0]->n64_data.b ? Black : White);
			ssd1306_SetCursor(97, 28);
			ssd1306_WriteChar('l', Font_6x8, ct[0][0]->n64_data.c_left ? Black : White);
			ssd1306_SetCursor(109, 28);
			ssd1306_WriteChar('r', Font_6x8, ct[0][0]->n64_data.c_right ? Black : White);
			ssd1306_SetCursor(103, 20);
			ssd1306_WriteChar('u', Font_6x8, ct[0][0]->n64_data.c_up ? Black : White);
			ssd1306_SetCursor(103, 36);
			ssd1306_WriteChar('d', Font_6x8, ct[0][0]->n64_data.c_down ? Black : White);

			break;

		default:
			break;

		}

		ssd1306_UpdateScreen();
		break;

	case MENUTYPE_TASSTATS:
		ssd1306_Fill(Black);
		sprintf(temp, "Latch: %ld", tasrun->frameCount);
		ssd1306_SetCursor(0, 0);
		ssd1306_WriteString(temp, Font_6x8, White);

		sprintf(temp, "Diskrd: %ld", readcount);
		ssd1306_SetCursor(0, 8);
		ssd1306_WriteString(temp, Font_6x8, White);

		sprintf(temp, "Buffer: %d", tasrun->size);
		ssd1306_SetCursor(0, 16);
		ssd1306_WriteString(temp, Font_6x8, White);


		ssd1306_UpdateScreen();
		break;
	}

}

void Menu_Init() {
	CurrentMenu = MENUTYPE_BROWSER;
}

