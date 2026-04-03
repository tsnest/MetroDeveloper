// https://github.com/joye-ramone/xray_xp_dev/commit/77d8bb876df84e69ee589232577fc1b7886b3663#diff-38bdb068bbc18c1790e04671a77c5407

#ifndef _WIN64
#include "wpn_bobbing_la.h"
#include "uconsole.h"

/* some X-Ray specific types */
float _cos(float a) { return cos(a); }
float _sin(float a) { return sin(a); }
float _abs(float a) { return fabs(a); }

void Fvector4::set(const Fvector4 &other)
{
	x = other.x;
	y = other.y;
	z = other.z;
	w = other.w;
}

void Fmatrix::mul(const Fmatrix &a, const Fmatrix &b)
{
	_11 = a._11*b._11 + a._21*b._12 + a._31*b._13 + a._41*b._14;
	_12 = a._12*b._11 + a._22*b._12 + a._32*b._13 + a._42*b._14;
	_13 = a._13*b._11 + a._23*b._12 + a._33*b._13 + a._43*b._14;
	_14 = a._14*b._11 + a._24*b._12 + a._34*b._13 + a._44*b._14;

	_21 = a._11*b._21 + a._21*b._22 + a._31*b._23 + a._41*b._24;
	_22 = a._12*b._21 + a._22*b._22 + a._32*b._23 + a._42*b._24;
	_23 = a._13*b._21 + a._23*b._22 + a._33*b._23 + a._43*b._24;
	_24 = a._14*b._21 + a._24*b._22 + a._34*b._23 + a._44*b._24;

	_31 = a._11*b._31 + a._21*b._32 + a._31*b._33 + a._41*b._34;
	_32 = a._12*b._31 + a._22*b._32 + a._32*b._33 + a._42*b._34;
	_33 = a._13*b._31 + a._23*b._32 + a._33*b._33 + a._43*b._34;
	_34 = a._14*b._31 + a._24*b._32 + a._34*b._33 + a._44*b._34;

	_41 = a._11*b._41 + a._21*b._42 + a._31*b._43 + a._41*b._44;
	_42 = a._12*b._41 + a._22*b._42 + a._32*b._43 + a._42*b._44;
	_43 = a._13*b._41 + a._23*b._42 + a._33*b._43 + a._43*b._44;
	_44 = a._14*b._41 + a._24*b._42 + a._34*b._43 + a._44*b._44;

	return;
}

void Fmatrix::setHPB(float h, float p, float b)
{
	float sh = std::sin(h);
	float ch = std::cos(h);
	float sp = std::sin(p);
	float cp = std::cos(p);
	float sb = std::sin(b);
	float cb = std::cos(b);

	_11 = ch*cb - sh*sp*sb;
	_12 = -cp*sb;
	_13 = ch*sb*sp + sh*cb;
	_14 = 0;

	_21 = sp*sh*cb + ch*sb;
	_22 = cb*cp;
	_23 = sh*sb - sp*ch*cb;
	_24 = 0;

	_31 = -cp*sh;
	_32 = sp;
	_33 = ch*cp;
	_34 = 0;

	_41 = 0;
	_42 = 0;
	_43 = 0;
	_44 = float(1);

	return;
}

bool isActorAccelerated(u32 state, bool b_zoom)
{
	return false;
}

bool fsimilar(float a, float b)
{
	return fabs(a - b) < 0.0001;
}

wpn_bobbing_la* g_pWpnBobbing;

void install_wpn_bobbing()
{
	g_pWpnBobbing = new wpn_bobbing_la();
}

cmd_float_struct_2033 la_wpn_bobbing_sprint_amplitude;
cmd_float_struct_2033 la_wpn_bobbing_walk_amplitude;
cmd_float_struct_2033 la_wpn_bobbing_aiming_amplitude;
cmd_float_struct_2033 la_wpn_bobbing_sprint_speed;
cmd_float_struct_2033 la_wpn_bobbing_walk_speed;
cmd_float_struct_2033 la_wpn_bobbing_aiming_speed;

