#include "kdtree.h"
#include "debug.h"

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

int main(int argc, char const *argv[])
{
    auto tree = kdt_t::create();

    insert(tree, {0, 1, 0, 0}, 1001);
    insert(tree, {0, 1, 1, 0}, 1002);

    print_tree(tree);

    return 0;
}
