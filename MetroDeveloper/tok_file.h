#ifndef __TOK_FILE_H__
#define __TOK_FILE_H__

enum AttrType
{
	atStringZ = 1,
	atInt = 2,
	atShort = 3,
	atByte = 4,
	atUInt = 5,
	atUShort = 6,
	atUByte = 7
};

typedef struct TOKAttr 
{
	char *name;
	char type;
	struct TOKAttr *next;
	
	char *valuestring;
	int valueint;

} TOKAttr;

typedef struct TOKNode 
{
	char *name;
	struct TOKNode *next;
	
	struct TOKAttr *attrs;
	struct TOKNode *child;
	
} TOKNode;

TOKNode *parse_tok(const void *buffer, size_t length);
void free_tok(TOKNode *node);

int nodeCount(TOKNode *n);
TOKNode *getNode(TOKNode *parent, const char *name);

int getAttribute_int(TOKNode *node, const char *name, int def);
float getAttribute_float(TOKNode *node, const char *name, float def);
const char *getAttribute_stringz(TOKNode *node, const char *name, const char *def);

#endif