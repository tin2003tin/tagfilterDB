#include "tagfilterdb/logging.hpp"
#include "tagfilterdb/support/rangeSet.hpp"

using namespace tagfilterdb::support;

int main() {
    RangeSet r;
    r.add(1, 3);
    r.add(2, 10);
    r.add(15, 20);
    r.add(100);
    LOG_DEBUG("r: ", r.toString())
    RangeSet temp;
    temp.add(1, 5);
    temp.add(10);
    temp.add(18, 25);
    LOG_DEBUG("temp: ", temp.toString())
    LOG_DEBUG((r.And(temp)).toString())
    LOG_DEBUG(r.contains((size_t)10))
    LOG_DEBUG(r.contains((size_t)200))
    LOG_DEBUG(r.empty());
    LOG_DEBUG(r.getMaxElement(), " ", r.getMinElement())
    r.remove((size_t)5);
    LOG_DEBUG(r.toString())
    LOG_DEBUG((r.Or(temp)).toString())
    // compliment dont work
    // subtract dont work
}