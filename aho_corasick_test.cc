#include "aho_corasick.h"

#include "gtest/gtest.h"

class AhoCorasickTest: public testing::Test {};

TEST_F(AhoCorasickTest, BuildAndQuery) {

    typedef AhoCorasick<int> Ac;

    Ac ac;

    Ac::PatternVec patterns = {
        {"abcd", 0}, 
        {"abce", 1}, 
        {"abcf", 2}, 
        {"abc", 3}, 
        {"ab", 4}, 
        {"bc", 5}, 
        {"acd", 6}, 
        {"ace", 7}, 
        {"bcd", 8}, 
        {"《论十大关系》导读", 12},
        {"《黄帝内经》对症养五脏", 12},
        {"一 一", 11},
        {"梅花二首", 9}, 
        {"梅花", 10},
    };

    ac.Build(patterns);


#define CHECK(query, match_num)\
    {\
        decltype(ac)::QueryResult result;\
        ac.Query(query, &result);\
        printf("%s match: ", query);\
        for(auto & item: result)\
        {\
            printf("{%s, %d} ", item.first.c_str(), item.second);\
        }\
        puts("");\
        ASSERT_EQ(match_num, result.size());\
    }

    CHECK("abcde", 5);
    CHECK("abcd", 5);
    CHECK("abce", 4);
    CHECK("abcf", 4);
    CHECK("ab", 1);
    CHECK("a", 0);
    CHECK("ac", 0);
    CHECK("abc", 3);
    CHECK("bcd", 2);
    CHECK("梅花二首", 2);
    CHECK("一 一", 1);

#undef CHECK
}
