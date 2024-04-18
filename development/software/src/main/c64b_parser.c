//----------------------------------------------------------------------------//
//         .XXXXXXXXXXXXXXXX.  .XXXXXXXXXXXXXXXX.  .XX.                       //
//         XXXXXXXXXXXXXXXXX'  XXXXXXXXXXXXXXXXXX  XXXX                       //
//         XXXX                XXXX          XXXX  XXXX                       //
//         XXXXXXXXXXXXXXXXX.  XXXXXXXXXXXXXXXXXX  XXXX                       //
//         'XXXXXXXXXXXXXXXXX  XXXXXXXXXXXXXXXXX'  XXXX                       //
//                       XXXX  XXXX                XXXX                       //
//         .XXXXXXXXXXXXXXXXX  XXXX                XXXXXXXXXXXXXXXXX.         //
//         'XXXXXXXXXXXXXXXX'  'XX'                'XXXXXXXXXXXXXXXX'         //
//----------------------------------------------------------------------------//
//             Copyright 2023 Vittorio Pascucci (SideProjectsLab)             //
//                                                                            //
// Licensed under the Apache License, Version 2.0 (the "License");            //
// you may not use this file except in compliance with the License.           //
// You may obtain a copy of the License at                                    //
//                                                                            //
//     http://www.apache.org/licenses/LICENSE-2.0                             //
//                                                                            //
// Unless required by applicable law or agreed to in writing, software        //
// distributed under the License is distributed on an "AS IS" BASIS,          //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   //
// See the License for the specific language governing permissions and        //
// limitations under the License.                                             //
//----------------------------------------------------------------------------//

#include "c64b_parser.h"
#include "esp_task_wdt.h"

extern bool c64b_parse_keyboard_logical(uni_controller_t* ctl);

//----------------------------------------------------------------------------//
// Static Variables

static uni_controller_t* ctrl_ptr[3] = {NULL, NULL, NULL};
static uni_controller_t  ctrl_tmp[3] = {{0}, {0}, {0}};
static uni_controller_t  ctrl_new[3] = {{0}, {0}, {0}};
static uni_controller_t  ctrl_old[3] = {{0}, {0}, {0}};

       t_c64b_keyboard   keyboard     = {0};
static t_c64b_kb_owner   kb_owner     = KB_OWNER_NONE;
static t_c64b_macro_id   kb_macro_id  = 0;
static bool              kb_macro_sel = false;
static bool              swap_ports   = false;

static const uint8_t     col_perm[] = COL_PERM;
static const uint8_t     row_perm[] = ROW_PERM;

static SemaphoreHandle_t prse_sem_h; // protects access to controller data
static SemaphoreHandle_t kbrd_sem_h; // protects access to keyboard keystrokes
static SemaphoreHandle_t feed_sem_h; // protects access to keyboard macro

//----------------------------------------------------------------------------//
// C64-Blue functions

void c64b_parser_connect(uni_hid_device_t* d)
{
	// keyboard ID always sits on index 0
	if(uni_hid_device_is_keyboard(d))
	{
		logi("parser: keyboard connected: %p\n", d);
		if(ctrl_ptr[0] == NULL)
			ctrl_ptr[0] = &(d->controller);
	}
	// inserting controller ID in the first free location after 0
	else if(uni_hid_device_is_gamepad(d))
	{
		logi("parser: gamepad connected: %p\n", d);
		if(ctrl_ptr[1] == NULL)
			ctrl_ptr[1] = &(d->controller);
		else if(ctrl_ptr[2] == NULL)
			ctrl_ptr[2] = &(d->controller);
	}
	else
	{
		logi("parser: device class not supported: %d\n", d->controller.klass);
	}
}

//----------------------------------------------------------------------------//

int c64b_parser_get_idx(uni_hid_device_t* d)
{
	for(unsigned int i = 0; i < 3; ++i)
	{
		if(ctrl_ptr[i] == &(d->controller))
			return (i);
	}
	return -1;
}

