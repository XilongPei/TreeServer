#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>   
	 
#include "dir.h"
#include "ctree.h"
#include "ctl.h"	 
#include "mistring.h"
#include "bc2watc.h"
	    
CtlFile	ctlFile;
CtlFile	*pCtlFile = NULL;
		 
static	char  *newBlock(CtlFile *ctlFile,short size)
{
	char  *p;

	p =  (char *)(ctlFile->memPool+ctlFile->count);
	ctlFile->count += size;

	return p;
}

void	downCtlFile(CtlFile *ctlFile)
{
	if (ctlFile->modified == True)
	{
	    short  i, memSize = 0;
	    long offset;
	    char ndxFile[12];

	    NodeSymbol *pNode = ctlFile->symbolTable;

	    fseek(ctlFile->is, 0L, SEEK_END);
	    offset = ftell(ctlFile->is);
	    putw(ctlFile->numSymbolNodes + ctlFile->curAuxNode, ctlFile->is);

	    for (i = 0; i < ctlFile->numSymbolNodes; i++, pNode++)
	    {
		short len = strlen(pNode->name) + 1;

		putw(len, ctlFile->is);
		fwrite((void *)pNode->name, len, 1, ctlFile->is);
		putw(pNode->num, ctlFile->is);
	    }

	    pNode = ctlFile->auxTable;
	    for (i = 0; i < ctlFile->curAuxNode; i++, pNode++)
	    {
		short len = strlen(pNode->name) + 1;

		putw(len, ctlFile->is);
		fwrite((void *)pNode->name, len, 1, ctlFile->is);
		putw(pNode->num, ctlFile->is);
		free(pNode->name);
		memSize += len + sizeof(NodeSymbol);
	    }

	    fseek(ctlFile->is, 0L, SEEK_SET);
	    fwrite((void *)&offset, sizeof(long), 1, ctlFile->is);

	    fseek(ctlFile->is, 0L, SEEK_SET);
	    fread((void *)&offset,  sizeof(long), 1, ctlFile->is);
	    fread((void *)&ndxFile, sizeof(ndxFile), 1, ctlFile->is);

	    putw(ctlFile->memSize + memSize, ctlFile->is);
	    putw(ctlFile->maxContentLen, ctlFile->is);
	    //maxAssignStrLen  = getw(is);
	}

	fclose(ctlFile->is);
	lioClose(ctlFile->indexHandle);
	free(ctlFile->memPool);
	free(ctlFile->content);
	free(ctlFile->auxTable);
}
static	int  key_cmp_function( const void *a, const void *b)
{
	return( stricmp(((NodeSymbol *)a)->name,((NodeSymbol *)b)->name) );
}


static	short  searchKey(CtlFile *ctlFile,char *key)
{
	short	   i, numNodes;
	NodeSymbol *pNode, *nodeArray, node;

	nodeArray = ctlFile->symbolTable;
	numNodes  = ctlFile->numSymbolNodes;

	node.name = key;
	pNode = bsearch((void *)&node,(void *)nodeArray,numNodes, sizeof(NodeSymbol), key_cmp_function);

	if (pNode == NULL && ctlFile->curAuxNode > 0) {
	    nodeArray = ctlFile->auxTable;
	    numNodes  = ctlFile->curAuxNode;
	    pNode = bsearch((void *)&node,(void *)nodeArray,numNodes, sizeof(NodeSymbol), key_cmp_function);
	}

	if (pNode != NULL) return pNode->num;
	else		   return 0;
}

static	Boolean	addKey(CtlFile *ctlFile, char *newKey)
{
	short 	i;

	if (searchKey(ctlFile, newKey) != 0) return True;

	ctlFile->curAuxNode++;
	if (ctlFile->curAuxNode >= ctlFile->numAuxNodes)
	{
	    void  *p;
	    p = realloc(ctlFile->auxTable, sizeof(NodeSymbol)*(ctlFile->numAuxNodes + 16));
	    if (p == NULL)
	    {
		ctlFile->curAuxNode--;
		return False;
	    }
	    else
	    {
		ctlFile->numAuxNodes += 16;
		ctlFile->auxTable = p;
	    }
	}
	i = ctlFile->curAuxNode - 1;

	ctlFile->auxTable[i].name = strdup(newKey);
	if (ctlFile->auxTable[i].name == NULL) {
	    ctlFile->curAuxNode--;
	    return False;
	}
	ctlFile->auxTable[i].num  = ctlFile->numSymbolNodes + ctlFile->curAuxNode;

	if (ctlFile->modified == False) ctlFile->modified = True;

	qsort((void *)ctlFile->auxTable, ctlFile->curAuxNode,
	       sizeof(NodeSymbol), key_cmp_function);

	return True;
}


