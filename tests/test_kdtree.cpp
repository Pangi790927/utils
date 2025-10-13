#define KDTREE_ENABLE_LOGGING

#include "kdtree.h"
// #include "debug.h"

#include <set>
#include <format>

// aproximatively from debug.h:
#define DBG(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)
#define ASSERT_FN(fn_call) do { int x = fn_call; if (x < 0) { print("error_str"); return -1; } while (0);

using data_t = int;
using coord_t = int;
constexpr int coord_cnt = 5;

using vecN_t = kdtree::vec_t<coord_t, coord_cnt>;
using kdt_t = kdtree::tree_t<coord_t, coord_cnt, data_t>;
using kdt_p = std::shared_ptr<kdt_t>;

void print_tree(kdt_p tree) {
    auto tstr = kdtree::to_string<coord_t, coord_cnt, data_t>(tree,
            [](const data_t& data) -> std::string { return std::to_string(data); });
    DBG("TREE:\n%s", tstr.c_str());
}

std::string to_string(const vecN_t& v) {
    std::string ret = "[";
    for (int i = 0; i < v.size(); i++) {
        ret += std::to_string(v[i]);
        if (i+1 != v.size())
            ret += ", ";
    }
    return ret + "]";
}

int main(int argc, char const *argv[])
{
    srand(0);

    /* This variable will help signal to repeat the test only once on error */
    bool already_repeated = false;
    for (int test = 0; test < 10000; test++) try {
        srand(test);
        auto tree = kdt_t::create();
        auto repeat_test = [&]() {
            throw std::runtime_error(std::format("REPEATING TEST {}", test));
        };
        auto assert_tree_validity = [&]() {
            if (!kdtree::is_tree_valid(tree)) {
                kdtree::enable_logging = true;
                kdtree::is_tree_valid(tree); /* second call to actually print the fault */
                print_tree(tree);
                repeat_test();
            }
        };

        std::set<vecN_t> inserted_points;
        assert_tree_validity();
        for (int i = 0; i < test / 10; i++) {
            assert_tree_validity();
            if (inserted_points.size() && (rand() % (test % 4 + 1) == 0)) {
                if (kdtree::enable_logging) {
                    DBG("BEFORE ERASING: ");
                    print_tree(tree);
                }
                assert_tree_validity();
                auto p = *inserted_points.begin();
                KDTREE_DEBUG("finding and erasing node: %s", kdtree::to_string(p).c_str());
                auto nodes = kdtree::find(tree, p, kdt_t::exact);
                if (!nodes.size() || !nodes.front()) {
                    DBG("Failed test: %d node not found %s sz %zu",
                            test, to_string(p).c_str(), nodes.size());
                    kdtree::enable_logging = true;
                    kdtree::find(tree, p, kdt_t::exact);
                    print_tree(tree);
                    repeat_test();
                }
                assert_tree_validity();
                if (!tree->o->eq(p, nodes.front()->p)) {
                    kdtree::enable_logging = true;
                    DBG("Failed test: %d find mismatched %s vs %s",
                            test, to_string(p).c_str(), to_string(nodes.front()->p).c_str());
                    kdtree::find(tree, p, kdt_t::exact);
                    print_tree(tree);
                    repeat_test();
                }
                inserted_points.erase(p);
                int ret;
                if ((ret = kdtree::remove(tree, p, kdt_t::exact)) != 1) {
                    DBG("Failed test: remove[%d]", ret);
                    print_tree(tree);
                    repeat_test();
                }
                if (kdtree::enable_logging) {
                    DBG("AFTER ERASING:");
                    print_tree(tree);
                }
                assert_tree_validity();
            }
            else {
                assert_tree_validity();
                vecN_t point;
                for (int i = 0; i < coord_cnt; i++)
                    point[i] = (rand() % 20) - 5;
                KDTREE_DEBUG("inserting point: %s", kdtree::to_string(point).c_str());
                kdtree::insert(tree, point, 1000 + i);
                inserted_points.insert(point);
                assert_tree_validity();
            }
        }
        if (test % 1000 == 0) {
            DBG("Passed test: %d", test);
        }
        KDTREE_DEBUG("Passed test: %d", test);
        if (already_repeated)
            break;
    }
    catch (std::exception &e) {
        if (already_repeated)
            break;
        DBG("++++++++++ EXCEPTION: %s", e.what());
        already_repeated = true;
        test--;
    }

    return 0;
}
