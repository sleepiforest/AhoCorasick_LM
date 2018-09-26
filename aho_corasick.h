#pragma once

#include <vector>
#include <string>
#include <unordered_map>

template<class Value, class KeyChar = char, int MAX_HASH_SIZE=32, int MAX_FAIL_DEPTH=5>
class AhoCorasick
{

typedef std::basic_string<KeyChar> KeyString;
typedef std::pair<KeyString, Value> PatternItem;
struct Node;
typedef uint32_t NodeOffset;
typedef std::pair<Node*, NodeOffset> NodePtr;
typedef std::vector<std::pair<NodePtr, NodeOffset>> FailVec;

public:
    AhoCorasick();

    AhoCorasick(const AhoCorasick & obj) = delete;
    AhoCorasick & operator = (const AhoCorasick & obj) = delete;

    ~AhoCorasick();

    void Clear();

    typedef std::vector<PatternItem> PatternVec;
	void Build( const PatternVec & patterns );

    typedef std::unordered_map<KeyString, Value> QueryResult;
    void Query(const KeyString & query, 
            QueryResult * result) const;

    const Value * QueryOne(const KeyString & pattern) const;

    //Use inside Build
    void Init();

    void Insert(const PatternItem & item);

    void BuildFailPtr();

    struct Summary {
        uint32_t max_depth;
        int max_bucket_depth;
        int max_fail_size;
        int max_value_size;
        int max_node_str_size;
        int node_num;
    };

    void GetSummary(Summary * summary);

private:
    Node *m_root;

private:
struct Node
{
    KeyString node_str;
    std::vector<Node *> next_bucket;
    typedef std::vector<std::pair<PatternItem, NodeOffset>> ValueItems;
    ValueItems * value_items;
    Node *next;
    FailVec * fail_vec;
    uint32_t depth;

    Node(const KeyChar c, uint32_t depth, const uint8_t bucket_size = MAX_HASH_SIZE);

    ~Node();

    inline uint8_t GetPos(KeyChar c) const;

    inline uint8_t GetNextBucketSize() const;

    NodePtr GetFail(const NodeOffset off, Node * root) const;

    NodePtr FindFail(const NodeOffset off, const KeyChar c, Node * root) const;

    void AddFail(const NodeOffset off, const NodePtr & fail_ptr);

    NodePtr GetNext(const NodeOffset off, const KeyChar c) const;

    inline Node * AddChild(const KeyChar c);

    inline bool HasChild() const;

    NodePtr AddNext(const NodeOffset off, const KeyChar c);

    void SetValue(const NodeOffset offset, const PatternItem & v);

    const PatternItem * GetValue(const NodeOffset offset) const;

    void SplitNode(const NodeOffset off);

    void GetSummary(Summary * summary);

}; //Node


}; //AhoCorasick

#include "aho_corasick.hpp"
