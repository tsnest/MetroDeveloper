#pragma once
#include "Patcher.h"

class Fly : public Patcher
{
public:
	Fly();

	static float refly_speed;
	static unsigned int refly_cycles;

	static void fly(const char* name, bool refly, float refly_speed, unsigned int refly_cycles);
	static void clevel_r_on_key_press(int action, int key, int state, int resending);

#ifdef _WIN64
	static void __fastcall exodus_cflycam_r_on_key_press(void* _cflycam, int action, int key, int state, int resending);
#endif
};

