#include <iostream>
#include "tagfilterdb/spatialIndex/spatialIndex.hpp"
#include <set>


int main() {
    using Sp2D = tagfilterdb::SpatialIndex<std::string, 2>;
    using Box2D = tagfilterdb::BoundingBox<2>;

    class CallBack : public tagfilterdb::ISIndexCallback<Sp2D> {
    public:
        struct SubNodeComparator {
            bool operator()(const CallBackValue &b, const CallBackValue &other) const {
                if (b.m_box.area() == other.m_box.area()) {
                    return b.m_data < other.m_data;
                }
                return b.m_box.area() > other.m_box.area();
            }
        };
        using setCallBack = std::set<CallBackValue, SubNodeComparator>;

        class CallBackIterator {
            setCallBack* _parent;
            setCallBack::iterator _curr;

        public:
            CallBackIterator(setCallBack* parent) : _parent(parent), _curr(parent->begin()) {}

            bool isEnd() const {
                return _curr == _parent->end();
            }

            std::string next() {
                if (!isEnd()) {
                    _curr++;
                    return _curr->m_data;
                }
                return ""; 
            }

            void reset() {
                _curr = _parent->begin();
            }

            std::string get() const {
                if (!isEnd()) {
                    return _curr->m_data;
                }
                return "";
            }

            std::string getAt(int index) {
                reset();
                if (index < 0 || index >= static_cast<int>(_parent->size())) {
                    return "";
                }
                std::advance(_curr, index);
                return _curr->m_data;
            }
        };

        setCallBack _item;

        bool process(const CallBackValue &value) override {
            _item.insert(value);
            return true;
        }

        void print() const {
            for (const auto &e : _item) {
                std::cout << e.m_data << " ";
            }
            std::cout << std::endl;
        }

        CallBackIterator getIterator() {
            return CallBackIterator(&_item);
        }
    };

    Sp2D sp;
    sp.Insert(Box2D::Universe(), "Chula");
    sp.Insert(Box2D({{0, 16}, {0, 12}}), "Engineer_facalty");
    sp.Insert(Box2D({{11, 16}, {5, 8}}), "Building1");
    sp.Insert(Box2D({{5, 11}, {5, 8}}), "Building2");
    sp.Insert(Box2D({{2, 11}, {0, 4}}), "Building3");
    sp.Insert(Box2D({{0, 5}, {9, 12}}), "Building4");
    sp.Insert(Box2D({{0, 5}, {5, 8}}), "Building100");
    sp.Insert(Box2D({{2, 3}, {11, 12}}), "Sky Cafe");
    sp.Insert(Box2D({{9, 10}, {1, 2}}), "Cafe Amazon");
    sp.Insert(Box2D({{0, 2}, {0, 4}}), "Icanteen");
    sp.Insert(Box2D({{17, 18}, {0, 12}}), "Road");

    Box2D skyCafeArea({{2, 3}, {11, 12}});
    CallBack skyCafeCallback;
    sp.SearchTag(skyCafeArea, &skyCafeCallback);

    skyCafeCallback.print();
    auto skyCafeCallbackIter = skyCafeCallback.getIterator();
    assert(skyCafeCallbackIter.get() == "Chula");
    assert(skyCafeCallbackIter.next() == "Engineer_facalty");
    assert(skyCafeCallbackIter.next()  == "Building4");
    assert(skyCafeCallbackIter.next()  == "Sky Cafe");
}