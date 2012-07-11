

/*------------
 !                      _xXin()
 ! xin(str, str1, str2, str3)
 !     if( str = str1 )	return 1
 !     if( str = str2 )	return 2
 !-----------------------------------------------------------------------*/
// #pragma argsused		/* Remed by NiuJingyu */
short PD_style _xXin(OpndType *lpOpnd, short ParaNum, short *OpndTop)
{
    unsigned short i, len, k;
    char 	   *sz;
    char	   buf[256];

    sz =  = xGetOpndString(&lpOpnd[0]);
    if( sz != NULL ) {
	len = strlen( strZcpy(buf, sz, 256) );
    } else {
	return  1;
    }

    for( k = 1;   k < ParaNum;   k++ ) {
	sz =  = xGetOpndString(&lpOpnd[k]);
	if( sz == NULL ) {
		return  1;
	}

	for(i = 0;  i < len;  i++ ) {

		register char   c1, c2;

		c1 = buf[i];
		c2 = sz[i];

		if( ( c1 != c2  && c1 != '*' && c2 != '*') || \
							    (c2 == '\0') ) {
			break;
		}
	}

	if( i >= len ) {
		return  0;
	}
    }

    return  0;

}