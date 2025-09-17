#define KDTREE_ENABLE_LOGGING

#include "kdtree.h"
#include "debug.h"

#include <set>

// DBG(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)
// ASSERT_FN(fn_call) do { int x = fn_call; if (x < 0) { print("error_str"); return -1; } while (0);

using data_t = int;
using coord_t = int;
constexpr int coord_cnt = 4;

using vec4_t = kdtree::vec_t<coord_t, coord_cnt>;
using kdt_t = kdtree::tree_t<coord_t, coord_cnt, data_t>;
using kdn_t = kdtree::node_t<coord_t, coord_cnt, data_t>;
using kdt_p = std::shared_ptr<kdt_t>;
using kdn_p = std::shared_ptr<kdn_t>;

void print_tree(kdt_p tree) {
    auto tstr = kdtree::to_string<coord_t, coord_cnt, data_t>(tree,
            [](const data_t& data) -> std::string { return std::to_string(data); });
    DBG("TREE:\n%s", tstr.c_str());
}

std::string to_string(const vec4_t& v) {
    std::string ret = "[";
    for (int i = 0; i < v.size(); i++) {
        ret += std::to_string(v[i]);
        if (i+1 != v.size())
            ret += ", ";
    }
    return ret + "]";
}

#define ASSERT_TREE_VALIDITY \
if (!kdtree::is_tree_valid(tree)) { \
    DBG("TREE IS INVALID"); \
    kdtree::enable_logging = true; \
    kdtree::is_tree_valid(tree); \
    print_tree(tree); \
    DBG("REPEATING TEST: %d", test); \
    test--; \
    continue; \
}

int main(int argc, char const *argv[])
{
    srand(0);

    for (int test = 0; test < 10000; test++) {
        auto tree = kdt_t::create();
        std::set<vec4_t> inserted_points;
        ASSERT_TREE_VALIDITY;
        for (int i = 0; i < test / 10; i++) {
            if (inserted_points.size() && (rand() % (test % 4 + 1) == 0)) {
                auto p = *inserted_points.begin();
                auto nodes = kdtree::find(tree, p, kdt_t::exact);
                if (!nodes.size() || !nodes.front()) {
                    DBG("Failed test: node not found %s sz %zu", to_string(p).c_str(), nodes.size());
                    kdtree::enable_logging = true;
                    kdtree::find(tree, p, kdt_t::exact);
                    print_tree(tree);
                    return -1;
                }
                ASSERT_TREE_VALIDITY;
                if (!tree->o->eq(p, nodes.front()->p)) {
                    kdtree::enable_logging = true;
                    DBG("Failed test: find mismatched %s vs %s",
                            to_string(p).c_str(), to_string(nodes.front()->p).c_str());
                    kdtree::find(tree, p, kdt_t::exact);
                    print_tree(tree);
                    return -1;
                }
                inserted_points.erase(p);
                int ret;
                if ((ret = kdtree::remove(tree, p, kdt_t::exact)) != 1) {
                    DBG("Failed test: remove[%d]", ret);
                    print_tree(tree);
                    return -1;
                }
                ASSERT_TREE_VALIDITY;
            }
            else {
                ASSERT_TREE_VALIDITY;
                vec4_t point = {rand()%4, rand()%4, rand()%4, rand()%4};
                kdtree::insert(tree, point, 1000 + i);
                inserted_points.insert(point);
                ASSERT_TREE_VALIDITY;
            }
        }
    }

    return 0;
}
