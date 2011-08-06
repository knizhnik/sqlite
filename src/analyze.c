/*
** 2005 July 8
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** This file contains code associated with the ANALYZE command.
**
** The ANALYZE command gather statistics about the content of tables
** and indices.  These statistics are made available to the query planner
** to help it make better decisions about the best way to implement a
** query.
**
** Two system tables are created as follows:
**
**    CREATE TABLE sqlite_stat1(tbl, idx, stat);
**    CREATE TABLE sqlite_stat2(tbl, idx, sampleno, sample, cnt);
**
** Additional tables might be added in future releases of SQLite.
** The sqlite_stat2 table is only created and used if SQLite is
** compiled with SQLITE_ENABLE_STAT2.  Older versions of SQLite
** omit the sqlite_stat2.cnt column.  Newer versions of SQLite are
** able to use older versions of the stat2 table that lack the cnt
** column.
**
** Format of sqlite_stat1:
**
** There is normally one row per index, with the index identified by the
** name in the idx column.  The tbl column is the name of the table to
** which the index belongs.  In each such row, the stat column will be
** a string consisting of a list of integers.  The first integer in this
** list is the number of rows in the index and in the table.  The second
** integer is the average number of rows in the index that have the same
** value in the first column of the index.  The third integer is the average
** number of rows in the index that have the same value for the first two
** columns.  The N-th integer (for N>1) is the average number of rows in 
** the index which have the same value for the first N-1 columns.  For
** a K-column index, there will be K+1 integers in the stat column.  If
** the index is unique, then the last integer will be 1.
**
** The list of integers in the stat column can optionally be followed
** by the keyword "unordered".  The "unordered" keyword, if it is present,
** must be separated from the last integer by a single space.  If the
** "unordered" keyword is present, then the query planner assumes that
** the index is unordered and will not use the index for a range query.
** 
** If the sqlite_stat1.idx column is NULL, then the sqlite_stat1.stat
** column contains a single integer which is the (estimated) number of
** rows in the table identified by sqlite_stat1.tbl.
**
** Format of sqlite_stat2:
**
** The sqlite_stat2 is only created and is only used if SQLite is compiled
** with SQLITE_ENABLE_STAT2.  The "stat2" table contains additional information
** about the key distribution within an index.  The index is identified by
** the "idx" column and the "tbl" column is the name of the table to which
** the index belongs.  There are usually multiple rows in the sqlite_stat2
** table for each index.
**
** The sqlite_stat2 entires for an index that have sampleno>=0 are
** sampled key values for the first column of the index taken at
** intervals along the index.  The sqlite_stat2.sample column holds
** the value of the key in the left-most column of the index.
**
** The samples are numbered from 0 to S-1
** where S is 10 by default.  The number of samples created by the
** ANALYZE command can be adjusted at compile-time using the
** SQLITE_INDEX_SAMPLES macro.  The maximum number of samples is
** SQLITE_MAX_SAMPLES, currently set to 100.  There are places in the
** code that use an unsigned character to count samples, so an upper
** bound on SQLITE_MAX_SAMPLES is 255.
**
** Suppose the index contains C rows.  And let the number
** of samples be S.  SQLite assumes that the samples are taken from the
** following rows for i between 0 and S-1:
**
**     rownumber = (i*C*2 + C)/(S*2)
**
** Conceptually, the index is divided into S bins and the sample is
** taken from the middle of each bin.  The ANALYZE will not attempt
** to populate sqlite_stat2 for an index that holds fewer than S*2
** entries.
**
** If the key value for a sample (the sqlite_stat2.sample column) is a 
** large string or blob, SQLite will only use the first 255 bytes of 
** that string or blob.
** 
** The sqlite_stat2.cnt column contains the number of entries in the
** index for which sqlite_stat2.sample matches the left-most column
** of the index.  In other words, sqlite_stat2.cnt holds the number of
** times the sqlite_stat2.sample value appears in the index..  Many 
** older versions of SQLite omit the sqlite_stat2.cnt column.
**
** If the sqlite_stat2.sampleno value is -1, then that row holds a first-
** column key that is a frequently used key in the index.  The
** sqlite_stat2.cnt column will hold the number of occurrances of that key.
** This information is useful to the query planner in cases where a
** large percentage of the rows in indexed field have one of a small
** handful of value but the balance of the rows in the index have
** distinct or nearly distinct keys.
*/
#ifndef SQLITE_OMIT_ANALYZE
#include "sqliteInt.h"

