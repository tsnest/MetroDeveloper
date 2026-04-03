// https://github.com/joye-ramone/xray_xp_dev/commit/77d8bb876df84e69ee589232577fc1b7886b3663#diff-38bdb068bbc18c1790e04671a77c5407

#ifndef _WIN64
#pragma once
#include "Patcher.h"
#include "Utils.h"
#include <cmath>
#include <stdio.h>

#define BOBBING_SECT "wpn_bobbing_effector"
#define CROUCH_FACTOR	0.75f
#define SPEED_REMINDER	5.f 

typedef int(__thiscall* _cplayer_state)(DWORD* _this);
typedef unsigned u32;
typedef unsigned long long u64;

struct Fvector
{
	float x, y, z;
};

struct Fvector4
{
	float x, y, z, w;

	void set(const Fvector4& other);
};

struct Fmatrix
{
	union {
		struct {
			float _11, _12, _13, _14;
			float _21, _22, _23, _24;
			float _31, _32, _33, _34;
			float _41, _42, _43, _44;
		};
		struct {
			Fvector4 i;
			Fvector4 j;
			Fvector4 k;
			Fvector4 c;
		};
	};

	void mul(const Fmatrix& a, const Fmatrix& b);
	void setHPB(float h, float p, float b);
};

/* metro specific types */
struct matrix_43T
{
	union {
		struct {
			float _11, _12, _13, _14;
			float _21, _22, _23, _24;
			float _31, _32, _33, _34;
		};
		struct {
			Fvector4 i;
			Fvector4 j;
			Fvector4 k;
		};
	};

	// conversion to X-Ray like matrix
	operator Fmatrix() const
	{
		Fmatrix m;
		m._11 = this->_11; m._12 = this->_21; m._13 = this->_31; m._14 = 0.f;
		m._21 = this->_12; m._22 = this->_22; m._23 = this->_32; m._24 = 0.f;
		m._31 = this->_13; m._32 = this->_23; m._33 = this->_33; m._34 = 0.f;
		m._41 = this->_14; m._42 = this->_24; m._43 = this->_34; m._44 = 1.f;
		return m;
	};

	// convertsion from X-Ray like matrix
	void operator = (const Fmatrix& m)
	{
		_11 = m._11; _12 = m._21; _13 = m._31; _14 = m._41;
		_21 = m._12; _22 = m._22; _23 = m._32; _24 = m._42;
		_31 = m._13; _32 = m._23; _33 = m._33; _34 = m._43;
	}
};

void install_wpn_bobbing();

class wpn_bobbing_la : public Patcher
{
public:
	wpn_bobbing_la();
	void Update(Fmatrix& m);
	void CheckState();

	static void do_bobbing(matrix_43T& hud_matrix);
	static void cmd_register_commands();

private:
	int state_sprint_offset;
	int state_run_offset;
	int state_walk_offset;
	int state_fall_offset;
	int state_crouch_offset;

	float	fTime;
	Fvector	vAngleAmplitude;
	float	fYAmplitude;
	float	fSpeed;

	bool is_walk;
	bool is_sprinting;
	bool is_aim_walk;
	bool is_crouch;

	bool is_walk_enabled;
	bool is_sprinting_enabled;
	bool is_aiming_enabled;

	float	fReminderFactor;

	float	m_fAmplitudeSprint;
	float	m_fAmplitudeWalk;
	float	m_fAmplitudeAiming;

	float	m_fSpeedSprint;
	float	m_fSpeedWalk;
	float	m_fSpeedAiming;
};

#endif