#ifndef _SQLITE_VDBE_GEN_H_
#define _SQLITE_VDBE_GEN_H_

#include "sqliteInt.h"
#include "vdbeInt.h"

/*
** Invoke this macro on memory cells just prior to changing the
** value of the cell.  This macro verifies that shallow copies are
** not misused.  A shallow copy of a string or blob just copies a
** pointer to the string or blob, not the content.  If the original
** is changed while the copy is still in use, the string or blob might
** be changed out from under the copy.  This macro verifies that nothing
** like that ever happens.
*/
#ifdef SQLITE_DEBUG
# define memAboutToChange(P,M) sqlite3VdbeMemAboutToChange(P,M)
#else
# define memAboutToChange(P,M)
#endif

/*
** Test a register to see if it exceeds the current maximum blob size.
** If it does, record the new maximum blob size.
*/
#if defined(SQLITE_TEST) && !defined(SQLITE_OMIT_BUILTIN_TEST)
# define UPDATE_MAX_BLOBSIZE(P)  updateMaxBlobsize(P)
#else
# define UPDATE_MAX_BLOBSIZE(P)
#endif


#ifdef SQLITE_DEBUG
#  define REGISTER_TRACE(R,M) if(db->flags&SQLITE_VdbeTrace)registerTrace(R,M)
#else
#  define REGISTER_TRACE(R,M)
#endif

/*
** Convert the given register into a string if it isn't one
** already. Return non-zero if a malloc() fails.
*/
#define Stringify(P, enc) \
   if(((P)->flags&(MEM_Str|MEM_Blob))==0 && sqlite3VdbeMemStringify(P,enc,0)) \
     { goto no_mem; }


#define Deephemeralize(P) \
   if( ((P)->flags&MEM_Ephem)!=0 \
       && sqlite3VdbeMemMakeWriteable(P) ){ goto no_mem;}

/* Return true if the cursor was opened using the OP_OpenSorter opcode. */
#define isSorter(x) ((x)->eCurType==CURTYPE_SORTER)

/*
** Allocate VdbeCursor number iCur.  Return a pointer to it.  Return NULL
** if we run out of memory.
*/
static VdbeCursor *allocateCursor(
  Vdbe *p,              /* The virtual machine */
  int iCur,             /* Index of the new VdbeCursor */
  int nField,           /* Number of fields in the table or index */
  int iDb,              /* Database the cursor belongs to, or -1 */
  u8 eCurType           /* Type of the new cursor */
){
  /* Find the memory cell that will be used to store the blob of memory
  ** required for this VdbeCursor structure. It is convenient to use a 
  ** vdbe memory cell to manage the memory allocation required for a
  ** VdbeCursor structure for the following reasons:
  **
  **   * Sometimes cursor numbers are used for a couple of different
  **     purposes in a vdbe program. The different uses might require
  **     different sized allocations. Memory cells provide growable
  **     allocations.
  **
  **   * When using ENABLE_MEMORY_MANAGEMENT, memory cell buffers can
  **     be freed lazily via the sqlite3_release_memory() API. This
  **     minimizes the number of malloc calls made by the system.
  **
  ** Memory cells for cursors are allocated at the top of the address
  ** space. Memory cell (p->nMem) corresponds to cursor 0. Space for
  ** cursor 1 is managed by memory cell (p->nMem-1), etc.
  */
  Mem *pMem = &p->aMem[p->nMem-iCur];

  int nByte;
  VdbeCursor *pCx = 0;
  nByte = 
      ROUND8(sizeof(VdbeCursor)) + 2*sizeof(u32)*nField + 
      (eCurType==CURTYPE_BTREE?sqlite3BtreeCursorSize():0);

  assert( iCur<p->nCursor );
  if( p->apCsr[iCur] ){
    sqlite3VdbeFreeCursor(p, p->apCsr[iCur]);
    p->apCsr[iCur] = 0;
  }
  if( SQLITE_OK==sqlite3VdbeMemClearAndResize(pMem, nByte) ){
    p->apCsr[iCur] = pCx = (VdbeCursor*)pMem->z;
    memset(pCx, 0, sizeof(VdbeCursor));
    pCx->eCurType = eCurType;
    pCx->iDb = iDb;
    pCx->nField = nField;
    pCx->aOffset = &pCx->aType[nField];
    if( eCurType==CURTYPE_BTREE ){
      pCx->uc.pCursor = (BtCursor*)
          &pMem->z[ROUND8(sizeof(VdbeCursor))+2*sizeof(u32)*nField];
      sqlite3BtreeCursorZero(pCx->uc.pCursor);
    }
  }
  return pCx;
}

