#pragma once
#include "Patcher.h"

class Fly : public Patcher
{
public:
	Fly();
	static void clevel_r_on_key_press(int action, int key, int state, int resending);
};

