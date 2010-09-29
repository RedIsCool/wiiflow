#include "menu.hpp"
#include "gecko.h"
#include <stdlib.h>

using namespace std;


static const u32 g_repeatDelay = 80;

void CMenu::SetupInput()
{
	wii_btnRepeat();
	gc_btnRepeat();

	for(int chan = WPAD_MAX_WIIMOTES-1; chan >= 0; chan--)
	{
		stickPointer_x[chan] = (m_vid.width() + m_cursor[chan].width())/2;
		stickPointer_y[chan] = (m_vid.height() + m_cursor[chan].height())/2;
		left_stick_angle[chan] = 0;
		left_stick_mag[chan] = 0;
		right_stick_angle[chan] = 0;
		right_stick_mag[chan] = 0;
		pointerhidedelay[chan] = 0;
		right_stick_skip[chan] = 0;
		wmote_roll[chan] = 0;
		wmote_roll_skip[chan] = 0;
	}
	
	enable_wmote_roll = m_cfg.getBool("GENERAL", "wiimote_gestures", false);
}

static int CalculateRepeatSpeed(float magnitude, int current_value)
{
	if (magnitude < 0) magnitude *= -1;

	// Calculate frameskips based on magnitude
	// Max frameskips is 50 (1 sec, or slightly less)
	if (magnitude < 0.15f)
	{
		return -1; // Force a direct start
	}
	else if (current_value > 0)
	{
		return current_value - 1; // Wait another frame
	}
	else if (current_value == -1)
	{
		return 0; // Process the input
	}
	else
	{
		s32 frames = 50 - ((u32) (50.f * magnitude)); // Calculate the amount of frames to wait
		return (frames < 0) ? 0 : frames;
	}
}

void CMenu::ScanInput()
{
	m_show_zone_main = false;
	m_show_zone_main2 = false;
	m_show_zone_main3 = false;
	m_show_zone_prev = false;
	m_show_zone_next = false;
	
    WPAD_ScanPads();
    PAD_ScanPads();
	
	ButtonsPressed();
	ButtonsHeld();
	LeftStick();
	
	for(int chan = WPAD_MAX_WIIMOTES-1; chan >= 0; chan--)
	{
		wd[chan] = WPAD_Data(chan);
		left_stick_angle[chan] = 0;
		left_stick_mag[chan] = 0;		
		right_stick_angle[chan] = 0;
		right_stick_mag[chan] = 0;
		switch (wd[chan]->exp.type)
		{
			case WPAD_EXP_NUNCHUK:
				right_stick_mag[chan] = wd[chan]->exp.nunchuk.js.mag;
				right_stick_angle[chan] = wd[chan]->exp.nunchuk.js.ang;
				break;
			case WPAD_EXP_GUITARHERO3:
				left_stick_mag[chan] = wd[chan]->exp.nunchuk.js.mag;
				left_stick_angle[chan] = wd[chan]->exp.nunchuk.js.ang;
				break;
			case WPAD_EXP_CLASSIC:
				left_stick_mag[chan] = wd[chan]->exp.classic.ljs.mag;
				left_stick_angle[chan] = wd[chan]->exp.classic.ljs.ang;
				right_stick_mag[chan] = wd[chan]->exp.classic.rjs.mag;
				right_stick_angle[chan] = wd[chan]->exp.classic.rjs.ang;
				break;
			default:
				break;
		}
		if (enable_wmote_roll)
		{
			wmote_roll[chan] = wd[chan]->orient.roll; // Use wd[chan]->ir.angle if you only want this to work when pointing at the screen
			wmote_roll_skip[chan] = CalculateRepeatSpeed(wmote_roll[chan] / 45.f, wmote_roll_skip[chan]);
		}
		right_stick_skip[chan] = CalculateRepeatSpeed(right_stick_mag[chan], right_stick_skip[chan]);
	}
	for(int chan = WPAD_MAX_WIIMOTES-1; chan >= 0; chan--)
	{
		m_btnMgr.setRumble(chan, WPadIR_Valid(chan), PAD_StickX(chan) < -20 || PAD_StickX(chan) > 20 || PAD_StickY(chan) < -20 || PAD_StickY(chan) > 20);

		if (WPadIR_Valid(chan))
		{
			m_cursor[chan].draw(wd[chan]->ir.x, wd[chan]->ir.y, wd[chan]->ir.angle);
			m_btnMgr.mouse(chan, wd[chan]->ir.x - m_cursor[chan].width() / 2, wd[chan]->ir.y - m_cursor[chan].height() / 2);
		}
		else if (m_show_pointer[chan])
		{
			m_cursor[chan].draw(stickPointer_x[chan], stickPointer_y[chan], 0);
			m_btnMgr.mouse(chan, stickPointer_x[chan] - m_cursor[chan].width() / 2, stickPointer_y[chan] - m_cursor[chan].height() / 2);
		}
	}
	ShowMainZone();
	ShowMainZone2();
	ShowMainZone3();
	ShowPrevZone();
	ShowNextZone();
	ShowGameZone();
}

