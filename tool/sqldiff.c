/*
** 2015-04-06
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
**
** This is a utility problem that computes the differences in content
** between two SQLite databases.
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include "sqlite3.h"

/*
** All global variables are gathered into the "g" singleton.
*/
struct GlobalVars {
  const char *zArgv0;       /* Name of program */
  int bSchemaOnly;          /* Only show schema differences */
  int bSchemaPK;            /* Use the schema-defined PK, not the true PK */
  unsigned fDebug;          /* Debug flags */
  sqlite3 *db;              /* The database connection */
} g;

/*
** Allowed values for g.fDebug
*/
#define DEBUG_COLUMN_NAMES  0x000001
#define DEBUG_DIFF_SQL      0x000002

/*
** Dynamic string object
*/
typedef struct Str Str;
struct Str {
  char *z;        /* Text of the string */
  int nAlloc;     /* Bytes allocated in z[] */
  int nUsed;      /* Bytes actually used in z[] */
};

/*
** Initialize a Str object
*/
static void strInit(Str *p){
  p->z = 0;
  p->nAlloc = 0;
  p->nUsed = 0;
}
  
/*
** Print an error resulting from faulting command-line arguments and
** abort the program.
*/
static void cmdlineError(const char *zFormat, ...){
  va_list ap;
  fprintf(stderr, "%s: ", g.zArgv0);
  va_start(ap, zFormat);
  vfprintf(stderr, zFormat, ap);
  va_end(ap);
  fprintf(stderr, "\n\"%s --help\" for more help\n", g.zArgv0);
  exit(1);
}

/*
** Print an error message for an error that occurs at runtime, then
** abort the program.
*/
static void runtimeError(const char *zFormat, ...){
  va_list ap;
  fprintf(stderr, "%s: ", g.zArgv0);
  va_start(ap, zFormat);
  vfprintf(stderr, zFormat, ap);
  va_end(ap);
  fprintf(stderr, "\n");
  exit(1);
}

/*
** Free all memory held by a Str object
*/
static void strFree(Str *p){
  sqlite3_free(p->z);
  strInit(p);
}

/*
** Add formatted text to the end of a Str object
*/
static void strPrintf(Str *p, const char *zFormat, ...){
  int nNew;
  for(;;){
    if( p->z ){
      va_list ap;
      va_start(ap, zFormat);
      sqlite3_vsnprintf(p->nAlloc-p->nUsed, p->z+p->nUsed, zFormat, ap);
      va_end(ap);
      nNew = (int)strlen(p->z + p->nUsed);
    }else{
      nNew = p->nAlloc;
    }
    if( p->nUsed+nNew < p->nAlloc-1 ){
      p->nUsed += nNew;
      break;
    }
    p->nAlloc = p->nAlloc*2 + 1000;
    p->z = sqlite3_realloc(p->z, p->nAlloc);
    if( p->z==0 ) runtimeError("out of memory");
  }
}



