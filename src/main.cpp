#include <iostream>
#include "tagfilterdb/status.hpp"

int main()
{
    tagfilterdb::Status s = tagfilterdb::Status::NotFound("Where are you go");
    std::cout << s.ToString();
}
