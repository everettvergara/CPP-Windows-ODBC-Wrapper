#define UNICODE
#include <iostream>
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

#include "mssql.hpp"

using namespace g80;
auto main(int argc, const char *argv[]) -> int {

    mssql sqlca;

    sqlca.alloc_null_env();
    sqlca.set_env_attr();
    sqlca.alloc_handle();
        
}