/*
** This routine generates code that opens the sqlite_stat1 table for
** writing with cursor iStatCur. If the library was built with the
** SQLITE_ENABLE_STAT2 macro defined, then the sqlite_stat2 table is
** opened for writing using cursor (iStatCur+1)
**
** If the sqlite_stat1 tables does not previously exist, it is created.
** Similarly, if the sqlite_stat2 table does not exist and the library
** is compiled with SQLITE_ENABLE_STAT2 defined, it is created. 
**
** Argument zWhere may be a pointer to a buffer containing a table name,
** or it may be a NULL pointer. If it is not NULL, then all entries in
** the sqlite_stat1 and (if applicable) sqlite_stat2 tables associated
** with the named table are deleted. If zWhere==0, then code is generated
** to delete all stat table entries.
*/
static void openStatTable(
  Parse *pParse,          /* Parsing context */
  int iDb,                /* The database we are looking in */
  int iStatCur,           /* Open the sqlite_stat1 table on this cursor */
  const char *zWhere,     /* Delete entries for this table or index */
  const char *zWhereType  /* Either "tbl" or "idx" */
){
  static const struct {
    const char *zName;
    const char *zCols;
  } aTable[] = {
    { "sqlite_stat1", "tbl,idx,stat" },
#ifdef SQLITE_ENABLE_STAT2
    { "sqlite_stat2", "tbl,idx,sampleno,sample,cnt" },
#endif
  };

  int aRoot[] = {0, 0};
  u8 aCreateTbl[] = {0, 0};

  int i;
  sqlite3 *db = pParse->db;
  Db *pDb;
  Vdbe *v = sqlite3GetVdbe(pParse);
  if( v==0 ) return;
  assert( sqlite3BtreeHoldsAllMutexes(db) );
  assert( sqlite3VdbeDb(v)==db );
  pDb = &db->aDb[iDb];

  for(i=0; i<ArraySize(aTable); i++){
    const char *zTab = aTable[i].zName;
    Table *pStat;
    if( (pStat = sqlite3FindTable(db, zTab, pDb->zName))==0 ){
      /* The sqlite_stat[12] table does not exist. Create it. Note that a 
      ** side-effect of the CREATE TABLE statement is to leave the rootpage 
      ** of the new table in register pParse->regRoot. This is important 
      ** because the OpenWrite opcode below will be needing it. */
      sqlite3NestedParse(pParse,
          "CREATE TABLE %Q.%s(%s)", pDb->zName, zTab, aTable[i].zCols
      );
      aRoot[i] = pParse->regRoot;
      aCreateTbl[i] = 1;
    }else{
      /* The table already exists. If zWhere is not NULL, delete all entries 
      ** associated with the table zWhere. If zWhere is NULL, delete the
      ** entire contents of the table. */
      aRoot[i] = pStat->tnum;
      sqlite3TableLock(pParse, iDb, aRoot[i], 1, zTab);
      if( zWhere ){
        sqlite3NestedParse(pParse,
           "DELETE FROM %Q.%s WHERE %s=%Q", pDb->zName, zTab, zWhereType, zWhere
        );
      }else{
        /* The sqlite_stat[12] table already exists.  Delete all rows. */
        sqlite3VdbeAddOp2(v, OP_Clear, aRoot[i], iDb);
      }
    }
  }

  /* Open the sqlite_stat[12] tables for writing. */
  for(i=0; i<ArraySize(aTable); i++){
    sqlite3VdbeAddOp3(v, OP_OpenWrite, iStatCur+i, aRoot[i], iDb);
    sqlite3VdbeChangeP4(v, -1, (char *)3, P4_INT32);
    sqlite3VdbeChangeP5(v, aCreateTbl[i]);
    VdbeComment((v, "%s", aTable[i].zName));
  }
}

