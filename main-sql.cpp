#define UNICODE
#include <iostream>
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

#include "mssql.hpp"

using namespace g80::odbc;
auto main(int argc, const char *argv[]) -> int {

    odbc sqlca;

    if(!sqlca.connect_by_dsn(L"test", L"sa", L"Kerberos2014!x")) 
        std::wcout << sqlca.get_formatted_last_error();
    
    // if(!sqlca.disconnect())
    //     std::wcout << sqlca.get_formatted_last_error();

    //if(!sqlca.connect_by_file_dsn(L"D:\\Everett\\Codes\\Projects\\Personal\\Tools\\MSSQL-Connector\\db\\local.dsn", L"sa", L"Kerberos2014!")) {std::cout << "error5";}
    std::cout << "here!";
        
}