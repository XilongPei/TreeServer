    {
	char  *FieldName[2];
	char   szGbNdx[256];

	sprintf(szGbNdx, "ORDBY%03X", intOfThread&0xFFF);
	makefilename(szGbNdx, tmpPath, szGbNdx);

	//build the index, no before index for reference
	if( fFrTo.wGbKeyLen == SHRT_MAX ) {
	    FieldName[0] = fFrTo.cOrderbyKey;
	    FieldName[1] = NULL;

	    fFrTo.orderbyBh = IndexBuild(dtp, FieldName, szGbNdx, BTREE_FOR_OPENDBF);
	} else {
	    fFrTo.orderbyBh = IndexBuildKeyExpr(dtp, fFrTo.cOrderbyKey, \
						fFrTo.wObKeyLen, \
						szGbNdx);
	}

	if( fFrTo.orderbyBh == NULL ) {
	    qsError = 5008;
	    strZcpy(szAsqlErrBuf, fFrTo.cOrderbyKey, sizeof(szAsqlErrBuf));
	    return  FALSE;
	}
    }
