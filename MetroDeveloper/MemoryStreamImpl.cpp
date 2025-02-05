#include "MemoryStreamImpl.h"

extern void convert_tok_to_bin(const void* tok_data, size_t tok_size, void** bin_data, size_t* bin_size, int _debug);

bool MemoryStreamImpl::save(const char* filename, bool isLL, bool supply_debug_info_bin)
{
	bool result;

	FILE* out = fopen(filename, "wb");
	if (!out)
		return false;

	if (!isLL)
	{
		size_t written = fwrite(&m_data.front(), 1, m_data.size(), out);
		result = (written == m_data.size());
	}
	else
	{
		void* bin_data;
		size_t bin_size;
		convert_tok_to_bin(&m_data[0], m_data.size(), &bin_data, &bin_size, supply_debug_info_bin);
		size_t written = fwrite(bin_data, bin_size, 1, out);
		result = (written == 1);
		free(bin_data);
	}

	fclose(out);

	return result;
}

const char* MemoryStreamImpl::ptr()
{
	return &m_data.front();
}

size_t MemoryStreamImpl::size()
{
	return m_data.size();
}

void MemoryStreamImpl::put(const char* data, tUnsigned32 dataSize)
{
	size_t pos = m_data.size();
	m_data.resize(m_data.size() + dataSize);
	memcpy(&m_data[pos], data, dataSize);
}

void MemoryStreamImpl::putInt(int value)
{
	put((char*)&value, sizeof(value));
}

void MemoryStreamImpl::putFloat(float value)
{
	put((char*)&value, sizeof(value));
}
