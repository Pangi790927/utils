#ifndef GKDTREE_H
#define GKDTREE_H

#include <memory>
#include <functional>

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

namespace kdtree {

/*! The type of any point, holding K coordinates. */
template <typename T, int K>
using vec_t = std::array<T, K>;

/*! An axis-aligned hyperbox */
template <typename T, int K>
struct hyperbox_t {
    vec_t<T, K> min_coords;
    vec_t<T, K> max_coords;
};

template <typename T, int K>
struct hypersphere_t {
    vec_t<T, K> center;
    T radius;
};

/*! The generic node inside the tree. Pointers of the type node_t<T, K, D>* should be consistent
 * until a deletion occours
 * 
 * @param T the type for coordinates of a point. T must be copiable, initializable by 0 and -1 and
 * have comparators (<, ==). T must be signed.
 * @param K the number of coordinates of a point
 * @param D the type of data stored for a point. Data must be copiable and initializable. */
template <typename T, int K, typename D>
struct node_t {
    /*! left sub-tree root */
    node_t *left = nullptr;

    /*! right sub-tree root */
    node_t *right = nullptr;

    /*! data that is held by the node. */
    D data;

    /*! The point aferent to the data. */
    vec_t<T, K> p;
};

/*! Tree options: functions and variables that change the behaviour of the tree. */
template <typename T, int K, typename D>
struct tree_opts_t {
    /*! eq - equal - This is the way this tree checks if two points are exactly the same */
    std::function<bool(const vec_t<T, K>& a, const vec_t<T, K>& b)> eq;

    /*! dist2 - distance squared - The function that comutes the square of the distance between two
     * points
     * 
     * @param a the first point
     * @param b the second point */
    std::function<T(const vec_t<T, K>& a, const vec_t<T, K>& b)> dist2;

    /*! rec_intersect - the function that returns true if a axis aligned hyperbox and a hypersphere
     * intersect.
     * 
     * @param rect the hyperbox
     * @param circle the hypersphere */
    std::function<bool(const hyperbox_t<T, K>& rect, const hypersphere_t<T, K>& circle)>
            rect_intersect;

    /*! inf - a number greater than any coordinate in any point. (-inf) should also be smaller than
     * any coordinate. */
    T inf;
};

/*! Main object that contains a root and some public helpers.
 * 
 * @param root Contains the root of the tree.
 * @param eq The function that checks if two points are equal.
 * @param dist2 The function that computes the distance squared of two points.
 * @param rect_intersect Function that checks if a hypersphere and rectangle intersect.
 * @param inf A value that is greater than any coord value squared
 */
template <typename T, int K, typename D>
struct tree_t {
    node_t<T, K, D> *root = nullptr;

    std::shared_ptr<tree_opts_t<T, K, D>> o;