/*
** Try to convert a value into a numeric representation if we can
** do so without loss of information.  In other words, if the string
** looks like a number, convert it into a number.  If it does not
** look like a number, leave it alone.
**
** If the bTryForInt flag is true, then extra effort is made to give
** an integer representation.  Strings that look like floating point
** values but which have no fractional component (example: '48.00')
** will have a MEM_Int representation when bTryForInt is true.
**
** If bTryForInt is false, then if the input string contains a decimal
** point or exponential notation, the result is only MEM_Real, even
** if there is an exact integer representation of the quantity.
*/
static void applyNumericAffinity(Mem *pRec, int bTryForInt){
  double rValue;
  i64 iValue;
  u8 enc = pRec->enc;
  assert( (pRec->flags & (MEM_Str|MEM_Int|MEM_Real))==MEM_Str );
  if( sqlite3AtoF(pRec->z, &rValue, pRec->n, enc)==0 ) return;
  if( 0==sqlite3Atoi64(pRec->z, &iValue, pRec->n, enc) ){
    pRec->u.i = iValue;
    pRec->flags |= MEM_Int;
  }else{
    pRec->u.r = rValue;
    pRec->flags |= MEM_Real;
    if( bTryForInt ) sqlite3VdbeIntegerAffinity(pRec);
  }
}

/*
** Processing is determine by the affinity parameter:
**
** SQLITE_AFF_INTEGER:
** SQLITE_AFF_REAL:
** SQLITE_AFF_NUMERIC:
**    Try to convert pRec to an integer representation or a 
**    floating-point representation if an integer representation
**    is not possible.  Note that the integer representation is
**    always preferred, even if the affinity is REAL, because
**    an integer representation is more space efficient on disk.
**
** SQLITE_AFF_TEXT:
**    Convert pRec to a text representation.
**
** SQLITE_AFF_BLOB:
**    No-op.  pRec is unchanged.
*/
static void applyAffinity(
  Mem *pRec,          /* The value to apply affinity to */
  char affinity,      /* The affinity to be applied */
  u8 enc              /* Use this text encoding */
){
  if( affinity>=SQLITE_AFF_NUMERIC ){
    assert( affinity==SQLITE_AFF_INTEGER || affinity==SQLITE_AFF_REAL
             || affinity==SQLITE_AFF_NUMERIC );
    if( (pRec->flags & MEM_Int)==0 ){
      if( (pRec->flags & MEM_Real)==0 ){
        if( pRec->flags & MEM_Str ) applyNumericAffinity(pRec,1);
      }else{
        sqlite3VdbeIntegerAffinity(pRec);
      }
    }
  }else if( affinity==SQLITE_AFF_TEXT ){
    /* Only attempt the conversion to TEXT if there is an integer or real
    ** representation (blob and NULL do not get converted) but no string
    ** representation.
    */
    if( 0==(pRec->flags&MEM_Str) && (pRec->flags&(MEM_Real|MEM_Int)) ){
      sqlite3VdbeMemStringify(pRec, enc, 1);
    }
    pRec->flags &= ~(MEM_Real|MEM_Int);
  }
}

/*
** pMem currently only holds a string type (or maybe a BLOB that we can
** interpret as a string if we want to).  Compute its corresponding
** numeric type, if has one.  Set the pMem->u.r and pMem->u.i fields
** accordingly.
*/
static u16 SQLITE_NOINLINE computeNumericType(Mem *pMem){
  assert( (pMem->flags & (MEM_Int|MEM_Real))==0 );
  assert( (pMem->flags & (MEM_Str|MEM_Blob))!=0 );
  if( sqlite3AtoF(pMem->z, &pMem->u.r, pMem->n, pMem->enc)==0 ){
    return 0;
  }
  if( sqlite3Atoi64(pMem->z, &pMem->u.i, pMem->n, pMem->enc)==SQLITE_OK ){
    return MEM_Int;
  }
  return MEM_Real;
}

/*
** Return the numeric type for pMem, either MEM_Int or MEM_Real or both or
** none.  
**
** Unlike applyNumericAffinity(), this routine does not modify pMem->flags.
** But it does set pMem->u.r and pMem->u.i appropriately.
*/
static u16 numericType(Mem *pMem){
  if( pMem->flags & (MEM_Int|MEM_Real) ){
    return pMem->flags & (MEM_Int|MEM_Real);
  }
  if( pMem->flags & (MEM_Str|MEM_Blob) ){
    return computeNumericType(pMem);
  }
  return 0;
}