/* Safely quote an SQL identifier.  Use the minimum amount of transformation
** necessary to allow the string to be used with %s.
**
** Space to hold the returned string is obtained from sqlite3_malloc().  The
** caller is responsible for ensuring this space is freed when no longer
** needed.
*/
static char *safeId(const char *zId){
  /* All SQLite keywords, in alphabetical order */
  static const char *azKeywords[] = {
    "ABORT", "ACTION", "ADD", "AFTER", "ALL", "ALTER", "ANALYZE", "AND", "AS",
    "ASC", "ATTACH", "AUTOINCREMENT", "BEFORE", "BEGIN", "BETWEEN", "BY",
    "CASCADE", "CASE", "CAST", "CHECK", "COLLATE", "COLUMN", "COMMIT",
    "CONFLICT", "CONSTRAINT", "CREATE", "CROSS", "CURRENT_DATE",
    "CURRENT_TIME", "CURRENT_TIMESTAMP", "DATABASE", "DEFAULT", "DEFERRABLE",
    "DEFERRED", "DELETE", "DESC", "DETACH", "DISTINCT", "DROP", "EACH",
    "ELSE", "END", "ESCAPE", "EXCEPT", "EXCLUSIVE", "EXISTS", "EXPLAIN",
    "FAIL", "FOR", "FOREIGN", "FROM", "FULL", "GLOB", "GROUP", "HAVING", "IF",
    "IGNORE", "IMMEDIATE", "IN", "INDEX", "INDEXED", "INITIALLY", "INNER",
    "INSERT", "INSTEAD", "INTERSECT", "INTO", "IS", "ISNULL", "JOIN", "KEY",
    "LEFT", "LIKE", "LIMIT", "MATCH", "NATURAL", "NO", "NOT", "NOTNULL",
    "NULL", "OF", "OFFSET", "ON", "OR", "ORDER", "OUTER", "PLAN", "PRAGMA",
    "PRIMARY", "QUERY", "RAISE", "RECURSIVE", "REFERENCES", "REGEXP",
    "REINDEX", "RELEASE", "RENAME", "REPLACE", "RESTRICT", "RIGHT",
    "ROLLBACK", "ROW", "SAVEPOINT", "SELECT", "SET", "TABLE", "TEMP",
    "TEMPORARY", "THEN", "TO", "TRANSACTION", "TRIGGER", "UNION", "UNIQUE",
    "UPDATE", "USING", "VACUUM", "VALUES", "VIEW", "VIRTUAL", "WHEN", "WHERE",
    "WITH", "WITHOUT",
  };
  int lwr, upr, mid, c, i, x;
  for(i=x=0; (c = zId[i])!=0; i++){
    if( !isalpha(c) && c!='_' ){
      if( i>0 && isdigit(c) ){
        x++;
      }else{
        return sqlite3_mprintf("\"%w\"", zId);
      }
    }
  }
  if( x ) return sqlite3_mprintf("%s", zId);
  lwr = 0;
  upr = sizeof(azKeywords)/sizeof(azKeywords[0]) - 1;
  while( lwr<=upr ){
    mid = (lwr+upr)/2;
    c = sqlite3_stricmp(azKeywords[mid], zId);
    if( c==0 ) return sqlite3_mprintf("\"%w\"", zId);
    if( c<0 ){
      lwr = mid+1;
    }else{
      upr = mid-1;
    }
  }
  return sqlite3_mprintf("%s", zId);
}

/*
** Prepare a new SQL statement.  Print an error and abort if anything
** goes wrong.
*/
static sqlite3_stmt *db_vprepare(const char *zFormat, va_list ap){
  char *zSql;
  int rc;
  sqlite3_stmt *pStmt;

  zSql = sqlite3_vmprintf(zFormat, ap);
  if( zSql==0 ) runtimeError("out of memory");
  rc = sqlite3_prepare_v2(g.db, zSql, -1, &pStmt, 0);
  if( rc ){
    runtimeError("SQL statement error: %s\n\"%s\"", sqlite3_errmsg(g.db),
                 zSql);
  }
  sqlite3_free(zSql);
  return pStmt;
}
static sqlite3_stmt *db_prepare(const char *zFormat, ...){
  va_list ap;
  sqlite3_stmt *pStmt;
  va_start(ap, zFormat);
  pStmt = db_vprepare(zFormat, ap);
  va_end(ap);
  return pStmt;
}

/*
** Free a list of strings
*/
static void namelistFree(char **az){
  if( az ){
    int i;
    for(i=0; az[i]; i++) sqlite3_free(az[i]);
    sqlite3_free(az);
  }
}

