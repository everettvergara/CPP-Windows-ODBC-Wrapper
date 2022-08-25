#pragma once

#include <sqltypes.h>

typedef struct STR_BINDING {
    SQLSMALLINT         cDisplaySize;           /* size to display  */
    WCHAR               *wszBuffer;             /* display buffer   */
    SQLLEN              indPtr;                 /* size or null     */
    BOOL                fChar;                  /* character col?   */
    struct STR_BINDING  *sNext;                 /* linked list      */
} BINDING;


void HandleDiagnosticRecord (SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE RetCode);
void DisplayResults(HSTMT hStmt, SQLSMALLINT cCols);
void AllocateBindings(HSTMT hStmt, SQLSMALLINT cCols, BINDING** ppBinding, SQLSMALLINT*  pDisplay);


namespace g80 {
    class mssql {

    private:
        SQLHENV hEnv = NULL;
        SQLHDBC hDbc = NULL;
        SQLHSTMT hStmt = NULL;
        WCHAR* pwszConnStr;
        WCHAR wszInput[SQL_QUERY_SIZE];


    };
}
