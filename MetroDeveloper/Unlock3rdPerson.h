#pragma once
#include "Patcher.h"

class Unlock3rdPerson : public Patcher
{
public:
	Unlock3rdPerson();
	static void clevel_r_on_key_press(void* _this, int action, int key, int state, int resending);
};

