#ifndef VULKAN_CONFIG_H
#define VULKAN_CONFIG_H

/*! @file This file will be used to auto-initialize Vulkan. This file will describe the structure
 * of the Vulkan objects and create an vulkan_obj_t with the specific type and an associated name.
 * In this way you can configure all the semaphores and buffers from here and ask for them inside
 * the program. The objective is to have the entire Vulkan pipeline described from outside of the
 * source code. */

#include "vulkan_utils.h"

struct vulkan_instance_t {
    std::string filename;
    vku_opts_t vku_opts;    /*! json object: opts */
    
};

#endif
