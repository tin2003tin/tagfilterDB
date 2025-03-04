#include "db/index/r_tree.h"
#include "util/arena.h"
#include "util/random.h"
#include <iostream>

using namespace tagfilterdb;

std::string toString(DataView *data) { return data->ToString(); }

std::vector<std::pair<index::BoundingBox *, DataView *>> vec;

bool callback(index::BoundingBox *box, DataView *data) {
    vec.push_back({box, data});
    return true;
}

int main() {
    Arena arena;
    Cache *cache = nullptr;

    index::RTree::Option option;

    index::RTree rtree(&option, &arena, cache);

    DataView data("This is a data");

    size_t range = 1000;
    Random r(102);

    // for (int i = 0; i < 1000; i++) {
    //     double a = r.Uniform(range);
    //     double b = r.Uniform(range);
    //     double c = r.Uniform(range);
    //     double d = r.Uniform(range);
    //     index::BoundingBox box({{std::min(a, b), std::max(a, b)},
    //                             {std::min(c, d), std::max(c, d)}});
    //     rtree.Insert(box, &data);
    //     delete[] box.dims;
    // }

    for (int i = 0; i < 1000; i++) {
        index::BoundingBox box({{i, i * 2}, {i, i * 2}});

        rtree.Insert(box, &data);

        delete[] box.dims;
    }

    // Searching for a box that overlaps with box2
    index::BoundingBox targetBox({{500, 550}, {500, 550}});
    rtree.SearchUnder(targetBox, callback);

    for (auto &e : vec) {
        std::cout << "Items: " << index::BoundingBoxMgr::toString(*e.first, 2)
                  << " " << e.second->ToString() << std::endl;
    }
    std::cout << vec.size() << "\n";
    std::cout << "Memory Usage: " << arena.MemoryUsage() << std::endl;
}