//----------------------------------------------------------------------------//

static void task_keyboard_macro_feed(void *arg)
{
	logi("Starting Keyboard Feed\n");
	if(xSemaphoreTake(kbrd_sem_h, (TickType_t)10) == pdTRUE)
	{
		if((kb_owner == KB_OWNER_FEED) || (kb_owner == KB_OWNER_NONE))
		{
			kb_owner = KB_OWNER_FEED;
			c64b_keyboard_feed_str(&keyboard, (char *)arg);
			kb_owner = KB_OWNER_NONE;
		}
		xSemaphoreGive(kbrd_sem_h);
	}
	xSemaphoreGive(feed_sem_h);
	vTaskDelete(NULL);
}

//----------------------------------------------------------------------------//
void keyboard_macro_feed(const char* str)
{
	xTaskCreatePinnedToCore(task_keyboard_macro_feed,
	                        "keyboard-macro-feed",
	                        2048,
	                        (void * const)str,
	                        3,
	                        NULL,
	                        tskNO_AFFINITY);
}

//----------------------------------------------------------------------------//

void c64b_parse_keyboard(uni_controller_t* ctl)
{
	if(ctl == NULL)
		return;

	// this function only supports keyboard
	if(ctl->klass != UNI_CONTROLLER_CLASS_KEYBOARD)
		return;

	uni_keyboard_t* kb = &(ctl->keyboard);
	uni_keyboard_t* kb_old;
	bool            kb_nop;

	kb_old = &(ctrl_old[0].keyboard);

	if (memcmp(kb_old, kb, sizeof(uni_keyboard_t)) == 0)
		return;

	#if (CONFIG_BLUEPAD32_UART_OUTPUT_ENABLE == 1)
		uni_controller_dump(ctl);
	#endif

	if(xSemaphoreTake(kbrd_sem_h, (TickType_t)0) == pdTRUE)
	{
		if((kb_owner == KB_OWNER_KBRD) || (kb_owner == KB_OWNER_NONE))
		{
			kb_owner = KB_OWNER_KBRD;

			kb_nop = c64b_parse_keyboard_logical(ctl);

			if(kb_nop)
				kb_owner = KB_OWNER_NONE;
		}
		xSemaphoreGive(kbrd_sem_h);
	}

	*kb_old = *kb;
}

//----------------------------------------------------------------------------//

