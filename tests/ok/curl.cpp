/*
local_settings:
    use_shared_libs: true
dependencies:
    pvt.cppan.demo.badger.curl.libcurl: 7
*/

#include <curl/curl.h>

int main()
{
    curl_version();
    return 0;
}
