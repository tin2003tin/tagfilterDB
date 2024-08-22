#include <iostream>
#include "tagfilterdb/status.hpp"

int main()
{
    tagfilterdb::Status s = tagfilterdb::Status::NotFound("");
    // std::cout << s.ToString();
    std::cout
        << s.ok()
        << std::endl;
}