/*
** Generate code to do an analysis of all indices associated with
** a single table.
*/
static void analyzeOneTable(
  Parse *pParse,   /* Parser context */
  Table *pTab,     /* Table whose indices are to be analyzed */
  Index *pOnlyIdx, /* If not NULL, only analyze this one index */
  int iStatCur,    /* Index of VdbeCursor that writes the sqlite_stat1 table */
  int iMem         /* Available memory locations begin here */
){
  sqlite3 *db = pParse->db;    /* Database handle */
  Index *pIdx;                 /* An index to being analyzed */
  int iIdxCur;                 /* Cursor open on index being analyzed */
  Vdbe *v;                     /* The virtual machine being built up */
  int i;                       /* Loop counter */
  int topOfLoop;               /* The top of the loop */
  int endOfLoop;               /* The end of the loop */
  int jZeroRows = -1;          /* Jump from here if number of rows is zero */
  int iDb;                     /* Index of database containing pTab */
  int regTabname = iMem++;     /* Register containing table name */
  int regIdxname = iMem++;     /* Register containing index name */
  int regSampleno = iMem++;    /* Sampleno (stat2) or stat (stat1) */
#ifdef SQLITE_ENABLE_STAT2
  int regSample = iMem++;      /* The next sample value */
  int regSampleCnt = iMem++;   /* Number of occurrances of regSample value */
  int shortJump = 0;           /* Instruction address */
  int addrStoreStat2 = 0;      /* Address of subroutine to wrote to stat2 */
  int regNext = iMem++;        /* Index of next sample to record */
  int regSampleIdx = iMem++;   /* Index of next sample */
  int regReady = iMem++;       /* True if ready to store a stat2 entry */
  int regGosub = iMem++;       /* Register holding subroutine return addr */
  int regSample2 = iMem++;     /* Number of samples to acquire times 2 */
  int regCount = iMem++;       /* Number of rows in the table */
  int regCount2 = iMem++;      /* regCount*2 */
  int once = 1;                /* One-time initialization */
#endif
  int regCol = iMem++;         /* Content of a column in analyzed table */
  int regRec = iMem++;         /* Register holding completed record */
  int regTemp = iMem++;        /* Temporary use register */
  int regRowid = iMem++;       /* Rowid for the inserted record */


  v = sqlite3GetVdbe(pParse);
  if( v==0 || NEVER(pTab==0) ){
    return;
  }
  if( pTab->tnum==0 ){
    /* Do not gather statistics on views or virtual tables */
    return;
  }
  if( memcmp(pTab->zName, "sqlite_", 7)==0 ){
    /* Do not gather statistics on system tables */
    return;
  }
  assert( sqlite3BtreeHoldsAllMutexes(db) );
  iDb = sqlite3SchemaToIndex(db, pTab->pSchema);
  assert( iDb>=0 );
  assert( sqlite3SchemaMutexHeld(db, iDb, 0) );
#ifndef SQLITE_OMIT_AUTHORIZATION
  if( sqlite3AuthCheck(pParse, SQLITE_ANALYZE, pTab->zName, 0,
      db->aDb[iDb].zName ) ){
    return;
  }
#endif

  /* Establish a read-lock on the table at the shared-cache level. */
  sqlite3TableLock(pParse, iDb, pTab->tnum, 0, pTab->zName);

  iIdxCur = pParse->nTab++;
  sqlite3VdbeAddOp4(v, OP_String8, 0, regTabname, 0, pTab->zName, 0);
  for(pIdx=pTab->pIndex; pIdx; pIdx=pIdx->pNext){
    int nCol;
    KeyInfo *pKey;
    int addrIfNot;               /* address of OP_IfNot */
    int *aChngAddr;              /* Array of jump instruction addresses */

    if( pOnlyIdx && pOnlyIdx!=pIdx ) continue;
    VdbeNoopComment((v, "Begin analysis of %s", pIdx->zName));
    nCol = pIdx->nColumn;
    pKey = sqlite3IndexKeyinfo(pParse, pIdx);
    if( iMem+1+(nCol*2)>pParse->nMem ){
      pParse->nMem = iMem+1+(nCol*2);
    }
    aChngAddr = sqlite3DbMallocRaw(db, sizeof(int)*pIdx->nColumn);
    if( aChngAddr==0 ) continue;

    /* Open a cursor to the index to be analyzed. */
    assert( iDb==sqlite3SchemaToIndex(db, pIdx->pSchema) );
    sqlite3VdbeAddOp4(v, OP_OpenRead, iIdxCur, pIdx->tnum, iDb,
        (char *)pKey, P4_KEYINFO_HANDOFF);
    VdbeComment((v, "%s", pIdx->zName));

    /* Populate the register containing the index name. */
    sqlite3VdbeAddOp4(v, OP_String8, 0, regIdxname, 0, pIdx->zName, 0);

#ifdef SQLITE_ENABLE_STAT2

    /* If this iteration of the loop is generating code to analyze the
    ** first index in the pTab->pIndex list, then register regLast has
    ** not been populated. In this case populate it now.  */
    if( once ){
      once = 0;
      sqlite3VdbeAddOp2(v, OP_Integer, SQLITE_INDEX_SAMPLES*2, regSample2);

      sqlite3VdbeAddOp2(v, OP_Count, iIdxCur, regCount);
      sqlite3VdbeAddOp3(v, OP_Add, regCount, regCount, regCount2);


      /* Generate code for a subroutine that store the most recent sample
      ** in the sqlite_stat2 table
      */
      shortJump = sqlite3VdbeAddOp0(v, OP_Goto);
      sqlite3VdbeAddOp4(v, OP_MakeRecord, regTabname, 5, regRec, "aaaba", 0);
      VdbeComment((v, "begin stat2 write subroutine"));
      sqlite3VdbeAddOp2(v, OP_NewRowid, iStatCur+1, regRowid);
      sqlite3VdbeAddOp3(v, OP_Insert, iStatCur+1, regRec, regRowid);
      sqlite3VdbeAddOp2(v, OP_AddImm, regSampleno, 1);
      sqlite3VdbeAddOp2(v, OP_AddImm, regReady, -1);
      addrStoreStat2 = sqlite3VdbeAddOp2(v, OP_IfPos, regReady, shortJump+1);
      sqlite3VdbeAddOp1(v, OP_Return, regGosub);
      VdbeComment((v, "end stat2 write subroutine"));
      sqlite3VdbeJumpHere(v, shortJump);
    }
    /* Reset state registers */
    sqlite3VdbeAddOp2(v, OP_Copy, regCount2, regNext);
    shortJump = sqlite3VdbeAddOp3(v, OP_Lt, regSample2, 0, regCount);
    sqlite3VdbeAddOp3(v, OP_Divide, regSample2, regCount, regNext);
    sqlite3VdbeJumpHere(v, shortJump);
    sqlite3VdbeAddOp2(v, OP_Integer, 0, regSampleno);
    sqlite3VdbeAddOp2(v, OP_Integer, 0, regSampleIdx);
    sqlite3VdbeAddOp2(v, OP_Integer, 0, regReady);

#endif /* SQLITE_ENABLE_STAT2 */

    /* The block of memory cells initialized here is used as follows.
    **
    **    iMem:                
    **        The total number of rows in the table.
    **
    **    iMem+1 .. iMem+nCol: 
    **        Number of distinct entries in index considering the 
    **        left-most N columns only, where N is between 1 and nCol, 
    **        inclusive.
    **
    **    iMem+nCol+1 .. Mem+2*nCol:  
    **        Previous value of indexed columns, from left to right.
    **
    ** Cells iMem through iMem+nCol are initialized to 0. The others are 
    ** initialized to contain an SQL NULL.
    */
    for(i=0; i<=nCol; i++){
      sqlite3VdbeAddOp2(v, OP_Integer, 0, iMem+i);
    }
    for(i=0; i<nCol; i++){
      sqlite3VdbeAddOp2(v, OP_Null, 0, iMem+nCol+i+1);
    }

    /* Start the analysis loop. This loop runs through all the entries in
    ** the index b-tree.  */
    endOfLoop = sqlite3VdbeMakeLabel(v);
    sqlite3VdbeAddOp2(v, OP_Rewind, iIdxCur, endOfLoop);
    topOfLoop = sqlite3VdbeCurrentAddr(v);
    sqlite3VdbeAddOp2(v, OP_AddImm, iMem, 1);  /* Increment row counter */

    for(i=0; i<nCol; i++){
      CollSeq *pColl;
      sqlite3VdbeAddOp3(v, OP_Column, iIdxCur, i, regCol);
      if( i==0 ){
        /* Always record the very first row */
        addrIfNot = sqlite3VdbeAddOp1(v, OP_IfNot, iMem+1);
      }
      assert( pIdx->azColl!=0 );
      assert( pIdx->azColl[i]!=0 );
      pColl = sqlite3LocateCollSeq(pParse, pIdx->azColl[i]);
      aChngAddr[i] = sqlite3VdbeAddOp4(v, OP_Ne, regCol, 0, iMem+nCol+i+1,
                                      (char*)pColl, P4_COLLSEQ);
      sqlite3VdbeChangeP5(v, SQLITE_NULLEQ);
      VdbeComment((v, "jump if column %d changed", i));
#ifdef SQLITE_ENABLE_STAT2
      if( i==0 && addrStoreStat2 ){
        sqlite3VdbeAddOp2(v, OP_AddImm, regSampleCnt, 1);
        VdbeComment((v, "incr repeat count"));
      }
#endif
    }
    sqlite3VdbeAddOp2(v, OP_Goto, 0, endOfLoop);
    for(i=0; i<nCol; i++){
      sqlite3VdbeJumpHere(v, aChngAddr[i]);  /* Set jump dest for the OP_Ne */
      if( i==0 ){
        sqlite3VdbeJumpHere(v, addrIfNot);   /* Jump dest for OP_IfNot */
#ifdef SQLITE_ENABLE_STAT2
        sqlite3VdbeAddOp2(v, OP_Gosub, regGosub, addrStoreStat2);
        sqlite3VdbeAddOp2(v, OP_Integer, 1, regSampleCnt);
#endif        
      }
      sqlite3VdbeAddOp2(v, OP_AddImm, iMem+i+1, 1);
      sqlite3VdbeAddOp3(v, OP_Column, iIdxCur, i, iMem+nCol+i+1);
    }
    sqlite3DbFree(db, aChngAddr);

    /* Always jump here after updating the iMem+1...iMem+1+nCol counters */
    sqlite3VdbeResolveLabel(v, endOfLoop);

#ifdef SQLITE_ENABLE_STAT2
    /* Check if the record that cursor iIdxCur points to contains a
    ** value that should be stored in the sqlite_stat2 table. If so,
    ** store it. 
    */
    int ne = sqlite3VdbeAddOp3(v, OP_Le, regNext, 0, iMem);
    VdbeComment((v, "jump if not a sample"));
    shortJump = sqlite3VdbeAddOp1(v, OP_If, regReady);
    sqlite3VdbeAddOp2(v, OP_Copy, iMem+nCol+1, regSample);
    sqlite3VdbeJumpHere(v, shortJump);
    sqlite3VdbeAddOp2(v, OP_AddImm, regReady, 1);

    /* Calculate new values for regNextSample.  Where N is the number
    ** of rows in the table and S is the number of samples to take:
    **
    **   nextSample = (sampleNumber*N*2 + N)/(2*S)
    */
    sqlite3VdbeAddOp2(v, OP_AddImm, regSampleIdx, 1);
    sqlite3VdbeAddOp3(v, OP_Multiply, regSampleIdx, regCount2, regNext);
    sqlite3VdbeAddOp3(v, OP_Add, regNext, regCount, regNext);
    sqlite3VdbeAddOp3(v, OP_Divide, regSample2, regNext, regNext);
    sqlite3VdbeJumpHere(v, ne);
#endif

    sqlite3VdbeAddOp2(v, OP_Next, iIdxCur, topOfLoop);
    sqlite3VdbeAddOp1(v, OP_Close, iIdxCur);
#ifdef SQLITE_ENABLE_STAT2
    sqlite3VdbeAddOp2(v, OP_Gosub, regGosub, addrStoreStat2);
#endif        

    /* Store the results in sqlite_stat1.
    **
    ** The result is a single row of the sqlite_stat1 table.  The first
    ** two columns are the names of the table and index.  The third column
    ** is a string composed of a list of integer statistics about the
    ** index.  The first integer in the list is the total number of entries
    ** in the index.  There is one additional integer in the list for each
    ** column of the table.  This additional integer is a guess of how many
    ** rows of the table the index will select.  If D is the count of distinct
    ** values and K is the total number of rows, then the integer is computed
    ** as:
    **
    **        I = (K+D-1)/D
    **
    ** If K==0 then no entry is made into the sqlite_stat1 table.  
    ** If K>0 then it is always the case the D>0 so division by zero
    ** is never possible.
    */
    sqlite3VdbeAddOp2(v, OP_SCopy, iMem, regSampleno);
    if( jZeroRows<0 ){
      jZeroRows = sqlite3VdbeAddOp1(v, OP_IfNot, iMem);
    }
    for(i=0; i<nCol; i++){
      sqlite3VdbeAddOp4(v, OP_String8, 0, regTemp, 0, " ", 0);
      sqlite3VdbeAddOp3(v, OP_Concat, regTemp, regSampleno, regSampleno);
      sqlite3VdbeAddOp3(v, OP_Add, iMem, iMem+i+1, regTemp);
      sqlite3VdbeAddOp2(v, OP_AddImm, regTemp, -1);
      sqlite3VdbeAddOp3(v, OP_Divide, iMem+i+1, regTemp, regTemp);
      sqlite3VdbeAddOp1(v, OP_ToInt, regTemp);
      sqlite3VdbeAddOp3(v, OP_Concat, regTemp, regSampleno, regSampleno);
    }
    sqlite3VdbeAddOp4(v, OP_MakeRecord, regTabname, 3, regRec, "aaa", 0);
    sqlite3VdbeAddOp2(v, OP_NewRowid, iStatCur, regRowid);
    sqlite3VdbeAddOp3(v, OP_Insert, iStatCur, regRec, regRowid);
    sqlite3VdbeChangeP5(v, OPFLAG_APPEND);
  }

  /* If the table has no indices, create a single sqlite_stat1 entry
  ** containing NULL as the index name and the row count as the content.
  */
  if( pTab->pIndex==0 ){
    sqlite3VdbeAddOp3(v, OP_OpenRead, iIdxCur, pTab->tnum, iDb);
    VdbeComment((v, "%s", pTab->zName));
    sqlite3VdbeAddOp2(v, OP_Count, iIdxCur, regSampleno);
    sqlite3VdbeAddOp1(v, OP_Close, iIdxCur);
    jZeroRows = sqlite3VdbeAddOp1(v, OP_IfNot, regSampleno);
  }else{
    sqlite3VdbeJumpHere(v, jZeroRows);
    jZeroRows = sqlite3VdbeAddOp0(v, OP_Goto);
  }
  sqlite3VdbeAddOp2(v, OP_Null, 0, regIdxname);
  sqlite3VdbeAddOp4(v, OP_MakeRecord, regTabname, 3, regRec, "aaa", 0);
  sqlite3VdbeAddOp2(v, OP_NewRowid, iStatCur, regRowid);
  sqlite3VdbeAddOp3(v, OP_Insert, iStatCur, regRec, regRowid);
  sqlite3VdbeChangeP5(v, OPFLAG_APPEND);
  if( pParse->nMem<regRec ) pParse->nMem = regRec;
  sqlite3VdbeJumpHere(v, jZeroRows);
}

