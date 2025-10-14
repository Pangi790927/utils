#ifndef VULKAN_CONFIG_H
#define VULKAN_CONFIG_H

/*! @file This file will be used to auto-initialize Vulkan. This file will describe the structure
 * of the Vulkan objects and create an vulkan_obj_t with the specific type and an associated name.
 * In this way you can configure all the semaphores and buffers from here and ask for them inside
 * the program. The objective is to have the entire Vulkan pipeline described from outside of the
 * source code. */

#include "vulkan_utils.h"

/* What do I really care about? So I can make a format that covers all the things I want:
    1. I want to be able to have buffers with:
        a. matrices of different dimensionalities, dimensions and object sizes:
            - one cell for a simple struct buffer
            - vectors of floats, ints, complex, etc.
            - vectors of more complex objects
            - matrices of the above with 1 dimension, 2, 3, etc.
        b. images (specifically rgb data), those will also be (I think) framebuffers and the sorts
        c. not a category, but different types of buffers, ubo's and normal in/outs
    2. Semaphores and Fences with clear linkages with objects (buffer - shaders, etc)
    3. Shaders, here, I need to have a way to describe what inputs I can have maybe? Or maybe a way
    to describe a computation pipeline
    4. User defined steps, for example moments the user needs to fill in data to be drawn, or moment
    the user sets ubos
    5. End point of drawing step

    (I should create a json in the way I want it to look, maybe in this way I figure the thing
    better)
 */

// struct vulkan_instance_t {
//     std::string filename;
//     vku_opts_t vku_opts;    /*! json object: opts */  
// };


void skelet_for_ingrid() {
    /* This is a list for all that I need for the vulkan/ingrid part: */

    /* A way to init/uninit dma memory that is common with the CS board */
    /* A generic way of having matrices, vectors and computation zones (that is covered by
    in locations and out locations, I guess) */
    /* A way of doing fft */

    /* (TODO: if possible) check if I can implement a recovery mechanism 
        see: MPK (Memory Protection Keys)
        see: sigsegv and signal()
        see: setjmp, longjmp...
        see: dlmopen */

    /* Do this for all the computation zones */
    // auto part_x = vku_spirv_compile(COMPUTE, compute_src);

    /* Also I need a way to load all those things and maybe a loadable dll, like the one we had,
    that would  */


    /*

    So the steps are:
        1. Write code to have those objects created from a json
        1. Test the way dma allocations work, fft, maybe also how that MPK thing works
        2. Write a test code with all those things, first without CS
        3. Integrate CS two
        4. Write the shaders That would make the thing work like the current one
        5. Replace current implementation with this new one
        6. Test for speed and check AGAIN if that buff bug is still there (this would mean it is
        from the gpu driver directly)
    */
}



#endif
