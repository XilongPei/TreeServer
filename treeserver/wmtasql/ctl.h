#ifndef _CTL_H_
#define _CTL_H_

#include <stdio.h>
#include "dio.h"
#include "util.h"

typedef struct 	tagMacroDef MacroDef;
struct	tagMacroDef
{
	short 	index;
	char    *string;
};
typedef struct tagSymbolNode 	SymbolNode;
struct  tagSymbolNode
{
	char 	*name;
	short 	num;
	SymbolNode *next;
};
typedef struct  tagNodeSymbol	NodeSymbol;
struct  tagNodeSymbol
{
	char 	*name;
	short 	num;
};

typedef struct 	tagCtlFile 	CtlFile;
struct 	tagCtlFile
{
	FILE 	*is;			//ctl  resource file handle
	short	indexHandle;

	short	memSize;		//symbol and segment index mem size
	short	count;
	char	*memPool;
	short	maxContentLen;

	char	*content;		//the return string is store here

	short	specialVal;		//val for the "FIELDCTL"
	Boolean modified;		//indicate the ndx file has been modified

	short		numSymbolNodes;
	NodeSymbol	*symbolTable;

	short             numAuxNodes;
	short             curAuxNode;
	NodeSymbol	*auxTable;
};

extern CtlFile ctlFile;
extern CtlFile *pCtlFile;

Boolean	 loadCtlFile(CtlFile *ctlFile,char *fileName);
void	 downCtlFile(CtlFile *ctlFile);

char	*getCtlInfomation(CtlFile *ctlFile,char *segment,char *label,char *key);
Boolean	PutCtlInformation(CtlFile *, char *, char *, char *, char *);
char	*getErrorInfomation(char *module,char *no);

#define	GetCtlInformation getCtlInfomation


/*
	ctlFile	:	a handle of the control resource file
	segment :	segment name [		]
	lable	:	label name   #
	key	:	the key name
*/
#endif