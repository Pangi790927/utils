#ifndef GKDTREE_H
#define GKDTREE_H

#include <memory>

/*! @file
 * 
 * https://www.geeksforgeeks.org/cpp/kd-trees-in-cpp/
 * 
 * KD Tree
 * =======
 * 
 * Props:
 * - Every node in the tree is a k-dimensional point.
 * - Every non-leaf node generates a splitting hyperplane that divides the space into two parts.
 * - Points to the left of the splitting hyperplane are represented by the left subtree of that node
 * and points to the right of the hyperplane are represented by the right subtree.
 * - The hyperplane direction is chosen in the following way: it is perpendicular to the axis
 * corresponding to the depth of the node (modulo k).
 * - The tree is balanced when constructed with points that are uniformly distributed.
 * 
 * */

namespace kd_tree {

template <typename T, int K>
using vec_t = std::array<T, K>;

template <typename T, int K>
struct rect_t {
    vec_t<T, K> min_coords;
    vec_t<T, K> max_coords;
};

template <typename T, int K>
struct circle_t {
    vec_t<T, K> center;
    T radius;
};

template <typename T, int K, typename D>
struct node_t {
    node_t *left = nullptr;
    node_t *right = nullptr;
    D data;
    vec_t<T, K> p;
};

template <typename T, int K, typename D>
struct tree_opts_t {
    std::function<bool(const vec_t<T, K>& a, const vec_t<T, K>& b)> eq;
    std::function<T(const vec_t<T, K>& a, const vec_t<T, K>& b)> dist2;
    std::function<T(const rect_t<T, K>& rect, const circle_t<T, K>& circle)> rect_intersect;
    T inf;
};

/*! Main object that contains a root and some public helpers.
 * 
 * @param root Contains the root of the tree.
 * @param eq The function that checks if two points are equal.
 * @param dist2 The function that computes the distance squared of two points.
 * @param rect_intersect Function that checks if a circle and rectangle intersect.
 * @param inf A value that is greater than any coord value squared
 */
template <typename T, int K, typename D>
struct tree_t {
    node_t<T, K, D> *root = nullptr;

