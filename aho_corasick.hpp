#include <queue>
#include <unordered_set>

#include <malloc.h>
#include <iostream>
//#include <unordered_set>

//#include <boost/functional/hash.hpp>

#define TEMPLATE_DECL \
    template<class Value, class KeyChar, int MAX_HASH_SIZE, int MAX_FAIL_DEPTH> 

#define AHO_CORASICK \
    AhoCorasick<Value, KeyChar, MAX_HASH_SIZE, MAX_FAIL_DEPTH>::AhoCorasick

TEMPLATE_DECL
AHO_CORASICK::AhoCorasick(): m_root(nullptr)
{
}

TEMPLATE_DECL
AHO_CORASICK::~AhoCorasick()
{
    Clear();
    malloc_trim(0);
}

TEMPLATE_DECL
void AHO_CORASICK::Clear()
{
    delete(m_root);
    m_root = nullptr;
}

TEMPLATE_DECL
void AHO_CORASICK::Build( const PatternVec & patterns )
{
    Init();

    for(const PatternItem & item: patterns) 
    {
        Insert(item);
    }

    BuildFailPtr();
}

TEMPLATE_DECL
void AHO_CORASICK::Query(const KeyString & query, 
        QueryResult * result) const {

    result->clear();
    NodePtr node = {m_root, 0};

    //int step = 0;
    //std::unordered_set<NodePtr, boost::hash<NodePtr>> value_set;

    for( uint32_t i = 0; i <= query.size(); ++i ) {

        //++step;

        uint32_t cur_depth = node.first->depth + node.second;

        //++step;

        if( cur_depth > MAX_FAIL_DEPTH && 
                ( i >= query.size() || 
                  node.first->GetNext(node.second, query[i]).first == nullptr ) )//no fail ptr
        {
            i -= (cur_depth + 1);
            node = {m_root, 0};
            continue;
        }

        if( i >= query.size() ) break;

        KeyChar c = query[i];

        while( node.first != m_root &&
                node.first->GetNext(node.second, c).first == nullptr ) 
        {
            //++step;
            node = node.first->GetFail(node.second, m_root);
            //++step;
        }

        node = node.first->GetNext(node.second, c);
        if( node.first == nullptr )
        {
            node = {m_root, 0};
            continue;
        }

        for(NodePtr t = node; t.first != m_root; t = t.first->GetFail(t.second, m_root))
        {
            //++step;

            auto value_ptr = t.first->GetValue(t.second);

            if( value_ptr != nullptr ) {
                if( result->emplace(*value_ptr).second == false ) {
                    break; 
                }
            }
        }
    }

    //std::cout << "query_step: " << step << std::endl;
}

TEMPLATE_DECL
void AHO_CORASICK::Init() {
    Clear();

    m_root = new Node(0, 0);
    m_root->node_str.clear();
}

TEMPLATE_DECL
const Value * AHO_CORASICK::QueryOne(const KeyString & pattern) const
{
    NodePtr p = {m_root, 0};
    for( auto & c: pattern )
    {
        NodePtr next = p.first->GetNext(p.second, c);
        if( next.first == nullptr)
        {
            return nullptr;
        }
        p = next;
    }

    auto res = p.first->GetValue(p.second);
    if( res != nullptr ) return &res->second;
    else return nullptr;
}

TEMPLATE_DECL
void AHO_CORASICK::Insert(const PatternItem & item)
{
    const KeyString & pattern = item.first;
    //const Value & value = item.second;

    NodePtr p = {m_root, 0};
    for( auto & c: pattern )
    {
        NodePtr next = p.first->GetNext(p.second, c);
        if( next.first == nullptr)
        {
            next = p.first->AddNext(p.second, c); 
        }
        p = next;
    }
    p.first->SetValue(p.second, item);
}

