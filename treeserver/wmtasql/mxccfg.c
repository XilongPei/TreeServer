/*-=-=-=-=-=-=-=-=
 *
 *   MXCCFG.C
 *
 *      Borland C/3.1 Version
 *
 *   Copyright (c) MX Groups 1993
 *   copyright (c) EastUnion Computer Service Co., Ltd. 1995
 *
 * 1995.08.26 add cfg_label() Xilong Pei
 -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#include <io.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mxccfg.h"
#include "mistring.h"
#include "filetool.h"

static char cache[cacheSize];
char 	    ccfgBlank = ' ';

//
//  ------------------- local function prototype --------------------
//
void  cfg_segEnd( MXCCFG *ch );
hPOS  cfg_findItem( MXCCFG *ch, char *cfgKey );   // found the  group's item


//
// ---------------------  function body -----------------------------
//

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//  cfg_create()
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
MXCCFG *cfg_create( char *file, int cfgHeadSize )
{
    MXCCFG *ch;

    ch = (MXCCFG *)malloc( sizeof(MXCCFG) );
    if ( ch == NULL ) return NULL;

    ch->seg = 0L;
    ch->cfgHeadSize = cfgHeadSize;

    ch->hCFG  = fopen( file, "w+b");
    if ( ch->hCFG == NULL )
    {
       free( ch );
       return NULL;
    }
    fseek( ch->hCFG, 0, SEEK_SET );
    fwrite(&ccfgBlank, 1, cfgHeadSize, ch->hCFG);
    fclose(ch->hCFG);
    ch->hCFG  = fopen( file, "w+b");
    return ch;

} // end of cfg_create()


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//  cfg_open()
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
MXCCFG *cfg_open( char *file, int cfgHeadSize )
{
    MXCCFG *ch;

    ch = (MXCCFG *)malloc( sizeof(MXCCFG) );
    if( ch == NULL ) return NULL;

    ch->seg = 0L;
    ch->cfgHeadSize = cfgHeadSize;
    ch->hCFG  = fopen( file, "r+b") ;
    if ( ch->hCFG == NULL )
    {
       free( ch );
       return NULL;
    }
    fseek(ch->hCFG, 0, SEEK_SET);

    return ch;

} // end of cfg_open()

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//  cfg_close()
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
void cfg_close( MXCCFG *ch )
{
    if ( ch->hCFG )       fclose(ch->hCFG);
    free ( ch );

} // end of cfg_close()


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//  cfg_segment()
// fail return e_failure
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
hPOS cfg_segment( MXCCFG *ch, char *grpKey, char *label )
{
    char *ptr;

    if ( !(ch->hCFG) ) 	return e_failure;

    fseek( ch->hCFG, ch->cfgHeadSize, SEEK_SET );

    // get the seg info
_nextLine:
    if( fgets( cache, cacheSize, ch->hCFG ) == NULL ) 	return  e_failure;

    if( lrtrim(cache)[0] == '[' )
    {
	if( !(ptr = strchr( cache, ']') ) ) { // error of config format
		return  e_failure;
	}

	*ptr = '\0';                          // clear the bolck mark
	if( stricmp(&cache[1], grpKey) == 0 ) {
		// ****

		if( label != NULL ) 	goto _nextLine2;

		ch->seg = ftell( ch->hCFG );
		cfg_segEnd( ch );
		return  ch->seg;
	}
    }
    if( !feof( ch->hCFG ) )  goto _nextLine;

    if( label != NULL ) 	return  e_failure;

    // get the label info
_nextLine2:
    if( fgets( cache, cacheSize, ch->hCFG ) == NULL ) 	return  e_failure;

    if( lrtrim(cache)[0] == '#' )
    {
	lrtrim( &cache[1] );
	if( stricmp(&cache[1], label) == 0 ) {
		// ****
		ch->seg = ftell( ch->hCFG );
		cfg_segEnd( ch );
		return  ch->seg;
	}
    }
    if( cache[0] == '[' )    goto _cfg_segment_ret;
    if( !feof( ch->hCFG ) )  goto _nextLine2;

_cfg_segment_ret:
    cfg_segEnd( ch );

    return  e_failure;

} // end of cfg_segment()

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//  cfg_segEnd()
// fail return e_failure
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
void cfg_segEnd( MXCCFG *ch )
{

    ch->segEnd = ftell( ch->hCFG );

_next_segEnd:
    if( fgets( cache, cacheSize, ch->hCFG ) == NULL ) {
	ch->segEnd = ftell( ch->hCFG );
	return;
    }

    if( cache[0] == '[' ) 	return;

    ch->segEnd = ftell( ch->hCFG );

    if( !feof( ch->hCFG ) )  goto _next_segEnd;

} // end of cfg_segEnd()

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//  cfg_make()
// fail return e_failure
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
int  cfg_make( MXCCFG *ch, char *grpKey, int rj )
{
    if( !(ch->hCFG) )  	return  0;

    if( !rj && (cfg_segment(ch, grpKey, NULL) >= 0) ) 	return  1;   // already exist
    sprintf( cache, "[%s]\r\n", grpKey );
    fseek( ch->hCFG, 0L, SEEK_END );
    fwrite( cache, strlen(grpKey)+4, 1, ch->hCFG);   // make a new one
    ch->segEnd = ch->seg = ftell(ch->hCFG);

    return  0;

} // end of cfg_make()


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//  cfg_findItem()
// fail return e_failure
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
hPOS  cfg_findItem( MXCCFG *ch, char *cfgKey )
{
    int   len;
    hPOS  pos;

    len = strlen( lrtrim(cfgKey) );

    fseek( ch->hCFG, ch->seg, SEEK_SET );     // locate the segment head

_nextItem:
    pos = ftell( ch->hCFG );

    if( fgets( cache, cacheSize, ch->hCFG ) == NULL) 	return e_failure;

    if( cache[0] == '[' || strlen(cache) == 0 )  // overlay boundary
    {
	 return( e_failure );
    }

    if( strchr(cache, '=') != NULL )
    {
       if( strnicmp(cache, cfgKey, len) == 0 )  	return pos;
    }

    if (!feof(ch->hCFG) ) 	goto   _nextItem;
    else   			return     e_failure;

}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//  return: fail, NULL; success, not 0
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
char *cfg_read( MXCCFG *ch, char *cfgKey, char *buf )
{
    char *ptr;

    if( ch == NULL || buf == NULL || cfgKey == NULL )	return  NULL;

    if( cfg_findItem( ch, cfgKey ) == e_failure ) 	return NULL;

    ptr = strchr(cache, '=');
    strcpy(buf, (char *)ptr + 1);
    return  lrtrim( buf );

} // end of cfg_read()


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//	cfg_write()
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
int  cfg_write( MXCCFG *ch, char *cfgKey, char *text, int ow )
{

    hPOS  curPos = ch->segEnd;        // no overwrite key
    hPOS  begPos, endPos;
    long  newItemlen;

    newItemlen = strlen(text)+strlen(cfgKey)+3;

    if( ow != 2 ) {
	if( ow && ((curPos = cfg_findItem( ch, cfgKey )) != e_failure) )
	{
		begPos = curPos + strlen(cache);
		endPos = begPos + (newItemlen-strlen(cache));
		ch->segEnd += (newItemlen-strlen(cache));
	}
	else  {
		curPos = ch->segEnd;
		begPos = curPos;
		endPos = curPos + newItemlen;
		ch->segEnd += newItemlen;
	}

	fileBytesMove( ch->hCFG, begPos, endPos );
	fseek( ch->hCFG, curPos, SEEK_SET );
    }
    else  {
	ch->segEnd += newItemlen;
	fseek(ch->hCFG, 0L, SEEK_END );
    }

    //fprintf(ch->hCFG,"%s=%s\n", cfgKey, text);
    fwrite( cfgKey, strlen(cfgKey), 1, ch->hCFG );
    fputc( '=', ch->hCFG );
    fwrite( text, strlen(text), 1, ch->hCFG );
    fputs( "\r\n", ch->hCFG );
    

    //fflush( ch->hCFG );  	// ask to swap cache to disk

    return 1;

} // end of cfg_write()

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//	cfg_setSeg()
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
int cfg_setSeg( MXCCFG *ch, hPOS newseg )
{
    ch->seg = newseg;
    ch->segEnd = newseg;

    fseek( ch->hCFG, ch->seg, SEEK_SET );

    cfg_segEnd( ch );

    return  0;

} // end of cfg_setSeg()


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//	cfg_getPos()
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
hPOS  cfg_getPos( MXCCFG *ch )
{
    return ch->seg;
}


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//	cfg_writeLine()
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
void  cfg_writeLine( MXCCFG *ch, char *memo )
{
    fputs( memo, ch->hCFG );
}


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//	cfg_readLine()
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
int  cfg_readLine( MXCCFG *ch, char *linebuf, int maxsize )
{
    if( ch == NULL || linebuf == NULL ) 	return  0;

    if ( maxsize <= 0 ) 	return 0;
    linebuf[0] = '\0';

    // need not check eof(), for fgets will check it
    if( fgets(linebuf, maxsize, ch->hCFG ) == NULL ) 	return 0;

    if( linebuf[0] == '[' ) { 		// overlay boundary
	 return cfg_readLine( ch, linebuf, maxsize );
    }
    return 1;

} // end of cfg_readLine()


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//	cfg_lable()
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
int  cfg_lable( MXCCFG *ch, char *cfgKey, char *label, int isIns )
{
    long  newItemlen, curPos;

    newItemlen = strlen(label)+3;

    if( isIns ) {
        if( cfg_segment(ch, cfgKey, label) == e_failure ) {

            if( cfg_segment(ch, cfgKey, NULL) == e_failure )
                return  e_failure;

            curPos = ch->segEnd;
            fileBytesMove( ch->hCFG, curPos, curPos+newItemlen );
	    fseek( ch->hCFG, curPos, SEEK_SET );

            fputc( '#', ch->hCFG );
            fwrite( label, strlen(label), 1, ch->hCFG );
            fputs( "\r\n", ch->hCFG );

            fflush( ch->hCFG );  	// ask to swap cache to disk

            return  0;   // already exist
        }
        return  1;
    }
    if( cfg_segment(ch, cfgKey, label) != e_failure ) {
            curPos = ch->seg;
            fileBytesMove(ch->hCFG, curPos, curPos-newItemlen);
            return  0;
    }
    return  1;

} // end of cfg_label()


/*
main()
{       int i;
        MXCCFG *ch;

        ch = cfg_open( "\\ls\\tgctl.txt", 0 );

        i = cfg_lable( ch, "MACRO", "Xilong", 1 );
        i = cfg_lable( ch, "eucfg", "Xilong", 1 );

        cfg_segment(ch, "macro", "Xilong");

        cfg_write( ch, "macro", "marlin", 1);
        cfg_segment(ch, "eucfg", "Xilong");
        cfg_write( ch, "macro", "marlin", 1);


        printf("%d\n", i);

}
*/

/*-=-=-=-=-=-=-=-=-=-=-=-= end of file msccfg.c  =-=-=-=-=-=-=-=-=-=-=-=-=-=*/