/*
** Return a list of column names for the table zDb.zTab.  Space to
** hold the list is obtained from sqlite3_malloc() and should released
** using namelistFree() when no longer needed.
**
** Primary key columns are listed first, followed by data columns.
** The number of columns in the primary key is returned in *pnPkey.
**
** Normally, the "primary key" in the previous sentence is the true
** primary key - the rowid or INTEGER PRIMARY KEY for ordinary tables
** or the declared PRIMARY KEY for WITHOUT ROWID tables.  However, if
** the g.bSchemaPK flag is set, then the schema-defined PRIMARY KEY is
** used in all cases.  In that case, entries that have NULL values in
** any of their primary key fields will be excluded from the analysis.
**
** If the primary key for a table is the rowid but rowid is inaccessible,
** then this routine returns a NULL pointer.
**
** Examples:
**    CREATE TABLE t1(a INT UNIQUE, b INTEGER, c TEXT, PRIMARY KEY(c));
**    *pnPKey = 1;
**    az = { "rowid", "a", "b", "c", 0 }  // Normal case
**    az = { "c", "a", "b", 0 }           // g.bSchemaPK==1
**
**    CREATE TABLE t2(a INT UNIQUE, b INTEGER, c TEXT, PRIMARY KEY(b));
**    *pnPKey = 1;
**    az = { "b", "a", "c", 0 }
**
**    CREATE TABLE t3(x,y,z,PRIMARY KEY(y,z));
**    *pnPKey = 1                         // Normal case
**    az = { "rowid", "x", "y", "z", 0 }  // Normal case
**    *pnPKey = 2                         // g.bSchemaPK==1
**    az = { "y", "x", "z", 0 }           // g.bSchemaPK==1
**
**    CREATE TABLE t4(x,y,z,PRIMARY KEY(y,z)) WITHOUT ROWID;
**    *pnPKey = 2
**    az = { "y", "z", "x", 0 }
**
**    CREATE TABLE t5(rowid,_rowid_,oid);
**    az = 0     // The rowid is not accessible
*/
static char **columnNames(const char *zDb, const char *zTab, int *pnPKey){
  char **az = 0;           /* List of column names to be returned */
  int naz = 0;             /* Number of entries in az[] */
  sqlite3_stmt *pStmt;     /* SQL statement being run */
  char *zPkIdxName = 0;    /* Name of the PRIMARY KEY index */
  int truePk = 0;          /* PRAGMA table_info indentifies the PK to use */
  int nPK = 0;             /* Number of PRIMARY KEY columns */
  int i, j;                /* Loop counters */

  if( g.bSchemaPK==0 ){
    /* Normal case:  Figure out what the true primary key is for the table.
    **   *  For WITHOUT ROWID tables, the true primary key is the same as
    **      the schema PRIMARY KEY, which is guaranteed to be present.
    **   *  For rowid tables with an INTEGER PRIMARY KEY, the true primary
    **      key is the INTEGER PRIMARY KEY.
    **   *  For all other rowid tables, the rowid is the true primary key.
    */
    pStmt = db_prepare("PRAGMA %s.index_list=%Q", zDb, zTab);
    while( SQLITE_ROW==sqlite3_step(pStmt) ){
      if( sqlite3_stricmp((const char*)sqlite3_column_text(pStmt,3),"pk")==0 ){
        zPkIdxName = sqlite3_mprintf("%s", sqlite3_column_text(pStmt, 1));
        break;
      }
    }
    sqlite3_finalize(pStmt);
    if( zPkIdxName ){
      int nKey = 0;
      int nCol = 0;
      truePk = 0;
      pStmt = db_prepare("PRAGMA %s.index_xinfo=%Q", zDb, zPkIdxName);
      while( SQLITE_ROW==sqlite3_step(pStmt) ){
        nCol++;
        if( sqlite3_column_int(pStmt,5) ){ nKey++; continue; }
        if( sqlite3_column_int(pStmt,1)>=0 ) truePk = 1;
      }
      if( nCol==nKey ) truePk = 1;
      if( truePk ){
        nPK = nKey;
      }else{
        nPK = 1;
      }
      sqlite3_finalize(pStmt);
      sqlite3_free(zPkIdxName);
    }else{
      truePk = 1;
      nPK = 1;
    }
    pStmt = db_prepare("PRAGMA %s.table_info=%Q", zDb, zTab);
  }else{
    /* The g.bSchemaPK==1 case:  Use whatever primary key is declared
    ** in the schema.  The "rowid" will still be used as the primary key
    ** if the table definition does not contain a PRIMARY KEY.
    */
    nPK = 0;
    pStmt = db_prepare("PRAGMA %s.table_info=%Q", zDb, zTab);
    while( SQLITE_ROW==sqlite3_step(pStmt) ){
      if( sqlite3_column_int(pStmt,5)>0 ) nPK++;
    }
    sqlite3_reset(pStmt);
    if( nPK==0 ) nPK = 1;
    truePk = 1;
  }
  *pnPKey = nPK;
  naz = nPK;
  az = sqlite3_malloc( sizeof(char*)*(nPK+1) );
  if( az==0 ) runtimeError("out of memory");
  memset(az, 0, sizeof(char*)*(nPK+1));
  while( SQLITE_ROW==sqlite3_step(pStmt) ){
    int iPKey;
    if( truePk && (iPKey = sqlite3_column_int(pStmt,5))>0 ){
      az[iPKey-1] = safeId((char*)sqlite3_column_text(pStmt,1));
    }else{
      az = sqlite3_realloc(az, sizeof(char*)*(naz+2) );
      if( az==0 ) runtimeError("out of memory");
      az[naz++] = safeId((char*)sqlite3_column_text(pStmt,1));
    }
  }
  sqlite3_finalize(pStmt);
  if( az ) az[naz] = 0;
  if( az[0]==0 ){
    const char *azRowid[] = { "rowid", "_rowid_", "oid" };
    for(i=0; i<sizeof(azRowid)/sizeof(azRowid[0]); i++){
      for(j=1; j<naz; j++){
        if( sqlite3_stricmp(az[j], azRowid[i])==0 ) break;
      }
      if( j>=naz ){
        az[0] = sqlite3_mprintf("%s", azRowid[i]);
        break;
      }
    }
    if( az[0]==0 ){
      for(i=1; i<naz; i++) sqlite3_free(az[i]);
      sqlite3_free(az);
      az = 0;
    }
  }
  return az;
}

