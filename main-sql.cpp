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

    if(!sqlca.alloc_null_env()) {std::cout << "error1";}
    if(!sqlca.set_env_attr()) {std::cout << "error2";}
    if(!sqlca.alloc_handle()) {std::cout << "error3";}
    if(!sqlca.connect(L"jgr", L"sa", L"Kerberos2014!")) {std::cout << "error4";}
    std::cout << "here!";
        
}