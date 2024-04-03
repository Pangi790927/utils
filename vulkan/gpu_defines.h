#ifndef GPU_DEFINES_H
#define GPU_DEFINES_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#define glfw_init                                       glfwInit
#define glfw_window_hint                                glfwWindowHint
#define glfw_create_window                              glfwCreateWindow
#define glfw_window_should_close                        glfwWindowShouldClose
#define glfw_poll_events                                glfwPollEvents
#define glfw_destroy_window                             glfwDestroyWindow
#define glfw_terminate                                  glfwTerminate
#define glfw_get_required_instance_extensions           glfwGetRequiredInstanceExtensions
#define glfw_get_framebuffer_size                       glfwGetFramebufferSize

#define vk_flags_t                                      VkFlags 
#define vk_bool32_t                                     VkBool32 
#define vk_device_size_t                                VkDeviceSize 
#define vk_sample_mask_t                                VkSampleMask 
#define vk_instance_t                                   VkInstance 
#define vk_physical_device_t                            VkPhysicalDevice 
#define vk_device_t                                     VkDevice 
#define vk_queue_t                                      VkQueue 
#define vk_semaphore_t                                  VkSemaphore 
#define vk_command_buffer_t                             VkCommandBuffer 
#define vk_fence_t                                      VkFence 
#define vk_device_memory_t                              VkDeviceMemory 
#define vk_buffer_t                                     VkBuffer 
#define vk_image_t                                      VkImage 
#define vk_event_t                                      VkEvent 
#define vk_query_pool_t                                 VkQueryPool 
#define vk_buffer_view_t                                VkBufferView 
#define vk_image_view_t                                 VkImageView 
#define vk_shader_module_t                              VkShaderModule 
#define vk_pipeline_cache_t                             VkPipelineCache 
#define vk_pipeline_layout_t                            VkPipelineLayout 
#define vk_render_pass_t                                VkRenderPass 
#define vk_pipeline_t                                   VkPipeline 
#define vk_descriptor_set_layout_t                      VkDescriptorSetLayout 
#define vk_sampler_t                                    VkSampler 
#define vk_descriptor_pool_t                            VkDescriptorPool 
#define vk_descriptor_set_t                             VkDescriptorSet 
#define vk_framebuffer_t                                VkFramebuffer 
#define vk_command_pool_t                               VkCommandPool 
#define vk_pipeline_cache_header_version_t              VkPipelineCacheHeaderVersion
#define vk_result_t                                     VkResult
#define vk_structure_type_t                             VkStructureType
#define vk_system_allocation_scope_t                    VkSystemAllocationScope
#define vk_internal_allocation_type_t                   VkInternalAllocationType
#define vk_format_t                                     VkFormat
#define vk_image_type_t                                 VkImageType
#define vk_image_tiling_t                               VkImageTiling
#define vk_physical_device_type_t                       VkPhysicalDeviceType
#define vk_query_type_t                                 VkQueryType
#define vk_sharing_mode_t                               VkSharingMode
#define vk_image_layout_t                               VkImageLayout
#define vk_image_view_type_t                            VkImageViewType
#define vk_component_swizzle_t                          VkComponentSwizzle
#define vk_vertex_input_rate_t                          VkVertexInputRate
#define vk_primitive_topology_t                         VkPrimitiveTopology
#define vk_polygon_mode_t                               VkPolygonMode
#define vk_front_face_t                                 VkFrontFace
#define vk_compare_op_t                                 VkCompareOp
#define vk_stencil_op_t                                 VkStencilOp
#define vk_logic_op_t                                   VkLogicOp
#define vk_blend_factor_t                               VkBlendFactor
#define vk_blend_op_t                                   VkBlendOp
#define vk_dynamic_state_t                              VkDynamicState
#define vk_filter_t                                     VkFilter
#define vk_sampler_mipmap_mode_t                        VkSamplerMipmapMode
#define vk_sampler_address_mode_t                       VkSamplerAddressMode
#define vk_border_color_t                               VkBorderColor
#define vk_descriptor_type_t                            VkDescriptorType
#define vk_attachment_load_op_t                         VkAttachmentLoadOp
#define vk_attachment_store_op_t                        VkAttachmentStoreOp
#define vk_pipeline_bind_point_t                        VkPipelineBindPoint
#define vk_command_buffer_level_t                       VkCommandBufferLevel
#define vk_index_type_t                                 VkIndexType
#define vk_subpass_contents_t                           VkSubpassContents
#define vk_object_type_t                                VkObjectType
#define vk_vendor_id_t                                  VkVendorId
#define vk_instance_create_flags_t                      VkInstanceCreateFlags 
#define vk_format_feature_flag_bits_t                   VkFormatFeatureFlagBits
#define vk_format_feature_flags_t                       VkFormatFeatureFlags 
#define vk_image_usage_flag_bits_t                      VkImageUsageFlagBits
#define vk_image_usage_flags_t                          VkImageUsageFlags 
#define vk_image_create_flag_bits_t                     VkImageCreateFlagBits
#define vk_image_create_flags_t                         VkImageCreateFlags 
#define vk_sample_count_flag_bits_t                     VkSampleCountFlagBits
#define vk_sample_count_flags_t                         VkSampleCountFlags 
#define vk_queue_flag_bits_t                            VkQueueFlagBits
#define vk_queue_flags_t                                VkQueueFlags 
#define vk_memory_property_flag_bits_t                  VkMemoryPropertyFlagBits
#define vk_memory_property_flags_t                      VkMemoryPropertyFlags 
#define vk_memory_heap_flag_bits_t                      VkMemoryHeapFlagBits
#define vk_memory_heap_flags_t                          VkMemoryHeapFlags 
#define vk_device_create_flags_t                        VkDeviceCreateFlags 
#define vk_device_queue_create_flag_bits_t              VkDeviceQueueCreateFlagBits
#define vk_device_queue_create_flags_t                  VkDeviceQueueCreateFlags 
#define vk_pipeline_stage_flag_bits_t                   VkPipelineStageFlagBits
#define vk_pipeline_stage_flags_t                       VkPipelineStageFlags 
#define vk_memory_map_flags_t                           VkMemoryMapFlags 
#define vk_image_aspect_flag_bits_t                     VkImageAspectFlagBits
#define vk_image_aspect_flags_t                         VkImageAspectFlags 
#define vk_sparse_image_format_flag_bits_t              VkSparseImageFormatFlagBits
#define vk_sparse_image_format_flags_t                  VkSparseImageFormatFlags 
#define vk_sparse_memory_bind_flag_bits_t               VkSparseMemoryBindFlagBits
#define vk_sparse_memory_bind_flags_t                   VkSparseMemoryBindFlags 
#define vk_fence_create_flag_bits_t                     VkFenceCreateFlagBits
#define vk_fence_create_flags_t                         VkFenceCreateFlags 
#define vk_semaphore_create_flags_t                     VkSemaphoreCreateFlags 
#define vk_event_create_flags_t                         VkEventCreateFlags 
#define vk_query_pool_create_flags_t                    VkQueryPoolCreateFlags 
#define vk_query_pipeline_statistic_flag_bits_t         VkQueryPipelineStatisticFlagBits
#define vk_query_pipeline_statistic_flags_t             VkQueryPipelineStatisticFlags 
#define vk_query_result_flag_bits_t                     VkQueryResultFlagBits
#define vk_query_result_flags_t                         VkQueryResultFlags 
#define vk_buffer_create_flag_bits_t                    VkBufferCreateFlagBits
#define vk_buffer_create_flags_t                        VkBufferCreateFlags 
#define vk_buffer_usage_flag_bits_t                     VkBufferUsageFlagBits
#define vk_buffer_usage_flags_t                         VkBufferUsageFlags 
#define vk_buffer_view_create_flags_t                   VkBufferViewCreateFlags 
#define vk_image_view_create_flag_bits_t                VkImageViewCreateFlagBits
#define vk_image_view_create_flags_t                    VkImageViewCreateFlags 
#define vk_shader_module_create_flag_bits_t             VkShaderModuleCreateFlagBits
#define vk_shader_module_create_flags_t                 VkShaderModuleCreateFlags 
#define vk_pipeline_cache_create_flags_t                VkPipelineCacheCreateFlags 
#define vk_pipeline_create_flag_bits_t                  VkPipelineCreateFlagBits
#define vk_pipeline_create_flags_t                      VkPipelineCreateFlags 
#define vk_pipeline_shader_stage_create_flag_bits_t     VkPipelineShaderStageCreateFlagBits
#define vk_pipeline_shader_stage_create_flags_t         VkPipelineShaderStageCreateFlags 
#define vk_shader_stage_flag_bits_t                     VkShaderStageFlagBits
#define vk_pipeline_vertex_input_state_create_flags_t   VkPipelineVertexInputStateCreateFlags 
#define vk_pipeline_input_assembly_state_create_flags_t VkPipelineInputAssemblyStateCreateFlags 
#define vk_pipeline_tessellation_state_create_flags_t   VkPipelineTessellationStateCreateFlags 
#define vk_pipeline_viewport_state_create_flags_t       VkPipelineViewportStateCreateFlags 
#define vk_pipeline_rasterization_state_create_flags_t  VkPipelineRasterizationStateCreateFlags 
#define vk_cull_mode_flag_bits_t                        VkCullModeFlagBits
#define vk_cull_mode_flags_t                            VkCullModeFlags 
#define vk_pipeline_multisample_state_create_flags_t    VkPipelineMultisampleStateCreateFlags 
#define vk_pipeline_depth_stencil_state_create_flags_t  VkPipelineDepthStencilStateCreateFlags 
#define vk_pipeline_color_blend_state_create_flags_t    VkPipelineColorBlendStateCreateFlags 
#define vk_color_component_flag_bits_t                  VkColorComponentFlagBits
#define vk_color_component_flags_t                      VkColorComponentFlags 
#define vk_pipeline_dynamic_state_create_flags_t        VkPipelineDynamicStateCreateFlags 
#define vk_pipeline_layout_create_flags_t               VkPipelineLayoutCreateFlags 
#define vk_shader_stage_flags_t                         VkShaderStageFlags 
#define vk_sampler_create_flag_bits_t                   VkSamplerCreateFlagBits
#define vk_sampler_create_flags_t                       VkSamplerCreateFlags 
#define vk_descriptor_set_layout_create_flag_bits_t     VkDescriptorSetLayoutCreateFlagBits
#define vk_descriptor_set_layout_create_flags_t         VkDescriptorSetLayoutCreateFlags 
#define vk_descriptor_pool_create_flag_bits_t           VkDescriptorPoolCreateFlagBits
#define vk_descriptor_pool_create_flags_t               VkDescriptorPoolCreateFlags 
#define vk_descriptor_pool_reset_flags_t                VkDescriptorPoolResetFlags 
#define vk_framebuffer_create_flag_bits_t               VkFramebufferCreateFlagBits
#define vk_framebuffer_create_flags_t                   VkFramebufferCreateFlags 
#define vk_render_pass_create_flag_bits_t               VkRenderPassCreateFlagBits
#define vk_render_pass_create_flags_t                   VkRenderPassCreateFlags 
#define vk_attachment_description_flag_bits_t           VkAttachmentDescriptionFlagBits
#define vk_attachment_description_flags_t               VkAttachmentDescriptionFlags 
#define vk_subpass_description_flag_bits_t              VkSubpassDescriptionFlagBits
#define vk_subpass_description_flags_t                  VkSubpassDescriptionFlags 
#define vk_access_flag_bits_t                           VkAccessFlagBits
#define vk_access_flags_t                               VkAccessFlags 
#define vk_dependency_flag_bits_t                       VkDependencyFlagBits
#define vk_dependency_flags_t                           VkDependencyFlags 
#define vk_command_pool_create_flag_bits_t              VkCommandPoolCreateFlagBits
#define vk_command_pool_create_flags_t                  VkCommandPoolCreateFlags 
#define vk_command_pool_reset_flag_bits_t               VkCommandPoolResetFlagBits
#define vk_command_pool_reset_flags_t                   VkCommandPoolResetFlags 
#define vk_command_buffer_usage_flag_bits_t             VkCommandBufferUsageFlagBits
#define vk_command_buffer_usage_flags_t                 VkCommandBufferUsageFlags 
#define vk_query_control_flag_bits_t                    VkQueryControlFlagBits
#define vk_query_control_flags_t                        VkQueryControlFlags 
#define vk_command_buffer_reset_flag_bits_t             VkCommandBufferResetFlagBits
#define vk_command_buffer_reset_flags_t                 VkCommandBufferResetFlags 
#define vk_stencil_face_flag_bits_t                     VkStencilFaceFlagBits
#define vk_stencil_face_flags_t                         VkStencilFaceFlags 

typedef struct vk_application_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_APPLICATION_INFO;
    const void* p_next =                               NULL;
    const char*                                        p_application_name;
    uint32_t                                           application_version;
    const char*                                        p_engine_name;
    uint32_t                                           engine_version;
    uint32_t                                           api_version;
} vk_application_info_t;

typedef struct vk_instance_create_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    const void* p_next =                               NULL;
    vk_instance_create_flags_t                         flags;
    const vk_application_info_t*                       p_application_info;
    uint32_t                                           enabled_layer_count;
    const char* const*                                 pp_enabled_layer_names;
    uint32_t                                           enabled_extension_count;
    const char* const*                                 pp_enabled_extension_names;
} vk_instance_create_info_t;
#define pfn_vk_allocation_function_fn_t                                   PFN_vkAllocationFunction
#define pfn_vk_reallocation_function_fn_t                                 PFN_vkReallocationFunction
#define pfn_vk_free_function_fn_t                                         PFN_vkFreeFunction
#define pfn_vk_internal_allocation_notification_fn_t                      PFN_vkInternalAllocationNotification
#define pfn_vk_internal_free_notification_fn_t                            PFN_vkInternalFreeNotification

typedef struct vk_allocation_callbacks_t {
    void*                                              p_user_data;
    pfn_vk_allocation_function_fn_t                    pfn_allocation;
    pfn_vk_reallocation_function_fn_t                  pfn_reallocation;
    pfn_vk_free_function_fn_t                          pfn_free;
    pfn_vk_internal_allocation_notification_fn_t       pfn_internal_allocation;
    pfn_vk_internal_free_notification_fn_t             pfn_internal_free;
} vk_allocation_callbacks_t;

typedef struct vk_physical_device_features_t {
    vk_bool32_t                                        robust_buffer_access;
    vk_bool32_t                                        full_draw_index_uint32;
    vk_bool32_t                                        image_cube_array;
    vk_bool32_t                                        independent_blend;
    vk_bool32_t                                        geometry_shader;
    vk_bool32_t                                        tessellation_shader;
    vk_bool32_t                                        sample_rate_shading;
    vk_bool32_t                                        dual_src_blend;
    vk_bool32_t                                        logic_op;
    vk_bool32_t                                        multi_draw_indirect;
    vk_bool32_t                                        draw_indirect_first_instance;
    vk_bool32_t                                        depth_clamp;
    vk_bool32_t                                        depth_bias_clamp;
    vk_bool32_t                                        fill_mode_non_solid;
    vk_bool32_t                                        depth_bounds;
    vk_bool32_t                                        wide_lines;
    vk_bool32_t                                        large_points;
    vk_bool32_t                                        alpha_to_one;
    vk_bool32_t                                        multi_viewport;
    vk_bool32_t                                        sampler_anisotropy;
    vk_bool32_t                                        texture_compression_etc2;
    vk_bool32_t                                        texture_compression_astc_ldr;
    vk_bool32_t                                        texture_compression_bc;
    vk_bool32_t                                        occlusion_query_precise;
    vk_bool32_t                                        pipeline_statistics_query;
    vk_bool32_t                                        vertex_pipeline_stores_and_atomics;
    vk_bool32_t                                        fragment_stores_and_atomics;
    vk_bool32_t                                        shader_tessellation_and_geometry_point_size;
    vk_bool32_t                                        shader_image_gather_extended;
    vk_bool32_t                                        shader_storage_image_extended_formats;
    vk_bool32_t                                        shader_storage_image_multisample;
    vk_bool32_t                                        shader_storage_image_read_without_format;
    vk_bool32_t                                        shader_storage_image_write_without_format;
    vk_bool32_t                                        shader_uniform_buffer_array_dynamic_indexing;
    vk_bool32_t                                        shader_sampled_image_array_dynamic_indexing;
    vk_bool32_t                                        shader_storage_buffer_array_dynamic_indexing;
    vk_bool32_t                                        shader_storage_image_array_dynamic_indexing;
    vk_bool32_t                                        shader_clip_distance;
    vk_bool32_t                                        shader_cull_distance;
    vk_bool32_t                                        shader_float64;
    vk_bool32_t                                        shader_int64;
    vk_bool32_t                                        shader_int16;
    vk_bool32_t                                        shader_resource_residency;
    vk_bool32_t                                        shader_resource_min_lod;
    vk_bool32_t                                        sparse_binding;
    vk_bool32_t                                        sparse_residency_buffer;
    vk_bool32_t                                        sparse_residency_image2_d;
    vk_bool32_t                                        sparse_residency_image3_d;
    vk_bool32_t                                        sparse_residency2_samples;
    vk_bool32_t                                        sparse_residency4_samples;
    vk_bool32_t                                        sparse_residency8_samples;
    vk_bool32_t                                        sparse_residency16_samples;
    vk_bool32_t                                        sparse_residency_aliased;
    vk_bool32_t                                        variable_multisample_rate;
    vk_bool32_t                                        inherited_queries;
} vk_physical_device_features_t;

typedef struct vk_format_properties_t {
    vk_format_feature_flags_t                          linear_tiling_features;
    vk_format_feature_flags_t                          optimal_tiling_features;
    vk_format_feature_flags_t                          buffer_features;
} vk_format_properties_t;

typedef struct vk_extent3_d_t {
    uint32_t                                           width;
    uint32_t                                           height;
    uint32_t                                           depth;
} vk_extent3_d_t;

typedef struct vk_image_format_properties_t {
    vk_extent3_d_t                                     max_extent;
    uint32_t                                           max_mip_levels;
    uint32_t                                           max_array_layers;
    vk_sample_count_flags_t                            sample_counts;
    vk_device_size_t                                   max_resource_size;
} vk_image_format_properties_t;

typedef struct vk_physical_device_limits_t {
    uint32_t                                           max_image_dimension1_d;
    uint32_t                                           max_image_dimension2_d;
    uint32_t                                           max_image_dimension3_d;
    uint32_t                                           max_image_dimension_cube;
    uint32_t                                           max_image_array_layers;
    uint32_t                                           max_texel_buffer_elements;
    uint32_t                                           max_uniform_buffer_range;
    uint32_t                                           max_storage_buffer_range;
    uint32_t                                           max_push_constants_size;
    uint32_t                                           max_memory_allocation_count;
    uint32_t                                           max_sampler_allocation_count;
    vk_device_size_t                                   buffer_image_granularity;
    vk_device_size_t                                   sparse_address_space_size;
    uint32_t                                           max_bound_descriptor_sets;
    uint32_t                                           max_per_stage_descriptor_samplers;
    uint32_t                                           max_per_stage_descriptor_uniform_buffers;
    uint32_t                                           max_per_stage_descriptor_storage_buffers;
    uint32_t                                           max_per_stage_descriptor_sampled_images;
    uint32_t                                           max_per_stage_descriptor_storage_images;
    uint32_t                                           max_per_stage_descriptor_input_attachments;
    uint32_t                                           max_per_stage_resources;
    uint32_t                                           max_descriptor_set_samplers;
    uint32_t                                           max_descriptor_set_uniform_buffers;
    uint32_t                                           max_descriptor_set_uniform_buffers_dynamic;
    uint32_t                                           max_descriptor_set_storage_buffers;
    uint32_t                                           max_descriptor_set_storage_buffers_dynamic;
    uint32_t                                           max_descriptor_set_sampled_images;
    uint32_t                                           max_descriptor_set_storage_images;
    uint32_t                                           max_descriptor_set_input_attachments;
    uint32_t                                           max_vertex_input_attributes;
    uint32_t                                           max_vertex_input_bindings;
    uint32_t                                           max_vertex_input_attribute_offset;
    uint32_t                                           max_vertex_input_binding_stride;
    uint32_t                                           max_vertex_output_components;
    uint32_t                                           max_tessellation_generation_level;
    uint32_t                                           max_tessellation_patch_size;
    uint32_t                                           max_tessellation_control_per_vertex_input_components;
    uint32_t                                           max_tessellation_control_per_vertex_output_components;
    uint32_t                                           max_tessellation_control_per_patch_output_components;
    uint32_t                                           max_tessellation_control_total_output_components;
    uint32_t                                           max_tessellation_evaluation_input_components;
    uint32_t                                           max_tessellation_evaluation_output_components;
    uint32_t                                           max_geometry_shader_invocations;
    uint32_t                                           max_geometry_input_components;
    uint32_t                                           max_geometry_output_components;
    uint32_t                                           max_geometry_output_vertices;
    uint32_t                                           max_geometry_total_output_components;
    uint32_t                                           max_fragment_input_components;
    uint32_t                                           max_fragment_output_attachments;
    uint32_t                                           max_fragment_dual_src_attachments;
    uint32_t                                           max_fragment_combined_output_resources;
    uint32_t                                           max_compute_shared_memory_size;
    uint32_t                                           max_compute_work_group_count[3];
    uint32_t                                           max_compute_work_group_invocations;
    uint32_t                                           max_compute_work_group_size[3];
    uint32_t                                           sub_pixel_precision_bits;
    uint32_t                                           sub_texel_precision_bits;
    uint32_t                                           mipmap_precision_bits;
    uint32_t                                           max_draw_indexed_index_value;
    uint32_t                                           max_draw_indirect_count;
    float                                              max_sampler_lod_bias;
    float                                              max_sampler_anisotropy;
    uint32_t                                           max_viewports;
    uint32_t                                           max_viewport_dimensions[2];
    float                                              viewport_bounds_range[2];
    uint32_t                                           viewport_sub_pixel_bits;
    size_t                                             min_memory_map_alignment;
    vk_device_size_t                                   min_texel_buffer_offset_alignment;
    vk_device_size_t                                   min_uniform_buffer_offset_alignment;
    vk_device_size_t                                   min_storage_buffer_offset_alignment;
    int32_t                                            min_texel_offset;
    uint32_t                                           max_texel_offset;
    int32_t                                            min_texel_gather_offset;
    uint32_t                                           max_texel_gather_offset;
    float                                              min_interpolation_offset;
    float                                              max_interpolation_offset;
    uint32_t                                           sub_pixel_interpolation_offset_bits;
    uint32_t                                           max_framebuffer_width;
    uint32_t                                           max_framebuffer_height;
    uint32_t                                           max_framebuffer_layers;
    vk_sample_count_flags_t                            framebuffer_color_sample_counts;
    vk_sample_count_flags_t                            framebuffer_depth_sample_counts;
    vk_sample_count_flags_t                            framebuffer_stencil_sample_counts;
    vk_sample_count_flags_t                            framebuffer_no_attachments_sample_counts;
    uint32_t                                           max_color_attachments;
    vk_sample_count_flags_t                            sampled_image_color_sample_counts;
    vk_sample_count_flags_t                            sampled_image_integer_sample_counts;
    vk_sample_count_flags_t                            sampled_image_depth_sample_counts;
    vk_sample_count_flags_t                            sampled_image_stencil_sample_counts;
    vk_sample_count_flags_t                            storage_image_sample_counts;
    uint32_t                                           max_sample_mask_words;
    vk_bool32_t                                        timestamp_compute_and_graphics;
    float                                              timestamp_period;
    uint32_t                                           max_clip_distances;
    uint32_t                                           max_cull_distances;
    uint32_t                                           max_combined_clip_and_cull_distances;
    uint32_t                                           discrete_queue_priorities;
    float                                              point_size_range[2];
    float                                              line_width_range[2];
    float                                              point_size_granularity;
    float                                              line_width_granularity;
    vk_bool32_t                                        strict_lines;
    vk_bool32_t                                        standard_sample_locations;
    vk_device_size_t                                   optimal_buffer_copy_offset_alignment;
    vk_device_size_t                                   optimal_buffer_copy_row_pitch_alignment;
    vk_device_size_t                                   non_coherent_atom_size;
} vk_physical_device_limits_t;

typedef struct vk_physical_device_sparse_properties_t {
    vk_bool32_t                                        residency_standard2_d_block_shape;
    vk_bool32_t                                        residency_standard2_d_multisample_block_shape;
    vk_bool32_t                                        residency_standard3_d_block_shape;
    vk_bool32_t                                        residency_aligned_mip_size;
    vk_bool32_t                                        residency_non_resident_strict;
} vk_physical_device_sparse_properties_t;

typedef struct vk_physical_device_properties_t {
    uint32_t                                           api_version;
    uint32_t                                           driver_version;
    uint32_t                                           vendor_id;
    uint32_t                                           device_id;
    vk_physical_device_type_t                          device_type;
    char                                               device_name[VK_MAX_PHYSICAL_DEVICE_NAME_SIZE];
    uint8_t                                            pipeline_cache_uuid[VK_UUID_SIZE];
    vk_physical_device_limits_t                        limits;
    vk_physical_device_sparse_properties_t             sparse_properties;
} vk_physical_device_properties_t;

typedef struct vk_queue_family_properties_t {
    vk_queue_flags_t                                   queue_flags;
    uint32_t                                           queue_count;
    uint32_t                                           timestamp_valid_bits;
    vk_extent3_d_t                                     min_image_transfer_granularity;
} vk_queue_family_properties_t;

typedef struct vk_memory_type_t {
    vk_memory_property_flags_t                         property_flags;
    uint32_t                                           heap_index;
} vk_memory_type_t;

typedef struct vk_memory_heap_t {
    vk_device_size_t                                   size;
    vk_memory_heap_flags_t                             flags;
} vk_memory_heap_t;

typedef struct vk_physical_device_memory_properties_t {
    uint32_t                                           memory_type_count;
    vk_memory_type_t                                   memory_types[VK_MAX_MEMORY_TYPES];
    uint32_t                                           memory_heap_count;
    vk_memory_heap_t                                   memory_heaps[VK_MAX_MEMORY_HEAPS];
} vk_physical_device_memory_properties_t;
#define pfn_vk_void_function_fn_t                                         PFN_vkVoidFunction

typedef struct vk_device_queue_create_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    const void* p_next =                               NULL;
    vk_device_queue_create_flags_t                     flags;
    uint32_t                                           queue_family_index;
    uint32_t                                           queue_count;
    const float*                                       p_queue_priorities;
} vk_device_queue_create_info_t;

typedef struct vk_device_create_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    const void* p_next =                               NULL;
    vk_device_create_flags_t                           flags;
    uint32_t                                           queue_create_info_count;
    const vk_device_queue_create_info_t*               p_queue_create_infos;
    uint32_t                                           enabled_layer_count;
    const char* const*                                 pp_enabled_layer_names;
    uint32_t                                           enabled_extension_count;
    const char* const*                                 pp_enabled_extension_names;
    const vk_physical_device_features_t*               p_enabled_features;
} vk_device_create_info_t;

typedef struct vk_extension_properties_t {
    char                                               extension_name[VK_MAX_EXTENSION_NAME_SIZE];
    uint32_t                                           spec_version;
} vk_extension_properties_t;

typedef struct vk_layer_properties_t {
    char                                               layer_name[VK_MAX_EXTENSION_NAME_SIZE];
    uint32_t                                           spec_version;
    uint32_t                                           implementation_version;
    char                                               description[VK_MAX_DESCRIPTION_SIZE];
} vk_layer_properties_t;

typedef struct vk_submit_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_SUBMIT_INFO;
    const void* p_next =                               NULL;
    uint32_t                                           wait_semaphore_count;
    const vk_semaphore_t*                              p_wait_semaphores;
    const vk_pipeline_stage_flags_t*                   p_wait_dst_stage_mask;
    uint32_t                                           command_buffer_count;
    const vk_command_buffer_t*                         p_command_buffers;
    uint32_t                                           signal_semaphore_count;
    const vk_semaphore_t*                              p_signal_semaphores;
} vk_submit_info_t;

typedef struct vk_memory_allocate_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    const void* p_next =                               NULL;
    vk_device_size_t                                   allocation_size;
    uint32_t                                           memory_type_index;
} vk_memory_allocate_info_t;

typedef struct vk_mapped_memory_range_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    const void* p_next =                               NULL;
    vk_device_memory_t                                 memory;
    vk_device_size_t                                   offset;
    vk_device_size_t                                   size;
} vk_mapped_memory_range_t;

typedef struct vk_memory_requirements_t {
    vk_device_size_t                                   size;
    vk_device_size_t                                   alignment;
    uint32_t                                           memory_type_bits;
} vk_memory_requirements_t;

typedef struct vk_sparse_image_format_properties_t {
    vk_image_aspect_flags_t                            aspect_mask;
    vk_extent3_d_t                                     image_granularity;
    vk_sparse_image_format_flags_t                     flags;
} vk_sparse_image_format_properties_t;

typedef struct vk_sparse_image_memory_requirements_t {
    vk_sparse_image_format_properties_t                format_properties;
    uint32_t                                           image_mip_tail_first_lod;
    vk_device_size_t                                   image_mip_tail_size;
    vk_device_size_t                                   image_mip_tail_offset;
    vk_device_size_t                                   image_mip_tail_stride;
} vk_sparse_image_memory_requirements_t;

typedef struct vk_sparse_memory_bind_t {
    vk_device_size_t                                   resource_offset;
    vk_device_size_t                                   size;
    vk_device_memory_t                                 memory;
    vk_device_size_t                                   memory_offset;
    vk_sparse_memory_bind_flags_t                      flags;
} vk_sparse_memory_bind_t;

typedef struct vk_sparse_buffer_memory_bind_info_t {
    vk_buffer_t                                        buffer;
    uint32_t                                           bind_count;
    const vk_sparse_memory_bind_t*                     p_binds;
} vk_sparse_buffer_memory_bind_info_t;

typedef struct vk_sparse_image_opaque_memory_bind_info_t {
    vk_image_t                                         image;
    uint32_t                                           bind_count;
    const vk_sparse_memory_bind_t*                     p_binds;
} vk_sparse_image_opaque_memory_bind_info_t;

typedef struct vk_image_subresource_t {
    vk_image_aspect_flags_t                            aspect_mask;
    uint32_t                                           mip_level;
    uint32_t                                           array_layer;
} vk_image_subresource_t;

typedef struct vk_offset3_d_t {
    int32_t                                            x;
    int32_t                                            y;
    int32_t                                            z;
} vk_offset3_d_t;

typedef struct vk_sparse_image_memory_bind_t {
    vk_image_subresource_t                             subresource;
    vk_offset3_d_t                                     offset;
    vk_extent3_d_t                                     extent;
    vk_device_memory_t                                 memory;
    vk_device_size_t                                   memory_offset;
    vk_sparse_memory_bind_flags_t                      flags;
} vk_sparse_image_memory_bind_t;

typedef struct vk_sparse_image_memory_bind_info_t {
    vk_image_t                                         image;
    uint32_t                                           bind_count;
    const vk_sparse_image_memory_bind_t*               p_binds;
} vk_sparse_image_memory_bind_info_t;

typedef struct vk_bind_sparse_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_BIND_SPARSE_INFO;
    const void* p_next =                               NULL;
    uint32_t                                           wait_semaphore_count;
    const vk_semaphore_t*                              p_wait_semaphores;
    uint32_t                                           buffer_bind_count;
    const vk_sparse_buffer_memory_bind_info_t*         p_buffer_binds;
    uint32_t                                           image_opaque_bind_count;
    const vk_sparse_image_opaque_memory_bind_info_t*   p_image_opaque_binds;
    uint32_t                                           image_bind_count;
    const vk_sparse_image_memory_bind_info_t*          p_image_binds;
    uint32_t                                           signal_semaphore_count;
    const vk_semaphore_t*                              p_signal_semaphores;
} vk_bind_sparse_info_t;

typedef struct vk_fence_create_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    const void* p_next =                               NULL;
    vk_fence_create_flags_t                            flags;
} vk_fence_create_info_t;

typedef struct vk_semaphore_create_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    const void* p_next =                               NULL;
    vk_semaphore_create_flags_t                        flags;
} vk_semaphore_create_info_t;

typedef struct vk_event_create_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
    const void* p_next =                               NULL;
    vk_event_create_flags_t                            flags;
} vk_event_create_info_t;

typedef struct vk_query_pool_create_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    const void* p_next =                               NULL;
    vk_query_pool_create_flags_t                       flags;
    vk_query_type_t                                    query_type;
    uint32_t                                           query_count;
    vk_query_pipeline_statistic_flags_t                pipeline_statistics;
} vk_query_pool_create_info_t;

typedef struct vk_buffer_create_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    const void* p_next =                               NULL;
    vk_buffer_create_flags_t                           flags;
    vk_device_size_t                                   size;
    vk_buffer_usage_flags_t                            usage;
    vk_sharing_mode_t                                  sharing_mode;
    uint32_t                                           queue_family_index_count;
    const uint32_t*                                    p_queue_family_indices;
} vk_buffer_create_info_t;

typedef struct vk_buffer_view_create_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
    const void* p_next =                               NULL;
    vk_buffer_view_create_flags_t                      flags;
    vk_buffer_t                                        buffer;
    vk_format_t                                        format;
    vk_device_size_t                                   offset;
    vk_device_size_t                                   range;
} vk_buffer_view_create_info_t;

typedef struct vk_image_create_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    const void* p_next =                               NULL;
    vk_image_create_flags_t                            flags;
    vk_image_type_t                                    image_type;
    vk_format_t                                        format;
    vk_extent3_d_t                                     extent;
    uint32_t                                           mip_levels;
    uint32_t                                           array_layers;
    vk_sample_count_flag_bits_t                        samples;
    vk_image_tiling_t                                  tiling;
    vk_image_usage_flags_t                             usage;
    vk_sharing_mode_t                                  sharing_mode;
    uint32_t                                           queue_family_index_count;
    const uint32_t*                                    p_queue_family_indices;
    vk_image_layout_t                                  initial_layout;
} vk_image_create_info_t;

typedef struct vk_subresource_layout_t {
    vk_device_size_t                                   offset;
    vk_device_size_t                                   size;
    vk_device_size_t                                   row_pitch;
    vk_device_size_t                                   array_pitch;
    vk_device_size_t                                   depth_pitch;
} vk_subresource_layout_t;

typedef struct vk_component_mapping_t {
    vk_component_swizzle_t                             r;
    vk_component_swizzle_t                             g;
    vk_component_swizzle_t                             b;
    vk_component_swizzle_t                             a;
} vk_component_mapping_t;

typedef struct vk_image_subresource_range_t {
    vk_image_aspect_flags_t                            aspect_mask;
    uint32_t                                           base_mip_level;
    uint32_t                                           level_count;
    uint32_t                                           base_array_layer;
    uint32_t                                           layer_count;
} vk_image_subresource_range_t;

typedef struct vk_image_view_create_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    const void* p_next =                               NULL;
    vk_image_view_create_flags_t                       flags;
    vk_image_t                                         image;
    vk_image_view_type_t                               view_type;
    vk_format_t                                        format;
    vk_component_mapping_t                             components;
    vk_image_subresource_range_t                       subresource_range;
} vk_image_view_create_info_t;

typedef struct vk_shader_module_create_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    const void* p_next =                               NULL;
    vk_shader_module_create_flags_t                    flags;
    size_t                                             code_size;
    const uint32_t*                                    p_code;
} vk_shader_module_create_info_t;

typedef struct vk_pipeline_cache_create_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    const void* p_next =                               NULL;
    vk_pipeline_cache_create_flags_t                   flags;
    size_t                                             initial_data_size;
    const void*                                        p_initial_data;
} vk_pipeline_cache_create_info_t;

typedef struct vk_specialization_map_entry_t {
    uint32_t                                           constant_id;
    uint32_t                                           offset;
    size_t                                             size;
} vk_specialization_map_entry_t;

typedef struct vk_specialization_info_t {
    uint32_t                                           map_entry_count;
    const vk_specialization_map_entry_t*               p_map_entries;
    size_t                                             data_size;
    const void*                                        p_data;
} vk_specialization_info_t;

typedef struct vk_pipeline_shader_stage_create_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    const void* p_next =                               NULL;
    vk_pipeline_shader_stage_create_flags_t            flags;
    vk_shader_stage_flag_bits_t                        stage;
    vk_shader_module_t                                 module;
    const char*                                        p_name;
    const vk_specialization_info_t*                    p_specialization_info;
} vk_pipeline_shader_stage_create_info_t;

typedef struct vk_vertex_input_binding_description_t {
    uint32_t                                           binding;
    uint32_t                                           stride;
    vk_vertex_input_rate_t                             input_rate;
} vk_vertex_input_binding_description_t;

typedef struct vk_vertex_input_attribute_description_t {
    uint32_t                                           location;
    uint32_t                                           binding;
    vk_format_t                                        format;
    uint32_t                                           offset;
} vk_vertex_input_attribute_description_t;

typedef struct vk_pipeline_vertex_input_state_create_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    const void* p_next =                               NULL;
    vk_pipeline_vertex_input_state_create_flags_t      flags;
    uint32_t                                           vertex_binding_description_count;
    const vk_vertex_input_binding_description_t*       p_vertex_binding_descriptions;
    uint32_t                                           vertex_attribute_description_count;
    const vk_vertex_input_attribute_description_t*     p_vertex_attribute_descriptions;
} vk_pipeline_vertex_input_state_create_info_t;

typedef struct vk_pipeline_input_assembly_state_create_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    const void* p_next =                               NULL;
    vk_pipeline_input_assembly_state_create_flags_t    flags;
    vk_primitive_topology_t                            topology;
    vk_bool32_t                                        primitive_restart_enable;
} vk_pipeline_input_assembly_state_create_info_t;

typedef struct vk_pipeline_tessellation_state_create_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
    const void* p_next =                               NULL;
    vk_pipeline_tessellation_state_create_flags_t      flags;
    uint32_t                                           patch_control_points;
} vk_pipeline_tessellation_state_create_info_t;

typedef struct vk_viewport_t {
    float                                              x;
    float                                              y;
    float                                              width;
    float                                              height;
    float                                              min_depth;
    float                                              max_depth;
} vk_viewport_t;

typedef struct vk_offset2_d_t {
    int32_t                                            x;
    int32_t                                            y;
} vk_offset2_d_t;

typedef struct vk_extent2d_t {
    uint32_t                                           width;
    uint32_t                                           height;
} vk_extent2d_t;

typedef struct vk_rect2d_t {
    vk_offset2_d_t                                     offset;
    vk_extent2d_t                                      extent;
} vk_rect2d_t;

typedef struct vk_pipeline_viewport_state_create_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    const void* p_next =                               NULL;
    vk_pipeline_viewport_state_create_flags_t          flags;
    uint32_t                                           viewport_count;
    const vk_viewport_t*                               p_viewports;
    uint32_t                                           scissor_count;
    const vk_rect2d_t*                                p_scissors;
} vk_pipeline_viewport_state_create_info_t;

typedef struct vk_pipeline_rasterization_state_create_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    const void* p_next =                               NULL;
    vk_pipeline_rasterization_state_create_flags_t     flags;
    vk_bool32_t                                        depth_clamp_enable;
    vk_bool32_t                                        rasterizer_discard_enable;
    vk_polygon_mode_t                                  polygon_mode;
    vk_cull_mode_flags_t                               cull_mode;
    vk_front_face_t                                    front_face;
    vk_bool32_t                                        depth_bias_enable;
    float                                              depth_bias_constant_factor;
    float                                              depth_bias_clamp;
    float                                              depth_bias_slope_factor;
    float                                              line_width;
} vk_pipeline_rasterization_state_create_info_t;

typedef struct vk_pipeline_multisample_state_create_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    const void* p_next =                               NULL;
    vk_pipeline_multisample_state_create_flags_t       flags;
    vk_sample_count_flag_bits_t                        rasterization_samples;
    vk_bool32_t                                        sample_shading_enable;
    float                                              min_sample_shading;
    const vk_sample_mask_t*                            p_sample_mask;
    vk_bool32_t                                        alpha_to_coverage_enable;
    vk_bool32_t                                        alpha_to_one_enable;
} vk_pipeline_multisample_state_create_info_t;

typedef struct vk_stencil_op_state_t {
    vk_stencil_op_t                                    fail_op;
    vk_stencil_op_t                                    pass_op;
    vk_stencil_op_t                                    depth_fail_op;
    vk_compare_op_t                                    compare_op;
    uint32_t                                           compare_mask;
    uint32_t                                           write_mask;
    uint32_t                                           reference;
} vk_stencil_op_state_t;

typedef struct vk_pipeline_depth_stencil_state_create_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    const void* p_next =                               NULL;
    vk_pipeline_depth_stencil_state_create_flags_t     flags;
    vk_bool32_t                                        depth_test_enable;
    vk_bool32_t                                        depth_write_enable;
    vk_compare_op_t                                    depth_compare_op;
    vk_bool32_t                                        depth_bounds_test_enable;
    vk_bool32_t                                        stencil_test_enable;
    vk_stencil_op_state_t                              front;
    vk_stencil_op_state_t                              back;
    float                                              min_depth_bounds;
    float                                              max_depth_bounds;
} vk_pipeline_depth_stencil_state_create_info_t;

typedef struct vk_pipeline_color_blend_attachment_state_t {
    vk_bool32_t                                        blend_enable;
    vk_blend_factor_t                                  src_color_blend_factor;
    vk_blend_factor_t                                  dst_color_blend_factor;
    vk_blend_op_t                                      color_blend_op;
    vk_blend_factor_t                                  src_alpha_blend_factor;
    vk_blend_factor_t                                  dst_alpha_blend_factor;
    vk_blend_op_t                                      alpha_blend_op;
    vk_color_component_flags_t                         color_write_mask;
} vk_pipeline_color_blend_attachment_state_t;

typedef struct vk_pipeline_color_blend_state_create_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    const void* p_next =                               NULL;
    vk_pipeline_color_blend_state_create_flags_t       flags;
    vk_bool32_t                                        logic_op_enable;
    vk_logic_op_t                                      logic_op;
    uint32_t                                           attachment_count;
    const vk_pipeline_color_blend_attachment_state_t*  p_attachments;
    float                                              blend_constants[4];
} vk_pipeline_color_blend_state_create_info_t;

typedef struct vk_pipeline_dynamic_state_create_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    const void* p_next =                               NULL;
    vk_pipeline_dynamic_state_create_flags_t           flags;
    uint32_t                                           dynamic_state_count;
    const vk_dynamic_state_t*                          p_dynamic_states;
} vk_pipeline_dynamic_state_create_info_t;

typedef struct vk_graphics_pipeline_create_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    const void* p_next =                               NULL;
    vk_pipeline_create_flags_t                         flags;
    uint32_t                                           stage_count;
    const vk_pipeline_shader_stage_create_info_t*      p_stages;
    const vk_pipeline_vertex_input_state_create_info_t* p_vertex_input_state;
    const vk_pipeline_input_assembly_state_create_info_t* p_input_assembly_state;
    const vk_pipeline_tessellation_state_create_info_t* p_tessellation_state;
    const vk_pipeline_viewport_state_create_info_t*    p_viewport_state;
    const vk_pipeline_rasterization_state_create_info_t* p_rasterization_state;
    const vk_pipeline_multisample_state_create_info_t* p_multisample_state;
    const vk_pipeline_depth_stencil_state_create_info_t* p_depth_stencil_state;
    const vk_pipeline_color_blend_state_create_info_t* p_color_blend_state;
    const vk_pipeline_dynamic_state_create_info_t*     p_dynamic_state;
    vk_pipeline_layout_t                               layout;
    vk_render_pass_t                                   render_pass;
    uint32_t                                           subpass;
    vk_pipeline_t                                      base_pipeline_handle;
    int32_t                                            base_pipeline_index;
} vk_graphics_pipeline_create_info_t;

typedef struct vk_compute_pipeline_create_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    const void* p_next =                               NULL;
    vk_pipeline_create_flags_t                         flags;
    vk_pipeline_shader_stage_create_info_t             stage;
    vk_pipeline_layout_t                               layout;
    vk_pipeline_t                                      base_pipeline_handle;
    int32_t                                            base_pipeline_index;
} vk_compute_pipeline_create_info_t;

typedef struct vk_push_constant_range_t {
    vk_shader_stage_flags_t                            stage_flags;
    uint32_t                                           offset;
    uint32_t                                           size;
} vk_push_constant_range_t;

typedef struct vk_pipeline_layout_create_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    const void* p_next =                               NULL;
    vk_pipeline_layout_create_flags_t                  flags;
    uint32_t                                           set_layout_count;
    const vk_descriptor_set_layout_t*                  p_set_layouts;
    uint32_t                                           push_constant_range_count;
    const vk_push_constant_range_t*                    p_push_constant_ranges;
} vk_pipeline_layout_create_info_t;

typedef struct vk_sampler_create_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    const void* p_next =                               NULL;
    vk_sampler_create_flags_t                          flags;
    vk_filter_t                                        mag_filter;
    vk_filter_t                                        min_filter;
    vk_sampler_mipmap_mode_t                           mipmap_mode;
    vk_sampler_address_mode_t                          address_mode_u;
    vk_sampler_address_mode_t                          address_mode_v;
    vk_sampler_address_mode_t                          address_mode_w;
    float                                              mip_lod_bias;
    vk_bool32_t                                        anisotropy_enable;
    float                                              max_anisotropy;
    vk_bool32_t                                        compare_enable;
    vk_compare_op_t                                    compare_op;
    float                                              min_lod;
    float                                              max_lod;
    vk_border_color_t                                  border_color;
    vk_bool32_t                                        unnormalized_coordinates;
} vk_sampler_create_info_t;

typedef struct vk_descriptor_set_layout_binding_t {
    uint32_t                                           binding;
    vk_descriptor_type_t                               descriptor_type;
    uint32_t                                           descriptor_count;
    vk_shader_stage_flags_t                            stage_flags;
    const vk_sampler_t*                                p_immutable_samplers;
} vk_descriptor_set_layout_binding_t;

typedef struct vk_descriptor_set_layout_create_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    const void* p_next =                               NULL;
    vk_descriptor_set_layout_create_flags_t            flags;
    uint32_t                                           binding_count;
    const vk_descriptor_set_layout_binding_t*          p_bindings;
} vk_descriptor_set_layout_create_info_t;

typedef struct vk_descriptor_pool_size_t {
    vk_descriptor_type_t                               type;
    uint32_t                                           descriptor_count;
} vk_descriptor_pool_size_t;

typedef struct vk_descriptor_pool_create_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    const void* p_next =                               NULL;
    vk_descriptor_pool_create_flags_t                  flags;
    uint32_t                                           max_sets;
    uint32_t                                           pool_size_count;
    const vk_descriptor_pool_size_t*                   p_pool_sizes;
} vk_descriptor_pool_create_info_t;

typedef struct vk_descriptor_set_allocate_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    const void* p_next =                               NULL;
    vk_descriptor_pool_t                               descriptor_pool;
    uint32_t                                           descriptor_set_count;
    const vk_descriptor_set_layout_t*                  p_set_layouts;
} vk_descriptor_set_allocate_info_t;

typedef struct vk_descriptor_image_info_t {
    vk_sampler_t                                       sampler;
    vk_image_view_t                                    image_view;
    vk_image_layout_t                                  image_layout;
} vk_descriptor_image_info_t;

typedef struct vk_descriptor_buffer_info_t {
    vk_buffer_t                                        buffer;
    vk_device_size_t                                   offset;
    vk_device_size_t                                   range;
} vk_descriptor_buffer_info_t;

typedef struct vk_write_descriptor_set_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    const void* p_next =                               NULL;
    vk_descriptor_set_t                                dst_set;
    uint32_t                                           dst_binding;
    uint32_t                                           dst_array_element;
    uint32_t                                           descriptor_count;
    vk_descriptor_type_t                               descriptor_type;
    const vk_descriptor_image_info_t*                  p_image_info;
    const vk_descriptor_buffer_info_t*                 p_buffer_info;
    const vk_buffer_view_t*                            p_texel_buffer_view;
} vk_write_descriptor_set_t;

typedef struct vk_copy_descriptor_set_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;
    const void* p_next =                               NULL;
    vk_descriptor_set_t                                src_set;
    uint32_t                                           src_binding;
    uint32_t                                           src_array_element;
    vk_descriptor_set_t                                dst_set;
    uint32_t                                           dst_binding;
    uint32_t                                           dst_array_element;
    uint32_t                                           descriptor_count;
} vk_copy_descriptor_set_t;

typedef struct vk_framebuffer_create_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    const void* p_next =                               NULL;
    vk_framebuffer_create_flags_t                      flags;
    vk_render_pass_t                                   render_pass;
    uint32_t                                           attachment_count;
    const vk_image_view_t*                             p_attachments;
    uint32_t                                           width;
    uint32_t                                           height;
    uint32_t                                           layers;
} vk_framebuffer_create_info_t;

typedef struct vk_attachment_description_t {
    vk_attachment_description_flags_t                  flags;
    vk_format_t                                        format;
    vk_sample_count_flag_bits_t                        samples;
    vk_attachment_load_op_t                            load_op;
    vk_attachment_store_op_t                           store_op;
    vk_attachment_load_op_t                            stencil_load_op;
    vk_attachment_store_op_t                           stencil_store_op;
    vk_image_layout_t                                  initial_layout;
    vk_image_layout_t                                  final_layout;
} vk_attachment_description_t;

typedef struct vk_attachment_reference_t {
    uint32_t                                           attachment;
    vk_image_layout_t                                  layout;
} vk_attachment_reference_t;

typedef struct vk_subpass_description_t {
    vk_subpass_description_flags_t                     flags;
    vk_pipeline_bind_point_t                           pipeline_bind_point;
    uint32_t                                           input_attachment_count;
    const vk_attachment_reference_t*                   p_input_attachments;
    uint32_t                                           color_attachment_count;
    const vk_attachment_reference_t*                   p_color_attachments;
    const vk_attachment_reference_t*                   p_resolve_attachments;
    const vk_attachment_reference_t*                   p_depth_stencil_attachment;
    uint32_t                                           preserve_attachment_count;
    const uint32_t*                                    p_preserve_attachments;
} vk_subpass_description_t;

typedef struct vk_subpass_dependency_t {
    uint32_t                                           src_subpass;
    uint32_t                                           dst_subpass;
    vk_pipeline_stage_flags_t                          src_stage_mask;
    vk_pipeline_stage_flags_t                          dst_stage_mask;
    vk_access_flags_t                                  src_access_mask;
    vk_access_flags_t                                  dst_access_mask;
    vk_dependency_flags_t                              dependency_flags;
} vk_subpass_dependency_t;

typedef struct vk_render_pass_create_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    const void* p_next =                               NULL;
    vk_render_pass_create_flags_t                      flags;
    uint32_t                                           attachment_count;
    const vk_attachment_description_t*                 p_attachments;
    uint32_t                                           subpass_count;
    const vk_subpass_description_t*                    p_subpasses;
    uint32_t                                           dependency_count;
    const vk_subpass_dependency_t*                     p_dependencies;
} vk_render_pass_create_info_t;

typedef struct vk_command_pool_create_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    const void* p_next =                               NULL;
    vk_command_pool_create_flags_t                     flags;
    uint32_t                                           queue_family_index;
} vk_command_pool_create_info_t;

typedef struct vk_command_buffer_allocate_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    const void* p_next =                               NULL;
    vk_command_pool_t                                  command_pool;
    vk_command_buffer_level_t                          level;
    uint32_t                                           command_buffer_count;
} vk_command_buffer_allocate_info_t;

typedef struct vk_command_buffer_inheritance_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    const void* p_next =                               NULL;
    vk_render_pass_t                                   render_pass;
    uint32_t                                           subpass;
    vk_framebuffer_t                                   framebuffer;
    vk_bool32_t                                        occlusion_query_enable;
    vk_query_control_flags_t                           query_flags;
    vk_query_pipeline_statistic_flags_t                pipeline_statistics;
} vk_command_buffer_inheritance_info_t;

typedef struct vk_command_buffer_begin_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    const void* p_next =                               NULL;
    vk_command_buffer_usage_flags_t                    flags;
    const vk_command_buffer_inheritance_info_t*        p_inheritance_info;
} vk_command_buffer_begin_info_t;

typedef struct vk_buffer_copy_t {
    vk_device_size_t                                   src_offset;
    vk_device_size_t                                   dst_offset;
    vk_device_size_t                                   size;
} vk_buffer_copy_t;

typedef struct vk_image_subresource_layers_t {
    vk_image_aspect_flags_t                            aspect_mask;
    uint32_t                                           mip_level;
    uint32_t                                           base_array_layer;
    uint32_t                                           layer_count;
} vk_image_subresource_layers_t;

typedef struct vk_image_copy_t {
    vk_image_subresource_layers_t                      src_subresource;
    vk_offset3_d_t                                     src_offset;
    vk_image_subresource_layers_t                      dst_subresource;
    vk_offset3_d_t                                     dst_offset;
    vk_extent3_d_t                                     extent;
} vk_image_copy_t;

typedef struct vk_image_blit_t {
    vk_image_subresource_layers_t                      src_subresource;
    vk_offset3_d_t                                     src_offsets[2];
    vk_image_subresource_layers_t                      dst_subresource;
    vk_offset3_d_t                                     dst_offsets[2];
} vk_image_blit_t;

typedef struct vk_buffer_image_copy_t {
    vk_device_size_t                                   buffer_offset;
    uint32_t                                           buffer_row_length;
    uint32_t                                           buffer_image_height;
    vk_image_subresource_layers_t                      image_subresource;
    vk_offset3_d_t                                     image_offset;
    vk_extent3_d_t                                     image_extent;
} vk_buffer_image_copy_t;

typedef union vk_clear_color_value_t {
    float                                              float32[4];
    int32_t                                            int32[4];
    uint32_t                                           uint32[4];
} vk_clear_color_value_t;

typedef struct vk_clear_depth_stencil_value_t {
    float                                              depth;
    uint32_t                                           stencil;
} vk_clear_depth_stencil_value_t;

typedef union vk_clear_value_t {
    vk_clear_color_value_t                             color;
    vk_clear_depth_stencil_value_t                     depth_stencil;
} vk_clear_value_t;

typedef struct vk_clear_attachment_t {
    vk_image_aspect_flags_t                            aspect_mask;
    uint32_t                                           color_attachment;
    vk_clear_value_t                                   clear_value;
} vk_clear_attachment_t;

typedef struct vk_clear_rect_t {
    vk_rect2d_t                                       rect;
    uint32_t                                           base_array_layer;
    uint32_t                                           layer_count;
} vk_clear_rect_t;

typedef struct vk_image_resolve_t {
    vk_image_subresource_layers_t                      src_subresource;
    vk_offset3_d_t                                     src_offset;
    vk_image_subresource_layers_t                      dst_subresource;
    vk_offset3_d_t                                     dst_offset;
    vk_extent3_d_t                                     extent;
} vk_image_resolve_t;

typedef struct vk_memory_barrier_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    const void* p_next =                               NULL;
    vk_access_flags_t                                  src_access_mask;
    vk_access_flags_t                                  dst_access_mask;
} vk_memory_barrier_t;

typedef struct vk_buffer_memory_barrier_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    const void* p_next =                               NULL;
    vk_access_flags_t                                  src_access_mask;
    vk_access_flags_t                                  dst_access_mask;
    uint32_t                                           src_queue_family_index;
    uint32_t                                           dst_queue_family_index;
    vk_buffer_t                                        buffer;
    vk_device_size_t                                   offset;
    vk_device_size_t                                   size;
} vk_buffer_memory_barrier_t;

typedef struct vk_image_memory_barrier_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    const void* p_next =                               NULL;
    vk_access_flags_t                                  src_access_mask;
    vk_access_flags_t                                  dst_access_mask;
    vk_image_layout_t                                  old_layout;
    vk_image_layout_t                                  new_layout;
    uint32_t                                           src_queue_family_index;
    uint32_t                                           dst_queue_family_index;
    vk_image_t                                         image;
    vk_image_subresource_range_t                       subresource_range;
} vk_image_memory_barrier_t;

typedef struct vk_render_pass_begin_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    const void* p_next =                               NULL;
    vk_render_pass_t                                   render_pass;
    vk_framebuffer_t                                   framebuffer;
    vk_rect2d_t                                       render_area;
    uint32_t                                           clear_value_count;
    const vk_clear_value_t*                            p_clear_values;
} vk_render_pass_begin_info_t;

typedef struct vk_dispatch_indirect_command_t {
    uint32_t                                           x;
    uint32_t                                           y;
    uint32_t                                           z;
} vk_dispatch_indirect_command_t;

typedef struct vk_draw_indexed_indirect_command_t {
    uint32_t                                           index_count;
    uint32_t                                           instance_count;
    uint32_t                                           first_index;
    int32_t                                            vertex_offset;
    uint32_t                                           first_instance;
} vk_draw_indexed_indirect_command_t;

typedef struct vk_draw_indirect_command_t {
    uint32_t                                           vertex_count;
    uint32_t                                           instance_count;
    uint32_t                                           first_vertex;
    uint32_t                                           first_instance;
} vk_draw_indirect_command_t;

typedef struct vk_base_out_structure_t {
    vk_structure_type_t                                s_type;
    struct vk_base_out_structure_t*                    p_next;
} vk_base_out_structure_t;

typedef struct vk_base_in_structure_t {
    vk_structure_type_t                                s_type;
    const struct vk_base_in_structure_t*               p_next;
} vk_base_in_structure_t;
#define pfn_vk_create_instance_fn_t                                       PFN_vkCreateInstance
#define pfn_vk_destroy_instance_fn_t                                      PFN_vkDestroyInstance
#define pfn_vk_enumerate_physical_devices_fn_t                            PFN_vkEnumeratePhysicalDevices
#define pfn_vk_get_physical_device_features_fn_t                          PFN_vkGetPhysicalDeviceFeatures
#define pfn_vk_get_physical_device_format_properties_fn_t                 PFN_vkGetPhysicalDeviceFormatProperties
#define pfn_vk_get_physical_device_image_format_properties_fn_t           PFN_vkGetPhysicalDeviceImageFormatProperties
#define pfn_vk_get_physical_device_properties_fn_t                        PFN_vkGetPhysicalDeviceProperties
#define pfn_vk_get_physical_device_queue_family_properties_fn_t           PFN_vkGetPhysicalDeviceQueueFamilyProperties
#define pfn_vk_get_physical_device_memory_properties_fn_t                 PFN_vkGetPhysicalDeviceMemoryProperties
#define pfn_vk_get_instance_proc_addr_fn_t                                PFN_vkGetInstanceProcAddr
#define pfn_vk_get_device_proc_addr_fn_t                                  PFN_vkGetDeviceProcAddr
#define pfn_vk_create_device_fn_t                                         PFN_vkCreateDevice
#define pfn_vk_destroy_device_fn_t                                        PFN_vkDestroyDevice
#define pfn_vk_enumerate_instance_extension_properties_fn_t               PFN_vkEnumerateInstanceExtensionProperties
#define pfn_vk_enumerate_device_extension_properties_fn_t                 PFN_vkEnumerateDeviceExtensionProperties
#define pfn_vk_enumerate_instance_layer_properties_fn_t                   PFN_vkEnumerateInstanceLayerProperties
#define pfn_vk_enumerate_device_layer_properties_fn_t                     PFN_vkEnumerateDeviceLayerProperties
#define pfn_vk_get_device_queue_fn_t                                      PFN_vkGetDeviceQueue
#define pfn_vk_queue_submit_fn_t                                          PFN_vkQueueSubmit
#define pfn_vk_queue_wait_idle_fn_t                                       PFN_vkQueueWaitIdle
#define pfn_vk_device_wait_idle_fn_t                                      PFN_vkDeviceWaitIdle
#define pfn_vk_allocate_memory_fn_t                                       PFN_vkAllocateMemory
#define pfn_vk_free_memory_fn_t                                           PFN_vkFreeMemory
#define pfn_vk_map_memory_fn_t                                            PFN_vkMapMemory
#define pfn_vk_unmap_memory_fn_t                                          PFN_vkUnmapMemory
#define pfn_vk_flush_mapped_memory_ranges_fn_t                            PFN_vkFlushMappedMemoryRanges
#define pfn_vk_invalidate_mapped_memory_ranges_fn_t                       PFN_vkInvalidateMappedMemoryRanges
#define pfn_vk_get_device_memory_commitment_fn_t                          PFN_vkGetDeviceMemoryCommitment
#define pfn_vk_bind_buffer_memory_fn_t                                    PFN_vkBindBufferMemory
#define pfn_vk_bind_image_memory_fn_t                                     PFN_vkBindImageMemory
#define pfn_vk_get_buffer_memory_requirements_fn_t                        PFN_vkGetBufferMemoryRequirements
#define pfn_vk_get_image_memory_requirements_fn_t                         PFN_vkGetImageMemoryRequirements
#define pfn_vk_get_image_sparse_memory_requirements_fn_t                  PFN_vkGetImageSparseMemoryRequirements
#define pfn_vk_get_physical_device_sparse_image_format_properties_fn_t    PFN_vkGetPhysicalDeviceSparseImageFormatProperties
#define pfn_vk_queue_bind_sparse_fn_t                                     PFN_vkQueueBindSparse
#define pfn_vk_create_fence_fn_t                                          PFN_vkCreateFence
#define pfn_vk_destroy_fence_fn_t                                         PFN_vkDestroyFence
#define pfn_vk_reset_fences_fn_t                                          PFN_vkResetFences
#define pfn_vk_get_fence_status_fn_t                                      PFN_vkGetFenceStatus
#define pfn_vk_wait_for_fences_fn_t                                       PFN_vkWaitForFences
#define pfn_vk_create_semaphore_fn_t                                      PFN_vkCreateSemaphore
#define pfn_vk_destroy_semaphore_fn_t                                     PFN_vkDestroySemaphore
#define pfn_vk_create_event_fn_t                                          PFN_vkCreateEvent
#define pfn_vk_destroy_event_fn_t                                         PFN_vkDestroyEvent
#define pfn_vk_get_event_status_fn_t                                      PFN_vkGetEventStatus
#define pfn_vk_set_event_fn_t                                             PFN_vkSetEvent
#define pfn_vk_reset_event_fn_t                                           PFN_vkResetEvent
#define pfn_vk_create_query_pool_fn_t                                     PFN_vkCreateQueryPool
#define pfn_vk_destroy_query_pool_fn_t                                    PFN_vkDestroyQueryPool
#define pfn_vk_get_query_pool_results_fn_t                                PFN_vkGetQueryPoolResults
#define pfn_vk_create_buffer_fn_t                                         PFN_vkCreateBuffer
#define pfn_vk_destroy_buffer_fn_t                                        PFN_vkDestroyBuffer
#define pfn_vk_create_buffer_view_fn_t                                    PFN_vkCreateBufferView
#define pfn_vk_destroy_buffer_view_fn_t                                   PFN_vkDestroyBufferView
#define pfn_vk_create_image_fn_t                                          PFN_vkCreateImage
#define pfn_vk_destroy_image_fn_t                                         PFN_vkDestroyImage
#define pfn_vk_get_image_subresource_layout_fn_t                          PFN_vkGetImageSubresourceLayout
#define pfn_vk_create_image_view_fn_t                                     PFN_vkCreateImageView
#define pfn_vk_destroy_image_view_fn_t                                    PFN_vkDestroyImageView
#define pfn_vk_create_shader_module_fn_t                                  PFN_vkCreateShaderModule
#define pfn_vk_destroy_shader_module_fn_t                                 PFN_vkDestroyShaderModule
#define pfn_vk_create_pipeline_cache_fn_t                                 PFN_vkCreatePipelineCache
#define pfn_vk_destroy_pipeline_cache_fn_t                                PFN_vkDestroyPipelineCache
#define pfn_vk_get_pipeline_cache_data_fn_t                               PFN_vkGetPipelineCacheData
#define pfn_vk_merge_pipeline_caches_fn_t                                 PFN_vkMergePipelineCaches
#define pfn_vk_create_graphics_pipelines_fn_t                             PFN_vkCreateGraphicsPipelines
#define pfn_vk_create_compute_pipelines_fn_t                              PFN_vkCreateComputePipelines
#define pfn_vk_destroy_pipeline_fn_t                                      PFN_vkDestroyPipeline
#define pfn_vk_create_pipeline_layout_fn_t                                PFN_vkCreatePipelineLayout
#define pfn_vk_destroy_pipeline_layout_fn_t                               PFN_vkDestroyPipelineLayout
#define pfn_vk_create_sampler_fn_t                                        PFN_vkCreateSampler
#define pfn_vk_destroy_sampler_fn_t                                       PFN_vkDestroySampler
#define pfn_vk_create_descriptor_set_layout_fn_t                          PFN_vkCreateDescriptorSetLayout
#define pfn_vk_destroy_descriptor_set_layout_fn_t                         PFN_vkDestroyDescriptorSetLayout
#define pfn_vk_create_descriptor_pool_fn_t                                PFN_vkCreateDescriptorPool
#define pfn_vk_destroy_descriptor_pool_fn_t                               PFN_vkDestroyDescriptorPool
#define pfn_vk_reset_descriptor_pool_fn_t                                 PFN_vkResetDescriptorPool
#define pfn_vk_allocate_descriptor_sets_fn_t                              PFN_vkAllocateDescriptorSets
#define pfn_vk_free_descriptor_sets_fn_t                                  PFN_vkFreeDescriptorSets
#define pfn_vk_update_descriptor_sets_fn_t                                PFN_vkUpdateDescriptorSets
#define pfn_vk_create_framebuffer_fn_t                                    PFN_vkCreateFramebuffer
#define pfn_vk_destroy_framebuffer_fn_t                                   PFN_vkDestroyFramebuffer
#define pfn_vk_create_render_pass_fn_t                                    PFN_vkCreateRenderPass
#define pfn_vk_destroy_render_pass_fn_t                                   PFN_vkDestroyRenderPass
#define pfn_vk_get_render_area_granularity_fn_t                           PFN_vkGetRenderAreaGranularity
#define pfn_vk_create_command_pool_fn_t                                   PFN_vkCreateCommandPool
#define pfn_vk_destroy_command_pool_fn_t                                  PFN_vkDestroyCommandPool
#define pfn_vk_reset_command_pool_fn_t                                    PFN_vkResetCommandPool
#define pfn_vk_allocate_command_buffers_fn_t                              PFN_vkAllocateCommandBuffers
#define pfn_vk_free_command_buffers_fn_t                                  PFN_vkFreeCommandBuffers
#define pfn_vk_begin_command_buffer_fn_t                                  PFN_vkBeginCommandBuffer
#define pfn_vk_end_command_buffer_fn_t                                    PFN_vkEndCommandBuffer
#define pfn_vk_reset_command_buffer_fn_t                                  PFN_vkResetCommandBuffer
#define pfn_vk_cmd_bind_pipeline_fn_t                                     PFN_vkCmdBindPipeline
#define pfn_vk_cmd_set_viewport_fn_t                                      PFN_vkCmdSetViewport
#define pfn_vk_cmd_set_scissor_fn_t                                       PFN_vkCmdSetScissor
#define pfn_vk_cmd_set_line_width_fn_t                                    PFN_vkCmdSetLineWidth
#define pfn_vk_cmd_set_depth_bias_fn_t                                    PFN_vkCmdSetDepthBias
#define pfn_vk_cmd_set_blend_constants_fn_t                               PFN_vkCmdSetBlendConstants
#define pfn_vk_cmd_set_depth_bounds_fn_t                                  PFN_vkCmdSetDepthBounds
#define pfn_vk_cmd_set_stencil_compare_mask_fn_t                          PFN_vkCmdSetStencilCompareMask
#define pfn_vk_cmd_set_stencil_write_mask_fn_t                            PFN_vkCmdSetStencilWriteMask
#define pfn_vk_cmd_set_stencil_reference_fn_t                             PFN_vkCmdSetStencilReference
#define pfn_vk_cmd_bind_descriptor_sets_fn_t                              PFN_vkCmdBindDescriptorSets
#define pfn_vk_cmd_bind_index_buffer_fn_t                                 PFN_vkCmdBindIndexBuffer
#define pfn_vk_cmd_bind_vertex_buffers_fn_t                               PFN_vkCmdBindVertexBuffers
#define pfn_vk_cmd_draw_fn_t                                              PFN_vkCmdDraw
#define pfn_vk_cmd_draw_indexed_fn_t                                      PFN_vkCmdDrawIndexed
#define pfn_vk_cmd_draw_indirect_fn_t                                     PFN_vkCmdDrawIndirect
#define pfn_vk_cmd_draw_indexed_indirect_fn_t                             PFN_vkCmdDrawIndexedIndirect
#define pfn_vk_cmd_dispatch_fn_t                                          PFN_vkCmdDispatch
#define pfn_vk_cmd_dispatch_indirect_fn_t                                 PFN_vkCmdDispatchIndirect
#define pfn_vk_cmd_copy_buffer_fn_t                                       PFN_vkCmdCopyBuffer
#define pfn_vk_cmd_copy_image_fn_t                                        PFN_vkCmdCopyImage
#define pfn_vk_cmd_blit_image_fn_t                                        PFN_vkCmdBlitImage
#define pfn_vk_cmd_copy_buffer_to_image_fn_t                              PFN_vkCmdCopyBufferToImage
#define pfn_vk_cmd_copy_image_to_buffer_fn_t                              PFN_vkCmdCopyImageToBuffer
#define pfn_vk_cmd_update_buffer_fn_t                                     PFN_vkCmdUpdateBuffer
#define pfn_vk_cmd_fill_buffer_fn_t                                       PFN_vkCmdFillBuffer
#define pfn_vk_cmd_clear_color_image_fn_t                                 PFN_vkCmdClearColorImage
#define pfn_vk_cmd_clear_depth_stencil_image_fn_t                         PFN_vkCmdClearDepthStencilImage
#define pfn_vk_cmd_clear_attachments_fn_t                                 PFN_vkCmdClearAttachments
#define pfn_vk_cmd_resolve_image_fn_t                                     PFN_vkCmdResolveImage
#define pfn_vk_cmd_set_event_fn_t                                         PFN_vkCmdSetEvent
#define pfn_vk_cmd_reset_event_fn_t                                       PFN_vkCmdResetEvent
#define pfn_vk_cmd_wait_events_fn_t                                       PFN_vkCmdWaitEvents
#define pfn_vk_cmd_pipeline_barrier_fn_t                                  PFN_vkCmdPipelineBarrier
#define pfn_vk_cmd_begin_query_fn_t                                       PFN_vkCmdBeginQuery
#define pfn_vk_cmd_end_query_fn_t                                         PFN_vkCmdEndQuery
#define pfn_vk_cmd_reset_query_pool_fn_t                                  PFN_vkCmdResetQueryPool
#define pfn_vk_cmd_write_timestamp_fn_t                                   PFN_vkCmdWriteTimestamp
#define pfn_vk_cmd_copy_query_pool_results_fn_t                           PFN_vkCmdCopyQueryPoolResults
#define pfn_vk_cmd_push_constants_fn_t                                    PFN_vkCmdPushConstants
#define pfn_vk_cmd_begin_render_pass_fn_t                                 PFN_vkCmdBeginRenderPass
#define pfn_vk_cmd_next_subpass_fn_t                                      PFN_vkCmdNextSubpass
#define pfn_vk_cmd_end_render_pass_fn_t                                   PFN_vkCmdEndRenderPass
#define pfn_vk_cmd_execute_commands_fn_t                                  PFN_vkCmdExecuteCommands

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_create_instance(
    const vk_instance_create_info_t*                   pCreateInfo,
    const vk_allocation_callbacks_t*                   pAllocator,
    vk_instance_t*                                     pInstance)
{
    return vkCreateInstance (
            *(    const VkInstanceCreateInfo*                 *)&pCreateInfo,
            *(    const VkAllocationCallbacks*                *)&pAllocator,
            *(    VkInstance*                                 *)&pInstance);
}

inline VKAPI_ATTR void VKAPI_CALL vk_destroy_instance(
    vk_instance_t                                      instance,
    const vk_allocation_callbacks_t*                   pAllocator)
{
    return vkDestroyInstance (
            *(    VkInstance                                  *)&instance,
            *(    const VkAllocationCallbacks*                *)&pAllocator);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_enumerate_physical_devices(
    vk_instance_t                                      instance,
    uint32_t*                                          pPhysicalDeviceCount,
    vk_physical_device_t*                              pPhysicalDevices)
{
    return vkEnumeratePhysicalDevices (
            *(    VkInstance                                  *)&instance,
            *(    uint32_t*                                   *)&pPhysicalDeviceCount,
            *(    VkPhysicalDevice*                           *)&pPhysicalDevices);
}

inline VKAPI_ATTR void VKAPI_CALL vk_get_physical_device_features(
    vk_physical_device_t                               physicalDevice,
    vk_physical_device_features_t*                     pFeatures)
{
    return vkGetPhysicalDeviceFeatures (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    VkPhysicalDeviceFeatures*                   *)&pFeatures);
}

inline VKAPI_ATTR void VKAPI_CALL vk_get_physical_device_format_properties(
    vk_physical_device_t                               physicalDevice,
    vk_format_t                                        format,
    vk_format_properties_t*                            pFormatProperties)
{
    return vkGetPhysicalDeviceFormatProperties (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    VkFormat                                    *)&format,
            *(    VkFormatProperties*                         *)&pFormatProperties);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_physical_device_image_format_properties(
    vk_physical_device_t                               physicalDevice,
    vk_format_t                                        format,
    vk_image_type_t                                    type,
    vk_image_tiling_t                                  tiling,
    vk_image_usage_flags_t                             usage,
    vk_image_create_flags_t                            flags,
    vk_image_format_properties_t*                      pImageFormatProperties)
{
    return vkGetPhysicalDeviceImageFormatProperties (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    VkFormat                                    *)&format,
            *(    VkImageType                                 *)&type,
            *(    VkImageTiling                               *)&tiling,
            *(    VkImageUsageFlags                           *)&usage,
            *(    VkImageCreateFlags                          *)&flags,
            *(    VkImageFormatProperties*                    *)&pImageFormatProperties);
}

inline VKAPI_ATTR void VKAPI_CALL vk_get_physical_device_properties(
    vk_physical_device_t                               physicalDevice,
    vk_physical_device_properties_t*                   pProperties)
{
    return vkGetPhysicalDeviceProperties (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    VkPhysicalDeviceProperties*                 *)&pProperties);
}

inline VKAPI_ATTR void VKAPI_CALL vk_get_physical_device_queue_family_properties(
    vk_physical_device_t                               physicalDevice,
    uint32_t*                                          pQueueFamilyPropertyCount,
    vk_queue_family_properties_t*                      pQueueFamilyProperties)
{
    return vkGetPhysicalDeviceQueueFamilyProperties (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    uint32_t*                                   *)&pQueueFamilyPropertyCount,
            *(    VkQueueFamilyProperties*                    *)&pQueueFamilyProperties);
}

inline VKAPI_ATTR void VKAPI_CALL vk_get_physical_device_memory_properties(
    vk_physical_device_t                               physicalDevice,
    vk_physical_device_memory_properties_t*            pMemoryProperties)
{
    return vkGetPhysicalDeviceMemoryProperties (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    VkPhysicalDeviceMemoryProperties*           *)&pMemoryProperties);
}

inline VKAPI_ATTR pfn_vk_void_function_fn_t VKAPI_CALL vk_get_instance_proc_addr(
    vk_instance_t                                      instance,
    const char*                                        pName)
{
    return vkGetInstanceProcAddr (
            *(    VkInstance                                  *)&instance,
            *(    const char*                                 *)&pName);
}

inline VKAPI_ATTR pfn_vk_void_function_fn_t VKAPI_CALL vk_get_device_proc_addr(
    vk_device_t                                        device,
    const char*                                        pName)
{
    return vkGetDeviceProcAddr (
            *(    VkDevice                                    *)&device,
            *(    const char*                                 *)&pName);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_create_device(
    vk_physical_device_t                               physicalDevice,
    const vk_device_create_info_t*                     pCreateInfo,
    const vk_allocation_callbacks_t*                   pAllocator,
    vk_device_t*                                       pDevice)
{
    return vkCreateDevice (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    const VkDeviceCreateInfo*                   *)&pCreateInfo,
            *(    const VkAllocationCallbacks*                *)&pAllocator,
            *(    VkDevice*                                   *)&pDevice);
}

inline VKAPI_ATTR void VKAPI_CALL vk_destroy_device(
    vk_device_t                                        device,
    const vk_allocation_callbacks_t*                   pAllocator)
{
    return vkDestroyDevice (
            *(    VkDevice                                    *)&device,
            *(    const VkAllocationCallbacks*                *)&pAllocator);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_enumerate_instance_extension_properties(
    const char*                                        pLayerName,
    uint32_t*                                          pPropertyCount,
    vk_extension_properties_t*                         pProperties)
{
    return vkEnumerateInstanceExtensionProperties (
            *(    const char*                                 *)&pLayerName,
            *(    uint32_t*                                   *)&pPropertyCount,
            *(    VkExtensionProperties*                      *)&pProperties);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_enumerate_device_extension_properties(
    vk_physical_device_t                               physicalDevice,
    const char*                                        pLayerName,
    uint32_t*                                          pPropertyCount,
    vk_extension_properties_t*                         pProperties)
{
    return vkEnumerateDeviceExtensionProperties (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    const char*                                 *)&pLayerName,
            *(    uint32_t*                                   *)&pPropertyCount,
            *(    VkExtensionProperties*                      *)&pProperties);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_enumerate_instance_layer_properties(
    uint32_t*                                          pPropertyCount,
    vk_layer_properties_t*                             pProperties)
{
    return vkEnumerateInstanceLayerProperties (
            *(    uint32_t*                                   *)&pPropertyCount,
            *(    VkLayerProperties*                          *)&pProperties);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_enumerate_device_layer_properties(
    vk_physical_device_t                               physicalDevice,
    uint32_t*                                          pPropertyCount,
    vk_layer_properties_t*                             pProperties)
{
    return vkEnumerateDeviceLayerProperties (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    uint32_t*                                   *)&pPropertyCount,
            *(    VkLayerProperties*                          *)&pProperties);
}

inline VKAPI_ATTR void VKAPI_CALL vk_get_device_queue(
    vk_device_t                                        device,
    uint32_t                                           queueFamilyIndex,
    uint32_t                                           queueIndex,
    vk_queue_t*                                        pQueue)
{
    return vkGetDeviceQueue (
            *(    VkDevice                                    *)&device,
            *(    uint32_t                                    *)&queueFamilyIndex,
            *(    uint32_t                                    *)&queueIndex,
            *(    VkQueue*                                    *)&pQueue);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_queue_submit(
    vk_queue_t                                         queue,
    uint32_t                                           submitCount,
    const vk_submit_info_t*                            pSubmits,
    vk_fence_t                                         fence)
{
    return vkQueueSubmit (
            *(    VkQueue                                     *)&queue,
            *(    uint32_t                                    *)&submitCount,
            *(    const VkSubmitInfo*                         *)&pSubmits,
            *(    VkFence                                     *)&fence);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_queue_wait_idle(
    vk_queue_t                                         queue)
{
    return vkQueueWaitIdle (
            *(    VkQueue                                     *)&queue);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_device_wait_idle(
    vk_device_t                                        device)
{
    return vkDeviceWaitIdle (
            *(    VkDevice                                    *)&device);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_allocate_memory(
    vk_device_t                                        device,
    const vk_memory_allocate_info_t*                   pAllocateInfo,
    const vk_allocation_callbacks_t*                   pAllocator,
    vk_device_memory_t*                                pMemory)
{
    return vkAllocateMemory (
            *(    VkDevice                                    *)&device,
            *(    const VkMemoryAllocateInfo*                 *)&pAllocateInfo,
            *(    const VkAllocationCallbacks*                *)&pAllocator,
            *(    VkDeviceMemory*                             *)&pMemory);
}

inline VKAPI_ATTR void VKAPI_CALL vk_free_memory(
    vk_device_t                                        device,
    vk_device_memory_t                                 memory,
    const vk_allocation_callbacks_t*                   pAllocator)
{
    return vkFreeMemory (
            *(    VkDevice                                    *)&device,
            *(    VkDeviceMemory                              *)&memory,
            *(    const VkAllocationCallbacks*                *)&pAllocator);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_map_memory(
    vk_device_t                                        device,
    vk_device_memory_t                                 memory,
    vk_device_size_t                                   offset,
    vk_device_size_t                                   size,
    vk_memory_map_flags_t                              flags,
    void**                                             ppData)
{
    return vkMapMemory (
            *(    VkDevice                                    *)&device,
            *(    VkDeviceMemory                              *)&memory,
            *(    VkDeviceSize                                *)&offset,
            *(    VkDeviceSize                                *)&size,
            *(    VkMemoryMapFlags                            *)&flags,
            *(    void**                                      *)&ppData);
}

inline VKAPI_ATTR void VKAPI_CALL vk_unmap_memory(
    vk_device_t                                        device,
    vk_device_memory_t                                 memory)
{
    return vkUnmapMemory (
            *(    VkDevice                                    *)&device,
            *(    VkDeviceMemory                              *)&memory);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_flush_mapped_memory_ranges(
    vk_device_t                                        device,
    uint32_t                                           memoryRangeCount,
    const vk_mapped_memory_range_t*                    pMemoryRanges)
{
    return vkFlushMappedMemoryRanges (
            *(    VkDevice                                    *)&device,
            *(    uint32_t                                    *)&memoryRangeCount,
            *(    const VkMappedMemoryRange*                  *)&pMemoryRanges);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_invalidate_mapped_memory_ranges(
    vk_device_t                                        device,
    uint32_t                                           memoryRangeCount,
    const vk_mapped_memory_range_t*                    pMemoryRanges)
{
    return vkInvalidateMappedMemoryRanges (
            *(    VkDevice                                    *)&device,
            *(    uint32_t                                    *)&memoryRangeCount,
            *(    const VkMappedMemoryRange*                  *)&pMemoryRanges);
}

inline VKAPI_ATTR void VKAPI_CALL vk_get_device_memory_commitment(
    vk_device_t                                        device,
    vk_device_memory_t                                 memory,
    vk_device_size_t*                                  pCommittedMemoryInBytes)
{
    return vkGetDeviceMemoryCommitment (
            *(    VkDevice                                    *)&device,
            *(    VkDeviceMemory                              *)&memory,
            *(    VkDeviceSize*                               *)&pCommittedMemoryInBytes);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_bind_buffer_memory(
    vk_device_t                                        device,
    vk_buffer_t                                        buffer,
    vk_device_memory_t                                 memory,
    vk_device_size_t                                   memoryOffset)
{
    return vkBindBufferMemory (
            *(    VkDevice                                    *)&device,
            *(    VkBuffer                                    *)&buffer,
            *(    VkDeviceMemory                              *)&memory,
            *(    VkDeviceSize                                *)&memoryOffset);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_bind_image_memory(
    vk_device_t                                        device,
    vk_image_t                                         image,
    vk_device_memory_t                                 memory,
    vk_device_size_t                                   memoryOffset)
{
    return vkBindImageMemory (
            *(    VkDevice                                    *)&device,
            *(    VkImage                                     *)&image,
            *(    VkDeviceMemory                              *)&memory,
            *(    VkDeviceSize                                *)&memoryOffset);
}

inline VKAPI_ATTR void VKAPI_CALL vk_get_buffer_memory_requirements(
    vk_device_t                                        device,
    vk_buffer_t                                        buffer,
    vk_memory_requirements_t*                          pMemoryRequirements)
{
    return vkGetBufferMemoryRequirements (
            *(    VkDevice                                    *)&device,
            *(    VkBuffer                                    *)&buffer,
            *(    VkMemoryRequirements*                       *)&pMemoryRequirements);
}

inline VKAPI_ATTR void VKAPI_CALL vk_get_image_memory_requirements(
    vk_device_t                                        device,
    vk_image_t                                         image,
    vk_memory_requirements_t*                          pMemoryRequirements)
{
    return vkGetImageMemoryRequirements (
            *(    VkDevice                                    *)&device,
            *(    VkImage                                     *)&image,
            *(    VkMemoryRequirements*                       *)&pMemoryRequirements);
}

inline VKAPI_ATTR void VKAPI_CALL vk_get_image_sparse_memory_requirements(
    vk_device_t                                        device,
    vk_image_t                                         image,
    uint32_t*                                          pSparseMemoryRequirementCount,
    vk_sparse_image_memory_requirements_t*             pSparseMemoryRequirements)
{
    return vkGetImageSparseMemoryRequirements (
            *(    VkDevice                                    *)&device,
            *(    VkImage                                     *)&image,
            *(    uint32_t*                                   *)&pSparseMemoryRequirementCount,
            *(    VkSparseImageMemoryRequirements*            *)&pSparseMemoryRequirements);
}

inline VKAPI_ATTR void VKAPI_CALL vk_get_physical_device_sparse_image_format_properties(
    vk_physical_device_t                               physicalDevice,
    vk_format_t                                        format,
    vk_image_type_t                                    type,
    vk_sample_count_flag_bits_t                        samples,
    vk_image_usage_flags_t                             usage,
    vk_image_tiling_t                                  tiling,
    uint32_t*                                          pPropertyCount,
    vk_sparse_image_format_properties_t*               pProperties)
{
    return vkGetPhysicalDeviceSparseImageFormatProperties (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    VkFormat                                    *)&format,
            *(    VkImageType                                 *)&type,
            *(    VkSampleCountFlagBits                       *)&samples,
            *(    VkImageUsageFlags                           *)&usage,
            *(    VkImageTiling                               *)&tiling,
            *(    uint32_t*                                   *)&pPropertyCount,
            *(    VkSparseImageFormatProperties*              *)&pProperties);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_queue_bind_sparse(
    vk_queue_t                                         queue,
    uint32_t                                           bindInfoCount,
    const vk_bind_sparse_info_t*                       pBindInfo,
    vk_fence_t                                         fence)
{
    return vkQueueBindSparse (
            *(    VkQueue                                     *)&queue,
            *(    uint32_t                                    *)&bindInfoCount,
            *(    const VkBindSparseInfo*                     *)&pBindInfo,
            *(    VkFence                                     *)&fence);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_create_fence(
    vk_device_t                                        device,
    const vk_fence_create_info_t*                      pCreateInfo,
    const vk_allocation_callbacks_t*                   pAllocator,
    vk_fence_t*                                        pFence)
{
    return vkCreateFence (
            *(    VkDevice                                    *)&device,
            *(    const VkFenceCreateInfo*                    *)&pCreateInfo,
            *(    const VkAllocationCallbacks*                *)&pAllocator,
            *(    VkFence*                                    *)&pFence);
}

inline VKAPI_ATTR void VKAPI_CALL vk_destroy_fence(
    vk_device_t                                        device,
    vk_fence_t                                         fence,
    const vk_allocation_callbacks_t*                   pAllocator)
{
    return vkDestroyFence (
            *(    VkDevice                                    *)&device,
            *(    VkFence                                     *)&fence,
            *(    const VkAllocationCallbacks*                *)&pAllocator);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_reset_fences(
    vk_device_t                                        device,
    uint32_t                                           fenceCount,
    const vk_fence_t*                                  pFences)
{
    return vkResetFences (
            *(    VkDevice                                    *)&device,
            *(    uint32_t                                    *)&fenceCount,
            *(    const VkFence*                              *)&pFences);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_fence_status(
    vk_device_t                                        device,
    vk_fence_t                                         fence)
{
    return vkGetFenceStatus (
            *(    VkDevice                                    *)&device,
            *(    VkFence                                     *)&fence);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_wait_for_fences(
    vk_device_t                                        device,
    uint32_t                                           fenceCount,
    const vk_fence_t*                                  pFences,
    vk_bool32_t                                        waitAll,
    uint64_t                                           timeout)
{
    return vkWaitForFences (
            *(    VkDevice                                    *)&device,
            *(    uint32_t                                    *)&fenceCount,
            *(    const VkFence*                              *)&pFences,
            *(    VkBool32                                    *)&waitAll,
            *(    uint64_t                                    *)&timeout);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_create_semaphore(
    vk_device_t                                        device,
    const vk_semaphore_create_info_t*                  pCreateInfo,
    const vk_allocation_callbacks_t*                   pAllocator,
    vk_semaphore_t*                                    pSemaphore)
{
    return vkCreateSemaphore (
            *(    VkDevice                                    *)&device,
            *(    const VkSemaphoreCreateInfo*                *)&pCreateInfo,
            *(    const VkAllocationCallbacks*                *)&pAllocator,
            *(    VkSemaphore*                                *)&pSemaphore);
}

inline VKAPI_ATTR void VKAPI_CALL vk_destroy_semaphore(
    vk_device_t                                        device,
    vk_semaphore_t                                     semaphore,
    const vk_allocation_callbacks_t*                   pAllocator)
{
    return vkDestroySemaphore (
            *(    VkDevice                                    *)&device,
            *(    VkSemaphore                                 *)&semaphore,
            *(    const VkAllocationCallbacks*                *)&pAllocator);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_create_event(
    vk_device_t                                        device,
    const vk_event_create_info_t*                      pCreateInfo,
    const vk_allocation_callbacks_t*                   pAllocator,
    vk_event_t*                                        pEvent)
{
    return vkCreateEvent (
            *(    VkDevice                                    *)&device,
            *(    const VkEventCreateInfo*                    *)&pCreateInfo,
            *(    const VkAllocationCallbacks*                *)&pAllocator,
            *(    VkEvent*                                    *)&pEvent);
}

inline VKAPI_ATTR void VKAPI_CALL vk_destroy_event(
    vk_device_t                                        device,
    vk_event_t                                         event,
    const vk_allocation_callbacks_t*                   pAllocator)
{
    return vkDestroyEvent (
            *(    VkDevice                                    *)&device,
            *(    VkEvent                                     *)&event,
            *(    const VkAllocationCallbacks*                *)&pAllocator);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_event_status(
    vk_device_t                                        device,
    vk_event_t                                         event)
{
    return vkGetEventStatus (
            *(    VkDevice                                    *)&device,
            *(    VkEvent                                     *)&event);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_set_event(
    vk_device_t                                        device,
    vk_event_t                                         event)
{
    return vkSetEvent (
            *(    VkDevice                                    *)&device,
            *(    VkEvent                                     *)&event);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_reset_event(
    vk_device_t                                        device,
    vk_event_t                                         event)
{
    return vkResetEvent (
            *(    VkDevice                                    *)&device,
            *(    VkEvent                                     *)&event);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_create_query_pool(
    vk_device_t                                        device,
    const vk_query_pool_create_info_t*                 pCreateInfo,
    const vk_allocation_callbacks_t*                   pAllocator,
    vk_query_pool_t*                                   pQueryPool)
{
    return vkCreateQueryPool (
            *(    VkDevice                                    *)&device,
            *(    const VkQueryPoolCreateInfo*                *)&pCreateInfo,
            *(    const VkAllocationCallbacks*                *)&pAllocator,
            *(    VkQueryPool*                                *)&pQueryPool);
}

inline VKAPI_ATTR void VKAPI_CALL vk_destroy_query_pool(
    vk_device_t                                        device,
    vk_query_pool_t                                    queryPool,
    const vk_allocation_callbacks_t*                   pAllocator)
{
    return vkDestroyQueryPool (
            *(    VkDevice                                    *)&device,
            *(    VkQueryPool                                 *)&queryPool,
            *(    const VkAllocationCallbacks*                *)&pAllocator);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_query_pool_results(
    vk_device_t                                        device,
    vk_query_pool_t                                    queryPool,
    uint32_t                                           firstQuery,
    uint32_t                                           queryCount,
    size_t                                             dataSize,
    void*                                              pData,
    vk_device_size_t                                   stride,
    vk_query_result_flags_t                            flags)
{
    return vkGetQueryPoolResults (
            *(    VkDevice                                    *)&device,
            *(    VkQueryPool                                 *)&queryPool,
            *(    uint32_t                                    *)&firstQuery,
            *(    uint32_t                                    *)&queryCount,
            *(    size_t                                      *)&dataSize,
            *(    void*                                       *)&pData,
            *(    VkDeviceSize                                *)&stride,
            *(    VkQueryResultFlags                          *)&flags);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_create_buffer(
    vk_device_t                                        device,
    const vk_buffer_create_info_t*                     pCreateInfo,
    const vk_allocation_callbacks_t*                   pAllocator,
    vk_buffer_t*                                       pBuffer)
{
    return vkCreateBuffer (
            *(    VkDevice                                    *)&device,
            *(    const VkBufferCreateInfo*                   *)&pCreateInfo,
            *(    const VkAllocationCallbacks*                *)&pAllocator,
            *(    VkBuffer*                                   *)&pBuffer);
}

inline VKAPI_ATTR void VKAPI_CALL vk_destroy_buffer(
    vk_device_t                                        device,
    vk_buffer_t                                        buffer,
    const vk_allocation_callbacks_t*                   pAllocator)
{
    return vkDestroyBuffer (
            *(    VkDevice                                    *)&device,
            *(    VkBuffer                                    *)&buffer,
            *(    const VkAllocationCallbacks*                *)&pAllocator);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_create_buffer_view(
    vk_device_t                                        device,
    const vk_buffer_view_create_info_t*                pCreateInfo,
    const vk_allocation_callbacks_t*                   pAllocator,
    vk_buffer_view_t*                                  pView)
{
    return vkCreateBufferView (
            *(    VkDevice                                    *)&device,
            *(    const VkBufferViewCreateInfo*               *)&pCreateInfo,
            *(    const VkAllocationCallbacks*                *)&pAllocator,
            *(    VkBufferView*                               *)&pView);
}

inline VKAPI_ATTR void VKAPI_CALL vk_destroy_buffer_view(
    vk_device_t                                        device,
    vk_buffer_view_t                                   bufferView,
    const vk_allocation_callbacks_t*                   pAllocator)
{
    return vkDestroyBufferView (
            *(    VkDevice                                    *)&device,
            *(    VkBufferView                                *)&bufferView,
            *(    const VkAllocationCallbacks*                *)&pAllocator);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_create_image(
    vk_device_t                                        device,
    const vk_image_create_info_t*                      pCreateInfo,
    const vk_allocation_callbacks_t*                   pAllocator,
    vk_image_t*                                        pImage)
{
    return vkCreateImage (
            *(    VkDevice                                    *)&device,
            *(    const VkImageCreateInfo*                    *)&pCreateInfo,
            *(    const VkAllocationCallbacks*                *)&pAllocator,
            *(    VkImage*                                    *)&pImage);
}

inline VKAPI_ATTR void VKAPI_CALL vk_destroy_image(
    vk_device_t                                        device,
    vk_image_t                                         image,
    const vk_allocation_callbacks_t*                   pAllocator)
{
    return vkDestroyImage (
            *(    VkDevice                                    *)&device,
            *(    VkImage                                     *)&image,
            *(    const VkAllocationCallbacks*                *)&pAllocator);
}

inline VKAPI_ATTR void VKAPI_CALL vk_get_image_subresource_layout(
    vk_device_t                                        device,
    vk_image_t                                         image,
    const vk_image_subresource_t*                      pSubresource,
    vk_subresource_layout_t*                           pLayout)
{
    return vkGetImageSubresourceLayout (
            *(    VkDevice                                    *)&device,
            *(    VkImage                                     *)&image,
            *(    const VkImageSubresource*                   *)&pSubresource,
            *(    VkSubresourceLayout*                        *)&pLayout);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_create_image_view(
    vk_device_t                                        device,
    const vk_image_view_create_info_t*                 pCreateInfo,
    const vk_allocation_callbacks_t*                   pAllocator,
    vk_image_view_t*                                   pView)
{
    return vkCreateImageView (
            *(    VkDevice                                    *)&device,
            *(    const VkImageViewCreateInfo*                *)&pCreateInfo,
            *(    const VkAllocationCallbacks*                *)&pAllocator,
            *(    VkImageView*                                *)&pView);
}

inline VKAPI_ATTR void VKAPI_CALL vk_destroy_image_view(
    vk_device_t                                        device,
    vk_image_view_t                                    imageView,
    const vk_allocation_callbacks_t*                   pAllocator)
{
    return vkDestroyImageView (
            *(    VkDevice                                    *)&device,
            *(    VkImageView                                 *)&imageView,
            *(    const VkAllocationCallbacks*                *)&pAllocator);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_create_shader_module(
    vk_device_t                                        device,
    const vk_shader_module_create_info_t*              pCreateInfo,
    const vk_allocation_callbacks_t*                   pAllocator,
    vk_shader_module_t*                                pShaderModule)
{
    return vkCreateShaderModule (
            *(    VkDevice                                    *)&device,
            *(    const VkShaderModuleCreateInfo*             *)&pCreateInfo,
            *(    const VkAllocationCallbacks*                *)&pAllocator,
            *(    VkShaderModule*                             *)&pShaderModule);
}

inline VKAPI_ATTR void VKAPI_CALL vk_destroy_shader_module(
    vk_device_t                                        device,
    vk_shader_module_t                                 shaderModule,
    const vk_allocation_callbacks_t*                   pAllocator)
{
    return vkDestroyShaderModule (
            *(    VkDevice                                    *)&device,
            *(    VkShaderModule                              *)&shaderModule,
            *(    const VkAllocationCallbacks*                *)&pAllocator);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_create_pipeline_cache(
    vk_device_t                                        device,
    const vk_pipeline_cache_create_info_t*             pCreateInfo,
    const vk_allocation_callbacks_t*                   pAllocator,
    vk_pipeline_cache_t*                               pPipelineCache)
{
    return vkCreatePipelineCache (
            *(    VkDevice                                    *)&device,
            *(    const VkPipelineCacheCreateInfo*            *)&pCreateInfo,
            *(    const VkAllocationCallbacks*                *)&pAllocator,
            *(    VkPipelineCache*                            *)&pPipelineCache);
}

inline VKAPI_ATTR void VKAPI_CALL vk_destroy_pipeline_cache(
    vk_device_t                                        device,
    vk_pipeline_cache_t                                pipelineCache,
    const vk_allocation_callbacks_t*                   pAllocator)
{
    return vkDestroyPipelineCache (
            *(    VkDevice                                    *)&device,
            *(    VkPipelineCache                             *)&pipelineCache,
            *(    const VkAllocationCallbacks*                *)&pAllocator);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_pipeline_cache_data(
    vk_device_t                                        device,
    vk_pipeline_cache_t                                pipelineCache,
    size_t*                                            pDataSize,
    void*                                              pData)
{
    return vkGetPipelineCacheData (
            *(    VkDevice                                    *)&device,
            *(    VkPipelineCache                             *)&pipelineCache,
            *(    size_t*                                     *)&pDataSize,
            *(    void*                                       *)&pData);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_merge_pipeline_caches(
    vk_device_t                                        device,
    vk_pipeline_cache_t                                dstCache,
    uint32_t                                           srcCacheCount,
    const vk_pipeline_cache_t*                         pSrcCaches)
{
    return vkMergePipelineCaches (
            *(    VkDevice                                    *)&device,
            *(    VkPipelineCache                             *)&dstCache,
            *(    uint32_t                                    *)&srcCacheCount,
            *(    const VkPipelineCache*                      *)&pSrcCaches);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_create_graphics_pipelines(
    vk_device_t                                        device,
    vk_pipeline_cache_t                                pipelineCache,
    uint32_t                                           createInfoCount,
    const vk_graphics_pipeline_create_info_t*          pCreateInfos,
    const vk_allocation_callbacks_t*                   pAllocator,
    vk_pipeline_t*                                     pPipelines)
{
    return vkCreateGraphicsPipelines (
            *(    VkDevice                                    *)&device,
            *(    VkPipelineCache                             *)&pipelineCache,
            *(    uint32_t                                    *)&createInfoCount,
            *(    const VkGraphicsPipelineCreateInfo*         *)&pCreateInfos,
            *(    const VkAllocationCallbacks*                *)&pAllocator,
            *(    VkPipeline*                                 *)&pPipelines);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_create_compute_pipelines(
    vk_device_t                                        device,
    vk_pipeline_cache_t                                pipelineCache,
    uint32_t                                           createInfoCount,
    const vk_compute_pipeline_create_info_t*           pCreateInfos,
    const vk_allocation_callbacks_t*                   pAllocator,
    vk_pipeline_t*                                     pPipelines)
{
    return vkCreateComputePipelines (
            *(    VkDevice                                    *)&device,
            *(    VkPipelineCache                             *)&pipelineCache,
            *(    uint32_t                                    *)&createInfoCount,
            *(    const VkComputePipelineCreateInfo*          *)&pCreateInfos,
            *(    const VkAllocationCallbacks*                *)&pAllocator,
            *(    VkPipeline*                                 *)&pPipelines);
}

inline VKAPI_ATTR void VKAPI_CALL vk_destroy_pipeline(
    vk_device_t                                        device,
    vk_pipeline_t                                      pipeline,
    const vk_allocation_callbacks_t*                   pAllocator)
{
    return vkDestroyPipeline (
            *(    VkDevice                                    *)&device,
            *(    VkPipeline                                  *)&pipeline,
            *(    const VkAllocationCallbacks*                *)&pAllocator);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_create_pipeline_layout(
    vk_device_t                                        device,
    const vk_pipeline_layout_create_info_t*            pCreateInfo,
    const vk_allocation_callbacks_t*                   pAllocator,
    vk_pipeline_layout_t*                              pPipelineLayout)
{
    return vkCreatePipelineLayout (
            *(    VkDevice                                    *)&device,
            *(    const VkPipelineLayoutCreateInfo*           *)&pCreateInfo,
            *(    const VkAllocationCallbacks*                *)&pAllocator,
            *(    VkPipelineLayout*                           *)&pPipelineLayout);
}

inline VKAPI_ATTR void VKAPI_CALL vk_destroy_pipeline_layout(
    vk_device_t                                        device,
    vk_pipeline_layout_t                               pipelineLayout,
    const vk_allocation_callbacks_t*                   pAllocator)
{
    return vkDestroyPipelineLayout (
            *(    VkDevice                                    *)&device,
            *(    VkPipelineLayout                            *)&pipelineLayout,
            *(    const VkAllocationCallbacks*                *)&pAllocator);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_create_sampler(
    vk_device_t                                        device,
    const vk_sampler_create_info_t*                    pCreateInfo,
    const vk_allocation_callbacks_t*                   pAllocator,
    vk_sampler_t*                                      pSampler)
{
    return vkCreateSampler (
            *(    VkDevice                                    *)&device,
            *(    const VkSamplerCreateInfo*                  *)&pCreateInfo,
            *(    const VkAllocationCallbacks*                *)&pAllocator,
            *(    VkSampler*                                  *)&pSampler);
}

inline VKAPI_ATTR void VKAPI_CALL vk_destroy_sampler(
    vk_device_t                                        device,
    vk_sampler_t                                       sampler,
    const vk_allocation_callbacks_t*                   pAllocator)
{
    return vkDestroySampler (
            *(    VkDevice                                    *)&device,
            *(    VkSampler                                   *)&sampler,
            *(    const VkAllocationCallbacks*                *)&pAllocator);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_create_descriptor_set_layout(
    vk_device_t                                        device,
    const vk_descriptor_set_layout_create_info_t*      pCreateInfo,
    const vk_allocation_callbacks_t*                   pAllocator,
    vk_descriptor_set_layout_t*                        pSetLayout)
{
    return vkCreateDescriptorSetLayout (
            *(    VkDevice                                    *)&device,
            *(    const VkDescriptorSetLayoutCreateInfo*      *)&pCreateInfo,
            *(    const VkAllocationCallbacks*                *)&pAllocator,
            *(    VkDescriptorSetLayout*                      *)&pSetLayout);
}

inline VKAPI_ATTR void VKAPI_CALL vk_destroy_descriptor_set_layout(
    vk_device_t                                        device,
    vk_descriptor_set_layout_t                         descriptorSetLayout,
    const vk_allocation_callbacks_t*                   pAllocator)
{
    return vkDestroyDescriptorSetLayout (
            *(    VkDevice                                    *)&device,
            *(    VkDescriptorSetLayout                       *)&descriptorSetLayout,
            *(    const VkAllocationCallbacks*                *)&pAllocator);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_create_descriptor_pool(
    vk_device_t                                        device,
    const vk_descriptor_pool_create_info_t*            pCreateInfo,
    const vk_allocation_callbacks_t*                   pAllocator,
    vk_descriptor_pool_t*                              pDescriptorPool)
{
    return vkCreateDescriptorPool (
            *(    VkDevice                                    *)&device,
            *(    const VkDescriptorPoolCreateInfo*           *)&pCreateInfo,
            *(    const VkAllocationCallbacks*                *)&pAllocator,
            *(    VkDescriptorPool*                           *)&pDescriptorPool);
}

inline VKAPI_ATTR void VKAPI_CALL vk_destroy_descriptor_pool(
    vk_device_t                                        device,
    vk_descriptor_pool_t                               descriptorPool,
    const vk_allocation_callbacks_t*                   pAllocator)
{
    return vkDestroyDescriptorPool (
            *(    VkDevice                                    *)&device,
            *(    VkDescriptorPool                            *)&descriptorPool,
            *(    const VkAllocationCallbacks*                *)&pAllocator);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_reset_descriptor_pool(
    vk_device_t                                        device,
    vk_descriptor_pool_t                               descriptorPool,
    vk_descriptor_pool_reset_flags_t                   flags)
{
    return vkResetDescriptorPool (
            *(    VkDevice                                    *)&device,
            *(    VkDescriptorPool                            *)&descriptorPool,
            *(    VkDescriptorPoolResetFlags                  *)&flags);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_allocate_descriptor_sets(
    vk_device_t                                        device,
    const vk_descriptor_set_allocate_info_t*           pAllocateInfo,
    vk_descriptor_set_t*                               pDescriptorSets)
{
    return vkAllocateDescriptorSets (
            *(    VkDevice                                    *)&device,
            *(    const VkDescriptorSetAllocateInfo*          *)&pAllocateInfo,
            *(    VkDescriptorSet*                            *)&pDescriptorSets);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_free_descriptor_sets(
    vk_device_t                                        device,
    vk_descriptor_pool_t                               descriptorPool,
    uint32_t                                           descriptorSetCount,
    const vk_descriptor_set_t*                         pDescriptorSets)
{
    return vkFreeDescriptorSets (
            *(    VkDevice                                    *)&device,
            *(    VkDescriptorPool                            *)&descriptorPool,
            *(    uint32_t                                    *)&descriptorSetCount,
            *(    const VkDescriptorSet*                      *)&pDescriptorSets);
}

inline VKAPI_ATTR void VKAPI_CALL vk_update_descriptor_sets(
    vk_device_t                                        device,
    uint32_t                                           descriptorWriteCount,
    const vk_write_descriptor_set_t*                   pDescriptorWrites,
    uint32_t                                           descriptorCopyCount,
    const vk_copy_descriptor_set_t*                    pDescriptorCopies)
{
    return vkUpdateDescriptorSets (
            *(    VkDevice                                    *)&device,
            *(    uint32_t                                    *)&descriptorWriteCount,
            *(    const VkWriteDescriptorSet*                 *)&pDescriptorWrites,
            *(    uint32_t                                    *)&descriptorCopyCount,
            *(    const VkCopyDescriptorSet*                  *)&pDescriptorCopies);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_create_framebuffer(
    vk_device_t                                        device,
    const vk_framebuffer_create_info_t*                pCreateInfo,
    const vk_allocation_callbacks_t*                   pAllocator,
    vk_framebuffer_t*                                  pFramebuffer)
{
    return vkCreateFramebuffer (
            *(    VkDevice                                    *)&device,
            *(    const VkFramebufferCreateInfo*              *)&pCreateInfo,
            *(    const VkAllocationCallbacks*                *)&pAllocator,
            *(    VkFramebuffer*                              *)&pFramebuffer);
}

inline VKAPI_ATTR void VKAPI_CALL vk_destroy_framebuffer(
    vk_device_t                                        device,
    vk_framebuffer_t                                   framebuffer,
    const vk_allocation_callbacks_t*                   pAllocator)
{
    return vkDestroyFramebuffer (
            *(    VkDevice                                    *)&device,
            *(    VkFramebuffer                               *)&framebuffer,
            *(    const VkAllocationCallbacks*                *)&pAllocator);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_create_render_pass(
    vk_device_t                                        device,
    const vk_render_pass_create_info_t*                pCreateInfo,
    const vk_allocation_callbacks_t*                   pAllocator,
    vk_render_pass_t*                                  pRenderPass)
{
    return vkCreateRenderPass (
            *(    VkDevice                                    *)&device,
            *(    const VkRenderPassCreateInfo*               *)&pCreateInfo,
            *(    const VkAllocationCallbacks*                *)&pAllocator,
            *(    VkRenderPass*                               *)&pRenderPass);
}

inline VKAPI_ATTR void VKAPI_CALL vk_destroy_render_pass(
    vk_device_t                                        device,
    vk_render_pass_t                                   renderPass,
    const vk_allocation_callbacks_t*                   pAllocator)
{
    return vkDestroyRenderPass (
            *(    VkDevice                                    *)&device,
            *(    VkRenderPass                                *)&renderPass,
            *(    const VkAllocationCallbacks*                *)&pAllocator);
}

inline VKAPI_ATTR void VKAPI_CALL vk_get_render_area_granularity(
    vk_device_t                                        device,
    vk_render_pass_t                                   renderPass,
    vk_extent2d_t*                                    pGranularity)
{
    return vkGetRenderAreaGranularity (
            *(    VkDevice                                    *)&device,
            *(    VkRenderPass                                *)&renderPass,
            *(    VkExtent2D*                                 *)&pGranularity);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_create_command_pool(
    vk_device_t                                        device,
    const vk_command_pool_create_info_t*               pCreateInfo,
    const vk_allocation_callbacks_t*                   pAllocator,
    vk_command_pool_t*                                 pCommandPool)
{
    return vkCreateCommandPool (
            *(    VkDevice                                    *)&device,
            *(    const VkCommandPoolCreateInfo*              *)&pCreateInfo,
            *(    const VkAllocationCallbacks*                *)&pAllocator,
            *(    VkCommandPool*                              *)&pCommandPool);
}

inline VKAPI_ATTR void VKAPI_CALL vk_destroy_command_pool(
    vk_device_t                                        device,
    vk_command_pool_t                                  commandPool,
    const vk_allocation_callbacks_t*                   pAllocator)
{
    return vkDestroyCommandPool (
            *(    VkDevice                                    *)&device,
            *(    VkCommandPool                               *)&commandPool,
            *(    const VkAllocationCallbacks*                *)&pAllocator);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_reset_command_pool(
    vk_device_t                                        device,
    vk_command_pool_t                                  commandPool,
    vk_command_pool_reset_flags_t                      flags)
{
    return vkResetCommandPool (
            *(    VkDevice                                    *)&device,
            *(    VkCommandPool                               *)&commandPool,
            *(    VkCommandPoolResetFlags                     *)&flags);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_allocate_command_buffers(
    vk_device_t                                        device,
    const vk_command_buffer_allocate_info_t*           pAllocateInfo,
    vk_command_buffer_t*                               pCommandBuffers)
{
    return vkAllocateCommandBuffers (
            *(    VkDevice                                    *)&device,
            *(    const VkCommandBufferAllocateInfo*          *)&pAllocateInfo,
            *(    VkCommandBuffer*                            *)&pCommandBuffers);
}

inline VKAPI_ATTR void VKAPI_CALL vk_free_command_buffers(
    vk_device_t                                        device,
    vk_command_pool_t                                  commandPool,
    uint32_t                                           commandBufferCount,
    const vk_command_buffer_t*                         pCommandBuffers)
{
    return vkFreeCommandBuffers (
            *(    VkDevice                                    *)&device,
            *(    VkCommandPool                               *)&commandPool,
            *(    uint32_t                                    *)&commandBufferCount,
            *(    const VkCommandBuffer*                      *)&pCommandBuffers);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_begin_command_buffer(
    vk_command_buffer_t                                commandBuffer,
    const vk_command_buffer_begin_info_t*              pBeginInfo)
{
    return vkBeginCommandBuffer (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    const VkCommandBufferBeginInfo*             *)&pBeginInfo);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_end_command_buffer(
    vk_command_buffer_t                                commandBuffer)
{
    return vkEndCommandBuffer (
            *(    VkCommandBuffer                             *)&commandBuffer);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_reset_command_buffer(
    vk_command_buffer_t                                commandBuffer,
    vk_command_buffer_reset_flags_t                    flags)
{
    return vkResetCommandBuffer (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    VkCommandBufferResetFlags                   *)&flags);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_bind_pipeline(
    vk_command_buffer_t                                commandBuffer,
    vk_pipeline_bind_point_t                           pipelineBindPoint,
    vk_pipeline_t                                      pipeline)
{
    return vkCmdBindPipeline (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    VkPipelineBindPoint                         *)&pipelineBindPoint,
            *(    VkPipeline                                  *)&pipeline);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_set_viewport(
    vk_command_buffer_t                                commandBuffer,
    uint32_t                                           firstViewport,
    uint32_t                                           viewportCount,
    const vk_viewport_t*                               pViewports)
{
    return vkCmdSetViewport (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    uint32_t                                    *)&firstViewport,
            *(    uint32_t                                    *)&viewportCount,
            *(    const VkViewport*                           *)&pViewports);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_set_scissor(
    vk_command_buffer_t                                commandBuffer,
    uint32_t                                           firstScissor,
    uint32_t                                           scissorCount,
    const vk_rect2d_t*                                pScissors)
{
    return vkCmdSetScissor (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    uint32_t                                    *)&firstScissor,
            *(    uint32_t                                    *)&scissorCount,
            *(    const VkRect2D*                             *)&pScissors);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_set_line_width(
    vk_command_buffer_t                                commandBuffer,
    float                                              lineWidth)
{
    return vkCmdSetLineWidth (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    float                                       *)&lineWidth);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_set_depth_bias(
    vk_command_buffer_t                                commandBuffer,
    float                                              depthBiasConstantFactor,
    float                                              depthBiasClamp,
    float                                              depthBiasSlopeFactor)
{
    return vkCmdSetDepthBias (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    float                                       *)&depthBiasConstantFactor,
            *(    float                                       *)&depthBiasClamp,
            *(    float                                       *)&depthBiasSlopeFactor);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_set_blend_constants(
    vk_command_buffer_t                                commandBuffer,
    const float                                        blendConstants[4])
{
    using bc_t = const float[4];
    return vkCmdSetBlendConstants (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    bc_t                                        *)&blendConstants);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_set_depth_bounds(
    vk_command_buffer_t                                commandBuffer,
    float                                              minDepthBounds,
    float                                              maxDepthBounds)
{
    return vkCmdSetDepthBounds (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    float                                       *)&minDepthBounds,
            *(    float                                       *)&maxDepthBounds);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_set_stencil_compare_mask(
    vk_command_buffer_t                                commandBuffer,
    vk_stencil_face_flags_t                            faceMask,
    uint32_t                                           compareMask)
{
    return vkCmdSetStencilCompareMask (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    VkStencilFaceFlags                          *)&faceMask,
            *(    uint32_t                                    *)&compareMask);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_set_stencil_write_mask(
    vk_command_buffer_t                                commandBuffer,
    vk_stencil_face_flags_t                            faceMask,
    uint32_t                                           writeMask)
{
    return vkCmdSetStencilWriteMask (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    VkStencilFaceFlags                          *)&faceMask,
            *(    uint32_t                                    *)&writeMask);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_set_stencil_reference(
    vk_command_buffer_t                                commandBuffer,
    vk_stencil_face_flags_t                            faceMask,
    uint32_t                                           reference)
{
    return vkCmdSetStencilReference (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    VkStencilFaceFlags                          *)&faceMask,
            *(    uint32_t                                    *)&reference);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_bind_descriptor_sets(
    vk_command_buffer_t                                commandBuffer,
    vk_pipeline_bind_point_t                           pipelineBindPoint,
    vk_pipeline_layout_t                               layout,
    uint32_t                                           firstSet,
    uint32_t                                           descriptorSetCount,
    const vk_descriptor_set_t*                         pDescriptorSets,
    uint32_t                                           dynamicOffsetCount,
    const uint32_t*                                    pDynamicOffsets)
{
    return vkCmdBindDescriptorSets (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    VkPipelineBindPoint                         *)&pipelineBindPoint,
            *(    VkPipelineLayout                            *)&layout,
            *(    uint32_t                                    *)&firstSet,
            *(    uint32_t                                    *)&descriptorSetCount,
            *(    const VkDescriptorSet*                      *)&pDescriptorSets,
            *(    uint32_t                                    *)&dynamicOffsetCount,
            *(    const uint32_t*                             *)&pDynamicOffsets);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_bind_index_buffer(
    vk_command_buffer_t                                commandBuffer,
    vk_buffer_t                                        buffer,
    vk_device_size_t                                   offset,
    vk_index_type_t                                    indexType)
{
    return vkCmdBindIndexBuffer (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    VkBuffer                                    *)&buffer,
            *(    VkDeviceSize                                *)&offset,
            *(    VkIndexType                                 *)&indexType);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_bind_vertex_buffers(
    vk_command_buffer_t                                commandBuffer,
    uint32_t                                           firstBinding,
    uint32_t                                           bindingCount,
    const vk_buffer_t*                                 pBuffers,
    const vk_device_size_t*                            pOffsets)
{
    return vkCmdBindVertexBuffers (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    uint32_t                                    *)&firstBinding,
            *(    uint32_t                                    *)&bindingCount,
            *(    const VkBuffer*                             *)&pBuffers,
            *(    const VkDeviceSize*                         *)&pOffsets);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_draw(
    vk_command_buffer_t                                commandBuffer,
    uint32_t                                           vertexCount,
    uint32_t                                           instanceCount,
    uint32_t                                           firstVertex,
    uint32_t                                           firstInstance)
{
    return vkCmdDraw (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    uint32_t                                    *)&vertexCount,
            *(    uint32_t                                    *)&instanceCount,
            *(    uint32_t                                    *)&firstVertex,
            *(    uint32_t                                    *)&firstInstance);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_draw_indexed(
    vk_command_buffer_t                                commandBuffer,
    uint32_t                                           indexCount,
    uint32_t                                           instanceCount,
    uint32_t                                           firstIndex,
    int32_t                                            vertexOffset,
    uint32_t                                           firstInstance)
{
    return vkCmdDrawIndexed (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    uint32_t                                    *)&indexCount,
            *(    uint32_t                                    *)&instanceCount,
            *(    uint32_t                                    *)&firstIndex,
            *(    int32_t                                     *)&vertexOffset,
            *(    uint32_t                                    *)&firstInstance);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_draw_indirect(
    vk_command_buffer_t                                commandBuffer,
    vk_buffer_t                                        buffer,
    vk_device_size_t                                   offset,
    uint32_t                                           drawCount,
    uint32_t                                           stride)
{
    return vkCmdDrawIndirect (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    VkBuffer                                    *)&buffer,
            *(    VkDeviceSize                                *)&offset,
            *(    uint32_t                                    *)&drawCount,
            *(    uint32_t                                    *)&stride);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_draw_indexed_indirect(
    vk_command_buffer_t                                commandBuffer,
    vk_buffer_t                                        buffer,
    vk_device_size_t                                   offset,
    uint32_t                                           drawCount,
    uint32_t                                           stride)
{
    return vkCmdDrawIndexedIndirect (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    VkBuffer                                    *)&buffer,
            *(    VkDeviceSize                                *)&offset,
            *(    uint32_t                                    *)&drawCount,
            *(    uint32_t                                    *)&stride);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_dispatch(
    vk_command_buffer_t                                commandBuffer,
    uint32_t                                           groupCountX,
    uint32_t                                           groupCountY,
    uint32_t                                           groupCountZ)
{
    return vkCmdDispatch (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    uint32_t                                    *)&groupCountX,
            *(    uint32_t                                    *)&groupCountY,
            *(    uint32_t                                    *)&groupCountZ);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_dispatch_indirect(
    vk_command_buffer_t                                commandBuffer,
    vk_buffer_t                                        buffer,
    vk_device_size_t                                   offset)
{
    return vkCmdDispatchIndirect (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    VkBuffer                                    *)&buffer,
            *(    VkDeviceSize                                *)&offset);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_copy_buffer(
    vk_command_buffer_t                                commandBuffer,
    vk_buffer_t                                        srcBuffer,
    vk_buffer_t                                        dstBuffer,
    uint32_t                                           regionCount,
    const vk_buffer_copy_t*                            pRegions)
{
    return vkCmdCopyBuffer (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    VkBuffer                                    *)&srcBuffer,
            *(    VkBuffer                                    *)&dstBuffer,
            *(    uint32_t                                    *)&regionCount,
            *(    const VkBufferCopy*                         *)&pRegions);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_copy_image(
    vk_command_buffer_t                                commandBuffer,
    vk_image_t                                         srcImage,
    vk_image_layout_t                                  srcImageLayout,
    vk_image_t                                         dstImage,
    vk_image_layout_t                                  dstImageLayout,
    uint32_t                                           regionCount,
    const vk_image_copy_t*                             pRegions)
{
    return vkCmdCopyImage (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    VkImage                                     *)&srcImage,
            *(    VkImageLayout                               *)&srcImageLayout,
            *(    VkImage                                     *)&dstImage,
            *(    VkImageLayout                               *)&dstImageLayout,
            *(    uint32_t                                    *)&regionCount,
            *(    const VkImageCopy*                          *)&pRegions);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_blit_image(
    vk_command_buffer_t                                commandBuffer,
    vk_image_t                                         srcImage,
    vk_image_layout_t                                  srcImageLayout,
    vk_image_t                                         dstImage,
    vk_image_layout_t                                  dstImageLayout,
    uint32_t                                           regionCount,
    const vk_image_blit_t*                             pRegions,
    vk_filter_t                                        filter)
{
    return vkCmdBlitImage (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    VkImage                                     *)&srcImage,
            *(    VkImageLayout                               *)&srcImageLayout,
            *(    VkImage                                     *)&dstImage,
            *(    VkImageLayout                               *)&dstImageLayout,
            *(    uint32_t                                    *)&regionCount,
            *(    const VkImageBlit*                          *)&pRegions,
            *(    VkFilter                                    *)&filter);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_copy_buffer_to_image(
    vk_command_buffer_t                                commandBuffer,
    vk_buffer_t                                        srcBuffer,
    vk_image_t                                         dstImage,
    vk_image_layout_t                                  dstImageLayout,
    uint32_t                                           regionCount,
    const vk_buffer_image_copy_t*                      pRegions)
{
    return vkCmdCopyBufferToImage (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    VkBuffer                                    *)&srcBuffer,
            *(    VkImage                                     *)&dstImage,
            *(    VkImageLayout                               *)&dstImageLayout,
            *(    uint32_t                                    *)&regionCount,
            *(    const VkBufferImageCopy*                    *)&pRegions);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_copy_image_to_buffer(
    vk_command_buffer_t                                commandBuffer,
    vk_image_t                                         srcImage,
    vk_image_layout_t                                  srcImageLayout,
    vk_buffer_t                                        dstBuffer,
    uint32_t                                           regionCount,
    const vk_buffer_image_copy_t*                      pRegions)
{
    return vkCmdCopyImageToBuffer (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    VkImage                                     *)&srcImage,
            *(    VkImageLayout                               *)&srcImageLayout,
            *(    VkBuffer                                    *)&dstBuffer,
            *(    uint32_t                                    *)&regionCount,
            *(    const VkBufferImageCopy*                    *)&pRegions);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_update_buffer(
    vk_command_buffer_t                                commandBuffer,
    vk_buffer_t                                        dstBuffer,
    vk_device_size_t                                   dstOffset,
    vk_device_size_t                                   dataSize,
    const void*                                        pData)
{
    return vkCmdUpdateBuffer (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    VkBuffer                                    *)&dstBuffer,
            *(    VkDeviceSize                                *)&dstOffset,
            *(    VkDeviceSize                                *)&dataSize,
            *(    const void*                                 *)&pData);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_fill_buffer(
    vk_command_buffer_t                                commandBuffer,
    vk_buffer_t                                        dstBuffer,
    vk_device_size_t                                   dstOffset,
    vk_device_size_t                                   size,
    uint32_t                                           data)
{
    return vkCmdFillBuffer (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    VkBuffer                                    *)&dstBuffer,
            *(    VkDeviceSize                                *)&dstOffset,
            *(    VkDeviceSize                                *)&size,
            *(    uint32_t                                    *)&data);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_clear_color_image(
    vk_command_buffer_t                                commandBuffer,
    vk_image_t                                         image,
    vk_image_layout_t                                  imageLayout,
    const vk_clear_color_value_t*                      pColor,
    uint32_t                                           rangeCount,
    const vk_image_subresource_range_t*                pRanges)
{
    return vkCmdClearColorImage (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    VkImage                                     *)&image,
            *(    VkImageLayout                               *)&imageLayout,
            *(    const VkClearColorValue*                    *)&pColor,
            *(    uint32_t                                    *)&rangeCount,
            *(    const VkImageSubresourceRange*              *)&pRanges);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_clear_depth_stencil_image(
    vk_command_buffer_t                                commandBuffer,
    vk_image_t                                         image,
    vk_image_layout_t                                  imageLayout,
    const vk_clear_depth_stencil_value_t*              pDepthStencil,
    uint32_t                                           rangeCount,
    const vk_image_subresource_range_t*                pRanges)
{
    return vkCmdClearDepthStencilImage (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    VkImage                                     *)&image,
            *(    VkImageLayout                               *)&imageLayout,
            *(    const VkClearDepthStencilValue*             *)&pDepthStencil,
            *(    uint32_t                                    *)&rangeCount,
            *(    const VkImageSubresourceRange*              *)&pRanges);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_clear_attachments(
    vk_command_buffer_t                                commandBuffer,
    uint32_t                                           attachmentCount,
    const vk_clear_attachment_t*                       pAttachments,
    uint32_t                                           rectCount,
    const vk_clear_rect_t*                             pRects)
{
    return vkCmdClearAttachments (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    uint32_t                                    *)&attachmentCount,
            *(    const VkClearAttachment*                    *)&pAttachments,
            *(    uint32_t                                    *)&rectCount,
            *(    const VkClearRect*                          *)&pRects);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_resolve_image(
    vk_command_buffer_t                                commandBuffer,
    vk_image_t                                         srcImage,
    vk_image_layout_t                                  srcImageLayout,
    vk_image_t                                         dstImage,
    vk_image_layout_t                                  dstImageLayout,
    uint32_t                                           regionCount,
    const vk_image_resolve_t*                          pRegions)
{
    return vkCmdResolveImage (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    VkImage                                     *)&srcImage,
            *(    VkImageLayout                               *)&srcImageLayout,
            *(    VkImage                                     *)&dstImage,
            *(    VkImageLayout                               *)&dstImageLayout,
            *(    uint32_t                                    *)&regionCount,
            *(    const VkImageResolve*                       *)&pRegions);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_set_event(
    vk_command_buffer_t                                commandBuffer,
    vk_event_t                                         event,
    vk_pipeline_stage_flags_t                          stageMask)
{
    return vkCmdSetEvent (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    VkEvent                                     *)&event,
            *(    VkPipelineStageFlags                        *)&stageMask);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_reset_event(
    vk_command_buffer_t                                commandBuffer,
    vk_event_t                                         event,
    vk_pipeline_stage_flags_t                          stageMask)
{
    return vkCmdResetEvent (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    VkEvent                                     *)&event,
            *(    VkPipelineStageFlags                        *)&stageMask);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_wait_events(
    vk_command_buffer_t                                commandBuffer,
    uint32_t                                           eventCount,
    const vk_event_t*                                  pEvents,
    vk_pipeline_stage_flags_t                          srcStageMask,
    vk_pipeline_stage_flags_t                          dstStageMask,
    uint32_t                                           memoryBarrierCount,
    const vk_memory_barrier_t*                         pMemoryBarriers,
    uint32_t                                           bufferMemoryBarrierCount,
    const vk_buffer_memory_barrier_t*                  pBufferMemoryBarriers,
    uint32_t                                           imageMemoryBarrierCount,
    const vk_image_memory_barrier_t*                   pImageMemoryBarriers)
{
    return vkCmdWaitEvents (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    uint32_t                                    *)&eventCount,
            *(    const VkEvent*                              *)&pEvents,
            *(    VkPipelineStageFlags                        *)&srcStageMask,
            *(    VkPipelineStageFlags                        *)&dstStageMask,
            *(    uint32_t                                    *)&memoryBarrierCount,
            *(    const VkMemoryBarrier*                      *)&pMemoryBarriers,
            *(    uint32_t                                    *)&bufferMemoryBarrierCount,
            *(    const VkBufferMemoryBarrier*                *)&pBufferMemoryBarriers,
            *(    uint32_t                                    *)&imageMemoryBarrierCount,
            *(    const VkImageMemoryBarrier*                 *)&pImageMemoryBarriers);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_pipeline_barrier(
    vk_command_buffer_t                                commandBuffer,
    vk_pipeline_stage_flags_t                          srcStageMask,
    vk_pipeline_stage_flags_t                          dstStageMask,
    vk_dependency_flags_t                              dependencyFlags,
    uint32_t                                           memoryBarrierCount,
    const vk_memory_barrier_t*                         pMemoryBarriers,
    uint32_t                                           bufferMemoryBarrierCount,
    const vk_buffer_memory_barrier_t*                  pBufferMemoryBarriers,
    uint32_t                                           imageMemoryBarrierCount,
    const vk_image_memory_barrier_t*                   pImageMemoryBarriers)
{
    return vkCmdPipelineBarrier (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    VkPipelineStageFlags                        *)&srcStageMask,
            *(    VkPipelineStageFlags                        *)&dstStageMask,
            *(    VkDependencyFlags                           *)&dependencyFlags,
            *(    uint32_t                                    *)&memoryBarrierCount,
            *(    const VkMemoryBarrier*                      *)&pMemoryBarriers,
            *(    uint32_t                                    *)&bufferMemoryBarrierCount,
            *(    const VkBufferMemoryBarrier*                *)&pBufferMemoryBarriers,
            *(    uint32_t                                    *)&imageMemoryBarrierCount,
            *(    const VkImageMemoryBarrier*                 *)&pImageMemoryBarriers);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_begin_query(
    vk_command_buffer_t                                commandBuffer,
    vk_query_pool_t                                    queryPool,
    uint32_t                                           query,
    vk_query_control_flags_t                           flags)
{
    return vkCmdBeginQuery (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    VkQueryPool                                 *)&queryPool,
            *(    uint32_t                                    *)&query,
            *(    VkQueryControlFlags                         *)&flags);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_end_query(
    vk_command_buffer_t                                commandBuffer,
    vk_query_pool_t                                    queryPool,
    uint32_t                                           query)
{
    return vkCmdEndQuery (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    VkQueryPool                                 *)&queryPool,
            *(    uint32_t                                    *)&query);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_reset_query_pool(
    vk_command_buffer_t                                commandBuffer,
    vk_query_pool_t                                    queryPool,
    uint32_t                                           firstQuery,
    uint32_t                                           queryCount)
{
    return vkCmdResetQueryPool (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    VkQueryPool                                 *)&queryPool,
            *(    uint32_t                                    *)&firstQuery,
            *(    uint32_t                                    *)&queryCount);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_write_timestamp(
    vk_command_buffer_t                                commandBuffer,
    vk_pipeline_stage_flag_bits_t                      pipelineStage,
    vk_query_pool_t                                    queryPool,
    uint32_t                                           query)
{
    return vkCmdWriteTimestamp (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    VkPipelineStageFlagBits                     *)&pipelineStage,
            *(    VkQueryPool                                 *)&queryPool,
            *(    uint32_t                                    *)&query);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_copy_query_pool_results(
    vk_command_buffer_t                                commandBuffer,
    vk_query_pool_t                                    queryPool,
    uint32_t                                           firstQuery,
    uint32_t                                           queryCount,
    vk_buffer_t                                        dstBuffer,
    vk_device_size_t                                   dstOffset,
    vk_device_size_t                                   stride,
    vk_query_result_flags_t                            flags)
{
    return vkCmdCopyQueryPoolResults (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    VkQueryPool                                 *)&queryPool,
            *(    uint32_t                                    *)&firstQuery,
            *(    uint32_t                                    *)&queryCount,
            *(    VkBuffer                                    *)&dstBuffer,
            *(    VkDeviceSize                                *)&dstOffset,
            *(    VkDeviceSize                                *)&stride,
            *(    VkQueryResultFlags                          *)&flags);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_push_constants(
    vk_command_buffer_t                                commandBuffer,
    vk_pipeline_layout_t                               layout,
    vk_shader_stage_flags_t                            stageFlags,
    uint32_t                                           offset,
    uint32_t                                           size,
    const void*                                        pValues)
{
    return vkCmdPushConstants (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    VkPipelineLayout                            *)&layout,
            *(    VkShaderStageFlags                          *)&stageFlags,
            *(    uint32_t                                    *)&offset,
            *(    uint32_t                                    *)&size,
            *(    const void*                                 *)&pValues);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_begin_render_pass(
    vk_command_buffer_t                                commandBuffer,
    const vk_render_pass_begin_info_t*                 pRenderPassBegin,
    vk_subpass_contents_t                              contents)
{
    return vkCmdBeginRenderPass (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    const VkRenderPassBeginInfo*                *)&pRenderPassBegin,
            *(    VkSubpassContents                           *)&contents);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_next_subpass(
    vk_command_buffer_t                                commandBuffer,
    vk_subpass_contents_t                              contents)
{
    return vkCmdNextSubpass (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    VkSubpassContents                           *)&contents);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_end_render_pass(
    vk_command_buffer_t                                commandBuffer)
{
    return vkCmdEndRenderPass (
            *(    VkCommandBuffer                             *)&commandBuffer);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_execute_commands(
    vk_command_buffer_t                                commandBuffer,
    uint32_t                                           commandBufferCount,
    const vk_command_buffer_t*                         pCommandBuffers)
{
    return vkCmdExecuteCommands (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    uint32_t                                    *)&commandBufferCount,
            *(    const VkCommandBuffer*                      *)&pCommandBuffers);
}
#define vk_sampler_ycbcr_conversion_t                                     VkSamplerYcbcrConversion 
#define vk_descriptor_update_template_t                                   VkDescriptorUpdateTemplate 
#define vk_point_clipping_behavior_t                                      VkPointClippingBehavior
#define vk_tessellation_domain_origin_t                                   VkTessellationDomainOrigin
#define vk_sampler_ycbcr_model_conversion_t                               VkSamplerYcbcrModelConversion
#define vk_sampler_ycbcr_range_t                                          VkSamplerYcbcrRange
#define vk_chroma_location_t                                              VkChromaLocation
#define vk_descriptor_update_template_type_t                              VkDescriptorUpdateTemplateType
#define vk_subgroup_feature_flag_bits_t                                   VkSubgroupFeatureFlagBits
#define vk_subgroup_feature_flags_t                                       VkSubgroupFeatureFlags 
#define vk_peer_memory_feature_flag_bits_t                                VkPeerMemoryFeatureFlagBits
#define vk_peer_memory_feature_flags_t                                    VkPeerMemoryFeatureFlags 
#define vk_memory_allocate_flag_bits_t                                    VkMemoryAllocateFlagBits
#define vk_memory_allocate_flags_t                                        VkMemoryAllocateFlags 
#define vk_command_pool_trim_flags_t                                      VkCommandPoolTrimFlags 
#define vk_descriptor_update_template_create_flags_t                      VkDescriptorUpdateTemplateCreateFlags 
#define vk_external_memory_handle_type_flag_bits_t                        VkExternalMemoryHandleTypeFlagBits
#define vk_external_memory_handle_type_flags_t                            VkExternalMemoryHandleTypeFlags 
#define vk_external_memory_feature_flag_bits_t                            VkExternalMemoryFeatureFlagBits
#define vk_external_memory_feature_flags_t                                VkExternalMemoryFeatureFlags 
#define vk_external_fence_handle_type_flag_bits_t                         VkExternalFenceHandleTypeFlagBits
#define vk_external_fence_handle_type_flags_t                             VkExternalFenceHandleTypeFlags 
#define vk_external_fence_feature_flag_bits_t                             VkExternalFenceFeatureFlagBits
#define vk_external_fence_feature_flags_t                                 VkExternalFenceFeatureFlags 
#define vk_fence_import_flag_bits_t                                       VkFenceImportFlagBits
#define vk_fence_import_flags_t                                           VkFenceImportFlags 
#define vk_semaphore_import_flag_bits_t                                   VkSemaphoreImportFlagBits
#define vk_semaphore_import_flags_t                                       VkSemaphoreImportFlags 
#define vk_external_semaphore_handle_type_flag_bits_t                     VkExternalSemaphoreHandleTypeFlagBits
#define vk_external_semaphore_handle_type_flags_t                         VkExternalSemaphoreHandleTypeFlags 
#define vk_external_semaphore_feature_flag_bits_t                         VkExternalSemaphoreFeatureFlagBits
#define vk_external_semaphore_feature_flags_t                             VkExternalSemaphoreFeatureFlags 

typedef struct vk_physical_device_subgroup_properties_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES;
    void* p_next =                                     NULL;
    uint32_t                                           subgroup_size;
    vk_shader_stage_flags_t                            supported_stages;
    vk_subgroup_feature_flags_t                        supported_operations;
    vk_bool32_t                                        quad_operations_in_all_stages;
} vk_physical_device_subgroup_properties_t;

typedef struct vk_bind_buffer_memory_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO;
    const void* p_next =                               NULL;
    vk_buffer_t                                        buffer;
    vk_device_memory_t                                 memory;
    vk_device_size_t                                   memory_offset;
} vk_bind_buffer_memory_info_t;

typedef struct vk_bind_image_memory_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO;
    const void* p_next =                               NULL;
    vk_image_t                                         image;
    vk_device_memory_t                                 memory;
    vk_device_size_t                                   memory_offset;
} vk_bind_image_memory_info_t;

typedef struct vk_physical_device16_bit_storage_features_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES;
    void* p_next =                                     NULL;
    vk_bool32_t                                        storage_buffer16_bit_access;
    vk_bool32_t                                        uniform_and_storage_buffer16_bit_access;
    vk_bool32_t                                        storage_push_constant16;
    vk_bool32_t                                        storage_input_output16;
} vk_physical_device16_bit_storage_features_t;

typedef struct vk_memory_dedicated_requirements_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS;
    void* p_next =                                     NULL;
    vk_bool32_t                                        prefers_dedicated_allocation;
    vk_bool32_t                                        requires_dedicated_allocation;
} vk_memory_dedicated_requirements_t;

typedef struct vk_memory_dedicated_allocate_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO;
    const void* p_next =                               NULL;
    vk_image_t                                         image;
    vk_buffer_t                                        buffer;
} vk_memory_dedicated_allocate_info_t;

typedef struct vk_memory_allocate_flags_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
    const void* p_next =                               NULL;
    vk_memory_allocate_flags_t                         flags;
    uint32_t                                           device_mask;
} vk_memory_allocate_flags_info_t;

typedef struct vk_device_group_render_pass_begin_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_DEVICE_GROUP_RENDER_PASS_BEGIN_INFO;
    const void* p_next =                               NULL;
    uint32_t                                           device_mask;
    uint32_t                                           device_render_area_count;
    const vk_rect2d_t*                                p_device_render_areas;
} vk_device_group_render_pass_begin_info_t;

typedef struct vk_device_group_command_buffer_begin_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_DEVICE_GROUP_COMMAND_BUFFER_BEGIN_INFO;
    const void* p_next =                               NULL;
    uint32_t                                           device_mask;
} vk_device_group_command_buffer_begin_info_t;

typedef struct vk_device_group_submit_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_DEVICE_GROUP_SUBMIT_INFO;
    const void* p_next =                               NULL;
    uint32_t                                           wait_semaphore_count;
    const uint32_t*                                    p_wait_semaphore_device_indices;
    uint32_t                                           command_buffer_count;
    const uint32_t*                                    p_command_buffer_device_masks;
    uint32_t                                           signal_semaphore_count;
    const uint32_t*                                    p_signal_semaphore_device_indices;
} vk_device_group_submit_info_t;

typedef struct vk_device_group_bind_sparse_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_DEVICE_GROUP_BIND_SPARSE_INFO;
    const void* p_next =                               NULL;
    uint32_t                                           resource_device_index;
    uint32_t                                           memory_device_index;
} vk_device_group_bind_sparse_info_t;

typedef struct vk_bind_buffer_memory_device_group_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_DEVICE_GROUP_INFO;
    const void* p_next =                               NULL;
    uint32_t                                           device_index_count;
    const uint32_t*                                    p_device_indices;
} vk_bind_buffer_memory_device_group_info_t;

typedef struct vk_bind_image_memory_device_group_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_DEVICE_GROUP_INFO;
    const void* p_next =                               NULL;
    uint32_t                                           device_index_count;
    const uint32_t*                                    p_device_indices;
    uint32_t                                           split_instance_bind_region_count;
    const vk_rect2d_t*                                p_split_instance_bind_regions;
} vk_bind_image_memory_device_group_info_t;

typedef struct vk_physical_device_group_properties_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GROUP_PROPERTIES;
    void* p_next =                                     NULL;
    uint32_t                                           physical_device_count;
    vk_physical_device_t                               physical_devices[VK_MAX_DEVICE_GROUP_SIZE];
    vk_bool32_t                                        subset_allocation;
} vk_physical_device_group_properties_t;

typedef struct vk_device_group_device_create_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_DEVICE_GROUP_DEVICE_CREATE_INFO;
    const void* p_next =                               NULL;
    uint32_t                                           physical_device_count;
    const vk_physical_device_t*                        p_physical_devices;
} vk_device_group_device_create_info_t;

typedef struct vk_buffer_memory_requirements_info2_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2;
    const void* p_next =                               NULL;
    vk_buffer_t                                        buffer;
} vk_buffer_memory_requirements_info2_t;

typedef struct vk_image_memory_requirements_info2_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2;
    const void* p_next =                               NULL;
    vk_image_t                                         image;
} vk_image_memory_requirements_info2_t;

typedef struct vk_image_sparse_memory_requirements_info2_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_IMAGE_SPARSE_MEMORY_REQUIREMENTS_INFO_2;
    const void* p_next =                               NULL;
    vk_image_t                                         image;
} vk_image_sparse_memory_requirements_info2_t;

typedef struct vk_memory_requirements2_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
    void* p_next =                                     NULL;
    vk_memory_requirements_t                           memory_requirements;
} vk_memory_requirements2_t;
#define vk_memory_requirements2_khr_t                                     VkMemoryRequirements2KHR 

typedef struct vk_sparse_image_memory_requirements2_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_SPARSE_IMAGE_MEMORY_REQUIREMENTS_2;
    void* p_next =                                     NULL;
    vk_sparse_image_memory_requirements_t              memory_requirements;
} vk_sparse_image_memory_requirements2_t;

typedef struct vk_physical_device_features2_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    void* p_next =                                     NULL;
    vk_physical_device_features_t                      features;
} vk_physical_device_features2_t;

typedef struct vk_physical_device_properties2_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    void* p_next =                                     NULL;
    vk_physical_device_properties_t                    properties;
} vk_physical_device_properties2_t;

typedef struct vk_format_properties2_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2;
    void* p_next =                                     NULL;
    vk_format_properties_t                             format_properties;
} vk_format_properties2_t;

typedef struct vk_image_format_properties2_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2;
    void* p_next =                                     NULL;
    vk_image_format_properties_t                       image_format_properties;
} vk_image_format_properties2_t;

typedef struct vk_physical_device_image_format_info2_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2;
    const void* p_next =                               NULL;
    vk_format_t                                        format;
    vk_image_type_t                                    type;
    vk_image_tiling_t                                  tiling;
    vk_image_usage_flags_t                             usage;
    vk_image_create_flags_t                            flags;
} vk_physical_device_image_format_info2_t;

typedef struct vk_queue_family_properties2_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
    void* p_next =                                     NULL;
    vk_queue_family_properties_t                       queue_family_properties;
} vk_queue_family_properties2_t;

typedef struct vk_physical_device_memory_properties2_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
    void* p_next =                                     NULL;
    vk_physical_device_memory_properties_t             memory_properties;
} vk_physical_device_memory_properties2_t;

typedef struct vk_sparse_image_format_properties2_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_SPARSE_IMAGE_FORMAT_PROPERTIES_2;
    void* p_next =                                     NULL;
    vk_sparse_image_format_properties_t                properties;
} vk_sparse_image_format_properties2_t;

typedef struct vk_physical_device_sparse_image_format_info2_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SPARSE_IMAGE_FORMAT_INFO_2;
    const void* p_next =                               NULL;
    vk_format_t                                        format;
    vk_image_type_t                                    type;
    vk_sample_count_flag_bits_t                        samples;
    vk_image_usage_flags_t                             usage;
    vk_image_tiling_t                                  tiling;
} vk_physical_device_sparse_image_format_info2_t;

typedef struct vk_physical_device_point_clipping_properties_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_POINT_CLIPPING_PROPERTIES;
    void* p_next =                                     NULL;
    vk_point_clipping_behavior_t                       point_clipping_behavior;
} vk_physical_device_point_clipping_properties_t;

typedef struct vk_input_attachment_aspect_reference_t {
    uint32_t                                           subpass;
    uint32_t                                           input_attachment_index;
    vk_image_aspect_flags_t                            aspect_mask;
} vk_input_attachment_aspect_reference_t;

typedef struct vk_render_pass_input_attachment_aspect_create_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_RENDER_PASS_INPUT_ATTACHMENT_ASPECT_CREATE_INFO;
    const void* p_next =                               NULL;
    uint32_t                                           aspect_reference_count;
    const vk_input_attachment_aspect_reference_t*      p_aspect_references;
} vk_render_pass_input_attachment_aspect_create_info_t;

typedef struct vk_image_view_usage_create_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO;
    const void* p_next =                               NULL;
    vk_image_usage_flags_t                             usage;
} vk_image_view_usage_create_info_t;

typedef struct vk_pipeline_tessellation_domain_origin_state_create_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_DOMAIN_ORIGIN_STATE_CREATE_INFO;
    const void* p_next =                               NULL;
    vk_tessellation_domain_origin_t                    domain_origin;
} vk_pipeline_tessellation_domain_origin_state_create_info_t;

typedef struct vk_render_pass_multiview_create_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_CREATE_INFO;
    const void* p_next =                               NULL;
    uint32_t                                           subpass_count;
    const uint32_t*                                    p_view_masks;
    uint32_t                                           dependency_count;
    const int32_t*                                     p_view_offsets;
    uint32_t                                           correlation_mask_count;
    const uint32_t*                                    p_correlation_masks;
} vk_render_pass_multiview_create_info_t;

typedef struct vk_physical_device_multiview_features_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES;
    void* p_next =                                     NULL;
    vk_bool32_t                                        multiview;
    vk_bool32_t                                        multiview_geometry_shader;
    vk_bool32_t                                        multiview_tessellation_shader;
} vk_physical_device_multiview_features_t;

typedef struct vk_physical_device_multiview_properties_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PROPERTIES;
    void* p_next =                                     NULL;
    uint32_t                                           max_multiview_view_count;
    uint32_t                                           max_multiview_instance_index;
} vk_physical_device_multiview_properties_t;

typedef struct vk_physical_device_variable_pointers_features_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VARIABLE_POINTERS_FEATURES;
    void* p_next =                                     NULL;
    vk_bool32_t                                        variable_pointers_storage_buffer;
    vk_bool32_t                                        variable_pointers;
} vk_physical_device_variable_pointers_features_t;
#define vk_physical_device_variable_pointer_features_t                    VkPhysicalDeviceVariablePointerFeatures 

typedef struct vk_physical_device_protected_memory_features_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROTECTED_MEMORY_FEATURES;
    void* p_next =                                     NULL;
    vk_bool32_t                                        protected_memory;
} vk_physical_device_protected_memory_features_t;

typedef struct vk_physical_device_protected_memory_properties_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROTECTED_MEMORY_PROPERTIES;
    void* p_next =                                     NULL;
    vk_bool32_t                                        protected_no_fault;
} vk_physical_device_protected_memory_properties_t;

typedef struct vk_device_queue_info2_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2;
    const void* p_next =                               NULL;
    vk_device_queue_create_flags_t                     flags;
    uint32_t                                           queue_family_index;
    uint32_t                                           queue_index;
} vk_device_queue_info2_t;

typedef struct vk_protected_submit_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PROTECTED_SUBMIT_INFO;
    const void* p_next =                               NULL;
    vk_bool32_t                                        protected_submit;
} vk_protected_submit_info_t;

typedef struct vk_sampler_ycbcr_conversion_create_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_CREATE_INFO;
    const void* p_next =                               NULL;
    vk_format_t                                        format;
    vk_sampler_ycbcr_model_conversion_t                ycbcr_model;
    vk_sampler_ycbcr_range_t                           ycbcr_range;
    vk_component_mapping_t                             components;
    vk_chroma_location_t                               x_chroma_offset;
    vk_chroma_location_t                               y_chroma_offset;
    vk_filter_t                                        chroma_filter;
    vk_bool32_t                                        force_explicit_reconstruction;
} vk_sampler_ycbcr_conversion_create_info_t;

typedef struct vk_sampler_ycbcr_conversion_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_INFO;
    const void* p_next =                               NULL;
    vk_sampler_ycbcr_conversion_t                      conversion;
} vk_sampler_ycbcr_conversion_info_t;

typedef struct vk_bind_image_plane_memory_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_BIND_IMAGE_PLANE_MEMORY_INFO;
    const void* p_next =                               NULL;
    vk_image_aspect_flag_bits_t                        plane_aspect;
} vk_bind_image_plane_memory_info_t;

typedef struct vk_image_plane_memory_requirements_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_IMAGE_PLANE_MEMORY_REQUIREMENTS_INFO;
    const void* p_next =                               NULL;
    vk_image_aspect_flag_bits_t                        plane_aspect;
} vk_image_plane_memory_requirements_info_t;

typedef struct vk_physical_device_sampler_ycbcr_conversion_features_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_YCBCR_CONVERSION_FEATURES;
    void* p_next =                                     NULL;
    vk_bool32_t                                        sampler_ycbcr_conversion;
} vk_physical_device_sampler_ycbcr_conversion_features_t;

typedef struct vk_sampler_ycbcr_conversion_image_format_properties_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_IMAGE_FORMAT_PROPERTIES;
    void* p_next =                                     NULL;
    uint32_t                                           combined_image_sampler_descriptor_count;
} vk_sampler_ycbcr_conversion_image_format_properties_t;

typedef struct vk_descriptor_update_template_entry_t {
    uint32_t                                           dst_binding;
    uint32_t                                           dst_array_element;
    uint32_t                                           descriptor_count;
    vk_descriptor_type_t                               descriptor_type;
    size_t                                             offset;
    size_t                                             stride;
} vk_descriptor_update_template_entry_t;

typedef struct vk_descriptor_update_template_create_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_CREATE_INFO;
    const void* p_next =                               NULL;
    vk_descriptor_update_template_create_flags_t       flags;
    uint32_t                                           descriptor_update_entry_count;
    const vk_descriptor_update_template_entry_t*       p_descriptor_update_entries;
    vk_descriptor_update_template_type_t               template_type;
    vk_descriptor_set_layout_t                         descriptor_set_layout;
    vk_pipeline_bind_point_t                           pipeline_bind_point;
    vk_pipeline_layout_t                               pipeline_layout;
    uint32_t                                           set;
} vk_descriptor_update_template_create_info_t;

typedef struct vk_external_memory_properties_t {
    vk_external_memory_feature_flags_t                 external_memory_features;
    vk_external_memory_handle_type_flags_t             export_from_imported_handle_types;
    vk_external_memory_handle_type_flags_t             compatible_handle_types;
} vk_external_memory_properties_t;

typedef struct vk_physical_device_external_image_format_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO;
    const void* p_next =                               NULL;
    vk_external_memory_handle_type_flag_bits_t         handle_type;
} vk_physical_device_external_image_format_info_t;

typedef struct vk_external_image_format_properties_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES;
    void* p_next =                                     NULL;
    vk_external_memory_properties_t                    external_memory_properties;
} vk_external_image_format_properties_t;

typedef struct vk_physical_device_external_buffer_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_BUFFER_INFO;
    const void* p_next =                               NULL;
    vk_buffer_create_flags_t                           flags;
    vk_buffer_usage_flags_t                            usage;
    vk_external_memory_handle_type_flag_bits_t         handle_type;
} vk_physical_device_external_buffer_info_t;

typedef struct vk_external_buffer_properties_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_EXTERNAL_BUFFER_PROPERTIES;
    void* p_next =                                     NULL;
    vk_external_memory_properties_t                    external_memory_properties;
} vk_external_buffer_properties_t;

typedef struct vk_physical_device_id_properties_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;
    void* p_next =                                     NULL;
    uint8_t                                            device_uuid[VK_UUID_SIZE];
    uint8_t                                            driver_uuid[VK_UUID_SIZE];
    uint8_t                                            device_luid[VK_LUID_SIZE];
    uint32_t                                           device_node_mask;
    vk_bool32_t                                        device_luid_valid;
} vk_physical_device_id_properties_t;

typedef struct vk_external_memory_image_create_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
    const void* p_next =                               NULL;
    vk_external_memory_handle_type_flags_t             handle_types;
} vk_external_memory_image_create_info_t;

typedef struct vk_external_memory_buffer_create_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO;
    const void* p_next =                               NULL;
    vk_external_memory_handle_type_flags_t             handle_types;
} vk_external_memory_buffer_create_info_t;

typedef struct vk_export_memory_allocate_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
    const void* p_next =                               NULL;
    vk_external_memory_handle_type_flags_t             handle_types;
} vk_export_memory_allocate_info_t;

typedef struct vk_physical_device_external_fence_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_FENCE_INFO;
    const void* p_next =                               NULL;
    vk_external_fence_handle_type_flag_bits_t          handle_type;
} vk_physical_device_external_fence_info_t;

typedef struct vk_external_fence_properties_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_EXTERNAL_FENCE_PROPERTIES;
    void* p_next =                                     NULL;
    vk_external_fence_handle_type_flags_t              export_from_imported_handle_types;
    vk_external_fence_handle_type_flags_t              compatible_handle_types;
    vk_external_fence_feature_flags_t                  external_fence_features;
} vk_external_fence_properties_t;

typedef struct vk_export_fence_create_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_EXPORT_FENCE_CREATE_INFO;
    const void* p_next =                               NULL;
    vk_external_fence_handle_type_flags_t              handle_types;
} vk_export_fence_create_info_t;

typedef struct vk_export_semaphore_create_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO;
    const void* p_next =                               NULL;
    vk_external_semaphore_handle_type_flags_t          handle_types;
} vk_export_semaphore_create_info_t;

typedef struct vk_physical_device_external_semaphore_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_SEMAPHORE_INFO;
    const void* p_next =                               NULL;
    vk_external_semaphore_handle_type_flag_bits_t      handle_type;
} vk_physical_device_external_semaphore_info_t;

typedef struct vk_external_semaphore_properties_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_PROPERTIES;
    void* p_next =                                     NULL;
    vk_external_semaphore_handle_type_flags_t          export_from_imported_handle_types;
    vk_external_semaphore_handle_type_flags_t          compatible_handle_types;
    vk_external_semaphore_feature_flags_t              external_semaphore_features;
} vk_external_semaphore_properties_t;

typedef struct vk_physical_device_maintenance3_properties_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_3_PROPERTIES;
    void* p_next =                                     NULL;
    uint32_t                                           max_per_set_descriptors;
    vk_device_size_t                                   max_memory_allocation_size;
} vk_physical_device_maintenance3_properties_t;

typedef struct vk_descriptor_set_layout_support_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_SUPPORT;
    void* p_next =                                     NULL;
    vk_bool32_t                                        supported;
} vk_descriptor_set_layout_support_t;

typedef struct vk_physical_device_shader_draw_parameters_features_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES;
    void* p_next =                                     NULL;
    vk_bool32_t                                        shader_draw_parameters;
} vk_physical_device_shader_draw_parameters_features_t;
#define vk_physical_device_shader_draw_parameter_features_t               VkPhysicalDeviceShaderDrawParameterFeatures 
#define pfn_vk_enumerate_instance_version_fn_t                            PFN_vkEnumerateInstanceVersion
#define pfn_vk_bind_buffer_memory2_fn_t                                   PFN_vkBindBufferMemory2
#define pfn_vk_bind_image_memory2_fn_t                                    PFN_vkBindImageMemory2
#define pfn_vk_get_device_group_peer_memory_features_fn_t                 PFN_vkGetDeviceGroupPeerMemoryFeatures
#define pfn_vk_cmd_set_device_mask_fn_t                                   PFN_vkCmdSetDeviceMask
#define pfn_vk_cmd_dispatch_base_fn_t                                     PFN_vkCmdDispatchBase
#define pfn_vk_enumerate_physical_device_groups_fn_t                      PFN_vkEnumeratePhysicalDeviceGroups
#define pfn_vk_get_image_memory_requirements2_fn_t                        PFN_vkGetImageMemoryRequirements2
#define pfn_vk_get_buffer_memory_requirements2_fn_t                       PFN_vkGetBufferMemoryRequirements2
#define pfn_vk_get_image_sparse_memory_requirements2_fn_t                 PFN_vkGetImageSparseMemoryRequirements2
#define pfn_vk_get_physical_device_features2_fn_t                         PFN_vkGetPhysicalDeviceFeatures2
#define pfn_vk_get_physical_device_properties2_fn_t                       PFN_vkGetPhysicalDeviceProperties2
#define pfn_vk_get_physical_device_format_properties2_fn_t                PFN_vkGetPhysicalDeviceFormatProperties2
#define pfn_vk_get_physical_device_image_format_properties2_fn_t          PFN_vkGetPhysicalDeviceImageFormatProperties2
#define pfn_vk_get_physical_device_queue_family_properties2_fn_t          PFN_vkGetPhysicalDeviceQueueFamilyProperties2
#define pfn_vk_get_physical_device_memory_properties2_fn_t                PFN_vkGetPhysicalDeviceMemoryProperties2
#define pfn_vk_get_physical_device_sparse_image_format_properties2_fn_t   PFN_vkGetPhysicalDeviceSparseImageFormatProperties2
#define pfn_vk_trim_command_pool_fn_t                                     PFN_vkTrimCommandPool
#define pfn_vk_get_device_queue2_fn_t                                     PFN_vkGetDeviceQueue2
#define pfn_vk_create_sampler_ycbcr_conversion_fn_t                       PFN_vkCreateSamplerYcbcrConversion
#define pfn_vk_destroy_sampler_ycbcr_conversion_fn_t                      PFN_vkDestroySamplerYcbcrConversion
#define pfn_vk_create_descriptor_update_template_fn_t                     PFN_vkCreateDescriptorUpdateTemplate
#define pfn_vk_destroy_descriptor_update_template_fn_t                    PFN_vkDestroyDescriptorUpdateTemplate
#define pfn_vk_update_descriptor_set_with_template_fn_t                   PFN_vkUpdateDescriptorSetWithTemplate
#define pfn_vk_get_physical_device_external_buffer_properties_fn_t        PFN_vkGetPhysicalDeviceExternalBufferProperties
#define pfn_vk_get_physical_device_external_fence_properties_fn_t         PFN_vkGetPhysicalDeviceExternalFenceProperties
#define pfn_vk_get_physical_device_external_semaphore_properties_fn_t     PFN_vkGetPhysicalDeviceExternalSemaphoreProperties
#define pfn_vk_get_descriptor_set_layout_support_fn_t                     PFN_vkGetDescriptorSetLayoutSupport

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_enumerate_instance_version(
    uint32_t*                                          pApiVersion)
{
    return vkEnumerateInstanceVersion (
            *(    uint32_t*                                   *)&pApiVersion);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_bind_buffer_memory2(
    vk_device_t                                        device,
    uint32_t                                           bindInfoCount,
    const vk_bind_buffer_memory_info_t*                pBindInfos)
{
    return vkBindBufferMemory2 (
            *(    VkDevice                                    *)&device,
            *(    uint32_t                                    *)&bindInfoCount,
            *(    const VkBindBufferMemoryInfo*               *)&pBindInfos);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_bind_image_memory2(
    vk_device_t                                        device,
    uint32_t                                           bindInfoCount,
    const vk_bind_image_memory_info_t*                 pBindInfos)
{
    return vkBindImageMemory2 (
            *(    VkDevice                                    *)&device,
            *(    uint32_t                                    *)&bindInfoCount,
            *(    const VkBindImageMemoryInfo*                *)&pBindInfos);
}

inline VKAPI_ATTR void VKAPI_CALL vk_get_device_group_peer_memory_features(
    vk_device_t                                        device,
    uint32_t                                           heapIndex,
    uint32_t                                           localDeviceIndex,
    uint32_t                                           remoteDeviceIndex,
    vk_peer_memory_feature_flags_t*                    pPeerMemoryFeatures)
{
    return vkGetDeviceGroupPeerMemoryFeatures (
            *(    VkDevice                                    *)&device,
            *(    uint32_t                                    *)&heapIndex,
            *(    uint32_t                                    *)&localDeviceIndex,
            *(    uint32_t                                    *)&remoteDeviceIndex,
            *(    VkPeerMemoryFeatureFlags*                   *)&pPeerMemoryFeatures);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_set_device_mask(
    vk_command_buffer_t                                commandBuffer,
    uint32_t                                           deviceMask)
{
    return vkCmdSetDeviceMask (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    uint32_t                                    *)&deviceMask);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_dispatch_base(
    vk_command_buffer_t                                commandBuffer,
    uint32_t                                           baseGroupX,
    uint32_t                                           baseGroupY,
    uint32_t                                           baseGroupZ,
    uint32_t                                           groupCountX,
    uint32_t                                           groupCountY,
    uint32_t                                           groupCountZ)
{
    return vkCmdDispatchBase (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    uint32_t                                    *)&baseGroupX,
            *(    uint32_t                                    *)&baseGroupY,
            *(    uint32_t                                    *)&baseGroupZ,
            *(    uint32_t                                    *)&groupCountX,
            *(    uint32_t                                    *)&groupCountY,
            *(    uint32_t                                    *)&groupCountZ);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_enumerate_physical_device_groups(
    vk_instance_t                                      instance,
    uint32_t*                                          pPhysicalDeviceGroupCount,
    vk_physical_device_group_properties_t*             pPhysicalDeviceGroupProperties)
{
    return vkEnumeratePhysicalDeviceGroups (
            *(    VkInstance                                  *)&instance,
            *(    uint32_t*                                   *)&pPhysicalDeviceGroupCount,
            *(    VkPhysicalDeviceGroupProperties*            *)&pPhysicalDeviceGroupProperties);
}

inline VKAPI_ATTR void VKAPI_CALL vk_get_image_memory_requirements2(
    vk_device_t                                        device,
    const vk_image_memory_requirements_info2_t*        pInfo,
    vk_memory_requirements2_t*                         pMemoryRequirements)
{
    return vkGetImageMemoryRequirements2 (
            *(    VkDevice                                    *)&device,
            *(    const VkImageMemoryRequirementsInfo2*       *)&pInfo,
            *(    VkMemoryRequirements2*                      *)&pMemoryRequirements);
}

inline VKAPI_ATTR void VKAPI_CALL vk_get_buffer_memory_requirements2(
    vk_device_t                                        device,
    const vk_buffer_memory_requirements_info2_t*       pInfo,
    vk_memory_requirements2_t*                         pMemoryRequirements)
{
    return vkGetBufferMemoryRequirements2 (
            *(    VkDevice                                    *)&device,
            *(    const VkBufferMemoryRequirementsInfo2*      *)&pInfo,
            *(    VkMemoryRequirements2*                      *)&pMemoryRequirements);
}

inline VKAPI_ATTR void VKAPI_CALL vk_get_image_sparse_memory_requirements2(
    vk_device_t                                        device,
    const vk_image_sparse_memory_requirements_info2_t* pInfo,
    uint32_t*                                          pSparseMemoryRequirementCount,
    vk_sparse_image_memory_requirements2_t*            pSparseMemoryRequirements)
{
    return vkGetImageSparseMemoryRequirements2 (
            *(    VkDevice                                    *)&device,
            *(    const VkImageSparseMemoryRequirementsInfo2* *)&pInfo,
            *(    uint32_t*                                   *)&pSparseMemoryRequirementCount,
            *(    VkSparseImageMemoryRequirements2*           *)&pSparseMemoryRequirements);
}

inline VKAPI_ATTR void VKAPI_CALL vk_get_physical_device_features2(
    vk_physical_device_t                               physicalDevice,
    vk_physical_device_features2_t*                    pFeatures)
{
    return vkGetPhysicalDeviceFeatures2 (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    VkPhysicalDeviceFeatures2*                  *)&pFeatures);
}

inline VKAPI_ATTR void VKAPI_CALL vk_get_physical_device_properties2(
    vk_physical_device_t                               physicalDevice,
    vk_physical_device_properties2_t*                  pProperties)
{
    return vkGetPhysicalDeviceProperties2 (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    VkPhysicalDeviceProperties2*                *)&pProperties);
}

inline VKAPI_ATTR void VKAPI_CALL vk_get_physical_device_format_properties2(
    vk_physical_device_t                               physicalDevice,
    vk_format_t                                        format,
    vk_format_properties2_t*                           pFormatProperties)
{
    return vkGetPhysicalDeviceFormatProperties2 (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    VkFormat                                    *)&format,
            *(    VkFormatProperties2*                        *)&pFormatProperties);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_physical_device_image_format_properties2(
    vk_physical_device_t                               physicalDevice,
    const vk_physical_device_image_format_info2_t*     pImageFormatInfo,
    vk_image_format_properties2_t*                     pImageFormatProperties)
{
    return vkGetPhysicalDeviceImageFormatProperties2 (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    const VkPhysicalDeviceImageFormatInfo2*     *)&pImageFormatInfo,
            *(    VkImageFormatProperties2*                   *)&pImageFormatProperties);
}

inline VKAPI_ATTR void VKAPI_CALL vk_get_physical_device_queue_family_properties2(
    vk_physical_device_t                               physicalDevice,
    uint32_t*                                          pQueueFamilyPropertyCount,
    vk_queue_family_properties2_t*                     pQueueFamilyProperties)
{
    return vkGetPhysicalDeviceQueueFamilyProperties2 (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    uint32_t*                                   *)&pQueueFamilyPropertyCount,
            *(    VkQueueFamilyProperties2*                   *)&pQueueFamilyProperties);
}

inline VKAPI_ATTR void VKAPI_CALL vk_get_physical_device_memory_properties2(
    vk_physical_device_t                               physicalDevice,
    vk_physical_device_memory_properties2_t*           pMemoryProperties)
{
    return vkGetPhysicalDeviceMemoryProperties2 (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    VkPhysicalDeviceMemoryProperties2*          *)&pMemoryProperties);
}

inline VKAPI_ATTR void VKAPI_CALL vk_get_physical_device_sparse_image_format_properties2(
    vk_physical_device_t                               physicalDevice,
    const vk_physical_device_sparse_image_format_info2_t* pFormatInfo,
    uint32_t*                                          pPropertyCount,
    vk_sparse_image_format_properties2_t*              pProperties)
{
    return vkGetPhysicalDeviceSparseImageFormatProperties2 (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    const VkPhysicalDeviceSparseImageFormatInfo2* *)&pFormatInfo,
            *(    uint32_t*                                   *)&pPropertyCount,
            *(    VkSparseImageFormatProperties2*             *)&pProperties);
}

inline VKAPI_ATTR void VKAPI_CALL vk_trim_command_pool(
    vk_device_t                                        device,
    vk_command_pool_t                                  commandPool,
    vk_command_pool_trim_flags_t                       flags)
{
    return vkTrimCommandPool (
            *(    VkDevice                                    *)&device,
            *(    VkCommandPool                               *)&commandPool,
            *(    VkCommandPoolTrimFlags                      *)&flags);
}

inline VKAPI_ATTR void VKAPI_CALL vk_get_device_queue2(
    vk_device_t                                        device,
    const vk_device_queue_info2_t*                     pQueueInfo,
    vk_queue_t*                                        pQueue)
{
    return vkGetDeviceQueue2 (
            *(    VkDevice                                    *)&device,
            *(    const VkDeviceQueueInfo2*                   *)&pQueueInfo,
            *(    VkQueue*                                    *)&pQueue);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_create_sampler_ycbcr_conversion(
    vk_device_t                                        device,
    const vk_sampler_ycbcr_conversion_create_info_t*   pCreateInfo,
    const vk_allocation_callbacks_t*                   pAllocator,
    vk_sampler_ycbcr_conversion_t*                     pYcbcrConversion)
{
    return vkCreateSamplerYcbcrConversion (
            *(    VkDevice                                    *)&device,
            *(    const VkSamplerYcbcrConversionCreateInfo*   *)&pCreateInfo,
            *(    const VkAllocationCallbacks*                *)&pAllocator,
            *(    VkSamplerYcbcrConversion*                   *)&pYcbcrConversion);
}

inline VKAPI_ATTR void VKAPI_CALL vk_destroy_sampler_ycbcr_conversion(
    vk_device_t                                        device,
    vk_sampler_ycbcr_conversion_t                      ycbcrConversion,
    const vk_allocation_callbacks_t*                   pAllocator)
{
    return vkDestroySamplerYcbcrConversion (
            *(    VkDevice                                    *)&device,
            *(    VkSamplerYcbcrConversion                    *)&ycbcrConversion,
            *(    const VkAllocationCallbacks*                *)&pAllocator);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_create_descriptor_update_template(
    vk_device_t                                        device,
    const vk_descriptor_update_template_create_info_t* pCreateInfo,
    const vk_allocation_callbacks_t*                   pAllocator,
    vk_descriptor_update_template_t*                   pDescriptorUpdateTemplate)
{
    return vkCreateDescriptorUpdateTemplate (
            *(    VkDevice                                    *)&device,
            *(    const VkDescriptorUpdateTemplateCreateInfo* *)&pCreateInfo,
            *(    const VkAllocationCallbacks*                *)&pAllocator,
            *(    VkDescriptorUpdateTemplate*                 *)&pDescriptorUpdateTemplate);
}

inline VKAPI_ATTR void VKAPI_CALL vk_destroy_descriptor_update_template(
    vk_device_t                                        device,
    vk_descriptor_update_template_t                    descriptorUpdateTemplate,
    const vk_allocation_callbacks_t*                   pAllocator)
{
    return vkDestroyDescriptorUpdateTemplate (
            *(    VkDevice                                    *)&device,
            *(    VkDescriptorUpdateTemplate                  *)&descriptorUpdateTemplate,
            *(    const VkAllocationCallbacks*                *)&pAllocator);
}

inline VKAPI_ATTR void VKAPI_CALL vk_update_descriptor_set_with_template(
    vk_device_t                                        device,
    vk_descriptor_set_t                                descriptorSet,
    vk_descriptor_update_template_t                    descriptorUpdateTemplate,
    const void*                                        pData)
{
    return vkUpdateDescriptorSetWithTemplate (
            *(    VkDevice                                    *)&device,
            *(    VkDescriptorSet                             *)&descriptorSet,
            *(    VkDescriptorUpdateTemplate                  *)&descriptorUpdateTemplate,
            *(    const void*                                 *)&pData);
}

inline VKAPI_ATTR void VKAPI_CALL vk_get_physical_device_external_buffer_properties(
    vk_physical_device_t                               physicalDevice,
    const vk_physical_device_external_buffer_info_t*   pExternalBufferInfo,
    vk_external_buffer_properties_t*                   pExternalBufferProperties)
{
    return vkGetPhysicalDeviceExternalBufferProperties (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    const VkPhysicalDeviceExternalBufferInfo*   *)&pExternalBufferInfo,
            *(    VkExternalBufferProperties*                 *)&pExternalBufferProperties);
}

inline VKAPI_ATTR void VKAPI_CALL vk_get_physical_device_external_fence_properties(
    vk_physical_device_t                               physicalDevice,
    const vk_physical_device_external_fence_info_t*    pExternalFenceInfo,
    vk_external_fence_properties_t*                    pExternalFenceProperties)
{
    return vkGetPhysicalDeviceExternalFenceProperties (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    const VkPhysicalDeviceExternalFenceInfo*    *)&pExternalFenceInfo,
            *(    VkExternalFenceProperties*                  *)&pExternalFenceProperties);
}

inline VKAPI_ATTR void VKAPI_CALL vk_get_physical_device_external_semaphore_properties(
    vk_physical_device_t                               physicalDevice,
    const vk_physical_device_external_semaphore_info_t* pExternalSemaphoreInfo,
    vk_external_semaphore_properties_t*                pExternalSemaphoreProperties)
{
    return vkGetPhysicalDeviceExternalSemaphoreProperties (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    const VkPhysicalDeviceExternalSemaphoreInfo* *)&pExternalSemaphoreInfo,
            *(    VkExternalSemaphoreProperties*              *)&pExternalSemaphoreProperties);
}

inline VKAPI_ATTR void VKAPI_CALL vk_get_descriptor_set_layout_support(
    vk_device_t                                        device,
    const vk_descriptor_set_layout_create_info_t*      pCreateInfo,
    vk_descriptor_set_layout_support_t*                pSupport)
{
    return vkGetDescriptorSetLayoutSupport (
            *(    VkDevice                                    *)&device,
            *(    const VkDescriptorSetLayoutCreateInfo*      *)&pCreateInfo,
            *(    VkDescriptorSetLayoutSupport*               *)&pSupport);
}
#define vk_device_address_t                                               VkDeviceAddress 
#define vk_driver_id_t                                                    VkDriverId
#define vk_shader_float_controls_independence_t                           VkShaderFloatControlsIndependence
#define vk_sampler_reduction_mode_t                                       VkSamplerReductionMode
#define vk_semaphore_type_t                                               VkSemaphoreType
#define vk_resolve_mode_flag_bits_t                                       VkResolveModeFlagBits
#define vk_resolve_mode_flags_t                                           VkResolveModeFlags 
#define vk_descriptor_binding_flag_bits_t                                 VkDescriptorBindingFlagBits
#define vk_descriptor_binding_flags_t                                     VkDescriptorBindingFlags 
#define vk_semaphore_wait_flag_bits_t                                     VkSemaphoreWaitFlagBits
#define vk_semaphore_wait_flags_t                                         VkSemaphoreWaitFlags 

typedef struct vk_physical_device_vulkan11_features_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    void* p_next =                                     NULL;
    vk_bool32_t                                        storage_buffer16_bit_access;
    vk_bool32_t                                        uniform_and_storage_buffer16_bit_access;
    vk_bool32_t                                        storage_push_constant16;
    vk_bool32_t                                        storage_input_output16;
    vk_bool32_t                                        multiview;
    vk_bool32_t                                        multiview_geometry_shader;
    vk_bool32_t                                        multiview_tessellation_shader;
    vk_bool32_t                                        variable_pointers_storage_buffer;
    vk_bool32_t                                        variable_pointers;
    vk_bool32_t                                        protected_memory;
    vk_bool32_t                                        sampler_ycbcr_conversion;
    vk_bool32_t                                        shader_draw_parameters;
} vk_physical_device_vulkan11_features_t;

typedef struct vk_physical_device_vulkan11_properties_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES;
    void* p_next =                                     NULL;
    uint8_t                                            device_uuid[VK_UUID_SIZE];
    uint8_t                                            driver_uuid[VK_UUID_SIZE];
    uint8_t                                            device_luid[VK_LUID_SIZE];
    uint32_t                                           device_node_mask;
    vk_bool32_t                                        device_luid_valid;
    uint32_t                                           subgroup_size;
    vk_shader_stage_flags_t                            subgroup_supported_stages;
    vk_subgroup_feature_flags_t                        subgroup_supported_operations;
    vk_bool32_t                                        subgroup_quad_operations_in_all_stages;
    vk_point_clipping_behavior_t                       point_clipping_behavior;
    uint32_t                                           max_multiview_view_count;
    uint32_t                                           max_multiview_instance_index;
    vk_bool32_t                                        protected_no_fault;
    uint32_t                                           max_per_set_descriptors;
    vk_device_size_t                                   max_memory_allocation_size;
} vk_physical_device_vulkan11_properties_t;

typedef struct vk_physical_device_vulkan12_features_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    void* p_next =                                     NULL;
    vk_bool32_t                                        sampler_mirror_clamp_to_edge;
    vk_bool32_t                                        draw_indirect_count;
    vk_bool32_t                                        storage_buffer8_bit_access;
    vk_bool32_t                                        uniform_and_storage_buffer8_bit_access;
    vk_bool32_t                                        storage_push_constant8;
    vk_bool32_t                                        shader_buffer_int64_atomics;
    vk_bool32_t                                        shader_shared_int64_atomics;
    vk_bool32_t                                        shader_float16;
    vk_bool32_t                                        shader_int8;
    vk_bool32_t                                        descriptor_indexing;
    vk_bool32_t                                        shader_input_attachment_array_dynamic_indexing;
    vk_bool32_t                                        shader_uniform_texel_buffer_array_dynamic_indexing;
    vk_bool32_t                                        shader_storage_texel_buffer_array_dynamic_indexing;
    vk_bool32_t                                        shader_uniform_buffer_array_non_uniform_indexing;
    vk_bool32_t                                        shader_sampled_image_array_non_uniform_indexing;
    vk_bool32_t                                        shader_storage_buffer_array_non_uniform_indexing;
    vk_bool32_t                                        shader_storage_image_array_non_uniform_indexing;
    vk_bool32_t                                        shader_input_attachment_array_non_uniform_indexing;
    vk_bool32_t                                        shader_uniform_texel_buffer_array_non_uniform_indexing;
    vk_bool32_t                                        shader_storage_texel_buffer_array_non_uniform_indexing;
    vk_bool32_t                                        descriptor_binding_uniform_buffer_update_after_bind;
    vk_bool32_t                                        descriptor_binding_sampled_image_update_after_bind;
    vk_bool32_t                                        descriptor_binding_storage_image_update_after_bind;
    vk_bool32_t                                        descriptor_binding_storage_buffer_update_after_bind;
    vk_bool32_t                                        descriptor_binding_uniform_texel_buffer_update_after_bind;
    vk_bool32_t                                        descriptor_binding_storage_texel_buffer_update_after_bind;
    vk_bool32_t                                        descriptor_binding_update_unused_while_pending;
    vk_bool32_t                                        descriptor_binding_partially_bound;
    vk_bool32_t                                        descriptor_binding_variable_descriptor_count;
    vk_bool32_t                                        runtime_descriptor_array;
    vk_bool32_t                                        sampler_filter_minmax;
    vk_bool32_t                                        scalar_block_layout;
    vk_bool32_t                                        imageless_framebuffer;
    vk_bool32_t                                        uniform_buffer_standard_layout;
    vk_bool32_t                                        shader_subgroup_extended_types;
    vk_bool32_t                                        separate_depth_stencil_layouts;
    vk_bool32_t                                        host_query_reset;
    vk_bool32_t                                        timeline_semaphore;
    vk_bool32_t                                        buffer_device_address;
    vk_bool32_t                                        buffer_device_address_capture_replay;
    vk_bool32_t                                        buffer_device_address_multi_device;
    vk_bool32_t                                        vulkan_memory_model;
    vk_bool32_t                                        vulkan_memory_model_device_scope;
    vk_bool32_t                                        vulkan_memory_model_availability_visibility_chains;
    vk_bool32_t                                        shader_output_viewport_index;
    vk_bool32_t                                        shader_output_layer;
    vk_bool32_t                                        subgroup_broadcast_dynamic_id;
} vk_physical_device_vulkan12_features_t;

typedef struct vk_conformance_version_t {
    uint8_t                                            major;
    uint8_t                                            minor;
    uint8_t                                            subminor;
    uint8_t                                            patch;
} vk_conformance_version_t;

typedef struct vk_physical_device_vulkan12_properties_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES;
    void* p_next =                                     NULL;
    vk_driver_id_t                                     driver_id;
    char                                               driver_name[VK_MAX_DRIVER_NAME_SIZE];
    char                                               driver_info[VK_MAX_DRIVER_INFO_SIZE];
    vk_conformance_version_t                           conformance_version;
    vk_shader_float_controls_independence_t            denorm_behavior_independence;
    vk_shader_float_controls_independence_t            rounding_mode_independence;
    vk_bool32_t                                        shader_signed_zero_inf_nan_preserve_float16;
    vk_bool32_t                                        shader_signed_zero_inf_nan_preserve_float32;
    vk_bool32_t                                        shader_signed_zero_inf_nan_preserve_float64;
    vk_bool32_t                                        shader_denorm_preserve_float16;
    vk_bool32_t                                        shader_denorm_preserve_float32;
    vk_bool32_t                                        shader_denorm_preserve_float64;
    vk_bool32_t                                        shader_denorm_flush_to_zero_float16;
    vk_bool32_t                                        shader_denorm_flush_to_zero_float32;
    vk_bool32_t                                        shader_denorm_flush_to_zero_float64;
    vk_bool32_t                                        shader_rounding_mode_rte_float16;
    vk_bool32_t                                        shader_rounding_mode_rte_float32;
    vk_bool32_t                                        shader_rounding_mode_rte_float64;
    vk_bool32_t                                        shader_rounding_mode_rtz_float16;
    vk_bool32_t                                        shader_rounding_mode_rtz_float32;
    vk_bool32_t                                        shader_rounding_mode_rtz_float64;
    uint32_t                                           max_update_after_bind_descriptors_in_all_pools;
    vk_bool32_t                                        shader_uniform_buffer_array_non_uniform_indexing_native;
    vk_bool32_t                                        shader_sampled_image_array_non_uniform_indexing_native;
    vk_bool32_t                                        shader_storage_buffer_array_non_uniform_indexing_native;
    vk_bool32_t                                        shader_storage_image_array_non_uniform_indexing_native;
    vk_bool32_t                                        shader_input_attachment_array_non_uniform_indexing_native;
    vk_bool32_t                                        robust_buffer_access_update_after_bind;
    vk_bool32_t                                        quad_divergent_implicit_lod;
    uint32_t                                           max_per_stage_descriptor_update_after_bind_samplers;
    uint32_t                                           max_per_stage_descriptor_update_after_bind_uniform_buffers;
    uint32_t                                           max_per_stage_descriptor_update_after_bind_storage_buffers;
    uint32_t                                           max_per_stage_descriptor_update_after_bind_sampled_images;
    uint32_t                                           max_per_stage_descriptor_update_after_bind_storage_images;
    uint32_t                                           max_per_stage_descriptor_update_after_bind_input_attachments;
    uint32_t                                           max_per_stage_update_after_bind_resources;
    uint32_t                                           max_descriptor_set_update_after_bind_samplers;
    uint32_t                                           max_descriptor_set_update_after_bind_uniform_buffers;
    uint32_t                                           max_descriptor_set_update_after_bind_uniform_buffers_dynamic;
    uint32_t                                           max_descriptor_set_update_after_bind_storage_buffers;
    uint32_t                                           max_descriptor_set_update_after_bind_storage_buffers_dynamic;
    uint32_t                                           max_descriptor_set_update_after_bind_sampled_images;
    uint32_t                                           max_descriptor_set_update_after_bind_storage_images;
    uint32_t                                           max_descriptor_set_update_after_bind_input_attachments;
    vk_resolve_mode_flags_t                            supported_depth_resolve_modes;
    vk_resolve_mode_flags_t                            supported_stencil_resolve_modes;
    vk_bool32_t                                        independent_resolve_none;
    vk_bool32_t                                        independent_resolve;
    vk_bool32_t                                        filter_minmax_single_component_formats;
    vk_bool32_t                                        filter_minmax_image_component_mapping;
    uint64_t                                           max_timeline_semaphore_value_difference;
    vk_sample_count_flags_t                            framebuffer_integer_color_sample_counts;
} vk_physical_device_vulkan12_properties_t;

typedef struct vk_image_format_list_create_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO;
    const void* p_next =                               NULL;
    uint32_t                                           view_format_count;
    const vk_format_t*                                 p_view_formats;
} vk_image_format_list_create_info_t;

typedef struct vk_attachment_description2_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
    const void* p_next =                               NULL;
    vk_attachment_description_flags_t                  flags;
    vk_format_t                                        format;
    vk_sample_count_flag_bits_t                        samples;
    vk_attachment_load_op_t                            load_op;
    vk_attachment_store_op_t                           store_op;
    vk_attachment_load_op_t                            stencil_load_op;
    vk_attachment_store_op_t                           stencil_store_op;
    vk_image_layout_t                                  initial_layout;
    vk_image_layout_t                                  final_layout;
} vk_attachment_description2_t;

typedef struct vk_attachment_reference2_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
    const void* p_next =                               NULL;
    uint32_t                                           attachment;
    vk_image_layout_t                                  layout;
    vk_image_aspect_flags_t                            aspect_mask;
} vk_attachment_reference2_t;

typedef struct vk_subpass_description2_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2;
    const void* p_next =                               NULL;
    vk_subpass_description_flags_t                     flags;
    vk_pipeline_bind_point_t                           pipeline_bind_point;
    uint32_t                                           view_mask;
    uint32_t                                           input_attachment_count;
    const vk_attachment_reference2_t*                  p_input_attachments;
    uint32_t                                           color_attachment_count;
    const vk_attachment_reference2_t*                  p_color_attachments;
    const vk_attachment_reference2_t*                  p_resolve_attachments;
    const vk_attachment_reference2_t*                  p_depth_stencil_attachment;
    uint32_t                                           preserve_attachment_count;
    const uint32_t*                                    p_preserve_attachments;
} vk_subpass_description2_t;

typedef struct vk_subpass_dependency2_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
    const void* p_next =                               NULL;
    uint32_t                                           src_subpass;
    uint32_t                                           dst_subpass;
    vk_pipeline_stage_flags_t                          src_stage_mask;
    vk_pipeline_stage_flags_t                          dst_stage_mask;
    vk_access_flags_t                                  src_access_mask;
    vk_access_flags_t                                  dst_access_mask;
    vk_dependency_flags_t                              dependency_flags;
    int32_t                                            view_offset;
} vk_subpass_dependency2_t;

typedef struct vk_render_pass_create_info2_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2;
    const void* p_next =                               NULL;
    vk_render_pass_create_flags_t                      flags;
    uint32_t                                           attachment_count;
    const vk_attachment_description2_t*                p_attachments;
    uint32_t                                           subpass_count;
    const vk_subpass_description2_t*                   p_subpasses;
    uint32_t                                           dependency_count;
    const vk_subpass_dependency2_t*                    p_dependencies;
    uint32_t                                           correlated_view_mask_count;
    const uint32_t*                                    p_correlated_view_masks;
} vk_render_pass_create_info2_t;

typedef struct vk_subpass_begin_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_SUBPASS_BEGIN_INFO;
    const void* p_next =                               NULL;
    vk_subpass_contents_t                              contents;
} vk_subpass_begin_info_t;

typedef struct vk_subpass_end_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_SUBPASS_END_INFO;
    const void* p_next =                               NULL;
} vk_subpass_end_info_t;

typedef struct vk_physical_device8_bit_storage_features_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES;
    void* p_next =                                     NULL;
    vk_bool32_t                                        storage_buffer8_bit_access;
    vk_bool32_t                                        uniform_and_storage_buffer8_bit_access;
    vk_bool32_t                                        storage_push_constant8;
} vk_physical_device8_bit_storage_features_t;

typedef struct vk_physical_device_driver_properties_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES;
    void* p_next =                                     NULL;
    vk_driver_id_t                                     driver_id;
    char                                               driver_name[VK_MAX_DRIVER_NAME_SIZE];
    char                                               driver_info[VK_MAX_DRIVER_INFO_SIZE];
    vk_conformance_version_t                           conformance_version;
} vk_physical_device_driver_properties_t;

typedef struct vk_physical_device_shader_atomic_int64_features_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_INT64_FEATURES;
    void* p_next =                                     NULL;
    vk_bool32_t                                        shader_buffer_int64_atomics;
    vk_bool32_t                                        shader_shared_int64_atomics;
} vk_physical_device_shader_atomic_int64_features_t;

typedef struct vk_physical_device_shader_float16_int8_features_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES;
    void* p_next =                                     NULL;
    vk_bool32_t                                        shader_float16;
    vk_bool32_t                                        shader_int8;
} vk_physical_device_shader_float16_int8_features_t;

typedef struct vk_physical_device_float_controls_properties_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT_CONTROLS_PROPERTIES;
    void* p_next =                                     NULL;
    vk_shader_float_controls_independence_t            denorm_behavior_independence;
    vk_shader_float_controls_independence_t            rounding_mode_independence;
    vk_bool32_t                                        shader_signed_zero_inf_nan_preserve_float16;
    vk_bool32_t                                        shader_signed_zero_inf_nan_preserve_float32;
    vk_bool32_t                                        shader_signed_zero_inf_nan_preserve_float64;
    vk_bool32_t                                        shader_denorm_preserve_float16;
    vk_bool32_t                                        shader_denorm_preserve_float32;
    vk_bool32_t                                        shader_denorm_preserve_float64;
    vk_bool32_t                                        shader_denorm_flush_to_zero_float16;
    vk_bool32_t                                        shader_denorm_flush_to_zero_float32;
    vk_bool32_t                                        shader_denorm_flush_to_zero_float64;
    vk_bool32_t                                        shader_rounding_mode_rte_float16;
    vk_bool32_t                                        shader_rounding_mode_rte_float32;
    vk_bool32_t                                        shader_rounding_mode_rte_float64;
    vk_bool32_t                                        shader_rounding_mode_rtz_float16;
    vk_bool32_t                                        shader_rounding_mode_rtz_float32;
    vk_bool32_t                                        shader_rounding_mode_rtz_float64;
} vk_physical_device_float_controls_properties_t;

typedef struct vk_descriptor_set_layout_binding_flags_create_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    const void* p_next =                               NULL;
    uint32_t                                           binding_count;
    const vk_descriptor_binding_flags_t*               p_binding_flags;
} vk_descriptor_set_layout_binding_flags_create_info_t;

typedef struct vk_physical_device_descriptor_indexing_features_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
    void* p_next =                                     NULL;
    vk_bool32_t                                        shader_input_attachment_array_dynamic_indexing;
    vk_bool32_t                                        shader_uniform_texel_buffer_array_dynamic_indexing;
    vk_bool32_t                                        shader_storage_texel_buffer_array_dynamic_indexing;
    vk_bool32_t                                        shader_uniform_buffer_array_non_uniform_indexing;
    vk_bool32_t                                        shader_sampled_image_array_non_uniform_indexing;
    vk_bool32_t                                        shader_storage_buffer_array_non_uniform_indexing;
    vk_bool32_t                                        shader_storage_image_array_non_uniform_indexing;
    vk_bool32_t                                        shader_input_attachment_array_non_uniform_indexing;
    vk_bool32_t                                        shader_uniform_texel_buffer_array_non_uniform_indexing;
    vk_bool32_t                                        shader_storage_texel_buffer_array_non_uniform_indexing;
    vk_bool32_t                                        descriptor_binding_uniform_buffer_update_after_bind;
    vk_bool32_t                                        descriptor_binding_sampled_image_update_after_bind;
    vk_bool32_t                                        descriptor_binding_storage_image_update_after_bind;
    vk_bool32_t                                        descriptor_binding_storage_buffer_update_after_bind;
    vk_bool32_t                                        descriptor_binding_uniform_texel_buffer_update_after_bind;
    vk_bool32_t                                        descriptor_binding_storage_texel_buffer_update_after_bind;
    vk_bool32_t                                        descriptor_binding_update_unused_while_pending;
    vk_bool32_t                                        descriptor_binding_partially_bound;
    vk_bool32_t                                        descriptor_binding_variable_descriptor_count;
    vk_bool32_t                                        runtime_descriptor_array;
} vk_physical_device_descriptor_indexing_features_t;

typedef struct vk_physical_device_descriptor_indexing_properties_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES;
    void* p_next =                                     NULL;
    uint32_t                                           max_update_after_bind_descriptors_in_all_pools;
    vk_bool32_t                                        shader_uniform_buffer_array_non_uniform_indexing_native;
    vk_bool32_t                                        shader_sampled_image_array_non_uniform_indexing_native;
    vk_bool32_t                                        shader_storage_buffer_array_non_uniform_indexing_native;
    vk_bool32_t                                        shader_storage_image_array_non_uniform_indexing_native;
    vk_bool32_t                                        shader_input_attachment_array_non_uniform_indexing_native;
    vk_bool32_t                                        robust_buffer_access_update_after_bind;
    vk_bool32_t                                        quad_divergent_implicit_lod;
    uint32_t                                           max_per_stage_descriptor_update_after_bind_samplers;
    uint32_t                                           max_per_stage_descriptor_update_after_bind_uniform_buffers;
    uint32_t                                           max_per_stage_descriptor_update_after_bind_storage_buffers;
    uint32_t                                           max_per_stage_descriptor_update_after_bind_sampled_images;
    uint32_t                                           max_per_stage_descriptor_update_after_bind_storage_images;
    uint32_t                                           max_per_stage_descriptor_update_after_bind_input_attachments;
    uint32_t                                           max_per_stage_update_after_bind_resources;
    uint32_t                                           max_descriptor_set_update_after_bind_samplers;
    uint32_t                                           max_descriptor_set_update_after_bind_uniform_buffers;
    uint32_t                                           max_descriptor_set_update_after_bind_uniform_buffers_dynamic;
    uint32_t                                           max_descriptor_set_update_after_bind_storage_buffers;
    uint32_t                                           max_descriptor_set_update_after_bind_storage_buffers_dynamic;
    uint32_t                                           max_descriptor_set_update_after_bind_sampled_images;
    uint32_t                                           max_descriptor_set_update_after_bind_storage_images;
    uint32_t                                           max_descriptor_set_update_after_bind_input_attachments;
} vk_physical_device_descriptor_indexing_properties_t;

typedef struct vk_descriptor_set_variable_descriptor_count_allocate_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
    const void* p_next =                               NULL;
    uint32_t                                           descriptor_set_count;
    const uint32_t*                                    p_descriptor_counts;
} vk_descriptor_set_variable_descriptor_count_allocate_info_t;

typedef struct vk_descriptor_set_variable_descriptor_count_layout_support_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_LAYOUT_SUPPORT;
    void* p_next =                                     NULL;
    uint32_t                                           max_variable_descriptor_count;
} vk_descriptor_set_variable_descriptor_count_layout_support_t;

typedef struct vk_subpass_description_depth_stencil_resolve_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_DEPTH_STENCIL_RESOLVE;
    const void* p_next =                               NULL;
    vk_resolve_mode_flag_bits_t                        depth_resolve_mode;
    vk_resolve_mode_flag_bits_t                        stencil_resolve_mode;
    const vk_attachment_reference2_t*                  p_depth_stencil_resolve_attachment;
} vk_subpass_description_depth_stencil_resolve_t;

typedef struct vk_physical_device_depth_stencil_resolve_properties_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_STENCIL_RESOLVE_PROPERTIES;
    void* p_next =                                     NULL;
    vk_resolve_mode_flags_t                            supported_depth_resolve_modes;
    vk_resolve_mode_flags_t                            supported_stencil_resolve_modes;
    vk_bool32_t                                        independent_resolve_none;
    vk_bool32_t                                        independent_resolve;
} vk_physical_device_depth_stencil_resolve_properties_t;

typedef struct vk_physical_device_scalar_block_layout_features_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES;
    void* p_next =                                     NULL;
    vk_bool32_t                                        scalar_block_layout;
} vk_physical_device_scalar_block_layout_features_t;

typedef struct vk_image_stencil_usage_create_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_IMAGE_STENCIL_USAGE_CREATE_INFO;
    const void* p_next =                               NULL;
    vk_image_usage_flags_t                             stencil_usage;
} vk_image_stencil_usage_create_info_t;

typedef struct vk_sampler_reduction_mode_create_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO;
    const void* p_next =                               NULL;
    vk_sampler_reduction_mode_t                        reduction_mode;
} vk_sampler_reduction_mode_create_info_t;

typedef struct vk_physical_device_sampler_filter_minmax_properties_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_FILTER_MINMAX_PROPERTIES;
    void* p_next =                                     NULL;
    vk_bool32_t                                        filter_minmax_single_component_formats;
    vk_bool32_t                                        filter_minmax_image_component_mapping;
} vk_physical_device_sampler_filter_minmax_properties_t;

typedef struct vk_physical_device_vulkan_memory_model_features_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_MEMORY_MODEL_FEATURES;
    void* p_next =                                     NULL;
    vk_bool32_t                                        vulkan_memory_model;
    vk_bool32_t                                        vulkan_memory_model_device_scope;
    vk_bool32_t                                        vulkan_memory_model_availability_visibility_chains;
} vk_physical_device_vulkan_memory_model_features_t;

typedef struct vk_physical_device_imageless_framebuffer_features_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGELESS_FRAMEBUFFER_FEATURES;
    void* p_next =                                     NULL;
    vk_bool32_t                                        imageless_framebuffer;
} vk_physical_device_imageless_framebuffer_features_t;

typedef struct vk_framebuffer_attachment_image_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENT_IMAGE_INFO;
    const void* p_next =                               NULL;
    vk_image_create_flags_t                            flags;
    vk_image_usage_flags_t                             usage;
    uint32_t                                           width;
    uint32_t                                           height;
    uint32_t                                           layer_count;
    uint32_t                                           view_format_count;
    const vk_format_t*                                 p_view_formats;
} vk_framebuffer_attachment_image_info_t;

typedef struct vk_framebuffer_attachments_create_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENTS_CREATE_INFO;
    const void* p_next =                               NULL;
    uint32_t                                           attachment_image_info_count;
    const vk_framebuffer_attachment_image_info_t*      p_attachment_image_infos;
} vk_framebuffer_attachments_create_info_t;

typedef struct vk_render_pass_attachment_begin_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_RENDER_PASS_ATTACHMENT_BEGIN_INFO;
    const void* p_next =                               NULL;
    uint32_t                                           attachment_count;
    const vk_image_view_t*                             p_attachments;
} vk_render_pass_attachment_begin_info_t;

typedef struct vk_physical_device_uniform_buffer_standard_layout_features_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_UNIFORM_BUFFER_STANDARD_LAYOUT_FEATURES;
    void* p_next =                                     NULL;
    vk_bool32_t                                        uniform_buffer_standard_layout;
} vk_physical_device_uniform_buffer_standard_layout_features_t;

typedef struct vk_physical_device_shader_subgroup_extended_types_features_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SUBGROUP_EXTENDED_TYPES_FEATURES;
    void* p_next =                                     NULL;
    vk_bool32_t                                        shader_subgroup_extended_types;
} vk_physical_device_shader_subgroup_extended_types_features_t;

typedef struct vk_physical_device_separate_depth_stencil_layouts_features_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SEPARATE_DEPTH_STENCIL_LAYOUTS_FEATURES;
    void* p_next =                                     NULL;
    vk_bool32_t                                        separate_depth_stencil_layouts;
} vk_physical_device_separate_depth_stencil_layouts_features_t;

typedef struct vk_attachment_reference_stencil_layout_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_STENCIL_LAYOUT;
    void* p_next =                                     NULL;
    vk_image_layout_t                                  stencil_layout;
} vk_attachment_reference_stencil_layout_t;

typedef struct vk_attachment_description_stencil_layout_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_STENCIL_LAYOUT;
    void* p_next =                                     NULL;
    vk_image_layout_t                                  stencil_initial_layout;
    vk_image_layout_t                                  stencil_final_layout;
} vk_attachment_description_stencil_layout_t;

typedef struct vk_physical_device_host_query_reset_features_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES;
    void* p_next =                                     NULL;
    vk_bool32_t                                        host_query_reset;
} vk_physical_device_host_query_reset_features_t;

typedef struct vk_physical_device_timeline_semaphore_features_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
    void* p_next =                                     NULL;
    vk_bool32_t                                        timeline_semaphore;
} vk_physical_device_timeline_semaphore_features_t;

typedef struct vk_physical_device_timeline_semaphore_properties_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_PROPERTIES;
    void* p_next =                                     NULL;
    uint64_t                                           max_timeline_semaphore_value_difference;
} vk_physical_device_timeline_semaphore_properties_t;

typedef struct vk_semaphore_type_create_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
    const void* p_next =                               NULL;
    vk_semaphore_type_t                                semaphore_type;
    uint64_t                                           initial_value;
} vk_semaphore_type_create_info_t;

typedef struct vk_timeline_semaphore_submit_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
    const void* p_next =                               NULL;
    uint32_t                                           wait_semaphore_value_count;
    const uint64_t*                                    p_wait_semaphore_values;
    uint32_t                                           signal_semaphore_value_count;
    const uint64_t*                                    p_signal_semaphore_values;
} vk_timeline_semaphore_submit_info_t;

typedef struct vk_semaphore_wait_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
    const void* p_next =                               NULL;
    vk_semaphore_wait_flags_t                          flags;
    uint32_t                                           semaphore_count;
    const vk_semaphore_t*                              p_semaphores;
    const uint64_t*                                    p_values;
} vk_semaphore_wait_info_t;

typedef struct vk_semaphore_signal_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
    const void* p_next =                               NULL;
    vk_semaphore_t                                     semaphore;
    uint64_t                                           value;
} vk_semaphore_signal_info_t;

typedef struct vk_physical_device_buffer_device_address_features_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    void* p_next =                                     NULL;
    vk_bool32_t                                        buffer_device_address;
    vk_bool32_t                                        buffer_device_address_capture_replay;
    vk_bool32_t                                        buffer_device_address_multi_device;
} vk_physical_device_buffer_device_address_features_t;

typedef struct vk_buffer_device_address_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    const void* p_next =                               NULL;
    vk_buffer_t                                        buffer;
} vk_buffer_device_address_info_t;

typedef struct vk_buffer_opaque_capture_address_create_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_BUFFER_OPAQUE_CAPTURE_ADDRESS_CREATE_INFO;
    const void* p_next =                               NULL;
    uint64_t                                           opaque_capture_address;
} vk_buffer_opaque_capture_address_create_info_t;

typedef struct vk_memory_opaque_capture_address_allocate_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_MEMORY_OPAQUE_CAPTURE_ADDRESS_ALLOCATE_INFO;
    const void* p_next =                               NULL;
    uint64_t                                           opaque_capture_address;
} vk_memory_opaque_capture_address_allocate_info_t;

typedef struct vk_device_memory_opaque_capture_address_info_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_DEVICE_MEMORY_OPAQUE_CAPTURE_ADDRESS_INFO;
    const void* p_next =                               NULL;
    vk_device_memory_t                                 memory;
} vk_device_memory_opaque_capture_address_info_t;
#define pfn_vk_cmd_draw_indirect_count_fn_t                               PFN_vkCmdDrawIndirectCount
#define pfn_vk_cmd_draw_indexed_indirect_count_fn_t                       PFN_vkCmdDrawIndexedIndirectCount
#define pfn_vk_create_render_pass2_fn_t                                   PFN_vkCreateRenderPass2
#define pfn_vk_cmd_begin_render_pass2_fn_t                                PFN_vkCmdBeginRenderPass2
#define pfn_vk_cmd_next_subpass2_fn_t                                     PFN_vkCmdNextSubpass2
#define pfn_vk_cmd_end_render_pass2_fn_t                                  PFN_vkCmdEndRenderPass2
#define pfn_vk_reset_query_pool_fn_t                                      PFN_vkResetQueryPool
#define pfn_vk_get_semaphore_counter_value_fn_t                           PFN_vkGetSemaphoreCounterValue
#define pfn_vk_wait_semaphores_fn_t                                       PFN_vkWaitSemaphores
#define pfn_vk_signal_semaphore_fn_t                                      PFN_vkSignalSemaphore
#define pfn_vk_get_buffer_device_address_fn_t                             PFN_vkGetBufferDeviceAddress
#define pfn_vk_get_buffer_opaque_capture_address_fn_t                     PFN_vkGetBufferOpaqueCaptureAddress
#define pfn_vk_get_device_memory_opaque_capture_address_fn_t              PFN_vkGetDeviceMemoryOpaqueCaptureAddress

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_draw_indirect_count(
    vk_command_buffer_t                                commandBuffer,
    vk_buffer_t                                        buffer,
    vk_device_size_t                                   offset,
    vk_buffer_t                                        countBuffer,
    vk_device_size_t                                   countBufferOffset,
    uint32_t                                           maxDrawCount,
    uint32_t                                           stride)
{
    return vkCmdDrawIndirectCount (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    VkBuffer                                    *)&buffer,
            *(    VkDeviceSize                                *)&offset,
            *(    VkBuffer                                    *)&countBuffer,
            *(    VkDeviceSize                                *)&countBufferOffset,
            *(    uint32_t                                    *)&maxDrawCount,
            *(    uint32_t                                    *)&stride);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_draw_indexed_indirect_count(
    vk_command_buffer_t                                commandBuffer,
    vk_buffer_t                                        buffer,
    vk_device_size_t                                   offset,
    vk_buffer_t                                        countBuffer,
    vk_device_size_t                                   countBufferOffset,
    uint32_t                                           maxDrawCount,
    uint32_t                                           stride)
{
    return vkCmdDrawIndexedIndirectCount (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    VkBuffer                                    *)&buffer,
            *(    VkDeviceSize                                *)&offset,
            *(    VkBuffer                                    *)&countBuffer,
            *(    VkDeviceSize                                *)&countBufferOffset,
            *(    uint32_t                                    *)&maxDrawCount,
            *(    uint32_t                                    *)&stride);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_create_render_pass2(
    vk_device_t                                        device,
    const vk_render_pass_create_info2_t*               pCreateInfo,
    const vk_allocation_callbacks_t*                   pAllocator,
    vk_render_pass_t*                                  pRenderPass)
{
    return vkCreateRenderPass2 (
            *(    VkDevice                                    *)&device,
            *(    const VkRenderPassCreateInfo2*              *)&pCreateInfo,
            *(    const VkAllocationCallbacks*                *)&pAllocator,
            *(    VkRenderPass*                               *)&pRenderPass);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_begin_render_pass2(
    vk_command_buffer_t                                commandBuffer,
    const vk_render_pass_begin_info_t*                 pRenderPassBegin,
    const vk_subpass_begin_info_t*                     pSubpassBeginInfo)
{
    return vkCmdBeginRenderPass2 (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    const VkRenderPassBeginInfo*                *)&pRenderPassBegin,
            *(    const VkSubpassBeginInfo*                   *)&pSubpassBeginInfo);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_next_subpass2(
    vk_command_buffer_t                                commandBuffer,
    const vk_subpass_begin_info_t*                     pSubpassBeginInfo,
    const vk_subpass_end_info_t*                       pSubpassEndInfo)
{
    return vkCmdNextSubpass2 (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    const VkSubpassBeginInfo*                   *)&pSubpassBeginInfo,
            *(    const VkSubpassEndInfo*                     *)&pSubpassEndInfo);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_end_render_pass2(
    vk_command_buffer_t                                commandBuffer,
    const vk_subpass_end_info_t*                       pSubpassEndInfo)
{
    return vkCmdEndRenderPass2 (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    const VkSubpassEndInfo*                     *)&pSubpassEndInfo);
}

inline VKAPI_ATTR void VKAPI_CALL vk_reset_query_pool(
    vk_device_t                                        device,
    vk_query_pool_t                                    queryPool,
    uint32_t                                           firstQuery,
    uint32_t                                           queryCount)
{
    return vkResetQueryPool (
            *(    VkDevice                                    *)&device,
            *(    VkQueryPool                                 *)&queryPool,
            *(    uint32_t                                    *)&firstQuery,
            *(    uint32_t                                    *)&queryCount);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_semaphore_counter_value(
    vk_device_t                                        device,
    vk_semaphore_t                                     semaphore,
    uint64_t*                                          pValue)
{
    return vkGetSemaphoreCounterValue (
            *(    VkDevice                                    *)&device,
            *(    VkSemaphore                                 *)&semaphore,
            *(    uint64_t*                                   *)&pValue);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_wait_semaphores(
    vk_device_t                                        device,
    const vk_semaphore_wait_info_t*                    pWaitInfo,
    uint64_t                                           timeout)
{
    return vkWaitSemaphores (
            *(    VkDevice                                    *)&device,
            *(    const VkSemaphoreWaitInfo*                  *)&pWaitInfo,
            *(    uint64_t                                    *)&timeout);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_signal_semaphore(
    vk_device_t                                        device,
    const vk_semaphore_signal_info_t*                  pSignalInfo)
{
    return vkSignalSemaphore (
            *(    VkDevice                                    *)&device,
            *(    const VkSemaphoreSignalInfo*                *)&pSignalInfo);
}

inline VKAPI_ATTR vk_device_address_t VKAPI_CALL vk_get_buffer_device_address(
    vk_device_t                                        device,
    const vk_buffer_device_address_info_t*             pInfo)
{
    return vkGetBufferDeviceAddress (
            *(    VkDevice                                    *)&device,
            *(    const VkBufferDeviceAddressInfo*            *)&pInfo);
}

inline VKAPI_ATTR uint64_t VKAPI_CALL vk_get_buffer_opaque_capture_address(
    vk_device_t                                        device,
    const vk_buffer_device_address_info_t*             pInfo)
{
    return vkGetBufferOpaqueCaptureAddress (
            *(    VkDevice                                    *)&device,
            *(    const VkBufferDeviceAddressInfo*            *)&pInfo);
}

inline VKAPI_ATTR uint64_t VKAPI_CALL vk_get_device_memory_opaque_capture_address(
    vk_device_t                                        device,
    const vk_device_memory_opaque_capture_address_info_t* pInfo)
{
    return vkGetDeviceMemoryOpaqueCaptureAddress (
            *(    VkDevice                                    *)&device,
            *(    const VkDeviceMemoryOpaqueCaptureAddressInfo* *)&pInfo);
}
#define vk_surface_khr_t                                                  VkSurfaceKHR 
#define vk_color_space_khr_t                                              VkColorSpaceKHR
#define vk_present_mode_khr_t                                             VkPresentModeKHR
#define vk_surface_transform_flag_bits_khr_t                              VkSurfaceTransformFlagBitsKHR
#define vk_surface_transform_flags_khr_t                                  VkSurfaceTransformFlagsKHR 
#define vk_composite_alpha_flag_bits_khr_t                                VkCompositeAlphaFlagBitsKHR
#define vk_composite_alpha_flags_khr_t                                    VkCompositeAlphaFlagsKHR 

typedef struct vk_surface_capabilities_khr_t {
    uint32_t                                           min_image_count;
    uint32_t                                           max_image_count;
    vk_extent2d_t                                     current_extent;
    vk_extent2d_t                                     min_image_extent;
    vk_extent2d_t                                     max_image_extent;
    uint32_t                                           max_image_array_layers;
    vk_surface_transform_flags_khr_t                   supported_transforms;
    vk_surface_transform_flag_bits_khr_t               current_transform;
    vk_composite_alpha_flags_khr_t                     supported_composite_alpha;
    vk_image_usage_flags_t                             supported_usage_flags;
} vk_surface_capabilities_khr_t;

typedef struct vk_surface_format_khr_t {
    vk_format_t                                        format;
    vk_color_space_khr_t                               color_space;
} vk_surface_format_khr_t;
#define pfn_vk_destroy_surface_khr_fn_t                                   PFN_vkDestroySurfaceKHR
#define pfn_vk_get_physical_device_surface_support_khr_fn_t               PFN_vkGetPhysicalDeviceSurfaceSupportKHR
#define pfn_vk_get_physical_device_surface_capabilities_khr_fn_t          PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR
#define pfn_vk_get_physical_device_surface_formats_khr_fn_t               PFN_vkGetPhysicalDeviceSurfaceFormatsKHR
#define pfn_vk_get_physical_device_surface_present_modes_khr_fn_t         PFN_vkGetPhysicalDeviceSurfacePresentModesKHR

inline VKAPI_ATTR void VKAPI_CALL vk_destroy_surface_khr(
    vk_instance_t                                      instance,
    vk_surface_khr_t                                   surface,
    const vk_allocation_callbacks_t*                   pAllocator)
{
    return vkDestroySurfaceKHR (
            *(    VkInstance                                  *)&instance,
            *(    VkSurfaceKHR                                *)&surface,
            *(    const VkAllocationCallbacks*                *)&pAllocator);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_physical_device_surface_support_khr(
    vk_physical_device_t                               physicalDevice,
    uint32_t                                           queueFamilyIndex,
    vk_surface_khr_t                                   surface,
    vk_bool32_t*                                       pSupported)
{
    return vkGetPhysicalDeviceSurfaceSupportKHR (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    uint32_t                                    *)&queueFamilyIndex,
            *(    VkSurfaceKHR                                *)&surface,
            *(    VkBool32*                                   *)&pSupported);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_physical_device_surface_capabilities_khr(
    vk_physical_device_t                               physicalDevice,
    vk_surface_khr_t                                   surface,
    vk_surface_capabilities_khr_t*                     pSurfaceCapabilities)
{
    return vkGetPhysicalDeviceSurfaceCapabilitiesKHR (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    VkSurfaceKHR                                *)&surface,
            *(    VkSurfaceCapabilitiesKHR*                   *)&pSurfaceCapabilities);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_physical_device_surface_formats_khr(
    vk_physical_device_t                               physicalDevice,
    vk_surface_khr_t                                   surface,
    uint32_t*                                          pSurfaceFormatCount,
    vk_surface_format_khr_t*                           pSurfaceFormats)
{
    return vkGetPhysicalDeviceSurfaceFormatsKHR (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    VkSurfaceKHR                                *)&surface,
            *(    uint32_t*                                   *)&pSurfaceFormatCount,
            *(    VkSurfaceFormatKHR*                         *)&pSurfaceFormats);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_physical_device_surface_present_modes_khr(
    vk_physical_device_t                               physicalDevice,
    vk_surface_khr_t                                   surface,
    uint32_t*                                          pPresentModeCount,
    vk_present_mode_khr_t*                             pPresentModes)
{
    return vkGetPhysicalDeviceSurfacePresentModesKHR (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    VkSurfaceKHR                                *)&surface,
            *(    uint32_t*                                   *)&pPresentModeCount,
            *(    VkPresentModeKHR*                           *)&pPresentModes);
}
#define vk_swapchain_khr_t                                                VkSwapchainKHR 
#define vk_swapchain_create_flag_bits_khr_t                               VkSwapchainCreateFlagBitsKHR
#define vk_swapchain_create_flags_khr_t                                   VkSwapchainCreateFlagsKHR 
#define vk_device_group_present_mode_flag_bits_khr_t                      VkDeviceGroupPresentModeFlagBitsKHR
#define vk_device_group_present_mode_flags_khr_t                          VkDeviceGroupPresentModeFlagsKHR 

typedef struct vk_swapchain_create_info_khr_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    const void* p_next =                               NULL;
    vk_swapchain_create_flags_khr_t                    flags;
    vk_surface_khr_t                                   surface;
    uint32_t                                           min_image_count;
    vk_format_t                                        image_format;
    vk_color_space_khr_t                               image_color_space;
    vk_extent2d_t                                     image_extent;
    uint32_t                                           image_array_layers;
    vk_image_usage_flags_t                             image_usage;
    vk_sharing_mode_t                                  image_sharing_mode;
    uint32_t                                           queue_family_index_count;
    const uint32_t*                                    p_queue_family_indices;
    vk_surface_transform_flag_bits_khr_t               pre_transform;
    vk_composite_alpha_flag_bits_khr_t                 composite_alpha;
    vk_present_mode_khr_t                              present_mode;
    vk_bool32_t                                        clipped;
    vk_swapchain_khr_t                                 old_swapchain;
} vk_swapchain_create_info_khr_t;

typedef struct vk_present_info_khr_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    const void* p_next =                               NULL;
    uint32_t                                           wait_semaphore_count;
    const vk_semaphore_t*                              p_wait_semaphores;
    uint32_t                                           swapchain_count;
    const vk_swapchain_khr_t*                          p_swapchains;
    const uint32_t*                                    p_image_indices;
    vk_result_t*                                       p_results;
} vk_present_info_khr_t;

typedef struct vk_image_swapchain_create_info_khr_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_IMAGE_SWAPCHAIN_CREATE_INFO_KHR;
    const void* p_next =                               NULL;
    vk_swapchain_khr_t                                 swapchain;
} vk_image_swapchain_create_info_khr_t;

typedef struct vk_bind_image_memory_swapchain_info_khr_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_SWAPCHAIN_INFO_KHR;
    const void* p_next =                               NULL;
    vk_swapchain_khr_t                                 swapchain;
    uint32_t                                           image_index;
} vk_bind_image_memory_swapchain_info_khr_t;

typedef struct vk_acquire_next_image_info_khr_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR;
    const void* p_next =                               NULL;
    vk_swapchain_khr_t                                 swapchain;
    uint64_t                                           timeout;
    vk_semaphore_t                                     semaphore;
    vk_fence_t                                         fence;
    uint32_t                                           device_mask;
} vk_acquire_next_image_info_khr_t;

typedef struct vk_device_group_present_capabilities_khr_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_DEVICE_GROUP_PRESENT_CAPABILITIES_KHR;
    const void* p_next =                               NULL;
    uint32_t                                           present_mask[VK_MAX_DEVICE_GROUP_SIZE];
    vk_device_group_present_mode_flags_khr_t           modes;
} vk_device_group_present_capabilities_khr_t;

typedef struct vk_device_group_present_info_khr_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_DEVICE_GROUP_PRESENT_INFO_KHR;
    const void* p_next =                               NULL;
    uint32_t                                           swapchain_count;
    const uint32_t*                                    p_device_masks;
    vk_device_group_present_mode_flag_bits_khr_t       mode;
} vk_device_group_present_info_khr_t;

typedef struct vk_device_group_swapchain_create_info_khr_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_DEVICE_GROUP_SWAPCHAIN_CREATE_INFO_KHR;
    const void* p_next =                               NULL;
    vk_device_group_present_mode_flags_khr_t           modes;
} vk_device_group_swapchain_create_info_khr_t;
#define pfn_vk_create_swapchain_khr_fn_t                                  PFN_vkCreateSwapchainKHR
#define pfn_vk_destroy_swapchain_khr_fn_t                                 PFN_vkDestroySwapchainKHR
#define pfn_vk_get_swapchain_images_khr_fn_t                              PFN_vkGetSwapchainImagesKHR
#define pfn_vk_acquire_next_image_khr_fn_t                                PFN_vkAcquireNextImageKHR
#define pfn_vk_queue_present_khr_fn_t                                     PFN_vkQueuePresentKHR
#define pfn_vk_get_device_group_present_capabilities_khr_fn_t             PFN_vkGetDeviceGroupPresentCapabilitiesKHR
#define pfn_vk_get_device_group_surface_present_modes_khr_fn_t            PFN_vkGetDeviceGroupSurfacePresentModesKHR
#define pfn_vk_get_physical_device_present_rectangles_khr_fn_t            PFN_vkGetPhysicalDevicePresentRectanglesKHR
#define pfn_vk_acquire_next_image2_khr_fn_t                               PFN_vkAcquireNextImage2KHR

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_create_swapchain_khr(
    vk_device_t                                        device,
    const vk_swapchain_create_info_khr_t*              pCreateInfo,
    const vk_allocation_callbacks_t*                   pAllocator,
    vk_swapchain_khr_t*                                pSwapchain)
{
    return vkCreateSwapchainKHR (
            *(    VkDevice                                    *)&device,
            *(    const VkSwapchainCreateInfoKHR*             *)&pCreateInfo,
            *(    const VkAllocationCallbacks*                *)&pAllocator,
            *(    VkSwapchainKHR*                             *)&pSwapchain);
}

inline VKAPI_ATTR void VKAPI_CALL vk_destroy_swapchain_khr(
    vk_device_t                                        device,
    vk_swapchain_khr_t                                 swapchain,
    const vk_allocation_callbacks_t*                   pAllocator)
{
    return vkDestroySwapchainKHR (
            *(    VkDevice                                    *)&device,
            *(    VkSwapchainKHR                              *)&swapchain,
            *(    const VkAllocationCallbacks*                *)&pAllocator);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_swapchain_images_khr(
    vk_device_t                                        device,
    vk_swapchain_khr_t                                 swapchain,
    uint32_t*                                          pSwapchainImageCount,
    vk_image_t*                                        pSwapchainImages)
{
    return vkGetSwapchainImagesKHR (
            *(    VkDevice                                    *)&device,
            *(    VkSwapchainKHR                              *)&swapchain,
            *(    uint32_t*                                   *)&pSwapchainImageCount,
            *(    VkImage*                                    *)&pSwapchainImages);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_acquire_next_image_khr(
    vk_device_t                                        device,
    vk_swapchain_khr_t                                 swapchain,
    uint64_t                                           timeout,
    vk_semaphore_t                                     semaphore,
    vk_fence_t                                         fence,
    uint32_t*                                          pImageIndex)
{
    return vkAcquireNextImageKHR (
            *(    VkDevice                                    *)&device,
            *(    VkSwapchainKHR                              *)&swapchain,
            *(    uint64_t                                    *)&timeout,
            *(    VkSemaphore                                 *)&semaphore,
            *(    VkFence                                     *)&fence,
            *(    uint32_t*                                   *)&pImageIndex);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_queue_present_khr(
    vk_queue_t                                         queue,
    const vk_present_info_khr_t*                       pPresentInfo)
{
    return vkQueuePresentKHR (
            *(    VkQueue                                     *)&queue,
            *(    const VkPresentInfoKHR*                     *)&pPresentInfo);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_device_group_present_capabilities_khr(
    vk_device_t                                        device,
    vk_device_group_present_capabilities_khr_t*        pDeviceGroupPresentCapabilities)
{
    return vkGetDeviceGroupPresentCapabilitiesKHR (
            *(    VkDevice                                    *)&device,
            *(    VkDeviceGroupPresentCapabilitiesKHR*        *)&pDeviceGroupPresentCapabilities);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_device_group_surface_present_modes_khr(
    vk_device_t                                        device,
    vk_surface_khr_t                                   surface,
    vk_device_group_present_mode_flags_khr_t*          pModes)
{
    return vkGetDeviceGroupSurfacePresentModesKHR (
            *(    VkDevice                                    *)&device,
            *(    VkSurfaceKHR                                *)&surface,
            *(    VkDeviceGroupPresentModeFlagsKHR*           *)&pModes);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_physical_device_present_rectangles_khr(
    vk_physical_device_t                               physicalDevice,
    vk_surface_khr_t                                   surface,
    uint32_t*                                          pRectCount,
    vk_rect2d_t*                                      pRects)
{
    return vkGetPhysicalDevicePresentRectanglesKHR (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    VkSurfaceKHR                                *)&surface,
            *(    uint32_t*                                   *)&pRectCount,
            *(    VkRect2D*                                   *)&pRects);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_acquire_next_image2_khr(
    vk_device_t                                        device,
    const vk_acquire_next_image_info_khr_t*            pAcquireInfo,
    uint32_t*                                          pImageIndex)
{
    return vkAcquireNextImage2KHR (
            *(    VkDevice                                    *)&device,
            *(    const VkAcquireNextImageInfoKHR*            *)&pAcquireInfo,
            *(    uint32_t*                                   *)&pImageIndex);
}
#define vk_display_khr_t                                                  VkDisplayKHR 
#define vk_display_mode_khr_t                                             VkDisplayModeKHR 
#define vk_display_plane_alpha_flag_bits_khr_t                            VkDisplayPlaneAlphaFlagBitsKHR
#define vk_display_plane_alpha_flags_khr_t                                VkDisplayPlaneAlphaFlagsKHR 
#define vk_display_mode_create_flags_khr_t                                VkDisplayModeCreateFlagsKHR 
#define vk_display_surface_create_flags_khr_t                             VkDisplaySurfaceCreateFlagsKHR 

typedef struct vk_display_properties_khr_t {
    vk_display_khr_t                                   display;
    const char*                                        display_name;
    vk_extent2d_t                                     physical_dimensions;
    vk_extent2d_t                                     physical_resolution;
    vk_surface_transform_flags_khr_t                   supported_transforms;
    vk_bool32_t                                        plane_reorder_possible;
    vk_bool32_t                                        persistent_content;
} vk_display_properties_khr_t;

typedef struct vk_display_mode_parameters_khr_t {
    vk_extent2d_t                                     visible_region;
    uint32_t                                           refresh_rate;
} vk_display_mode_parameters_khr_t;

typedef struct vk_display_mode_properties_khr_t {
    vk_display_mode_khr_t                              display_mode;
    vk_display_mode_parameters_khr_t                   parameters;
} vk_display_mode_properties_khr_t;

typedef struct vk_display_mode_create_info_khr_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_DISPLAY_MODE_CREATE_INFO_KHR;
    const void* p_next =                               NULL;
    vk_display_mode_create_flags_khr_t                 flags;
    vk_display_mode_parameters_khr_t                   parameters;
} vk_display_mode_create_info_khr_t;

typedef struct vk_display_plane_capabilities_khr_t {
    vk_display_plane_alpha_flags_khr_t                 supported_alpha;
    vk_offset2_d_t                                     min_src_position;
    vk_offset2_d_t                                     max_src_position;
    vk_extent2d_t                                     min_src_extent;
    vk_extent2d_t                                     max_src_extent;
    vk_offset2_d_t                                     min_dst_position;
    vk_offset2_d_t                                     max_dst_position;
    vk_extent2d_t                                     min_dst_extent;
    vk_extent2d_t                                     max_dst_extent;
} vk_display_plane_capabilities_khr_t;

typedef struct vk_display_plane_properties_khr_t {
    vk_display_khr_t                                   current_display;
    uint32_t                                           current_stack_index;
} vk_display_plane_properties_khr_t;

typedef struct vk_display_surface_create_info_khr_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR;
    const void* p_next =                               NULL;
    vk_display_surface_create_flags_khr_t              flags;
    vk_display_mode_khr_t                              display_mode;
    uint32_t                                           plane_index;
    uint32_t                                           plane_stack_index;
    vk_surface_transform_flag_bits_khr_t               transform;
    float                                              global_alpha;
    vk_display_plane_alpha_flag_bits_khr_t             alpha_mode;
    vk_extent2d_t                                     image_extent;
} vk_display_surface_create_info_khr_t;
#define pfn_vk_get_physical_device_display_properties_khr_fn_t            PFN_vkGetPhysicalDeviceDisplayPropertiesKHR
#define pfn_vk_get_physical_device_display_plane_properties_khr_fn_t      PFN_vkGetPhysicalDeviceDisplayPlanePropertiesKHR
#define pfn_vk_get_display_plane_supported_displays_khr_fn_t              PFN_vkGetDisplayPlaneSupportedDisplaysKHR
#define pfn_vk_get_display_mode_properties_khr_fn_t                       PFN_vkGetDisplayModePropertiesKHR
#define pfn_vk_create_display_mode_khr_fn_t                               PFN_vkCreateDisplayModeKHR
#define pfn_vk_get_display_plane_capabilities_khr_fn_t                    PFN_vkGetDisplayPlaneCapabilitiesKHR
#define pfn_vk_create_display_plane_surface_khr_fn_t                      PFN_vkCreateDisplayPlaneSurfaceKHR

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_physical_device_display_properties_khr(
    vk_physical_device_t                               physicalDevice,
    uint32_t*                                          pPropertyCount,
    vk_display_properties_khr_t*                       pProperties)
{
    return vkGetPhysicalDeviceDisplayPropertiesKHR (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    uint32_t*                                   *)&pPropertyCount,
            *(    VkDisplayPropertiesKHR*                     *)&pProperties);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_physical_device_display_plane_properties_khr(
    vk_physical_device_t                               physicalDevice,
    uint32_t*                                          pPropertyCount,
    vk_display_plane_properties_khr_t*                 pProperties)
{
    return vkGetPhysicalDeviceDisplayPlanePropertiesKHR (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    uint32_t*                                   *)&pPropertyCount,
            *(    VkDisplayPlanePropertiesKHR*                *)&pProperties);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_display_plane_supported_displays_khr(
    vk_physical_device_t                               physicalDevice,
    uint32_t                                           planeIndex,
    uint32_t*                                          pDisplayCount,
    vk_display_khr_t*                                  pDisplays)
{
    return vkGetDisplayPlaneSupportedDisplaysKHR (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    uint32_t                                    *)&planeIndex,
            *(    uint32_t*                                   *)&pDisplayCount,
            *(    VkDisplayKHR*                               *)&pDisplays);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_display_mode_properties_khr(
    vk_physical_device_t                               physicalDevice,
    vk_display_khr_t                                   display,
    uint32_t*                                          pPropertyCount,
    vk_display_mode_properties_khr_t*                  pProperties)
{
    return vkGetDisplayModePropertiesKHR (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    VkDisplayKHR                                *)&display,
            *(    uint32_t*                                   *)&pPropertyCount,
            *(    VkDisplayModePropertiesKHR*                 *)&pProperties);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_create_display_mode_khr(
    vk_physical_device_t                               physicalDevice,
    vk_display_khr_t                                   display,
    const vk_display_mode_create_info_khr_t*           pCreateInfo,
    const vk_allocation_callbacks_t*                   pAllocator,
    vk_display_mode_khr_t*                             pMode)
{
    return vkCreateDisplayModeKHR (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    VkDisplayKHR                                *)&display,
            *(    const VkDisplayModeCreateInfoKHR*           *)&pCreateInfo,
            *(    const VkAllocationCallbacks*                *)&pAllocator,
            *(    VkDisplayModeKHR*                           *)&pMode);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_display_plane_capabilities_khr(
    vk_physical_device_t                               physicalDevice,
    vk_display_mode_khr_t                              mode,
    uint32_t                                           planeIndex,
    vk_display_plane_capabilities_khr_t*               pCapabilities)
{
    return vkGetDisplayPlaneCapabilitiesKHR (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    VkDisplayModeKHR                            *)&mode,
            *(    uint32_t                                    *)&planeIndex,
            *(    VkDisplayPlaneCapabilitiesKHR*              *)&pCapabilities);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_create_display_plane_surface_khr(
    vk_instance_t                                      instance,
    const vk_display_surface_create_info_khr_t*        pCreateInfo,
    const vk_allocation_callbacks_t*                   pAllocator,
    vk_surface_khr_t*                                  pSurface)
{
    return vkCreateDisplayPlaneSurfaceKHR (
            *(    VkInstance                                  *)&instance,
            *(    const VkDisplaySurfaceCreateInfoKHR*        *)&pCreateInfo,
            *(    const VkAllocationCallbacks*                *)&pAllocator,
            *(    VkSurfaceKHR*                               *)&pSurface);
}

typedef struct vk_display_present_info_khr_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_DISPLAY_PRESENT_INFO_KHR;
    const void* p_next =                               NULL;
    vk_rect2d_t                                       src_rect;
    vk_rect2d_t                                       dst_rect;
    vk_bool32_t                                        persistent;
} vk_display_present_info_khr_t;
#define pfn_vk_create_shared_swapchains_khr_fn_t                          PFN_vkCreateSharedSwapchainsKHR

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_create_shared_swapchains_khr(
    vk_device_t                                        device,
    uint32_t                                           swapchainCount,
    const vk_swapchain_create_info_khr_t*              pCreateInfos,
    const vk_allocation_callbacks_t*                   pAllocator,
    vk_swapchain_khr_t*                                pSwapchains)
{
    return vkCreateSharedSwapchainsKHR (
            *(    VkDevice                                    *)&device,
            *(    uint32_t                                    *)&swapchainCount,
            *(    const VkSwapchainCreateInfoKHR*             *)&pCreateInfos,
            *(    const VkAllocationCallbacks*                *)&pAllocator,
            *(    VkSwapchainKHR*                             *)&pSwapchains);
}
#define vk_render_pass_multiview_create_info_khr_t                        VkRenderPassMultiviewCreateInfoKHR 
#define vk_physical_device_multiview_features_khr_t                       VkPhysicalDeviceMultiviewFeaturesKHR 
#define vk_physical_device_multiview_properties_khr_t                     VkPhysicalDeviceMultiviewPropertiesKHR 
#define vk_physical_device_features2_khr_t                                VkPhysicalDeviceFeatures2KHR 
#define vk_physical_device_properties2_khr_t                              VkPhysicalDeviceProperties2KHR 
#define vk_format_properties2_khr_t                                       VkFormatProperties2KHR 
#define vk_image_format_properties2_khr_t                                 VkImageFormatProperties2KHR 
#define vk_physical_device_image_format_info2_khr_t                       VkPhysicalDeviceImageFormatInfo2KHR 
#define vk_queue_family_properties2_khr_t                                 VkQueueFamilyProperties2KHR 
#define vk_physical_device_memory_properties2_khr_t                       VkPhysicalDeviceMemoryProperties2KHR 
#define vk_sparse_image_format_properties2_khr_t                          VkSparseImageFormatProperties2KHR 
#define vk_physical_device_sparse_image_format_info2_khr_t                VkPhysicalDeviceSparseImageFormatInfo2KHR 
#define pfn_vk_get_physical_device_features2_khr_fn_t                     PFN_vkGetPhysicalDeviceFeatures2KHR
#define pfn_vk_get_physical_device_properties2_khr_fn_t                   PFN_vkGetPhysicalDeviceProperties2KHR
#define pfn_vk_get_physical_device_format_properties2_khr_fn_t            PFN_vkGetPhysicalDeviceFormatProperties2KHR
#define pfn_vk_get_physical_device_image_format_properties2_khr_fn_t      PFN_vkGetPhysicalDeviceImageFormatProperties2KHR
#define pfn_vk_get_physical_device_queue_family_properties2_khr_fn_t      PFN_vkGetPhysicalDeviceQueueFamilyProperties2KHR
#define pfn_vk_get_physical_device_memory_properties2_khr_fn_t            PFN_vkGetPhysicalDeviceMemoryProperties2KHR
#define pfn_vk_get_physical_device_sparse_image_format_properties2_khr_fn_t PFN_vkGetPhysicalDeviceSparseImageFormatProperties2KHR

inline VKAPI_ATTR void VKAPI_CALL vk_get_physical_device_features2_khr(
    vk_physical_device_t                               physicalDevice,
    vk_physical_device_features2_t*                    pFeatures)
{
    return vkGetPhysicalDeviceFeatures2KHR (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    VkPhysicalDeviceFeatures2*                  *)&pFeatures);
}

inline VKAPI_ATTR void VKAPI_CALL vk_get_physical_device_properties2_khr(
    vk_physical_device_t                               physicalDevice,
    vk_physical_device_properties2_t*                  pProperties)
{
    return vkGetPhysicalDeviceProperties2KHR (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    VkPhysicalDeviceProperties2*                *)&pProperties);
}

inline VKAPI_ATTR void VKAPI_CALL vk_get_physical_device_format_properties2_khr(
    vk_physical_device_t                               physicalDevice,
    vk_format_t                                        format,
    vk_format_properties2_t*                           pFormatProperties)
{
    return vkGetPhysicalDeviceFormatProperties2KHR (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    VkFormat                                    *)&format,
            *(    VkFormatProperties2*                        *)&pFormatProperties);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_physical_device_image_format_properties2_khr(
    vk_physical_device_t                               physicalDevice,
    const vk_physical_device_image_format_info2_t*     pImageFormatInfo,
    vk_image_format_properties2_t*                     pImageFormatProperties)
{
    return vkGetPhysicalDeviceImageFormatProperties2KHR (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    const VkPhysicalDeviceImageFormatInfo2*     *)&pImageFormatInfo,
            *(    VkImageFormatProperties2*                   *)&pImageFormatProperties);
}

inline VKAPI_ATTR void VKAPI_CALL vk_get_physical_device_queue_family_properties2_khr(
    vk_physical_device_t                               physicalDevice,
    uint32_t*                                          pQueueFamilyPropertyCount,
    vk_queue_family_properties2_t*                     pQueueFamilyProperties)
{
    return vkGetPhysicalDeviceQueueFamilyProperties2KHR (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    uint32_t*                                   *)&pQueueFamilyPropertyCount,
            *(    VkQueueFamilyProperties2*                   *)&pQueueFamilyProperties);
}

inline VKAPI_ATTR void VKAPI_CALL vk_get_physical_device_memory_properties2_khr(
    vk_physical_device_t                               physicalDevice,
    vk_physical_device_memory_properties2_t*           pMemoryProperties)
{
    return vkGetPhysicalDeviceMemoryProperties2KHR (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    VkPhysicalDeviceMemoryProperties2*          *)&pMemoryProperties);
}

inline VKAPI_ATTR void VKAPI_CALL vk_get_physical_device_sparse_image_format_properties2_khr(
    vk_physical_device_t                               physicalDevice,
    const vk_physical_device_sparse_image_format_info2_t* pFormatInfo,
    uint32_t*                                          pPropertyCount,
    vk_sparse_image_format_properties2_t*              pProperties)
{
    return vkGetPhysicalDeviceSparseImageFormatProperties2KHR (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    const VkPhysicalDeviceSparseImageFormatInfo2* *)&pFormatInfo,
            *(    uint32_t*                                   *)&pPropertyCount,
            *(    VkSparseImageFormatProperties2*             *)&pProperties);
}
#define vk_peer_memory_feature_flags_khr_t                                VkPeerMemoryFeatureFlagsKHR 
#define vk_peer_memory_feature_flag_bits_khr_t                            VkPeerMemoryFeatureFlagBitsKHR 
#define vk_memory_allocate_flags_khr_t                                    VkMemoryAllocateFlagsKHR 
#define vk_memory_allocate_flag_bits_khr_t                                VkMemoryAllocateFlagBitsKHR 
#define vk_memory_allocate_flags_info_khr_t                               VkMemoryAllocateFlagsInfoKHR 
#define vk_device_group_render_pass_begin_info_khr_t                      VkDeviceGroupRenderPassBeginInfoKHR 
#define vk_device_group_command_buffer_begin_info_khr_t                   VkDeviceGroupCommandBufferBeginInfoKHR 
#define vk_device_group_submit_info_khr_t                                 VkDeviceGroupSubmitInfoKHR 
#define vk_device_group_bind_sparse_info_khr_t                            VkDeviceGroupBindSparseInfoKHR 
#define vk_bind_buffer_memory_device_group_info_khr_t                     VkBindBufferMemoryDeviceGroupInfoKHR 
#define vk_bind_image_memory_device_group_info_khr_t                      VkBindImageMemoryDeviceGroupInfoKHR 
#define pfn_vk_get_device_group_peer_memory_features_khr_fn_t             PFN_vkGetDeviceGroupPeerMemoryFeaturesKHR
#define pfn_vk_cmd_set_device_mask_khr_fn_t                               PFN_vkCmdSetDeviceMaskKHR
#define pfn_vk_cmd_dispatch_base_khr_fn_t                                 PFN_vkCmdDispatchBaseKHR

inline VKAPI_ATTR void VKAPI_CALL vk_get_device_group_peer_memory_features_khr(
    vk_device_t                                        device,
    uint32_t                                           heapIndex,
    uint32_t                                           localDeviceIndex,
    uint32_t                                           remoteDeviceIndex,
    vk_peer_memory_feature_flags_t*                    pPeerMemoryFeatures)
{
    return vkGetDeviceGroupPeerMemoryFeaturesKHR (
            *(    VkDevice                                    *)&device,
            *(    uint32_t                                    *)&heapIndex,
            *(    uint32_t                                    *)&localDeviceIndex,
            *(    uint32_t                                    *)&remoteDeviceIndex,
            *(    VkPeerMemoryFeatureFlags*                   *)&pPeerMemoryFeatures);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_set_device_mask_khr(
    vk_command_buffer_t                                commandBuffer,
    uint32_t                                           deviceMask)
{
    return vkCmdSetDeviceMaskKHR (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    uint32_t                                    *)&deviceMask);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_dispatch_base_khr(
    vk_command_buffer_t                                commandBuffer,
    uint32_t                                           baseGroupX,
    uint32_t                                           baseGroupY,
    uint32_t                                           baseGroupZ,
    uint32_t                                           groupCountX,
    uint32_t                                           groupCountY,
    uint32_t                                           groupCountZ)
{
    return vkCmdDispatchBaseKHR (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    uint32_t                                    *)&baseGroupX,
            *(    uint32_t                                    *)&baseGroupY,
            *(    uint32_t                                    *)&baseGroupZ,
            *(    uint32_t                                    *)&groupCountX,
            *(    uint32_t                                    *)&groupCountY,
            *(    uint32_t                                    *)&groupCountZ);
}
#define vk_command_pool_trim_flags_khr_t                                  VkCommandPoolTrimFlagsKHR 
#define pfn_vk_trim_command_pool_khr_fn_t                                 PFN_vkTrimCommandPoolKHR

inline VKAPI_ATTR void VKAPI_CALL vk_trim_command_pool_khr(
    vk_device_t                                        device,
    vk_command_pool_t                                  commandPool,
    vk_command_pool_trim_flags_t                       flags)
{
    return vkTrimCommandPoolKHR (
            *(    VkDevice                                    *)&device,
            *(    VkCommandPool                               *)&commandPool,
            *(    VkCommandPoolTrimFlags                      *)&flags);
}
#define vk_physical_device_group_properties_khr_t                         VkPhysicalDeviceGroupPropertiesKHR 
#define vk_device_group_device_create_info_khr_t                          VkDeviceGroupDeviceCreateInfoKHR 
#define pfn_vk_enumerate_physical_device_groups_khr_fn_t                  PFN_vkEnumeratePhysicalDeviceGroupsKHR

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_enumerate_physical_device_groups_khr(
    vk_instance_t                                      instance,
    uint32_t*                                          pPhysicalDeviceGroupCount,
    vk_physical_device_group_properties_t*             pPhysicalDeviceGroupProperties)
{
    return vkEnumeratePhysicalDeviceGroupsKHR (
            *(    VkInstance                                  *)&instance,
            *(    uint32_t*                                   *)&pPhysicalDeviceGroupCount,
            *(    VkPhysicalDeviceGroupProperties*            *)&pPhysicalDeviceGroupProperties);
}
#define vk_external_memory_handle_type_flags_khr_t                        VkExternalMemoryHandleTypeFlagsKHR 
#define vk_external_memory_handle_type_flag_bits_khr_t                    VkExternalMemoryHandleTypeFlagBitsKHR 
#define vk_external_memory_feature_flags_khr_t                            VkExternalMemoryFeatureFlagsKHR 
#define vk_external_memory_feature_flag_bits_khr_t                        VkExternalMemoryFeatureFlagBitsKHR 
#define vk_external_memory_properties_khr_t                               VkExternalMemoryPropertiesKHR 
#define vk_physical_device_external_image_format_info_khr_t               VkPhysicalDeviceExternalImageFormatInfoKHR 
#define vk_external_image_format_properties_khr_t                         VkExternalImageFormatPropertiesKHR 
#define vk_physical_device_external_buffer_info_khr_t                     VkPhysicalDeviceExternalBufferInfoKHR 
#define vk_external_buffer_properties_khr_t                               VkExternalBufferPropertiesKHR 
#define vk_physical_device_id_properties_khr_t                            VkPhysicalDeviceIDPropertiesKHR 
#define pfn_vk_get_physical_device_external_buffer_properties_khr_fn_t    PFN_vkGetPhysicalDeviceExternalBufferPropertiesKHR

inline VKAPI_ATTR void VKAPI_CALL vk_get_physical_device_external_buffer_properties_khr(
    vk_physical_device_t                               physicalDevice,
    const vk_physical_device_external_buffer_info_t*   pExternalBufferInfo,
    vk_external_buffer_properties_t*                   pExternalBufferProperties)
{
    return vkGetPhysicalDeviceExternalBufferPropertiesKHR (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    const VkPhysicalDeviceExternalBufferInfo*   *)&pExternalBufferInfo,
            *(    VkExternalBufferProperties*                 *)&pExternalBufferProperties);
}
#define vk_external_memory_image_create_info_khr_t                        VkExternalMemoryImageCreateInfoKHR 
#define vk_external_memory_buffer_create_info_khr_t                       VkExternalMemoryBufferCreateInfoKHR 
#define vk_export_memory_allocate_info_khr_t                              VkExportMemoryAllocateInfoKHR 

typedef struct vk_import_memory_fd_info_khr_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR;
    const void* p_next =                               NULL;
    vk_external_memory_handle_type_flag_bits_t         handle_type;
    int                                                fd;
} vk_import_memory_fd_info_khr_t;

typedef struct vk_memory_fd_properties_khr_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_MEMORY_FD_PROPERTIES_KHR;
    void* p_next =                                     NULL;
    uint32_t                                           memory_type_bits;
} vk_memory_fd_properties_khr_t;

typedef struct vk_memory_get_fd_info_khr_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
    const void* p_next =                               NULL;
    vk_device_memory_t                                 memory;
    vk_external_memory_handle_type_flag_bits_t         handle_type;
} vk_memory_get_fd_info_khr_t;
#define pfn_vk_get_memory_fd_khr_fn_t                                     PFN_vkGetMemoryFdKHR
#define pfn_vk_get_memory_fd_properties_khr_fn_t                          PFN_vkGetMemoryFdPropertiesKHR

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_memory_fd_khr(
    vk_device_t                                        device,
    const vk_memory_get_fd_info_khr_t*                 pGetFdInfo,
    int*                                               pFd)
{
    return vkGetMemoryFdKHR (
            *(    VkDevice                                    *)&device,
            *(    const VkMemoryGetFdInfoKHR*                 *)&pGetFdInfo,
            *(    int*                                        *)&pFd);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_memory_fd_properties_khr(
    vk_device_t                                        device,
    vk_external_memory_handle_type_flag_bits_t         handleType,
    int                                                fd,
    vk_memory_fd_properties_khr_t*                     pMemoryFdProperties)
{
    return vkGetMemoryFdPropertiesKHR (
            *(    VkDevice                                    *)&device,
            *(    VkExternalMemoryHandleTypeFlagBits          *)&handleType,
            *(    int                                         *)&fd,
            *(    VkMemoryFdPropertiesKHR*                    *)&pMemoryFdProperties);
}
#define vk_external_semaphore_handle_type_flags_khr_t                     VkExternalSemaphoreHandleTypeFlagsKHR 
#define vk_external_semaphore_handle_type_flag_bits_khr_t                 VkExternalSemaphoreHandleTypeFlagBitsKHR 
#define vk_external_semaphore_feature_flags_khr_t                         VkExternalSemaphoreFeatureFlagsKHR 
#define vk_external_semaphore_feature_flag_bits_khr_t                     VkExternalSemaphoreFeatureFlagBitsKHR 
#define vk_physical_device_external_semaphore_info_khr_t                  VkPhysicalDeviceExternalSemaphoreInfoKHR 
#define vk_external_semaphore_properties_khr_t                            VkExternalSemaphorePropertiesKHR 
#define pfn_vk_get_physical_device_external_semaphore_properties_khr_fn_t PFN_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR

inline VKAPI_ATTR void VKAPI_CALL vk_get_physical_device_external_semaphore_properties_khr(
    vk_physical_device_t                               physicalDevice,
    const vk_physical_device_external_semaphore_info_t* pExternalSemaphoreInfo,
    vk_external_semaphore_properties_t*                pExternalSemaphoreProperties)
{
    return vkGetPhysicalDeviceExternalSemaphorePropertiesKHR (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    const VkPhysicalDeviceExternalSemaphoreInfo* *)&pExternalSemaphoreInfo,
            *(    VkExternalSemaphoreProperties*              *)&pExternalSemaphoreProperties);
}
#define vk_semaphore_import_flags_khr_t                                   VkSemaphoreImportFlagsKHR 
#define vk_semaphore_import_flag_bits_khr_t                               VkSemaphoreImportFlagBitsKHR 
#define vk_export_semaphore_create_info_khr_t                             VkExportSemaphoreCreateInfoKHR 

typedef struct vk_import_semaphore_fd_info_khr_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_FD_INFO_KHR;
    const void* p_next =                               NULL;
    vk_semaphore_t                                     semaphore;
    vk_semaphore_import_flags_t                        flags;
    vk_external_semaphore_handle_type_flag_bits_t      handle_type;
    int                                                fd;
} vk_import_semaphore_fd_info_khr_t;

typedef struct vk_semaphore_get_fd_info_khr_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR;
    const void* p_next =                               NULL;
    vk_semaphore_t                                     semaphore;
    vk_external_semaphore_handle_type_flag_bits_t      handle_type;
} vk_semaphore_get_fd_info_khr_t;
#define pfn_vk_import_semaphore_fd_khr_fn_t                               PFN_vkImportSemaphoreFdKHR
#define pfn_vk_get_semaphore_fd_khr_fn_t                                  PFN_vkGetSemaphoreFdKHR

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_import_semaphore_fd_khr(
    vk_device_t                                        device,
    const vk_import_semaphore_fd_info_khr_t*           pImportSemaphoreFdInfo)
{
    return vkImportSemaphoreFdKHR (
            *(    VkDevice                                    *)&device,
            *(    const VkImportSemaphoreFdInfoKHR*           *)&pImportSemaphoreFdInfo);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_semaphore_fd_khr(
    vk_device_t                                        device,
    const vk_semaphore_get_fd_info_khr_t*              pGetFdInfo,
    int*                                               pFd)
{
    return vkGetSemaphoreFdKHR (
            *(    VkDevice                                    *)&device,
            *(    const VkSemaphoreGetFdInfoKHR*              *)&pGetFdInfo,
            *(    int*                                        *)&pFd);
}

typedef struct vk_physical_device_push_descriptor_properties_khr_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PUSH_DESCRIPTOR_PROPERTIES_KHR;
    void* p_next =                                     NULL;
    uint32_t                                           max_push_descriptors;
} vk_physical_device_push_descriptor_properties_khr_t;
#define pfn_vk_cmd_push_descriptor_set_khr_fn_t                           PFN_vkCmdPushDescriptorSetKHR
#define pfn_vk_cmd_push_descriptor_set_with_template_khr_fn_t             PFN_vkCmdPushDescriptorSetWithTemplateKHR

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_push_descriptor_set_khr(
    vk_command_buffer_t                                commandBuffer,
    vk_pipeline_bind_point_t                           pipelineBindPoint,
    vk_pipeline_layout_t                               layout,
    uint32_t                                           set,
    uint32_t                                           descriptorWriteCount,
    const vk_write_descriptor_set_t*                   pDescriptorWrites)
{
    return vkCmdPushDescriptorSetKHR (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    VkPipelineBindPoint                         *)&pipelineBindPoint,
            *(    VkPipelineLayout                            *)&layout,
            *(    uint32_t                                    *)&set,
            *(    uint32_t                                    *)&descriptorWriteCount,
            *(    const VkWriteDescriptorSet*                 *)&pDescriptorWrites);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_push_descriptor_set_with_template_khr(
    vk_command_buffer_t                                commandBuffer,
    vk_descriptor_update_template_t                    descriptorUpdateTemplate,
    vk_pipeline_layout_t                               layout,
    uint32_t                                           set,
    const void*                                        pData)
{
    return vkCmdPushDescriptorSetWithTemplateKHR (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    VkDescriptorUpdateTemplate                  *)&descriptorUpdateTemplate,
            *(    VkPipelineLayout                            *)&layout,
            *(    uint32_t                                    *)&set,
            *(    const void*                                 *)&pData);
}
#define vk_physical_device_shader_float16_int8_features_khr_t             VkPhysicalDeviceShaderFloat16Int8FeaturesKHR 
#define vk_physical_device_float16_int8_features_khr_t                    VkPhysicalDeviceFloat16Int8FeaturesKHR 
#define vk_physical_device16_bit_storage_features_khr_t                   VkPhysicalDevice16BitStorageFeaturesKHR 

typedef struct vk_rect_layer_khr_t {
    vk_offset2_d_t                                     offset;
    vk_extent2d_t                                     extent;
    uint32_t                                           layer;
} vk_rect_layer_khr_t;

typedef struct vk_present_region_khr_t {
    uint32_t                                           rectangle_count;
    const vk_rect_layer_khr_t*                         p_rectangles;
} vk_present_region_khr_t;

typedef struct vk_present_regions_khr_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PRESENT_REGIONS_KHR;
    const void* p_next =                               NULL;
    uint32_t                                           swapchain_count;
    const vk_present_region_khr_t*                     p_regions;
} vk_present_regions_khr_t;
#define vk_descriptor_update_template_khr_t                               VkDescriptorUpdateTemplateKHR 
#define vk_descriptor_update_template_type_khr_t                          VkDescriptorUpdateTemplateTypeKHR 
#define vk_descriptor_update_template_create_flags_khr_t                  VkDescriptorUpdateTemplateCreateFlagsKHR 
#define vk_descriptor_update_template_entry_khr_t                         VkDescriptorUpdateTemplateEntryKHR 
#define vk_descriptor_update_template_create_info_khr_t                   VkDescriptorUpdateTemplateCreateInfoKHR 
#define pfn_vk_create_descriptor_update_template_khr_fn_t                 PFN_vkCreateDescriptorUpdateTemplateKHR
#define pfn_vk_destroy_descriptor_update_template_khr_fn_t                PFN_vkDestroyDescriptorUpdateTemplateKHR
#define pfn_vk_update_descriptor_set_with_template_khr_fn_t               PFN_vkUpdateDescriptorSetWithTemplateKHR

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_create_descriptor_update_template_khr(
    vk_device_t                                        device,
    const vk_descriptor_update_template_create_info_t* pCreateInfo,
    const vk_allocation_callbacks_t*                   pAllocator,
    vk_descriptor_update_template_t*                   pDescriptorUpdateTemplate)
{
    return vkCreateDescriptorUpdateTemplateKHR (
            *(    VkDevice                                    *)&device,
            *(    const VkDescriptorUpdateTemplateCreateInfo* *)&pCreateInfo,
            *(    const VkAllocationCallbacks*                *)&pAllocator,
            *(    VkDescriptorUpdateTemplate*                 *)&pDescriptorUpdateTemplate);
}

inline VKAPI_ATTR void VKAPI_CALL vk_destroy_descriptor_update_template_khr(
    vk_device_t                                        device,
    vk_descriptor_update_template_t                    descriptorUpdateTemplate,
    const vk_allocation_callbacks_t*                   pAllocator)
{
    return vkDestroyDescriptorUpdateTemplateKHR (
            *(    VkDevice                                    *)&device,
            *(    VkDescriptorUpdateTemplate                  *)&descriptorUpdateTemplate,
            *(    const VkAllocationCallbacks*                *)&pAllocator);
}

inline VKAPI_ATTR void VKAPI_CALL vk_update_descriptor_set_with_template_khr(
    vk_device_t                                        device,
    vk_descriptor_set_t                                descriptorSet,
    vk_descriptor_update_template_t                    descriptorUpdateTemplate,
    const void*                                        pData)
{
    return vkUpdateDescriptorSetWithTemplateKHR (
            *(    VkDevice                                    *)&device,
            *(    VkDescriptorSet                             *)&descriptorSet,
            *(    VkDescriptorUpdateTemplate                  *)&descriptorUpdateTemplate,
            *(    const void*                                 *)&pData);
}
#define vk_physical_device_imageless_framebuffer_features_khr_t           VkPhysicalDeviceImagelessFramebufferFeaturesKHR 
#define vk_framebuffer_attachments_create_info_khr_t                      VkFramebufferAttachmentsCreateInfoKHR 
#define vk_framebuffer_attachment_image_info_khr_t                        VkFramebufferAttachmentImageInfoKHR 
#define vk_render_pass_attachment_begin_info_khr_t                        VkRenderPassAttachmentBeginInfoKHR 
#define vk_render_pass_create_info2_khr_t                                 VkRenderPassCreateInfo2KHR 
#define vk_attachment_description2_khr_t                                  VkAttachmentDescription2KHR 
#define vk_attachment_reference2_khr_t                                    VkAttachmentReference2KHR 
#define vk_subpass_description2_khr_t                                     VkSubpassDescription2KHR 
#define vk_subpass_dependency2_khr_t                                      VkSubpassDependency2KHR 
#define vk_subpass_begin_info_khr_t                                       VkSubpassBeginInfoKHR 
#define vk_subpass_end_info_khr_t                                         VkSubpassEndInfoKHR 
#define pfn_vk_create_render_pass2_khr_fn_t                               PFN_vkCreateRenderPass2KHR
#define pfn_vk_cmd_begin_render_pass2_khr_fn_t                            PFN_vkCmdBeginRenderPass2KHR
#define pfn_vk_cmd_next_subpass2_khr_fn_t                                 PFN_vkCmdNextSubpass2KHR
#define pfn_vk_cmd_end_render_pass2_khr_fn_t                              PFN_vkCmdEndRenderPass2KHR

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_create_render_pass2_khr(
    vk_device_t                                        device,
    const vk_render_pass_create_info2_t*               pCreateInfo,
    const vk_allocation_callbacks_t*                   pAllocator,
    vk_render_pass_t*                                  pRenderPass)
{
    return vkCreateRenderPass2KHR (
            *(    VkDevice                                    *)&device,
            *(    const VkRenderPassCreateInfo2*              *)&pCreateInfo,
            *(    const VkAllocationCallbacks*                *)&pAllocator,
            *(    VkRenderPass*                               *)&pRenderPass);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_begin_render_pass2_khr(
    vk_command_buffer_t                                commandBuffer,
    const vk_render_pass_begin_info_t*                 pRenderPassBegin,
    const vk_subpass_begin_info_t*                     pSubpassBeginInfo)
{
    return vkCmdBeginRenderPass2KHR (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    const VkRenderPassBeginInfo*                *)&pRenderPassBegin,
            *(    const VkSubpassBeginInfo*                   *)&pSubpassBeginInfo);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_next_subpass2_khr(
    vk_command_buffer_t                                commandBuffer,
    const vk_subpass_begin_info_t*                     pSubpassBeginInfo,
    const vk_subpass_end_info_t*                       pSubpassEndInfo)
{
    return vkCmdNextSubpass2KHR (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    const VkSubpassBeginInfo*                   *)&pSubpassBeginInfo,
            *(    const VkSubpassEndInfo*                     *)&pSubpassEndInfo);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_end_render_pass2_khr(
    vk_command_buffer_t                                commandBuffer,
    const vk_subpass_end_info_t*                       pSubpassEndInfo)
{
    return vkCmdEndRenderPass2KHR (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    const VkSubpassEndInfo*                     *)&pSubpassEndInfo);
}

typedef struct vk_shared_present_surface_capabilities_khr_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_SHARED_PRESENT_SURFACE_CAPABILITIES_KHR;
    void* p_next =                                     NULL;
    vk_image_usage_flags_t                             shared_present_supported_usage_flags;
} vk_shared_present_surface_capabilities_khr_t;
#define pfn_vk_get_swapchain_status_khr_fn_t                              PFN_vkGetSwapchainStatusKHR

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_swapchain_status_khr(
    vk_device_t                                        device,
    vk_swapchain_khr_t                                 swapchain)
{
    return vkGetSwapchainStatusKHR (
            *(    VkDevice                                    *)&device,
            *(    VkSwapchainKHR                              *)&swapchain);
}
#define vk_external_fence_handle_type_flags_khr_t                         VkExternalFenceHandleTypeFlagsKHR 
#define vk_external_fence_handle_type_flag_bits_khr_t                     VkExternalFenceHandleTypeFlagBitsKHR 
#define vk_external_fence_feature_flags_khr_t                             VkExternalFenceFeatureFlagsKHR 
#define vk_external_fence_feature_flag_bits_khr_t                         VkExternalFenceFeatureFlagBitsKHR 
#define vk_physical_device_external_fence_info_khr_t                      VkPhysicalDeviceExternalFenceInfoKHR 
#define vk_external_fence_properties_khr_t                                VkExternalFencePropertiesKHR 
#define pfn_vk_get_physical_device_external_fence_properties_khr_fn_t     PFN_vkGetPhysicalDeviceExternalFencePropertiesKHR

inline VKAPI_ATTR void VKAPI_CALL vk_get_physical_device_external_fence_properties_khr(
    vk_physical_device_t                               physicalDevice,
    const vk_physical_device_external_fence_info_t*    pExternalFenceInfo,
    vk_external_fence_properties_t*                    pExternalFenceProperties)
{
    return vkGetPhysicalDeviceExternalFencePropertiesKHR (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    const VkPhysicalDeviceExternalFenceInfo*    *)&pExternalFenceInfo,
            *(    VkExternalFenceProperties*                  *)&pExternalFenceProperties);
}
#define vk_fence_import_flags_khr_t                                       VkFenceImportFlagsKHR 
#define vk_fence_import_flag_bits_khr_t                                   VkFenceImportFlagBitsKHR 
#define vk_export_fence_create_info_khr_t                                 VkExportFenceCreateInfoKHR 

typedef struct vk_import_fence_fd_info_khr_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_IMPORT_FENCE_FD_INFO_KHR;
    const void* p_next =                               NULL;
    vk_fence_t                                         fence;
    vk_fence_import_flags_t                            flags;
    vk_external_fence_handle_type_flag_bits_t          handle_type;
    int                                                fd;
} vk_import_fence_fd_info_khr_t;

typedef struct vk_fence_get_fd_info_khr_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_FENCE_GET_FD_INFO_KHR;
    const void* p_next =                               NULL;
    vk_fence_t                                         fence;
    vk_external_fence_handle_type_flag_bits_t          handle_type;
} vk_fence_get_fd_info_khr_t;
#define pfn_vk_import_fence_fd_khr_fn_t                                   PFN_vkImportFenceFdKHR
#define pfn_vk_get_fence_fd_khr_fn_t                                      PFN_vkGetFenceFdKHR

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_import_fence_fd_khr(
    vk_device_t                                        device,
    const vk_import_fence_fd_info_khr_t*               pImportFenceFdInfo)
{
    return vkImportFenceFdKHR (
            *(    VkDevice                                    *)&device,
            *(    const VkImportFenceFdInfoKHR*               *)&pImportFenceFdInfo);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_fence_fd_khr(
    vk_device_t                                        device,
    const vk_fence_get_fd_info_khr_t*                  pGetFdInfo,
    int*                                               pFd)
{
    return vkGetFenceFdKHR (
            *(    VkDevice                                    *)&device,
            *(    const VkFenceGetFdInfoKHR*                  *)&pGetFdInfo,
            *(    int*                                        *)&pFd);
}
#define vk_performance_counter_unit_khr_t                                 VkPerformanceCounterUnitKHR
#define vk_performance_counter_scope_khr_t                                VkPerformanceCounterScopeKHR
#define vk_performance_counter_storage_khr_t                              VkPerformanceCounterStorageKHR
#define vk_performance_counter_description_flag_bits_khr_t                VkPerformanceCounterDescriptionFlagBitsKHR
#define vk_performance_counter_description_flags_khr_t                    VkPerformanceCounterDescriptionFlagsKHR 
#define vk_acquire_profiling_lock_flag_bits_khr_t                         VkAcquireProfilingLockFlagBitsKHR
#define vk_acquire_profiling_lock_flags_khr_t                             VkAcquireProfilingLockFlagsKHR 

typedef struct vk_physical_device_performance_query_features_khr_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PERFORMANCE_QUERY_FEATURES_KHR;
    void* p_next =                                     NULL;
    vk_bool32_t                                        performance_counter_query_pools;
    vk_bool32_t                                        performance_counter_multiple_query_pools;
} vk_physical_device_performance_query_features_khr_t;

typedef struct vk_physical_device_performance_query_properties_khr_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PERFORMANCE_QUERY_PROPERTIES_KHR;
    void* p_next =                                     NULL;
    vk_bool32_t                                        allow_command_buffer_query_copies;
} vk_physical_device_performance_query_properties_khr_t;

typedef struct vk_performance_counter_khr_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PERFORMANCE_COUNTER_KHR;
    const void* p_next =                               NULL;
    vk_performance_counter_unit_khr_t                  unit;
    vk_performance_counter_scope_khr_t                 scope;
    vk_performance_counter_storage_khr_t               storage;
    uint8_t                                            uuid[VK_UUID_SIZE];
} vk_performance_counter_khr_t;

typedef struct vk_performance_counter_description_khr_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PERFORMANCE_COUNTER_DESCRIPTION_KHR;
    const void* p_next =                               NULL;
    vk_performance_counter_description_flags_khr_t     flags;
    char                                               name[VK_MAX_DESCRIPTION_SIZE];
    char                                               category[VK_MAX_DESCRIPTION_SIZE];
    char                                               description[VK_MAX_DESCRIPTION_SIZE];
} vk_performance_counter_description_khr_t;

typedef struct vk_query_pool_performance_create_info_khr_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_QUERY_POOL_PERFORMANCE_CREATE_INFO_KHR;
    const void* p_next =                               NULL;
    uint32_t                                           queue_family_index;
    uint32_t                                           counter_index_count;
    const uint32_t*                                    p_counter_indices;
} vk_query_pool_performance_create_info_khr_t;

typedef union vk_performance_counter_result_khr_t {
    int32_t                                            int32;
    int64_t                                            int64;
    uint32_t                                           uint32;
    uint64_t                                           uint64;
    float                                              float32;
    double                                             float64;
} vk_performance_counter_result_khr_t;

typedef struct vk_acquire_profiling_lock_info_khr_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_ACQUIRE_PROFILING_LOCK_INFO_KHR;
    const void* p_next =                               NULL;
    vk_acquire_profiling_lock_flags_khr_t              flags;
    uint64_t                                           timeout;
} vk_acquire_profiling_lock_info_khr_t;

typedef struct vk_performance_query_submit_info_khr_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PERFORMANCE_QUERY_SUBMIT_INFO_KHR;
    const void* p_next =                               NULL;
    uint32_t                                           counter_pass_index;
} vk_performance_query_submit_info_khr_t;
#define pfn_vk_enumerate_physical_device_queue_family_performance_query_counters_khr_fn_t PFN_vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR
#define pfn_vk_get_physical_device_queue_family_performance_query_passes_khr_fn_t PFN_vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR
#define pfn_vk_acquire_profiling_lock_khr_fn_t                            PFN_vkAcquireProfilingLockKHR
#define pfn_vk_release_profiling_lock_khr_fn_t                            PFN_vkReleaseProfilingLockKHR

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_enumerate_physical_device_queue_family_performance_query_counters_khr(
    vk_physical_device_t                               physicalDevice,
    uint32_t                                           queueFamilyIndex,
    uint32_t*                                          pCounterCount,
    vk_performance_counter_khr_t*                      pCounters,
    vk_performance_counter_description_khr_t*          pCounterDescriptions)
{
    return vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    uint32_t                                    *)&queueFamilyIndex,
            *(    uint32_t*                                   *)&pCounterCount,
            *(    VkPerformanceCounterKHR*                    *)&pCounters,
            *(    VkPerformanceCounterDescriptionKHR*         *)&pCounterDescriptions);
}

inline VKAPI_ATTR void VKAPI_CALL vk_get_physical_device_queue_family_performance_query_passes_khr(
    vk_physical_device_t                               physicalDevice,
    const vk_query_pool_performance_create_info_khr_t* pPerformanceQueryCreateInfo,
    uint32_t*                                          pNumPasses)
{
    return vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    const VkQueryPoolPerformanceCreateInfoKHR*  *)&pPerformanceQueryCreateInfo,
            *(    uint32_t*                                   *)&pNumPasses);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_acquire_profiling_lock_khr(
    vk_device_t                                        device,
    const vk_acquire_profiling_lock_info_khr_t*        pInfo)
{
    return vkAcquireProfilingLockKHR (
            *(    VkDevice                                    *)&device,
            *(    const VkAcquireProfilingLockInfoKHR*        *)&pInfo);
}

inline VKAPI_ATTR void VKAPI_CALL vk_release_profiling_lock_khr(
    vk_device_t                                        device)
{
    return vkReleaseProfilingLockKHR (
            *(    VkDevice                                    *)&device);
}
#define vk_point_clipping_behavior_khr_t                                  VkPointClippingBehaviorKHR 
#define vk_tessellation_domain_origin_khr_t                               VkTessellationDomainOriginKHR 
#define vk_physical_device_point_clipping_properties_khr_t                VkPhysicalDevicePointClippingPropertiesKHR 
#define vk_render_pass_input_attachment_aspect_create_info_khr_t          VkRenderPassInputAttachmentAspectCreateInfoKHR 
#define vk_input_attachment_aspect_reference_khr_t                        VkInputAttachmentAspectReferenceKHR 
#define vk_image_view_usage_create_info_khr_t                             VkImageViewUsageCreateInfoKHR 
#define vk_pipeline_tessellation_domain_origin_state_create_info_khr_t    VkPipelineTessellationDomainOriginStateCreateInfoKHR 

typedef struct vk_physical_device_surface_info2_khr_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR;
    const void* p_next =                               NULL;
    vk_surface_khr_t                                   surface;
} vk_physical_device_surface_info2_khr_t;

typedef struct vk_surface_capabilities2_khr_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR;
    void* p_next =                                     NULL;
    vk_surface_capabilities_khr_t                      surface_capabilities;
} vk_surface_capabilities2_khr_t;

typedef struct vk_surface_format2_khr_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR;
    void* p_next =                                     NULL;
    vk_surface_format_khr_t                            surface_format;
} vk_surface_format2_khr_t;
#define pfn_vk_get_physical_device_surface_capabilities2_khr_fn_t         PFN_vkGetPhysicalDeviceSurfaceCapabilities2KHR
#define pfn_vk_get_physical_device_surface_formats2_khr_fn_t              PFN_vkGetPhysicalDeviceSurfaceFormats2KHR

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_physical_device_surface_capabilities2_khr(
    vk_physical_device_t                               physicalDevice,
    const vk_physical_device_surface_info2_khr_t*      pSurfaceInfo,
    vk_surface_capabilities2_khr_t*                    pSurfaceCapabilities)
{
    return vkGetPhysicalDeviceSurfaceCapabilities2KHR (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    const VkPhysicalDeviceSurfaceInfo2KHR*      *)&pSurfaceInfo,
            *(    VkSurfaceCapabilities2KHR*                  *)&pSurfaceCapabilities);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_physical_device_surface_formats2_khr(
    vk_physical_device_t                               physicalDevice,
    const vk_physical_device_surface_info2_khr_t*      pSurfaceInfo,
    uint32_t*                                          pSurfaceFormatCount,
    vk_surface_format2_khr_t*                          pSurfaceFormats)
{
    return vkGetPhysicalDeviceSurfaceFormats2KHR (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    const VkPhysicalDeviceSurfaceInfo2KHR*      *)&pSurfaceInfo,
            *(    uint32_t*                                   *)&pSurfaceFormatCount,
            *(    VkSurfaceFormat2KHR*                        *)&pSurfaceFormats);
}
#define vk_physical_device_variable_pointer_features_khr_t                VkPhysicalDeviceVariablePointerFeaturesKHR 
#define vk_physical_device_variable_pointers_features_khr_t               VkPhysicalDeviceVariablePointersFeaturesKHR 

typedef struct vk_display_properties2_khr_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_DISPLAY_PROPERTIES_2_KHR;
    void* p_next =                                     NULL;
    vk_display_properties_khr_t                        display_properties;
} vk_display_properties2_khr_t;

typedef struct vk_display_plane_properties2_khr_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_DISPLAY_PLANE_PROPERTIES_2_KHR;
    void* p_next =                                     NULL;
    vk_display_plane_properties_khr_t                  display_plane_properties;
} vk_display_plane_properties2_khr_t;

typedef struct vk_display_mode_properties2_khr_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_DISPLAY_MODE_PROPERTIES_2_KHR;
    void* p_next =                                     NULL;
    vk_display_mode_properties_khr_t                   display_mode_properties;
} vk_display_mode_properties2_khr_t;

typedef struct vk_display_plane_info2_khr_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_DISPLAY_PLANE_INFO_2_KHR;
    const void* p_next =                               NULL;
    vk_display_mode_khr_t                              mode;
    uint32_t                                           plane_index;
} vk_display_plane_info2_khr_t;

typedef struct vk_display_plane_capabilities2_khr_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_DISPLAY_PLANE_CAPABILITIES_2_KHR;
    void* p_next =                                     NULL;
    vk_display_plane_capabilities_khr_t                capabilities;
} vk_display_plane_capabilities2_khr_t;
#define pfn_vk_get_physical_device_display_properties2_khr_fn_t           PFN_vkGetPhysicalDeviceDisplayProperties2KHR
#define pfn_vk_get_physical_device_display_plane_properties2_khr_fn_t     PFN_vkGetPhysicalDeviceDisplayPlaneProperties2KHR
#define pfn_vk_get_display_mode_properties2_khr_fn_t                      PFN_vkGetDisplayModeProperties2KHR
#define pfn_vk_get_display_plane_capabilities2_khr_fn_t                   PFN_vkGetDisplayPlaneCapabilities2KHR

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_physical_device_display_properties2_khr(
    vk_physical_device_t                               physicalDevice,
    uint32_t*                                          pPropertyCount,
    vk_display_properties2_khr_t*                      pProperties)
{
    return vkGetPhysicalDeviceDisplayProperties2KHR (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    uint32_t*                                   *)&pPropertyCount,
            *(    VkDisplayProperties2KHR*                    *)&pProperties);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_physical_device_display_plane_properties2_khr(
    vk_physical_device_t                               physicalDevice,
    uint32_t*                                          pPropertyCount,
    vk_display_plane_properties2_khr_t*                pProperties)
{
    return vkGetPhysicalDeviceDisplayPlaneProperties2KHR (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    uint32_t*                                   *)&pPropertyCount,
            *(    VkDisplayPlaneProperties2KHR*               *)&pProperties);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_display_mode_properties2_khr(
    vk_physical_device_t                               physicalDevice,
    vk_display_khr_t                                   display,
    uint32_t*                                          pPropertyCount,
    vk_display_mode_properties2_khr_t*                 pProperties)
{
    return vkGetDisplayModeProperties2KHR (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    VkDisplayKHR                                *)&display,
            *(    uint32_t*                                   *)&pPropertyCount,
            *(    VkDisplayModeProperties2KHR*                *)&pProperties);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_display_plane_capabilities2_khr(
    vk_physical_device_t                               physicalDevice,
    const vk_display_plane_info2_khr_t*                pDisplayPlaneInfo,
    vk_display_plane_capabilities2_khr_t*              pCapabilities)
{
    return vkGetDisplayPlaneCapabilities2KHR (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    const VkDisplayPlaneInfo2KHR*               *)&pDisplayPlaneInfo,
            *(    VkDisplayPlaneCapabilities2KHR*             *)&pCapabilities);
}
#define vk_memory_dedicated_requirements_khr_t                            VkMemoryDedicatedRequirementsKHR 
#define vk_memory_dedicated_allocate_info_khr_t                           VkMemoryDedicatedAllocateInfoKHR 
#define vk_buffer_memory_requirements_info2_khr_t                         VkBufferMemoryRequirementsInfo2KHR 
#define vk_image_memory_requirements_info2_khr_t                          VkImageMemoryRequirementsInfo2KHR 
#define vk_image_sparse_memory_requirements_info2_khr_t                   VkImageSparseMemoryRequirementsInfo2KHR 
#define vk_sparse_image_memory_requirements2_khr_t                        VkSparseImageMemoryRequirements2KHR 
#define pfn_vk_get_image_memory_requirements2_khr_fn_t                    PFN_vkGetImageMemoryRequirements2KHR
#define pfn_vk_get_buffer_memory_requirements2_khr_fn_t                   PFN_vkGetBufferMemoryRequirements2KHR
#define pfn_vk_get_image_sparse_memory_requirements2_khr_fn_t             PFN_vkGetImageSparseMemoryRequirements2KHR

inline VKAPI_ATTR void VKAPI_CALL vk_get_image_memory_requirements2_khr(
    vk_device_t                                        device,
    const vk_image_memory_requirements_info2_t*        pInfo,
    vk_memory_requirements2_t*                         pMemoryRequirements)
{
    return vkGetImageMemoryRequirements2KHR (
            *(    VkDevice                                    *)&device,
            *(    const VkImageMemoryRequirementsInfo2*       *)&pInfo,
            *(    VkMemoryRequirements2*                      *)&pMemoryRequirements);
}

inline VKAPI_ATTR void VKAPI_CALL vk_get_buffer_memory_requirements2_khr(
    vk_device_t                                        device,
    const vk_buffer_memory_requirements_info2_t*       pInfo,
    vk_memory_requirements2_t*                         pMemoryRequirements)
{
    return vkGetBufferMemoryRequirements2KHR (
            *(    VkDevice                                    *)&device,
            *(    const VkBufferMemoryRequirementsInfo2*      *)&pInfo,
            *(    VkMemoryRequirements2*                      *)&pMemoryRequirements);
}

inline VKAPI_ATTR void VKAPI_CALL vk_get_image_sparse_memory_requirements2_khr(
    vk_device_t                                        device,
    const vk_image_sparse_memory_requirements_info2_t* pInfo,
    uint32_t*                                          pSparseMemoryRequirementCount,
    vk_sparse_image_memory_requirements2_t*            pSparseMemoryRequirements)
{
    return vkGetImageSparseMemoryRequirements2KHR (
            *(    VkDevice                                    *)&device,
            *(    const VkImageSparseMemoryRequirementsInfo2* *)&pInfo,
            *(    uint32_t*                                   *)&pSparseMemoryRequirementCount,
            *(    VkSparseImageMemoryRequirements2*           *)&pSparseMemoryRequirements);
}
#define vk_image_format_list_create_info_khr_t                            VkImageFormatListCreateInfoKHR 
#define vk_sampler_ycbcr_conversion_khr_t                                 VkSamplerYcbcrConversionKHR 
#define vk_sampler_ycbcr_model_conversion_khr_t                           VkSamplerYcbcrModelConversionKHR 
#define vk_sampler_ycbcr_range_khr_t                                      VkSamplerYcbcrRangeKHR 
#define vk_chroma_location_khr_t                                          VkChromaLocationKHR 
#define vk_sampler_ycbcr_conversion_create_info_khr_t                     VkSamplerYcbcrConversionCreateInfoKHR 
#define vk_sampler_ycbcr_conversion_info_khr_t                            VkSamplerYcbcrConversionInfoKHR 
#define vk_bind_image_plane_memory_info_khr_t                             VkBindImagePlaneMemoryInfoKHR 
#define vk_image_plane_memory_requirements_info_khr_t                     VkImagePlaneMemoryRequirementsInfoKHR 
#define vk_physical_device_sampler_ycbcr_conversion_features_khr_t        VkPhysicalDeviceSamplerYcbcrConversionFeaturesKHR 
#define vk_sampler_ycbcr_conversion_image_format_properties_khr_t         VkSamplerYcbcrConversionImageFormatPropertiesKHR 
#define pfn_vk_create_sampler_ycbcr_conversion_khr_fn_t                   PFN_vkCreateSamplerYcbcrConversionKHR
#define pfn_vk_destroy_sampler_ycbcr_conversion_khr_fn_t                  PFN_vkDestroySamplerYcbcrConversionKHR

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_create_sampler_ycbcr_conversion_khr(
    vk_device_t                                        device,
    const vk_sampler_ycbcr_conversion_create_info_t*   pCreateInfo,
    const vk_allocation_callbacks_t*                   pAllocator,
    vk_sampler_ycbcr_conversion_t*                     pYcbcrConversion)
{
    return vkCreateSamplerYcbcrConversionKHR (
            *(    VkDevice                                    *)&device,
            *(    const VkSamplerYcbcrConversionCreateInfo*   *)&pCreateInfo,
            *(    const VkAllocationCallbacks*                *)&pAllocator,
            *(    VkSamplerYcbcrConversion*                   *)&pYcbcrConversion);
}

inline VKAPI_ATTR void VKAPI_CALL vk_destroy_sampler_ycbcr_conversion_khr(
    vk_device_t                                        device,
    vk_sampler_ycbcr_conversion_t                      ycbcrConversion,
    const vk_allocation_callbacks_t*                   pAllocator)
{
    return vkDestroySamplerYcbcrConversionKHR (
            *(    VkDevice                                    *)&device,
            *(    VkSamplerYcbcrConversion                    *)&ycbcrConversion,
            *(    const VkAllocationCallbacks*                *)&pAllocator);
}
#define vk_bind_buffer_memory_info_khr_t                                  VkBindBufferMemoryInfoKHR 
#define vk_bind_image_memory_info_khr_t                                   VkBindImageMemoryInfoKHR 
#define pfn_vk_bind_buffer_memory2_khr_fn_t                               PFN_vkBindBufferMemory2KHR
#define pfn_vk_bind_image_memory2_khr_fn_t                                PFN_vkBindImageMemory2KHR

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_bind_buffer_memory2_khr(
    vk_device_t                                        device,
    uint32_t                                           bindInfoCount,
    const vk_bind_buffer_memory_info_t*                pBindInfos)
{
    return vkBindBufferMemory2KHR (
            *(    VkDevice                                    *)&device,
            *(    uint32_t                                    *)&bindInfoCount,
            *(    const VkBindBufferMemoryInfo*               *)&pBindInfos);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_bind_image_memory2_khr(
    vk_device_t                                        device,
    uint32_t                                           bindInfoCount,
    const vk_bind_image_memory_info_t*                 pBindInfos)
{
    return vkBindImageMemory2KHR (
            *(    VkDevice                                    *)&device,
            *(    uint32_t                                    *)&bindInfoCount,
            *(    const VkBindImageMemoryInfo*                *)&pBindInfos);
}
#define vk_physical_device_maintenance3_properties_khr_t                  VkPhysicalDeviceMaintenance3PropertiesKHR 
#define vk_descriptor_set_layout_support_khr_t                            VkDescriptorSetLayoutSupportKHR 
#define pfn_vk_get_descriptor_set_layout_support_khr_fn_t                 PFN_vkGetDescriptorSetLayoutSupportKHR

inline VKAPI_ATTR void VKAPI_CALL vk_get_descriptor_set_layout_support_khr(
    vk_device_t                                        device,
    const vk_descriptor_set_layout_create_info_t*      pCreateInfo,
    vk_descriptor_set_layout_support_t*                pSupport)
{
    return vkGetDescriptorSetLayoutSupportKHR (
            *(    VkDevice                                    *)&device,
            *(    const VkDescriptorSetLayoutCreateInfo*      *)&pCreateInfo,
            *(    VkDescriptorSetLayoutSupport*               *)&pSupport);
}
#define pfn_vk_cmd_draw_indirect_count_khr_fn_t                           PFN_vkCmdDrawIndirectCountKHR
#define pfn_vk_cmd_draw_indexed_indirect_count_khr_fn_t                   PFN_vkCmdDrawIndexedIndirectCountKHR

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_draw_indirect_count_khr(
    vk_command_buffer_t                                commandBuffer,
    vk_buffer_t                                        buffer,
    vk_device_size_t                                   offset,
    vk_buffer_t                                        countBuffer,
    vk_device_size_t                                   countBufferOffset,
    uint32_t                                           maxDrawCount,
    uint32_t                                           stride)
{
    return vkCmdDrawIndirectCountKHR (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    VkBuffer                                    *)&buffer,
            *(    VkDeviceSize                                *)&offset,
            *(    VkBuffer                                    *)&countBuffer,
            *(    VkDeviceSize                                *)&countBufferOffset,
            *(    uint32_t                                    *)&maxDrawCount,
            *(    uint32_t                                    *)&stride);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_draw_indexed_indirect_count_khr(
    vk_command_buffer_t                                commandBuffer,
    vk_buffer_t                                        buffer,
    vk_device_size_t                                   offset,
    vk_buffer_t                                        countBuffer,
    vk_device_size_t                                   countBufferOffset,
    uint32_t                                           maxDrawCount,
    uint32_t                                           stride)
{
    return vkCmdDrawIndexedIndirectCountKHR (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    VkBuffer                                    *)&buffer,
            *(    VkDeviceSize                                *)&offset,
            *(    VkBuffer                                    *)&countBuffer,
            *(    VkDeviceSize                                *)&countBufferOffset,
            *(    uint32_t                                    *)&maxDrawCount,
            *(    uint32_t                                    *)&stride);
}
#define vk_physical_device_shader_subgroup_extended_types_features_khr_t  VkPhysicalDeviceShaderSubgroupExtendedTypesFeaturesKHR 
#define vk_physical_device8_bit_storage_features_khr_t                    VkPhysicalDevice8BitStorageFeaturesKHR 
#define vk_physical_device_shader_atomic_int64_features_khr_t             VkPhysicalDeviceShaderAtomicInt64FeaturesKHR 

typedef struct vk_physical_device_shader_clock_features_khr_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CLOCK_FEATURES_KHR;
    void* p_next =                                     NULL;
    vk_bool32_t                                        shader_subgroup_clock;
    vk_bool32_t                                        shader_device_clock;
} vk_physical_device_shader_clock_features_khr_t;
#define vk_driver_id_khr_t                                                VkDriverIdKHR 
#define vk_conformance_version_khr_t                                      VkConformanceVersionKHR 
#define vk_physical_device_driver_properties_khr_t                        VkPhysicalDeviceDriverPropertiesKHR 
#define vk_shader_float_controls_independence_khr_t                       VkShaderFloatControlsIndependenceKHR 
#define vk_physical_device_float_controls_properties_khr_t                VkPhysicalDeviceFloatControlsPropertiesKHR 
#define vk_resolve_mode_flag_bits_khr_t                                   VkResolveModeFlagBitsKHR 
#define vk_resolve_mode_flags_khr_t                                       VkResolveModeFlagsKHR 
#define vk_subpass_description_depth_stencil_resolve_khr_t                VkSubpassDescriptionDepthStencilResolveKHR 
#define vk_physical_device_depth_stencil_resolve_properties_khr_t         VkPhysicalDeviceDepthStencilResolvePropertiesKHR 
#define vk_semaphore_type_khr_t                                           VkSemaphoreTypeKHR 
#define vk_semaphore_wait_flag_bits_khr_t                                 VkSemaphoreWaitFlagBitsKHR 
#define vk_semaphore_wait_flags_khr_t                                     VkSemaphoreWaitFlagsKHR 
#define vk_physical_device_timeline_semaphore_features_khr_t              VkPhysicalDeviceTimelineSemaphoreFeaturesKHR 
#define vk_physical_device_timeline_semaphore_properties_khr_t            VkPhysicalDeviceTimelineSemaphorePropertiesKHR 
#define vk_semaphore_type_create_info_khr_t                               VkSemaphoreTypeCreateInfoKHR 
#define vk_timeline_semaphore_submit_info_khr_t                           VkTimelineSemaphoreSubmitInfoKHR 
#define vk_semaphore_wait_info_khr_t                                      VkSemaphoreWaitInfoKHR 
#define vk_semaphore_signal_info_khr_t                                    VkSemaphoreSignalInfoKHR 
#define pfn_vk_get_semaphore_counter_value_khr_fn_t                       PFN_vkGetSemaphoreCounterValueKHR
#define pfn_vk_wait_semaphores_khr_fn_t                                   PFN_vkWaitSemaphoresKHR
#define pfn_vk_signal_semaphore_khr_fn_t                                  PFN_vkSignalSemaphoreKHR

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_semaphore_counter_value_khr(
    vk_device_t                                        device,
    vk_semaphore_t                                     semaphore,
    uint64_t*                                          pValue)
{
    return vkGetSemaphoreCounterValueKHR (
            *(    VkDevice                                    *)&device,
            *(    VkSemaphore                                 *)&semaphore,
            *(    uint64_t*                                   *)&pValue);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_wait_semaphores_khr(
    vk_device_t                                        device,
    const vk_semaphore_wait_info_t*                    pWaitInfo,
    uint64_t                                           timeout)
{
    return vkWaitSemaphoresKHR (
            *(    VkDevice                                    *)&device,
            *(    const VkSemaphoreWaitInfo*                  *)&pWaitInfo,
            *(    uint64_t                                    *)&timeout);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_signal_semaphore_khr(
    vk_device_t                                        device,
    const vk_semaphore_signal_info_t*                  pSignalInfo)
{
    return vkSignalSemaphoreKHR (
            *(    VkDevice                                    *)&device,
            *(    const VkSemaphoreSignalInfo*                *)&pSignalInfo);
}
#define vk_physical_device_vulkan_memory_model_features_khr_t             VkPhysicalDeviceVulkanMemoryModelFeaturesKHR 

typedef struct vk_surface_protected_capabilities_khr_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_SURFACE_PROTECTED_CAPABILITIES_KHR;
    const void* p_next =                               NULL;
    vk_bool32_t                                        supports_protected;
} vk_surface_protected_capabilities_khr_t;
#define vk_physical_device_separate_depth_stencil_layouts_features_khr_t  VkPhysicalDeviceSeparateDepthStencilLayoutsFeaturesKHR 
#define vk_attachment_reference_stencil_layout_khr_t                      VkAttachmentReferenceStencilLayoutKHR 
#define vk_attachment_description_stencil_layout_khr_t                    VkAttachmentDescriptionStencilLayoutKHR 
#define vk_physical_device_uniform_buffer_standard_layout_features_khr_t  VkPhysicalDeviceUniformBufferStandardLayoutFeaturesKHR 
#define vk_physical_device_buffer_device_address_features_khr_t           VkPhysicalDeviceBufferDeviceAddressFeaturesKHR 
#define vk_buffer_device_address_info_khr_t                               VkBufferDeviceAddressInfoKHR 
#define vk_buffer_opaque_capture_address_create_info_khr_t                VkBufferOpaqueCaptureAddressCreateInfoKHR 
#define vk_memory_opaque_capture_address_allocate_info_khr_t              VkMemoryOpaqueCaptureAddressAllocateInfoKHR 
#define vk_device_memory_opaque_capture_address_info_khr_t                VkDeviceMemoryOpaqueCaptureAddressInfoKHR 
#define pfn_vk_get_buffer_device_address_khr_fn_t                         PFN_vkGetBufferDeviceAddressKHR
#define pfn_vk_get_buffer_opaque_capture_address_khr_fn_t                 PFN_vkGetBufferOpaqueCaptureAddressKHR
#define pfn_vk_get_device_memory_opaque_capture_address_khr_fn_t          PFN_vkGetDeviceMemoryOpaqueCaptureAddressKHR

inline VKAPI_ATTR vk_device_address_t VKAPI_CALL vk_get_buffer_device_address_khr(
    vk_device_t                                        device,
    const vk_buffer_device_address_info_t*             pInfo)
{
    return vkGetBufferDeviceAddressKHR (
            *(    VkDevice                                    *)&device,
            *(    const VkBufferDeviceAddressInfo*            *)&pInfo);
}

inline VKAPI_ATTR uint64_t VKAPI_CALL vk_get_buffer_opaque_capture_address_khr(
    vk_device_t                                        device,
    const vk_buffer_device_address_info_t*             pInfo)
{
    return vkGetBufferOpaqueCaptureAddressKHR (
            *(    VkDevice                                    *)&device,
            *(    const VkBufferDeviceAddressInfo*            *)&pInfo);
}

inline VKAPI_ATTR uint64_t VKAPI_CALL vk_get_device_memory_opaque_capture_address_khr(
    vk_device_t                                        device,
    const vk_device_memory_opaque_capture_address_info_t* pInfo)
{
    return vkGetDeviceMemoryOpaqueCaptureAddressKHR (
            *(    VkDevice                                    *)&device,
            *(    const VkDeviceMemoryOpaqueCaptureAddressInfo* *)&pInfo);
}
#define vk_pipeline_executable_statistic_format_khr_t                     VkPipelineExecutableStatisticFormatKHR

typedef struct vk_physical_device_pipeline_executable_properties_features_khr_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_EXECUTABLE_PROPERTIES_FEATURES_KHR;
    void* p_next =                                     NULL;
    vk_bool32_t                                        pipeline_executable_info;
} vk_physical_device_pipeline_executable_properties_features_khr_t;

typedef struct vk_pipeline_info_khr_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PIPELINE_INFO_KHR;
    const void* p_next =                               NULL;
    vk_pipeline_t                                      pipeline;
} vk_pipeline_info_khr_t;

typedef struct vk_pipeline_executable_properties_khr_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_PROPERTIES_KHR;
    void* p_next =                                     NULL;
    vk_shader_stage_flags_t                            stages;
    char                                               name[VK_MAX_DESCRIPTION_SIZE];
    char                                               description[VK_MAX_DESCRIPTION_SIZE];
    uint32_t                                           subgroup_size;
} vk_pipeline_executable_properties_khr_t;

typedef struct vk_pipeline_executable_info_khr_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_INFO_KHR;
    const void* p_next =                               NULL;
    vk_pipeline_t                                      pipeline;
    uint32_t                                           executable_index;
} vk_pipeline_executable_info_khr_t;

typedef union vk_pipeline_executable_statistic_value_khr_t {
    vk_bool32_t                                        b32;
    int64_t                                            i64;
    uint64_t                                           u64;
    double                                             f64;
} vk_pipeline_executable_statistic_value_khr_t;

typedef struct vk_pipeline_executable_statistic_khr_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_STATISTIC_KHR;
    void* p_next =                                     NULL;
    char                                               name[VK_MAX_DESCRIPTION_SIZE];
    char                                               description[VK_MAX_DESCRIPTION_SIZE];
    vk_pipeline_executable_statistic_format_khr_t      format;
    vk_pipeline_executable_statistic_value_khr_t       value;
} vk_pipeline_executable_statistic_khr_t;

typedef struct vk_pipeline_executable_internal_representation_khr_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_INTERNAL_REPRESENTATION_KHR;
    void* p_next =                                     NULL;
    char                                               name[VK_MAX_DESCRIPTION_SIZE];
    char                                               description[VK_MAX_DESCRIPTION_SIZE];
    vk_bool32_t                                        is_text;
    size_t                                             data_size;
    void*                                              p_data;
} vk_pipeline_executable_internal_representation_khr_t;
#define pfn_vk_get_pipeline_executable_properties_khr_fn_t                PFN_vkGetPipelineExecutablePropertiesKHR
#define pfn_vk_get_pipeline_executable_statistics_khr_fn_t                PFN_vkGetPipelineExecutableStatisticsKHR
#define pfn_vk_get_pipeline_executable_internal_representations_khr_fn_t  PFN_vkGetPipelineExecutableInternalRepresentationsKHR

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_pipeline_executable_properties_khr(
    vk_device_t                                        device,
    const vk_pipeline_info_khr_t*                      pPipelineInfo,
    uint32_t*                                          pExecutableCount,
    vk_pipeline_executable_properties_khr_t*           pProperties)
{
    return vkGetPipelineExecutablePropertiesKHR (
            *(    VkDevice                                    *)&device,
            *(    const VkPipelineInfoKHR*                    *)&pPipelineInfo,
            *(    uint32_t*                                   *)&pExecutableCount,
            *(    VkPipelineExecutablePropertiesKHR*          *)&pProperties);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_pipeline_executable_statistics_khr(
    vk_device_t                                        device,
    const vk_pipeline_executable_info_khr_t*           pExecutableInfo,
    uint32_t*                                          pStatisticCount,
    vk_pipeline_executable_statistic_khr_t*            pStatistics)
{
    return vkGetPipelineExecutableStatisticsKHR (
            *(    VkDevice                                    *)&device,
            *(    const VkPipelineExecutableInfoKHR*          *)&pExecutableInfo,
            *(    uint32_t*                                   *)&pStatisticCount,
            *(    VkPipelineExecutableStatisticKHR*           *)&pStatistics);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_pipeline_executable_internal_representations_khr(
    vk_device_t                                        device,
    const vk_pipeline_executable_info_khr_t*           pExecutableInfo,
    uint32_t*                                          pInternalRepresentationCount,
    vk_pipeline_executable_internal_representation_khr_t* pInternalRepresentations)
{
    return vkGetPipelineExecutableInternalRepresentationsKHR (
            *(    VkDevice                                    *)&device,
            *(    const VkPipelineExecutableInfoKHR*          *)&pExecutableInfo,
            *(    uint32_t*                                   *)&pInternalRepresentationCount,
            *(    VkPipelineExecutableInternalRepresentationKHR* *)&pInternalRepresentations);
}
#define vk_debug_report_callback_ext_t                                    VkDebugReportCallbackEXT 
#define vk_debug_report_object_type_ext_t                                 VkDebugReportObjectTypeEXT
#define vk_debug_report_flag_bits_ext_t                                   VkDebugReportFlagBitsEXT
#define vk_debug_report_flags_ext_t                                       VkDebugReportFlagsEXT 
#define pfn_vk_debug_report_callback_ext_fn_t                             PFN_vkDebugReportCallbackEXT

typedef struct vk_debug_report_callback_create_info_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    const void* p_next =                               NULL;
    vk_debug_report_flags_ext_t                        flags;
    pfn_vk_debug_report_callback_ext_fn_t              pfn_callback;
    void*                                              p_user_data;
} vk_debug_report_callback_create_info_ext_t;
#define pfn_vk_create_debug_report_callback_ext_fn_t                      PFN_vkCreateDebugReportCallbackEXT
#define pfn_vk_destroy_debug_report_callback_ext_fn_t                     PFN_vkDestroyDebugReportCallbackEXT
#define pfn_vk_debug_report_message_ext_fn_t                              PFN_vkDebugReportMessageEXT

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_create_debug_report_callback_ext(
    vk_instance_t                                      instance,
    const vk_debug_report_callback_create_info_ext_t*  pCreateInfo,
    const vk_allocation_callbacks_t*                   pAllocator,
    vk_debug_report_callback_ext_t*                    pCallback)
{
    return vkCreateDebugReportCallbackEXT (
            *(    VkInstance                                  *)&instance,
            *(    const VkDebugReportCallbackCreateInfoEXT*   *)&pCreateInfo,
            *(    const VkAllocationCallbacks*                *)&pAllocator,
            *(    VkDebugReportCallbackEXT*                   *)&pCallback);
}

inline VKAPI_ATTR void VKAPI_CALL vk_destroy_debug_report_callback_ext(
    vk_instance_t                                      instance,
    vk_debug_report_callback_ext_t                     callback,
    const vk_allocation_callbacks_t*                   pAllocator)
{
    return vkDestroyDebugReportCallbackEXT (
            *(    VkInstance                                  *)&instance,
            *(    VkDebugReportCallbackEXT                    *)&callback,
            *(    const VkAllocationCallbacks*                *)&pAllocator);
}

inline VKAPI_ATTR void VKAPI_CALL vk_debug_report_message_ext(
    vk_instance_t                                      instance,
    vk_debug_report_flags_ext_t                        flags,
    vk_debug_report_object_type_ext_t                  objectType,
    uint64_t                                           object,
    size_t                                             location,
    int32_t                                            messageCode,
    const char*                                        pLayerPrefix,
    const char*                                        pMessage)
{
    return vkDebugReportMessageEXT (
            *(    VkInstance                                  *)&instance,
            *(    VkDebugReportFlagsEXT                       *)&flags,
            *(    VkDebugReportObjectTypeEXT                  *)&objectType,
            *(    uint64_t                                    *)&object,
            *(    size_t                                      *)&location,
            *(    int32_t                                     *)&messageCode,
            *(    const char*                                 *)&pLayerPrefix,
            *(    const char*                                 *)&pMessage);
}
#define vk_rasterization_order_amd_t                                      VkRasterizationOrderAMD

typedef struct vk_pipeline_rasterization_state_rasterization_order_amd_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_RASTERIZATION_ORDER_AMD;
    const void* p_next =                               NULL;
    vk_rasterization_order_amd_t                       rasterization_order;
} vk_pipeline_rasterization_state_rasterization_order_amd_t;

typedef struct vk_debug_marker_object_name_info_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
    const void* p_next =                               NULL;
    vk_debug_report_object_type_ext_t                  object_type;
    uint64_t                                           object;
    const char*                                        p_object_name;
} vk_debug_marker_object_name_info_ext_t;

typedef struct vk_debug_marker_object_tag_info_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_TAG_INFO_EXT;
    const void* p_next =                               NULL;
    vk_debug_report_object_type_ext_t                  object_type;
    uint64_t                                           object;
    uint64_t                                           tag_name;
    size_t                                             tag_size;
    const void*                                        p_tag;
} vk_debug_marker_object_tag_info_ext_t;

typedef struct vk_debug_marker_marker_info_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
    const void* p_next =                               NULL;
    const char*                                        p_marker_name;
    float                                              color[4];
} vk_debug_marker_marker_info_ext_t;
#define pfn_vk_debug_marker_set_object_tag_ext_fn_t                       PFN_vkDebugMarkerSetObjectTagEXT
#define pfn_vk_debug_marker_set_object_name_ext_fn_t                      PFN_vkDebugMarkerSetObjectNameEXT
#define pfn_vk_cmd_debug_marker_begin_ext_fn_t                            PFN_vkCmdDebugMarkerBeginEXT
#define pfn_vk_cmd_debug_marker_end_ext_fn_t                              PFN_vkCmdDebugMarkerEndEXT
#define pfn_vk_cmd_debug_marker_insert_ext_fn_t                           PFN_vkCmdDebugMarkerInsertEXT

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_debug_marker_set_object_tag_ext(
    vk_device_t                                        device,
    const vk_debug_marker_object_tag_info_ext_t*       pTagInfo)
{
    return vkDebugMarkerSetObjectTagEXT (
            *(    VkDevice                                    *)&device,
            *(    const VkDebugMarkerObjectTagInfoEXT*        *)&pTagInfo);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_debug_marker_set_object_name_ext(
    vk_device_t                                        device,
    const vk_debug_marker_object_name_info_ext_t*      pNameInfo)
{
    return vkDebugMarkerSetObjectNameEXT (
            *(    VkDevice                                    *)&device,
            *(    const VkDebugMarkerObjectNameInfoEXT*       *)&pNameInfo);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_debug_marker_begin_ext(
    vk_command_buffer_t                                commandBuffer,
    const vk_debug_marker_marker_info_ext_t*           pMarkerInfo)
{
    return vkCmdDebugMarkerBeginEXT (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    const VkDebugMarkerMarkerInfoEXT*           *)&pMarkerInfo);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_debug_marker_end_ext(
    vk_command_buffer_t                                commandBuffer)
{
    return vkCmdDebugMarkerEndEXT (
            *(    VkCommandBuffer                             *)&commandBuffer);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_debug_marker_insert_ext(
    vk_command_buffer_t                                commandBuffer,
    const vk_debug_marker_marker_info_ext_t*           pMarkerInfo)
{
    return vkCmdDebugMarkerInsertEXT (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    const VkDebugMarkerMarkerInfoEXT*           *)&pMarkerInfo);
}

typedef struct vk_dedicated_allocation_image_create_info_nv_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_IMAGE_CREATE_INFO_NV;
    const void* p_next =                               NULL;
    vk_bool32_t                                        dedicated_allocation;
} vk_dedicated_allocation_image_create_info_nv_t;

typedef struct vk_dedicated_allocation_buffer_create_info_nv_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_BUFFER_CREATE_INFO_NV;
    const void* p_next =                               NULL;
    vk_bool32_t                                        dedicated_allocation;
} vk_dedicated_allocation_buffer_create_info_nv_t;

typedef struct vk_dedicated_allocation_memory_allocate_info_nv_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_MEMORY_ALLOCATE_INFO_NV;
    const void* p_next =                               NULL;
    vk_image_t                                         image;
    vk_buffer_t                                        buffer;
} vk_dedicated_allocation_memory_allocate_info_nv_t;
#define vk_pipeline_rasterization_state_stream_create_flags_ext_t         VkPipelineRasterizationStateStreamCreateFlagsEXT 

typedef struct vk_physical_device_transform_feedback_features_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_FEATURES_EXT;
    void* p_next =                                     NULL;
    vk_bool32_t                                        transform_feedback;
    vk_bool32_t                                        geometry_streams;
} vk_physical_device_transform_feedback_features_ext_t;

typedef struct vk_physical_device_transform_feedback_properties_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_PROPERTIES_EXT;
    void* p_next =                                     NULL;
    uint32_t                                           max_transform_feedback_streams;
    uint32_t                                           max_transform_feedback_buffers;
    vk_device_size_t                                   max_transform_feedback_buffer_size;
    uint32_t                                           max_transform_feedback_stream_data_size;
    uint32_t                                           max_transform_feedback_buffer_data_size;
    uint32_t                                           max_transform_feedback_buffer_data_stride;
    vk_bool32_t                                        transform_feedback_queries;
    vk_bool32_t                                        transform_feedback_streams_lines_triangles;
    vk_bool32_t                                        transform_feedback_rasterization_stream_select;
    vk_bool32_t                                        transform_feedback_draw;
} vk_physical_device_transform_feedback_properties_ext_t;

typedef struct vk_pipeline_rasterization_state_stream_create_info_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_STREAM_CREATE_INFO_EXT;
    const void* p_next =                               NULL;
    vk_pipeline_rasterization_state_stream_create_flags_ext_t flags;
    uint32_t                                           rasterization_stream;
} vk_pipeline_rasterization_state_stream_create_info_ext_t;
#define pfn_vk_cmd_bind_transform_feedback_buffers_ext_fn_t               PFN_vkCmdBindTransformFeedbackBuffersEXT
#define pfn_vk_cmd_begin_transform_feedback_ext_fn_t                      PFN_vkCmdBeginTransformFeedbackEXT
#define pfn_vk_cmd_end_transform_feedback_ext_fn_t                        PFN_vkCmdEndTransformFeedbackEXT
#define pfn_vk_cmd_begin_query_indexed_ext_fn_t                           PFN_vkCmdBeginQueryIndexedEXT
#define pfn_vk_cmd_end_query_indexed_ext_fn_t                             PFN_vkCmdEndQueryIndexedEXT
#define pfn_vk_cmd_draw_indirect_byte_count_ext_fn_t                      PFN_vkCmdDrawIndirectByteCountEXT

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_bind_transform_feedback_buffers_ext(
    vk_command_buffer_t                                commandBuffer,
    uint32_t                                           firstBinding,
    uint32_t                                           bindingCount,
    const vk_buffer_t*                                 pBuffers,
    const vk_device_size_t*                            pOffsets,
    const vk_device_size_t*                            pSizes)
{
    return vkCmdBindTransformFeedbackBuffersEXT (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    uint32_t                                    *)&firstBinding,
            *(    uint32_t                                    *)&bindingCount,
            *(    const VkBuffer*                             *)&pBuffers,
            *(    const VkDeviceSize*                         *)&pOffsets,
            *(    const VkDeviceSize*                         *)&pSizes);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_begin_transform_feedback_ext(
    vk_command_buffer_t                                commandBuffer,
    uint32_t                                           firstCounterBuffer,
    uint32_t                                           counterBufferCount,
    const vk_buffer_t*                                 pCounterBuffers,
    const vk_device_size_t*                            pCounterBufferOffsets)
{
    return vkCmdBeginTransformFeedbackEXT (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    uint32_t                                    *)&firstCounterBuffer,
            *(    uint32_t                                    *)&counterBufferCount,
            *(    const VkBuffer*                             *)&pCounterBuffers,
            *(    const VkDeviceSize*                         *)&pCounterBufferOffsets);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_end_transform_feedback_ext(
    vk_command_buffer_t                                commandBuffer,
    uint32_t                                           firstCounterBuffer,
    uint32_t                                           counterBufferCount,
    const vk_buffer_t*                                 pCounterBuffers,
    const vk_device_size_t*                            pCounterBufferOffsets)
{
    return vkCmdEndTransformFeedbackEXT (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    uint32_t                                    *)&firstCounterBuffer,
            *(    uint32_t                                    *)&counterBufferCount,
            *(    const VkBuffer*                             *)&pCounterBuffers,
            *(    const VkDeviceSize*                         *)&pCounterBufferOffsets);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_begin_query_indexed_ext(
    vk_command_buffer_t                                commandBuffer,
    vk_query_pool_t                                    queryPool,
    uint32_t                                           query,
    vk_query_control_flags_t                           flags,
    uint32_t                                           index)
{
    return vkCmdBeginQueryIndexedEXT (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    VkQueryPool                                 *)&queryPool,
            *(    uint32_t                                    *)&query,
            *(    VkQueryControlFlags                         *)&flags,
            *(    uint32_t                                    *)&index);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_end_query_indexed_ext(
    vk_command_buffer_t                                commandBuffer,
    vk_query_pool_t                                    queryPool,
    uint32_t                                           query,
    uint32_t                                           index)
{
    return vkCmdEndQueryIndexedEXT (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    VkQueryPool                                 *)&queryPool,
            *(    uint32_t                                    *)&query,
            *(    uint32_t                                    *)&index);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_draw_indirect_byte_count_ext(
    vk_command_buffer_t                                commandBuffer,
    uint32_t                                           instanceCount,
    uint32_t                                           firstInstance,
    vk_buffer_t                                        counterBuffer,
    vk_device_size_t                                   counterBufferOffset,
    uint32_t                                           counterOffset,
    uint32_t                                           vertexStride)
{
    return vkCmdDrawIndirectByteCountEXT (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    uint32_t                                    *)&instanceCount,
            *(    uint32_t                                    *)&firstInstance,
            *(    VkBuffer                                    *)&counterBuffer,
            *(    VkDeviceSize                                *)&counterBufferOffset,
            *(    uint32_t                                    *)&counterOffset,
            *(    uint32_t                                    *)&vertexStride);
}

typedef struct vk_image_view_handle_info_nvx_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_IMAGE_VIEW_HANDLE_INFO_NVX;
    const void* p_next =                               NULL;
    vk_image_view_t                                    image_view;
    vk_descriptor_type_t                               descriptor_type;
    vk_sampler_t                                       sampler;
} vk_image_view_handle_info_nvx_t;
#define pfn_vk_get_image_view_handle_nvx_fn_t                             PFN_vkGetImageViewHandleNVX

inline VKAPI_ATTR uint32_t VKAPI_CALL vk_get_image_view_handle_nvx(
    vk_device_t                                        device,
    const vk_image_view_handle_info_nvx_t*             pInfo)
{
    return vkGetImageViewHandleNVX (
            *(    VkDevice                                    *)&device,
            *(    const VkImageViewHandleInfoNVX*             *)&pInfo);
}
#define pfn_vk_cmd_draw_indirect_count_amd_fn_t                           PFN_vkCmdDrawIndirectCountAMD
#define pfn_vk_cmd_draw_indexed_indirect_count_amd_fn_t                   PFN_vkCmdDrawIndexedIndirectCountAMD

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_draw_indirect_count_amd(
    vk_command_buffer_t                                commandBuffer,
    vk_buffer_t                                        buffer,
    vk_device_size_t                                   offset,
    vk_buffer_t                                        countBuffer,
    vk_device_size_t                                   countBufferOffset,
    uint32_t                                           maxDrawCount,
    uint32_t                                           stride)
{
    return vkCmdDrawIndirectCountAMD (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    VkBuffer                                    *)&buffer,
            *(    VkDeviceSize                                *)&offset,
            *(    VkBuffer                                    *)&countBuffer,
            *(    VkDeviceSize                                *)&countBufferOffset,
            *(    uint32_t                                    *)&maxDrawCount,
            *(    uint32_t                                    *)&stride);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_draw_indexed_indirect_count_amd(
    vk_command_buffer_t                                commandBuffer,
    vk_buffer_t                                        buffer,
    vk_device_size_t                                   offset,
    vk_buffer_t                                        countBuffer,
    vk_device_size_t                                   countBufferOffset,
    uint32_t                                           maxDrawCount,
    uint32_t                                           stride)
{
    return vkCmdDrawIndexedIndirectCountAMD (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    VkBuffer                                    *)&buffer,
            *(    VkDeviceSize                                *)&offset,
            *(    VkBuffer                                    *)&countBuffer,
            *(    VkDeviceSize                                *)&countBufferOffset,
            *(    uint32_t                                    *)&maxDrawCount,
            *(    uint32_t                                    *)&stride);
}

typedef struct vk_texture_lod_gather_format_properties_amd_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_TEXTURE_LOD_GATHER_FORMAT_PROPERTIES_AMD;
    void* p_next =                                     NULL;
    vk_bool32_t                                        supports_texture_gather_lod_bias_amd;
} vk_texture_lod_gather_format_properties_amd_t;
#define vk_shader_info_type_amd_t                                         VkShaderInfoTypeAMD

typedef struct vk_shader_resource_usage_amd_t {
    uint32_t                                           num_used_vgprs;
    uint32_t                                           num_used_sgprs;
    uint32_t                                           lds_size_per_local_work_group;
    size_t                                             lds_usage_size_in_bytes;
    size_t                                             scratch_mem_usage_in_bytes;
} vk_shader_resource_usage_amd_t;

typedef struct vk_shader_statistics_info_amd_t {
    vk_shader_stage_flags_t                            shader_stage_mask;
    vk_shader_resource_usage_amd_t                     resource_usage;
    uint32_t                                           num_physical_vgprs;
    uint32_t                                           num_physical_sgprs;
    uint32_t                                           num_available_vgprs;
    uint32_t                                           num_available_sgprs;
    uint32_t                                           compute_work_group_size[3];
} vk_shader_statistics_info_amd_t;
#define pfn_vk_get_shader_info_amd_fn_t                                   PFN_vkGetShaderInfoAMD

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_shader_info_amd(
    vk_device_t                                        device,
    vk_pipeline_t                                      pipeline,
    vk_shader_stage_flag_bits_t                        shaderStage,
    vk_shader_info_type_amd_t                          infoType,
    size_t*                                            pInfoSize,
    void*                                              pInfo)
{
    return vkGetShaderInfoAMD (
            *(    VkDevice                                    *)&device,
            *(    VkPipeline                                  *)&pipeline,
            *(    VkShaderStageFlagBits                       *)&shaderStage,
            *(    VkShaderInfoTypeAMD                         *)&infoType,
            *(    size_t*                                     *)&pInfoSize,
            *(    void*                                       *)&pInfo);
}

typedef struct vk_physical_device_corner_sampled_image_features_nv_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CORNER_SAMPLED_IMAGE_FEATURES_NV;
    void* p_next =                                     NULL;
    vk_bool32_t                                        corner_sampled_image;
} vk_physical_device_corner_sampled_image_features_nv_t;
#define vk_external_memory_handle_type_flag_bits_nv_t                     VkExternalMemoryHandleTypeFlagBitsNV
#define vk_external_memory_handle_type_flags_nv_t                         VkExternalMemoryHandleTypeFlagsNV 
#define vk_external_memory_feature_flag_bits_nv_t                         VkExternalMemoryFeatureFlagBitsNV
#define vk_external_memory_feature_flags_nv_t                             VkExternalMemoryFeatureFlagsNV 

typedef struct vk_external_image_format_properties_nv_t {
    vk_image_format_properties_t                       image_format_properties;
    vk_external_memory_feature_flags_nv_t              external_memory_features;
    vk_external_memory_handle_type_flags_nv_t          export_from_imported_handle_types;
    vk_external_memory_handle_type_flags_nv_t          compatible_handle_types;
} vk_external_image_format_properties_nv_t;
#define pfn_vk_get_physical_device_external_image_format_properties_nv_fn_t PFN_vkGetPhysicalDeviceExternalImageFormatPropertiesNV

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_physical_device_external_image_format_properties_nv(
    vk_physical_device_t                               physicalDevice,
    vk_format_t                                        format,
    vk_image_type_t                                    type,
    vk_image_tiling_t                                  tiling,
    vk_image_usage_flags_t                             usage,
    vk_image_create_flags_t                            flags,
    vk_external_memory_handle_type_flags_nv_t          externalHandleType,
    vk_external_image_format_properties_nv_t*          pExternalImageFormatProperties)
{
    return vkGetPhysicalDeviceExternalImageFormatPropertiesNV (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    VkFormat                                    *)&format,
            *(    VkImageType                                 *)&type,
            *(    VkImageTiling                               *)&tiling,
            *(    VkImageUsageFlags                           *)&usage,
            *(    VkImageCreateFlags                          *)&flags,
            *(    VkExternalMemoryHandleTypeFlagsNV           *)&externalHandleType,
            *(    VkExternalImageFormatPropertiesNV*          *)&pExternalImageFormatProperties);
}

typedef struct vk_external_memory_image_create_info_nv_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO_NV;
    const void* p_next =                               NULL;
    vk_external_memory_handle_type_flags_nv_t          handle_types;
} vk_external_memory_image_create_info_nv_t;

typedef struct vk_export_memory_allocate_info_nv_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO_NV;
    const void* p_next =                               NULL;
    vk_external_memory_handle_type_flags_nv_t          handle_types;
} vk_export_memory_allocate_info_nv_t;
#define vk_validation_check_ext_t                                         VkValidationCheckEXT

typedef struct vk_validation_flags_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_VALIDATION_FLAGS_EXT;
    const void* p_next =                               NULL;
    uint32_t                                           disabled_validation_check_count;
    const vk_validation_check_ext_t*                   p_disabled_validation_checks;
} vk_validation_flags_ext_t;

typedef struct vk_physical_device_texture_compression_astchdr_features_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TEXTURE_COMPRESSION_ASTC_HDR_FEATURES_EXT;
    void* p_next =                                     NULL;
    vk_bool32_t                                        texture_compression_astc_hdr;
} vk_physical_device_texture_compression_astchdr_features_ext_t;

typedef struct vk_image_view_astc_decode_mode_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_IMAGE_VIEW_ASTC_DECODE_MODE_EXT;
    const void* p_next =                               NULL;
    vk_format_t                                        decode_mode;
} vk_image_view_astc_decode_mode_ext_t;

typedef struct vk_physical_device_astc_decode_features_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ASTC_DECODE_FEATURES_EXT;
    void* p_next =                                     NULL;
    vk_bool32_t                                        decode_mode_shared_exponent;
} vk_physical_device_astc_decode_features_ext_t;
#define vk_conditional_rendering_flag_bits_ext_t                          VkConditionalRenderingFlagBitsEXT
#define vk_conditional_rendering_flags_ext_t                              VkConditionalRenderingFlagsEXT 

typedef struct vk_conditional_rendering_begin_info_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_CONDITIONAL_RENDERING_BEGIN_INFO_EXT;
    const void* p_next =                               NULL;
    vk_buffer_t                                        buffer;
    vk_device_size_t                                   offset;
    vk_conditional_rendering_flags_ext_t               flags;
} vk_conditional_rendering_begin_info_ext_t;

typedef struct vk_physical_device_conditional_rendering_features_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONDITIONAL_RENDERING_FEATURES_EXT;
    void* p_next =                                     NULL;
    vk_bool32_t                                        conditional_rendering;
    vk_bool32_t                                        inherited_conditional_rendering;
} vk_physical_device_conditional_rendering_features_ext_t;

typedef struct vk_command_buffer_inheritance_conditional_rendering_info_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_CONDITIONAL_RENDERING_INFO_EXT;
    const void* p_next =                               NULL;
    vk_bool32_t                                        conditional_rendering_enable;
} vk_command_buffer_inheritance_conditional_rendering_info_ext_t;
#define pfn_vk_cmd_begin_conditional_rendering_ext_fn_t                   PFN_vkCmdBeginConditionalRenderingEXT
#define pfn_vk_cmd_end_conditional_rendering_ext_fn_t                     PFN_vkCmdEndConditionalRenderingEXT

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_begin_conditional_rendering_ext(
    vk_command_buffer_t                                commandBuffer,
    const vk_conditional_rendering_begin_info_ext_t*   pConditionalRenderingBegin)
{
    return vkCmdBeginConditionalRenderingEXT (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    const VkConditionalRenderingBeginInfoEXT*   *)&pConditionalRenderingBegin);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_end_conditional_rendering_ext(
    vk_command_buffer_t                                commandBuffer)
{
    return vkCmdEndConditionalRenderingEXT (
            *(    VkCommandBuffer                             *)&commandBuffer);
}

/* This nvx stuff, I don't know what it does and it doesn't exist on one of my computers */
// #define vk_object_table_nvx_t                                             VkObjectTableNVX 
// #define vk_indirect_commands_layout_nvx_t                                 VkIndirectCommandsLayoutNVX 
// #define vk_indirect_commands_token_type_nvx_t                             VkIndirectCommandsTokenTypeNVX
// #define vk_object_entry_type_nvx_t                                        VkObjectEntryTypeNVX
// #define vk_indirect_commands_layout_usage_flag_bits_nvx_t                 VkIndirectCommandsLayoutUsageFlagBitsNVX
// #define vk_indirect_commands_layout_usage_flags_nvx_t                     VkIndirectCommandsLayoutUsageFlagsNVX 
// #define vk_object_entry_usage_flag_bits_nvx_t                             VkObjectEntryUsageFlagBitsNVX
// #define vk_object_entry_usage_flags_nvx_t                                 VkObjectEntryUsageFlagsNVX 

// typedef struct vk_device_generated_commands_features_nvx_t {
//     vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_DEVICE_GENERATED_COMMANDS_FEATURES_NVX;
//     const void* p_next =                               NULL;
//     vk_bool32_t                                        compute_binding_point_support;
// } vk_device_generated_commands_features_nvx_t;

// typedef struct vk_device_generated_commands_limits_nvx_t {
//     vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_DEVICE_GENERATED_COMMANDS_LIMITS_NVX;
//     const void* p_next =                               NULL;
//     uint32_t                                           max_indirect_commands_layout_token_count;
//     uint32_t                                           max_object_entry_counts;
//     uint32_t                                           min_sequence_count_buffer_offset_alignment;
//     uint32_t                                           min_sequence_index_buffer_offset_alignment;
//     uint32_t                                           min_commands_token_buffer_offset_alignment;
// } vk_device_generated_commands_limits_nvx_t;

// typedef struct vk_indirect_commands_token_nvx_t {
//     vk_indirect_commands_token_type_nvx_t              token_type;
//     vk_buffer_t                                        buffer;
//     vk_device_size_t                                   offset;
// } vk_indirect_commands_token_nvx_t;

// typedef struct vk_indirect_commands_layout_token_nvx_t {
//     vk_indirect_commands_token_type_nvx_t              token_type;
//     uint32_t                                           binding_unit;
//     uint32_t                                           dynamic_count;
//     uint32_t                                           divisor;
// } vk_indirect_commands_layout_token_nvx_t;

// typedef struct vk_indirect_commands_layout_create_info_nvx_t {
//     vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_INDIRECT_COMMANDS_LAYOUT_CREATE_INFO_NVX;
//     const void* p_next =                               NULL;
//     vk_pipeline_bind_point_t                           pipeline_bind_point;
//     vk_indirect_commands_layout_usage_flags_nvx_t      flags;
//     uint32_t                                           token_count;
//     const vk_indirect_commands_layout_token_nvx_t*     p_tokens;
// } vk_indirect_commands_layout_create_info_nvx_t;

// typedef struct vk_cmd_process_commands_info_nvx_t {
//     vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_CMD_PROCESS_COMMANDS_INFO_NVX;
//     const void* p_next =                               NULL;
//     vk_object_table_nvx_t                              object_table;
//     vk_indirect_commands_layout_nvx_t                  indirect_commands_layout;
//     uint32_t                                           indirect_commands_token_count;
//     const vk_indirect_commands_token_nvx_t*            p_indirect_commands_tokens;
//     uint32_t                                           max_sequences_count;
//     vk_command_buffer_t                                target_command_buffer;
//     vk_buffer_t                                        sequences_count_buffer;
//     vk_device_size_t                                   sequences_count_offset;
//     vk_buffer_t                                        sequences_index_buffer;
//     vk_device_size_t                                   sequences_index_offset;
// } vk_cmd_process_commands_info_nvx_t;

// typedef struct vk_cmd_reserve_space_for_commands_info_nvx_t {
//     vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_CMD_RESERVE_SPACE_FOR_COMMANDS_INFO_NVX;
//     const void* p_next =                               NULL;
//     vk_object_table_nvx_t                              object_table;
//     vk_indirect_commands_layout_nvx_t                  indirect_commands_layout;
//     uint32_t                                           max_sequences_count;
// } vk_cmd_reserve_space_for_commands_info_nvx_t;

// typedef struct vk_object_table_create_info_nvx_t {
//     vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_OBJECT_TABLE_CREATE_INFO_NVX;
//     const void* p_next =                               NULL;
//     uint32_t                                           object_count;
//     const vk_object_entry_type_nvx_t*                  p_object_entry_types;
//     const uint32_t*                                    p_object_entry_counts;
//     const vk_object_entry_usage_flags_nvx_t*           p_object_entry_usage_flags;
//     uint32_t                                           max_uniform_buffers_per_descriptor;
//     uint32_t                                           max_storage_buffers_per_descriptor;
//     uint32_t                                           max_storage_images_per_descriptor;
//     uint32_t                                           max_sampled_images_per_descriptor;
//     uint32_t                                           max_pipeline_layouts;
// } vk_object_table_create_info_nvx_t;

// typedef struct vk_object_table_entry_nvx_t {
//     vk_object_entry_type_nvx_t                         type;
//     vk_object_entry_usage_flags_nvx_t                  flags;
// } vk_object_table_entry_nvx_t;

// typedef struct vk_object_table_pipeline_entry_nvx_t {
//     vk_object_entry_type_nvx_t                         type;
//     vk_object_entry_usage_flags_nvx_t                  flags;
//     vk_pipeline_t                                      pipeline;
// } vk_object_table_pipeline_entry_nvx_t;

// typedef struct vk_object_table_descriptor_set_entry_nvx_t {
//     vk_object_entry_type_nvx_t                         type;
//     vk_object_entry_usage_flags_nvx_t                  flags;
//     vk_pipeline_layout_t                               pipeline_layout;
//     vk_descriptor_set_t                                descriptor_set;
// } vk_object_table_descriptor_set_entry_nvx_t;

// typedef struct vk_object_table_vertex_buffer_entry_nvx_t {
//     vk_object_entry_type_nvx_t                         type;
//     vk_object_entry_usage_flags_nvx_t                  flags;
//     vk_buffer_t                                        buffer;
// } vk_object_table_vertex_buffer_entry_nvx_t;

// typedef struct vk_object_table_index_buffer_entry_nvx_t {
//     vk_object_entry_type_nvx_t                         type;
//     vk_object_entry_usage_flags_nvx_t                  flags;
//     vk_buffer_t                                        buffer;
//     vk_index_type_t                                    index_type;
// } vk_object_table_index_buffer_entry_nvx_t;

// typedef struct vk_object_table_push_constant_entry_nvx_t {
//     vk_object_entry_type_nvx_t                         type;
//     vk_object_entry_usage_flags_nvx_t                  flags;
//     vk_pipeline_layout_t                               pipeline_layout;
//     vk_shader_stage_flags_t                            stage_flags;
// } vk_object_table_push_constant_entry_nvx_t;
// #define pfn_vk_cmd_process_commands_nvx_fn_t                              PFN_vkCmdProcessCommandsNVX
// #define pfn_vk_cmd_reserve_space_for_commands_nvx_fn_t                    PFN_vkCmdReserveSpaceForCommandsNVX
// #define pfn_vk_create_indirect_commands_layout_nvx_fn_t                   PFN_vkCreateIndirectCommandsLayoutNVX
// #define pfn_vk_destroy_indirect_commands_layout_nvx_fn_t                  PFN_vkDestroyIndirectCommandsLayoutNVX
// #define pfn_vk_create_object_table_nvx_fn_t                               PFN_vkCreateObjectTableNVX
// #define pfn_vk_destroy_object_table_nvx_fn_t                              PFN_vkDestroyObjectTableNVX
// #define pfn_vk_register_objects_nvx_fn_t                                  PFN_vkRegisterObjectsNVX
// #define pfn_vk_unregister_objects_nvx_fn_t                                PFN_vkUnregisterObjectsNVX
// #define pfn_vk_get_physical_device_generated_commands_properties_nvx_fn_t PFN_vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX

// inline VKAPI_ATTR void VKAPI_CALL vk_cmd_process_commands_nvx(
//     vk_command_buffer_t                                commandBuffer,
//     const vk_cmd_process_commands_info_nvx_t*          pProcessCommandsInfo)
// {
//     return vkCmdProcessCommandsNVX (
//             *(    VkCommandBuffer                             *)&commandBuffer,
//             *(    const VkCmdProcessCommandsInfoNVX*          *)&pProcessCommandsInfo);
// }

// inline VKAPI_ATTR void VKAPI_CALL vk_cmd_reserve_space_for_commands_nvx(
//     vk_command_buffer_t                                commandBuffer,
//     const vk_cmd_reserve_space_for_commands_info_nvx_t* pReserveSpaceInfo)
// {
//     return vkCmdReserveSpaceForCommandsNVX (
//             *(    VkCommandBuffer                             *)&commandBuffer,
//             *(    const VkCmdReserveSpaceForCommandsInfoNVX*  *)&pReserveSpaceInfo);
// }

// inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_create_indirect_commands_layout_nvx(
//     vk_device_t                                        device,
//     const vk_indirect_commands_layout_create_info_nvx_t* pCreateInfo,
//     const vk_allocation_callbacks_t*                   pAllocator,
//     vk_indirect_commands_layout_nvx_t*                 pIndirectCommandsLayout)
// {
//     return vkCreateIndirectCommandsLayoutNVX (
//             *(    VkDevice                                    *)&device,
//             *(    const VkIndirectCommandsLayoutCreateInfoNVX* *)&pCreateInfo,
//             *(    const VkAllocationCallbacks*                *)&pAllocator,
//             *(    VkIndirectCommandsLayoutNVX*                *)&pIndirectCommandsLayout);
// }

// inline VKAPI_ATTR void VKAPI_CALL vk_destroy_indirect_commands_layout_nvx(
//     vk_device_t                                        device,
//     vk_indirect_commands_layout_nvx_t                  indirectCommandsLayout,
//     const vk_allocation_callbacks_t*                   pAllocator)
// {
//     return vkDestroyIndirectCommandsLayoutNVX (
//             *(    VkDevice                                    *)&device,
//             *(    VkIndirectCommandsLayoutNVX                 *)&indirectCommandsLayout,
//             *(    const VkAllocationCallbacks*                *)&pAllocator);
// }

// inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_create_object_table_nvx(
//     vk_device_t                                        device,
//     const vk_object_table_create_info_nvx_t*           pCreateInfo,
//     const vk_allocation_callbacks_t*                   pAllocator,
//     vk_object_table_nvx_t*                             pObjectTable)
// {
//     return vkCreateObjectTableNVX (
//             *(    VkDevice                                    *)&device,
//             *(    const VkObjectTableCreateInfoNVX*           *)&pCreateInfo,
//             *(    const VkAllocationCallbacks*                *)&pAllocator,
//             *(    VkObjectTableNVX*                           *)&pObjectTable);
// }

// inline VKAPI_ATTR void VKAPI_CALL vk_destroy_object_table_nvx(
//     vk_device_t                                        device,
//     vk_object_table_nvx_t                              objectTable,
//     const vk_allocation_callbacks_t*                   pAllocator)
// {
//     return vkDestroyObjectTableNVX (
//             *(    VkDevice                                    *)&device,
//             *(    VkObjectTableNVX                            *)&objectTable,
//             *(    const VkAllocationCallbacks*                *)&pAllocator);
// }

// inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_register_objects_nvx(
//     vk_device_t                                        device,
//     vk_object_table_nvx_t                              objectTable,
//     uint32_t                                           objectCount,
//     const vk_object_table_entry_nvx_t* const*          ppObjectTableEntries,
//     const uint32_t*                                    pObjectIndices)
// {
//     return vkRegisterObjectsNVX (
//             *(    VkDevice                                    *)&device,
//             *(    VkObjectTableNVX                            *)&objectTable,
//             *(    uint32_t                                    *)&objectCount,
//             *(    const VkObjectTableEntryNVX* const*         *)&ppObjectTableEntries,
//             *(    const uint32_t*                             *)&pObjectIndices);
// }

// inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_unregister_objects_nvx(
//     vk_device_t                                        device,
//     vk_object_table_nvx_t                              objectTable,
//     uint32_t                                           objectCount,
//     const vk_object_entry_type_nvx_t*                  pObjectEntryTypes,
//     const uint32_t*                                    pObjectIndices)
// {
//     return vkUnregisterObjectsNVX (
//             *(    VkDevice                                    *)&device,
//             *(    VkObjectTableNVX                            *)&objectTable,
//             *(    uint32_t                                    *)&objectCount,
//             *(    const VkObjectEntryTypeNVX*                 *)&pObjectEntryTypes,
//             *(    const uint32_t*                             *)&pObjectIndices);
// }

// inline VKAPI_ATTR void VKAPI_CALL vk_get_physical_device_generated_commands_properties_nvx(
//     vk_physical_device_t                               physicalDevice,
//     vk_device_generated_commands_features_nvx_t*       pFeatures,
//     vk_device_generated_commands_limits_nvx_t*         pLimits)
// {
//     return vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX (
//             *(    VkPhysicalDevice                            *)&physicalDevice,
//             *(    VkDeviceGeneratedCommandsFeaturesNVX*       *)&pFeatures,
//             *(    VkDeviceGeneratedCommandsLimitsNVX*         *)&pLimits);
// }

typedef struct vk_viewport_w_scaling_nv_t {
    float                                              xcoeff;
    float                                              ycoeff;
} vk_viewport_w_scaling_nv_t;

typedef struct vk_pipeline_viewport_w_scaling_state_create_info_nv_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_W_SCALING_STATE_CREATE_INFO_NV;
    const void* p_next =                               NULL;
    vk_bool32_t                                        viewport_w_scaling_enable;
    uint32_t                                           viewport_count;
    const vk_viewport_w_scaling_nv_t*                  p_viewport_w_scalings;
} vk_pipeline_viewport_w_scaling_state_create_info_nv_t;
#define pfn_vk_cmd_set_viewport_w_scaling_nv_fn_t                         PFN_vkCmdSetViewportWScalingNV

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_set_viewport_w_scaling_nv(
    vk_command_buffer_t                                commandBuffer,
    uint32_t                                           firstViewport,
    uint32_t                                           viewportCount,
    const vk_viewport_w_scaling_nv_t*                  pViewportWScalings)
{
    return vkCmdSetViewportWScalingNV (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    uint32_t                                    *)&firstViewport,
            *(    uint32_t                                    *)&viewportCount,
            *(    const VkViewportWScalingNV*                 *)&pViewportWScalings);
}
#define pfn_vk_release_display_ext_fn_t                                   PFN_vkReleaseDisplayEXT

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_release_display_ext(
    vk_physical_device_t                               physicalDevice,
    vk_display_khr_t                                   display)
{
    return vkReleaseDisplayEXT (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    VkDisplayKHR                                *)&display);
}
#define vk_surface_counter_flag_bits_ext_t                                VkSurfaceCounterFlagBitsEXT
#define vk_surface_counter_flags_ext_t                                    VkSurfaceCounterFlagsEXT 

typedef struct vk_surface_capabilities2_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES2_EXT;
    void* p_next =                                     NULL;
    uint32_t                                           min_image_count;
    uint32_t                                           max_image_count;
    vk_extent2d_t                                     current_extent;
    vk_extent2d_t                                     min_image_extent;
    vk_extent2d_t                                     max_image_extent;
    uint32_t                                           max_image_array_layers;
    vk_surface_transform_flags_khr_t                   supported_transforms;
    vk_surface_transform_flag_bits_khr_t               current_transform;
    vk_composite_alpha_flags_khr_t                     supported_composite_alpha;
    vk_image_usage_flags_t                             supported_usage_flags;
    vk_surface_counter_flags_ext_t                     supported_surface_counters;
} vk_surface_capabilities2_ext_t;
#define pfn_vk_get_physical_device_surface_capabilities2_ext_fn_t         PFN_vkGetPhysicalDeviceSurfaceCapabilities2EXT

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_physical_device_surface_capabilities2_ext(
    vk_physical_device_t                               physicalDevice,
    vk_surface_khr_t                                   surface,
    vk_surface_capabilities2_ext_t*                    pSurfaceCapabilities)
{
    return vkGetPhysicalDeviceSurfaceCapabilities2EXT (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    VkSurfaceKHR                                *)&surface,
            *(    VkSurfaceCapabilities2EXT*                  *)&pSurfaceCapabilities);
}
#define vk_display_power_state_ext_t                                      VkDisplayPowerStateEXT
#define vk_device_event_type_ext_t                                        VkDeviceEventTypeEXT
#define vk_display_event_type_ext_t                                       VkDisplayEventTypeEXT

typedef struct vk_display_power_info_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_DISPLAY_POWER_INFO_EXT;
    const void* p_next =                               NULL;
    vk_display_power_state_ext_t                       power_state;
} vk_display_power_info_ext_t;

typedef struct vk_device_event_info_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_DEVICE_EVENT_INFO_EXT;
    const void* p_next =                               NULL;
    vk_device_event_type_ext_t                         device_event;
} vk_device_event_info_ext_t;

typedef struct vk_display_event_info_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_DISPLAY_EVENT_INFO_EXT;
    const void* p_next =                               NULL;
    vk_display_event_type_ext_t                        display_event;
} vk_display_event_info_ext_t;

typedef struct vk_swapchain_counter_create_info_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_SWAPCHAIN_COUNTER_CREATE_INFO_EXT;
    const void* p_next =                               NULL;
    vk_surface_counter_flags_ext_t                     surface_counters;
} vk_swapchain_counter_create_info_ext_t;
#define pfn_vk_display_power_control_ext_fn_t                             PFN_vkDisplayPowerControlEXT
#define pfn_vk_register_device_event_ext_fn_t                             PFN_vkRegisterDeviceEventEXT
#define pfn_vk_register_display_event_ext_fn_t                            PFN_vkRegisterDisplayEventEXT
#define pfn_vk_get_swapchain_counter_ext_fn_t                             PFN_vkGetSwapchainCounterEXT

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_display_power_control_ext(
    vk_device_t                                        device,
    vk_display_khr_t                                   display,
    const vk_display_power_info_ext_t*                 pDisplayPowerInfo)
{
    return vkDisplayPowerControlEXT (
            *(    VkDevice                                    *)&device,
            *(    VkDisplayKHR                                *)&display,
            *(    const VkDisplayPowerInfoEXT*                *)&pDisplayPowerInfo);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_register_device_event_ext(
    vk_device_t                                        device,
    const vk_device_event_info_ext_t*                  pDeviceEventInfo,
    const vk_allocation_callbacks_t*                   pAllocator,
    vk_fence_t*                                        pFence)
{
    return vkRegisterDeviceEventEXT (
            *(    VkDevice                                    *)&device,
            *(    const VkDeviceEventInfoEXT*                 *)&pDeviceEventInfo,
            *(    const VkAllocationCallbacks*                *)&pAllocator,
            *(    VkFence*                                    *)&pFence);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_register_display_event_ext(
    vk_device_t                                        device,
    vk_display_khr_t                                   display,
    const vk_display_event_info_ext_t*                 pDisplayEventInfo,
    const vk_allocation_callbacks_t*                   pAllocator,
    vk_fence_t*                                        pFence)
{
    return vkRegisterDisplayEventEXT (
            *(    VkDevice                                    *)&device,
            *(    VkDisplayKHR                                *)&display,
            *(    const VkDisplayEventInfoEXT*                *)&pDisplayEventInfo,
            *(    const VkAllocationCallbacks*                *)&pAllocator,
            *(    VkFence*                                    *)&pFence);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_swapchain_counter_ext(
    vk_device_t                                        device,
    vk_swapchain_khr_t                                 swapchain,
    vk_surface_counter_flag_bits_ext_t                 counter,
    uint64_t*                                          pCounterValue)
{
    return vkGetSwapchainCounterEXT (
            *(    VkDevice                                    *)&device,
            *(    VkSwapchainKHR                              *)&swapchain,
            *(    VkSurfaceCounterFlagBitsEXT                 *)&counter,
            *(    uint64_t*                                   *)&pCounterValue);
}

typedef struct vk_refresh_cycle_duration_google_t {
    uint64_t                                           refresh_duration;
} vk_refresh_cycle_duration_google_t;

typedef struct vk_past_presentation_timing_google_t {
    uint32_t                                           present_id;
    uint64_t                                           desired_present_time;
    uint64_t                                           actual_present_time;
    uint64_t                                           earliest_present_time;
    uint64_t                                           present_margin;
} vk_past_presentation_timing_google_t;

typedef struct vk_present_time_google_t {
    uint32_t                                           present_id;
    uint64_t                                           desired_present_time;
} vk_present_time_google_t;

typedef struct vk_present_times_info_google_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PRESENT_TIMES_INFO_GOOGLE;
    const void* p_next =                               NULL;
    uint32_t                                           swapchain_count;
    const vk_present_time_google_t*                    p_times;
} vk_present_times_info_google_t;
#define pfn_vk_get_refresh_cycle_duration_google_fn_t                     PFN_vkGetRefreshCycleDurationGOOGLE
#define pfn_vk_get_past_presentation_timing_google_fn_t                   PFN_vkGetPastPresentationTimingGOOGLE

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_refresh_cycle_duration_google(
    vk_device_t                                        device,
    vk_swapchain_khr_t                                 swapchain,
    vk_refresh_cycle_duration_google_t*                pDisplayTimingProperties)
{
    return vkGetRefreshCycleDurationGOOGLE (
            *(    VkDevice                                    *)&device,
            *(    VkSwapchainKHR                              *)&swapchain,
            *(    VkRefreshCycleDurationGOOGLE*               *)&pDisplayTimingProperties);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_past_presentation_timing_google(
    vk_device_t                                        device,
    vk_swapchain_khr_t                                 swapchain,
    uint32_t*                                          pPresentationTimingCount,
    vk_past_presentation_timing_google_t*              pPresentationTimings)
{
    return vkGetPastPresentationTimingGOOGLE (
            *(    VkDevice                                    *)&device,
            *(    VkSwapchainKHR                              *)&swapchain,
            *(    uint32_t*                                   *)&pPresentationTimingCount,
            *(    VkPastPresentationTimingGOOGLE*             *)&pPresentationTimings);
}

typedef struct vk_physical_device_multiview_per_view_attributes_properties_nvx_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PER_VIEW_ATTRIBUTES_PROPERTIES_NVX;
    void* p_next =                                     NULL;
    vk_bool32_t                                        per_view_position_all_components;
} vk_physical_device_multiview_per_view_attributes_properties_nvx_t;
#define vk_viewport_coordinate_swizzle_nv_t                               VkViewportCoordinateSwizzleNV
#define vk_pipeline_viewport_swizzle_state_create_flags_nv_t              VkPipelineViewportSwizzleStateCreateFlagsNV 

typedef struct vk_viewport_swizzle_nv_t {
    vk_viewport_coordinate_swizzle_nv_t                x;
    vk_viewport_coordinate_swizzle_nv_t                y;
    vk_viewport_coordinate_swizzle_nv_t                z;
    vk_viewport_coordinate_swizzle_nv_t                w;
} vk_viewport_swizzle_nv_t;

typedef struct vk_pipeline_viewport_swizzle_state_create_info_nv_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_SWIZZLE_STATE_CREATE_INFO_NV;
    const void* p_next =                               NULL;
    vk_pipeline_viewport_swizzle_state_create_flags_nv_t flags;
    uint32_t                                           viewport_count;
    const vk_viewport_swizzle_nv_t*                    p_viewport_swizzles;
} vk_pipeline_viewport_swizzle_state_create_info_nv_t;
#define vk_discard_rectangle_mode_ext_t                                   VkDiscardRectangleModeEXT
#define vk_pipeline_discard_rectangle_state_create_flags_ext_t            VkPipelineDiscardRectangleStateCreateFlagsEXT 

typedef struct vk_physical_device_discard_rectangle_properties_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DISCARD_RECTANGLE_PROPERTIES_EXT;
    void* p_next =                                     NULL;
    uint32_t                                           max_discard_rectangles;
} vk_physical_device_discard_rectangle_properties_ext_t;

typedef struct vk_pipeline_discard_rectangle_state_create_info_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PIPELINE_DISCARD_RECTANGLE_STATE_CREATE_INFO_EXT;
    const void* p_next =                               NULL;
    vk_pipeline_discard_rectangle_state_create_flags_ext_t flags;
    vk_discard_rectangle_mode_ext_t                    discard_rectangle_mode;
    uint32_t                                           discard_rectangle_count;
    const vk_rect2d_t*                                p_discard_rectangles;
} vk_pipeline_discard_rectangle_state_create_info_ext_t;
#define pfn_vk_cmd_set_discard_rectangle_ext_fn_t                         PFN_vkCmdSetDiscardRectangleEXT

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_set_discard_rectangle_ext(
    vk_command_buffer_t                                commandBuffer,
    uint32_t                                           firstDiscardRectangle,
    uint32_t                                           discardRectangleCount,
    const vk_rect2d_t*                                pDiscardRectangles)
{
    return vkCmdSetDiscardRectangleEXT (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    uint32_t                                    *)&firstDiscardRectangle,
            *(    uint32_t                                    *)&discardRectangleCount,
            *(    const VkRect2D*                             *)&pDiscardRectangles);
}
#define vk_conservative_rasterization_mode_ext_t                          VkConservativeRasterizationModeEXT
#define vk_pipeline_rasterization_conservative_state_create_flags_ext_t   VkPipelineRasterizationConservativeStateCreateFlagsEXT 

typedef struct vk_physical_device_conservative_rasterization_properties_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONSERVATIVE_RASTERIZATION_PROPERTIES_EXT;
    void* p_next =                                     NULL;
    float                                              primitive_overestimation_size;
    float                                              max_extra_primitive_overestimation_size;
    float                                              extra_primitive_overestimation_size_granularity;
    vk_bool32_t                                        primitive_underestimation;
    vk_bool32_t                                        conservative_point_and_line_rasterization;
    vk_bool32_t                                        degenerate_triangles_rasterized;
    vk_bool32_t                                        degenerate_lines_rasterized;
    vk_bool32_t                                        fully_covered_fragment_shader_input_variable;
    vk_bool32_t                                        conservative_rasterization_post_depth_coverage;
} vk_physical_device_conservative_rasterization_properties_ext_t;

typedef struct vk_pipeline_rasterization_conservative_state_create_info_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_CONSERVATIVE_STATE_CREATE_INFO_EXT;
    const void* p_next =                               NULL;
    vk_pipeline_rasterization_conservative_state_create_flags_ext_t flags;
    vk_conservative_rasterization_mode_ext_t           conservative_rasterization_mode;
    float                                              extra_primitive_overestimation_size;
} vk_pipeline_rasterization_conservative_state_create_info_ext_t;
#define vk_pipeline_rasterization_depth_clip_state_create_flags_ext_t     VkPipelineRasterizationDepthClipStateCreateFlagsEXT 

typedef struct vk_physical_device_depth_clip_enable_features_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_ENABLE_FEATURES_EXT;
    void* p_next =                                     NULL;
    vk_bool32_t                                        depth_clip_enable;
} vk_physical_device_depth_clip_enable_features_ext_t;

typedef struct vk_pipeline_rasterization_depth_clip_state_create_info_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_DEPTH_CLIP_STATE_CREATE_INFO_EXT;
    const void* p_next =                               NULL;
    vk_pipeline_rasterization_depth_clip_state_create_flags_ext_t flags;
    vk_bool32_t                                        depth_clip_enable;
} vk_pipeline_rasterization_depth_clip_state_create_info_ext_t;

typedef struct vk_xy_color_ext_t {
    float                                              x;
    float                                              y;
} vk_xy_color_ext_t;

typedef struct vk_hdr_metadata_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_HDR_METADATA_EXT;
    const void* p_next =                               NULL;
    vk_xy_color_ext_t                                  display_primary_red;
    vk_xy_color_ext_t                                  display_primary_green;
    vk_xy_color_ext_t                                  display_primary_blue;
    vk_xy_color_ext_t                                  white_point;
    float                                              max_luminance;
    float                                              min_luminance;
    float                                              max_content_light_level;
    float                                              max_frame_average_light_level;
} vk_hdr_metadata_ext_t;
#define pfn_vk_set_hdr_metadata_ext_fn_t                                  PFN_vkSetHdrMetadataEXT

inline VKAPI_ATTR void VKAPI_CALL vk_set_hdr_metadata_ext(
    vk_device_t                                        device,
    uint32_t                                           swapchainCount,
    const vk_swapchain_khr_t*                          pSwapchains,
    const vk_hdr_metadata_ext_t*                       pMetadata)
{
    return vkSetHdrMetadataEXT (
            *(    VkDevice                                    *)&device,
            *(    uint32_t                                    *)&swapchainCount,
            *(    const VkSwapchainKHR*                       *)&pSwapchains,
            *(    const VkHdrMetadataEXT*                     *)&pMetadata);
}
#define vk_debug_utils_messenger_ext_t                                    VkDebugUtilsMessengerEXT 
#define vk_debug_utils_messenger_callback_data_flags_ext_t                VkDebugUtilsMessengerCallbackDataFlagsEXT 
#define vk_debug_utils_messenger_create_flags_ext_t                       VkDebugUtilsMessengerCreateFlagsEXT 
#define vk_debug_utils_message_severity_flag_bits_ext_t                   VkDebugUtilsMessageSeverityFlagBitsEXT
#define vk_debug_utils_message_severity_flags_ext_t                       VkDebugUtilsMessageSeverityFlagsEXT 
#define vk_debug_utils_message_type_flag_bits_ext_t                       VkDebugUtilsMessageTypeFlagBitsEXT
#define vk_debug_utils_message_type_flags_ext_t                           VkDebugUtilsMessageTypeFlagsEXT 

typedef struct vk_debug_utils_object_name_info_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    const void* p_next =                               NULL;
    vk_object_type_t                                   object_type;
    uint64_t                                           object_handle;
    const char*                                        p_object_name;
} vk_debug_utils_object_name_info_ext_t;

typedef struct vk_debug_utils_object_tag_info_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_TAG_INFO_EXT;
    const void* p_next =                               NULL;
    vk_object_type_t                                   object_type;
    uint64_t                                           object_handle;
    uint64_t                                           tag_name;
    size_t                                             tag_size;
    const void*                                        p_tag;
} vk_debug_utils_object_tag_info_ext_t;

typedef struct vk_debug_utils_label_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
    const void* p_next =                               NULL;
    const char*                                        p_label_name;
    float                                              color[4];
} vk_debug_utils_label_ext_t;

typedef struct vk_debug_utils_messenger_callback_data_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CALLBACK_DATA_EXT;
    const void* p_next =                               NULL;
    vk_debug_utils_messenger_callback_data_flags_ext_t flags;
    const char*                                        p_message_id_name;
    int32_t                                            message_id_number;
    const char*                                        p_message;
    uint32_t                                           queue_label_count;
    const vk_debug_utils_label_ext_t*                  p_queue_labels;
    uint32_t                                           cmd_buf_label_count;
    const vk_debug_utils_label_ext_t*                  p_cmd_buf_labels;
    uint32_t                                           object_count;
    const vk_debug_utils_object_name_info_ext_t*       p_objects;
} vk_debug_utils_messenger_callback_data_ext_t;

typedef struct vk_debug_utils_messenger_create_info_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    const void* p_next =                               NULL;
    vk_debug_utils_messenger_create_flags_ext_t        flags;
    vk_debug_utils_message_severity_flags_ext_t        message_severity;
    vk_debug_utils_message_type_flags_ext_t            message_type;
    PFN_vkDebugUtilsMessengerCallbackEXT               pfn_user_callback;
    void*                                              p_user_data;
} vk_debug_utils_messenger_create_info_ext_t;
#define pfn_vk_set_debug_utils_object_name_ext_fn_t                       PFN_vkSetDebugUtilsObjectNameEXT
#define pfn_vk_set_debug_utils_object_tag_ext_fn_t                        PFN_vkSetDebugUtilsObjectTagEXT
#define pfn_vk_queue_begin_debug_utils_label_ext_fn_t                     PFN_vkQueueBeginDebugUtilsLabelEXT
#define pfn_vk_queue_end_debug_utils_label_ext_fn_t                       PFN_vkQueueEndDebugUtilsLabelEXT
#define pfn_vk_queue_insert_debug_utils_label_ext_fn_t                    PFN_vkQueueInsertDebugUtilsLabelEXT
#define pfn_vk_cmd_begin_debug_utils_label_ext_fn_t                       PFN_vkCmdBeginDebugUtilsLabelEXT
#define pfn_vk_cmd_end_debug_utils_label_ext_fn_t                         PFN_vkCmdEndDebugUtilsLabelEXT
#define pfn_vk_cmd_insert_debug_utils_label_ext_fn_t                      PFN_vkCmdInsertDebugUtilsLabelEXT
#define pfn_vk_create_debug_utils_messenger_ext_fn_t                      PFN_vkCreateDebugUtilsMessengerEXT
#define pfn_vk_destroy_debug_utils_messenger_ext_fn_t                     PFN_vkDestroyDebugUtilsMessengerEXT
#define pfn_vk_submit_debug_utils_message_ext_fn_t                        PFN_vkSubmitDebugUtilsMessageEXT

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_set_debug_utils_object_name_ext(
    vk_device_t                                        device,
    const vk_debug_utils_object_name_info_ext_t*       pNameInfo)
{
    return vkSetDebugUtilsObjectNameEXT (
            *(    VkDevice                                    *)&device,
            *(    const VkDebugUtilsObjectNameInfoEXT*        *)&pNameInfo);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_set_debug_utils_object_tag_ext(
    vk_device_t                                        device,
    const vk_debug_utils_object_tag_info_ext_t*        pTagInfo)
{
    return vkSetDebugUtilsObjectTagEXT (
            *(    VkDevice                                    *)&device,
            *(    const VkDebugUtilsObjectTagInfoEXT*         *)&pTagInfo);
}

inline VKAPI_ATTR void VKAPI_CALL vk_queue_begin_debug_utils_label_ext(
    vk_queue_t                                         queue,
    const vk_debug_utils_label_ext_t*                  pLabelInfo)
{
    return vkQueueBeginDebugUtilsLabelEXT (
            *(    VkQueue                                     *)&queue,
            *(    const VkDebugUtilsLabelEXT*                 *)&pLabelInfo);
}

inline VKAPI_ATTR void VKAPI_CALL vk_queue_end_debug_utils_label_ext(
    vk_queue_t                                         queue)
{
    return vkQueueEndDebugUtilsLabelEXT (
            *(    VkQueue                                     *)&queue);
}

inline VKAPI_ATTR void VKAPI_CALL vk_queue_insert_debug_utils_label_ext(
    vk_queue_t                                         queue,
    const vk_debug_utils_label_ext_t*                  pLabelInfo)
{
    return vkQueueInsertDebugUtilsLabelEXT (
            *(    VkQueue                                     *)&queue,
            *(    const VkDebugUtilsLabelEXT*                 *)&pLabelInfo);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_begin_debug_utils_label_ext(
    vk_command_buffer_t                                commandBuffer,
    const vk_debug_utils_label_ext_t*                  pLabelInfo)
{
    return vkCmdBeginDebugUtilsLabelEXT (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    const VkDebugUtilsLabelEXT*                 *)&pLabelInfo);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_end_debug_utils_label_ext(
    vk_command_buffer_t                                commandBuffer)
{
    return vkCmdEndDebugUtilsLabelEXT (
            *(    VkCommandBuffer                             *)&commandBuffer);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_insert_debug_utils_label_ext(
    vk_command_buffer_t                                commandBuffer,
    const vk_debug_utils_label_ext_t*                  pLabelInfo)
{
    return vkCmdInsertDebugUtilsLabelEXT (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    const VkDebugUtilsLabelEXT*                 *)&pLabelInfo);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_create_debug_utils_messenger_ext(
    vk_instance_t                                      instance,
    const vk_debug_utils_messenger_create_info_ext_t*  pCreateInfo,
    const vk_allocation_callbacks_t*                   pAllocator,
    vk_debug_utils_messenger_ext_t*                    pMessenger)
{
    return vkCreateDebugUtilsMessengerEXT (
            *(    VkInstance                                  *)&instance,
            *(    const VkDebugUtilsMessengerCreateInfoEXT*   *)&pCreateInfo,
            *(    const VkAllocationCallbacks*                *)&pAllocator,
            *(    VkDebugUtilsMessengerEXT*                   *)&pMessenger);
}

inline VKAPI_ATTR void VKAPI_CALL vk_destroy_debug_utils_messenger_ext(
    vk_instance_t                                      instance,
    vk_debug_utils_messenger_ext_t                     messenger,
    const vk_allocation_callbacks_t*                   pAllocator)
{
    return vkDestroyDebugUtilsMessengerEXT (
            *(    VkInstance                                  *)&instance,
            *(    VkDebugUtilsMessengerEXT                    *)&messenger,
            *(    const VkAllocationCallbacks*                *)&pAllocator);
}

inline VKAPI_ATTR void VKAPI_CALL vk_submit_debug_utils_message_ext(
    vk_instance_t                                      instance,
    vk_debug_utils_message_severity_flag_bits_ext_t    messageSeverity,
    vk_debug_utils_message_type_flags_ext_t            messageTypes,
    const vk_debug_utils_messenger_callback_data_ext_t* pCallbackData)
{
    return vkSubmitDebugUtilsMessageEXT (
            *(    VkInstance                                  *)&instance,
            *(    VkDebugUtilsMessageSeverityFlagBitsEXT      *)&messageSeverity,
            *(    VkDebugUtilsMessageTypeFlagsEXT             *)&messageTypes,
            *(    const VkDebugUtilsMessengerCallbackDataEXT* *)&pCallbackData);
}
#define vk_sampler_reduction_mode_ext_t                                   VkSamplerReductionModeEXT 
#define vk_sampler_reduction_mode_create_info_ext_t                       VkSamplerReductionModeCreateInfoEXT 
#define vk_physical_device_sampler_filter_minmax_properties_ext_t         VkPhysicalDeviceSamplerFilterMinmaxPropertiesEXT 

typedef struct vk_physical_device_inline_uniform_block_features_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INLINE_UNIFORM_BLOCK_FEATURES_EXT;
    void* p_next =                                     NULL;
    vk_bool32_t                                        inline_uniform_block;
    vk_bool32_t                                        descriptor_binding_inline_uniform_block_update_after_bind;
} vk_physical_device_inline_uniform_block_features_ext_t;

typedef struct vk_physical_device_inline_uniform_block_properties_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INLINE_UNIFORM_BLOCK_PROPERTIES_EXT;
    void* p_next =                                     NULL;
    uint32_t                                           max_inline_uniform_block_size;
    uint32_t                                           max_per_stage_descriptor_inline_uniform_blocks;
    uint32_t                                           max_per_stage_descriptor_update_after_bind_inline_uniform_blocks;
    uint32_t                                           max_descriptor_set_inline_uniform_blocks;
    uint32_t                                           max_descriptor_set_update_after_bind_inline_uniform_blocks;
} vk_physical_device_inline_uniform_block_properties_ext_t;

typedef struct vk_write_descriptor_set_inline_uniform_block_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_INLINE_UNIFORM_BLOCK_EXT;
    const void* p_next =                               NULL;
    uint32_t                                           data_size;
    const void*                                        p_data;
} vk_write_descriptor_set_inline_uniform_block_ext_t;

typedef struct vk_descriptor_pool_inline_uniform_block_create_info_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_INLINE_UNIFORM_BLOCK_CREATE_INFO_EXT;
    const void* p_next =                               NULL;
    uint32_t                                           max_inline_uniform_block_bindings;
} vk_descriptor_pool_inline_uniform_block_create_info_ext_t;

typedef struct vk_sample_location_ext_t {
    float                                              x;
    float                                              y;
} vk_sample_location_ext_t;

typedef struct vk_sample_locations_info_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_SAMPLE_LOCATIONS_INFO_EXT;
    const void* p_next =                               NULL;
    vk_sample_count_flag_bits_t                        sample_locations_per_pixel;
    vk_extent2d_t                                     sample_location_grid_size;
    uint32_t                                           sample_locations_count;
    const vk_sample_location_ext_t*                    p_sample_locations;
} vk_sample_locations_info_ext_t;

typedef struct vk_attachment_sample_locations_ext_t {
    uint32_t                                           attachment_index;
    vk_sample_locations_info_ext_t                     sample_locations_info;
} vk_attachment_sample_locations_ext_t;

typedef struct vk_subpass_sample_locations_ext_t {
    uint32_t                                           subpass_index;
    vk_sample_locations_info_ext_t                     sample_locations_info;
} vk_subpass_sample_locations_ext_t;

typedef struct vk_render_pass_sample_locations_begin_info_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_RENDER_PASS_SAMPLE_LOCATIONS_BEGIN_INFO_EXT;
    const void* p_next =                               NULL;
    uint32_t                                           attachment_initial_sample_locations_count;
    const vk_attachment_sample_locations_ext_t*        p_attachment_initial_sample_locations;
    uint32_t                                           post_subpass_sample_locations_count;
    const vk_subpass_sample_locations_ext_t*           p_post_subpass_sample_locations;
} vk_render_pass_sample_locations_begin_info_ext_t;

typedef struct vk_pipeline_sample_locations_state_create_info_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PIPELINE_SAMPLE_LOCATIONS_STATE_CREATE_INFO_EXT;
    const void* p_next =                               NULL;
    vk_bool32_t                                        sample_locations_enable;
    vk_sample_locations_info_ext_t                     sample_locations_info;
} vk_pipeline_sample_locations_state_create_info_ext_t;

typedef struct vk_physical_device_sample_locations_properties_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLE_LOCATIONS_PROPERTIES_EXT;
    void* p_next =                                     NULL;
    vk_sample_count_flags_t                            sample_location_sample_counts;
    vk_extent2d_t                                     max_sample_location_grid_size;
    float                                              sample_location_coordinate_range[2];
    uint32_t                                           sample_location_sub_pixel_bits;
    vk_bool32_t                                        variable_sample_locations;
} vk_physical_device_sample_locations_properties_ext_t;

typedef struct vk_multisample_properties_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_MULTISAMPLE_PROPERTIES_EXT;
    void* p_next =                                     NULL;
    vk_extent2d_t                                     max_sample_location_grid_size;
} vk_multisample_properties_ext_t;
#define pfn_vk_cmd_set_sample_locations_ext_fn_t                          PFN_vkCmdSetSampleLocationsEXT
#define pfn_vk_get_physical_device_multisample_properties_ext_fn_t        PFN_vkGetPhysicalDeviceMultisamplePropertiesEXT

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_set_sample_locations_ext(
    vk_command_buffer_t                                commandBuffer,
    const vk_sample_locations_info_ext_t*              pSampleLocationsInfo)
{
    return vkCmdSetSampleLocationsEXT (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    const VkSampleLocationsInfoEXT*             *)&pSampleLocationsInfo);
}

inline VKAPI_ATTR void VKAPI_CALL vk_get_physical_device_multisample_properties_ext(
    vk_physical_device_t                               physicalDevice,
    vk_sample_count_flag_bits_t                        samples,
    vk_multisample_properties_ext_t*                   pMultisampleProperties)
{
    return vkGetPhysicalDeviceMultisamplePropertiesEXT (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    VkSampleCountFlagBits                       *)&samples,
            *(    VkMultisamplePropertiesEXT*                 *)&pMultisampleProperties);
}
#define vk_blend_overlap_ext_t                                            VkBlendOverlapEXT

typedef struct vk_physical_device_blend_operation_advanced_features_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BLEND_OPERATION_ADVANCED_FEATURES_EXT;
    void* p_next =                                     NULL;
    vk_bool32_t                                        advanced_blend_coherent_operations;
} vk_physical_device_blend_operation_advanced_features_ext_t;

typedef struct vk_physical_device_blend_operation_advanced_properties_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BLEND_OPERATION_ADVANCED_PROPERTIES_EXT;
    void* p_next =                                     NULL;
    uint32_t                                           advanced_blend_max_color_attachments;
    vk_bool32_t                                        advanced_blend_independent_blend;
    vk_bool32_t                                        advanced_blend_non_premultiplied_src_color;
    vk_bool32_t                                        advanced_blend_non_premultiplied_dst_color;
    vk_bool32_t                                        advanced_blend_correlated_overlap;
    vk_bool32_t                                        advanced_blend_all_operations;
} vk_physical_device_blend_operation_advanced_properties_ext_t;

typedef struct vk_pipeline_color_blend_advanced_state_create_info_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_ADVANCED_STATE_CREATE_INFO_EXT;
    const void* p_next =                               NULL;
    vk_bool32_t                                        src_premultiplied;
    vk_bool32_t                                        dst_premultiplied;
    vk_blend_overlap_ext_t                             blend_overlap;
} vk_pipeline_color_blend_advanced_state_create_info_ext_t;
#define vk_pipeline_coverage_to_color_state_create_flags_nv_t             VkPipelineCoverageToColorStateCreateFlagsNV 

typedef struct vk_pipeline_coverage_to_color_state_create_info_nv_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PIPELINE_COVERAGE_TO_COLOR_STATE_CREATE_INFO_NV;
    const void* p_next =                               NULL;
    vk_pipeline_coverage_to_color_state_create_flags_nv_t flags;
    vk_bool32_t                                        coverage_to_color_enable;
    uint32_t                                           coverage_to_color_location;
} vk_pipeline_coverage_to_color_state_create_info_nv_t;
#define vk_coverage_modulation_mode_nv_t                                  VkCoverageModulationModeNV
#define vk_pipeline_coverage_modulation_state_create_flags_nv_t           VkPipelineCoverageModulationStateCreateFlagsNV 

typedef struct vk_pipeline_coverage_modulation_state_create_info_nv_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PIPELINE_COVERAGE_MODULATION_STATE_CREATE_INFO_NV;
    const void* p_next =                               NULL;
    vk_pipeline_coverage_modulation_state_create_flags_nv_t flags;
    vk_coverage_modulation_mode_nv_t                   coverage_modulation_mode;
    vk_bool32_t                                        coverage_modulation_table_enable;
    uint32_t                                           coverage_modulation_table_count;
    const float*                                       p_coverage_modulation_table;
} vk_pipeline_coverage_modulation_state_create_info_nv_t;

typedef struct vk_physical_device_shader_sm_builtins_properties_nv_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SM_BUILTINS_PROPERTIES_NV;
    void* p_next =                                     NULL;
    uint32_t                                           shader_sm_count;
    uint32_t                                           shader_warps_per_sm;
} vk_physical_device_shader_sm_builtins_properties_nv_t;

typedef struct vk_physical_device_shader_sm_builtins_features_nv_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SM_BUILTINS_FEATURES_NV;
    void* p_next =                                     NULL;
    vk_bool32_t                                        shader_sm_builtins;
} vk_physical_device_shader_sm_builtins_features_nv_t;

typedef struct vk_drm_format_modifier_properties_ext_t {
    uint64_t                                           drm_format_modifier;
    uint32_t                                           drm_format_modifier_plane_count;
    vk_format_feature_flags_t                          drm_format_modifier_tiling_features;
} vk_drm_format_modifier_properties_ext_t;

typedef struct vk_drm_format_modifier_properties_list_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_DRM_FORMAT_MODIFIER_PROPERTIES_LIST_EXT;
    void* p_next =                                     NULL;
    uint32_t                                           drm_format_modifier_count;
    vk_drm_format_modifier_properties_ext_t*           p_drm_format_modifier_properties;
} vk_drm_format_modifier_properties_list_ext_t;

typedef struct vk_physical_device_image_drm_format_modifier_info_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_DRM_FORMAT_MODIFIER_INFO_EXT;
    const void* p_next =                               NULL;
    uint64_t                                           drm_format_modifier;
    vk_sharing_mode_t                                  sharing_mode;
    uint32_t                                           queue_family_index_count;
    const uint32_t*                                    p_queue_family_indices;
} vk_physical_device_image_drm_format_modifier_info_ext_t;

typedef struct vk_image_drm_format_modifier_list_create_info_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_LIST_CREATE_INFO_EXT;
    const void* p_next =                               NULL;
    uint32_t                                           drm_format_modifier_count;
    const uint64_t*                                    p_drm_format_modifiers;
} vk_image_drm_format_modifier_list_create_info_ext_t;

typedef struct vk_image_drm_format_modifier_explicit_create_info_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_EXPLICIT_CREATE_INFO_EXT;
    const void* p_next =                               NULL;
    uint64_t                                           drm_format_modifier;
    uint32_t                                           drm_format_modifier_plane_count;
    const vk_subresource_layout_t*                     p_plane_layouts;
} vk_image_drm_format_modifier_explicit_create_info_ext_t;

typedef struct vk_image_drm_format_modifier_properties_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_PROPERTIES_EXT;
    void* p_next =                                     NULL;
    uint64_t                                           drm_format_modifier;
} vk_image_drm_format_modifier_properties_ext_t;
#define pfn_vk_get_image_drm_format_modifier_properties_ext_fn_t          PFN_vkGetImageDrmFormatModifierPropertiesEXT

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_image_drm_format_modifier_properties_ext(
    vk_device_t                                        device,
    vk_image_t                                         image,
    vk_image_drm_format_modifier_properties_ext_t*     pProperties)
{
    return vkGetImageDrmFormatModifierPropertiesEXT (
            *(    VkDevice                                    *)&device,
            *(    VkImage                                     *)&image,
            *(    VkImageDrmFormatModifierPropertiesEXT*      *)&pProperties);
}
#define vk_validation_cache_ext_t                                         VkValidationCacheEXT 
#define vk_validation_cache_header_version_ext_t                          VkValidationCacheHeaderVersionEXT
#define vk_validation_cache_create_flags_ext_t                            VkValidationCacheCreateFlagsEXT 

typedef struct vk_validation_cache_create_info_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_VALIDATION_CACHE_CREATE_INFO_EXT;
    const void* p_next =                               NULL;
    vk_validation_cache_create_flags_ext_t             flags;
    size_t                                             initial_data_size;
    const void*                                        p_initial_data;
} vk_validation_cache_create_info_ext_t;

typedef struct vk_shader_module_validation_cache_create_info_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_SHADER_MODULE_VALIDATION_CACHE_CREATE_INFO_EXT;
    const void* p_next =                               NULL;
    vk_validation_cache_ext_t                          validation_cache;
} vk_shader_module_validation_cache_create_info_ext_t;
#define pfn_vk_create_validation_cache_ext_fn_t                           PFN_vkCreateValidationCacheEXT
#define pfn_vk_destroy_validation_cache_ext_fn_t                          PFN_vkDestroyValidationCacheEXT
#define pfn_vk_merge_validation_caches_ext_fn_t                           PFN_vkMergeValidationCachesEXT
#define pfn_vk_get_validation_cache_data_ext_fn_t                         PFN_vkGetValidationCacheDataEXT

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_create_validation_cache_ext(
    vk_device_t                                        device,
    const vk_validation_cache_create_info_ext_t*       pCreateInfo,
    const vk_allocation_callbacks_t*                   pAllocator,
    vk_validation_cache_ext_t*                         pValidationCache)
{
    return vkCreateValidationCacheEXT (
            *(    VkDevice                                    *)&device,
            *(    const VkValidationCacheCreateInfoEXT*       *)&pCreateInfo,
            *(    const VkAllocationCallbacks*                *)&pAllocator,
            *(    VkValidationCacheEXT*                       *)&pValidationCache);
}

inline VKAPI_ATTR void VKAPI_CALL vk_destroy_validation_cache_ext(
    vk_device_t                                        device,
    vk_validation_cache_ext_t                          validationCache,
    const vk_allocation_callbacks_t*                   pAllocator)
{
    return vkDestroyValidationCacheEXT (
            *(    VkDevice                                    *)&device,
            *(    VkValidationCacheEXT                        *)&validationCache,
            *(    const VkAllocationCallbacks*                *)&pAllocator);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_merge_validation_caches_ext(
    vk_device_t                                        device,
    vk_validation_cache_ext_t                          dstCache,
    uint32_t                                           srcCacheCount,
    const vk_validation_cache_ext_t*                   pSrcCaches)
{
    return vkMergeValidationCachesEXT (
            *(    VkDevice                                    *)&device,
            *(    VkValidationCacheEXT                        *)&dstCache,
            *(    uint32_t                                    *)&srcCacheCount,
            *(    const VkValidationCacheEXT*                 *)&pSrcCaches);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_validation_cache_data_ext(
    vk_device_t                                        device,
    vk_validation_cache_ext_t                          validationCache,
    size_t*                                            pDataSize,
    void*                                              pData)
{
    return vkGetValidationCacheDataEXT (
            *(    VkDevice                                    *)&device,
            *(    VkValidationCacheEXT                        *)&validationCache,
            *(    size_t*                                     *)&pDataSize,
            *(    void*                                       *)&pData);
}
#define vk_descriptor_binding_flag_bits_ext_t                             VkDescriptorBindingFlagBitsEXT 
#define vk_descriptor_binding_flags_ext_t                                 VkDescriptorBindingFlagsEXT 
#define vk_descriptor_set_layout_binding_flags_create_info_ext_t          VkDescriptorSetLayoutBindingFlagsCreateInfoEXT 
#define vk_physical_device_descriptor_indexing_features_ext_t             VkPhysicalDeviceDescriptorIndexingFeaturesEXT 
#define vk_physical_device_descriptor_indexing_properties_ext_t           VkPhysicalDeviceDescriptorIndexingPropertiesEXT 
#define vk_descriptor_set_variable_descriptor_count_allocate_info_ext_t   VkDescriptorSetVariableDescriptorCountAllocateInfoEXT 
#define vk_descriptor_set_variable_descriptor_count_layout_support_ext_t  VkDescriptorSetVariableDescriptorCountLayoutSupportEXT 
#define vk_shading_rate_palette_entry_nv_t                                VkShadingRatePaletteEntryNV
#define vk_coarse_sample_order_type_nv_t                                  VkCoarseSampleOrderTypeNV

typedef struct vk_shading_rate_palette_nv_t {
    uint32_t                                           shading_rate_palette_entry_count;
    const vk_shading_rate_palette_entry_nv_t*          p_shading_rate_palette_entries;
} vk_shading_rate_palette_nv_t;

typedef struct vk_pipeline_viewport_shading_rate_image_state_create_info_nv_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_SHADING_RATE_IMAGE_STATE_CREATE_INFO_NV;
    const void* p_next =                               NULL;
    vk_bool32_t                                        shading_rate_image_enable;
    uint32_t                                           viewport_count;
    const vk_shading_rate_palette_nv_t*                p_shading_rate_palettes;
} vk_pipeline_viewport_shading_rate_image_state_create_info_nv_t;

typedef struct vk_physical_device_shading_rate_image_features_nv_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADING_RATE_IMAGE_FEATURES_NV;
    void* p_next =                                     NULL;
    vk_bool32_t                                        shading_rate_image;
    vk_bool32_t                                        shading_rate_coarse_sample_order;
} vk_physical_device_shading_rate_image_features_nv_t;

typedef struct vk_physical_device_shading_rate_image_properties_nv_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADING_RATE_IMAGE_PROPERTIES_NV;
    void* p_next =                                     NULL;
    vk_extent2d_t                                     shading_rate_texel_size;
    uint32_t                                           shading_rate_palette_size;
    uint32_t                                           shading_rate_max_coarse_samples;
} vk_physical_device_shading_rate_image_properties_nv_t;

typedef struct vk_coarse_sample_location_nv_t {
    uint32_t                                           pixel_x;
    uint32_t                                           pixel_y;
    uint32_t                                           sample;
} vk_coarse_sample_location_nv_t;

typedef struct vk_coarse_sample_order_custom_nv_t {
    vk_shading_rate_palette_entry_nv_t                 shading_rate;
    uint32_t                                           sample_count;
    uint32_t                                           sample_location_count;
    const vk_coarse_sample_location_nv_t*              p_sample_locations;
} vk_coarse_sample_order_custom_nv_t;

typedef struct vk_pipeline_viewport_coarse_sample_order_state_create_info_nv_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_COARSE_SAMPLE_ORDER_STATE_CREATE_INFO_NV;
    const void* p_next =                               NULL;
    vk_coarse_sample_order_type_nv_t                   sample_order_type;
    uint32_t                                           custom_sample_order_count;
    const vk_coarse_sample_order_custom_nv_t*          p_custom_sample_orders;
} vk_pipeline_viewport_coarse_sample_order_state_create_info_nv_t;
#define pfn_vk_cmd_bind_shading_rate_image_nv_fn_t                        PFN_vkCmdBindShadingRateImageNV
#define pfn_vk_cmd_set_viewport_shading_rate_palette_nv_fn_t              PFN_vkCmdSetViewportShadingRatePaletteNV
#define pfn_vk_cmd_set_coarse_sample_order_nv_fn_t                        PFN_vkCmdSetCoarseSampleOrderNV

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_bind_shading_rate_image_nv(
    vk_command_buffer_t                                commandBuffer,
    vk_image_view_t                                    imageView,
    vk_image_layout_t                                  imageLayout)
{
    return vkCmdBindShadingRateImageNV (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    VkImageView                                 *)&imageView,
            *(    VkImageLayout                               *)&imageLayout);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_set_viewport_shading_rate_palette_nv(
    vk_command_buffer_t                                commandBuffer,
    uint32_t                                           firstViewport,
    uint32_t                                           viewportCount,
    const vk_shading_rate_palette_nv_t*                pShadingRatePalettes)
{
    return vkCmdSetViewportShadingRatePaletteNV (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    uint32_t                                    *)&firstViewport,
            *(    uint32_t                                    *)&viewportCount,
            *(    const VkShadingRatePaletteNV*               *)&pShadingRatePalettes);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_set_coarse_sample_order_nv(
    vk_command_buffer_t                                commandBuffer,
    vk_coarse_sample_order_type_nv_t                   sampleOrderType,
    uint32_t                                           customSampleOrderCount,
    const vk_coarse_sample_order_custom_nv_t*          pCustomSampleOrders)
{
    return vkCmdSetCoarseSampleOrderNV (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    VkCoarseSampleOrderTypeNV                   *)&sampleOrderType,
            *(    uint32_t                                    *)&customSampleOrderCount,
            *(    const VkCoarseSampleOrderCustomNV*          *)&pCustomSampleOrders);
}
#define vk_acceleration_structure_nv_t                                    VkAccelerationStructureNV 
#define vk_acceleration_structure_type_nv_t                               VkAccelerationStructureTypeNV
#define vk_ray_tracing_shader_group_type_nv_t                             VkRayTracingShaderGroupTypeNV
#define vk_geometry_type_nv_t                                             VkGeometryTypeNV
#define vk_copy_acceleration_structure_mode_nv_t                          VkCopyAccelerationStructureModeNV
#define vk_acceleration_structure_memory_requirements_type_nv_t           VkAccelerationStructureMemoryRequirementsTypeNV
#define vk_geometry_flag_bits_nv_t                                        VkGeometryFlagBitsNV
#define vk_geometry_flags_nv_t                                            VkGeometryFlagsNV 
#define vk_geometry_instance_flag_bits_nv_t                               VkGeometryInstanceFlagBitsNV
#define vk_geometry_instance_flags_nv_t                                   VkGeometryInstanceFlagsNV 
#define vk_build_acceleration_structure_flag_bits_nv_t                    VkBuildAccelerationStructureFlagBitsNV
#define vk_build_acceleration_structure_flags_nv_t                        VkBuildAccelerationStructureFlagsNV 

typedef struct vk_ray_tracing_shader_group_create_info_nv_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_NV;
    const void* p_next =                               NULL;
    vk_ray_tracing_shader_group_type_nv_t              type;
    uint32_t                                           general_shader;
    uint32_t                                           closest_hit_shader;
    uint32_t                                           any_hit_shader;
    uint32_t                                           intersection_shader;
} vk_ray_tracing_shader_group_create_info_nv_t;

typedef struct vk_ray_tracing_pipeline_create_info_nv_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_NV;
    const void* p_next =                               NULL;
    vk_pipeline_create_flags_t                         flags;
    uint32_t                                           stage_count;
    const vk_pipeline_shader_stage_create_info_t*      p_stages;
    uint32_t                                           group_count;
    const vk_ray_tracing_shader_group_create_info_nv_t* p_groups;
    uint32_t                                           max_recursion_depth;
    vk_pipeline_layout_t                               layout;
    vk_pipeline_t                                      base_pipeline_handle;
    int32_t                                            base_pipeline_index;
} vk_ray_tracing_pipeline_create_info_nv_t;

typedef struct vk_geometry_triangles_nv_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_GEOMETRY_TRIANGLES_NV;
    const void* p_next =                               NULL;
    vk_buffer_t                                        vertex_data;
    vk_device_size_t                                   vertex_offset;
    uint32_t                                           vertex_count;
    vk_device_size_t                                   vertex_stride;
    vk_format_t                                        vertex_format;
    vk_buffer_t                                        index_data;
    vk_device_size_t                                   index_offset;
    uint32_t                                           index_count;
    vk_index_type_t                                    index_type;
    vk_buffer_t                                        transform_data;
    vk_device_size_t                                   transform_offset;
} vk_geometry_triangles_nv_t;

typedef struct vk_geometry_aabbnv_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_GEOMETRY_AABB_NV;
    const void* p_next =                               NULL;
    vk_buffer_t                                        aabb_data;
    uint32_t                                           num_aab_bs;
    uint32_t                                           stride;
    vk_device_size_t                                   offset;
} vk_geometry_aabbnv_t;

typedef struct vk_geometry_data_nv_t {
    vk_geometry_triangles_nv_t                         triangles;
    vk_geometry_aabbnv_t                               aabbs;
} vk_geometry_data_nv_t;

typedef struct vk_geometry_nv_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_GEOMETRY_NV;
    const void* p_next =                               NULL;
    vk_geometry_type_nv_t                              geometry_type;
    vk_geometry_data_nv_t                              geometry;
    vk_geometry_flags_nv_t                             flags;
} vk_geometry_nv_t;

typedef struct vk_acceleration_structure_info_nv_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
    const void* p_next =                               NULL;
    vk_acceleration_structure_type_nv_t                type;
    vk_build_acceleration_structure_flags_nv_t         flags;
    uint32_t                                           instance_count;
    uint32_t                                           geometry_count;
    const vk_geometry_nv_t*                            p_geometries;
} vk_acceleration_structure_info_nv_t;

typedef struct vk_acceleration_structure_create_info_nv_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
    const void* p_next =                               NULL;
    vk_device_size_t                                   compacted_size;
    vk_acceleration_structure_info_nv_t                info;
} vk_acceleration_structure_create_info_nv_t;

typedef struct vk_bind_acceleration_structure_memory_info_nv_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
    const void* p_next =                               NULL;
    vk_acceleration_structure_nv_t                     acceleration_structure;
    vk_device_memory_t                                 memory;
    vk_device_size_t                                   memory_offset;
    uint32_t                                           device_index_count;
    const uint32_t*                                    p_device_indices;
} vk_bind_acceleration_structure_memory_info_nv_t;

typedef struct vk_write_descriptor_set_acceleration_structure_nv_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_NV;
    const void* p_next =                               NULL;
    uint32_t                                           acceleration_structure_count;
    const vk_acceleration_structure_nv_t*              p_acceleration_structures;
} vk_write_descriptor_set_acceleration_structure_nv_t;

typedef struct vk_acceleration_structure_memory_requirements_info_nv_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
    const void* p_next =                               NULL;
    vk_acceleration_structure_memory_requirements_type_nv_t type;
    vk_acceleration_structure_nv_t                     acceleration_structure;
} vk_acceleration_structure_memory_requirements_info_nv_t;

typedef struct vk_physical_device_ray_tracing_properties_nv_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV;
    void* p_next =                                     NULL;
    uint32_t                                           shader_group_handle_size;
    uint32_t                                           max_recursion_depth;
    uint32_t                                           max_shader_group_stride;
    uint32_t                                           shader_group_base_alignment;
    uint64_t                                           max_geometry_count;
    uint64_t                                           max_instance_count;
    uint64_t                                           max_triangle_count;
    uint32_t                                           max_descriptor_set_acceleration_structures;
} vk_physical_device_ray_tracing_properties_nv_t;
#define pfn_vk_create_acceleration_structure_nv_fn_t                      PFN_vkCreateAccelerationStructureNV
#define pfn_vk_destroy_acceleration_structure_nv_fn_t                     PFN_vkDestroyAccelerationStructureNV
#define pfn_vk_get_acceleration_structure_memory_requirements_nv_fn_t     PFN_vkGetAccelerationStructureMemoryRequirementsNV
#define pfn_vk_bind_acceleration_structure_memory_nv_fn_t                 PFN_vkBindAccelerationStructureMemoryNV
#define pfn_vk_cmd_build_acceleration_structure_nv_fn_t                   PFN_vkCmdBuildAccelerationStructureNV
#define pfn_vk_cmd_copy_acceleration_structure_nv_fn_t                    PFN_vkCmdCopyAccelerationStructureNV
#define pfn_vk_cmd_trace_rays_nv_fn_t                                     PFN_vkCmdTraceRaysNV
#define pfn_vk_create_ray_tracing_pipelines_nv_fn_t                       PFN_vkCreateRayTracingPipelinesNV
#define pfn_vk_get_ray_tracing_shader_group_handles_nv_fn_t               PFN_vkGetRayTracingShaderGroupHandlesNV
#define pfn_vk_get_acceleration_structure_handle_nv_fn_t                  PFN_vkGetAccelerationStructureHandleNV
#define pfn_vk_cmd_write_acceleration_structures_properties_nv_fn_t       PFN_vkCmdWriteAccelerationStructuresPropertiesNV
#define pfn_vk_compile_deferred_nv_fn_t                                   PFN_vkCompileDeferredNV

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_create_acceleration_structure_nv(
    vk_device_t                                        device,
    const vk_acceleration_structure_create_info_nv_t*  pCreateInfo,
    const vk_allocation_callbacks_t*                   pAllocator,
    vk_acceleration_structure_nv_t*                    pAccelerationStructure)
{
    return vkCreateAccelerationStructureNV (
            *(    VkDevice                                    *)&device,
            *(    const VkAccelerationStructureCreateInfoNV*  *)&pCreateInfo,
            *(    const VkAllocationCallbacks*                *)&pAllocator,
            *(    VkAccelerationStructureNV*                  *)&pAccelerationStructure);
}

inline VKAPI_ATTR void VKAPI_CALL vk_destroy_acceleration_structure_nv(
    vk_device_t                                        device,
    vk_acceleration_structure_nv_t                     accelerationStructure,
    const vk_allocation_callbacks_t*                   pAllocator)
{
    return vkDestroyAccelerationStructureNV (
            *(    VkDevice                                    *)&device,
            *(    VkAccelerationStructureNV                   *)&accelerationStructure,
            *(    const VkAllocationCallbacks*                *)&pAllocator);
}

inline VKAPI_ATTR void VKAPI_CALL vk_get_acceleration_structure_memory_requirements_nv(
    vk_device_t                                        device,
    const vk_acceleration_structure_memory_requirements_info_nv_t* pInfo,
    vk_memory_requirements2_khr_t*                     pMemoryRequirements)
{
    return vkGetAccelerationStructureMemoryRequirementsNV (
            *(    VkDevice                                    *)&device,
            *(    const VkAccelerationStructureMemoryRequirementsInfoNV* *)&pInfo,
            *(    VkMemoryRequirements2KHR*                   *)&pMemoryRequirements);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_bind_acceleration_structure_memory_nv(
    vk_device_t                                        device,
    uint32_t                                           bindInfoCount,
    const vk_bind_acceleration_structure_memory_info_nv_t* pBindInfos)
{
    return vkBindAccelerationStructureMemoryNV (
            *(    VkDevice                                    *)&device,
            *(    uint32_t                                    *)&bindInfoCount,
            *(    const VkBindAccelerationStructureMemoryInfoNV* *)&pBindInfos);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_build_acceleration_structure_nv(
    vk_command_buffer_t                                commandBuffer,
    const vk_acceleration_structure_info_nv_t*         pInfo,
    vk_buffer_t                                        instanceData,
    vk_device_size_t                                   instanceOffset,
    vk_bool32_t                                        update,
    vk_acceleration_structure_nv_t                     dst,
    vk_acceleration_structure_nv_t                     src,
    vk_buffer_t                                        scratch,
    vk_device_size_t                                   scratchOffset)
{
    return vkCmdBuildAccelerationStructureNV (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    const VkAccelerationStructureInfoNV*        *)&pInfo,
            *(    VkBuffer                                    *)&instanceData,
            *(    VkDeviceSize                                *)&instanceOffset,
            *(    VkBool32                                    *)&update,
            *(    VkAccelerationStructureNV                   *)&dst,
            *(    VkAccelerationStructureNV                   *)&src,
            *(    VkBuffer                                    *)&scratch,
            *(    VkDeviceSize                                *)&scratchOffset);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_copy_acceleration_structure_nv(
    vk_command_buffer_t                                commandBuffer,
    vk_acceleration_structure_nv_t                     dst,
    vk_acceleration_structure_nv_t                     src,
    vk_copy_acceleration_structure_mode_nv_t           mode)
{
    return vkCmdCopyAccelerationStructureNV (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    VkAccelerationStructureNV                   *)&dst,
            *(    VkAccelerationStructureNV                   *)&src,
            *(    VkCopyAccelerationStructureModeNV           *)&mode);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_trace_rays_nv(
    vk_command_buffer_t                                commandBuffer,
    vk_buffer_t                                        raygenShaderBindingTableBuffer,
    vk_device_size_t                                   raygenShaderBindingOffset,
    vk_buffer_t                                        missShaderBindingTableBuffer,
    vk_device_size_t                                   missShaderBindingOffset,
    vk_device_size_t                                   missShaderBindingStride,
    vk_buffer_t                                        hitShaderBindingTableBuffer,
    vk_device_size_t                                   hitShaderBindingOffset,
    vk_device_size_t                                   hitShaderBindingStride,
    vk_buffer_t                                        callableShaderBindingTableBuffer,
    vk_device_size_t                                   callableShaderBindingOffset,
    vk_device_size_t                                   callableShaderBindingStride,
    uint32_t                                           width,
    uint32_t                                           height,
    uint32_t                                           depth)
{
    return vkCmdTraceRaysNV (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    VkBuffer                                    *)&raygenShaderBindingTableBuffer,
            *(    VkDeviceSize                                *)&raygenShaderBindingOffset,
            *(    VkBuffer                                    *)&missShaderBindingTableBuffer,
            *(    VkDeviceSize                                *)&missShaderBindingOffset,
            *(    VkDeviceSize                                *)&missShaderBindingStride,
            *(    VkBuffer                                    *)&hitShaderBindingTableBuffer,
            *(    VkDeviceSize                                *)&hitShaderBindingOffset,
            *(    VkDeviceSize                                *)&hitShaderBindingStride,
            *(    VkBuffer                                    *)&callableShaderBindingTableBuffer,
            *(    VkDeviceSize                                *)&callableShaderBindingOffset,
            *(    VkDeviceSize                                *)&callableShaderBindingStride,
            *(    uint32_t                                    *)&width,
            *(    uint32_t                                    *)&height,
            *(    uint32_t                                    *)&depth);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_create_ray_tracing_pipelines_nv(
    vk_device_t                                        device,
    vk_pipeline_cache_t                                pipelineCache,
    uint32_t                                           createInfoCount,
    const vk_ray_tracing_pipeline_create_info_nv_t*    pCreateInfos,
    const vk_allocation_callbacks_t*                   pAllocator,
    vk_pipeline_t*                                     pPipelines)
{
    return vkCreateRayTracingPipelinesNV (
            *(    VkDevice                                    *)&device,
            *(    VkPipelineCache                             *)&pipelineCache,
            *(    uint32_t                                    *)&createInfoCount,
            *(    const VkRayTracingPipelineCreateInfoNV*     *)&pCreateInfos,
            *(    const VkAllocationCallbacks*                *)&pAllocator,
            *(    VkPipeline*                                 *)&pPipelines);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_ray_tracing_shader_group_handles_nv(
    vk_device_t                                        device,
    vk_pipeline_t                                      pipeline,
    uint32_t                                           firstGroup,
    uint32_t                                           groupCount,
    size_t                                             dataSize,
    void*                                              pData)
{
    return vkGetRayTracingShaderGroupHandlesNV (
            *(    VkDevice                                    *)&device,
            *(    VkPipeline                                  *)&pipeline,
            *(    uint32_t                                    *)&firstGroup,
            *(    uint32_t                                    *)&groupCount,
            *(    size_t                                      *)&dataSize,
            *(    void*                                       *)&pData);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_acceleration_structure_handle_nv(
    vk_device_t                                        device,
    vk_acceleration_structure_nv_t                     accelerationStructure,
    size_t                                             dataSize,
    void*                                              pData)
{
    return vkGetAccelerationStructureHandleNV (
            *(    VkDevice                                    *)&device,
            *(    VkAccelerationStructureNV                   *)&accelerationStructure,
            *(    size_t                                      *)&dataSize,
            *(    void*                                       *)&pData);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_write_acceleration_structures_properties_nv(
    vk_command_buffer_t                                commandBuffer,
    uint32_t                                           accelerationStructureCount,
    const vk_acceleration_structure_nv_t*              pAccelerationStructures,
    vk_query_type_t                                    queryType,
    vk_query_pool_t                                    queryPool,
    uint32_t                                           firstQuery)
{
    return vkCmdWriteAccelerationStructuresPropertiesNV (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    uint32_t                                    *)&accelerationStructureCount,
            *(    const VkAccelerationStructureNV*            *)&pAccelerationStructures,
            *(    VkQueryType                                 *)&queryType,
            *(    VkQueryPool                                 *)&queryPool,
            *(    uint32_t                                    *)&firstQuery);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_compile_deferred_nv(
    vk_device_t                                        device,
    vk_pipeline_t                                      pipeline,
    uint32_t                                           shader)
{
    return vkCompileDeferredNV (
            *(    VkDevice                                    *)&device,
            *(    VkPipeline                                  *)&pipeline,
            *(    uint32_t                                    *)&shader);
}

typedef struct vk_physical_device_representative_fragment_test_features_nv_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_REPRESENTATIVE_FRAGMENT_TEST_FEATURES_NV;
    void* p_next =                                     NULL;
    vk_bool32_t                                        representative_fragment_test;
} vk_physical_device_representative_fragment_test_features_nv_t;

typedef struct vk_pipeline_representative_fragment_test_state_create_info_nv_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PIPELINE_REPRESENTATIVE_FRAGMENT_TEST_STATE_CREATE_INFO_NV;
    const void* p_next =                               NULL;
    vk_bool32_t                                        representative_fragment_test_enable;
} vk_pipeline_representative_fragment_test_state_create_info_nv_t;

typedef struct vk_physical_device_image_view_image_format_info_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_VIEW_IMAGE_FORMAT_INFO_EXT;
    void* p_next =                                     NULL;
    vk_image_view_type_t                               image_view_type;
} vk_physical_device_image_view_image_format_info_ext_t;

typedef struct vk_filter_cubic_image_view_image_format_properties_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_FILTER_CUBIC_IMAGE_VIEW_IMAGE_FORMAT_PROPERTIES_EXT;
    void* p_next =                                     NULL;
    vk_bool32_t                                        filter_cubic;
    vk_bool32_t                                        filter_cubic_minmax;
} vk_filter_cubic_image_view_image_format_properties_ext_t;
#define vk_queue_global_priority_ext_t                                    VkQueueGlobalPriorityEXT

typedef struct vk_device_queue_global_priority_create_info_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_DEVICE_QUEUE_GLOBAL_PRIORITY_CREATE_INFO_EXT;
    const void* p_next =                               NULL;
    vk_queue_global_priority_ext_t                     global_priority;
} vk_device_queue_global_priority_create_info_ext_t;

typedef struct vk_import_memory_host_pointer_info_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_IMPORT_MEMORY_HOST_POINTER_INFO_EXT;
    const void* p_next =                               NULL;
    vk_external_memory_handle_type_flag_bits_t         handle_type;
    void*                                              p_host_pointer;
} vk_import_memory_host_pointer_info_ext_t;

typedef struct vk_memory_host_pointer_properties_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_MEMORY_HOST_POINTER_PROPERTIES_EXT;
    void* p_next =                                     NULL;
    uint32_t                                           memory_type_bits;
} vk_memory_host_pointer_properties_ext_t;

typedef struct vk_physical_device_external_memory_host_properties_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_MEMORY_HOST_PROPERTIES_EXT;
    void* p_next =                                     NULL;
    vk_device_size_t                                   min_imported_host_pointer_alignment;
} vk_physical_device_external_memory_host_properties_ext_t;
#define pfn_vk_get_memory_host_pointer_properties_ext_fn_t                PFN_vkGetMemoryHostPointerPropertiesEXT

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_memory_host_pointer_properties_ext(
    vk_device_t                                        device,
    vk_external_memory_handle_type_flag_bits_t         handleType,
    const void*                                        pHostPointer,
    vk_memory_host_pointer_properties_ext_t*           pMemoryHostPointerProperties)
{
    return vkGetMemoryHostPointerPropertiesEXT (
            *(    VkDevice                                    *)&device,
            *(    VkExternalMemoryHandleTypeFlagBits          *)&handleType,
            *(    const void*                                 *)&pHostPointer,
            *(    VkMemoryHostPointerPropertiesEXT*           *)&pMemoryHostPointerProperties);
}
#define pfn_vk_cmd_write_buffer_marker_amd_fn_t                           PFN_vkCmdWriteBufferMarkerAMD

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_write_buffer_marker_amd(
    vk_command_buffer_t                                commandBuffer,
    vk_pipeline_stage_flag_bits_t                      pipelineStage,
    vk_buffer_t                                        dstBuffer,
    vk_device_size_t                                   dstOffset,
    uint32_t                                           marker)
{
    return vkCmdWriteBufferMarkerAMD (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    VkPipelineStageFlagBits                     *)&pipelineStage,
            *(    VkBuffer                                    *)&dstBuffer,
            *(    VkDeviceSize                                *)&dstOffset,
            *(    uint32_t                                    *)&marker);
}
#define vk_pipeline_compiler_control_flag_bits_amd_t                      VkPipelineCompilerControlFlagBitsAMD
#define vk_pipeline_compiler_control_flags_amd_t                          VkPipelineCompilerControlFlagsAMD 

typedef struct vk_pipeline_compiler_control_create_info_amd_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PIPELINE_COMPILER_CONTROL_CREATE_INFO_AMD;
    const void* p_next =                               NULL;
    vk_pipeline_compiler_control_flags_amd_t           compiler_control_flags;
} vk_pipeline_compiler_control_create_info_amd_t;
#define vk_time_domain_ext_t                                              VkTimeDomainEXT

typedef struct vk_calibrated_timestamp_info_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_CALIBRATED_TIMESTAMP_INFO_EXT;
    const void* p_next =                               NULL;
    vk_time_domain_ext_t                               time_domain;
} vk_calibrated_timestamp_info_ext_t;
#define pfn_vk_get_physical_device_calibrateable_time_domains_ext_fn_t    PFN_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT
#define pfn_vk_get_calibrated_timestamps_ext_fn_t                         PFN_vkGetCalibratedTimestampsEXT

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_physical_device_calibrateable_time_domains_ext(
    vk_physical_device_t                               physicalDevice,
    uint32_t*                                          pTimeDomainCount,
    vk_time_domain_ext_t*                              pTimeDomains)
{
    return vkGetPhysicalDeviceCalibrateableTimeDomainsEXT (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    uint32_t*                                   *)&pTimeDomainCount,
            *(    VkTimeDomainEXT*                            *)&pTimeDomains);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_calibrated_timestamps_ext(
    vk_device_t                                        device,
    uint32_t                                           timestampCount,
    const vk_calibrated_timestamp_info_ext_t*          pTimestampInfos,
    uint64_t*                                          pTimestamps,
    uint64_t*                                          pMaxDeviation)
{
    return vkGetCalibratedTimestampsEXT (
            *(    VkDevice                                    *)&device,
            *(    uint32_t                                    *)&timestampCount,
            *(    const VkCalibratedTimestampInfoEXT*         *)&pTimestampInfos,
            *(    uint64_t*                                   *)&pTimestamps,
            *(    uint64_t*                                   *)&pMaxDeviation);
}

typedef struct vk_physical_device_shader_core_properties_amd_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CORE_PROPERTIES_AMD;
    void* p_next =                                     NULL;
    uint32_t                                           shader_engine_count;
    uint32_t                                           shader_arrays_per_engine_count;
    uint32_t                                           compute_units_per_shader_array;
    uint32_t                                           simd_per_compute_unit;
    uint32_t                                           wavefronts_per_simd;
    uint32_t                                           wavefront_size;
    uint32_t                                           sgprs_per_simd;
    uint32_t                                           min_sgpr_allocation;
    uint32_t                                           max_sgpr_allocation;
    uint32_t                                           sgpr_allocation_granularity;
    uint32_t                                           vgprs_per_simd;
    uint32_t                                           min_vgpr_allocation;
    uint32_t                                           max_vgpr_allocation;
    uint32_t                                           vgpr_allocation_granularity;
} vk_physical_device_shader_core_properties_amd_t;
#define vk_memory_overallocation_behavior_amd_t                           VkMemoryOverallocationBehaviorAMD

typedef struct vk_device_memory_overallocation_create_info_amd_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_DEVICE_MEMORY_OVERALLOCATION_CREATE_INFO_AMD;
    const void* p_next =                               NULL;
    vk_memory_overallocation_behavior_amd_t            overallocation_behavior;
} vk_device_memory_overallocation_create_info_amd_t;

typedef struct vk_physical_device_vertex_attribute_divisor_properties_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_PROPERTIES_EXT;
    void* p_next =                                     NULL;
    uint32_t                                           max_vertex_attrib_divisor;
} vk_physical_device_vertex_attribute_divisor_properties_ext_t;

typedef struct vk_vertex_input_binding_divisor_description_ext_t {
    uint32_t                                           binding;
    uint32_t                                           divisor;
} vk_vertex_input_binding_divisor_description_ext_t;

typedef struct vk_pipeline_vertex_input_divisor_state_create_info_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_DIVISOR_STATE_CREATE_INFO_EXT;
    const void* p_next =                               NULL;
    uint32_t                                           vertex_binding_divisor_count;
    const vk_vertex_input_binding_divisor_description_ext_t* p_vertex_binding_divisors;
} vk_pipeline_vertex_input_divisor_state_create_info_ext_t;

typedef struct vk_physical_device_vertex_attribute_divisor_features_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_FEATURES_EXT;
    void* p_next =                                     NULL;
    vk_bool32_t                                        vertex_attribute_instance_rate_divisor;
    vk_bool32_t                                        vertex_attribute_instance_rate_zero_divisor;
} vk_physical_device_vertex_attribute_divisor_features_ext_t;
#define vk_pipeline_creation_feedback_flag_bits_ext_t                     VkPipelineCreationFeedbackFlagBitsEXT
#define vk_pipeline_creation_feedback_flags_ext_t                         VkPipelineCreationFeedbackFlagsEXT 

typedef struct vk_pipeline_creation_feedback_ext_t {
    vk_pipeline_creation_feedback_flags_ext_t          flags;
    uint64_t                                           duration;
} vk_pipeline_creation_feedback_ext_t;

typedef struct vk_pipeline_creation_feedback_create_info_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PIPELINE_CREATION_FEEDBACK_CREATE_INFO_EXT;
    const void* p_next =                               NULL;
    vk_pipeline_creation_feedback_ext_t*               p_pipeline_creation_feedback;
    uint32_t                                           pipeline_stage_creation_feedback_count;
    vk_pipeline_creation_feedback_ext_t*               p_pipeline_stage_creation_feedbacks;
} vk_pipeline_creation_feedback_create_info_ext_t;

typedef struct vk_physical_device_compute_shader_derivatives_features_nv_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COMPUTE_SHADER_DERIVATIVES_FEATURES_NV;
    void* p_next =                                     NULL;
    vk_bool32_t                                        compute_derivative_group_quads;
    vk_bool32_t                                        compute_derivative_group_linear;
} vk_physical_device_compute_shader_derivatives_features_nv_t;

typedef struct vk_physical_device_mesh_shader_features_nv_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV;
    void* p_next =                                     NULL;
    vk_bool32_t                                        task_shader;
    vk_bool32_t                                        mesh_shader;
} vk_physical_device_mesh_shader_features_nv_t;

typedef struct vk_physical_device_mesh_shader_properties_nv_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_NV;
    void* p_next =                                     NULL;
    uint32_t                                           max_draw_mesh_tasks_count;
    uint32_t                                           max_task_work_group_invocations;
    uint32_t                                           max_task_work_group_size[3];
    uint32_t                                           max_task_total_memory_size;
    uint32_t                                           max_task_output_count;
    uint32_t                                           max_mesh_work_group_invocations;
    uint32_t                                           max_mesh_work_group_size[3];
    uint32_t                                           max_mesh_total_memory_size;
    uint32_t                                           max_mesh_output_vertices;
    uint32_t                                           max_mesh_output_primitives;
    uint32_t                                           max_mesh_multiview_view_count;
    uint32_t                                           mesh_output_per_vertex_granularity;
    uint32_t                                           mesh_output_per_primitive_granularity;
} vk_physical_device_mesh_shader_properties_nv_t;

typedef struct vk_draw_mesh_tasks_indirect_command_nv_t {
    uint32_t                                           task_count;
    uint32_t                                           first_task;
} vk_draw_mesh_tasks_indirect_command_nv_t;
#define pfn_vk_cmd_draw_mesh_tasks_nv_fn_t                                PFN_vkCmdDrawMeshTasksNV
#define pfn_vk_cmd_draw_mesh_tasks_indirect_nv_fn_t                       PFN_vkCmdDrawMeshTasksIndirectNV
#define pfn_vk_cmd_draw_mesh_tasks_indirect_count_nv_fn_t                 PFN_vkCmdDrawMeshTasksIndirectCountNV

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_draw_mesh_tasks_nv(
    vk_command_buffer_t                                commandBuffer,
    uint32_t                                           taskCount,
    uint32_t                                           firstTask)
{
    return vkCmdDrawMeshTasksNV (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    uint32_t                                    *)&taskCount,
            *(    uint32_t                                    *)&firstTask);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_draw_mesh_tasks_indirect_nv(
    vk_command_buffer_t                                commandBuffer,
    vk_buffer_t                                        buffer,
    vk_device_size_t                                   offset,
    uint32_t                                           drawCount,
    uint32_t                                           stride)
{
    return vkCmdDrawMeshTasksIndirectNV (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    VkBuffer                                    *)&buffer,
            *(    VkDeviceSize                                *)&offset,
            *(    uint32_t                                    *)&drawCount,
            *(    uint32_t                                    *)&stride);
}

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_draw_mesh_tasks_indirect_count_nv(
    vk_command_buffer_t                                commandBuffer,
    vk_buffer_t                                        buffer,
    vk_device_size_t                                   offset,
    vk_buffer_t                                        countBuffer,
    vk_device_size_t                                   countBufferOffset,
    uint32_t                                           maxDrawCount,
    uint32_t                                           stride)
{
    return vkCmdDrawMeshTasksIndirectCountNV (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    VkBuffer                                    *)&buffer,
            *(    VkDeviceSize                                *)&offset,
            *(    VkBuffer                                    *)&countBuffer,
            *(    VkDeviceSize                                *)&countBufferOffset,
            *(    uint32_t                                    *)&maxDrawCount,
            *(    uint32_t                                    *)&stride);
}

typedef struct vk_physical_device_fragment_shader_barycentric_features_nv_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_NV;
    void* p_next =                                     NULL;
    vk_bool32_t                                        fragment_shader_barycentric;
} vk_physical_device_fragment_shader_barycentric_features_nv_t;

typedef struct vk_physical_device_shader_image_footprint_features_nv_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_IMAGE_FOOTPRINT_FEATURES_NV;
    void* p_next =                                     NULL;
    vk_bool32_t                                        image_footprint;
} vk_physical_device_shader_image_footprint_features_nv_t;

typedef struct vk_pipeline_viewport_exclusive_scissor_state_create_info_nv_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_EXCLUSIVE_SCISSOR_STATE_CREATE_INFO_NV;
    const void* p_next =                               NULL;
    uint32_t                                           exclusive_scissor_count;
    const vk_rect2d_t*                                p_exclusive_scissors;
} vk_pipeline_viewport_exclusive_scissor_state_create_info_nv_t;

typedef struct vk_physical_device_exclusive_scissor_features_nv_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXCLUSIVE_SCISSOR_FEATURES_NV;
    void* p_next =                                     NULL;
    vk_bool32_t                                        exclusive_scissor;
} vk_physical_device_exclusive_scissor_features_nv_t;
#define pfn_vk_cmd_set_exclusive_scissor_nv_fn_t                          PFN_vkCmdSetExclusiveScissorNV

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_set_exclusive_scissor_nv(
    vk_command_buffer_t                                commandBuffer,
    uint32_t                                           firstExclusiveScissor,
    uint32_t                                           exclusiveScissorCount,
    const vk_rect2d_t*                                pExclusiveScissors)
{
    return vkCmdSetExclusiveScissorNV (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    uint32_t                                    *)&firstExclusiveScissor,
            *(    uint32_t                                    *)&exclusiveScissorCount,
            *(    const VkRect2D*                             *)&pExclusiveScissors);
}

typedef struct vk_queue_family_checkpoint_properties_nv_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_QUEUE_FAMILY_CHECKPOINT_PROPERTIES_NV;
    void* p_next =                                     NULL;
    vk_pipeline_stage_flags_t                          checkpoint_execution_stage_mask;
} vk_queue_family_checkpoint_properties_nv_t;

typedef struct vk_checkpoint_data_nv_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_CHECKPOINT_DATA_NV;
    void* p_next =                                     NULL;
    vk_pipeline_stage_flag_bits_t                      stage;
    void*                                              p_checkpoint_marker;
} vk_checkpoint_data_nv_t;
#define pfn_vk_cmd_set_checkpoint_nv_fn_t                                 PFN_vkCmdSetCheckpointNV
#define pfn_vk_get_queue_checkpoint_data_nv_fn_t                          PFN_vkGetQueueCheckpointDataNV

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_set_checkpoint_nv(
    vk_command_buffer_t                                commandBuffer,
    const void*                                        pCheckpointMarker)
{
    return vkCmdSetCheckpointNV (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    const void*                                 *)&pCheckpointMarker);
}

inline VKAPI_ATTR void VKAPI_CALL vk_get_queue_checkpoint_data_nv(
    vk_queue_t                                         queue,
    uint32_t*                                          pCheckpointDataCount,
    vk_checkpoint_data_nv_t*                           pCheckpointData)
{
    return vkGetQueueCheckpointDataNV (
            *(    VkQueue                                     *)&queue,
            *(    uint32_t*                                   *)&pCheckpointDataCount,
            *(    VkCheckpointDataNV*                         *)&pCheckpointData);
}

typedef struct vk_physical_device_shader_integer_functions2_features_intel_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_INTEGER_FUNCTIONS_2_FEATURES_INTEL;
    void* p_next =                                     NULL;
    vk_bool32_t                                        shader_integer_functions2;
} vk_physical_device_shader_integer_functions2_features_intel_t;
#define vk_performance_configuration_intel_t                              VkPerformanceConfigurationINTEL 
#define vk_performance_configuration_type_intel_t                         VkPerformanceConfigurationTypeINTEL
#define vk_query_pool_sampling_mode_intel_t                               VkQueryPoolSamplingModeINTEL
#define vk_performance_override_type_intel_t                              VkPerformanceOverrideTypeINTEL
#define vk_performance_parameter_type_intel_t                             VkPerformanceParameterTypeINTEL
#define vk_performance_value_type_intel_t                                 VkPerformanceValueTypeINTEL

typedef union vk_performance_value_data_intel_t {
    uint32_t                                           value32;
    uint64_t                                           value64;
    float                                              value_float;
    vk_bool32_t                                        value_bool;
    const char*                                        value_string;
} vk_performance_value_data_intel_t;

typedef struct vk_performance_value_intel_t {
    vk_performance_value_type_intel_t                  type;
    vk_performance_value_data_intel_t                  data;
} vk_performance_value_intel_t;

typedef struct vk_initialize_performance_api_info_intel_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_INITIALIZE_PERFORMANCE_API_INFO_INTEL;
    const void* p_next =                               NULL;
    void*                                              p_user_data;
} vk_initialize_performance_api_info_intel_t;

typedef struct vk_query_pool_create_info_intel_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO_INTEL;
    const void* p_next =                               NULL;
    vk_query_pool_sampling_mode_intel_t                performance_counters_sampling;
} vk_query_pool_create_info_intel_t;

typedef struct vk_performance_marker_info_intel_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PERFORMANCE_MARKER_INFO_INTEL;
    const void* p_next =                               NULL;
    uint64_t                                           marker;
} vk_performance_marker_info_intel_t;

typedef struct vk_performance_stream_marker_info_intel_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PERFORMANCE_STREAM_MARKER_INFO_INTEL;
    const void* p_next =                               NULL;
    uint32_t                                           marker;
} vk_performance_stream_marker_info_intel_t;

typedef struct vk_performance_override_info_intel_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PERFORMANCE_OVERRIDE_INFO_INTEL;
    const void* p_next =                               NULL;
    vk_performance_override_type_intel_t               type;
    vk_bool32_t                                        enable;
    uint64_t                                           parameter;
} vk_performance_override_info_intel_t;

typedef struct vk_performance_configuration_acquire_info_intel_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PERFORMANCE_CONFIGURATION_ACQUIRE_INFO_INTEL;
    const void* p_next =                               NULL;
    vk_performance_configuration_type_intel_t          type;
} vk_performance_configuration_acquire_info_intel_t;
#define pfn_vk_initialize_performance_api_intel_fn_t                      PFN_vkInitializePerformanceApiINTEL
#define pfn_vk_uninitialize_performance_api_intel_fn_t                    PFN_vkUninitializePerformanceApiINTEL
#define pfn_vk_cmd_set_performance_marker_intel_fn_t                      PFN_vkCmdSetPerformanceMarkerINTEL
#define pfn_vk_cmd_set_performance_stream_marker_intel_fn_t               PFN_vkCmdSetPerformanceStreamMarkerINTEL
#define pfn_vk_cmd_set_performance_override_intel_fn_t                    PFN_vkCmdSetPerformanceOverrideINTEL
#define pfn_vk_acquire_performance_configuration_intel_fn_t               PFN_vkAcquirePerformanceConfigurationINTEL
#define pfn_vk_release_performance_configuration_intel_fn_t               PFN_vkReleasePerformanceConfigurationINTEL
#define pfn_vk_queue_set_performance_configuration_intel_fn_t             PFN_vkQueueSetPerformanceConfigurationINTEL
#define pfn_vk_get_performance_parameter_intel_fn_t                       PFN_vkGetPerformanceParameterINTEL

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_initialize_performance_api_intel(
    vk_device_t                                        device,
    const vk_initialize_performance_api_info_intel_t*  pInitializeInfo)
{
    return vkInitializePerformanceApiINTEL (
            *(    VkDevice                                    *)&device,
            *(    const VkInitializePerformanceApiInfoINTEL*  *)&pInitializeInfo);
}

inline VKAPI_ATTR void VKAPI_CALL vk_uninitialize_performance_api_intel(
    vk_device_t                                        device)
{
    return vkUninitializePerformanceApiINTEL (
            *(    VkDevice                                    *)&device);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_cmd_set_performance_marker_intel(
    vk_command_buffer_t                                commandBuffer,
    const vk_performance_marker_info_intel_t*          pMarkerInfo)
{
    return vkCmdSetPerformanceMarkerINTEL (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    const VkPerformanceMarkerInfoINTEL*         *)&pMarkerInfo);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_cmd_set_performance_stream_marker_intel(
    vk_command_buffer_t                                commandBuffer,
    const vk_performance_stream_marker_info_intel_t*   pMarkerInfo)
{
    return vkCmdSetPerformanceStreamMarkerINTEL (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    const VkPerformanceStreamMarkerInfoINTEL*   *)&pMarkerInfo);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_cmd_set_performance_override_intel(
    vk_command_buffer_t                                commandBuffer,
    const vk_performance_override_info_intel_t*        pOverrideInfo)
{
    return vkCmdSetPerformanceOverrideINTEL (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    const VkPerformanceOverrideInfoINTEL*       *)&pOverrideInfo);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_acquire_performance_configuration_intel(
    vk_device_t                                        device,
    const vk_performance_configuration_acquire_info_intel_t* pAcquireInfo,
    vk_performance_configuration_intel_t*              pConfiguration)
{
    return vkAcquirePerformanceConfigurationINTEL (
            *(    VkDevice                                    *)&device,
            *(    const VkPerformanceConfigurationAcquireInfoINTEL* *)&pAcquireInfo,
            *(    VkPerformanceConfigurationINTEL*            *)&pConfiguration);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_release_performance_configuration_intel(
    vk_device_t                                        device,
    vk_performance_configuration_intel_t               configuration)
{
    return vkReleasePerformanceConfigurationINTEL (
            *(    VkDevice                                    *)&device,
            *(    VkPerformanceConfigurationINTEL             *)&configuration);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_queue_set_performance_configuration_intel(
    vk_queue_t                                         queue,
    vk_performance_configuration_intel_t               configuration)
{
    return vkQueueSetPerformanceConfigurationINTEL (
            *(    VkQueue                                     *)&queue,
            *(    VkPerformanceConfigurationINTEL             *)&configuration);
}

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_performance_parameter_intel(
    vk_device_t                                        device,
    vk_performance_parameter_type_intel_t              parameter,
    vk_performance_value_intel_t*                      pValue)
{
    return vkGetPerformanceParameterINTEL (
            *(    VkDevice                                    *)&device,
            *(    VkPerformanceParameterTypeINTEL             *)&parameter,
            *(    VkPerformanceValueINTEL*                    *)&pValue);
}

typedef struct vk_physical_device_pci_bus_info_properties_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PCI_BUS_INFO_PROPERTIES_EXT;
    void* p_next =                                     NULL;
    uint32_t                                           pci_domain;
    uint32_t                                           pci_bus;
    uint32_t                                           pci_device;
    uint32_t                                           pci_function;
} vk_physical_device_pci_bus_info_properties_ext_t;

typedef struct vk_display_native_hdr_surface_capabilities_amd_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_DISPLAY_NATIVE_HDR_SURFACE_CAPABILITIES_AMD;
    void* p_next =                                     NULL;
    vk_bool32_t                                        local_dimming_support;
} vk_display_native_hdr_surface_capabilities_amd_t;

typedef struct vk_swapchain_display_native_hdr_create_info_amd_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_SWAPCHAIN_DISPLAY_NATIVE_HDR_CREATE_INFO_AMD;
    const void* p_next =                               NULL;
    vk_bool32_t                                        local_dimming_enable;
} vk_swapchain_display_native_hdr_create_info_amd_t;
#define pfn_vk_set_local_dimming_amd_fn_t                                 PFN_vkSetLocalDimmingAMD

inline VKAPI_ATTR void VKAPI_CALL vk_set_local_dimming_amd(
    vk_device_t                                        device,
    vk_swapchain_khr_t                                 swapChain,
    vk_bool32_t                                        localDimmingEnable)
{
    return vkSetLocalDimmingAMD (
            *(    VkDevice                                    *)&device,
            *(    VkSwapchainKHR                              *)&swapChain,
            *(    VkBool32                                    *)&localDimmingEnable);
}

typedef struct vk_physical_device_fragment_density_map_features_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_FEATURES_EXT;
    void* p_next =                                     NULL;
    vk_bool32_t                                        fragment_density_map;
    vk_bool32_t                                        fragment_density_map_dynamic;
    vk_bool32_t                                        fragment_density_map_non_subsampled_images;
} vk_physical_device_fragment_density_map_features_ext_t;

typedef struct vk_physical_device_fragment_density_map_properties_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_PROPERTIES_EXT;
    void* p_next =                                     NULL;
    vk_extent2d_t                                     min_fragment_density_texel_size;
    vk_extent2d_t                                     max_fragment_density_texel_size;
    vk_bool32_t                                        fragment_density_invocations;
} vk_physical_device_fragment_density_map_properties_ext_t;

typedef struct vk_render_pass_fragment_density_map_create_info_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_RENDER_PASS_FRAGMENT_DENSITY_MAP_CREATE_INFO_EXT;
    const void* p_next =                               NULL;
    vk_attachment_reference_t                          fragment_density_map_attachment;
} vk_render_pass_fragment_density_map_create_info_ext_t;
#define vk_physical_device_scalar_block_layout_features_ext_t             VkPhysicalDeviceScalarBlockLayoutFeaturesEXT 

typedef struct vk_physical_device_subgroup_size_control_features_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_FEATURES_EXT;
    void* p_next =                                     NULL;
    vk_bool32_t                                        subgroup_size_control;
    vk_bool32_t                                        compute_full_subgroups;
} vk_physical_device_subgroup_size_control_features_ext_t;

typedef struct vk_physical_device_subgroup_size_control_properties_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_PROPERTIES_EXT;
    void* p_next =                                     NULL;
    uint32_t                                           min_subgroup_size;
    uint32_t                                           max_subgroup_size;
    uint32_t                                           max_compute_workgroup_subgroups;
    vk_shader_stage_flags_t                            required_subgroup_size_stages;
} vk_physical_device_subgroup_size_control_properties_ext_t;

typedef struct vk_pipeline_shader_stage_required_subgroup_size_create_info_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_REQUIRED_SUBGROUP_SIZE_CREATE_INFO_EXT;
    void* p_next =                                     NULL;
    uint32_t                                           required_subgroup_size;
} vk_pipeline_shader_stage_required_subgroup_size_create_info_ext_t;
#define vk_shader_core_properties_flag_bits_amd_t                         VkShaderCorePropertiesFlagBitsAMD
#define vk_shader_core_properties_flags_amd_t                             VkShaderCorePropertiesFlagsAMD 

typedef struct vk_physical_device_shader_core_properties2_amd_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CORE_PROPERTIES_2_AMD;
    void* p_next =                                     NULL;
    vk_shader_core_properties_flags_amd_t              shader_core_features;
    uint32_t                                           active_compute_unit_count;
} vk_physical_device_shader_core_properties2_amd_t;

typedef struct vk_physical_device_coherent_memory_features_amd_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COHERENT_MEMORY_FEATURES_AMD;
    void* p_next =                                     NULL;
    vk_bool32_t                                        device_coherent_memory;
} vk_physical_device_coherent_memory_features_amd_t;

typedef struct vk_physical_device_memory_budget_properties_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT;
    void* p_next =                                     NULL;
    vk_device_size_t                                   heap_budget[VK_MAX_MEMORY_HEAPS];
    vk_device_size_t                                   heap_usage[VK_MAX_MEMORY_HEAPS];
} vk_physical_device_memory_budget_properties_ext_t;

typedef struct vk_physical_device_memory_priority_features_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PRIORITY_FEATURES_EXT;
    void* p_next =                                     NULL;
    vk_bool32_t                                        memory_priority;
} vk_physical_device_memory_priority_features_ext_t;

typedef struct vk_memory_priority_allocate_info_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_MEMORY_PRIORITY_ALLOCATE_INFO_EXT;
    const void* p_next =                               NULL;
    float                                              priority;
} vk_memory_priority_allocate_info_ext_t;

typedef struct vk_physical_device_dedicated_allocation_image_aliasing_features_nv_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEDICATED_ALLOCATION_IMAGE_ALIASING_FEATURES_NV;
    void* p_next =                                     NULL;
    vk_bool32_t                                        dedicated_allocation_image_aliasing;
} vk_physical_device_dedicated_allocation_image_aliasing_features_nv_t;

typedef struct vk_physical_device_buffer_device_address_features_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_EXT;
    void* p_next =                                     NULL;
    vk_bool32_t                                        buffer_device_address;
    vk_bool32_t                                        buffer_device_address_capture_replay;
    vk_bool32_t                                        buffer_device_address_multi_device;
} vk_physical_device_buffer_device_address_features_ext_t;
#define vk_physical_device_buffer_address_features_ext_t                  VkPhysicalDeviceBufferAddressFeaturesEXT 
#define vk_buffer_device_address_info_ext_t                               VkBufferDeviceAddressInfoEXT 

typedef struct vk_buffer_device_address_create_info_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_CREATE_INFO_EXT;
    const void* p_next =                               NULL;
    vk_device_address_t                                device_address;
} vk_buffer_device_address_create_info_ext_t;
#define pfn_vk_get_buffer_device_address_ext_fn_t                         PFN_vkGetBufferDeviceAddressEXT

inline VKAPI_ATTR vk_device_address_t VKAPI_CALL vk_get_buffer_device_address_ext(
    vk_device_t                                        device,
    const vk_buffer_device_address_info_t*             pInfo)
{
    return vkGetBufferDeviceAddressEXT (
            *(    VkDevice                                    *)&device,
            *(    const VkBufferDeviceAddressInfo*            *)&pInfo);
}
#define vk_tool_purpose_flag_bits_ext_t                                   VkToolPurposeFlagBitsEXT
#define vk_tool_purpose_flags_ext_t                                       VkToolPurposeFlagsEXT 

typedef struct vk_physical_device_tool_properties_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TOOL_PROPERTIES_EXT;
    void* p_next =                                     NULL;
    char                                               name[VK_MAX_EXTENSION_NAME_SIZE];
    char                                               version[VK_MAX_EXTENSION_NAME_SIZE];
    vk_tool_purpose_flags_ext_t                        purposes;
    char                                               description[VK_MAX_DESCRIPTION_SIZE];
    char                                               layer[VK_MAX_EXTENSION_NAME_SIZE];
} vk_physical_device_tool_properties_ext_t;
#define pfn_vk_get_physical_device_tool_properties_ext_fn_t               PFN_vkGetPhysicalDeviceToolPropertiesEXT

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_physical_device_tool_properties_ext(
    vk_physical_device_t                               physicalDevice,
    uint32_t*                                          pToolCount,
    vk_physical_device_tool_properties_ext_t*          pToolProperties)
{
    return vkGetPhysicalDeviceToolPropertiesEXT (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    uint32_t*                                   *)&pToolCount,
            *(    VkPhysicalDeviceToolPropertiesEXT*          *)&pToolProperties);
}
#define vk_image_stencil_usage_create_info_ext_t                          VkImageStencilUsageCreateInfoEXT 
#define vk_validation_feature_enable_ext_t                                VkValidationFeatureEnableEXT
#define vk_validation_feature_disable_ext_t                               VkValidationFeatureDisableEXT

typedef struct vk_validation_features_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
    const void* p_next =                               NULL;
    uint32_t                                           enabled_validation_feature_count;
    const vk_validation_feature_enable_ext_t*          p_enabled_validation_features;
    uint32_t                                           disabled_validation_feature_count;
    const vk_validation_feature_disable_ext_t*         p_disabled_validation_features;
} vk_validation_features_ext_t;
#define vk_component_type_nv_t                                            VkComponentTypeNV
#define vk_scope_nv_t                                                     VkScopeNV

typedef struct vk_cooperative_matrix_properties_nv_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_COOPERATIVE_MATRIX_PROPERTIES_NV;
    void* p_next =                                     NULL;
    uint32_t                                           m_size;
    uint32_t                                           n_size;
    uint32_t                                           k_size;
    vk_component_type_nv_t                             a_type;
    vk_component_type_nv_t                             b_type;
    vk_component_type_nv_t                             c_type;
    vk_component_type_nv_t                             d_type;
    vk_scope_nv_t                                      scope;
} vk_cooperative_matrix_properties_nv_t;

typedef struct vk_physical_device_cooperative_matrix_features_nv_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_FEATURES_NV;
    void* p_next =                                     NULL;
    vk_bool32_t                                        cooperative_matrix;
    vk_bool32_t                                        cooperative_matrix_robust_buffer_access;
} vk_physical_device_cooperative_matrix_features_nv_t;

typedef struct vk_physical_device_cooperative_matrix_properties_nv_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_PROPERTIES_NV;
    void* p_next =                                     NULL;
    vk_shader_stage_flags_t                            cooperative_matrix_supported_stages;
} vk_physical_device_cooperative_matrix_properties_nv_t;
#define pfn_vk_get_physical_device_cooperative_matrix_properties_nv_fn_t  PFN_vkGetPhysicalDeviceCooperativeMatrixPropertiesNV

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_physical_device_cooperative_matrix_properties_nv(
    vk_physical_device_t                               physicalDevice,
    uint32_t*                                          pPropertyCount,
    vk_cooperative_matrix_properties_nv_t*             pProperties)
{
    return vkGetPhysicalDeviceCooperativeMatrixPropertiesNV (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    uint32_t*                                   *)&pPropertyCount,
            *(    VkCooperativeMatrixPropertiesNV*            *)&pProperties);
}
#define vk_coverage_reduction_mode_nv_t                                   VkCoverageReductionModeNV
#define vk_pipeline_coverage_reduction_state_create_flags_nv_t            VkPipelineCoverageReductionStateCreateFlagsNV 

typedef struct vk_physical_device_coverage_reduction_mode_features_nv_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COVERAGE_REDUCTION_MODE_FEATURES_NV;
    void* p_next =                                     NULL;
    vk_bool32_t                                        coverage_reduction_mode;
} vk_physical_device_coverage_reduction_mode_features_nv_t;

typedef struct vk_pipeline_coverage_reduction_state_create_info_nv_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PIPELINE_COVERAGE_REDUCTION_STATE_CREATE_INFO_NV;
    const void* p_next =                               NULL;
    vk_pipeline_coverage_reduction_state_create_flags_nv_t flags;
    vk_coverage_reduction_mode_nv_t                    coverage_reduction_mode;
} vk_pipeline_coverage_reduction_state_create_info_nv_t;

typedef struct vk_framebuffer_mixed_samples_combination_nv_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_FRAMEBUFFER_MIXED_SAMPLES_COMBINATION_NV;
    void* p_next =                                     NULL;
    vk_coverage_reduction_mode_nv_t                    coverage_reduction_mode;
    vk_sample_count_flag_bits_t                        rasterization_samples;
    vk_sample_count_flags_t                            depth_stencil_samples;
    vk_sample_count_flags_t                            color_samples;
} vk_framebuffer_mixed_samples_combination_nv_t;
#define pfn_vk_get_physical_device_supported_framebuffer_mixed_samples_combinations_nv_fn_t PFN_vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_get_physical_device_supported_framebuffer_mixed_samples_combinations_nv(
    vk_physical_device_t                               physicalDevice,
    uint32_t*                                          pCombinationCount,
    vk_framebuffer_mixed_samples_combination_nv_t*     pCombinations)
{
    return vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV (
            *(    VkPhysicalDevice                            *)&physicalDevice,
            *(    uint32_t*                                   *)&pCombinationCount,
            *(    VkFramebufferMixedSamplesCombinationNV*     *)&pCombinations);
}

typedef struct vk_physical_device_fragment_shader_interlock_features_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_INTERLOCK_FEATURES_EXT;
    void* p_next =                                     NULL;
    vk_bool32_t                                        fragment_shader_sample_interlock;
    vk_bool32_t                                        fragment_shader_pixel_interlock;
    vk_bool32_t                                        fragment_shader_shading_rate_interlock;
} vk_physical_device_fragment_shader_interlock_features_ext_t;

typedef struct vk_physical_device_ycbcr_image_arrays_features_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_YCBCR_IMAGE_ARRAYS_FEATURES_EXT;
    void* p_next =                                     NULL;
    vk_bool32_t                                        ycbcr_image_arrays;
} vk_physical_device_ycbcr_image_arrays_features_ext_t;
#define vk_headless_surface_create_flags_ext_t                            VkHeadlessSurfaceCreateFlagsEXT 

typedef struct vk_headless_surface_create_info_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_HEADLESS_SURFACE_CREATE_INFO_EXT;
    const void* p_next =                               NULL;
    vk_headless_surface_create_flags_ext_t             flags;
} vk_headless_surface_create_info_ext_t;
#define pfn_vk_create_headless_surface_ext_fn_t                           PFN_vkCreateHeadlessSurfaceEXT

inline VKAPI_ATTR vk_result_t VKAPI_CALL vk_create_headless_surface_ext(
    vk_instance_t                                      instance,
    const vk_headless_surface_create_info_ext_t*       pCreateInfo,
    const vk_allocation_callbacks_t*                   pAllocator,
    vk_surface_khr_t*                                  pSurface)
{
    return vkCreateHeadlessSurfaceEXT (
            *(    VkInstance                                  *)&instance,
            *(    const VkHeadlessSurfaceCreateInfoEXT*       *)&pCreateInfo,
            *(    const VkAllocationCallbacks*                *)&pAllocator,
            *(    VkSurfaceKHR*                               *)&pSurface);
}
#define vk_line_rasterization_mode_ext_t                                  VkLineRasterizationModeEXT

typedef struct vk_physical_device_line_rasterization_features_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_FEATURES_EXT;
    void* p_next =                                     NULL;
    vk_bool32_t                                        rectangular_lines;
    vk_bool32_t                                        bresenham_lines;
    vk_bool32_t                                        smooth_lines;
    vk_bool32_t                                        stippled_rectangular_lines;
    vk_bool32_t                                        stippled_bresenham_lines;
    vk_bool32_t                                        stippled_smooth_lines;
} vk_physical_device_line_rasterization_features_ext_t;

typedef struct vk_physical_device_line_rasterization_properties_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_PROPERTIES_EXT;
    void* p_next =                                     NULL;
    uint32_t                                           line_sub_pixel_precision_bits;
} vk_physical_device_line_rasterization_properties_ext_t;

typedef struct vk_pipeline_rasterization_line_state_create_info_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_LINE_STATE_CREATE_INFO_EXT;
    const void* p_next =                               NULL;
    vk_line_rasterization_mode_ext_t                   line_rasterization_mode;
    vk_bool32_t                                        stippled_line_enable;
    uint32_t                                           line_stipple_factor;
    uint16_t                                           line_stipple_pattern;
} vk_pipeline_rasterization_line_state_create_info_ext_t;
#define pfn_vk_cmd_set_line_stipple_ext_fn_t                              PFN_vkCmdSetLineStippleEXT

inline VKAPI_ATTR void VKAPI_CALL vk_cmd_set_line_stipple_ext(
    vk_command_buffer_t                                commandBuffer,
    uint32_t                                           lineStippleFactor,
    uint16_t                                           lineStipplePattern)
{
    return vkCmdSetLineStippleEXT (
            *(    VkCommandBuffer                             *)&commandBuffer,
            *(    uint32_t                                    *)&lineStippleFactor,
            *(    uint16_t                                    *)&lineStipplePattern);
}
#define vk_physical_device_host_query_reset_features_ext_t                VkPhysicalDeviceHostQueryResetFeaturesEXT 
#define pfn_vk_reset_query_pool_ext_fn_t                                  PFN_vkResetQueryPoolEXT

inline VKAPI_ATTR void VKAPI_CALL vk_reset_query_pool_ext(
    vk_device_t                                        device,
    vk_query_pool_t                                    queryPool,
    uint32_t                                           firstQuery,
    uint32_t                                           queryCount)
{
    return vkResetQueryPoolEXT (
            *(    VkDevice                                    *)&device,
            *(    VkQueryPool                                 *)&queryPool,
            *(    uint32_t                                    *)&firstQuery,
            *(    uint32_t                                    *)&queryCount);
}

typedef struct vk_physical_device_index_type_uint8_features_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INDEX_TYPE_UINT8_FEATURES_EXT;
    void* p_next =                                     NULL;
    vk_bool32_t                                        index_type_uint8;
} vk_physical_device_index_type_uint8_features_ext_t;

typedef struct vk_physical_device_shader_demote_to_helper_invocation_features_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DEMOTE_TO_HELPER_INVOCATION_FEATURES_EXT;
    void* p_next =                                     NULL;
    vk_bool32_t                                        shader_demote_to_helper_invocation;
} vk_physical_device_shader_demote_to_helper_invocation_features_ext_t;

typedef struct vk_physical_device_texel_buffer_alignment_features_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TEXEL_BUFFER_ALIGNMENT_FEATURES_EXT;
    void* p_next =                                     NULL;
    vk_bool32_t                                        texel_buffer_alignment;
} vk_physical_device_texel_buffer_alignment_features_ext_t;

typedef struct vk_physical_device_texel_buffer_alignment_properties_ext_t {
    vk_structure_type_t s_type =                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TEXEL_BUFFER_ALIGNMENT_PROPERTIES_EXT;
    void* p_next =                                     NULL;
    vk_device_size_t                                   storage_texel_buffer_offset_alignment_bytes;
    vk_bool32_t                                        storage_texel_buffer_offset_single_texel_alignment;
    vk_device_size_t                                   uniform_texel_buffer_offset_alignment_bytes;
    vk_bool32_t                                        uniform_texel_buffer_offset_single_texel_alignment;
} vk_physical_device_texel_buffer_alignment_properties_ext_t;

#endif