/*
** Print the sqlite3_value X as an SQL literal.
*/
static void printQuoted(sqlite3_value *X){
  switch( sqlite3_value_type(X) ){
    case SQLITE_FLOAT: {
      double r1;
      char zBuf[50];
      r1 = sqlite3_value_double(X);
      sqlite3_snprintf(sizeof(zBuf), zBuf, "%!.15g", r1);
      printf("%s", zBuf);
      break;
    }
    case SQLITE_INTEGER: {
      printf("%lld", sqlite3_value_int64(X));
      break;
    }
    case SQLITE_BLOB: {
      const unsigned char *zBlob = sqlite3_value_blob(X);
      int nBlob = sqlite3_value_bytes(X);
      if( zBlob ){
        int i;
        printf("x'");
        for(i=0; i<nBlob; i++){
          printf("%02x", zBlob[i]);
        }
        printf("'");
      }else{
        printf("NULL");
      }
      break;
    }
    case SQLITE_TEXT: {
      const unsigned char *zArg = sqlite3_value_text(X);
      int i, j;

      if( zArg==0 ){
        printf("NULL");
      }else{
        printf("'");
        for(i=j=0; zArg[i]; i++){
          if( zArg[i]=='\'' ){
            printf("%.*s'", i-j+1, &zArg[j]);
            j = i+1;
          }
        }
        printf("%s'", &zArg[j]);
      }
      break;
    }
    case SQLITE_NULL: {
      printf("NULL");
      break;
    }
  }
}