TEMPLATE_DECL
void AHO_CORASICK::BuildFailPtr()
{
    std::queue<NodePtr> q;
    q.push({m_root, 0});

    while( !q.empty() )
    {
        NodePtr cur = q.front();q.pop();            
        uint32_t depth = cur.first->depth + cur.second;

        if( cur.second + 1u < cur.first->node_str.size() )
        {
            if( depth + 1 < MAX_FAIL_DEPTH ) q.push({cur.first, cur.second + 1});

            KeyChar c = cur.first->node_str[cur.second+1];
            NodePtr fail_node = cur.first->FindFail(cur.second, c, m_root);

            if( fail_node.first != nullptr &&
                    (fail_node.first != cur.first || fail_node.second != cur.second + 1) )
            {
                cur.first->AddFail(cur.second+1, fail_node);
            }

            continue;
        }
        for( Node * next_node: cur.first->next_bucket )
        {
            while( next_node != nullptr )
            {
                if( depth + 1 < MAX_FAIL_DEPTH )q.push({next_node, 0});

                KeyChar c = next_node->node_str[0];
                NodePtr fail_node = cur.first->FindFail(cur.second, c, m_root);

                if( fail_node.first != nullptr &&
                        (fail_node.first != next_node || fail_node.second != 0) )
                {
                    next_node->AddFail(0, fail_node);
                }

                next_node = next_node->next;
            }
        }
    }
}

TEMPLATE_DECL
void AHO_CORASICK::GetSummary(Summary * summary) {
    return m_root->GetSummary(summary);
}

#define NODE \
    AHO_CORASICK::Node

TEMPLATE_DECL
NODE::Node(const KeyChar c, uint32_t depth, const uint8_t bucket_size): node_str(1, c),
    next_bucket(bucket_size, nullptr),
    value_items(nullptr), next(nullptr), fail_vec(nullptr), depth(depth)
{
}

TEMPLATE_DECL
NODE::~Node()
{
    for(Node * node: next_bucket) 
    {
        while(node != nullptr)
        {
            Node * t = node;
            node = node->next;
            delete(t);
        }
    }
    delete value_items;
    delete fail_vec;
}

TEMPLATE_DECL
inline uint8_t NODE::GetPos(KeyChar c) const
{
    return (c * 13) & (next_bucket.size() - 1);
}

TEMPLATE_DECL
inline uint8_t NODE::GetNextBucketSize() const
{
    uint8_t next_bucket_size = (next_bucket.size() >> 1);
    if( next_bucket_size <= 1 ) next_bucket_size = 2;
    return next_bucket_size;
}

TEMPLATE_DECL
typename AHO_CORASICK::NodePtr NODE::GetFail(const NodeOffset off, Node * root) const
{
    if( fail_vec == nullptr ) return {root, 0};

    for( auto & ptr: *fail_vec ) 
    {
        if( ptr.second == off ) return ptr.first;
    }
    return {root, 0};
}

TEMPLATE_DECL
typename AHO_CORASICK::NodePtr NODE::FindFail(const NodeOffset off, const KeyChar c, Node * root) const
{
    NodePtr cur_pre = GetFail(off, root);
    while(cur_pre.first->GetNext(cur_pre.second, c).first == nullptr && cur_pre.first != root)
    {
        cur_pre = cur_pre.first->GetFail(cur_pre.second, root);
    }

    return cur_pre.first->GetNext(cur_pre.second, c);
}

TEMPLATE_DECL
void NODE::AddFail(const NodeOffset off, const NodePtr & fail_ptr) 
{
    if( fail_vec == nullptr ) fail_vec = new FailVec();

    fail_vec->emplace_back(fail_ptr, off);
}

TEMPLATE_DECL
typename AHO_CORASICK::NodePtr NODE::GetNext(const NodeOffset off, const KeyChar c) const {

    if( off + 1u < node_str.size() )
    {
        if( node_str[off + 1] == c ) return {const_cast<Node*>(this), off+1};
        else return {nullptr, 0};
    }
    else
    {
        uint8_t pos = GetPos(c);
        for(Node * node = next_bucket[pos]; 
                node != nullptr; node = node->next) 
        {
            if( !node->node_str.empty() && node->node_str[0] == c) return {node, 0};
        }
        return {nullptr, 0};
    }
}