/*
** Generate code that will cause the most recent index analysis to
** be loaded into internal hash tables where is can be used.
*/
static void loadAnalysis(Parse *pParse, int iDb){
  Vdbe *v = sqlite3GetVdbe(pParse);
  if( v ){
    sqlite3VdbeAddOp1(v, OP_LoadAnalysis, iDb);
  }
}

/*
** Generate code that will do an analysis of an entire database
*/
static void analyzeDatabase(Parse *pParse, int iDb){
  sqlite3 *db = pParse->db;
  Schema *pSchema = db->aDb[iDb].pSchema;    /* Schema of database iDb */
  HashElem *k;
  int iStatCur;
  int iMem;

  sqlite3BeginWriteOperation(pParse, 0, iDb);
  iStatCur = pParse->nTab;
  pParse->nTab += 2;
  openStatTable(pParse, iDb, iStatCur, 0, 0);
  iMem = pParse->nMem+1;
  assert( sqlite3SchemaMutexHeld(db, iDb, 0) );
  for(k=sqliteHashFirst(&pSchema->tblHash); k; k=sqliteHashNext(k)){
    Table *pTab = (Table*)sqliteHashData(k);
    analyzeOneTable(pParse, pTab, 0, iStatCur, iMem);
  }
  loadAnalysis(pParse, iDb);
}

