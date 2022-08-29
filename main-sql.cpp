#define UNICODE
#include <iostream>
#include "odbc_trans.hpp"
#include <types/decimal.hpp>

using namespace g80::odbc;
// auto main(int argc, const char *argv[]) -> int {
auto main() -> int {


    decimal currency1(2);
    decimal currency2(2);

    currency2 += 123.45f;
    //currency2 += currency1;
    std::cout << currency2.get_as_ldouble() << "\n";

    return true;
    odbc_trans mssql;

    if(!mssql.connect_by_dsn(L"test", L"sa", L"Kerberos2014!")) 

        std::wcout << mssql.get_formatted_last_msg();
    // std::wcout << mssql.get_formatted_last_msg();

    wchar_t command[1024] {L"select * from users"};
    // wchar_t command[1024] {L"update a set a.name += 'x' from users as a where a.code = 'egv'"};
    mssql.exec(command);
    std::wcout << mssql.get_formatted_last_msg();

    if(!mssql.disconnect())
        std::wcout << mssql.get_formatted_last_msg();

    //if(!mssql.connect_by_file_dsn(L"D:\\Everett\\Codes\\Projects\\Personal\\Tools\\MSSQL-Connector\\db\\local.dsn", L"sa", L"Kerberos2014!")) {std::cout << "error5";}
    std::cout << "Clean exit!";
} 