void c64b_parse_gamepad(uni_controller_t* ctl)
{
	if(ctl == NULL)
		return;

	// this function only supports gamepads
	if(ctl->klass != UNI_CONTROLLER_CLASS_GAMEPAD)
		return;

	t_c64b_cport_idx cport_idx;
	uni_gamepad_t*   gp = &(ctl->gamepad);
	uni_gamepad_t*   gp_old;
	bool             kb_nop = true;
	bool             cport_inhibit = false;

	if(ctl == &(ctrl_new[1]))
	{
		gp_old    = &(ctrl_old[1].gamepad);
		cport_idx = swap_ports ? CPORT_1 : CPORT_2;
	}
	else if(ctl == &(ctrl_new[2]))
	{
		gp_old    = &(ctrl_old[2].gamepad);
		cport_idx = swap_ports ? CPORT_2 : CPORT_1;
	}
	else
	{
		logi("parser: gamepad unregistered\n");
		return;
	}

	if (memcmp(gp_old, gp, sizeof(uni_gamepad_t)) == 0)
		return;

	#if (CONFIG_BLUEPAD32_UART_OUTPUT_ENABLE == 1)
		uni_controller_dump(ctl);
	#endif

	//------------------------------------------------------------------------//

	if(gp->misc_buttons & BTN_SELECT_MASK)
	{
		cport_inhibit = true;

		//--------------------------------------------------------------------//
		// keyboard macros

		if(xSemaphoreTake(feed_sem_h, (TickType_t)0) == pdTRUE)
		{
			if((gp->buttons & BTN_B_MASK) && !(gp_old->buttons & BTN_B_MASK))
			{
				if(kb_macro_sel)
					kb_macro_id = kb_macro_id == KB_MACRO_COUNT - 1 ? 0 : kb_macro_id + 1;
				kb_macro_sel = true;
				keyboard_macro_feed(feed_cmd_gui[kb_macro_id]);
			}
			else if((gp->buttons & BTN_A_MASK) && !(gp_old->buttons & BTN_A_MASK))
			{
				if(kb_macro_sel)
					kb_macro_id = kb_macro_id == 0 ? KB_MACRO_COUNT - 1 : kb_macro_id - 1;
				kb_macro_sel = true;
				keyboard_macro_feed(feed_cmd_gui[kb_macro_id]);
			}
			else if((gp->misc_buttons & BTN_START_MASK) && !(gp_old->misc_buttons & BTN_START_MASK))
			{
				if(kb_macro_sel)
				{
					kb_macro_sel = false;
					keyboard_macro_feed(feed_cmd_str[kb_macro_id]);
				}
				else
				{
					xSemaphoreGive(feed_sem_h);
				}
			}
			else
			{
				xSemaphoreGive(feed_sem_h);
			}
		}

		// swap ports
		if((gp->buttons & BTN_Y_MASK) && !(gp_old->buttons & BTN_Y_MASK))
		{
			if((cport_idx == CPORT_2) || (ctrl_ptr[1] == NULL) || (ctrl_ptr[2] == NULL))
			{
				logi("Swapping Ports\n");
				swap_ports ^= true;
				if(xSemaphoreTake(kbrd_sem_h, portMAX_DELAY) == pdTRUE)
				{
					c64b_keyboard_reset(&keyboard);
					xSemaphoreGive(kbrd_sem_h);
				}
			}
		}
	}

	//---------------------------------------------------------------------//
	// direct keyboard control

	if(xSemaphoreTake(kbrd_sem_h, (TickType_t)0) == pdTRUE)
	{
		if((kb_owner == cport_idx + 1) || (kb_owner == KB_OWNER_NONE))
		{
			kb_owner = cport_idx + 1;

			// space
			if(!cport_inhibit)
			{
				if(gp->misc_buttons & BTN_START_MASK)
				{
					if(!(gp->misc_buttons & BTN_SELECT_MASK))
					{
						kb_nop = false;
						c64b_keyboard_char_psh(&keyboard, " ");
					}
				}

				if(gp->buttons & BTN_Y_MASK)
				{
					if(!(gp->misc_buttons & BTN_SELECT_MASK))
					{
						kb_nop = false;
						c64b_keyboard_char_psh(&keyboard, "~f1~");
					}
				}
			}

			if(kb_nop)
			{
				c64b_keyboard_keys_rel(&keyboard, true);
				kb_owner = KB_OWNER_NONE;
			}
		}
		xSemaphoreGive(kbrd_sem_h);
	}

	//------------------------------------------------------------------------//
	// controller ports override characters

	if(!cport_inhibit)
	{
		bool rr_pressed = false;
		bool ll_pressed = false;
		bool up_pressed = false;
		bool dn_pressed = false;
		bool ff_pressed = false;

		if((gp->dpad & BTN_DPAD_UP_MASK) || (gp->buttons & BTN_A_MASK))
			up_pressed = true;

		if((gp->dpad & BTN_DPAD_DN_MASK) || (gp->throttle != 0))
			dn_pressed = true;

		if(gp->dpad & BTN_DPAD_RR_MASK)
			rr_pressed = true;

		if(gp->dpad & BTN_DPAD_LL_MASK)
			ll_pressed = true;

		if(gp->buttons & BTN_B_MASK)
			ff_pressed = true;

		// if left analog stick is outside the dead zone it overrides the dpad
		if((abs(gp->axis_x) > ANL_DEADZONE) || (abs(gp->axis_y) > ANL_DEADZONE))
		{
			unsigned int quadrant = 0;
			if((gp->axis_x >= 0) && (gp->axis_y < 0))
			{
				quadrant = 0;
			}
			else if((gp->axis_x < 0) && (gp->axis_y < 0))
			{
				quadrant = 1;
			}
			else if((gp->axis_x < 0) && (gp->axis_y >= 0))
			{
				quadrant = 2;
			}
			else if((gp->axis_x >= 0) && (gp->axis_y >= 0))
			{
				quadrant = 3;
			}

			if(abs(gp->axis_y) < abs(gp->axis_x) * 2)
			{
				if(quadrant == 0 || quadrant == 3)
					rr_pressed = true;
				else
					ll_pressed = true;
			}

			if(abs(gp->axis_x) < abs(gp->axis_y * 2))
			{
				if(quadrant == 0 || quadrant == 1)
					up_pressed = true;
				else
					dn_pressed = true;
			}
		}

		// these GPIO accesses are all thread-safe on the ESP32
		if(rr_pressed)
			c64b_keyboard_cport_psh(&keyboard, CPORT_RR, cport_idx);
		else
			c64b_keyboard_cport_rel(&keyboard, CPORT_RR, cport_idx);

		if(ll_pressed)
			c64b_keyboard_cport_psh(&keyboard, CPORT_LL, cport_idx);
		else
			c64b_keyboard_cport_rel(&keyboard, CPORT_LL, cport_idx);

		if(up_pressed)
			c64b_keyboard_cport_psh(&keyboard, CPORT_UP, cport_idx);
		else
			c64b_keyboard_cport_rel(&keyboard, CPORT_UP, cport_idx);

		if(dn_pressed)
			c64b_keyboard_cport_psh(&keyboard, CPORT_DN, cport_idx);
		else
			c64b_keyboard_cport_rel(&keyboard, CPORT_DN, cport_idx);

		if(ff_pressed)
			c64b_keyboard_cport_psh(&keyboard, CPORT_FF, cport_idx);
		else
			c64b_keyboard_cport_rel(&keyboard, CPORT_FF, cport_idx);
	}
	else
	{
		c64b_keyboard_cport_rel(&keyboard, CPORT_UP, cport_idx);
		c64b_keyboard_cport_rel(&keyboard, CPORT_DN, cport_idx);
		c64b_keyboard_cport_rel(&keyboard, CPORT_LL, cport_idx);
		c64b_keyboard_cport_rel(&keyboard, CPORT_RR, cport_idx);
		c64b_keyboard_cport_rel(&keyboard, CPORT_FF, cport_idx);
	}

	*gp_old = *gp;
}