/*
** Generate code that will do an analysis of a single table in
** a database.  If pOnlyIdx is not NULL then it is a single index
** in pTab that should be analyzed.
*/
static void analyzeTable(Parse *pParse, Table *pTab, Index *pOnlyIdx){
  int iDb;
  int iStatCur;

  assert( pTab!=0 );
  assert( sqlite3BtreeHoldsAllMutexes(pParse->db) );
  iDb = sqlite3SchemaToIndex(pParse->db, pTab->pSchema);
  sqlite3BeginWriteOperation(pParse, 0, iDb);
  iStatCur = pParse->nTab;
  pParse->nTab += 2;
  if( pOnlyIdx ){
    openStatTable(pParse, iDb, iStatCur, pOnlyIdx->zName, "idx");
  }else{
    openStatTable(pParse, iDb, iStatCur, pTab->zName, "tbl");
  }
  analyzeOneTable(pParse, pTab, pOnlyIdx, iStatCur, pParse->nMem+1);
  loadAnalysis(pParse, iDb);
}

/*
** Generate code for the ANALYZE command.  The parser calls this routine
** when it recognizes an ANALYZE command.
**
**        ANALYZE                            -- 1
**        ANALYZE  <database>                -- 2
**        ANALYZE  ?<database>.?<tablename>  -- 3
**
** Form 1 causes all indices in all attached databases to be analyzed.
** Form 2 analyzes all indices the single database named.
** Form 3 analyzes all indices associated with the named table.
*/
void sqlite3Analyze(Parse *pParse, Token *pName1, Token *pName2){
  sqlite3 *db = pParse->db;
  int iDb;
  int i;
  char *z, *zDb;
  Table *pTab;
  Index *pIdx;
  Token *pTableName;

  /* Read the database schema. If an error occurs, leave an error message
  ** and code in pParse and return NULL. */
  assert( sqlite3BtreeHoldsAllMutexes(pParse->db) );
  if( SQLITE_OK!=sqlite3ReadSchema(pParse) ){
    return;
  }

  assert( pName2!=0 || pName1==0 );
  if( pName1==0 ){
    /* Form 1:  Analyze everything */
    for(i=0; i<db->nDb; i++){
      if( i==1 ) continue;  /* Do not analyze the TEMP database */
      analyzeDatabase(pParse, i);
    }
  }else if( pName2->n==0 ){
    /* Form 2:  Analyze the database or table named */
    iDb = sqlite3FindDb(db, pName1);
    if( iDb>=0 ){
      analyzeDatabase(pParse, iDb);
    }else{
      z = sqlite3NameFromToken(db, pName1);
      if( z ){
        if( (pIdx = sqlite3FindIndex(db, z, 0))!=0 ){
          analyzeTable(pParse, pIdx->pTable, pIdx);
        }else if( (pTab = sqlite3LocateTable(pParse, 0, z, 0))!=0 ){
          analyzeTable(pParse, pTab, 0);
        }
        sqlite3DbFree(db, z);
      }
    }
  }else{
    /* Form 3: Analyze the fully qualified table name */
    iDb = sqlite3TwoPartName(pParse, pName1, pName2, &pTableName);
    if( iDb>=0 ){
      zDb = db->aDb[iDb].zName;
      z = sqlite3NameFromToken(db, pTableName);
      if( z ){
        if( (pIdx = sqlite3FindIndex(db, z, zDb))!=0 ){
          analyzeTable(pParse, pIdx->pTable, pIdx);
        }else if( (pTab = sqlite3LocateTable(pParse, 0, z, zDb))!=0 ){
          analyzeTable(pParse, pTab, 0);
        }
        sqlite3DbFree(db, z);
      }
    }   
  }
}

