#ifndef NMTREE_H
#define NMTREE_H

/* TODO: Think this better */

/*! @file
 * 
 * A N-Tree is a structure that holds N dimensional points inside a tree where each node splits
 * the space in 2^N equal partitions. Leaf nodes all hold exactly one point.
 * 
 * (TODO: This file may need to be C compatible)
 */

namespace nmtree {

constexpr size_t ipow(size_t num, size_t pow) {
    return pow == 0 ? 1 : num * ipow(num, pow-1);
}

template <size_t N, typename T>
using vec_t = std::array<T, N>;

template <size_t N, size_t M, typename T, typename D>
struct storage_t;

template <size_t N, size_t M, typename T, typename D>
struct node_t {
    using data_t = D;
    using coord_t = T;

    vec_t<N, T> point = {};
    vec_t<N, T> min_coord = {}, max_coord = {};

    std::shared_ptr<storage_t> storage;

    int parrent = 0;      /* The parrent */
    int own_slot = 0;     /* This is the spot that this node has inside the parrent */
    std::array<int, ipow(N, M)> childs = {};
};

template <size_t N, size_t M, typename T, typename D>
struct storage_t;
    /* The first node inside the vector is there just as a sentry, so that no one really uses the
    index 0 */
    std::vector<node_t<N, M, T, D>> data(1);
};

/* IMPLEMENTATION
=================================================================================================
=================================================================================================
================================================================================================= */




#endif
