#ifndef TAGFILTERDB_FILTER_POLICY_H
#define TAGFILTERDB_FILTER_POLICY_H

#include <string>
#include "dataView.h"

namespace tagfilterdb {
    class FilterPolicy {
        public:
        virtual ~FilterPolicy() = default;

        virtual std::string Name() const = 0;

        virtual void CreateFilter(const DataView* keys,int n,std::string *dst) const = 0;

        virtual bool KeyMayMatch(const DataView& key,const DataView& filter) const = 0;
    };
}

#endif