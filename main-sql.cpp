#define UNICODE
#include <iostream>
#include "mssql.hpp"

using namespace g80::odbc;
auto main(int argc, const char *argv[]) -> int {

    odbc sqlca;

    if(!sqlca.connect_by_dsn(L"test", L"sa", L"Kerberos2014!")) 
        std::wcout << sqlca.get_formatted_last_msg();
    std::wcout << sqlca.get_formatted_last_msg();

    wchar_t command[1024] {L"select * from users"};
    if(!sqlca.exec(command))
        std::wcout << sqlca.get_formatted_last_msg();

    // if(!sqlca.disconnect())
    //     std::wcout << sqlca.get_formatted_last_error();

    //if(!sqlca.connect_by_file_dsn(L"D:\\Everett\\Codes\\Projects\\Personal\\Tools\\MSSQL-Connector\\db\\local.dsn", L"sa", L"Kerberos2014!")) {std::cout << "error5";}
    std::cout << "here!";
        
}