/*
** Used to pass information from the analyzer reader through to the
** callback routine.
*/
typedef struct analysisInfo analysisInfo;
struct analysisInfo {
  sqlite3 *db;
  const char *zDatabase;
};

/*
** This callback is invoked once for each index when reading the
** sqlite_stat1 table.  
**
**     argv[0] = name of the table
**     argv[1] = name of the index (might be NULL)
**     argv[2] = results of analysis - on integer for each column
**
** Entries for which argv[1]==NULL simply record the number of rows in
** the table.
*/
static int analysisLoader(void *pData, int argc, char **argv, char **NotUsed){
  analysisInfo *pInfo = (analysisInfo*)pData;
  Index *pIndex;
  Table *pTable;
  int i, c, n;
  unsigned int v;
  const char *z;

  assert( argc==3 );
  UNUSED_PARAMETER2(NotUsed, argc);

  if( argv==0 || argv[0]==0 || argv[2]==0 ){
    return 0;
  }
  pTable = sqlite3FindTable(pInfo->db, argv[0], pInfo->zDatabase);
  if( pTable==0 ){
    return 0;
  }
  if( argv[1] ){
    pIndex = sqlite3FindIndex(pInfo->db, argv[1], pInfo->zDatabase);
  }else{
    pIndex = 0;
  }
  n = pIndex ? pIndex->nColumn : 0;
  z = argv[2];
  for(i=0; *z && i<=n; i++){
    v = 0;
    while( (c=z[0])>='0' && c<='9' ){
      v = v*10 + c - '0';
      z++;
    }
    if( i==0 ) pTable->nRowEst = v;
    if( pIndex==0 ) break;
    pIndex->aiRowEst[i] = v;
    if( *z==' ' ) z++;
    if( memcmp(z, "unordered", 10)==0 ){
      pIndex->bUnordered = 1;
      break;
    }
  }
  return 0;
}