//----------------------------------------------------------------------------//

static void task_c64b_parse(void *arg)
{
	while(1)
	{
		if(xSemaphoreTake(prse_sem_h, (TickType_t)portMAX_DELAY) == pdTRUE)
		{
			// latching controller data in a thread-safe manner
			ctrl_new[0] = ctrl_tmp[0];
			ctrl_new[1] = ctrl_tmp[1];
			ctrl_new[2] = ctrl_tmp[2];
			xSemaphoreGive(prse_sem_h);

			c64b_parse_keyboard(&(ctrl_new[0]));
			c64b_parse_gamepad(&(ctrl_new[1]));
			c64b_parse_gamepad(&(ctrl_new[2]));
		}
	}
}

//----------------------------------------------------------------------------//

void c64b_parse(uni_hid_device_t* d)
{
	static bool first_parse = true;
	if(first_parse)
	{
		// this task always runs in the background so it needs to have
		// very low priority
		xTaskCreatePinnedToCore(task_c64b_parse,
		                        "parse task",
		                        4096 * 2,
		                        NULL,
		                        1,
		                        NULL,
		                        tskNO_AFFINITY);
	}
	first_parse = false;

	if(xSemaphoreTake(prse_sem_h, (TickType_t)portMAX_DELAY) == pdTRUE)
	{
		if(ctrl_ptr[0] == &(d->controller))
		{
			ctrl_tmp[0] = d->controller;
		}
		else if(ctrl_ptr[1] == &(d->controller))
		{
			ctrl_tmp[1] = d->controller;
		}
		else if(ctrl_ptr[2] == &(d->controller))
		{
			ctrl_tmp[2] = d->controller;
		}
		xSemaphoreGive(prse_sem_h);
	}
}

