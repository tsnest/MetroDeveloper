#pragma once
#include "Patcher.h"
class ConsoleUnlocker : public Patcher
{
public:
	static void clevel_r_on_key_press(int action, int key, int state, int resending);
};

