#pragma once

#include "tagfilterdb/support/range.hpp"
#include "tree/ParseTreeType.hpp"

namespace tagfilterdb::compiler::tree {

class ParseTree {
  public:
    ParseTree(ParseTree const &) = delete;

    virtual ~ParseTree() = default;

    ParseTree &operator=(ParseTree const &) = delete;

    ParseTreeType getPraseTreeType() const { return m_treeNodeType; }

  protected:
    explicit ParseTree(ParseTreeType treeNodeType)
        : m_treeNodeType(treeNodeType) {}

  public:
    ParseTree *m_parent = nullptr;
    std::vector<ParseTree *> m_children;

  private:
    const ParseTreeType m_treeNodeType;
};

} // namespace tagfilterdb::compiler::tree