//----------------------------------------------------------------------------//

void c64b_parser_disconnect(uni_hid_device_t* d)
{
	if(xSemaphoreTake(prse_sem_h, (TickType_t)portMAX_DELAY) == pdTRUE)
	{
		unsigned int idx;
		if(ctrl_ptr[0] == &(d->controller))
			idx = 0;
		else if(ctrl_ptr[1] == &(d->controller))
			idx = 1;
		else if(ctrl_ptr[2] == &(d->controller))
			idx = 2;
		else
			return;

		logi("Device Disconnected: %d\n", idx);

		ctrl_ptr[idx] = NULL;
		memset(&(ctrl_tmp[0]), 0, sizeof(uni_controller_t));
		if(idx == 0)
			ctrl_tmp[0].klass = UNI_CONTROLLER_CLASS_KEYBOARD;
		else
			ctrl_tmp[0].klass = UNI_CONTROLLER_CLASS_GAMEPAD;

		xSemaphoreGive(prse_sem_h);
	}

	if(xSemaphoreTake(kbrd_sem_h, portMAX_DELAY) == pdTRUE)
	{
		c64b_keyboard_reset(&keyboard);
		xSemaphoreGive(kbrd_sem_h);
	}
}

//----------------------------------------------------------------------------//

void c64b_parser_init()
{
	prse_sem_h = xSemaphoreCreateBinary();
	kbrd_sem_h = xSemaphoreCreateBinary();
	feed_sem_h = xSemaphoreCreateBinary();
	xSemaphoreGive(prse_sem_h);
	xSemaphoreGive(kbrd_sem_h);
	xSemaphoreGive(feed_sem_h);

	kb_owner = KB_OWNER_NONE;

	keyboard.pin_col[0] = PIN_COL0;
	keyboard.pin_col[1] = PIN_COL1;
	keyboard.pin_col[2] = PIN_COL2;
	keyboard.pin_col[3] = PIN_COL3;
	keyboard.pin_col[4] = PIN_COL4;

	keyboard.pin_kca[0] = PIN_KCA0;
	keyboard.pin_kca[1] = PIN_KCA1;
	keyboard.pin_kca[2] = PIN_KCA2;

	keyboard.pin_row[0] = PIN_ROW0;
	keyboard.pin_row[1] = PIN_ROW1;
	keyboard.pin_row[2] = PIN_ROW2;
	keyboard.pin_row[3] = PIN_ROW3;
	keyboard.pin_row[4] = PIN_ROW4;

	keyboard.pin_kra[0] = PIN_KRA0;
	keyboard.pin_kra[1] = PIN_KRA1;
	keyboard.pin_kra[2] = PIN_KRA2;

	keyboard.pin_kben  = PIN_KBEN;
	keyboard.pin_nrst  = PIN_nRST;
	keyboard.pin_ctrl  = PIN_CTRL;
	keyboard.pin_shft  = PIN_SHFT;
	keyboard.pin_cmdr  = PIN_CMDR;

	keyboard.feed_psh_ms = 30;
	keyboard.feed_rel_ms = 30;

	keyboard.col_perm  = col_perm;
	keyboard.row_perm  = row_perm;

	c64b_keyboard_init(&keyboard);
}