    std::shared_ptr<tree_opts_t<T, K, D>> o;
};

/*! Initializes a kd-tree structure with default helpers.
 * 
 * @param T The type of the spatial coordinates.
 * @param K The number of spatial coordinates.
 * @param D The type of the remembered data by the nodes.
 * 
 * @return A shared pointer to the newly created structure. Is null on error. */
template <typename T, int K, typename D>
inline std::shared_ptr<tree_t<T, K, D>> create();

/*! Inserts a new point into the kd-tree structure
 * 
 * @param tree The tree in which to insert the point.
 * @param p The point to insert.
 * @param data The data which acompanies the point.
 * 
 * @return A shared pointer to the newly created node. Is null on error. */
template <typename T, int K, typename D>
inline node_t<T, K, D> *insert(tree_t<T, K, D> *tree, const vec_t<T, K> &p, D&& data);


/*! Finds a list of points inside the kd-tree, given another point and a distance to it.
 * 
 * @param tree The tree in which to search for points.
 * @param p The point to wich to compare other points.
 * @param range The distance to wich to limit the search. If the range is equal to T{0}, an exact
 * match is searched and if the range is equal to T{-1} the nearest is searched. Any other negative
 * value besides -1 consists an error.
 * 
 * #return A vector containing the nodes respective to the matching points. For exact matches an
 * empty returned vector signals an error. */
template <typename T, int K, typename D>
inline std::vector<node_t<T, K, D>> find(tree_t<T, K, D> *tree, const vec_t<T, K> &p, T range);

/*! Finds a list of points inside the kd-tree, given another point and a distance to it and removes
 * the found nodes.
 * 
 * @param tree The tree in which to search for points.
 * @param p The point to wich to compare other points.
 * @param range The distance to wich to limit the search. If the range is equal to T{0}, the nearest
 * is searched
 * 
 * @return The number of removed nodes or a negative number on error. */
template <typename T, int K, typename D>
inline int remove(tree_t<T, K, D> *tree, const vec_t<T, K> &p, T range);

/*! Creates a string representation of a kd tree.
 * 
 * @param tree A pointer to the tree in question.
 * @param data_to_string_fn a custom function that converts Data to a string. */
template <typename T, int K, typename D>
inline std::string to_string(const tree_t<T, K, D> *tree,
        std::function<std::string(const D&)> data_to_string_fn = [](const D&){ return "[data]"; },
        std::function<std::string(const T&)> coord_to_string_fn = [](const T& val){
                return std::to_string(val); });

/*! Creates a string representation of a kd tree node.
 * 
 * @param tree A pointer to the node in question.
 * @param data_to_string_fn a custom function that converts Data to a string. */
template <typename T, int K, typename D>
inline std::string to_string(const node_t<T, K, D> *node,
        std::function<std::string(const D&)> data_to_string_fn = [](const D&){ return "[data]"; },
        std::function<std::string(const T&)> coord_to_string_fn = [](const T& val){
                return std::to_string(val); });

/* IMPLEMENTATION 
=================================================================================================
=================================================================================================
================================================================================================= */

template <typename T, int K, typename D>
inline std::shared_ptr<tree_t<T, K, D>> create() {
    auto ret = std::make_shared<tree_t<T, K, D>>();

    ret->o = std::make_shared<tree_opts_t<T, K, D>>();

    ret->o->eq = [](const vec_t<T, K>& a, const vec_t<T, K>& b) {
        return a == b;
    };
    ret->o->dist2 = [](const vec_t<T, K>& a, const vec_t<T, K>& b) {
        T dst_squared = 0;
        for (int i = 0; i < K; i++)
            dst_squared += (a[i] - b[i]) * (a[i] - b[i]);
        return dst_squared;
    };

    // https://stackoverflow.com/questions/401847/circle-rectangle-collision-detection-intersection
    ret->o->rect_intersect = [](const rect_t<T, K>& rect, const circle_t<T, K>& circle) {
        vec_t<T, K> circle_distance;
        for (int i = 0; i < K; i++)
            circle_distance[i] = abs(circle.center[i] - rect.min_coords[i]);

        vec_t<T, K> half_rect_sizes;
        for (int i = 0; i < K; i++) {
            half_rect_sizes[i] = (rect.max_coords[i] - rect.min_coords[i]) / 2.;
            if (circle_distance[i] > (half_rect_sizes[i] + circle.radius))
                return false;
        }

        for (int i = 0; i < K; i++) {
            if (circle_distance[i] <= half_rect_sizes[i])
                return true;
        }

        T dst_squared = 0;
        for (int i = 0; i < K; i++)
            dst_squared += (circle_distance[i] - half_rect_sizes[i]) * (circle_distance[i] -
                    half_rect_sizes[i]);

        return dst_squared <= circle.radius * circle.radius; 
    };
    ret->o->inf = std::numeric_limits<T>::max();
    return ret;
}

template <typename T, int K, typename D>
inline node_t<T, K, D> *insert_recursive(node_t<T, K, D> *&root,
        const vec_t<T, K> &p, D&& data, int depth, tree_t<T, K, D> *tree)
{
    if (!root)
        return root = new node_t{ .p = p, .data = std::forward<D>(data) };
    int coord = depth % K;
    if (p[coord] < root->p[coord])
        return tree_insert_recursive(root->left, p, depth + 1, tree);
    else
        return tree_insert_recursive(root->right, p, depth + 1, tree);
}

template <typename T, int K, typename D>
inline node_t<T, K, D> *insert(tree_t<T, K, D> *tree,
        const vec_t<T, K> &p, D&& data)
{
    return tree_insert_recursive(tree->root, p, std::forward<D>(data), 0);
}

template <typename T, int K, typename D>
inline void find_in_range_recursive(node_t<T, K, D> *root,
        const vec_t<T, K> &p, double range, std::vector<node_t<T, K, D> *> &result,
        int depth, tree_t<T, K, D> *tree, const rect_t<T, K>& bb)
{
    if (!root)
        return;

    circle_t<T, K> zone_of_interest = { .center = p, .radius = range };
    int coord = depth % K;

    rect_t<T, K> left_bb = bb;
    left_bb.max_coords[coord] = root->p[coord];
    if (tree->o->rect_intersect(left_bb, zone_of_interest))
        find_in_range_recursive(root->left, range, result, depth + 1, tree);

    rect_t<T, K> right_bb = bb;
    right_bb.min_coords[coord] = root->p[coord];
    if (tree->o->rect_intersect(right_bb, zone_of_interest))
        find_in_range_recursive(root->right, range, result, depth + 1, tree);
}

template <typename T, int K, typename D>
inline node_t<T, K, D> *find_exact_recursive(node_t<T, K, D> *root,
        const vec_t<T, K> &p, int depth, tree_t<T, K, D> *tree)
{
    if (!root)
        return nullptr;
    if (tree->o->eq(root->p, p))
        return root;
    int coord = depth % K;
    if (p[coord] < root->p[coord])
        return find_exact_recursive(root->left, p, depth + 1, tree);
    else
        return find_exact_recursive(root->right, p, depth + 1, tree);
}

template <typename T, int K, typename D>
inline node_t<T, K, D> *find_nearest_recursive(node_t<T, K, D> *root,
        const vec_t<T, K> &p, int depth, tree_t<T, K, D> *tree, T min_dist_squared,
        rect_t<T, K> bb)
{
    if (!root)
        return nullptr;

    /* We first actualize the minimum distance to take into consideration the current node */
    node_t<T, K, D> *ret = nullptr;
    T dist_squared = tree->o->dist2(root->p, p);
    if (dist_squared < min_dist_squared) {
        min_dist_squared = dist_squared;
        ret = root;
    }

    /* We create a circle centered at p and of radius the new minimum distance */
    int coord = depth % K;
    T min_dist = sqrt(min_dist_squared);
    circle_t<T, K> zone_of_interest = { .center = p, .radius = min_dist };

    rect_t<T, K> left_bb = bb;
    left_bb.max_coords[coord] = root->p[coord];

    /* If the bounding box of the left zone intersects with our zone of interest we recurse the left
    branch */
    node_t<T, K, D> *ret_left = nullptr;
    if (tree->o->rect_intersect(left_bb, zone_of_interest)) {
        ret_left = find_nearest_recursive(root->left, p, depth + 1, tree, min_dist_squared,
                left_bb);
    }

    rect_t<T, K> right_bb = bb;
    right_bb.min_coords[coord] = root->p[coord];

    /* If the bounding box of the right zone intersects with our zone of interest we recurse the
    right branch */
    node_t<T, K, D> *ret_right = nullptr;
    if (tree->o->rect_intersect(right_bb, zone_of_interest)) {
        ret_right = find_nearest_recursive(root->right, p, depth + 1, tree, min_dist_squared,
                right_bb); 
    }

    /* Now we have 3 candidates: the current point, the minimum found for the left branch and
    the minimum found on the right branch. So we first compare this point to the left one and
    remember the best in ret (that is if it is not null) */
    T new_min = min_dist_squared;
    if (!ret)
        ret = ret_left;
    else if (ret_left && (new_min = tree->o->dist2(ret->p, ret_left->p)) < min_dist_squared) {
        min_dist_squared = new_min;
        ret = ret_left;
    }

    /* now if we have a point from the previous comparation we compare it with the right result and
    choose the best */
    if (ret && ret_right && tree->o->dist2(ret->p, ret_right->p) < min_dist_squared)
        ret = ret_right;
    if (!ret)
        ret = ret_right;

    return ret;
}

template <typename T, int K, typename D>
inline std::vector<node_t<T, K, D> *> find(tree_t<T, K, D> *tree,
        const vec_t<T, K> &p, T range)
{
    using ret_t = std::vector<node_t<T, K, D> *>;

    if (range == T{0}) {
        auto ret = tree_find_exact_recursive(tree->root, p, 0, tree);
        return ret ? ret_t{ret} : ret_t{};
    }
    else if (range == T{-1}) {
        rect_t<T, K> bb;
        for (int i = 0; i < K; i++) {
            bb.min_coords[i] = -tree->o->inf;
            bb.max_coords[i] = tree->o->inf;
        }
        auto ret = tree_find_nearest_recursive(tree->root, p, 0, tree, tree->o->inf, bb);
        return ret ? ret_t{ret} : ret_t{};
    }
    else {
        ret_t ret;
        tree_find_in_range_recursive(tree->root, p, range, ret, 0, tree);
        return ret;
    }
    return ret_t{};
}

template <typename T, int K, typename D>
inline int remove_recursive(tree_t<T, K, D> *&root, const vec_t<T, K>& p) {
    if (!root) {
        /* Can't delete a null node */
        return 0;
    }
    if (!root->left && !root->right) {
        delete root;
        root = nullptr;
        return 1;
    }
    /* TODO: implement */
}

template <typename T, int K, typename D>
inline int remove(tree_t<T, K, D> *tree, const vec_t<T, K> &p, T range) {
    /* TODO: all the cases */
    // return remove_recursive(tree->root, p);
    return -1;
}

template <typename T, int K, typename D>
inline std::string to_string_recursive(const tree_t<T, K, D> *root,
        std::function<std::string(const D&)> data_to_string_fn,
        std::function<std::string(const T&)> coord_to_string_fn,
        int depth)
{
    std::string ret = std::string(depth * 2, ' ') + to_string(root) + "\n";
    if (root->left)
        ret += to_string_recursive(root->left, data_to_string_fn, coord_to_string_fn, depth + 1);
    else
        ret += std::string((depth + 1) * 2, ' ') + "[null_left]\n";
    if (root->right)
        ret += to_string_recursive(root->right, data_to_string_fn, coord_to_string_fn, depth + 1);
    else
        ret += std::string((depth + 1) * 2, ' ') + "[null_right]\n";
    return ret;
}

template <typename T, int K, typename D>
inline std::string to_string(const tree_t<T, K, D> *tree,
        std::function<std::string(const D&)> data_to_string_fn,
        std::function<std::string(const T&)> coord_to_string_fn)
{
    if (!tree)
        return "[invalid_null_tree]";
    if (!tree->root)
        return "[null_root]";
    return to_string_recursive(tree->root, data_to_string_fn, coord_to_string_fn, 0);
}

template <typename T, int K, typename D>
inline std::string to_string(const node_t<T, K, D> *node,
        std::function<std::string(const D&)> data_to_string_fn,
        std::function<std::string(const T&)> coord_to_string_fn)
{
    std::string ret = "[";
    for (int i = 0; i < K; i++) {
        ret += coord_to_string_fn(node->p[i]);
        if (i != K - 1)
            ret += ", ";
    }
    ret += "]{data: " + data_to_string_fn(node->data) + "}";
    return ret;
}

} /* namespace kd_tree */

#endif