void CMenu::ButtonsPressed()
{
	wii_btnsPressed = 0;
	gc_btnsPressed = 0;

	for(int chan = WPAD_MAX_WIIMOTES-1; chan >= 0; chan--)
	{
        wii_btnsPressed |= WPAD_ButtonsDown(chan);
        gc_btnsPressed |= PAD_ButtonsDown(chan);
    }
}

void CMenu::ButtonsHeld()
{
	wii_btnsHeld = 0;
	gc_btnsHeld = 0;

	for(int chan = WPAD_MAX_WIIMOTES-1; chan >= 0; chan--)
	{
        wii_btnsHeld |= WPAD_ButtonsHeld(chan);
        gc_btnsHeld |= PAD_ButtonsHeld(chan);
    }
}

void CMenu::LeftStick()
{
	u8 speed = 0,pSpeed = 0;
	for(int chan = WPAD_MAX_WIIMOTES-1; chan >= 0; chan--)
	{

		if (left_stick_mag[chan] > 0.15 || abs(PAD_StickX(chan)) > 20 || abs(PAD_StickY(chan)) > 20)
		{
			m_show_pointer[chan] = true;
			if (LEFT_STICK_LEFT)
			{
				speed = (u8)(left_stick_mag[chan] * 10.00);
				pSpeed = (u8)abs(PAD_StickX(chan))/10;
				if (stickPointer_x[chan] > m_cursor[chan].width()/2) stickPointer_x[chan] = stickPointer_x[chan]-speed-pSpeed;
				pointerhidedelay[chan] = 150;
			}
			if (LEFT_STICK_DOWN)
			{
				speed = (u8)(left_stick_mag[chan] * 10.00);
				pSpeed = (u8)abs(PAD_StickY(chan))/10;
				if (stickPointer_y[chan] < (m_vid.height() + (m_cursor[chan].height()/2))) stickPointer_y[chan] = stickPointer_y[chan]+speed+pSpeed;
				pointerhidedelay[chan] = 150;
			}
			if (LEFT_STICK_RIGHT)
			{
				speed = (u8)(left_stick_mag[chan] * 10.00);
				pSpeed = (u8)abs(PAD_StickX(chan))/10;
				if (stickPointer_x[chan] < (m_vid.width() + (m_cursor[chan].width()/2))) stickPointer_x[chan] = stickPointer_x[chan]+speed+pSpeed;
				pointerhidedelay[chan] = 150;
			}
			if (LEFT_STICK_UP)
			{
				speed = (u8)(left_stick_mag[chan] * 10.00);
				pSpeed = (u8)abs(PAD_StickY(chan))/10;
				if (stickPointer_y[chan] > m_cursor[chan].height()/2) stickPointer_y[chan] = stickPointer_y[chan]-speed-pSpeed;
				pointerhidedelay[chan] = 150;
			}
		}
		else 
		{
			if(pointerhidedelay[chan] > 0 && !wii_btnsHeld && !wii_btnsPressed && !gc_btnsHeld && !gc_btnsPressed) 
				pointerhidedelay[chan]--;
			else
			{
				if (!wii_btnsHeld && !wii_btnsPressed)
				{
					pointerhidedelay[chan] = 0;
					stickPointer_x[chan] = (m_vid.width() + m_cursor[chan].width())/2;
					stickPointer_y[chan] = (m_vid.height() + m_cursor[chan].height())/2;
				}
				else if (pointerhidedelay[chan] > 0) 
						pointerhidedelay[chan] = 150;
			}
		}
		if (pointerhidedelay[chan] == 0)
			m_show_pointer[chan] = false;
    }
}

bool CMenu::WPadIR_Valid(int i)
{
	wd[i] = WPAD_Data(i);
	if (wd[i]->ir.valid)
		return true;
	return false;
}

bool CMenu::WPadIR_ANY()
{
	return (wd[0]->ir.valid || wd[1]->ir.valid || wd[2]->ir.valid || wd[3]->ir.valid);
}

