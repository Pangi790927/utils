#ifndef BVH_TREE_H
#define BVH_TREE_H

/*! @file
 * 
 * Bounding Volume Hierarchies
 * 
 * Main inspiration:
 * https://www.pbr-book.org/3ed-2018/Primitives_and_Intersection_Acceleration/Bounding_Volume_Hierarchies
 * https://github.com/mmp/pbrt-v3/blob/master/src/accelerators/bvh.cpp
 * 
 * Rotations:
 * https://pismin.com/10.1109/rt.2008.4634624
 * 
 * (TODO: This file may need to be C compatible)
 */

/*! TODO: possible rotations: child with grandchild
 * 
 *              root
 *            /      \
 *           /        \
 *          /          \
 *        ch1          ch2
 *       /   \        /   \
 *      /     \      /     \
 *    gc1     gc2  gc3     gc4
 * 
 * So in this example possible swaps are: (gc1, ch2), (gc2, ch2), (gc3, ch1), (gc4, ch1)
 * By a swap we understand moving the points around, for example (gc1, ch2) would look like so:
 * 
 *               root
 *              /    \
 *             /      \
 *           ch1      gc1
 *          /   \
 *         /     \ 
 *       ch2     gc2
 *      /   \
 *     /     \
 *   gc3     gc4
 * 
 * Of course some bounding boxes should be racalculated after this and the result should be a bvh
 * with a different SAH score.
 * 
 */


#endif
