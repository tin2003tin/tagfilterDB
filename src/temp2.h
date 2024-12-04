/**
 * @file spatialIndex.h
 * @brief N-dimensional RTree implementation in C++
 * 
 * This implementation is inspired by the N-dimensional RTree implementation from
 * the `nushoin/RTree` repository. The original implementation can be found at:
 * https://github.com/nushoin/RTree
 * 
 * Credit: RTree implementation by nushoin.
 * 
 * @note This code is based on the original work in the `nushoin/RTree` repository.
 * 
 * @license MIT License
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * provided to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef TAGFILTERDB_SPATIAL_INDEX_H
#define TAGFILTERDB_SPATIAL_INDEX_H

#include "broundingbox.h"
#include "arena.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <limits>
#include <stddef.h>

namespace tagfilterdb {

struct SpatialIndexOptions {
    size_t DIMANSION = 2;
    size_t MAX_CHILD = 4; 
    size_t MIN_CHILD = MAX_CHILD / 2;
};

class SpatialIndex {
    protected:
    struct Node;
    struct SubNode;
    struct Group;
    struct GroupAssign;
};




}; // namespace tagfilterdb


#endif

