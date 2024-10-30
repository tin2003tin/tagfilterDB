#pragma once

#include "tree/ParseTree.hpp"

namespace tagfilterdb::compiler {
class RuleContext : public tree::ParseTree {
  public:
    size_t m_invokingState;

    RuleContext() : ParseTree(tree::ParseTreeType::RULE) {
        InitializeInstanceFields();
    }
    RuleContext(RuleContext *parent, size_t invokingState)
        : ParseTree(tree::ParseTreeType::RULE) {
        InitializeInstanceFields();
        this->m_parent = parent;
        this->m_invokingState = invokingState;
    }

    virtual int depth() {
        int n = 1;
        RuleContext *p = this;
        while (true) {
            if (p->m_parent == nullptr)
                break;
            p = static_cast<RuleContext *>(p->m_parent);
            n++;
        }
        return n;
    }
    virtual bool isEmpty();

    static bool is(const tree::ParseTree &parseTree) {
        return parseTree.getPraseTreeType() == tree::ParseTreeType::RULE;
    }
    static bool is(const tree::ParseTree *parseTree) {
        return parseTree != nullptr && is(*parseTree);
    }

    bool operator==(const RuleContext &other) { return this == &other; }

  private:
    void InitializeInstanceFields();
};
} // namespace tagfilterdb::compiler