void wpn_bobbing_la::cmd_register_commands()
{
	uconsole cu = uconsole::uconsole(Utils::GetConsole());

	// 89 3D ? ? ? ? 89 35 ? ? ? ? F3 0F 11 05 ? ? ? ? E8 ? ? ? ? 83 C4 04 8B 0D ? ? ? ? 85 C9 75 0B E8 ? ? ? ? 8B 0D ? ? ? ? 8B 01 8B 50 08 68 ? ? ? ? FF D2 F6 05 ? ? ? ? ? 75 6A F3 0F 10 05
	DWORD mov = FindPatternInEXE(
		"\x89\x3D\x00\x00\x00\x00\x89\x35\x00\x00\x00\x00\xF3\x0F\x11\x05\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x83\xC4\x04\x8B\x0D\x00\x00\x00\x00\x85\xC9\x75\x0B\xE8\x00\x00\x00\x00\x8B\x0D\x00\x00\x00\x00\x8B\x01\x8B\x50\x08\x68\x00\x00\x00\x00\xFF\xD2\xF6\x05\x00\x00\x00\x00\x00\x75\x6A\xF3\x0F\x10\x05",
		"xx????xx????xxxx????x????xxxxx????xxxxx????xx????xxxxxx????xxxx?????xxxxxx");

	// Вычисляем указатель cmd_float r_base_fov что-бы вытащить из него __vftable
	cmd_float_struct_2033* pCmd = (cmd_float_struct_2033*)(*(DWORD*)(mov + 2));

	// Создаём команды
	la_wpn_bobbing_sprint_amplitude.construct(pCmd->__vftable, "la_wpn_bobbing_sprint_amplitude", &g_pWpnBobbing->m_fAmplitudeSprint, 0.f, 1.f, false);
	cu.command_add(&la_wpn_bobbing_sprint_amplitude);

	la_wpn_bobbing_walk_amplitude.construct(pCmd->__vftable, "la_wpn_bobbing_walk_amplitude", &g_pWpnBobbing->m_fAmplitudeWalk, 0.f, 1.f, false);
	cu.command_add(&la_wpn_bobbing_walk_amplitude);

	la_wpn_bobbing_aiming_amplitude.construct(pCmd->__vftable, "la_wpn_bobbing_aiming_amplitude", &g_pWpnBobbing->m_fAmplitudeAiming, 0.f, 1.f, false);
	cu.command_add(&la_wpn_bobbing_aiming_amplitude);

	la_wpn_bobbing_sprint_speed.construct(pCmd->__vftable, "la_wpn_bobbing_sprint_speed", &g_pWpnBobbing->m_fSpeedSprint, 0.f, 100.f, false);
	cu.command_add(&la_wpn_bobbing_sprint_speed);

	la_wpn_bobbing_walk_speed.construct(pCmd->__vftable, "la_wpn_bobbing_walk_speed", &g_pWpnBobbing->m_fSpeedWalk, 0.f, 100.f, false);
	cu.command_add(&la_wpn_bobbing_walk_speed);

	la_wpn_bobbing_aiming_speed.construct(pCmd->__vftable, "la_wpn_bobbing_aiming_speed", &g_pWpnBobbing->m_fSpeedAiming, 0.f, 100.f, false);
	cu.command_add(&la_wpn_bobbing_aiming_speed);
}