TEMPLATE_DECL
inline typename NODE * NODE::AddChild(const KeyChar c) {

    uint8_t pos = GetPos(c);
    uint8_t next_bucket_size = GetNextBucketSize();

    Node * node = new Node(c, depth + node_str.size(), next_bucket_size);
    node->next = next_bucket[pos];
    next_bucket[pos] = node;

    return node;
}

TEMPLATE_DECL
inline bool NODE::HasChild() const {

    for( auto & node: next_bucket )
    {
        if( node != nullptr ) return true;
    }

    if( next_bucket.size() == MAX_HASH_SIZE ) return true; //root

    return false;
}

TEMPLATE_DECL
typename AHO_CORASICK::NodePtr NODE::AddNext(const NodeOffset off, const KeyChar c) {

    if( off + 1u >= node_str.size() ) 
    {
        if( !HasChild() ) 
        {
            node_str.push_back(c);
            return {this, off+1};
        }
        else //add child node
        {
            Node * node = AddChild(c);

            return {node, 0};
        }
    }
    else //split node
    {
        SplitNode(off+1);
        {// new str
            Node * node = AddChild(c);
            return {node, 0};
        } 
    }
}

TEMPLATE_DECL
void NODE::SetValue(const NodeOffset offset, const PatternItem & v) {
    
    if( value_items == nullptr ) {

        value_items = new ValueItems();
    }

    for( auto & item: *value_items )
    {
        if( item.second == offset )
        {
            item.first = v;
            return;
        }
    }

    value_items->emplace_back(v, offset);
}

TEMPLATE_DECL
const typename AHO_CORASICK::PatternItem * NODE::GetValue(const NodeOffset offset) const {

    if( value_items == nullptr ) return nullptr;

    for( auto & item: *value_items ) {

        if( item.second == offset )
        {
            return &item.first;
        }
    }

    return nullptr; 
}

TEMPLATE_DECL
void NODE::SplitNode(const NodeOffset off) {

    decltype(next_bucket) old_bucket(next_bucket.size(), nullptr);
    old_bucket.swap(next_bucket);

    KeyString split_node_str = node_str.substr(off);
    node_str.erase(off);

    // old sub str
    Node * node = AddChild(split_node_str[0]);
    node->node_str = std::move(split_node_str);
    node->next_bucket.swap(old_bucket);

    if( value_items )
    {
        ValueItems old_values, new_values;
        for( auto &item: *value_items )
        {
            if( item.second >= off ) new_values.emplace_back(item.first, item.second-off);
            else old_values.emplace_back(item);
        }

        value_items->swap(old_values);
        if( !new_values.empty() )
            node->value_items = new ValueItems(std::move(new_values));
    }

}

TEMPLATE_DECL
void NODE::GetSummary(Summary * summary) {
    *summary = {0}; 
    summary->max_depth = depth;
    summary->node_num = 1;
    summary->max_node_str_size = node_str.size();
    if( fail_vec != nullptr ) {
        summary->max_fail_size = fail_vec->size();
    }
    if( value_items != nullptr ) {
        summary->max_value_size = value_items->size();
    }

    for( Node * next_node: next_bucket ) {
        int bucket_depth = 0;
        while( next_node != nullptr ) {
            ++bucket_depth;
            summary->max_bucket_depth = std::max(bucket_depth, summary->max_bucket_depth);

            Summary next_summary = {0}; 
            next_node->GetSummary(&next_summary);

            summary->max_depth = std::max(next_summary.max_depth, summary->max_depth);
            summary->max_bucket_depth = std::max(next_summary.max_bucket_depth, summary->max_bucket_depth);
            summary->max_fail_size = std::max(next_summary.max_fail_size, summary->max_fail_size);
            summary->max_value_size = std::max(next_summary.max_value_size, summary->max_value_size);
            summary->max_node_str_size = std::max(next_summary.max_node_str_size, summary->max_node_str_size);
            summary->node_num += next_summary.node_num;

            next_node = next_node->next;
        }
    }
}

#undef NODE
#undef TEMPLATE_DECL
#undef AHO_CORASICK
