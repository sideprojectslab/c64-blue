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

//----------------------------------------------------------------------------//

SemaphoreHandle_t autofire_sem_h[2];
SemaphoreHandle_t afsleep_sem_h[2];
bool              autofire[2] = {0};

//----------------------------------------------------------------------------//

static void c64b_gamepad_autofire(t_c64b_cport_idx i)
{
	while(1)
	{
		if(xSemaphoreTake(autofire_sem_h[i], (TickType_t)portMAX_DELAY) == pdTRUE)
		{
			bool active = autofire[i];
			xSemaphoreGive(autofire_sem_h[i]);

			if (active)
			{
				// make sure this tpress is smaller than the smallest autofire
				// interval (currently 100ms)
				const TickType_t tpress = 30 / portTICK_PERIOD_MS;

				c64b_keyboard_cport_psh(&keyboard, CPORT_FF, i);
				vTaskDelay(tpress);
				c64b_keyboard_cport_rel(&keyboard, CPORT_FF, i);
				vTaskDelay(tpress);

				xSemaphoreTake(afsleep_sem_h[i], af_prd - tpress * 2);
			}
		}
	}
}

//----------------------------------------------------------------------------//

static void task_c64b_gamepad_autofire_2(void *arg)
{
	c64b_gamepad_autofire(CPORT_2);
}

static void task_c64b_gamepad_autofire_1(void *arg)
{
	c64b_gamepad_autofire(CPORT_1);
}

//----------------------------------------------------------------------------//

static void c64b_gamepad_autofire_start(unsigned int i)
{
	if(xSemaphoreTake(autofire_sem_h[i], (TickType_t)portMAX_DELAY) == pdTRUE)
	{
		if (!autofire[i])
		{
			logi("starting autofire on port %i\n", i + 1);
			autofire[i] = true;
			xSemaphoreGive(afsleep_sem_h[i]); // for instant restart
		}
		xSemaphoreGive(autofire_sem_h[i]);
	}
}

//----------------------------------------------------------------------------//

static void c64b_gamepad_autofire_stop(unsigned int i)
{
	if(xSemaphoreTake(autofire_sem_h[i], (TickType_t)portMAX_DELAY) == pdTRUE)
	{
		if (autofire[i])
		{
			logi("stopping autofire on port %i\n", i + 1);
			autofire[i] = false;
		}
		xSemaphoreGive(autofire_sem_h[i]);
	}
}

//----------------------------------------------------------------------------//

void c64b_parse_gamepad_init()
{
	autofire_sem_h[0] = xSemaphoreCreateBinary();
	autofire_sem_h[1] = xSemaphoreCreateBinary();
	afsleep_sem_h[0] = xSemaphoreCreateBinary();
	afsleep_sem_h[1] = xSemaphoreCreateBinary();
	xSemaphoreGive(autofire_sem_h[0]);
	xSemaphoreGive(autofire_sem_h[1]);
	xSemaphoreGive(afsleep_sem_h[0]);
	xSemaphoreGive(afsleep_sem_h[1]);

	xTaskCreatePinnedToCore(task_c64b_gamepad_autofire_2,
	                        "autofire 2",
	                        4096,
	                        NULL,
	                        1,
	                        NULL,
	                        tskNO_AFFINITY);

	xTaskCreatePinnedToCore(task_c64b_gamepad_autofire_1,
	                        "autofire 1",
	                        4096,
	                        NULL,
	                        1,
	                        NULL,
	                        tskNO_AFFINITY);
}

//----------------------------------------------------------------------------//

bool c64b_parse_gamepad_menu(uni_gamepad_t* gp, uni_gamepad_t* gp_old)
{
	if(xSemaphoreTake(feed_sem_h, (TickType_t)0) == pdTRUE)
	{
		if((gp->buttons & BTN_B_MASK) && !(gp_old->buttons & BTN_B_MASK))
		{
			menu_fwd();
		}
		else if((gp->buttons & BTN_A_MASK) && !(gp_old->buttons & BTN_A_MASK))
		{
			menu_bwd();
		}
		else if((gp->misc_buttons & BTN_MENU_MASK) && !(gp_old->misc_buttons & BTN_MENU_MASK))
		{
			menu_act();
		}
		else
		{
			xSemaphoreGive(feed_sem_h);
		}
	}

	return true; // placeholder for now
}