wpn_bobbing_la::wpn_bobbing_la()
{
	if (Utils::is2033()) {
		is_sprinting_enabled	= Utils::GetBool(BOBBING_SECT, "sprint", false);
		is_walk_enabled			= Utils::GetBool(BOBBING_SECT, "walk", false);
		is_aiming_enabled		= Utils::GetBool(BOBBING_SECT, "aiming", false);

		if (Utils::GetBool("wpn_bobbing_effector", "enabled", false) && (is_sprinting_enabled || is_walk_enabled || is_aiming_enabled)) {
			// в версии с dlc смещение + 0x8
			state_sprint_offset = Utils::is2033Patched ? 0x478 : 0x470;
			state_run_offset	= Utils::is2033Patched ? 0x47C : 0x474;
			state_walk_offset	= Utils::is2033Patched ? 0x480 : 0x478;
			state_fall_offset	= Utils::is2033Patched ? 0x484 : 0x47C;
			state_crouch_offset = Utils::is2033Patched ? 0x488 : 0x480;

			fTime = 0;
			fReminderFactor = 0;

			m_fAmplitudeSprint	= Utils::GetFloat(BOBBING_SECT, "sprint_amplitude", 0.0075f);
			m_fAmplitudeWalk	= Utils::GetFloat(BOBBING_SECT, "walk_amplitude", 0.005f);
			m_fAmplitudeAiming	= Utils::GetFloat(BOBBING_SECT, "aiming_amplitude", 0.011f);

			m_fSpeedSprint	= Utils::GetFloat(BOBBING_SECT, "sprint_speed", 10.0f);
			m_fSpeedWalk	= Utils::GetFloat(BOBBING_SECT, "walk_speed", 7.0f);
			m_fSpeedAiming	= Utils::GetFloat(BOBBING_SECT, "aiming_speed", 6.0f);
		}
	}
}

void wpn_bobbing_la::CheckState()
{
	DWORD* control_entity = *(DWORD**)(Utils::GetGLevel() + 0x14);

	_cplayer_state state_sprint = *(_cplayer_state*)(*control_entity + state_sprint_offset);
	_cplayer_state state_run	= *(_cplayer_state*)(*control_entity + state_run_offset);
	_cplayer_state state_walk	= *(_cplayer_state*)(*control_entity + state_walk_offset);
	//_cplayer_state state_fall = *(_cplayer_state*)(*control_entity + state_fall_offset); // not used
	_cplayer_state state_crouch = *(_cplayer_state*)(*control_entity + state_crouch_offset);

	is_walk				= is_walk_enabled && state_run(control_entity);
	if (is_walk) {
		is_crouch		= state_crouch(control_entity);
	} else {
		is_sprinting	= is_sprinting_enabled && state_sprint(control_entity);
		is_aim_walk		= is_aiming_enabled && state_walk(control_entity);
	}

	fTime				+= Utils::GetDeltaF(); //Device.fTimeDelta;
}

void wpn_bobbing_la::Update(Fmatrix &m)
{
	CheckState();
	if (is_walk || is_sprinting || is_aim_walk)
	{
		if (fReminderFactor < 1.f)
			fReminderFactor += SPEED_REMINDER * Utils::GetDeltaF(); //Device.fTimeDelta;
		else						
			fReminderFactor = 1.f;
	}
	else
	{
		if (fReminderFactor > 0.f)
			fReminderFactor -= SPEED_REMINDER * Utils::GetDeltaF(); //Device.fTimeDelta;
		else			
			fReminderFactor = 0.f;
	}
	if (!fsimilar(fReminderFactor, 0))
	{
		Fvector dangle;
		Fmatrix		R, mR;
		float k		= (is_crouch ? CROUCH_FACTOR : 1.f);

		float A, ST;

		if (is_sprinting)
		{
			A	= m_fAmplitudeSprint * k;
			ST	= m_fSpeedSprint * fTime * k;
		}
		else if (is_aim_walk)
		{
			A	= m_fAmplitudeAiming * k;
			ST	= m_fSpeedAiming * fTime * k;
		}
		else
		{
			A	= m_fAmplitudeWalk * k;
			ST	= m_fSpeedWalk * fTime * k;
		}

		float _sinA	= _abs(_sin(ST) * A) * fReminderFactor;
		float _cosA	= _cos(ST) * A * fReminderFactor;

		m.c.y		+=	_sinA;
		dangle.x	=	_cosA;
		dangle.z	=	_cosA;
		dangle.y	=	_sinA;


		R.setHPB(dangle.x, dangle.y, dangle.z);

		mR.mul		(m, R);

		m.k.set(mR.k);
		m.j.set(mR.j);
	}
}

void wpn_bobbing_la::do_bobbing(matrix_43T &hud_matrix)
{
	Fmatrix xr_matrix = hud_matrix;
	g_pWpnBobbing->Update(xr_matrix);
	hud_matrix = xr_matrix;
}

#endif