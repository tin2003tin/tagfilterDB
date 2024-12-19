#ifndef TAGFILTERDB_JSON_MGR_H
#define TAGFILTERDB_JSON_MGR_H

#include "json.hpp"
#include "dataView.h"
#include <vector>
#include <cstdlib>

namespace tagfilterdb {
    struct JsonMgrOp {
        bool checkAll = false; 
    };

    class JsonMgr {
        private:
        JsonMgrOp op_;

        public:
        JsonMgr(JsonMgrOp op) : op_(op) {}

        nlohmann::json ToJson(DataView* view) {
            std::string jsonString = std::string(view->data,view->size);
            json data;
            try
            {
                data = nlohmann::json(jsonString);
            }
            catch(const std::exception& e)
            {
                std::cerr << e.what() << '\n';
            }
            return data;
        }
        
        bool GetPairDouble(std::vector<std::pair<std::string,std::string>>& ref,
                            std::vector<std::pair<double,double>>& out, nlohmann::json &data) {
            out.resize(ref.size());
            for (int i = 0; i < 2; i++) {
                if (op_.checkAll && (!data.contains(ref[i].first) || 
                                    !data.contains(ref[i].second))) {
                    return false;
                }
                if (data.contains(ref[i].first)) out[i].first = data[ref[i].first];   
                if (data.contains(ref[i].second)) out[i].second = data[ref[i].second];
            }
            return true;
        }
    };
}

#endif

