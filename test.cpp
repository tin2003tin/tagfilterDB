#include <vector>
#include <string>
#include <algorithm>
#include <unordered_map>

using namespace std;

struct TreeNode
{
    int val;
    TreeNode *left;
    TreeNode *right;
    TreeNode() : val(0), left(nullptr), right(nullptr) {}
    TreeNode(int x) : val(x), left(nullptr), right(nullptr) {}
    TreeNode(int x, TreeNode *left, TreeNode *right) : val(x), left(left), right(right) {}
};

class Solution
{
public:
    TreeNode *subtreeWithAllDeepest(TreeNode *root)
    {
        unordered_map<TreeNode *, int> mapping;
        recursive(root, 0, mapping);
        TreeNode *minNode;
        int maxHeight = INT32_MAX;
        for (auto m : mapping)
        {
            if (m.second > maxHeight)
            {
                maxHeight = m.second;
                minNode = m.first;
            }
        }
        return minNode;
    }
    void recursive(TreeNode *node, int height, unordered_map<TreeNode *, int> mapping)
    {
        if (node == nullptr)
        {
            return;
        }
        recursive(node->left, height + 1, mapping);
        recursive(node->right, height + 1, mapping);
        if (!isLeaf(node))
        {
            mapping[node] = height;
        }
    }

    bool isLeaf(TreeNode *node)
    {
        return node->left == nullptr || node->right == nullptr;
    }
};