Boolean	loadCtlFile(CtlFile *ctlFile,char *fileName)
{
	long	symbolTableOffset;
	char	ndxFile[12];
	NodeSymbol 	*pNode;
	short		i;
	FILE		*is;
	short		maxAssignStrLen;
	short		keyDes;
	char 	buf[MAXPATH];

	is = fopen(fileName,"r+b");
	if (is == NULL)
	    return False;

	ctlFile->is = is;

	fseek(is, 0L, SEEK_SET);


//	fread((void *)&ctlFile->segmentIndexOffset, sizeof(long), 1,is);

	fread((void *)&symbolTableOffset,sizeof(long), 1,is);
	fread((void *)&ndxFile,sizeof(ndxFile),1,is);
	ctlFile->memSize = getw(is);
	ctlFile->maxContentLen = getw(is);

	makefilename(buf, fileName, ndxFile);
	keyDes = lioOpen(buf);
	if (keyDes == ERROR)
	{
	    fclose(is);
	    return NULL;
	}
	ctlFile->indexHandle = keyDes;

	ctlFile->count 	    = 0;
	ctlFile->memPool    = NULL;
	ctlFile->content    = NULL;


	ctlFile->memPool = (char *)malloc(ctlFile->memSize);
	ctlFile->content = (char *)malloc(ctlFile->maxContentLen+16);

	if ( ctlFile->memPool == NULL || ctlFile->content == NULL )
	{
	     free(ctlFile->memPool);
	     free(ctlFile->content);
	     return False;
	}

	fseek(is,symbolTableOffset,SEEK_SET);

	ctlFile->numSymbolNodes = getw(is);         //number of symbol node

	pNode = ctlFile->symbolTable = (NodeSymbol *)newBlock(ctlFile,ctlFile->numSymbolNodes*sizeof(NodeSymbol));

	for (i = 0; i < ctlFile->numSymbolNodes; i++,pNode++)
	{
	    short   len;
	    char  *s;

	    len = getw(is);
	    s = (char *)newBlock(ctlFile,len);

	    fread((void *)s, len, 1, is);

	    pNode->name = s;

	    pNode->num  = getw(is);	//modified by marlin on 1995/08/28

	}
	qsort((void *)ctlFile->symbolTable, ctlFile->numSymbolNodes,
	       sizeof(NodeSymbol), key_cmp_function);

	//the code of "FIELDCTL"
	ctlFile->specialVal  = searchKey(ctlFile, "FIELDCTL");
	ctlFile->modified    = False;
	ctlFile->numAuxNodes = 0;
	ctlFile->curAuxNode  = 0;
	ctlFile->auxTable    = NULL;


	return True;
}

char	*getCtlInfomation(CtlFile *ctlFile,char *segment,char *label,char *topic)
{
	short	segmentVal,labelVal,topicVal, otherVal = -1;
	char	*sp, baseName[32];
	short	key[MAXSHORTNUM], *p;
	short	numChars;
	long	offset;

	segmentVal = searchKey(ctlFile,segment);
	if (segmentVal == ctlFile->specialVal)
	{
	    sp = strchr(label, '.');
	    if (sp == NULL) {
		otherVal = -1;
	    } else {
		*sp++ = EOS;
		strcpy(baseName, sp);
		otherVal = searchKey(ctlFile, baseName);
	    }
	}

	if (label != NULL)
	{
	    labelVal  = searchKey(ctlFile,label);
	    if (labelVal == 0) return NULL;
	}
	else labelVal = 0;

	topicVal  = searchKey(ctlFile,topic);

	if (segmentVal == 0 || topicVal == 0)
	    return NULL;

	/* make a key */
	/*
	p = key;
	sprshortf(p,"%05d",segmentVal);
	p+=5;

	sprshortf(p,"%05d",labelVal);
	p+=5;

	sprshortf(p,"%05d",topicVal);
	*/
	//modified on 1995/08/24
	p = key;

	*p++ = segmentVal;
	*p++ = labelVal;
	*p++ = topicVal;
	*p   = otherVal;

	offset = lioSearch(ctlFile->indexHandle,(char *)key);
	if (offset == ERROR || offset == NULL)
		return NULL;

	fseek(ctlFile->is,offset,SEEK_SET);
	numChars = getw(ctlFile->is);

	fread((void *)ctlFile->content,numChars,1,ctlFile->is);

	return ctlFile->content;
}