#if SQLITE_ENABLE_STAT2
/*
** Delete an array of IndexSample objects
*/
static void deleteIndexSampleArray(
  sqlite3 *db,                 /* The database connection */
  IndexSampleArray *pArray     /* Array of IndexSample objects */
){
  int j;
  if( pArray->a==0 ) return;
  for(j=0; j<pArray->n; j++){
    IndexSample *p = &pArray->a[j];
    if( p->eType==SQLITE_TEXT || p->eType==SQLITE_BLOB ){
      sqlite3_free(p->u.z);
    }
  }
  sqlite3_free(pArray->a);
  memset(pArray, 0, sizeof(*pArray));
}
#endif

/*
** Delete the sample and common-key arrays from the index.
*/
void sqlite3DeleteIndexSamples(sqlite3 *db, Index *pIdx){
#ifdef SQLITE_ENABLE_STAT2
  deleteIndexSampleArray(db, &pIdx->sample);
  deleteIndexSampleArray(db, &pIdx->comkey);
#else
  UNUSED_PARAMETER(db);
  UNUSED_PARAMETER(pIdx);
#endif
}

#ifdef SQLITE_ENABLE_STAT2
/*
** Enlarge an array of IndexSample objects.
*/
static IndexSample *allocIndexSample(
  sqlite3 *db,              /* Database connection to malloc against */
  IndexSampleArray *pArray, /* The array to enlarge */
  int i                     /* Return this element */
){
  IndexSample *p;
  if( i>=pArray->nAlloc ){
    int szNew = i+1;
    p = (IndexSample*)sqlite3_realloc(pArray->a, szNew*sizeof(IndexSample));
    if( p==0 ) return 0;
    pArray->a = p;
    memset(&pArray->a[pArray->n], 0, (szNew-(pArray->n))*sizeof(IndexSample));
    pArray->nAlloc = szNew;
  }
  if( i>=pArray->n ) pArray->n = i+1;
  return &pArray->a[i];
}
#endif