/*
** Output SQL that will recreate the aux.zTab table.
*/
static void dump_table(const char *zTab){
  char *zId = safeId(zTab); /* Name of the table */
  char **az = 0;            /* List of columns */
  int nPk;                  /* Number of true primary key columns */
  int nCol;                 /* Number of data columns */
  int i;                    /* Loop counter */
  sqlite3_stmt *pStmt;      /* SQL statement */
  const char *zSep;         /* Separator string */
  Str ins;                  /* Beginning of the INSERT statement */

  pStmt = db_prepare("SELECT sql FROM aux.sqlite_master WHERE name=%Q", zTab);
  if( SQLITE_ROW==sqlite3_step(pStmt) ){
    printf("%s;\n", sqlite3_column_text(pStmt,0));
  }
  sqlite3_finalize(pStmt);
  if( !g.bSchemaOnly ){
    az = columnNames("aux", zTab, &nPk);
    strInit(&ins);
    if( az==0 ){
      pStmt = db_prepare("SELECT * FROM aux.%s", zId);
      strPrintf(&ins,"INSERT INTO %s VALUES", zId);
    }else{
      Str sql;
      strInit(&sql);
      zSep =  "SELECT";
      for(i=0; az[i]; i++){
        strPrintf(&sql, "%s %s", zSep, az[i]);
        zSep = ",";
      }
      strPrintf(&sql," FROM aux.%s", zId);
      zSep = " ORDER BY";
      for(i=1; i<=nPk; i++){
        strPrintf(&sql, "%s %d", zSep, i);
        zSep = ",";
      }
      pStmt = db_prepare("%s", sql.z);
      strFree(&sql);
      strPrintf(&ins, "INSERT INTO %s", zId);
      zSep = "(";
      for(i=0; az[i]; i++){
        strPrintf(&ins, "%s%s", zSep, az[i]);
        zSep = ",";
      }
      strPrintf(&ins,") VALUES");
      namelistFree(az);
    }
    nCol = sqlite3_column_count(pStmt);
    while( SQLITE_ROW==sqlite3_step(pStmt) ){
      printf("%s",ins.z);
      zSep = "(";
      for(i=0; i<nCol; i++){
        printf("%s",zSep);
        printQuoted(sqlite3_column_value(pStmt,i));
        zSep = ",";
      }
      printf(");\n");
    }
    sqlite3_finalize(pStmt);
    strFree(&ins);
  } /* endif !g.bSchemaOnly */
  pStmt = db_prepare("SELECT sql FROM aux.sqlite_master"
                     " WHERE type='index' AND tbl_name=%Q AND sql IS NOT NULL",
                     zTab);
  while( SQLITE_ROW==sqlite3_step(pStmt) ){
    printf("%s;\n", sqlite3_column_text(pStmt,0));
  }
  sqlite3_finalize(pStmt);
}


