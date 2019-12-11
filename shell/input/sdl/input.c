#include <SDL/SDL.h>
#include <stdint.h>
#include <stdio.h>

#include "menu.h"
#include "shared.h"
#include "config.h"

int32_t axis_joypad[2] = {0, 0};

uint8_t update_input(void)
{
	SDL_Event event;
	uint8_t button = 0;
	uint8_t* keys;
	
	keys = SDL_GetKeyState(NULL);

	while (SDL_PollEvent(&event))
	{
		switch(event.type)
		{
			case SDL_KEYDOWN:
				switch(event.key.keysym.sym)
				{
					case SDLK_END:
					case SDLK_RCTRL:
					case SDLK_ESCAPE:
						emulator_state = 1;
					break;
					default:
					break;
				}
			break;
			case SDL_KEYUP:
				switch(event.key.keysym.sym)
				{
					case SDLK_HOME:
						emulator_state = 1;
					break;
					default:
					break;
				}
			break;
			case SDL_JOYAXISMOTION:
				if (event.jaxis.axis == 0) axis_joypad[0] = event.jaxis.value;
				else if (event.jaxis.axis == 1) axis_joypad[1] = event.jaxis.value;
			break;
		}
	}
	
	if (axis_joypad[0] > DEADZONE_AXIS)
	
	// UP -> DPAD UP
	if (keys[option.config_buttons[0]] == SDL_PRESSED || axis_joypad[1] < -DEADZONE_AXIS)
	{
		button |= 0x01;
	}
	
	// DOWN -> DPAD DOWN
	if (keys[option.config_buttons[1]] == SDL_PRESSED || axis_joypad[1] > DEADZONE_AXIS)
	{
		button |= 0x02;
	}
	
	// LEFT -> DPAD LEFT
	if (keys[option.config_buttons[2] ] == SDL_PRESSED || axis_joypad[0] < -DEADZONE_AXIS)
	{
		button |= 0x04;
	}
	
	// RIGHT -> DPAD RIGHT
	if (keys[option.config_buttons[3] ] == SDL_PRESSED || axis_joypad[0] > DEADZONE_AXIS)
	{
		button |= 0x08;
	}
	
	// A -> A
	if (keys[option.config_buttons[4] ] == SDL_PRESSED)
	{
		button |= 0x10;
	}
	
	// B -> B
	if (keys[option.config_buttons[5] ] == SDL_PRESSED)
	{
		button |= 0x20;
	}
	
	// START -> OPTION
	if (keys[option.config_buttons[6] ] == SDL_PRESSED)
	{
		button |= 0x40;
	}
	
	return button;
}