/*
** Return the register of pOp->p2 after first preparing it to be
** overwritten with an integer value.
*/
static SQLITE_NOINLINE Mem *out2PrereleaseWithClear(Mem *pOut){
  sqlite3VdbeMemSetNull(pOut);
  pOut->flags = MEM_Int;
  return pOut;
}

static Mem *out2Prerelease(Vdbe *p, int p2){
  Mem *pOut;
  assert( p2>0 );
  assert( p2<=(p->nMem-p->nCursor) );
  pOut = &p->aMem[p2];
  memAboutToChange(p, pOut);
  if( VdbeMemDynamic(pOut) ){
    return out2PrereleaseWithClear(pOut);
  }else{
    pOut->flags = MEM_Int;
    return pOut;
  }
}

int sqlite3VdbeGenerate(
	FILE* out, 
	int qid, 
	Vdbe *p);

typedef int(*VdbeCompiledQuery)(Vdbe *p);
extern VdbeCompiledQuery const VdbeCompiledQueryCode[];
extern char const*const* const VdbeCompiledQueryText[];


static int sqlite3VdbeGetColumn(Vdbe *p, Op* pOp, int nColumns) 
{
  int rc = SQLITE_OK;
  i64 payloadSize64; /* Number of bytes in the record */
  int p2;            /* column number to retrieve */
  VdbeCursor *pC;    /* The VDBE cursor */
  BtCursor *pCrsr;   /* The BTree cursor */
  u32 *aOffset;      /* aOffset[i] is offset to start of data for i-th column */
  int len;           /* The length of the serialized data for the column */
  int i;             /* Loop counter */
  Mem *pDest;        /* Where to write the extracted value */
  Mem sMem;          /* For storing the record being decoded */
  const u8 *zData;   /* Part of the record being decoded */
  const u8 *zHdr;    /* Next unparsed byte of the header */
  const u8 *zEndHdr; /* Pointer to first byte after the header */
  u32 offset;        /* Offset into the data */
  u64 offset64;      /* 64-bit offset */
  u32 avail;         /* Number of bytes of available data */
  u32 t;             /* A type code from the record header */
  Mem *pReg;         /* PseudoTable input register */
  sqlite3 *db = p->db; 
  u8 encoding = ENC(db);

  pC = p->apCsr[pOp->p1];
  p2 = pOp->p2;

  for (i = 1; i < nColumns; i++) {
	  if (pOp[i].p2 > p2) { 
		  p2 = pOp[i].p2;
	  }
  }
  /* If the cursor cache is stale, bring it up-to-date */
  rc = sqlite3VdbeCursorMoveto(&pC, &p2);

  assert( pOp->p3>0 && pOp->p3<=(p->nMem-p->nCursor) );
  assert( pOp->p1>=0 && pOp->p1<p->nCursor );
  assert( pC!=0 );
  assert( p2<pC->nField );
  aOffset = pC->aOffset;
  assert( pC->eCurType!=CURTYPE_VTAB );
  assert( pC->eCurType!=CURTYPE_PSEUDO || pC->nullRow );
  assert( pC->eCurType!=CURTYPE_SORTER );
  pCrsr = pC->uc.pCursor;

  if( rc ) goto abort_due_to_error;
  if( pC->cacheStatus!=p->cacheCtr ){
    if( pC->nullRow ){
      if( pC->eCurType==CURTYPE_PSEUDO ){
        assert( pC->uc.pseudoTableReg>0 );
        pReg = &p->aMem[pC->uc.pseudoTableReg];
        assert( pReg->flags & MEM_Blob );
        assert( memIsValid(pReg) );
        pC->payloadSize = pC->szRow = avail = pReg->n;
        pC->aRow = (u8*)pReg->z;
      }else{
		  for (i = 0; i < nColumns; i++, pOp++) {
			  pDest = &p->aMem[pOp->p3];
			  memAboutToChange(p, pDest);
			  sqlite3VdbeMemSetNull(pDest);
			  UPDATE_MAX_BLOBSIZE(pDest);
			  REGISTER_TRACE(pOp->p3, pDest);
		  }
		  goto op_column_out;
      }
    }else{
      assert( pC->eCurType==CURTYPE_BTREE );
      assert( pCrsr );
      if( pC->isTable==0 ){
        assert( sqlite3BtreeCursorIsValid(pCrsr) );
        VVA_ONLY(rc =) sqlite3BtreeKeySize(pCrsr, &payloadSize64);
        assert( rc==SQLITE_OK ); /* True because of CursorMoveto() call above */
        /* sqlite3BtreeParseCellPtr() uses getVarint32() to extract the
        ** payload size, so it is impossible for payloadSize64 to be
        ** larger than 32 bits. */
        assert( (payloadSize64 & SQLITE_MAX_U32)==(u64)payloadSize64 );
        pC->aRow = sqlite3BtreeKeyFetch(pCrsr, &avail);
        pC->payloadSize = (u32)payloadSize64;
      }else{
        assert( sqlite3BtreeCursorIsValid(pCrsr) );
        VVA_ONLY(rc =) sqlite3BtreeDataSize(pCrsr, &pC->payloadSize);
        assert( rc==SQLITE_OK );   /* DataSize() cannot fail */
        pC->aRow = sqlite3BtreeDataFetch(pCrsr, &avail);
      }
      assert( avail<=65536 );  /* Maximum page size is 64KiB */
      if( pC->payloadSize <= (u32)avail ){
        pC->szRow = pC->payloadSize;
      }else if( pC->payloadSize > (u32)db->aLimit[SQLITE_LIMIT_LENGTH] ){
         rc = SQLITE_TOOBIG;
		 goto abort_due_to_error;
      }else{
        pC->szRow = avail;
      }
    }
    pC->cacheStatus = p->cacheCtr;
    pC->iHdrOffset = getVarint32(pC->aRow, offset);
    pC->nHdrParsed = 0;
    aOffset[0] = offset;


    if( avail<offset ){
      /* pC->aRow does not have to hold the entire row, but it does at least
      ** need to cover the header of the record.  If pC->aRow does not contain
      ** the complete header, then set it to zero, forcing the header to be
      ** dynamically allocated. */
      pC->aRow = 0;
      pC->szRow = 0;

      /* Make sure a corrupt database has not given us an oversize header.
      ** Do this now to avoid an oversize memory allocation.
      **
      ** Type entries can be between 1 and 5 bytes each.  But 4 and 5 byte
      ** types use so much data space that there can only be 4096 and 32 of
      ** them, respectively.  So the maximum header length results from a
      ** 3-byte type for each of the maximum of 32768 columns plus three
      ** extra bytes for the header length itself.  32768*3 + 3 = 98307.
      */
      if( offset > 98307 || offset > pC->payloadSize ){
        rc = SQLITE_CORRUPT_BKPT;
        goto abort_due_to_error;
      }
    }

    /* The following goto is an optimization.  It can be omitted and
    ** everything will still work.  But OP_Column is measurably faster
    ** by skipping the subsequent conditional, which is always true.
    */
    assert( pC->nHdrParsed<=p2 );         /* Conditional skipped */
    goto op_column_read_header;
  }

  /* Make sure at least the first p2+1 entries of the header have been
  ** parsed and valid information is in aOffset[] and pC->aType[].
  */
  if( pC->nHdrParsed<=p2 ){
    /* If there is more header available for parsing in the record, try
    ** to extract additional fields up through the p2+1-th field 
    */
    op_column_read_header:
    if( pC->iHdrOffset<aOffset[0] ){
      /* Make sure zData points to enough of the record to cover the header. */
      if( pC->aRow==0 ){
        memset(&sMem, 0, sizeof(sMem));
        rc = sqlite3VdbeMemFromBtree(pCrsr, 0, aOffset[0], !pC->isTable, &sMem);
        if( rc!=SQLITE_OK ) goto abort_due_to_error;
        zData = (u8*)sMem.z;
      }else{
        zData = pC->aRow;
      }
  
      /* Fill in pC->aType[i] and aOffset[i] values through the p2-th field. */
      i = pC->nHdrParsed;
      offset64 = aOffset[i];
      zHdr = zData + pC->iHdrOffset;
      zEndHdr = zData + aOffset[0];
      assert( i<=p2 && zHdr<zEndHdr );
      do{
        if( (t = zHdr[0])<0x80 ){
          zHdr++;
          offset64 += sqlite3VdbeOneByteSerialTypeLen(t);
        }else{
          zHdr += sqlite3GetVarint32(zHdr, &t);
          offset64 += sqlite3VdbeSerialTypeLen(t);
        }
        pC->aType[i++] = t;
        aOffset[i] = (u32)(offset64 & 0xffffffff);
      }while( i<=p2 && zHdr<zEndHdr );
      pC->nHdrParsed = i;
      pC->iHdrOffset = (u32)(zHdr - zData);
      if( pC->aRow==0 ) sqlite3VdbeMemRelease(&sMem);
  
      /* The record is corrupt if any of the following are true:
      ** (1) the bytes of the header extend past the declared header size
      ** (2) the entire header was used but not all data was used
      ** (3) the end of the data extends beyond the end of the record.
      */
      if( (zHdr>=zEndHdr && (zHdr>zEndHdr || offset64!=pC->payloadSize))
       || (offset64 > pC->payloadSize)
      ){
        rc = SQLITE_CORRUPT_BKPT;
        goto abort_due_to_error;
      }
    }else{
      t = 0;
    }

    /* If after trying to extract new entries from the header, nHdrParsed is
    ** still not up to p2, that means that the record has fewer than p2
    ** columns.  So the result will be either the default value or a NULL.
    */
    if( pC->nHdrParsed<=p2 ){
		for (i = 0; i < nColumns; i++, pOp++) {
			pDest = &p->aMem[pOp->p3];
			memAboutToChange(p, pDest);
			if( pOp->p4type==P4_MEM ){
				sqlite3VdbeMemShallowCopy(pDest, pOp->p4.pMem, MEM_Static);
			}else{
				sqlite3VdbeMemSetNull(pDest);
			}
			UPDATE_MAX_BLOBSIZE(pDest);
			REGISTER_TRACE(pOp->p3, pDest);
		}
		goto op_column_out;
    }
  }else{
    t = pC->aType[p2];
  }

  /* Extract the content for the p2+1-th column.  Control can only
  ** reach this point if aOffset[p2], aOffset[p2+1], and pC->aType[p2] are
  ** all valid.
  */

  for (i = 0; i < nColumns; i++, pOp++) {
	  pDest = &p->aMem[pOp->p3];
	  memAboutToChange(p, pDest);
	  p2 = pOp->p2;
	  t=pC->aType[p2];

	  assert( p2<pC->nHdrParsed );
	  assert( rc==SQLITE_OK );
	  assert( sqlite3VdbeCheckMemInvariants(pDest) );
	  if( VdbeMemDynamic(pDest) ) sqlite3VdbeMemSetNull(pDest);
	  assert( t==pC->aType[p2] );
	  pDest->enc = encoding;
	  if( pC->szRow>=aOffset[p2+1] ){
		  /* This is the common case where the desired content fits on the original
		  ** page - where the content is not on an overflow page */
		  zData = pC->aRow + aOffset[p2];
		  if( t<12 ){
			  sqlite3VdbeSerialGet(zData, t, pDest);
		  }else{
			  /* If the column value is a string, we need a persistent value, not
			  ** a MEM_Ephem value.  This branch is a fast short-cut that is equivalent
			  ** to calling sqlite3VdbeSerialGet() and sqlite3VdbeDeephemeralize().
			  */
			  static const u16 aFlag[] = { MEM_Blob, MEM_Str|MEM_Term };
			  pDest->n = len = (t-12)/2;
			  if( pDest->szMalloc < len+2 ){
				  pDest->flags = MEM_Null;
				  if( sqlite3VdbeMemGrow(pDest, len+2, 0) ) { 
					  rc = SQLITE_NOMEM_BKPT;
					  goto abort_due_to_error;
				  }
			  }else{
				  pDest->z = pDest->zMalloc;
			  }
			  memcpy(pDest->z, zData, len);
			  pDest->z[len] = 0;
			  pDest->z[len+1] = 0;
			  pDest->flags = aFlag[t&1];
		  }
	  }else{
		  /* This branch happens only when content is on overflow pages */
		  if( ((pOp->p5 & (OPFLAG_LENGTHARG|OPFLAG_TYPEOFARG))!=0
			   && ((t>=12 && (t&1)==0) || (pOp->p5 & OPFLAG_TYPEOFARG)!=0))
			  || (len = sqlite3VdbeSerialTypeLen(t))==0
			  ){
			  /* Content is irrelevant for
			  **    1. the typeof() function,
			  **    2. the length(X) function if X is a blob, and
			  **    3. if the content length is zero.
			  ** So we might as well use bogus content rather than reading
			  ** content from disk. */
			  static u8 aZero[8];  /* This is the bogus content */
			  sqlite3VdbeSerialGet(aZero, t, pDest);
		  }else{
			  rc = sqlite3VdbeMemFromBtree(pCrsr, aOffset[p2], len, !pC->isTable,
										   pDest);
			  if( rc!=SQLITE_OK ) goto abort_due_to_error;
			  sqlite3VdbeSerialGet((const u8*)pDest->z, t, pDest);
			  pDest->flags &= ~MEM_Ephem;
		  }
	  }
	  UPDATE_MAX_BLOBSIZE(pDest);
	  REGISTER_TRACE(pOp->p3, pDest);
  }
op_column_out:
abort_due_to_error:
  return rc;
}

#endif