/*
** Compute all differences for a single table.
*/
static void diff_one_table(const char *zTab){
  char *zId = safeId(zTab); /* Name of table (translated for us in SQL) */
  char **az = 0;            /* Columns in main */
  char **az2 = 0;           /* Columns in aux */
  int nPk;                  /* Primary key columns in main */
  int nPk2;                 /* Primary key columns in aux */
  int n;                    /* Number of columns in main */
  int n2;                   /* Number of columns in aux */
  int nQ;                   /* Number of output columns in the diff query */
  int i;                    /* Loop counter */
  const char *zSep;         /* Separator string */
  Str sql;                  /* Comparison query */
  sqlite3_stmt *pStmt;      /* Query statement to do the diff */

  strInit(&sql);
  if( g.fDebug==DEBUG_COLUMN_NAMES ){
    /* Simply run columnNames() on all tables of the origin
    ** database and show the results.  This is used for testing
    ** and debugging of the columnNames() function.
    */
    az = columnNames("aux",zTab, &nPk);
    if( az==0 ){
      printf("Rowid not accessible for %s\n", zId);
    }else{
      printf("%s:", zId);
      for(i=0; az[i]; i++){
        printf(" %s", az[i]);
        if( i+1==nPk ) printf(" *");
      }
      printf("\n");
    }
    goto end_diff_one_table;
  }
    

  if( sqlite3_table_column_metadata(g.db,"aux",zTab,0,0,0,0,0,0) ){
    if( !sqlite3_table_column_metadata(g.db,"main",zTab,0,0,0,0,0,0) ){
      /* Table missing from second database. */
      printf("DROP TABLE %s;\n", zId);
    }
    goto end_diff_one_table;
  }

  if( sqlite3_table_column_metadata(g.db,"main",zTab,0,0,0,0,0,0) ){
    /* Table missing from source */
    dump_table(zTab);
    goto end_diff_one_table;
  }

  az = columnNames("main", zTab, &nPk);
  az2 = columnNames("aux", zTab, &nPk2);
  if( az && az2 ){
    for(n=0; az[n]; n++){
      if( sqlite3_stricmp(az[n],az2[n])!=0 ) break;
    }
  }
  if( az==0
   || az2==0
   || nPk!=nPk2
   || az[n]
  ){
    /* Schema mismatch */
    printf("DROP TABLE %s;\n", zId);
    dump_table(zTab);
    goto end_diff_one_table;
  }

  /* Build the comparison query */
  for(n2=n; az[n2]; n2++){}
  nQ = nPk2+1+2*(n2-nPk2);
  if( n2>nPk2 ){
    zSep = "SELECT ";
    for(i=0; i<nPk; i++){
      strPrintf(&sql, "%sB.%s", zSep, az[i]);
      zSep = ", ";
    }
    strPrintf(&sql, ", 1%s -- changed row\n", nPk==n ? "" : ",");
    while( az[i] ){
      strPrintf(&sql, "       A.%s IS NOT B.%s, B.%s%s\n",
                az[i], az[i], az[i], i==n2-1 ? "" : ",");
      i++;
    }
    strPrintf(&sql, "  FROM main.%s A, aux.%s B\n", zId, zId);
    zSep = " WHERE";
    for(i=0; i<nPk; i++){
      strPrintf(&sql, "%s A.%s=B.%s", zSep, az[i], az[i]);
      zSep = " AND";
    }
    zSep = "\n   AND (";
    while( az[i] ){
      strPrintf(&sql, "%sA.%s IS NOT B.%s%s\n",
                zSep, az[i], az[i], i==n2-1 ? ")" : "");
      zSep = "        OR ";
      i++;
    }
    strPrintf(&sql, " UNION ALL\n");
  }
  zSep = "SELECT ";
  for(i=0; i<nPk; i++){
    strPrintf(&sql, "%sA.%s", zSep, az[i]);
    zSep = ", ";
  }
  strPrintf(&sql, ", 2%s -- deleted row\n", nPk==n ? "" : ",");
  while( az[i] ){
    strPrintf(&sql, "       NULL, NULL%s\n", i==n2-1 ? "" : ",");
    i++;
  }
  strPrintf(&sql, "  FROM main.%s A\n", zId);
  strPrintf(&sql, " WHERE NOT EXISTS(SELECT 1 FROM aux.%s B\n", zId);
  zSep =          "                   WHERE";
  for(i=0; i<nPk; i++){
    strPrintf(&sql, "%s A.%s=B.%s", zSep, az[i], az[i]);
    zSep = " AND";
  }
  strPrintf(&sql, ")\n");
  zSep = " UNION ALL\nSELECT ";
  for(i=0; i<nPk; i++){
    strPrintf(&sql, "%sB.%s", zSep, az[i]);
    zSep = ", ";
  }
  strPrintf(&sql, ", 3%s -- inserted row\n", nPk==n ? "" : ",");
  while( az2[i] ){
    strPrintf(&sql, "       1, B.%s%s\n", az[i], i==n2-1 ? "" : ",");
    i++;
  }
  strPrintf(&sql, "  FROM aux.%s B\n", zId);
  strPrintf(&sql, " WHERE NOT EXISTS(SELECT 1 FROM main.%s A\n", zId);
  zSep =          "                   WHERE";
  for(i=0; i<nPk; i++){
    strPrintf(&sql, "%s A.%s=B.%s", zSep, az[i], az[i]);
    zSep = " AND";
  }
  strPrintf(&sql, ")\n ORDER BY");
  zSep = " ";
  for(i=1; i<=nPk; i++){
    strPrintf(&sql, "%s%d", zSep, i);
    zSep = ", ";
  }
  strPrintf(&sql, ";\n");

  if( g.fDebug & DEBUG_DIFF_SQL ){ 
    printf("SQL for %s:\n%s\n", zId, sql.z);
    goto end_diff_one_table;
  }

  /* Drop indexes that are missing in the destination */
  pStmt = db_prepare(
    "SELECT name FROM main.sqlite_master"
    " WHERE type='index' AND tbl_name=%Q"
    "   AND sql IS NOT NULL"
    "   AND sql NOT IN (SELECT sql FROM aux.sqlite_master"
    "                    WHERE type='index' AND tbl_name=%Q"
    "                      AND sql IS NOT NULL)",
    zTab, zTab);
  while( SQLITE_ROW==sqlite3_step(pStmt) ){
    char *z = safeId((const char*)sqlite3_column_text(pStmt,0));
    printf("DROP INDEX %s;\n", z);
    sqlite3_free(z);
  }
  sqlite3_finalize(pStmt);

  /* Run the query and output differences */
  if( !g.bSchemaOnly ){
    pStmt = db_prepare(sql.z);
    while( SQLITE_ROW==sqlite3_step(pStmt) ){
      int iType = sqlite3_column_int(pStmt, nPk);
      if( iType==1 || iType==2 ){
        if( iType==1 ){       /* Change the content of a row */
          printf("UPDATE %s", zId);
          zSep = " SET";
          for(i=nPk+1; i<nQ; i+=2){
            if( sqlite3_column_int(pStmt,i)==0 ) continue;
            printf("%s %s=", zSep, az2[(i+nPk-1)/2]);
            zSep = ",";
            printQuoted(sqlite3_column_value(pStmt,i+1));
          }
        }else{                /* Delete a row */
          printf("DELETE FROM %s", zId);
        }
        zSep = " WHERE";
        for(i=0; i<nPk; i++){
          printf("%s %s=", zSep, az2[i]);
          printQuoted(sqlite3_column_value(pStmt,i));
          zSep = ",";
        }
        printf(";\n");
      }else{                  /* Insert a row */
        printf("INSERT INTO %s(%s", zId, az2[0]);
        for(i=1; az2[i]; i++) printf(",%s", az2[i]);
        printf(") VALUES");
        zSep = "(";
        for(i=0; i<nPk2; i++){
          printf("%s", zSep);
          zSep = ",";
          printQuoted(sqlite3_column_value(pStmt,i));
        }
        for(i=nPk2+2; i<nQ; i+=2){
          printf(",");
          printQuoted(sqlite3_column_value(pStmt,i));
        }
        printf(");\n");
      }
    }
    sqlite3_finalize(pStmt);
  } /* endif !g.bSchemaOnly */

  /* Create indexes that are missing in the source */
  pStmt = db_prepare(
    "SELECT sql FROM aux.sqlite_master"
    " WHERE type='index' AND tbl_name=%Q"
    "   AND sql IS NOT NULL"
    "   AND sql NOT IN (SELECT sql FROM main.sqlite_master"
    "                    WHERE type='index' AND tbl_name=%Q"
    "                      AND sql IS NOT NULL)",
    zTab, zTab);
  while( SQLITE_ROW==sqlite3_step(pStmt) ){
    printf("%s;\n", sqlite3_column_text(pStmt,0));
  }
  sqlite3_finalize(pStmt);

end_diff_one_table:
  strFree(&sql);
  sqlite3_free(zId);
  namelistFree(az);
  namelistFree(az2);
  return;
}

