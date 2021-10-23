#include <stdlib.h>
#include <string.h>
#include "mfile.h"

MFILE *mfopen(const void *buffer, size_t length)
{
	MFILE *f = malloc(sizeof(MFILE));
	
	if(!f)
		return 0;
	
	f->mode = MFILE_READ;
	f->buffer = (void*)buffer;
	f->length = length;
	f->cursor = 0;
	f->capacity = length;
	f->step = 0;
	
	return f;
}

MFILE *mfopenchunk(MFILE *base, int _id)
{
	int id;
	int size;
	
	base->cursor = 0;
	while(mfread(&id, 4, base) == 4 && mfread(&size, 4, base) == 4 && size <= (base->length - base->cursor))
	{
		if(id == _id)
			return mfopen((char*)base->buffer + base->cursor, size);
		else
			base->cursor += size;
	}
	
	return NULL;
}

MFILE *mfcreate(size_t step)
{
	MFILE *f = malloc(sizeof(MFILE));
	
	if(!f)
		return 0;
	
	f->mode = MFILE_WRITE;
	f->buffer = NULL;
	f->length = 0;
	f->cursor = 0;
	f->capacity = 0;
	f->step = step;
	
	return f;
}

void mfclose(MFILE *f)
{
	if(f->mode == MFILE_WRITE)
		free(f->buffer);
		
	free(f);
}

int mfread(void *dest, size_t count, MFILE *f)
{
	if(count > f->length-f->cursor)
		count = f->length-f->cursor;
		
	memcpy(dest, (char*)f->buffer + f->cursor, count);
	f->cursor += count;
	
	return count;
}

const char *mfread_stringz(MFILE *f)
{
	const char *ptr = (char*)f->buffer + f->cursor;
	const char *end = memchr(ptr, '\0', f->length-f->cursor);
	
	if(end)
	{
		f->cursor += (end - ptr) + 1;
		return ptr;
	}
	
	return NULL;
}

char mfread_char(MFILE *f)
{
	char val = 0;
	mfread(&val, 1, f);
	return val;
}

short mfread_short(MFILE *f)
{
	short val = 0;
	mfread(&val, 2, f);
	return val;
}

int mfread_int(MFILE *f)
{
	int val = 0;
	mfread(&val, 4, f);
	return val;
}

float mfread_float(MFILE *f)
{
	float val = 0;
	mfread(&val, 4, f);
	return val;
}

int mfwrite(const void *src, size_t length, MFILE *f)
{
	size_t newsize;
	void *ptr;
	
	if(!f || f->mode != MFILE_WRITE)
		return 0;
		
	if(f->capacity < (f->cursor + length))
	{
		newsize = (((f->cursor + length) / f->step) + 1) * f->step;
		ptr = realloc(f->buffer, newsize);
		if(!ptr)
		{
			if(f->cursor > f->capacity)
				return 0;
				
			length = f->capacity - f->cursor;
			goto _write;
		}
			
		f->buffer = ptr;
		f->capacity = newsize;
	}
	
	_write:
	memcpy((char*)f->buffer + f->cursor, src, length);
	f->cursor += length;
	if(f->cursor > f->length) f->length = f->cursor;
	
	return length;
}

int mfwrite_stringz(const char *str, MFILE *f)
{
	return mfwrite(str, strlen(str)+1, f);
}

int mfwrite_char(char val, MFILE *f)
{
	return mfwrite(&val, sizeof(val), f);
}

int mfwrite_short(short val, MFILE *f)
{
	return mfwrite(&val, sizeof(val), f);
}

int mfwrite_int(int val, MFILE *f)
{
	return mfwrite(&val, sizeof(val), f);
}

int mfwrite_float(float val, MFILE *f)
{
	return mfwrite(&val, sizeof(val), f);
}

