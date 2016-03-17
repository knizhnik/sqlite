#include "vdbe-gen.h"
char const* const VdbeCompiledQueryTextQ2 = "select\n    sum(l_extendedprice*l_discount) as revenue\nfrom\n    lineitem\nwhere\n    l_shipdate between '1996-01-01' and '1997-01-01'\n    and l_discount between 0.08 and 0.1\n    and l_quantity < 24;";
int VdbeCompiledQueryCodeQ2(Vdbe *p) {
static void* const labels[]={&&L0,&&L1,&&L2,&&L3,&&L4,&&L5,&&L6,&&L7,&&L8,&&L9,&&L10,&&L11,&&L12,&&L13,&&L14,&&L15,&&L16,&&L17,&&L18,&&L19,&&L20,&&L21,&&L22,&&L23,&&L24,&&L25,&&L26,&&L27,&&L28,&&L29,&&L30,&&L31,&&Lend};
 void * target; int rc = SQLITE_OK; sqlite3 *db = p->db; u8 resetSchemaOnFault = 0; u8 encoding = ENC(db); int iCompare = 0; unsigned nVmStep = 0; unsigned nProgressLimit = 0; Mem *aMem = p->aMem; Mem *pIn1 = 0; Mem *pIn2 = 0; Mem *pIn3 = 0; Mem *pOut = 0; int *aPermute = 0; i64 lastRowid = db->lastRowid; assert( p->magic==VDBE_MAGIC_RUN ); sqlite3VdbeEnter(p); if( p->rc==SQLITE_NOMEM ){ goto no_mem; } assert( p->rc==SQLITE_OK || (p->rc&0xff)==SQLITE_BUSY ); assert( p->bIsReader || p->readOnly!=0 ); p->rc = SQLITE_OK; p->iCurrentTime = 0; assert( p->explain==0 ); p->pResultSet = 0; db->busyHandler.nBusy = 0; if( db->u1.isInterrupted ) goto abort_due_to_interrupt; sqlite3VdbeIOTraceSql(p); goto *labels[p->pc]; 
L0:
do {{ if( 24 ) goto L24; } } while(0);
L1:
do {{ int cnt; u16 nullFlag; pOut = out2Prerelease(p, 1); cnt = 3-1; assert( 3<=(p->nMem-p->nCursor) ); pOut->flags = nullFlag = 0 ? (MEM_Null|MEM_Cleared) : MEM_Null; while( cnt>0 ){ pOut++; memAboutToChange(p, pOut); sqlite3VdbeMemSetNull(pOut); pOut->flags = nullFlag; cnt--; } } } while(0);
L2:
do {{ int nField; KeyInfo *pKeyInfo; int p2; int iDb; int wrFlag; Btree *pX; VdbeCursor *pCur; Db *pDb; if (54 == OP_ReopenIdx) { assert( 0==0 || 0==OPFLAG_SEEKEQ ); assert( -14==P4_KEYINFO ); pCur = p->apCsr[0]; if( pCur && pCur->pgnoRoot==(u32)2 ){ assert( pCur->iDb==0 ); goto open_cursor_set_hints2; } } assert( 54==OP_OpenWrite || 0==0 || 0==OPFLAG_SEEKEQ ); assert( p->bIsReader ); assert( 54==OP_OpenRead || 54==OP_ReopenIdx || p->readOnly==0 ); if( p->expired ){ rc = SQLITE_ABORT_ROLLBACK; goto abort_due_to_error; } nField = 0; pKeyInfo = 0; p2 = 2; iDb = 0; assert( iDb>=0 && iDb<db->nDb ); assert( DbMaskTest(p->btreeMask, iDb) ); pDb = &db->aDb[iDb]; pX = pDb->pBt; assert( pX!=0 ); if( 54==OP_OpenWrite ){ assert( OPFLAG_FORDELETE==BTREE_FORDELETE ); wrFlag = BTREE_WRCSR | (0 & OPFLAG_FORDELETE); assert( sqlite3SchemaMutexHeld(db, iDb, 0) ); if( pDb->pSchema->file_format < p->minWriteFileFormat ){ p->minWriteFileFormat = pDb->pSchema->file_format; } }else{ wrFlag = 0; } if( 0 & OPFLAG_P2ISREG ){ assert( p2>0 ); assert( p2<=(p->nMem-p->nCursor) ); pIn2 = &aMem[p2]; assert( memIsValid(pIn2) ); assert( (pIn2->flags & MEM_Int)!=0 ); sqlite3VdbeMemIntegerify(pIn2); p2 = (int)pIn2->u.i; assert( p2>=2 ); } if( -14==P4_KEYINFO ){ pKeyInfo = p->aOp[2].p4.pKeyInfo; assert( pKeyInfo->enc==ENC(db) ); assert( pKeyInfo->db==db ); nField = pKeyInfo->nField+pKeyInfo->nXField; }else if( -14==P4_INT32 ){ nField = 11; } assert( 0>=0 ); assert( nField>=0 ); testcase( nField==0 ); pCur = allocateCursor(p, 0, nField, iDb, CURTYPE_BTREE); if( pCur==0 ) goto no_mem; pCur->nullRow = 1; pCur->isOrdered = 1; pCur->pgnoRoot = p2; rc = sqlite3BtreeCursor(pX, p2, wrFlag, pKeyInfo, pCur->uc.pCursor); pCur->pKeyInfo = pKeyInfo; pCur->isTable = -14!=P4_KEYINFO; open_cursor_set_hints2: assert( OPFLAG_BULKCSR==BTREE_BULKLOAD ); assert( OPFLAG_SEEKEQ==BTREE_SEEK_EQ ); testcase( 0 & OPFLAG_BULKCSR ); sqlite3BtreeCursorHintFlags(pCur->uc.pCursor, (0 & (OPFLAG_BULKCSR|OPFLAG_SEEKEQ))); if( rc ) goto abort_due_to_error; } } while(0);
L3:
do {{ VdbeCursor *pC; BtCursor *pCrsr; int res; if (108 != OP_Rewind) { p->aCounter[SQLITE_STMTSTATUS_SORT]++; } assert( 0>=0 && 0<p->nCursor ); pC = p->apCsr[0]; assert( pC!=0 ); assert( isSorter(pC)==(108==OP_SorterSort) ); res = 1; if( isSorter(pC) ){ rc = sqlite3VdbeSorterRewind(pC, &res); }else{ assert( pC->eCurType==CURTYPE_BTREE ); pCrsr = pC->uc.pCursor; assert( pCrsr ); rc = sqlite3BtreeFirst(pCrsr, &res); pC->deferredMoveto = 0; pC->cacheStatus = CACHE_STALE; } if( rc ) goto abort_due_to_error; pC->nullRow = (u8)res; assert( 19>0 && 19<p->nOp ); VdbeBranchTaken(res!=0,2); if( res ) goto L19; } } while(0);
L4:
do {{ int rc = sqlite3VdbeGetColumn(p, &p->aOp[4]); int p1 = 0; int p2 = 10; int p3 = 4; if (rc != SQLITE_OK) { if (rc == SQLITE_TOOBIG) { goto too_big; } else if (rc == SQLITE_NOMEM_BKPT) { goto no_mem; } else { goto abort_due_to_error; } } } } while(0);
L5:
do {{ int res; char affinity; u16 flags1; u16 flags3; pIn1 = &aMem[6]; pIn3 = &aMem[4]; flags1 = pIn1->flags; flags3 = pIn3->flags; if( (flags1 | flags3)&MEM_Null ){ if( 83 & SQLITE_NULLEQ ){ assert( 82==OP_Eq || 82==OP_Ne ); assert( (flags1 & MEM_Cleared)==0 ); assert( (83 & SQLITE_JUMPIFNULL)==0 ); if( (flags1&MEM_Null)!=0 && (flags3&MEM_Null)!=0 && (flags3&MEM_Cleared)==0 ){ res = 0; }else{ res = 1; } }else{ if( 83 & SQLITE_STOREP2 ){ pOut = &aMem[18]; memAboutToChange(p, pOut); MemSetTypeFlag(pOut, MEM_Null); REGISTER_TRACE(18, pOut); }else{ VdbeBranchTaken(2,3); if( 83 & SQLITE_JUMPIFNULL ){ goto L18; } } break; } }else{ affinity = 83 & SQLITE_AFF_MASK; if( affinity>=SQLITE_AFF_NUMERIC ){ if( (flags1 & (MEM_Int|MEM_Real|MEM_Str))==MEM_Str ){ applyNumericAffinity(pIn1,0); } if( (flags3 & (MEM_Int|MEM_Real|MEM_Str))==MEM_Str ){ applyNumericAffinity(pIn3,0); } }else if( affinity==SQLITE_AFF_TEXT ){ if( (flags1 & MEM_Str)==0 && (flags1 & (MEM_Int|MEM_Real))!=0 ){ testcase( pIn1->flags & MEM_Int ); testcase( pIn1->flags & MEM_Real ); sqlite3VdbeMemStringify(pIn1, encoding, 1); testcase( (flags1&MEM_Dyn) != (pIn1->flags&MEM_Dyn) ); flags1 = (pIn1->flags & ~MEM_TypeMask) | (flags1 & MEM_TypeMask); } if( (flags3 & MEM_Str)==0 && (flags3 & (MEM_Int|MEM_Real))!=0 ){ testcase( pIn3->flags & MEM_Int ); testcase( pIn3->flags & MEM_Real ); sqlite3VdbeMemStringify(pIn3, encoding, 1); testcase( (flags3&MEM_Dyn) != (pIn3->flags&MEM_Dyn) ); flags3 = (pIn3->flags & ~MEM_TypeMask) | (flags3 & MEM_TypeMask); } } assert( -4==P4_COLLSEQ || p->aOp[5].p4.pColl==0 ); if( flags1 & MEM_Zero ){ sqlite3VdbeMemExpandBlob(pIn1); flags1 &= ~MEM_Zero; } if( flags3 & MEM_Zero ){ sqlite3VdbeMemExpandBlob(pIn3); flags3 &= ~MEM_Zero; } res = sqlite3MemCompare(pIn3, pIn1, p->aOp[5].p4.pColl); } switch( 82 ){ case OP_Eq: res = res==0; break; case OP_Ne: res = res!=0; break; case OP_Lt: res = res<0; break; case OP_Le: res = res<=0; break; case OP_Gt: res = res>0; break; default: res = res>=0; break; } assert( (pIn1->flags & MEM_Dyn) == (flags1 & MEM_Dyn) ); pIn1->flags = flags1; assert( (pIn3->flags & MEM_Dyn) == (flags3 & MEM_Dyn) ); pIn3->flags = flags3; if( 83 & SQLITE_STOREP2 ){ pOut = &aMem[18]; memAboutToChange(p, pOut); MemSetTypeFlag(pOut, MEM_Int); pOut->u.i = res; REGISTER_TRACE(18, pOut); }else{ VdbeBranchTaken(res!=0, (83 & SQLITE_NULLEQ)?2:3); if( res ){ goto L18; } } } } while(0);
L6:
do {{ int res; char affinity; u16 flags1; u16 flags3; pIn1 = &aMem[7]; pIn3 = &aMem[4]; flags1 = pIn1->flags; flags3 = pIn3->flags; if( (flags1 | flags3)&MEM_Null ){ if( 83 & SQLITE_NULLEQ ){ assert( 80==OP_Eq || 80==OP_Ne ); assert( (flags1 & MEM_Cleared)==0 ); assert( (83 & SQLITE_JUMPIFNULL)==0 ); if( (flags1&MEM_Null)!=0 && (flags3&MEM_Null)!=0 && (flags3&MEM_Cleared)==0 ){ res = 0; }else{ res = 1; } }else{ if( 83 & SQLITE_STOREP2 ){ pOut = &aMem[18]; memAboutToChange(p, pOut); MemSetTypeFlag(pOut, MEM_Null); REGISTER_TRACE(18, pOut); }else{ VdbeBranchTaken(2,3); if( 83 & SQLITE_JUMPIFNULL ){ goto L18; } } break; } }else{ affinity = 83 & SQLITE_AFF_MASK; if( affinity>=SQLITE_AFF_NUMERIC ){ if( (flags1 & (MEM_Int|MEM_Real|MEM_Str))==MEM_Str ){ applyNumericAffinity(pIn1,0); } if( (flags3 & (MEM_Int|MEM_Real|MEM_Str))==MEM_Str ){ applyNumericAffinity(pIn3,0); } }else if( affinity==SQLITE_AFF_TEXT ){ if( (flags1 & MEM_Str)==0 && (flags1 & (MEM_Int|MEM_Real))!=0 ){ testcase( pIn1->flags & MEM_Int ); testcase( pIn1->flags & MEM_Real ); sqlite3VdbeMemStringify(pIn1, encoding, 1); testcase( (flags1&MEM_Dyn) != (pIn1->flags&MEM_Dyn) ); flags1 = (pIn1->flags & ~MEM_TypeMask) | (flags1 & MEM_TypeMask); } if( (flags3 & MEM_Str)==0 && (flags3 & (MEM_Int|MEM_Real))!=0 ){ testcase( pIn3->flags & MEM_Int ); testcase( pIn3->flags & MEM_Real ); sqlite3VdbeMemStringify(pIn3, encoding, 1); testcase( (flags3&MEM_Dyn) != (pIn3->flags&MEM_Dyn) ); flags3 = (pIn3->flags & ~MEM_TypeMask) | (flags3 & MEM_TypeMask); } } assert( -4==P4_COLLSEQ || p->aOp[6].p4.pColl==0 ); if( flags1 & MEM_Zero ){ sqlite3VdbeMemExpandBlob(pIn1); flags1 &= ~MEM_Zero; } if( flags3 & MEM_Zero ){ sqlite3VdbeMemExpandBlob(pIn3); flags3 &= ~MEM_Zero; } res = sqlite3MemCompare(pIn3, pIn1, p->aOp[6].p4.pColl); } switch( 80 ){ case OP_Eq: res = res==0; break; case OP_Ne: res = res!=0; break; case OP_Lt: res = res<0; break; case OP_Le: res = res<=0; break; case OP_Gt: res = res>0; break; default: res = res>=0; break; } assert( (pIn1->flags & MEM_Dyn) == (flags1 & MEM_Dyn) ); pIn1->flags = flags1; assert( (pIn3->flags & MEM_Dyn) == (flags3 & MEM_Dyn) ); pIn3->flags = flags3; if( 83 & SQLITE_STOREP2 ){ pOut = &aMem[18]; memAboutToChange(p, pOut); MemSetTypeFlag(pOut, MEM_Int); pOut->u.i = res; REGISTER_TRACE(18, pOut); }else{ VdbeBranchTaken(res!=0, (83 & SQLITE_NULLEQ)?2:3); if( res ){ goto L18; } } } } while(0);
L7:
do {{ int rc = sqlite3VdbeGetColumn(p, &p->aOp[7]); int p1 = 0; int p2 = 6; int p3 = 5; if (rc != SQLITE_OK) { if (rc == SQLITE_TOOBIG) { goto too_big; } else if (rc == SQLITE_NOMEM_BKPT) { goto no_mem; } else { goto abort_due_to_error; } } } } while(0);
L8:
do {{ pIn1 = &aMem[5]; if( pIn1->flags & MEM_Int ){ sqlite3VdbeMemRealify(pIn1); } } } while(0);
L9:
do {{ int res; char affinity; u16 flags1; u16 flags3; pIn1 = &aMem[9]; pIn3 = &aMem[5]; flags1 = pIn1->flags; flags3 = pIn3->flags; if( (flags1 | flags3)&MEM_Null ){ if( 85 & SQLITE_NULLEQ ){ assert( 82==OP_Eq || 82==OP_Ne ); assert( (flags1 & MEM_Cleared)==0 ); assert( (85 & SQLITE_JUMPIFNULL)==0 ); if( (flags1&MEM_Null)!=0 && (flags3&MEM_Null)!=0 && (flags3&MEM_Cleared)==0 ){ res = 0; }else{ res = 1; } }else{ if( 85 & SQLITE_STOREP2 ){ pOut = &aMem[18]; memAboutToChange(p, pOut); MemSetTypeFlag(pOut, MEM_Null); REGISTER_TRACE(18, pOut); }else{ VdbeBranchTaken(2,3); if( 85 & SQLITE_JUMPIFNULL ){ goto L18; } } break; } }else{ affinity = 85 & SQLITE_AFF_MASK; if( affinity>=SQLITE_AFF_NUMERIC ){ if( (flags1 & (MEM_Int|MEM_Real|MEM_Str))==MEM_Str ){ applyNumericAffinity(pIn1,0); } if( (flags3 & (MEM_Int|MEM_Real|MEM_Str))==MEM_Str ){ applyNumericAffinity(pIn3,0); } }else if( affinity==SQLITE_AFF_TEXT ){ if( (flags1 & MEM_Str)==0 && (flags1 & (MEM_Int|MEM_Real))!=0 ){ testcase( pIn1->flags & MEM_Int ); testcase( pIn1->flags & MEM_Real ); sqlite3VdbeMemStringify(pIn1, encoding, 1); testcase( (flags1&MEM_Dyn) != (pIn1->flags&MEM_Dyn) ); flags1 = (pIn1->flags & ~MEM_TypeMask) | (flags1 & MEM_TypeMask); } if( (flags3 & MEM_Str)==0 && (flags3 & (MEM_Int|MEM_Real))!=0 ){ testcase( pIn3->flags & MEM_Int ); testcase( pIn3->flags & MEM_Real ); sqlite3VdbeMemStringify(pIn3, encoding, 1); testcase( (flags3&MEM_Dyn) != (pIn3->flags&MEM_Dyn) ); flags3 = (pIn3->flags & ~MEM_TypeMask) | (flags3 & MEM_TypeMask); } } assert( -4==P4_COLLSEQ || p->aOp[9].p4.pColl==0 ); if( flags1 & MEM_Zero ){ sqlite3VdbeMemExpandBlob(pIn1); flags1 &= ~MEM_Zero; } if( flags3 & MEM_Zero ){ sqlite3VdbeMemExpandBlob(pIn3); flags3 &= ~MEM_Zero; } res = sqlite3MemCompare(pIn3, pIn1, p->aOp[9].p4.pColl); } switch( 82 ){ case OP_Eq: res = res==0; break; case OP_Ne: res = res!=0; break; case OP_Lt: res = res<0; break; case OP_Le: res = res<=0; break; case OP_Gt: res = res>0; break; default: res = res>=0; break; } assert( (pIn1->flags & MEM_Dyn) == (flags1 & MEM_Dyn) ); pIn1->flags = flags1; assert( (pIn3->flags & MEM_Dyn) == (flags3 & MEM_Dyn) ); pIn3->flags = flags3; if( 85 & SQLITE_STOREP2 ){ pOut = &aMem[18]; memAboutToChange(p, pOut); MemSetTypeFlag(pOut, MEM_Int); pOut->u.i = res; REGISTER_TRACE(18, pOut); }else{ VdbeBranchTaken(res!=0, (85 & SQLITE_NULLEQ)?2:3); if( res ){ goto L18; } } } } while(0);
L10:
do {{ int res; char affinity; u16 flags1; u16 flags3; pIn1 = &aMem[10]; pIn3 = &aMem[5]; flags1 = pIn1->flags; flags3 = pIn3->flags; if( (flags1 | flags3)&MEM_Null ){ if( 85 & SQLITE_NULLEQ ){ assert( 80==OP_Eq || 80==OP_Ne ); assert( (flags1 & MEM_Cleared)==0 ); assert( (85 & SQLITE_JUMPIFNULL)==0 ); if( (flags1&MEM_Null)!=0 && (flags3&MEM_Null)!=0 && (flags3&MEM_Cleared)==0 ){ res = 0; }else{ res = 1; } }else{ if( 85 & SQLITE_STOREP2 ){ pOut = &aMem[18]; memAboutToChange(p, pOut); MemSetTypeFlag(pOut, MEM_Null); REGISTER_TRACE(18, pOut); }else{ VdbeBranchTaken(2,3); if( 85 & SQLITE_JUMPIFNULL ){ goto L18; } } break; } }else{ affinity = 85 & SQLITE_AFF_MASK; if( affinity>=SQLITE_AFF_NUMERIC ){ if( (flags1 & (MEM_Int|MEM_Real|MEM_Str))==MEM_Str ){ applyNumericAffinity(pIn1,0); } if( (flags3 & (MEM_Int|MEM_Real|MEM_Str))==MEM_Str ){ applyNumericAffinity(pIn3,0); } }else if( affinity==SQLITE_AFF_TEXT ){ if( (flags1 & MEM_Str)==0 && (flags1 & (MEM_Int|MEM_Real))!=0 ){ testcase( pIn1->flags & MEM_Int ); testcase( pIn1->flags & MEM_Real ); sqlite3VdbeMemStringify(pIn1, encoding, 1); testcase( (flags1&MEM_Dyn) != (pIn1->flags&MEM_Dyn) ); flags1 = (pIn1->flags & ~MEM_TypeMask) | (flags1 & MEM_TypeMask); } if( (flags3 & MEM_Str)==0 && (flags3 & (MEM_Int|MEM_Real))!=0 ){ testcase( pIn3->flags & MEM_Int ); testcase( pIn3->flags & MEM_Real ); sqlite3VdbeMemStringify(pIn3, encoding, 1); testcase( (flags3&MEM_Dyn) != (pIn3->flags&MEM_Dyn) ); flags3 = (pIn3->flags & ~MEM_TypeMask) | (flags3 & MEM_TypeMask); } } assert( -4==P4_COLLSEQ || p->aOp[10].p4.pColl==0 ); if( flags1 & MEM_Zero ){ sqlite3VdbeMemExpandBlob(pIn1); flags1 &= ~MEM_Zero; } if( flags3 & MEM_Zero ){ sqlite3VdbeMemExpandBlob(pIn3); flags3 &= ~MEM_Zero; } res = sqlite3MemCompare(pIn3, pIn1, p->aOp[10].p4.pColl); } switch( 80 ){ case OP_Eq: res = res==0; break; case OP_Ne: res = res!=0; break; case OP_Lt: res = res<0; break; case OP_Le: res = res<=0; break; case OP_Gt: res = res>0; break; default: res = res>=0; break; } assert( (pIn1->flags & MEM_Dyn) == (flags1 & MEM_Dyn) ); pIn1->flags = flags1; assert( (pIn3->flags & MEM_Dyn) == (flags3 & MEM_Dyn) ); pIn3->flags = flags3; if( 85 & SQLITE_STOREP2 ){ pOut = &aMem[18]; memAboutToChange(p, pOut); MemSetTypeFlag(pOut, MEM_Int); pOut->u.i = res; REGISTER_TRACE(18, pOut); }else{ VdbeBranchTaken(res!=0, (85 & SQLITE_NULLEQ)?2:3); if( res ){ goto L18; } } } } while(0);
L11:
do {{ int rc = sqlite3VdbeGetColumn(p, &p->aOp[11]); int p1 = 0; int p2 = 4; int p3 = 8; if (rc != SQLITE_OK) { if (rc == SQLITE_TOOBIG) { goto too_big; } else if (rc == SQLITE_NOMEM_BKPT) { goto no_mem; } else { goto abort_due_to_error; } } } } while(0);
L12:
do {{ pIn1 = &aMem[8]; if( pIn1->flags & MEM_Int ){ sqlite3VdbeMemRealify(pIn1); } } } while(0);
L13:
do {{ int res; char affinity; u16 flags1; u16 flags3; pIn1 = &aMem[11]; pIn3 = &aMem[8]; flags1 = pIn1->flags; flags3 = pIn3->flags; if( (flags1 | flags3)&MEM_Null ){ if( 85 & SQLITE_NULLEQ ){ assert( 83==OP_Eq || 83==OP_Ne ); assert( (flags1 & MEM_Cleared)==0 ); assert( (85 & SQLITE_JUMPIFNULL)==0 ); if( (flags1&MEM_Null)!=0 && (flags3&MEM_Null)!=0 && (flags3&MEM_Cleared)==0 ){ res = 0; }else{ res = 1; } }else{ if( 85 & SQLITE_STOREP2 ){ pOut = &aMem[18]; memAboutToChange(p, pOut); MemSetTypeFlag(pOut, MEM_Null); REGISTER_TRACE(18, pOut); }else{ VdbeBranchTaken(2,3); if( 85 & SQLITE_JUMPIFNULL ){ goto L18; } } break; } }else{ affinity = 85 & SQLITE_AFF_MASK; if( affinity>=SQLITE_AFF_NUMERIC ){ if( (flags1 & (MEM_Int|MEM_Real|MEM_Str))==MEM_Str ){ applyNumericAffinity(pIn1,0); } if( (flags3 & (MEM_Int|MEM_Real|MEM_Str))==MEM_Str ){ applyNumericAffinity(pIn3,0); } }else if( affinity==SQLITE_AFF_TEXT ){ if( (flags1 & MEM_Str)==0 && (flags1 & (MEM_Int|MEM_Real))!=0 ){ testcase( pIn1->flags & MEM_Int ); testcase( pIn1->flags & MEM_Real ); sqlite3VdbeMemStringify(pIn1, encoding, 1); testcase( (flags1&MEM_Dyn) != (pIn1->flags&MEM_Dyn) ); flags1 = (pIn1->flags & ~MEM_TypeMask) | (flags1 & MEM_TypeMask); } if( (flags3 & MEM_Str)==0 && (flags3 & (MEM_Int|MEM_Real))!=0 ){ testcase( pIn3->flags & MEM_Int ); testcase( pIn3->flags & MEM_Real ); sqlite3VdbeMemStringify(pIn3, encoding, 1); testcase( (flags3&MEM_Dyn) != (pIn3->flags&MEM_Dyn) ); flags3 = (pIn3->flags & ~MEM_TypeMask) | (flags3 & MEM_TypeMask); } } assert( -4==P4_COLLSEQ || p->aOp[13].p4.pColl==0 ); if( flags1 & MEM_Zero ){ sqlite3VdbeMemExpandBlob(pIn1); flags1 &= ~MEM_Zero; } if( flags3 & MEM_Zero ){ sqlite3VdbeMemExpandBlob(pIn3); flags3 &= ~MEM_Zero; } res = sqlite3MemCompare(pIn3, pIn1, p->aOp[13].p4.pColl); } switch( 83 ){ case OP_Eq: res = res==0; break; case OP_Ne: res = res!=0; break; case OP_Lt: res = res<0; break; case OP_Le: res = res<=0; break; case OP_Gt: res = res>0; break; default: res = res>=0; break; } assert( (pIn1->flags & MEM_Dyn) == (flags1 & MEM_Dyn) ); pIn1->flags = flags1; assert( (pIn3->flags & MEM_Dyn) == (flags3 & MEM_Dyn) ); pIn3->flags = flags3; if( 85 & SQLITE_STOREP2 ){ pOut = &aMem[18]; memAboutToChange(p, pOut); MemSetTypeFlag(pOut, MEM_Int); pOut->u.i = res; REGISTER_TRACE(18, pOut); }else{ VdbeBranchTaken(res!=0, (85 & SQLITE_NULLEQ)?2:3); if( res ){ goto L18; } } } } while(0);
L14:
do {{ int rc = sqlite3VdbeGetColumn(p, &p->aOp[14]); int p1 = 0; int p2 = 5; int p3 = 13; if (rc != SQLITE_OK) { if (rc == SQLITE_TOOBIG) { goto too_big; } else if (rc == SQLITE_NOMEM_BKPT) { goto no_mem; } else { goto abort_due_to_error; } } } } while(0);
L15:
do {{ pIn1 = &aMem[13]; if( pIn1->flags & MEM_Int ){ sqlite3VdbeMemRealify(pIn1); } } } while(0);
L16:
do {{ char bIntint; u16 flags; u16 type1; u16 type2; i64 iA; i64 iB; double rA; double rB; pIn1 = &aMem[5]; type1 = numericType(pIn1); pIn2 = &aMem[13]; type2 = numericType(pIn2); pOut = &aMem[12]; flags = pIn1->flags | pIn2->flags; if( (flags & MEM_Null)!=0 ) goto arithmetic_result_is_null16; if( (type1 & type2 & MEM_Int)!=0 ){ iA = pIn1->u.i; iB = pIn2->u.i; bIntint = 1; switch( 91 ){ case OP_Add: if( sqlite3AddInt64(&iB,iA) ) goto fp16_math; break; case OP_Subtract: if( sqlite3SubInt64(&iB,iA) ) goto fp16_math; break; case OP_Multiply: if( sqlite3MulInt64(&iB,iA) ) goto fp16_math; break; case OP_Divide: { if( iA==0 ) goto arithmetic_result_is_null16; if( iA==-1 && iB==SMALLEST_INT64 ) goto fp16_math; iB /= iA; break; } default: { if( iA==0 ) goto arithmetic_result_is_null16; if( iA==-1 ) iA = 1; iB %= iA; break; } } pOut->u.i = iB; MemSetTypeFlag(pOut, MEM_Int); }else{ bIntint = 0; fp16_math: rA = sqlite3VdbeRealValue(pIn1); rB = sqlite3VdbeRealValue(pIn2); switch( 91 ){ case OP_Add: rB += rA; break; case OP_Subtract: rB -= rA; break; case OP_Multiply: rB *= rA; break; case OP_Divide: { if( rA==(double)0 ) goto arithmetic_result_is_null16; rB /= rA; break; } default: { iA = (i64)rA; iB = (i64)rB; if( iA==0 ) goto arithmetic_result_is_null16; if( iA==-1 ) iA = 1; rB = (double)(iB % iA); break; } } if( sqlite3IsNaN(rB) ){ goto arithmetic_result_is_null16; } pOut->u.r = rB; MemSetTypeFlag(pOut, MEM_Real); if( ((type1|type2)&MEM_Real)==0 && !bIntint ){ sqlite3VdbeIntegerAffinity(pOut); } } break; arithmetic_result_is_null16: sqlite3VdbeMemSetNull(pOut); } } while(0);
L17:
do {{ int i; sqlite3_context *pCtx; Mem *pMem; Mem t; Op *op = &p->aOp[17]; if ( op->p4type==P4_FUNCDEF ) { int n = 1; assert( 1>0 && 1<=(p->nMem-p->nCursor) ); assert( n==0 || (12>0 && 12+n<=(p->nMem-p->nCursor)+1) ); assert( 1<12 || 1>=12+n ); pCtx = sqlite3DbMallocRawNN(db, sizeof(*pCtx) + (n-1)*sizeof(sqlite3_value*)); if( pCtx==0 ) goto no_mem; pCtx->pMem = 0; pCtx->pFunc = op->p4.pFunc; pCtx->iOp = (int)17; pCtx->pVdbe = p; pCtx->argc = n; op->p4.pCtx = pCtx; op->p4type=P4_FUNCCTX; } else { assert( op->p4type==P4_FUNCCTX ); pCtx = op->p4.pCtx; } pMem = &aMem[1]; if( pCtx->pMem != pMem ){ pCtx->pMem = pMem; for(i=pCtx->argc-1; i>=0; i--) pCtx->argv[i] = &aMem[12+i]; } pMem->n++; sqlite3VdbeMemInit(&t, db, MEM_Null); pCtx->pOut = &t; pCtx->fErrorOrAux = 0; pCtx->skipFlag = 0; (pCtx->pFunc->xSFunc)(pCtx,pCtx->argc,pCtx->argv); if( pCtx->fErrorOrAux ){ if( pCtx->isError ){ sqlite3VdbeError(p, "%s", sqlite3_value_text(&t)); rc = pCtx->isError; } sqlite3VdbeMemRelease(&t); if( rc ) goto abort_due_to_error; }else{ assert( t.flags==MEM_Null ); } if( pCtx->skipFlag ){ assert(p->aOp[17-1].opcode==OP_CollSeq ); i = p->aOp[17-1].p1; if( i ) sqlite3VdbeMemSetInt64(&aMem[i], 1); } } } while(0);
L18:
do {{ VdbeCursor *pC; int res; if (7 == OP_SorterNext) { pC = p->apCsr[0]; assert( isSorter(pC) ); res = 0; rc = sqlite3VdbeSorterNext(db, pC, &res); goto next_tail18; } if (7 == OP_PrevIfOpen || 7 == OP_NextIfOpen) { if( p->apCsr[0]==0 ) break; } assert( 0>=0 && 0<p->nCursor ); assert( 1<ArraySize(p->aCounter) ); pC = p->apCsr[0]; res = 0; assert( pC!=0 ); assert( pC->deferredMoveto==0 ); assert( pC->eCurType==CURTYPE_BTREE ); assert( res==0 || (res==1 && pC->isTable==0) ); testcase( res==1 ); assert( 7!=OP_Next || p->aOp[18].p4.xAdvance==sqlite3BtreeNext ); assert( 7!=OP_Prev || p->aOp[18].p4.xAdvance==sqlite3BtreePrevious ); assert( 7!=OP_NextIfOpen || p->aOp[18].p4.xAdvance==sqlite3BtreeNext ); assert( 7!=OP_PrevIfOpen || p->aOp[18].p4.xAdvance==sqlite3BtreePrevious); assert( 7!=OP_Next || 7!=OP_NextIfOpen || pC->seekOp==OP_SeekGT || pC->seekOp==OP_SeekGE || pC->seekOp==OP_Rewind || pC->seekOp==OP_Found); assert( 7!=OP_Prev || 7!=OP_PrevIfOpen || pC->seekOp==OP_SeekLT || pC->seekOp==OP_SeekLE || pC->seekOp==OP_Last ); rc = p->aOp[18].p4.xAdvance(pC->uc.pCursor, &res); next_tail18: pC->cacheStatus = CACHE_STALE; VdbeBranchTaken(res==0,2); if( rc ) goto abort_due_to_error; if( res==0 ){ pC->nullRow = 0; p->aCounter[1]++; if( db->u1.isInterrupted ) goto abort_due_to_interrupt; goto L4; }else{ pC->nullRow = 1; } if( db->u1.isInterrupted ) goto abort_due_to_interrupt; } } while(0);
L19:
do {{ assert( 0>=0 && 0<p->nCursor ); sqlite3VdbeFreeCursor(p, p->apCsr[0]); p->apCsr[0] = 0; } } while(0);
L20:
do {{ Mem *pMem; assert( 1>0 && 1<=(p->nMem-p->nCursor) ); pMem = &aMem[1]; assert( (pMem->flags & ~(MEM_Null|MEM_Agg))==0 ); rc = sqlite3VdbeMemFinalize(pMem, p->aOp[20].p4.pFunc); if( rc ){ sqlite3VdbeError(p, "%s", sqlite3_value_text(pMem)); goto abort_due_to_error; } sqlite3VdbeChangeEncoding(pMem, encoding); UPDATE_MAX_BLOBSIZE(pMem); if( sqlite3VdbeMemTooBig(pMem) ){ goto too_big; } } } while(0);
L21:
do {{ int n; n = 0; pIn1 = &aMem[1]; pOut = &aMem[15]; assert( pOut!=pIn1 ); while( 1 ){ sqlite3VdbeMemShallowCopy(pOut, pIn1, MEM_Ephem); Deephemeralize(pOut); REGISTER_TRACE(15+0-n, pOut); if( (n--)==0 ) break; pOut++; pIn1++; } } } while(0);
L22:
do {{ Mem *pMem; int i; assert( p->nResColumn==1 ); assert( 15>0 ); assert( 15+1<=(p->nMem-p->nCursor)+1 ); if( SQLITE_OK!=(rc = sqlite3VdbeCheckFk(p, 0)) ){ assert( db->flags&SQLITE_CountRows ); assert( p->usesStmtJournal ); goto abort_due_to_error; } assert( p->iStatement==0 || db->flags&SQLITE_CountRows ); rc = sqlite3VdbeCloseStatement(p, SAVEPOINT_RELEASE); assert( rc==SQLITE_OK ); p->cacheCtr = (p->cacheCtr + 2)|1; pMem = p->pResultSet = &aMem[15]; for(i=0; i<1; i++){ assert( memIsValid(&pMem[i]) ); Deephemeralize(&pMem[i]); assert( (pMem[i].flags & MEM_Ephem)==0 || (pMem[i].flags & (MEM_Str|MEM_Blob))==0 ); sqlite3VdbeMemNulTerminate(&pMem[i]); REGISTER_TRACE(15+i, &pMem[i]); } if( db->mallocFailed ) goto no_mem; p->pc = (int)22 + 1; rc = SQLITE_ROW; goto vdbe_return; } } while(0);
L23:
do { if (21 == OP_Halt || (aMem[0].flags & MEM_Null)!=0 ) { const char *zType; const char *zLogFmt; VdbeFrame *pFrame; int pcx; pcx = (int)23; if( 0==SQLITE_OK && p->pFrame ){ pFrame = p->pFrame; p->pFrame = pFrame->pParent; p->nFrame--; sqlite3VdbeSetChanges(db, p->nChange); pcx = sqlite3VdbeFrameRestore(pFrame); lastRowid = db->lastRowid; if( 0==OE_Ignore ){ pcx = p->aOp[pcx].p2-1; } aMem = p->aMem; goto *labels[pcx+1]; } p->rc = 0; p->errorAction = (u8)0; p->pc = pcx; if( p->rc ){ if( 0 ){ static const char * const azType[] = { "NOT NULL", "UNIQUE", "CHECK", "FOREIGN KEY" }; assert( 0>=1 && 0<=4 ); testcase( 0==1 ); testcase( 0==2 ); testcase( 0==3 ); testcase( 0==4 ); zType = azType[0-1]; }else{ zType = 0; } assert( zType!=0 || 0!=0 ); zLogFmt = "abort at %d in [%s]: %s"; if( zType && 0 ){ sqlite3VdbeError(p, "%s constraint failed: %s", zType, 0); }else if( 0 ){ sqlite3VdbeError(p, "%s", 0); }else{ sqlite3VdbeError(p, "%s constraint failed", zType); } sqlite3_log(0, zLogFmt, pcx, p->zSql, p->zErrMsg); } rc = sqlite3VdbeHalt(p); assert( rc==SQLITE_BUSY || rc==SQLITE_OK || rc==SQLITE_ERROR ); if( rc==SQLITE_BUSY ){ p->rc = rc = SQLITE_BUSY; }else{ assert( rc==SQLITE_OK || (p->rc&0xff)==SQLITE_CONSTRAINT ); assert( rc==SQLITE_OK || db->nDeferredCons>0 || db->nDeferredImmCons>0 ); rc = p->rc ? SQLITE_ERROR : SQLITE_DONE; } goto vdbe_return; } } while(0);
L24:
do {{ Btree *pBt; int iMeta; int iGen; assert( p->bIsReader ); assert( p->readOnly==0 || 0==0 ); assert( 0>=0 && 0<db->nDb ); assert( DbMaskTest(p->btreeMask, 0) ); if( 0 && (db->flags & SQLITE_QueryOnly)!=0 ){ rc = SQLITE_READONLY; goto abort_due_to_error; } pBt = db->aDb[0].pBt; if( pBt ){ rc = sqlite3BtreeBeginTrans(pBt, 0); testcase( rc==SQLITE_BUSY_SNAPSHOT ); testcase( rc==SQLITE_BUSY_RECOVERY ); if( (rc&0xff)==SQLITE_BUSY ){ p->pc = (int)24; p->rc = rc; goto vdbe_return; } if( rc!=SQLITE_OK ){ goto abort_due_to_error; } if( 0 && p->usesStmtJournal && (db->autoCommit==0 || db->nVdbeRead>1) ){ assert( sqlite3BtreeIsInTrans(pBt) ); if( p->iStatement==0 ){ assert( db->nStatement>=0 && db->nSavepoint>=0 ); db->nStatement++; p->iStatement = db->nSavepoint + db->nStatement; } rc = sqlite3VtabSavepoint(db, SAVEPOINT_BEGIN, p->iStatement-1); if( rc==SQLITE_OK ){ rc = sqlite3BtreeBeginStmt(pBt, p->iStatement); } p->nStmtDefCons = db->nDeferredCons; p->nStmtDefImmCons = db->nDeferredImmCons; } sqlite3BtreeGetMeta(pBt, BTREE_SCHEMA_VERSION, (u32 *)&iMeta); iGen = db->aDb[0].pSchema->iGeneration; }else{ iGen = iMeta = 0; } assert( 1==0 || -14==P4_INT32 ); if( 1 && (iMeta!=1 || iGen!=0) ){ sqlite3DbFree(db, p->zErrMsg); p->zErrMsg = sqlite3DbStrDup(db, "database schema has changed"); if( db->aDb[0].pSchema->schema_cookie!=iMeta ){ sqlite3ResetOneSchema(db, 0); } p->expired = 1; rc = SQLITE_SCHEMA; } if( rc ) goto abort_due_to_error; } } while(0);
L25:
do {{ u8 isWriteLock = (u8)0; if( isWriteLock || 0==(db->flags&SQLITE_ReadUncommitted) ){ int p1 = 0; assert( p1>=0 && p1<db->nDb ); assert( DbMaskTest(p->btreeMask, p1) ); assert( isWriteLock==0 || isWriteLock==1 ); rc = sqlite3BtreeLockTable(db->aDb[p1].pBt, 2, isWriteLock); if( rc ){ if( (rc&0xFF)==SQLITE_LOCKED ){ const char *z = "lineitem"; sqlite3VdbeError(p, "database table is locked: %s", z); } goto abort_due_to_error; } } } } while(0);
L26:
do {{ assert( "1996-01-01"!=0 ); pOut = out2Prerelease(p, 6); pOut->flags = MEM_Str|MEM_Static|MEM_Term; pOut->z = "1996-01-01"; pOut->n = 10; pOut->enc = encoding; UPDATE_MAX_BLOBSIZE(pOut); if( 0 ){ assert( 0>0 ); assert( 0<=(p->nMem-p->nCursor) ); pIn3 = &aMem[0]; assert( pIn3->flags & MEM_Int ); if( pIn3->u.i ) pOut->flags = MEM_Blob|MEM_Static|MEM_Term; } } } while(0);
L27:
do {{ assert( "1997-01-01"!=0 ); pOut = out2Prerelease(p, 7); pOut->flags = MEM_Str|MEM_Static|MEM_Term; pOut->z = "1997-01-01"; pOut->n = 10; pOut->enc = encoding; UPDATE_MAX_BLOBSIZE(pOut); if( 0 ){ assert( 0>0 ); assert( 0<=(p->nMem-p->nCursor) ); pIn3 = &aMem[0]; assert( pIn3->flags & MEM_Int ); if( pIn3->u.i ) pOut->flags = MEM_Blob|MEM_Static|MEM_Term; } } } while(0);
L28:
do {{ pOut = out2Prerelease(p, 9); pOut->flags = MEM_Real; assert( !sqlite3IsNaN(8.000000e-02) ); pOut->u.r = 8.000000e-02; } } while(0);
L29:
do {{ pOut = out2Prerelease(p, 10); pOut->flags = MEM_Real; assert( !sqlite3IsNaN(1.000000e-01) ); pOut->u.r = 1.000000e-01; } } while(0);
L30:
do {{ pOut = out2Prerelease(p, 11); pOut->u.i = 24; } } while(0);
L31:
do {{ goto L1; } } while(0);
Lend: abort_due_to_error: if( db->mallocFailed ) rc = SQLITE_NOMEM_BKPT; assert( rc ); if( p->zErrMsg==0 && rc!=SQLITE_IOERR_NOMEM ){ sqlite3VdbeError(p, "%s", sqlite3ErrStr(rc)); } p->rc = rc; testcase( sqlite3GlobalConfig.xLog!=0 ); sqlite3_log(rc, "statement aborts: [%s] %s", p->zSql, p->zErrMsg); sqlite3VdbeHalt(p); if( rc==SQLITE_IOERR_NOMEM ) sqlite3OomFault(db); rc = SQLITE_ERROR; if( resetSchemaOnFault>0 ){ sqlite3ResetOneSchema(db, resetSchemaOnFault-1); } vdbe_return: db->lastRowid = lastRowid; testcase( nVmStep>0 ); p->aCounter[SQLITE_STMTSTATUS_VM_STEP] += (int)nVmStep; sqlite3VdbeLeave(p); assert( rc!=SQLITE_OK || nExtraDelete==0 || sqlite3_strlike("DELETE%",p->zSql,0)!=0 ); return rc; too_big: sqlite3VdbeError(p, "string or blob too big"); rc = SQLITE_TOOBIG; goto abort_due_to_error; no_mem: sqlite3OomFault(db); sqlite3VdbeError(p, "out of memory"); rc = SQLITE_NOMEM_BKPT; goto abort_due_to_error; abort_due_to_interrupt: assert( db->u1.isInterrupted ); rc = db->mallocFailed ? SQLITE_NOMEM_BKPT : SQLITE_INTERRUPT; p->rc = rc; sqlite3VdbeError(p, "%s", sqlite3ErrStr(rc)); goto abort_due_to_error; } 
