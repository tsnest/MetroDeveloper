#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "tok_file.h"
#include "mfile.h"

static TOKAttr *parse_tok_attrs(const char **attr_names, char *attr_types, MFILE *f)
{
	TOKAttr *first = NULL, *last = NULL;
	int idx;
	
	idx = mfread_char(f);
	if(idx == 0)
		return NULL;
	
	while(idx != 0)
	{
		if(last)
		{
			last->next = calloc(sizeof(TOKAttr), 1);
			last = last->next;
		}
		else
			first = last = calloc(sizeof(TOKAttr), 1);
			
		last->name = strdup(attr_names[idx-1]);
		last->type = attr_types[idx-1];
		
		switch(last->type)
		{
			case atStringZ: last->valuestring = strdup(mfread_stringz(f)); break;
			case atInt:   last->valueint = mfread_int(f);   break;
			case atShort: last->valueint = mfread_short(f); break;
			case atByte:  last->valueint = mfread_char(f);  break;
			case atUInt:   last->valueint = (unsigned)mfread_int(f);   break;
			case atUShort: last->valueint = (unsigned short)mfread_short(f); break;
			case atUByte:  last->valueint = (unsigned char)mfread_char(f);  break;
			default: printf("unknown attr type %d\n", last->type);
		}
			
		idx = mfread_char(f);
	}
	
	return first;
}

static TOKNode *parse_tok_node(const char **node_names, const char **attr_names, char *attr_types, MFILE *f)
{
	TOKNode *first = NULL, *last = NULL;
	int idx;
	
	idx = mfread_char(f);
	if(idx == 0)
		return NULL;
		
	while(idx != 0)
	{
		if(last)
		{
			last->next = calloc(sizeof(TOKNode), 1);
			last = last->next;
		}
		else
			first = last = calloc(sizeof(TOKNode), 1);
			
		last->name = strdup(node_names[idx-1]);
		last->attrs = parse_tok_attrs(attr_names, attr_types, f);
		last->child = parse_tok_node(node_names, attr_names, attr_types, f);
		
		idx = mfread_char(f);
	}
	
	return first;
}

TOKNode *parse_tok(const void *buffer, size_t length)
{
	TOKNode *result;
	MFILE *f;
	
	const char *node_names[256];
	const char *attr_names[256];
	char attr_types[256];
	
	int count;
	
	f = mfopen(buffer, length);
	
	count = 0;
	do {
		node_names[count++] = mfread_stringz(f);
	} while(node_names[count-1][0]);
	
	count = 0;
	do {
		attr_types[count] = mfread_char(f);
		if(attr_types[count] != 0)
			attr_names[count] = mfread_stringz(f);
			
		count++;
	} while(attr_types[count-1] != 0);
	
	result = parse_tok_node(node_names, attr_names, attr_types, f);
	mfclose(f);
	
	return result;
}

void free_tok(TOKNode *node)
{
	TOKAttr *attr, *nextattr;
	TOKNode *nextnode;
	
	while(node)
	{
		nextnode = node->next;
		free(node->name);
		
		attr = node->attrs;
		while(attr)
		{
			nextattr = attr->next;
			free(attr->name);
			free(attr->valuestring);
			free(attr);
			attr = nextattr;
		}
		
		free_tok(node->child);
		node = nextnode;
	}
}

int nodeCount(TOKNode *n)
{
	int count = 0;
	
	n = n->child;
	while(n)
	{
		count++;
		n = n->next;
	}
	
	return count;
}

TOKNode *getNode(TOKNode *parent, const char *name)
{
	TOKNode *n = parent->child;
	
	while(n)
	{
		if(strcmp(name, n->name) == 0)
			return n;
			
		n = n->next;
	}
	
	return NULL;
}

int getAttribute_int(TOKNode *node, const char *name, int def)
{
	TOKAttr *a = node->attrs;
	
	while(a)
	{
		if(strcmp(name, a->name) == 0)
			return a->valueint;
			
		a = a->next;
	}
	
	return def;
}

float getAttribute_float(TOKNode *node, const char *name, float def)
{
	TOKAttr *a = node->attrs;
	const char *str;
	
	while(a)
	{
		if(strcmp(name, a->name) == 0)
		{
			str = a->valuestring;
			if(str)
			{
				int i;
				
				if(str[0] == 'x')
				{
					sscanf(str+1, "%x", &i);
					return *(float*)&i;
				}
				else
					return atof(str);
			}
			else
				return def;
		}
			
		a = a->next;
	}
	
	return def;
}

const char *getAttribute_stringz(TOKNode *node, const char *name, const char *def)
{
	TOKAttr *a = node->attrs;
	
	while(a)
	{
		if(strcmp(name, a->name) == 0 && a->valuestring)
			return a->valuestring;
			
		a = a->next;
	}
	
	return def;
}