//----------------------------------------------------------------------------//

bool c64b_parse_gamepad_swap(uni_gamepad_t* gp, uni_gamepad_t* gp_old)
{
	bool swap_ports = false;

	if((gp->buttons & BTN_Y_MASK) && !(gp_old->buttons & BTN_Y_MASK))
	{
		logi("Swapping Ports\n");
		swap_ports = true;
		if(xSemaphoreTake(kbrd_sem_h, portMAX_DELAY) == pdTRUE)
		{
			c64b_keyboard_reset(&keyboard);
			kb_owner = KB_OWNER_NONE;
			xSemaphoreGive(kbrd_sem_h);
		}
	}

	return swap_ports;
}

//----------------------------------------------------------------------------//

bool c64b_parse_gamepad_kbemu(uni_gamepad_t* gp, uni_gamepad_t* gp_old, t_c64b_cport_idx cport_idx)
{
	bool kb_nop = true;

	if(xSemaphoreTake(kbrd_sem_h, (TickType_t)0) == pdTRUE)
	{
		if((kb_owner == cport_idx + 1) || (kb_owner == KB_OWNER_NONE))
		{
			kb_owner = cport_idx + 1;
			kb_nop   = false;

			if(gp->misc_buttons & BTN_MENU_MASK)
				c64b_keyboard_char_psh(&keyboard, c64b_keyboard_idx_to_key(ct_map[CT_MAP_IDX_BM]));
			else if(gp->misc_buttons & BTN_HOME_MASK)
				c64b_keyboard_char_psh(&keyboard, c64b_keyboard_idx_to_key(ct_map[CT_MAP_IDX_BH]));
			else if(gp->buttons & BTN_LS_MASK)
				c64b_keyboard_char_psh(&keyboard, c64b_keyboard_idx_to_key(ct_map[CT_MAP_IDX_LS]));
			else if(gp->buttons & BTN_RS_MASK)
				c64b_keyboard_char_psh(&keyboard, c64b_keyboard_idx_to_key(ct_map[CT_MAP_IDX_RS]));
			else if(gp->brake > 40)
				c64b_keyboard_char_psh(&keyboard, c64b_keyboard_idx_to_key(ct_map[CT_MAP_IDX_LT]));
			else if(gp->throttle > 40)
				c64b_keyboard_char_psh(&keyboard, c64b_keyboard_idx_to_key(ct_map[CT_MAP_IDX_RT]));
			else
				kb_nop = true;

			if(kb_nop)
			{
				c64b_keyboard_keys_rel(&keyboard, true);
				kb_owner = KB_OWNER_NONE;
			}
		}
		xSemaphoreGive(kbrd_sem_h);
	}

	return kb_nop;
}

//----------------------------------------------------------------------------//

bool c64b_parse_gamepad_ctrl(uni_gamepad_t* gp, uni_gamepad_t* gp_old, t_c64b_cport_idx cport_idx)
{
	bool rr_pressed = false;
	bool ll_pressed = false;
	bool up_pressed = false;
	bool dn_pressed = false;
	bool ff_pressed = false;
	bool af_pressed = false;

	if((gp->dpad & BTN_DPAD_UP_MASK) || (gp->buttons & BTN_B_MASK))
		up_pressed = true;

	if((gp->dpad & BTN_DPAD_DN_MASK) || (gp->buttons & BTN_X_MASK))
		dn_pressed = true;

	if(gp->dpad & BTN_DPAD_RR_MASK)
		rr_pressed = true;

	if(gp->dpad & BTN_DPAD_LL_MASK)
		ll_pressed = true;

	if(gp->buttons & BTN_A_MASK)
		ff_pressed = true;

	if(gp->buttons & BTN_Y_MASK)
		af_pressed = true;

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

	if(af_pressed)
		c64b_gamepad_autofire_start(cport_idx);
	else
		c64b_gamepad_autofire_stop(cport_idx);

	return true;
}