/*
** Load the content of the sqlite_stat1 and sqlite_stat2 tables. The
** contents of sqlite_stat1 are used to populate the Index.aiRowEst[]
** arrays. The contents of sqlite_stat2 are used to populate the
** Index.sample and Index.comkey arrays.
**
** If the sqlite_stat1 table is not present in the database, SQLITE_ERROR
** is returned. In this case, even if SQLITE_ENABLE_STAT2 was defined 
** during compilation and the sqlite_stat2 table is present, no data is 
** read from it.
**
** If SQLITE_ENABLE_STAT2 was defined during compilation and the 
** sqlite_stat2 table is not present in the database, SQLITE_ERROR is
** returned. However, in this case, data is read from the sqlite_stat1
** table (if it is present) before returning.
**
** If an OOM error occurs, this function always sets db->mallocFailed.
** This means if the caller does not care about other errors, the return
** code may be ignored.
*/
int sqlite3AnalysisLoad(sqlite3 *db, int iDb){
  analysisInfo sInfo;
  HashElem *i;
  char *zSql;
  int rc;
  Table *pTab;    /* Stat1 or Stat2 table */

  assert( iDb>=0 && iDb<db->nDb );
  assert( db->aDb[iDb].pBt!=0 );

  /* Clear any prior statistics */
  assert( sqlite3SchemaMutexHeld(db, iDb, 0) );
  for(i=sqliteHashFirst(&db->aDb[iDb].pSchema->idxHash);i;i=sqliteHashNext(i)){
    Index *pIdx = sqliteHashData(i);
    sqlite3DefaultRowEst(pIdx);
    sqlite3DeleteIndexSamples(db, pIdx);
  }

  /* Check to make sure the sqlite_stat1 table exists */
  sInfo.db = db;
  sInfo.zDatabase = db->aDb[iDb].zName;
  if( (pTab=sqlite3FindTable(db, "sqlite_stat1", sInfo.zDatabase))==0 ){
    return SQLITE_ERROR;
  }

  /* Load new statistics out of the sqlite_stat1 table */
  zSql = sqlite3MPrintf(db, 
      "SELECT tbl, idx, stat FROM %Q.sqlite_stat1", sInfo.zDatabase);
  if( zSql==0 ){
    rc = SQLITE_NOMEM;
  }else{
    rc = sqlite3_exec(db, zSql, analysisLoader, &sInfo, 0);
    sqlite3DbFree(db, zSql);
  }


  /* Load the statistics from the sqlite_stat2 table. */
#ifdef SQLITE_ENABLE_STAT2
  if( rc==SQLITE_OK 
    && (pTab=sqlite3FindTable(db, "sqlite_stat2", sInfo.zDatabase))==0 ){
    rc = SQLITE_ERROR;
  }
  if( rc==SQLITE_OK ){
    sqlite3_stmt *pStmt = 0;

    zSql = sqlite3MPrintf(db, 
        "SELECT idx, sampleno, sample, %s FROM %Q.sqlite_stat2"
        " ORDER BY rowid DESC",
        pTab->nCol>=5 ? "cnt" : "0", sInfo.zDatabase);
    if( !zSql ){
      rc = SQLITE_NOMEM;
    }else{
      rc = sqlite3_prepare(db, zSql, -1, &pStmt, 0);
      sqlite3DbFree(db, zSql);
    }

    if( rc==SQLITE_OK ){
      while( sqlite3_step(pStmt)==SQLITE_ROW ){
        char *zIndex;   /* Index name */
        Index *pIdx;    /* Pointer to the index object */
        int iSample;
        int eType;
        IndexSample *pSample;

        zIndex = (char *)sqlite3_column_text(pStmt, 0);
        if( zIndex==0 ) continue;
        pIdx = sqlite3FindIndex(db, zIndex, sInfo.zDatabase);
        if( pIdx==0 ) continue;
        iSample = sqlite3_column_int(pStmt, 1);
        if( iSample>=SQLITE_MAX_SAMPLES ) continue;
        if( iSample<0 ){
          pSample = allocIndexSample(db, &pIdx->comkey, pIdx->comkey.n);
        }else{
          pSample = allocIndexSample(db, &pIdx->sample, iSample);
        }
        if( pSample==0 ) break;
        eType = sqlite3_column_type(pStmt, 2);
        pSample->eType = (u8)eType;
        pSample->nCopy = sqlite3_column_int(pStmt, 4);
        if( eType==SQLITE_INTEGER || eType==SQLITE_FLOAT ){
          pSample->u.r = sqlite3_column_double(pStmt, 2);
        }else if( eType==SQLITE_TEXT || eType==SQLITE_BLOB ){
          const char *z = (const char *)(
             (eType==SQLITE_BLOB) ?
              sqlite3_column_blob(pStmt, 2):
              sqlite3_column_text(pStmt, 2)
          );
          int n = sqlite3_column_bytes(pStmt, 2);
          if( n>255 ) n = 255;
          pSample->nByte = (u8)n;
          if( n < 1){
            pSample->u.z = 0;
          }else{
            pSample->u.z = sqlite3DbStrNDup(0, z, n);
            if( pSample->u.z==0 ){
              db->mallocFailed = 1;
              break;
            }
          }
        }
      }
      rc = sqlite3_finalize(pStmt);
    }
  }
#endif

  if( rc==SQLITE_NOMEM ){
    db->mallocFailed = 1;
  }
  return rc;
}


#endif /* SQLITE_OMIT_ANALYZE */
