#include "ShadersHack.h"
#include "Utils.h"

ShadersHack::ShadersHack()
{
	// shaders hack
	// � �������.1 �� ��������� �� ���� ������ �������� ������������ /_OCULUS_=1, ������ ��� ������ ������� ������� �������� � ����
	// ������ ���� ��������� ���� � ���������� -build_key m3 �� /_OCULUS_=1 � ������ �������� �� ������������, � ������ ��������
	// �.�. �� ����� ����� � ����� ������ �������.
	// ����� ���� ��� ������������ ���� �������� ����� �������� ����� /_OCULUS_=1 ����������� ������, �� �������� �� �����

	if (Utils::GetBool("arktika1", "shaders_hack", false)) {
		// 0f 84 ? ? ? ? 49 8b 06 4c 8d 40
		LPVOID addr = (LPVOID)FindPatternInEXE(
			(BYTE*)"\x0f\x84\x00\x00\x00\x00\x49\x8b\x06\x4c\x8d\x40",
			"xx????xxxxxx");

		BYTE mem[] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
		ASMWrite(addr, mem, sizeof(mem));
	}
}
