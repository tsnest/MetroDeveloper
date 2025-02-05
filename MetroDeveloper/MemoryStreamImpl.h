#pragma once
#include "vector"
#include "i_pathengine.h"

class MemoryStreamImpl : public iOutputStream
{
public:
	std::vector<char> m_data;

	bool save(const char* filename, bool isLL, bool supply_debug_info_bin);
	const char* ptr();
	size_t size();
	virtual void put(const char* data, tUnsigned32 dataSize);
	void putInt(int value);
	void putFloat(float value);
};