Boolean	PutCtlInformation(CtlFile *ctlFile, char *segment,
			  char *label, char *topic, char *text)
{
	short	segmentVal,labelVal,topicVal, otherVal = -1;
	char	*sp, baseName[32];
	short	key[MAXSHORTNUM], *p;
	short	numChars;
	long	offset;

	segmentVal = searchKey(ctlFile,segment);

	if (segmentVal == 0) addKey(ctlFile, segment);
	segmentVal = searchKey(ctlFile,segment);

	if (segmentVal == ctlFile->specialVal)
	{
	    sp = strchr(label, '.');
	    if (sp == NULL) {
		otherVal = -1;
	    } else {
		*sp++ = EOS;
		strcpy(baseName, sp);
		otherVal = searchKey(ctlFile, baseName);
	    }
	}

	if (label != NULL)
	{
	    labelVal  = searchKey(ctlFile,label);
	    if (labelVal == 0)
	    {
		addKey(ctlFile, label);
		labelVal  = searchKey(ctlFile,label);
		if (labelVal == 0) return NULL;
	    }
	}
	else labelVal = 0;

	topicVal  = searchKey(ctlFile,topic);
	if (topicVal == 0) addKey(ctlFile, topic);
	topicVal  = searchKey(ctlFile,topic);

	if (segmentVal == 0 || topicVal == 0)
	    return NULL;

	p = key;

	*p++ = segmentVal;
	*p++ = labelVal;
	*p++ = topicVal;
	*p   = otherVal;

	offset = lioSearch(ctlFile->indexHandle,(char *)key);
	/*
	if (offset == ERROR || offset == NULL)
		return False;
	*/
	if (offset == ERROR || offset == NULL)
	{
	    short len = 0;

	    fseek(ctlFile->is, 0L, SEEK_END);
	    offset = ftell(ctlFile->is);

	    len = strlen(text) + 1;
	    putw(len, ctlFile->is);
	    fwrite((void *)text, len, 1, ctlFile->is);
	    if (len > ctlFile->maxContentLen)
		ctlFile->maxContentLen = len;

	    lioInsert(ctlFile->indexHandle, (char *)key, offset);

	    offset = lioSearch(ctlFile->indexHandle, (char *)key);
	    if (offset == ERROR || offset == NULL)
		return False;
	    //else
	    return True;
	}
	else
	{
	    fseek(ctlFile->is,offset,SEEK_SET);
	    numChars = getw(ctlFile->is);

	    if (numChars >= strlen(text) + 1)
	    {
		fseek(ctlFile->is, offset, SEEK_SET);
		putw(numChars, ctlFile->is);
		fwrite((void *)text, strlen(text) + 1, 1, ctlFile->is);
	    }
	    else
	    {
		short len = 0;

		lioDelete(ctlFile->indexHandle, (char *)key);
		fseek(ctlFile->is, 0L, SEEK_END);
		offset = ftell(ctlFile->is);

		len = strlen(text) + 1;
		putw(len, ctlFile->is);
		fwrite((void *)text, len, 1, ctlFile->is);

		if (len > ctlFile->maxContentLen)
		    ctlFile->maxContentLen = len;

		lioInsert(ctlFile->indexHandle, (char *)key, offset);
	    }
        }
	return True;
}


char	*getErrorInfomation(char *module,char *no)
{
	char	*msg;

	if( pCtlFile == NULL ) return NULL;

	msg = getCtlInfomation(&ctlFile,"Error Message",module,no);

	return msg;
}

