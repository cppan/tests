/*
local_settings:
    build:
        use_shared_libs: true
dependencies:
    pvt.cppan.demo.sqlite3: 3
*/

#include <sqlite3.h>

int main()
{
    sqlite3 *pFile;
    sqlite3_open(":memory:", &pFile);
    return 0;
}
