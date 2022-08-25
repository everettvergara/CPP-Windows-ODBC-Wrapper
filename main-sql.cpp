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

    if(!sqlca.alloc_env()) {std::cout << "error1";}
    if(!sqlca.alloc_handle()) {std::cout << "error3";}
    if(!sqlca.connect_by_dsn(L"test", L"sa", L"Kerberos2014!")) {std::cout << "error4";}
    if(!sqlca.connect_by_conn_string(L"test")) {std::cout << "error5";}
    std::cout << "here!";
        
}