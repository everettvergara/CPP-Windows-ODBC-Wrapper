#define UNICODE
#include <iostream>
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

#include "mssql.hpp"

using namespace g80;
auto main(int argc, const char *argv[]) -> int {

    mssql sq;

    sq.alloc_handle();
    sq.set_env_attr();
}