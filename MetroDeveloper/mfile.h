#ifndef __MFILE_H__
#define __MFILE_H__

#include <stdlib.h>

#define MFILE_READ 1
#define MFILE_WRITE 2

typedef struct MFILE 
{
	int mode;
	
	void *buffer;
	size_t length;
	size_t cursor;
	
	size_t capacity;
	size_t step;
} MFILE;

MFILE *mfopen(const void *buffer, size_t length);
MFILE *mfopenchunk(MFILE *base, int _id);
MFILE *mfcreate(size_t step);
void mfclose(MFILE *f);

int mfread(void *dest, size_t count, MFILE *f);
const char *mfread_stringz(MFILE *f);
char mfread_char(MFILE *f);
short mfread_short(MFILE *f);
int mfread_int(MFILE *f);
float mfread_float(MFILE *f);

int mfwrite(const void *src, size_t count, MFILE *f);
int mfwrite_stringz(const char *src, MFILE *f);
int mfwrite_char(char val, MFILE *f);
int mfwrite_short(short val, MFILE *f);
int mfwrite_int(int val, MFILE *f);
int mfwrite_float(float val, MFILE *f);

#endif