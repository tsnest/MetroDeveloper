#pragma once
#include "Patcher.h"

typedef void(__thiscall* _base_npc_cameras_cam_set)(void* _this, int camera_style, float speed, int preserve_attach);
typedef void(__cdecl* _set_camera_2033)(...); // this function has weird calling convention; 'this' passed in EDI, and two parameters passed through stack

enum enpc_cameras_old // 2033, Last Light, Redux
{
	enc_first_eye_old,
	enc_ladder_old,
	enc_look_at_old,
	enc_free_look_old,
	enc_station_old,
	enc_locked_old,
	//enc_max_cam_old // not used in game
};

enum enpc_cameras_new // Arktika.1 and Exodus
{
	enc_first_eye_new,
	enc_look_at_new,
	enc_free_look_new,
	enc_station_new,
	enc_locked_new,
	//enc_max_cam_new // not used in game
};

class Unlock3rdPerson : public Patcher
{
public:
	Unlock3rdPerson();
	static void clevel_r_on_key_press(void* _this, int action, int key, int state, int resending);
};