    static std::shared_ptr<tree_t<T, K, D>> create(std::shared_ptr<tree_opts_t<T, K, D>>
            co = nullptr);
};

/*! Initializes a kd-tree structure with default helpers.
 * 
 * @param T The type of the spatial coordinates.
 * @param K The number of spatial coordinates.
 * @param D The type of the remembered data by the nodes.
 * 
 * @param custom_opts A set of custom tree options, those are defined in tree_opts_t
 * 
 * @return A shared pointer to the newly created structure. Is null on error. */
template <typename T, int K, typename D>
inline std::shared_ptr<tree_t<T, K, D>> create(std::shared_ptr<tree_opts_t<T, K, D>> custom_opts
        = nullptr);

/*! Inserts a new point into the kd-tree structure
 * 
 * @param tree The tree in which to insert the point.
 * @param p The point to insert.
 * @param data The data which acompanies the point.
 * 
 * @return A shared pointer to the newly created node. Is null on error. */
template <typename T, int K, typename D>
inline node_t<T, K, D> *insert(tree_t<T, K, D> *tree, const vec_t<T, K> &p, D&& data);

/*! same as above, but accepts shared pointer as input */
template <typename T, int K, typename D>
inline node_t<T, K, D> *insert(std::shared_ptr<tree_t<T, K, D>> tree, const vec_t<T, K> &p,
        D &&data)
{
    return kdtree::insert<T, K, D>(tree.get(), p, std::forward<D>(data));
}

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
inline std::vector<node_t<T, K, D>> find(tree_t<T, K, D> *tree, const vec_t<T, K> &p, T&& range);

/*! same as above, but accepts shared pointer as input */
template <typename T, int K, typename D>
inline std::vector<node_t<T, K, D>> find(std::shared_ptr<tree_t<T, K, D>> tree,
        const vec_t<T, K> &p, T&& range)
{
    return kdtree::find(tree.get(), p, std::forward<T>(range));
}


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
inline int remove(tree_t<T, K, D> *tree, const vec_t<T, K> &p, T&& range);

/*! same as above, but accepts shared pointer as input */
template <typename T, int K, typename D>
inline int remove(std::shared_ptr<tree_t<T, K, D>> tree, const vec_t<T, K> &p, T&& range) {
    return kdtree::remove(tree.get, p, std::forward<T>(range));
}

/*! Creates a string representation of a kd tree.
 * 
 * @param tree A pointer to the tree in question.
 * @param data_to_string_fn a custom function that converts Data to a string. */
template <typename T, int K, typename D>
inline std::string to_string(const tree_t<T, K, D> *tree,
        std::function<std::string(const D&)> data_to_string_fn = [](const D&){ return "[data]"; },
        std::function<std::string(const T&)> coord_to_string_fn = [](const T& val){
                return std::to_string(val); });

/*! same as above, but accepts shared pointer as input */
template <typename T, int K, typename D>
inline std::string to_string(std::shared_ptr<tree_t<T, K, D>> tree,
        std::function<std::string(const D&)> data_to_string_fn = [](const D&){ return "[data]"; },
        std::function<std::string(const T&)> coord_to_string_fn = [](const T& val){
                return std::to_string(val); })
{
    return kdtree::to_string<T, K, D>(tree.get(), data_to_string_fn, coord_to_string_fn);
}

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
inline std::shared_ptr<tree_t<T, K, D>> tree_t<T, K, D>::create(
        std::shared_ptr<tree_opts_t<T, K, D>> co)
{
    return kdtree::create<T, K, D>(co);
}

template <typename T, int K, typename D>
inline std::shared_ptr<tree_t<T, K, D>> create(std::shared_ptr<tree_opts_t<T, K, D>> custom_opts) {
    auto ret = std::make_shared<tree_t<T, K, D>>();

    if (custom_opts) {
        ret->o = custom_opts;
        return ret;
    }

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
    ret->o->rect_intersect = [](const hyperbox_t<T, K>& rect, const hypersphere_t<T, K>& circle) {
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
inline node_t<T, K, D> *insert_recursive(node_t<T, K, D> **root,
        const vec_t<T, K> &p, D&& data, int depth, tree_t<T, K, D> *tree)
{
    if (!(*root))
        return (*root) = new node_t<T, K, D>{ .data = std::forward<D>(data), .p = p };
    int coord = depth % K;
    if (p[coord] < (*root)->p[coord])
        return insert_recursive<T, K, D>(&(*root)->left, p, std::forward<D>(data), depth + 1, tree);
    else
        return insert_recursive<T, K, D>(&(*root)->right, p, std::forward<D>(data), depth + 1, tree);
}

template <typename T, int K, typename D>
inline node_t<T, K, D> *insert(tree_t<T, K, D> *tree,
        const vec_t<T, K> &p, D&& data)
{
    return insert_recursive<T, K, D>(&(tree->root), p, std::forward<D>(data), 0, tree);
}

template <typename T, int K, typename D>
inline void find_in_range_recursive(node_t<T, K, D> *root,
        const vec_t<T, K> &p, T &&range, std::vector<node_t<T, K, D> *> &result,
        int depth, tree_t<T, K, D> *tree, const hyperbox_t<T, K>& bb)
{
    if (!root)
        return;

    hypersphere_t<T, K> zone_of_interest = { .center = p, .radius = std::forward<T>(range) };
    int coord = depth % K;

    hyperbox_t<T, K> left_bb = bb;
    left_bb.max_coords[coord] = root->p[coord];
    if (tree->o->rect_intersect(left_bb, zone_of_interest))
        find_in_range_recursive(root->left, std::forward<T>(range), result, depth + 1, tree);

    hyperbox_t<T, K> right_bb = bb;
    right_bb.min_coords[coord] = root->p[coord];
    if (tree->o->rect_intersect(right_bb, zone_of_interest))
        find_in_range_recursive(root->right, std::forward<T>(range), result, depth + 1, tree);
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
        hyperbox_t<T, K> bb)
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
    hypersphere_t<T, K> zone_of_interest = { .center = p, .radius = min_dist };

    hyperbox_t<T, K> left_bb = bb;
    left_bb.max_coords[coord] = root->p[coord];

    /* If the bounding box of the left zone intersects with our zone of interest we recurse the left
    branch */
    node_t<T, K, D> *ret_left = nullptr;
    if (tree->o->rect_intersect(left_bb, zone_of_interest)) {
        ret_left = find_nearest_recursive(root->left, p, depth + 1, tree, min_dist_squared,
                left_bb);
    }

    hyperbox_t<T, K> right_bb = bb;
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
        const vec_t<T, K> &p, T&& range)
{
    using ret_t = std::vector<node_t<T, K, D> *>;

    if (range == T{0}) {
        auto ret = find_exact_recursive(tree->root, p, 0, tree);
        return ret ? ret_t{ret} : ret_t{};
    }
    else if (range == T{-1}) {
        hyperbox_t<T, K> bb;
        for (int i = 0; i < K; i++) {
            bb.min_coords[i] = -tree->o->inf;
            bb.max_coords[i] = tree->o->inf;
        }
        auto ret = find_nearest_recursive(tree->root, p, 0, tree, tree->o->inf, bb);
        return ret ? ret_t{ret} : ret_t{};
    }
    else {
        ret_t ret;
        find_in_range_recursive(tree->root, p, std::forward<T>(range), ret, 0, tree);
        return ret;
    }
    return ret_t{};
}

template <typename T, int K, typename D>
inline node_t<T, K, D> *find_min_coord(node_t<T, K, D> *root, int d, int depth) {
    if (!root)
        return nullptr;

    int cd = depth % K; /* this is the current coord of the root */

    /* if we are on a node that has cd as it's active coord then we know from kd-tree props that
    it's children are ordered by the tree props */
    if (cd == d) {
        if (root->left == nullptr)
            return root;
        return find_min_coord(root->right, d, depth+1);
    }

    /* else we must search both sub-trees for said min */
    auto res = root;
    auto l = find_min_coord(root->left, d, depth+1);
    auto r = find_min_coord(root->right, d, depth+1);
    if (l && l->p[d] < res->p[d])
        res = l;
    if (r && r->p[d] < res->p[d])
        res = r;
    return res;
}

template <typename T, int K, typename D>
inline node_t<T, K, D> *remove_recursive(tree_t<T, K, D> *tree, node_t<T, K, D> *root,
        const vec_t<T, K>& p, int depth)
{
    int cd = depth % K;

    if (!root)
        return 0;

    if (tree->o->eq(root->p, p)) {
        if (root->right != nullptr) {
            auto min_node = find_min_coord(root->right, cd, depth+1);

            root->p = min_node->p;
            root->data = min_node->data;

            root->right = remove_recursive(tree, root->right, min_node->p, depth+1);
        }
        else if (root->left != nullptr) {
            auto min_node = find_min_coord(root->left, cd, depth+1);

            root->p = min_node->p;
            root->data = min_node->data;

            root->right = remove_recursive(tree, root->left, min_node->p, depth+1);
            root->left = nullptr;
        }
        else {
            delete root;
            return nullptr;
        }
    }

    if (p[cd] < root->p[cd])
        root->left = remove_recursive(tree, root->left, p, depth+1);
    else
        root->right = remove_recursive(tree, root->right, p, depth+1);
    return root;
}

template <typename T, int K, typename D>
inline int remove(tree_t<T, K, D> *tree, const vec_t<T, K> &p, T&& range) {
    /* TODO: all the cases */
    // return remove_recursive(tree->root, p);
    return -1;
}

template <typename T, int K, typename D>
inline std::string to_string_recursive(const node_t<T, K, D> *root,
        std::function<std::string(const D&)> data_to_string_fn,
        std::function<std::string(const T&)> coord_to_string_fn,
        int depth)
{
    std::string ret = std::string(depth * 2, ' ') +
            to_string(root, data_to_string_fn, coord_to_string_fn) + "\n";
    if (root->left)
        ret += to_string_recursive(root->left, data_to_string_fn, coord_to_string_fn, depth+1);
    else
        ret += std::string((depth + 1) * 2, ' ') + "[null_left]\n";
    if (root->right)
        ret += to_string_recursive(root->right, data_to_string_fn, coord_to_string_fn, depth+1);
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