/*
** Print sketchy documentation for this utility program
*/
static void showHelp(void){
  printf("Usage: %s [options] DB1 DB2\n", g.zArgv0);
  printf(
"Output SQL text that would transform DB1 into DB2.\n"
"Options:\n"
"  --primarykey          Use schema-defined PRIMARY KEYs\n"
"  --schema              Show only differences in the schema\n"
"  --table TAB           Show only differences in table TAB\n"
  );
}

int main(int argc, char **argv){
  const char *zDb1 = 0;
  const char *zDb2 = 0;
  int i;
  int rc;
  char *zErrMsg = 0;
  char *zSql;
  sqlite3_stmt *pStmt;
  char *zTab = 0;

  g.zArgv0 = argv[0];
  for(i=1; i<argc; i++){
    const char *z = argv[i];
    if( z[0]=='-' ){
      z++;
      if( z[0]=='-' ) z++;
      if( strcmp(z,"debug")==0 ){
        g.fDebug = strtol(argv[++i], 0, 0);
      }else
      if( strcmp(z,"help")==0 ){
        showHelp();
        return 0;
      }else
      if( strcmp(z,"primarykey")==0 ){
        g.bSchemaPK = 1;
      }else
      if( strcmp(z,"schema")==0 ){
        g.bSchemaOnly = 1;
      }else
      if( strcmp(z,"table")==0 ){
        zTab = argv[++i];
      }else
      {
        cmdlineError("unknown option: %s", argv[i]);
      }
    }else if( zDb1==0 ){
      zDb1 = argv[i];
    }else if( zDb2==0 ){
      zDb2 = argv[i];
    }else{
      cmdlineError("unknown argument: %s", argv[i]);
    }
  }
  if( zDb2==0 ){
    cmdlineError("two database arguments required");
  }
  rc = sqlite3_open(zDb1, &g.db);
  if( rc ){
    cmdlineError("cannot open database file \"%s\"", zDb1);
  }
  rc = sqlite3_exec(g.db, "SELECT * FROM sqlite_master", 0, 0, &zErrMsg);
  if( rc || zErrMsg ){
    cmdlineError("\"%s\" does not appear to be a valid SQLite database", zDb1);
  }
  zSql = sqlite3_mprintf("ATTACH %Q as aux;", zDb2);
  rc = sqlite3_exec(g.db, zSql, 0, 0, &zErrMsg);
  if( rc || zErrMsg ){
    cmdlineError("cannot attach database \"%s\"", zDb2);
  }
  rc = sqlite3_exec(g.db, "SELECT * FROM aux.sqlite_master", 0, 0, &zErrMsg);
  if( rc || zErrMsg ){
    cmdlineError("\"%s\" does not appear to be a valid SQLite database", zDb2);
  }

  if( zTab ){
    diff_one_table(zTab);
  }else{
    /* Handle tables one by one */
    pStmt = db_prepare(
      "SELECT name FROM main.sqlite_master\n"
      " WHERE type='table' AND sql NOT LIKE 'CREATE VIRTUAL%%'\n"
      " UNION\n"
      "SELECT name FROM aux.sqlite_master\n"
      " WHERE type='table' AND sql NOT LIKE 'CREATE VIRTUAL%%'\n"
      " ORDER BY name"
    );
    while( SQLITE_ROW==sqlite3_step(pStmt) ){
      diff_one_table((const char*)sqlite3_column_text(pStmt, 0));
    }
    sqlite3_finalize(pStmt);
  }

  /* TBD: Handle trigger differences */
  /* TBD: Handle view differences */
  sqlite3_close(g.db);
  return 0;
}