u32 CMenu::wii_btnRepeat()
{
	u32 b = 0;

	if ((wii_btnsHeld & WBTN_LEFT) != 0)
	{
		if (m_wpadLeftDelay == 0 || m_wpadLeftDelay > g_repeatDelay)
			b |= WPAD_BUTTON_LEFT;
		++m_wpadLeftDelay;
	}
	else
		m_wpadLeftDelay = 0;
	if ((wii_btnsHeld & WBTN_DOWN) != 0)
	{
		if (m_wpadDownDelay == 0 || m_wpadDownDelay > g_repeatDelay)
			b |= WPAD_BUTTON_DOWN;
		++m_wpadDownDelay;
	}
	else
		m_wpadDownDelay = 0;
	if ((wii_btnsHeld & WBTN_RIGHT) != 0)
	{
		if (m_wpadRightDelay == 0 || m_wpadRightDelay > g_repeatDelay)
			b |= WPAD_BUTTON_RIGHT;
		++m_wpadRightDelay;
	}
	else
		m_wpadRightDelay = 0;
	if ((wii_btnsHeld & WBTN_UP) != 0)
	{
		if (m_wpadUpDelay == 0 || m_wpadUpDelay > g_repeatDelay)
			b |= WPAD_BUTTON_UP;
		++m_wpadUpDelay;
	}
	else
		m_wpadUpDelay = 0;
	if ((wii_btnsHeld & WBTN_A) != 0)
	{
		if (m_wpadADelay == 0 || m_wpadADelay > g_repeatDelay)
			b |= WPAD_BUTTON_A;
		m_btnMgr.noClick(true);
		++m_wpadADelay;
	}
	else
	{
		m_btnMgr.noClick();
		m_wpadADelay = 0;
	}
	if ((wii_btnsHeld & WBTN_B) != 0)
	{
		if (m_wpadBDelay == 0 || m_wpadBDelay > g_repeatDelay)
			b |= WPAD_BUTTON_B;
		m_btnMgr.noClick(true);
		++m_wpadBDelay;
	}
	else
	{
		m_btnMgr.noClick();
		m_wpadBDelay = 0;
	}
	return b;
}

u32 CMenu::gc_btnRepeat()
{
	u32 b = 0;

	if ((gc_btnsHeld & BTN_LEFT) != 0)
	{
		if (m_padLeftDelay == 0 || m_padLeftDelay > g_repeatDelay)
			b |= BTN_LEFT;
		++m_padLeftDelay;
	}
	else
		m_padLeftDelay = 0;
	if (gc_btnsHeld & BTN_DOWN)
	{
		if (m_padDownDelay == 0 || m_padDownDelay > g_repeatDelay)
			b |= BTN_DOWN;
		++m_padDownDelay;
	}
	else
		m_padDownDelay = 0;
	if (gc_btnsHeld & BTN_RIGHT)
	{
		if (m_padRightDelay == 0 || m_padRightDelay > g_repeatDelay)
			b |= BTN_RIGHT;
		++m_padRightDelay;
	}
	else
		m_padRightDelay = 0;
	if (gc_btnsHeld & BTN_UP)
	{
		if (m_padUpDelay == 0 || m_padUpDelay > g_repeatDelay)
			b |= BTN_UP;
		++m_padUpDelay;
	}
	else
		m_padUpDelay = 0;
	if (gc_btnsHeld & BTN_A)
	{
		if (m_padADelay == 0 || m_padADelay > g_repeatDelay)
			b |= BTN_A;
		m_btnMgr.noClick(true);
		++m_padADelay;
	}
	else
	{
		m_btnMgr.noClick();
		m_padADelay = 0;
	}
	if (gc_btnsHeld & BTN_B)
	{
		if (m_padBDelay == 0 || m_padBDelay > g_repeatDelay)
			b |= BTN_B;
		m_btnMgr.noClick(true);
		++m_padBDelay;
	}
	else
	{
		m_btnMgr.noClick();
		m_padBDelay = 0;
	}
	return b;
}

void CMenu::ShowZone(SZone zone, bool &showZone)
{
	showZone = false;
	for(int chan = WPAD_MAX_WIIMOTES-1; chan >= 0; chan--)
		if ((WPadIR_Valid(chan) || m_show_pointer[chan]) && m_cursor[chan].x() >= zone.x && m_cursor[chan].y() >= zone.y
			&& m_cursor[chan].x() < zone.x + zone.w && m_cursor[chan].y() < zone.y + zone.h)
			showZone = true;
}

void CMenu::ShowMainZone()
{
	ShowZone(m_mainButtonsZone, m_show_zone_main);
}

void CMenu::ShowMainZone2()
{
	ShowZone(m_mainButtonsZone2, m_show_zone_main2);
}

void CMenu::ShowMainZone3()
{
	ShowZone(m_mainButtonsZone3, m_show_zone_main3);
}

void CMenu::ShowPrevZone()
{
	ShowZone(m_mainPrevZone, m_show_zone_prev);
}

void CMenu::ShowNextZone()
{
	ShowZone(m_mainNextZone, m_show_zone_next);
}

void CMenu::ShowGameZone()
{
	ShowZone(m_gameButtonsZone, m_show_zone_game);
}
