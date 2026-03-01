#ifndef VULKAN_COMPOSER_H
#define VULKAN_COMPOSER_H

/*! TODO:
 * 
 * - It is not at all clear what functions can be used in here
 * - This should be further split:
 *      1. The drawing part should be one, ie, those vku::ref_t thigs
 *      2. The lua part
 *      3. The config reading part (not the gpu defines, only the parsing logic)
 * - The functions that can be used should be clearly marked at the start of the code.
 * - There should be less global objects (none that do not define things) and the state should be
 * remembered in a state object. This object should be able to follow stages of completness.
 * (function definitions)
 */

/*!
 * Core Objects and Functions:
 * ===========================
 * 
 * The following are used to manage all the objects inside the Vulkan Utils (vkc) wrapper:
 * 
 * vku::ref_t<VkuT>
 * ----------------
 *
 * Description: Reference handle to a Vulkan wrapper object (VkuT). Implements 
 * safe shared ownership and automatic update propagation via base_t. Users hold 
 * ref_t instead of direct object pointers.
 *
 * Member functions:
 * - operator->(): Access the underlying VkuT object.
 * - operator*(): Dereference to the underlying VkuT object.
 * - get(): Returns raw pointer to the underlying VkuT object.
 * - to_related(): Cast between related reference types safely.
 * - operator bool(): Checks if the reference holds a valid object.
 *
 * Init: create_obj_ref(obj, dependencies)
 *   - Parameters:
 *     - obj: Unique pointer to a VkuT object.
 *     - dependencies: List of ref_base_t objects this object depends on.
 *
 * Notes:
 * - Manages the DAG of dependencies for update propagation.
 * - Allows safe, type-checked access to Vulkan wrapper objects.
 * 
 * 
 * vku::ref_t<VkuT>::update()
 * --------------------------
 *
 * Propagates changes in this object to all dependent objects in the DAG.
 * 
 * - Each Vulkan wrapper object (object_t) may implement update() to reflect
 *   modifications to its GPU resources (e.g., updating descriptor sets or buffers).
 * - The base_t wrapper ensures that calling update() on a ref_t-managed object
 *   automatically calls update() on all dependees (objects that depend on it).
 * - This allows changes to a low-level resource (like a buffer or image) to
 *   cascade safely to higher-level objects (like pipelines or descriptor sets)
 *   that rely on it.
 *
 * Example:
 *   If a buffer bound in a descriptor set is modified, calling update() on
 *   the buffer’s ref_t triggers update() on all desc_set_t objects that use it,
 *   ensuring that GPU descriptor sets remain in sync.
 *
 * Notes:
 * - The base_t update mechanism is recursive, but only follows _dependees.
 * - Users usually call update() on high-level objects (e.g., desc_set_t),
 *   which automatically propagates to underlying resources as needed.
 * 
 * 
 * vku::ref_t<VkuT>::rebuild()
 * ---------------------------
 *
 * Re-initializes this object and all dependent objects in the DAG.
 * 
 * - Each Vulkan wrapper object (object_t) implements _init() and _uninit() to 
 *   allocate or release GPU resources as needed.
 * - This allows structural changes to a low-level resource (like resizing an image,
 *   changing a buffer format, or replacing a shader) to propagate safely to all 
 *   higher-level objects (like pipelines or descriptor sets) that rely on it.
 *
 * Example:
 *   If a window or swapchain’s dimensions change, calling rebuild() on the swapchain’s
 *   ref_t will uninitialize and reinitialize the swapchain with the new size, then
 *   automatically rebuild all framebuffers, pipelines, or descriptor sets that depend
 *   on it, ensuring the GPU objects stay consistent with the new window size. All those
 *   objects will be rebuilt using their stored parameters.
 *
 * Notes:
 * - The base_t rebuild mechanism is recursive and follows _dependees.
 * - rebuild() is used for structural or configuration changes that require full
 *   GPU resource re-creation, whereas update() is for incremental changes.
 * 
 * 
 * vku::object_t
 * --------
 *
 * Description: Base class for all Vulkan wrapper objects in the library. Provides 
 * virtual methods for initialization (_init/_uninit), type identification, 
 * string representation, and optional update propagation.
 *
 * Members:
 * - cbks: Optional callbacks invoked before/after init and uninit.
 *
 * Member functions:
 * - type_id(): Returns the type identifier of the object.
 * - to_string(): Returns a human-readable string describing the object.
 * - update(): Default no-op. Can be overridden to propagate changes to dependent GPU resources.
 *
 * Notes:
 * - update() may be called to refresh GPU state after modifying object parameters.
 * 
 * 
 * LUA API: Vulkan Utils
 * =====================
 * 
 * The Vulkan Utils (vulkan_utils or vku) Lua bindings provide a scripting interface for interacting
 * with Vulkan objects managed by the C++ backend. The bindings are designed to give users a
 * flexible way to control GPU resources, rendering loops, and engine logic directly from Lua
 * scripts, without exposing the raw Vulkan API.
 * 
 * Key Concepts:
 * -------------
 *
 * 1. Utility functions:
 *    - Examples: glfw_pool_events, get_key, signal_close, etc.
 *    - Used to manage input, events, and window state.
 *
 * 2. References to objects:
 *    - Objects created in YAML or in C++ are accessible as named references in Lua:
 *      vku.object_name
 *    - These are ref_t-managed Vulkan objects, allowing safe manipulation and
 *      automatic dependency propagation.
 *
 * 3. Constants and defines:
 *    - Vulkan and framework-specific constants are exposed in Lua for convenience:
 *      vku.VK_PIPELINE_STAGE_HOST_BIT
 *
 * 4. Object methods:
 *    - Named objects in Lua can call internal methods implemented in C++:
 *      - vku.cbuff:begin(...)
 *      - vku.window:rebuild()
 *    - Methods operate directly on GPU resources or manage Vulkan object lifetimes.
 *
 * How it works:
 * -------------
 * 1. YAML is loaded -> objects created -> Lua can be executed.
 * 2. C++ backend creates objects wrapped in ref_t handles for safe reference counting
 *    and dependency tracking.
 * 3. Lua scripts interact with objects, submit GPU commands, and respond to events.
 * 4. Users can expose C++ plugins to Lua (int function(lua_State *)) for advanced
 *    GPU operations such as filling triangle data or analyzing compute shader results.
 * 5. Lua can create internal objects dynamically, similar to YAML object creation.
 *
 * Example Usage:
 * --------------
 * vku = require("vulkan_utils")
 *
 * -- example filling buffers:
 * vku.fill_buffer_with_triangles_vertices(vku.staging_buffer.data(), vku.staging_buffer.size())
 * vku.copy_from_cpu_to_gpu(vku.staging_buffer, vku.vbuff, vku.staging_buffer.size(), 0)
 *
 * vku.fill_buffer_with_triangles_indexes(vku.staging_buffer.data(), vku.staging_buffer.size())
 * vku.copy_from_cpu_to_gpu(vku.staging_buffer, vku.ibuff, vku.SIZEOF_INT*3, 0)
 * 
 * -- example creating a vku object (vkc::lua_var_t is a dummy object)
 * t = {
 *     m_type = "vkc::lua_var_t",
 *     var1 = 1,
 *     var2 = 2,
 *     var3 = { var4 = "str" }
 * }
 * to = vku.create_object("tag_name", t)
 *
 * function on_loop_run()
 *     vku.glfw_pool_events()
 *     if vku.get_key(vku.window, vku.GLFW_KEY_ESCAPE) == vku.GLFW_PRESS then
 *         vku.signal_close()
 *     end
 *
 *     img_idx = vku.aquire_next_img(vku.swc, vku.img_sem)
 *
 *     vku.cbuff:begin(vku.VK_COMMAND_BUFFER_USAGE_NONE)
 *     vku.cbuff:begin_rpass(vku.fbs, img_idx)
 *     vku.cbuff:bind_vert_buffs(0, {{vku.vbuff, 0}})
 *     vku.cbuff:bind_idx_buff(vku.ibuff, 0, vku.VK_INDEX_TYPE_UINT16)
 *     vku.cbuff:bind_desc_set(vku.VK_PIPELINE_BIND_POINT_GRAPHICS, vku.pl, vku.desc_set)
 *     vku.cbuff:draw_idx(vku.pl, 3)
 *     vku.cbuff:end_rpass()
 *     vku.cbuff:end()
 *
 *     vku.submit_cmdbuff({{vku.img_sem, vku.VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT}},
 *         vku.cbuff, vku.fence, {vku.draw_sem})
 *     vku.present(vku.swc, {vku.draw_sem}, img_idx)
 *
 *     vku.wait_fences({vku.fence})
 *     vku.reset_fences({vku.fence})
 * end
 *
 * function on_window_resize(w, h)
 *     vku.device_wait_handle(vku.dev)
 *     vku.window.m_width = w
 *     vku.window.m_height = h
 *     vku.window.rebuild()
 * end
 *
 * Key Points:
 * -----------
 * - Lua scripts operate on top of C++ objects; everything is managed safely via ref_t.
 * - Both static objects from YAML and dynamic objects from Lua can be manipulated similarly.
 * - Lua functions can drive the render loop, event handling, GPU resource updates, and
 *   compute shader analysis.
 * - Provides rapid prototyping and scripting capabilities while keeping Vulkan resource
 *   management safe and automatic.
 * 
 * 
 * How objects work in the configuration(YAML) file:
 * =================================================
 *
 * 1. Declaring a standalone object:
 *    An object can be defined on its own, using a tag name as its identifier:
 *
 *    ```yaml
 *     tag-name:
 *         m_type: object_type
 *         ...
 *    ```
 *
 *    In this form:
 *
 *    * The tag name (e.g., `tag-name`) is automatically treated as the object's type identifier.
 *    * The field `m_type` is required — it marks the entry as an object.
 *    * Other properties or fields may follow.
 *
 * 2. Declaring an object within another object:
 *    An object can also appear as a nested field inside another object:
 *
 *    ```yaml
 *     another-tag-name:
 *         ...
 *         m_field:
 *             tag-name:
 *                 m_type: object_type
 *                 ...
 *    ```
 *
 *    In this case:
 *
 *    * `m_field` contains an object named `tag-name`.
 *    * `tag-name` again includes an `m_type` to specify its type.
 *
 * 3. Declaring an inline (anonymous) object:
 *    You can define an object directly within a field without giving it an outer tag.
 *    However, you may optionally include an `m_tag` if you plan to reference it later:
 *
 *    ```yaml
 *     another-tag-name:
 *         ...
 *         m_field:
 *             m_type: object_type
 *             m_tag: optional-tag-name
 *    ```
 *
 *    Here:
 *
 *    * `m_type` identifies the object type.
 *    * `m_tag` (optional) assigns a reference name so this object can be reused elsewhere.
 * 
 * 
 * Object Types:
 * =============
 * 
 * Following are the objects that are part of vulkan utils (vku) and vulkan composer (vkc):
 * 
 * vku::window_t
 * -------------
 *
 * Description: Represents a GLFW window. This object manages a platform-specific window
 * and serves as the target for Vulkan rendering. It allows resizing, title changes, and
 * provides access to the underlying GLFWwindow* for integration with other libraries.
 *
 * Members:
 * - m_name: Window title.
 * - m_width, m_height: Window dimensions.
 *
 * Init: create(width, height, name)
 *   - Parameters:
 *     - width: Initial width of the window (default: 800).
 *     - height: Initial height of the window (default: 600).
 *     - name: Title of the window (default: "vku::window_name_placeholder").
 * 
 * 
 * vku::instance_t
 * ---------------
 *
 * Description: Represents a Vulkan instance. A Vulkan instance is the foundational 
 * object that initializes the Vulkan library for a specific application. It manages 
 * the connection between the application and the Vulkan runtime, and it enables 
 * creation of devices, surfaces, and other Vulkan objects. This object also supports 
 * optional debug layers for development and validation.
 *
 * Members:
 * - m_app_name: Name of the application. Used for debugging and identification.
 * - m_engine_name: Name of the engine. Used for debugging and identification.
 * - m_extensions: List of Vulkan extensions to enable on creation.
 * - m_layers: List of Vulkan layers (such as validation layers) to enable.
 *
 * Init: create(app_name, engine_name, extensions, layers)
 *   - Parameters:
 *     - app_name: Name of the application (default: "vku::app_name_placeholder").
 *     - engine_name: Name of the engine (default: "vku::engine_name_placeholder").
 *     - extensions: Vector of extension names to enable (default: { "VK_EXT_debug_utils" }).
 *     - layers: Vector of layer names to enable (default: { "VK_LAYER_KHRONOS_validation" }).
 *
 * Notes:
 * - Debug layers can be optionally enabled to catch errors and warnings during development.
 * 
 * 
 * vku::surface_t
 * --------------
 *
 * Description: Wraps a Vulkan surface. A Vulkan surface is an abstraction that allows 
 * rendering to be presented to a window system. This object manages the relationship 
 * between a Vulkan instance and a platform-specific window (GLFW in this case). 
 * It handles creation and destruction of the VkSurfaceKHR handle and ensures that 
 * the surface is properly associated with the correct window and Vulkan instance.
 *
 * Members:
 * - m_window: Reference to a window_t object. This is the window that the surface 
 *   is associated with. The surface will present images to this window.
 * - m_instance: Reference to an instance_t object. The Vulkan instance that created 
 *   and manages this surface.
 *
 * Init: create(window, instance)
 *   - Parameters:
 *     - window: A reference to a window_t object to associate the surface with.
 *     - instance: A reference to an instance_t object used to create the surface.
 *   - Returns: A reference-counted surface_t object with a valid VkSurfaceKHR handle.
 * 
 * vku::device_t
 * -------------
 *
 * Description: Represents a Vulkan logical device. This object abstracts a physical 
 * GPU and provides access to queues for graphics and presentation. It is used to 
 * create buffers, images, pipelines, and other Vulkan resources.
 *
 * Members:
 * - m_surface: Reference to a surface_t object. The surface used for presentation and 
 *   swapchain creation.
 *
 * Init: create(surface)
 *   - Parameters:
 *     - surface: A reference to a surface_t object that this device will render to.
 *
 * Notes:
 * - Automatically selects suitable graphics and presentation queues.
 * - Provides access to device-local and host-visible memory through buffer objects.
 * 
 * TODO:
 * - Add options for selecting the phys dev
 * 
 * 
 * vku::swapchain_t
 * ----------------
 *
 * Description: Wraps a Vulkan swapchain. A swapchain manages a set of images that 
 * are presented to a window in a controlled manner. This object handles creation 
 * of the VkSwapchainKHR, its images, and associated image views.
 *
 * Members:
 * - m_device: Reference to a device_t object. The device used to create and manage 
 *   the swapchain.
 * - m_depth_imag: Reference to a depth image used for depth testing (automatically created).
 * - m_depth_view: Reference to an image view for the depth image (automatically created).
 *
 * Init: create(device)
 *   - Parameters:
 *     - device: Reference to the device_t object.
 *
 * Notes:
 * - Automatically chooses the surface format, present mode, and image count.
 * - Provides access to the swapchain images and their views for rendering.
 * 
 * TODO:
 * - more init hints?
 * 
 * 
 * vku::shader_t
 * -------------
 *
 * Description: Wraps a Vulkan shader module. This object represents a compiled 
 * shader in SPIR-V format and can be used in graphics or compute pipelines. It 
 * supports initialization from a SPIR-V object or directly from a precompiled file.
 *
 * Members:
 * - m_device: Reference to the device_t object that owns this shader.
 * - m_type: Shader stage (vertex, fragment, compute, etc.).
 * - m_spirv: Reference to a spirv_t object containing the compiled SPIR-V code.
 * - m_path: Path to the shader file (used if initialized from file).
 * - m_init_from_path: Flag indicating whether the shader was initialized from a file.
 *
 * Init:
 * - create(device, spirv): Initialize from a spirv_t object.
 * - create(device, path, type): Initialize from a compiled shader file.
 *
 * Notes:
 * - For graphics pipelines, shaders must match the pipeline’s stage requirements.
 * 
 * 
 * vku::renderpass_t
 * -----------------
 *
 * Description: Wraps a Vulkan render pass. A render pass defines how framebuffer 
 * attachments are used during rendering, including their load/store operations 
 * and the subpass dependencies. This object manages the creation of a VkRenderPass 
 * for a given swapchain.
 *
 * Members:
 * - m_swapchain: Reference to a swapchain_t object. The swapchain whose images 
 *   will be rendered into using this render pass.
 *
 * Init: create(swc)
 *   - Parameters:
 *     - swc: Reference to a swapchain_t object that will provide the framebuffer images.
 *
 * Notes:
 * - Handles attachment descriptions, subpass definitions, and dependencies automatically.
 * 
 * 
 * vku::pipeline_t
 * ---------------
 *
 * Description: Wraps a Vulkan graphics pipeline. This object encapsulates the entire 
 * pipeline state, including shaders, vertex input, topology, viewport, rasterization, 
 * and descriptor set bindings. It is used for rendering commands submitted to a 
 * command buffer.
 *
 * Members:
 * - m_renderpass: Reference to a renderpass_t object. The render pass this pipeline 
 *   will be used with.
 * - m_shaders: Vector of references to shader_t objects. The shaders used in the 
 *   pipeline stages.
 * - m_topology: Primitive topology (triangle list, line list, etc.).
 * - m_input_desc: Vertex input description (binding, attributes, stride, input rate).
 * - m_bindings: Reference to a binding_desc_set_t object. Descriptor sets used 
 *   by the pipeline.
 * - m_width, m_height: Pipeline viewport dimensions.
 *
 * Init: create(width, height, renderpass, shaders, topology, input_desc, bindings)
 *   - Parameters:
 *     - width, height: Pipeline viewport dimensions.
 *     - renderpass: Reference to the renderpass_t object.
 *     - shaders: Vector of shader_t references for each stage.
 *     - topology: Primitive topology.
 *     - input_desc: Vertex input description.
 *     - bindings: Reference to binding_desc_set_t describing descriptor sets.
 *
 * TODO:
 * - maybe get rid of m_width, m_height, create new objects for viewport and stuff
 * 
 * vku::compute_pipeline_t
 * -----------------------
 *
 * Description: Wraps a Vulkan compute pipeline. This object encapsulates a compute 
 * shader and the descriptor sets it uses. It is used to dispatch compute workloads 
 * on the GPU.
 *
 * Members:
 * - m_device: Reference to a device_t object. The device that owns this compute pipeline.
 * - m_shader: Reference to a shader_t object containing the compute shader.
 * - m_bindings: Reference to a binding_desc_set_t object describing descriptor sets 
 *   used by the shader.
 *
 * Init: create(device, shader, bindings)
 *   - Parameters:
 *     - device: Reference to the device_t object.
 *     - shader: Reference to the compute shader (shader_t).
 *     - bindings: Reference to binding_desc_set_t describing descriptor sets.
 * 
 * 
 * vku::framebuffs_t
 * -----------------
 *
 * Description: Wraps Vulkan framebuffers. A framebuffer represents a collection of 
 * attachments (color, depth, etc.) used by a render pass for rendering. This object 
 * manages the creation of VkFramebuffer objects corresponding to the swapchain images.
 *
 * Members:
 * - m_renderpass: Reference to a renderpass_t object. The render pass that these 
 *   framebuffers are compatible with.
 *
 * Init: create(renderpass)
 *   - Parameters:
 *     - renderpass: Reference to the renderpass_t object these framebuffers will be used with.
 * 
 *
 * vku::cmdpool_t
 * --------------
 *
 * Description: Wraps a Vulkan command pool. A command pool manages the memory and 
 * allocation of command buffers, which record rendering and compute commands. This 
 * object simplifies creation and management of command buffers for a device.
 *
 * Members:
 * - m_device: Reference to a device_t object. The device that owns this command pool.
 *
 * Init: create(device)
 *   - Parameters:
 *     - device: Reference to the device_t object that will own this command pool.
 *
 * Notes:
 * - All command buffers allocated from this pool are implicitly associated with
 * the device’s queues.
 * 
 * 
 * vku::cmdbuff_t
 * --------------
 *
 * Description: Wraps a Vulkan command buffer. Command buffers record rendering and 
 * compute commands that are submitted to a queue for execution. This object manages 
 * allocation, recording, and submission of commands, and provides utility functions 
 * for common operations like binding vertex buffers, descriptor sets, and drawing.
 *
 * Members:
 * - m_cmdpool: Reference to a cmdpool_t object. The command pool from which this 
 *   command buffer was allocated.
 * - m_host_free: TODO explanation
 *
 * Member functions:
 * - begin(flags): Begin recording commands with specified usage flags.
 * - begin_rpass(fbs, img_idx): Begin a render pass using the specified framebuffers.
 * - bind_vert_buffs(first_bind, buffs): Bind vertex buffers.
 * - bind_idx_buff(ibuff, offset, idx_type): Bind an index buffer.
 * - bind_desc_set(bind_point, pl, desc_set): Bind a descriptor set for the pipeline.
 * - draw(pl, vert_cnt): Issue a non-indexed draw call.
 * - draw_idx(pl, vert_cnt): Issue an indexed draw call.
 * - end_rpass(): End the current render pass.
 * - end(): Finish recording commands.
 * - reset(): Reset the command buffer for reuse.
 * - bind_compute(cpl): Bind a compute pipeline.
 * - dispatch_compute(x, y, z): Dispatch compute shader workgroups.
 *
 * Init: create(cmdpool, host_free=false)
 *   - Parameters:
 *     - cmdpool: Reference to the cmdpool_t object from which this buffer will be allocated.
 *     - host_free: Optional flag indicating whether the buffer is host-allocated (default: false).
 *
 * TODO:
 * - check this description again
 * 
 * 
 * vku::sem_t
 * ----------
 *
 * Description: Wraps a Vulkan semaphore. Semaphores are synchronization primitives 
 * used to coordinate execution between command buffers and queues, and to synchronize 
 * presentation with rendering.
 *
 * Members:
 * - m_device: Reference to the device_t object that owns this semaphore.
 *
 * Init: create(device)
 *   - Parameters:
 *     - device: Reference to the device_t object that will own this semaphore.
 *
 * Notes:
 * - Typically used to signal when an image is available from the swapchain or when 
 *   rendering is complete.
 * 
 * 
 * vku::fence_t
 * ------------
 *
 * Description: Wraps a Vulkan fence. Fences are synchronization primitives used to 
 * coordinate CPU and GPU operations. They allow the host to wait for GPU execution 
 * to complete, ensuring proper synchronization between command submissions.
 *
 * Members:
 * - m_device: Reference to the device_t object that owns this fence.
 * - m_flags: Fence creation flags. These control initial fence state (e.g., signaled 
 *   or unsignaled).
 *
 * Init: create(device, flags=0)
 *   - Parameters:
 *     - device: Reference to the device_t object that will own this fence.
 *     - flags: Optional fence creation flags (default: 0).
 *
 * Notes:
 * - Fences can be waited on from the CPU to ensure GPU completion of submitted work.
 * 
 * 
 * vku::buffer_t
 * -------------
 *
 * Description: Wraps a Vulkan buffer and its associated device memory. Buffers are 
 * linear memory resources used to store vertex data, index data, uniform data, or 
 * any other structured GPU-accessible data. This object manages both the VkBuffer 
 * and its backing VkDeviceMemory, and provides helper functions for mapping and 
 * unmapping memory for CPU access.
 *
 * Members:
 * - m_device: Reference to the device_t object that owns this buffer.
 * - m_size: The total size of the buffer in bytes.
 * - m_usage_flags: Vulkan usage flags (e.g., VK_BUFFER_USAGE_VERTEX_BUFFER_BIT) that 
 *   determine how the buffer will be used by the GPU.
 * - m_sharing_mode: Vulkan sharing mode (exclusive or concurrent) that determines 
 *   how the buffer is accessed across multiple queue families.
 * - m_memory_flags: Vulkan memory property flags that specify the type of memory 
 *   allocation (e.g., host-visible, device-local, coherent).
 * - m_map_ptr: Pointer to mapped memory (valid only when the buffer is mapped).
 *
 * Member functions:
 * - map_data(offset, size): Maps the buffer's device memory to CPU-visible address space 
 *   so that data can be written directly from the host.
 * - unmap_data(): Unmaps the buffer's device memory after CPU writes are complete.
 *
 * Init: create(device, size, usage_flags, sharing_mode, memory_flags)
 *   - Parameters:
 *     - device: Reference to the device_t that will own this buffer.
 *     - size: The total size of the buffer in bytes.
 *     - usage_flags: Vulkan usage flags indicating how the buffer will be used.
 *     - sharing_mode: How the buffer is shared across queue families.
 *     - memory_flags: Memory properties for the buffer’s allocation.
 *
 * 
 * vku::image_t
 * ------------
 *
 * Description: Wraps a Vulkan image and its associated device memory. Images are
 * multidimensional resources used for textures, color attachments, depth buffers,
 * and other GPU-readable or -writable image data. This object manages the VkImage
 * handle, its VkDeviceMemory allocation, and supports layout transitions and view creation.
 *
 * Members:
 * - m_device: Reference to the device_t object that owns this image.
 * - m_width, m_height: Dimensions of the image in pixels.
 * - m_format: Vulkan format (e.g., VK_FORMAT_R8G8B8A8_SRGB) that defines the pixel layout.
 *   how the image will be used.
 * - m_usage: Vulkan usage flags (e.g., VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) indicating 
 *   how the image will be used.
 *
 * Member functions:
 * - transition_layout(cmd_buff, old_layout, new_layout): Records a layout transition
 *   command for this image, changing how the GPU interprets its contents.
 * - set_data(cmd_pool, ptr, size, cmd_buff=nullptr): Copies data from a buffer into the image.
 *
 * Init: create(device, width, height, format, usage)
 *   - Parameters:
 *     - device: Reference to the device_t that will own this image.
 *     - width, height: Dimensions of the image.
 *     - format: Image format.
 *     - usage: Usage flags describing intended image use.
 *
 * TODO:
 * - add m_tiling and m_mem_flags? 
 * 
 * vku::img_view_t
 * ---------------
 *
 * Description: Wraps a Vulkan image view. An image view defines how a Vulkan image’s
 * data can be accessed and interpreted within shaders or as attachments. This object
 * manages the VkImageView handle and ensures it is correctly associated with its
 * underlying VkImage.
 *
 * Members:
 * - m_device: Reference to the device_t object that owns this image view.
 * - m_image: Reference to the image_t object that this view is based on.
 * - m_aspect_flags: Aspect flags defining which parts of the image are accessible 
 *   (e.g., color, depth, or stencil).
 *
 * Init: create(device, image, aspect_flags)
 *   - Parameters:
 *     - device: Reference to the device_t object that owns this image view.
 *     - image: Reference to the image_t object to create a view for.
 *     - aspect_flags: Aspect flags specifying which parts of the image the view will access.
 *
 * Notes:
 * - Required for using images as color or depth attachments, or as sampled textures.
 * - The view type (1D, 2D, 3D, or cube) is inferred from the image and usage flags.
 * - Multiple views can be created from the same image to represent different mip levels
 *   or aspects.
 * 
 * 
 * vku::img_sampl_t
 * ----------------
 *
 * Description: Wraps a Vulkan sampler. A sampler defines how image data is read in shaders, 
 * including filtering, addressing modes, and mipmap behavior. This object manages the 
 * VkSampler handle and its configuration, and is used in combination with an img_view_t 
 * when binding textures to descriptor sets.
 *
 * Members:
 * - m_device: Reference to the device_t object that owns this sampler.
 * - m_filter: Filtering mode used for magnification and minification (e.g., 
 *   VK_FILTER_LINEAR, VK_FILTER_NEAREST).
 *
 * Init: create(device, filter)
 *   - Parameters:
 *     - device: Reference to the device_t object that will own this sampler.
 *     - filter: Filtering mode for magnification and minification.
 *
 * Notes:
 * - Samplers are independent of specific images and can be reused across multiple textures.
 * 
 * TODO:
 * - add m_address_mode, max_anisotropy, mipmap_mode
 * 
 * 
 * vku::binding_desc_set_t
 * -----------------------
 *
 * Description: Represents a collection of Vulkan descriptor bindings that define 
 * how GPU resources (buffers, images, samplers) are connected to shaders. 
 * This object holds a list of binding descriptors (either buffer or sampler bindings) 
 * and provides helper functions to produce Vulkan structures used during 
 * descriptor set and layout creation.
 *
 * Members:
 * - m_binds: Vector of binding_desc_t objects, each describing a single resource 
 *   binding (uniform buffer, storage buffer, or combined image sampler).
 *
 * Nested types:
 * - binding_desc_t: Abstract base class for a descriptor binding. 
 *   Defines a common interface for obtaining a VkWriteDescriptorSet structure.
 * - buff_binding_t: Represents a buffer descriptor binding. Holds a reference 
 *   to a buffer_t and its associated VkDescriptorBufferInfo.
 * - sampl_binding_t: Represents a combined image sampler binding. Holds references 
 *   to an img_view_t and an img_sampl_t, as well as the associated VkDescriptorImageInfo.
 *
 * Member functions:
 * - get_writes(): Returns a vector of VkWriteDescriptorSet structures for updating 
 *   descriptor sets.
 * - get_descriptors(): Returns a vector of VkDescriptorSetLayoutBinding structures 
 *   describing all bindings for layout creation.
 *
 * Init: create(binds)
 *   - Parameters:
 *     - binds: Vector of binding_desc_t references (each created using buff_binding_t::create 
 *       or sampl_binding_t::create).
 *
 * Notes:
 * - This object is used by desc_pool_t and desc_set_t to create and populate 
 *   Vulkan descriptor sets.
 * 
 * 
 * vku::binding_desc_set_t::buff_binding_t
 * ---------------------------------------
 *
 * Description: Represents a buffer binding within a Vulkan descriptor set. 
 * This binding connects a GPU buffer (uniform or storage) to a shader stage. 
 * It wraps a buffer_t reference and the corresponding VkDescriptorBufferInfo needed 
 * for descriptor updates.
 *
 * Members:
 * - m_buffer: Reference to a buffer_t object containing the GPU buffer.
 *
 * Member functions:
 * - get_write(): Returns a VkWriteDescriptorSet structure suitable for updating 
 *   a descriptor set with this buffer binding.
 *
 * Init: create(desc, buffer)
 *   - Parameters:
 *     - desc: VkDescriptorSetLayoutBinding describing this binding (binding index, 
 *       descriptor type, stage flags, etc.).
 *     - buffer: Reference to a buffer_t object to bind.
 *
 * Notes:
 * - Typically used for uniform buffers or storage buffers in graphics or compute pipelines.
 * 
 * 
 * vku::binding_desc_set_t::sampl_binding_t
 * ----------------------------------------
 *
 * Description: Represents a combined image sampler binding within a Vulkan descriptor set. 
 * This binding connects a GPU image (via img_view_t) and a sampler (img_sampl_t) 
 * to a shader stage, allowing shaders to sample textures.
 *
 * Members:
 * - m_view: Reference to an img_view_t object representing the image to be sampled.
 * - m_sampler: Reference to an img_sampl_t object representing the sampler used for filtering.
 *
 * Member functions:
 * - get_write(): Returns a VkWriteDescriptorSet structure suitable for updating 
 *   a descriptor set with this image-sampler binding.
 *
 * Init: create(desc, view, sampler)
 *   - Parameters:
 *     - desc: VkDescriptorSetLayoutBinding describing this binding (binding index, 
 *       descriptor type, stage flags, etc.).
 *     - view: Reference to an img_view_t object to bind.
 *     - sampler: Reference to an img_sampl_t object to bind.
 *
 * 

 * 
 * 

 * 
 * 
 * vkc::desc_pool_t
 * ----------------
 *
 * Description: Represents a Vulkan descriptor pool. A descriptor pool allocates 
 * and manages descriptor sets, which are used to bind resources (buffers, 
 * images, samplers) to shaders. This object wraps the VkDescriptorPool handle 
 * and tracks associated bindings and allocation count.
 *
 * Members:
 * - m_device: Reference to the device_t object that owns this pool.
 * - m_bindings: Reference to a binding_desc_set_t object describing the types 
 *   of bindings this pool can allocate.
 * - m_cnt: Number of descriptor sets this pool can allocate.
 * - vk_descpool: Vulkan descriptor pool handle.
 *
 *
 * Init: create(dev, bindings, cnt)
 *   - Parameters:
 *     - dev: Reference to the device_t object used to create the pool.
 *     - bindings: Reference to a binding_desc_set_t describing the binding types.
 *     - cnt: Maximum number of descriptor sets that can be allocated from this pool.
 * 
 * 
 * vkc::desc_set_t
 * ---------------
 *
 * Description: Represents a Vulkan descriptor set. Descriptor sets are allocated 
 * from a descriptor pool and define how resources (buffers, images, samplers) 
 * are bound to shaders in a pipeline. This object wraps the VkDescriptorSet handle 
 * and tracks the pool, pipeline, and bindings associated with the set.
 *
 * Members:
 * - m_descriptor_pool: Reference to the desc_pool_t object that allocated this set.
 * - m_pipeline: Reference to the pipeline_t object using this descriptor set.
 * - m_bindings: Reference to a binding_desc_set_t object describing the resources 
 *   bound to this descriptor set.
 * - vk_desc_set: Vulkan descriptor set handle.
 *
 * Member functions:
 * - update(): Updates the GPU descriptor set with the current resources from the
 *   associated binding_desc_set_t. Should be called after changing any buffers, images,
 *   or samplers in the bindings. This function must be called before binding the descriptor set in
 *   a command buffer if any resources have changed.
 *
 * Init: create(dp, pl, bindings)
 *   - Parameters:
 *     - dp: Reference to the desc_pool_t to allocate the descriptor set from.
 *     - pl: Reference to the pipeline_t that will use this descriptor set.
 *     - bindings: Reference to a binding_desc_set_t describing the bindings for this set.
 * 
 * Notes:
 * - desc_set_t represents an actual Vulkan descriptor set allocated from a descriptor pool.
 *   It implements the resources described by a binding_desc_set_t. While binding_desc_set_t
 *   defines the layout and points to the specific resources (buffers, images, samplers),
 *   desc_set_t is the concrete GPU object that can be bound in a pipeline. Multiple 
 *   desc_set_t instances can share the same binding_desc_set_t layout.
 * - Buffers and samplers are stored in the binding_desc_set_t bindings. To change the
 *   resource used by a desc_set_t, modify the resource in the corresponding 
 *   binding_desc_set_t::binding_desc_t and call update() on the desc_set_t. Alternatively,
 *   calling update() on the binding_desc_set_t itself also works, as desc_set_t depends
 *   on it and will propagate the changes automatically.
 */

/*! @file This file will be used to auto-initialize Vulkan. This file will describe the structure
 * of the Vulkan objects and create an vulkan_obj_t with the specific type and an associated name.
 * In this way you can configure all the semaphores and buffers from here and ask for them inside
 * the program. The objective is to have the entire Vulkan pipeline described from outside of the
 * source code. */

/* TODO: figure out what to do with this: (The LUA_IMPL part, I think most of the things in
vulkan_composer would stay better in a CPP file) */
#include "vulkan_utils.h"
#include "yaml.h"
#include "tinyexpr.h"
#include "minilua.h"
#include "demangle.h"
#include "co_utils.h"

#include <filesystem>

/* TODO: Thsi is almost done now, I must think what I want to further expose, and some tweaks:
    - I want to be able to add objects from the outside, so those need to be:
        1. added to build_object, such that those will be created from yaml/lua
        2. added to lua
        3. integrated with vku::ref_t
    - I want to be able to add more enums to lua
    - I want to be able to add more functions to lua (maybe add something to identify builtins)
    - I still need to create matrices and vectors
    - I want to have includes for shaders
    - I want this header to be split into some objects because it takes too much to compile and
    there is no real point of having the composer in a single file 
    - Add functions to manipulate buffers, matrices and vectors
    ~ We need to fix the stupidity that is descriptors?
 */

enum vkc_error_e : int32_t {
    VKC_ERROR_OK = 0,
    VKC_ERROR_GENERIC = -1, 
    VKC_ERROR_PARSE_YAML = -2,
    VKC_ERROR_LUA_CALL = -3,

    VKC_ERROR_FAILED_CALL = -4,
    VKC_ERROR_FAILED_LOAD = -5,
    VKC_ERROR_FAILED_LUA_INIT = -6,
    VKC_ERROR_FAILED_LUA_LOAD = -7,
    VKC_ERROR_FAILED_LUA_EXEC = -8,
};

namespace virt_composer {

extern inline std::unordered_map<std::string, VkBufferUsageFlagBits>
        vk_buffer_usage_flag_bits_from_str;

extern inline std::unordered_map<std::string, VkSharingMode>
        vk_sharing_mode_from_str;

extern inline std::unordered_map<std::string, VkMemoryPropertyFlagBits>
        vk_memory_property_flag_bits_from_str;

extern inline std::unordered_map<std::string, VkPrimitiveTopology>
        vk_primitive_topology_from_str;

extern inline std::unordered_map<std::string, VkImageAspectFlagBits>
        vk_image_aspect_flag_bits_from_str;

extern inline std::unordered_map<std::string, VkCommandBufferUsageFlagBits>
        vk_command_buffer_usage_flag_bits_from_str;

extern inline std::unordered_map<std::string, VkPipelineBindPoint>
        vk_pipeline_bind_point_from_str;

extern inline std::unordered_map<std::string, VkIndexType>
        vk_index_type_from_str;

extern inline std::unordered_map<std::string, VkPipelineStageFlagBits>
        vk_pipeline_stage_flag_bits_from_str;

extern inline std::unordered_map<std::string, VkFormat>
        vk_format_from_str;

extern inline std::unordered_map<std::string, VkVertexInputRate>
        vk_vertex_input_rate_from_str;

extern inline std::unordered_map<std::string, VkShaderStageFlagBits>
        vk_shader_stage_flag_bits_from_str;

extern inline std::unordered_map<std::string, VkDescriptorType>
        vk_descriptor_type_from_str;

extern inline std::unordered_map<std::string, vku_shader_stage_e>
        shader_stage_from_string;

template <> inline VkBufferUsageFlagBits get_enum_val<VkBufferUsageFlagBits>(fkyaml::node &n);
template <> inline VkSharingMode get_enum_val<VkSharingMode>(fkyaml::node &n);
template <> inline VkMemoryPropertyFlagBits get_enum_val<VkMemoryPropertyFlagBits>(fkyaml::node &n);
template <> inline VkPrimitiveTopology get_enum_val<VkPrimitiveTopology>(fkyaml::node &n);
template <> inline VkImageAspectFlagBits get_enum_val<VkImageAspectFlagBits>(fkyaml::node &n);
template <> inline VkCommandBufferUsageFlagBits get_enum_val<VkCommandBufferUsageFlagBits>(
        fkyaml::node &n);
template <> inline VkPipelineBindPoint get_enum_val<VkPipelineBindPoint>(fkyaml::node &n);
template <> inline VkIndexType get_enum_val<VkIndexType>(fkyaml::node &n);
template <> inline VkPipelineStageFlagBits get_enum_val<VkPipelineStageFlagBits>(fkyaml::node &n);
template <> inline VkFormat get_enum_val<VkFormat>(fkyaml::node &n);
template <> inline VkVertexInputRate get_enum_val<VkVertexInputRate>(fkyaml::node &n);
template <> inline VkShaderStageFlagBits get_enum_val<VkShaderStageFlagBits>(fkyaml::node &n);
template <> inline VkDescriptorType get_enum_val<VkDescriptorType>(fkyaml::node &n);
template <> inline vku_shader_stage_e get_enum_val<vku_shader_stage_e>(fkyaml::node &n);

} /* virt_composer */

namespace vulkan_composer {

namespace vo = virt_object;
namespace vku = vulkan_utils;
namespace vc = virt_composer;
namespace vkc = vulkan_composer;

VIRT_COMPOSER_REGISTER_TYPE(VKC_TYPE_SPIRV);
VIRT_COMPOSER_REGISTER_TYPE(VKC_TYPE_CPU_BUFFER);
VIRT_COMPOSER_REGISTER_TYPE(VKC_TYPE_LUA_VARIABLE);
VIRT_COMPOSER_REGISTER_TYPE(VKC_TYPE_VERTEX_INPUT_DESC);
VIRT_COMPOSER_REGISTER_TYPE(VKC_TYPE_BINDING_DESC);

inline std::string app_path = std::filesystem::canonical("./");
vc::ref_t<vku::image_t> load_image(vc::ref_t<vku::cmdpool_t> cp, std::string path);

/* Does this really have any irl usage? */
struct lua_var_t : public vku::object_t {
    std::string name;

    static vku::object_type_e type_id_static() { return VKC_TYPE_LUA_VARIABLE; }
    static vku::ref_t<lua_var_t> create(std::string name) {
        auto ret = vku::ref_t<lua_var_t>::create_obj_ref(std::make_unique<lua_var_t>(), {});
        ret->name = name;
        return ret;
    }

    virtual vku::object_type_e type_id() const override { return VKC_TYPE_LUA_VARIABLE; }

    inline std::string to_string() const override {
        return std::format("vkc::lua_var[{}]: m_name={} ", (void*)this, name);
    }


private:
    virtual vc::ret_t _init() override { return VK_SUCCESS; }
    virtual vc::ret_t _uninit() override { return VK_SUCCESS; }
};

/*!
 * 
 * vkc::cpu_buffer_t
 * -----------------
 *
 * Description: Represents a CPU-side memory buffer. Can be used for passing data between Lua
 * scripts and C++ callbacks, or for staging data to  and from Vulkan buffers. Essentially,
 * this object wraps a byte array.
 *
 * Member functions:
 * - data(): Returns a pointer to the buffer data.
 * - size(): Returns the size of the buffer.
 *
 * Init: create(sz)
 *   - Parameters:
 *     - sz: Initial size of the buffer in bytes
 * 
 */
struct cpu_buffer_t : public vku::object_t {
    static vku::object_type_e type_id_static() { return VKC_TYPE_CPU_BUFFER; }
    static vku::ref_t<cpu_buffer_t> create(size_t sz) {
        auto ret = vku::ref_t<cpu_buffer_t>::create_obj_ref(std::make_unique<cpu_buffer_t>(), {});
        ret->_data.resize(sz);
        return ret;
    }

    void *data() { return (void *)_data.data(); }
    size_t size() const { return _data.size(); }

    virtual vku::object_type_e type_id() const override { return VKC_TYPE_CPU_BUFFER; }

    inline std::string to_string() const override {
        return std::format("vkc::cpu_buffer_t[{}]: size={} ", (void*)this, size());
    }

private:
    virtual vc::ret_t _init() override { return VK_SUCCESS; }
    virtual vc::ret_t _uninit() override { return VK_SUCCESS; }

    std::vector<uint8_t> _data;
};

/*!
 * 
 * vkc::spirv_t
 * ------------
 * 
 * Description: Wraps a SPIR-V shader module representation. Stores the SPIR-V bytecode 
 * along with metadata (type, stage, etc.) and allows it to be passed around in the Vulkan 
 * framework or used for shader module creation.
 *
 * Members:
 * - spirv: The SPIR-V representation, including type and bytecode content.
 *
 * Member functions:
 * - to_string(): Returns a formatted string showing the SPIR-V type and a hex dump of the content.
 *
 * TODO: fix: Init: create(spirv)
 *   - Parameters:
 *     - spirv: The SPIR-V object containing bytecode and type information.
 * 
 */
struct spirv_t : public vku::object_t {
    vku::spirv_t spirv;

    static vku::object_type_e type_id_static() { return VKC_TYPE_SPIRV; }
    static vku::ref_t<spirv_t> create(const vku::spirv_t& spirv) {
        auto ret = vku::ref_t<spirv_t>::create_obj_ref(std::make_unique<spirv_t>(), {});
        ret->spirv = spirv;
        return ret;
    }

    virtual vku::object_type_e type_id() const override { return VKC_TYPE_SPIRV; }

    inline std::string to_string() const override {
        return std::format("vkc::spirv[{}]: spirv-type={} spirv-content=\n{}", (void*)this,
                vulkan_utils::to_string(spirv.type),
                hexdump_str((void *)spirv.content.data(), spirv.content.size() * sizeof(uint32_t)));
    }

private:
    virtual vc::ret_t _init() override { return VK_SUCCESS; }
    virtual vc::ret_t _uninit() override { return VK_SUCCESS; }
};

/*!
 * 
 * vkc::vertex_input_desc_t
 * ------------------------
 *
 * Description: Represents a Vulkan vertex input description, including the binding 
 * and attribute layouts. Wraps a vku::vertex_input_desc_t structure containing the 
 * binding description (stride, input rate) and attribute descriptions (format, 
 * location, offset, etc.).
 *
 * Member functions:
 * - to_string(): Returns a formatted string showing the binding, stride, input rate, 
 *   and attribute descriptions.
 *
 * TODO: more clear: Init: create(vid)
 *   - Parameters:
 *     - vid: A vku::vertex_input_desc_t object describing the vertex input layout.
 *
 * Notes:
 * - Serves as a reusable object to describe vertex buffer layout for pipeline creation.
 * 
 */
struct vertex_input_desc_t : public vku::object_t {
    vku::vertex_input_desc_t vid;

    static vku::object_type_e type_id_static() { return VKC_TYPE_VERTEX_INPUT_DESC; }
    static vku::ref_t<vertex_input_desc_t> create(const vku::vertex_input_desc_t& vid) {
        auto ret = vku::ref_t<vertex_input_desc_t>::create_obj_ref(
                std::make_unique<vertex_input_desc_t>(), {});
        ret->vid = vid;
        return ret;
    }

    virtual vku::object_type_e type_id() const override { return VKC_TYPE_VERTEX_INPUT_DESC; }

    inline std::string to_string() const override {
        std::string ret = std::format("[binding={}, stride={}, in_rate={}]{{",
                vid.bind_desc.binding, vid.bind_desc.stride,
                vku::to_string(vid.bind_desc.inputRate));
        for (auto &attr : vid.attr_desc)
            ret += std::format("[loc={} bind={} fmt={} off=],", attr.location, attr.binding,
                    vku::to_string(attr.format), attr.offset);
        ret += "}}";
        return ret;
    }

private:
    virtual vc::ret_t _init() override { return VK_SUCCESS; }
    virtual vc::ret_t _uninit() override { return VK_SUCCESS; }
};

/*!
 * 
 * vkc::binding_t
 * --------------
 *
 * Description: Represents a generic descriptor set binding in Vulkan. 
 * This object wraps a VkDescriptorSetLayoutBinding structure, which defines 
 * the binding index, descriptor type, number of descriptors, and shader stage flags.
 *
 * Init: create(bd)
 *   - Parameters:
 *     - bd: A VkDescriptorSetLayoutBinding structure describing the binding.
 *
 * Notes:
 * - Serves as a base or standalone representation for descriptor set bindings.
 * - Can be used to initialize more specific binding types such as buffer or 
 *   sampler bindings.
 * 
 */
struct binding_t : public vku::object_t {
    VkDescriptorSetLayoutBinding bd;

    static vku::object_type_e type_id_static() { return VKC_TYPE_BINDING_DESC; }
    static vku::ref_t<binding_t> create(const VkDescriptorSetLayoutBinding& bd) {
        auto ret = vku::ref_t<binding_t>::create_obj_ref(
                std::make_unique<binding_t>(), {});
        ret->bd = bd;
        return ret;
    }

    virtual vku::object_type_e type_id() const override { return VKC_TYPE_BINDING_DESC; }

    inline std::string to_string() const override {
        return std::format("[binding={}, type={}, stage={}]",
                bd.binding,
                vku::to_string(bd.descriptorType),
                vku::to_string((VkShaderStageFlagBits)bd.stageFlags));
    }

private:
    virtual vc::ret_t _init() override { return VK_SUCCESS; }
    virtual vc::ret_t _uninit() override { return VK_SUCCESS; }
};

/*! IMPLEMENTATION
 ***************************************************************************************************
 ***************************************************************************************************
 ***************************************************************************************************
 */

inline bool starts_with(const std::string& a, const std::string& b) {
    return a.size() >= b.size() && a.compare(0, b.size(), b) == 0;
}

template <typename T, typename K>
constexpr auto has(T&& data_struct, K&& key) {
    return std::forward<T>(data_struct).find(std::forward<K>(key))
            != std::forward<T>(data_struct).end();
}

inline auto get_from_map(auto &m, const std::string& str) {
    if (!vkc::has(m, str))
        throw vc::except_t(std::format("Failed to get object: {} from: {}",
                str, demangle<decltype(m), 2>()));
    return m[str];
}

inline std::string get_file_string_content(const std::string& file_path_relative) {
    std::string file_path = std::filesystem::canonical(file_path_relative);

    if (!starts_with(file_path, app_path)) {
        DBG("The path is restricted to the application main directory");
        throw vc::except_t(std::format("File_error [{} vs {}]", file_path, app_path));
    }

    std::ifstream ifs(file_path.c_str());

    if (!ifs.good()) {
        DBG("Failed to open path: %s", file_path.c_str());
        throw std::runtime_error("File_error");
    }

    return std::string((std::istreambuf_iterator<char>(ifs)),
                       (std::istreambuf_iterator<char>()));
}

inline bool build_pseudo_object_match(fkyaml::node& node) {
    if (node.is_mapping() && node.contains("m_shader_type")) {
        return true;
    }
    return false;
}

inline co::task_t build_pseudo_object_cbk(vc::virt_state_t *vs, const std::string& name,
        fkyaml::node& node)
{
    if (node.is_mapping() && node.contains("m_shader_type")) {
        vku::spirv_t spirv;

        if (node.contains("m_source")) {
            spirv = vku::spirv_compile(
                    get_from_map(vc::shader_stage_from_string, node["m_shader_type"].as_str()),
                    node["m_source"].as_str().c_str());
        }

        if (node.contains("m_source_path")) {
            if (spirv.content.size()) {
                DBG("Trying to initialize spirv from 2 sources (only one of source, "
                        "source-path, or spirv-path allowed)");
                co_return -1;
            }
            spirv = vku::spirv_compile(
                    get_from_map(vc::shader_stage_from_string, node["m_shader_type"].as_str()),
                    get_file_string_content(node["m_source_path"].as_str()).c_str());
        }

        if (node.contains("m_spirv_path")) {
            if (spirv.content.size()) {
                DBG("Trying to initialize spirv from 2 sources (only one of source, "
                        "source-path, or spirv-path allowed)");
                co_return -1;
            }

            spirv.type = get_from_map(vc::shader_stage_from_string, node["m_shader_type"].as_str());
            std::string file_path = std::filesystem::canonical(node["m_spirv_path"].as_str());

            if (!starts_with(file_path, app_path)) {
                DBG("The path is restricted to the application main directory");
                co_return 0;
            }

            std::ifstream file(file_path.c_str(), std::ios::binary | std::ios::ate);
            std::streamsize size = file.tellg();

            if (size % sizeof(uint32_t) != 0) {
                DBG("File must be a shader, so it must have it's data multiple of %zu",
                        sizeof(uint32_t));
                co_return 0;
            }

            file.seekg(0, std::ios::beg);
            spirv.content.resize(size / sizeof(uint32_t));
            if (!file.read((char *)spirv.content.data(), size)) {
                DBG("Failed to read shader data");
                co_return -1;
            }
        }
        
        if (!spirv.content.size()) {
            DBG("Spirv shader can't be empty!")
            co_return -1;
        }

        auto obj = spirv_t::create(spirv);
        mark_dependency_solved(vs, name, obj.to_related<vku::object_t>());

        co_return 0;
    }
    DBG("Failed to build anything from this object[%s], so the object is invalid", name.c_str());
    co_return -1;
}

inline void glfw_pool_events() {
    glfwPollEvents();
}

inline uint32_t glfw_get_key(vku::ref_t<vku::window_t> window, uint32_t key) {
    if (!window)
        throw vc::except_t("Window parameter can't be null");
    return glfwGetKey(window->get_window(), key);
}

inline void internal_signal_close() {
    /* TODO: set loop closed */
    DBG("TODO: set loop closed");
}

inline uint32_t internal_aquire_next_img(
        vku::ref_t<vku::swapchain_t> swc, vku::ref_t<vku::sem_t> sem)
{
    uint32_t ret;
    vku::aquire_next_img(swc, sem, &ret);
    return ret;
}

inline uint32_t internal_device_wait_handle(vku::ref_t<vku::device_t> dev) {
    return vkDeviceWaitIdle(dev->vk_dev);
}

inline int copy_from_cpu_to_gpu(vku::ref_t<vku::buffer_t> dst, void *src,
        size_t len, size_t off)
{
    (void)dst, void(src), void(len), void(off);
    DBG("TODO: transfer cpu->gpu");
    return 0;
}

inline int copy_from_gpu_to_cpu(void *dst, vku::ref_t<vku::buffer_t> src,
        size_t len, size_t off)
{
    (void)dst, void(src), void(len), void(off);
    DBG("TODO: transfer gpu->cpu");
    return 0;
}

inline void luaw_set_glfw_fields(vc::virt_state_t *vs);

inline int register_meta(vc::virt_state_t *vs) {
    DBG_SCOPE();

    std::vector<luaL_Reg> vku_tab_funcs = {
        {"glfw_pool_events",    vc::luaw_function_wrapper<glfw_pool_events>},
        {"get_key",             vc::luaw_function_wrapper<glfw_get_key,
                vc::ref_t<vku::window_t>, uint32_t>},
        {"signal_close",        vc::luaw_function_wrapper<internal_signal_close>},
        {"aquire_next_img",     vc::luaw_function_wrapper<internal_aquire_next_img,
                vc::ref_t<vku::swapchain_t>, vc::ref_t<vku::sem_t>>},
        {"submit_cmdbuff",      vc::luaw_function_wrapper<vku::submit_cmdbuff,
                std::vector<std::pair<vc::ref_t<vku::sem_t>, vc::bm_t<VkPipelineStageFlagBits>>>,
                vc::ref_t<vku::cmdbuff_t>, vc::ref_t<vku::fence_t>,
                std::vector<vc::ref_t<vku::sem_t>>>},
        {"present",             vc::luaw_function_wrapper<vku::present,
                vc::ref_t<vku::swapchain_t>,
                std::vector<vc::ref_t<vku::sem_t>>,
                uint32_t>},
        {"wait_fences",         vc::luaw_function_wrapper<vku::wait_fences,
                std::vector<vc::ref_t<vku::fence_t>>>},
        {"reset_fences",        vc::luaw_function_wrapper<vku::reset_fences,
                std::vector<vc::ref_t<vku::fence_t>>>},
        {"device_wait_handle",  vc::luaw_function_wrapper<internal_device_wait_handle,
                vc::ref_t<vku::device_t>>},
        {"copy_from_cpu_to_gpu",vc::luaw_function_wrapper<copy_from_cpu_to_gpu,
                vc::ref_t<vku::buffer_t>, void *, size_t, size_t>},
        {"copy_from_gpu_to_cpu",vc::luaw_function_wrapper<copy_from_gpu_to_cpu,
                void *, vc::ref_t<vku::buffer_t>, size_t, size_t>},
    };
    ASSERT_FN(add_lua_tab_funcs(vs, vku_tab_funcs));

    /* TODO: register object structures */
    // /* vku::window_t
    // ----------------------------------------------------------------------------------------- */

    VC_REGISTER_MEMBER_OBJECT(vs, vku::window_t, m_name);
    VC_REGISTER_MEMBER_OBJECT(vs, vku::window_t, m_width);
    VC_REGISTER_MEMBER_OBJECT(vs, vku::window_t, m_height);

    // /* vku::instance_t
    // ----------------------------------------------------------------------------------------- */

    VC_REGISTER_MEMBER_OBJECT(vs, vku::instance_t, m_app_name);
    VC_REGISTER_MEMBER_OBJECT(vs, vku::instance_t, m_engine_name);
    VC_REGISTER_MEMBER_OBJECT(vs, vku::instance_t, m_extensions);
    VC_REGISTER_MEMBER_OBJECT(vs, vku::instance_t, m_layers);

    // /* vku::cmdbuff_t
    // ----------------------------------------------------------------------------------------- */

    VC_REGISTER_MEMBER_FUNCTION(vs, vku::cmdbuff_t, begin, vc::bm_t<VkCommandBufferUsageFlagBits>);
    VC_REGISTER_MEMBER_FUNCTION(vs, vku::cmdbuff_t, begin_rpass, vku::ref_t<vku::framebuffs_t>, uint32_t);
    VC_REGISTER_MEMBER_FUNCTION(vs, vku::cmdbuff_t, bind_vert_buffs,
            uint32_t, std::vector<std::pair<vku::ref_t<vku::buffer_t>, VkDeviceSize>>);
    VC_REGISTER_MEMBER_FUNCTION(vs, vku::cmdbuff_t, bind_desc_set,
            vc::bm_t<VkPipelineBindPoint>, vku::ref_t<vku::pipeline_t>, vku::ref_t<vku::desc_set_t>);
    VC_REGISTER_MEMBER_FUNCTION(vs, vku::cmdbuff_t, bind_idx_buff,
            vc::ref_t<vku::buffer_t>, uint64_t, vc::bm_t<VkIndexType>);
    VC_REGISTER_MEMBER_FUNCTION(vs, vku::cmdbuff_t, draw, vku::ref_t<vku::pipeline_t>, uint64_t);
    VC_REGISTER_MEMBER_FUNCTION(vs, vku::cmdbuff_t, draw_idx, vku::ref_t<vku::pipeline_t>, uint64_t);
    VC_REGISTER_MEMBER_FUNCTION(vs, vku::cmdbuff_t, end_rpass);
    VC_REGISTER_MEMBER_FUNCTION(vs, vku::cmdbuff_t, end);
    vc::luaw_register_member_function<vku::cmdbuff_t, &vku::cmdbuff_t::end>(vs, "end_begin");
    VC_REGISTER_MEMBER_FUNCTION(vs, vku::cmdbuff_t, reset);
    VC_REGISTER_MEMBER_FUNCTION(vs, vku::cmdbuff_t, bind_compute, vku::ref_t<vku::compute_pipeline_t>);
    VC_REGISTER_MEMBER_FUNCTION(vs, vku::cmdbuff_t, dispatch_compute, uint32_t, uint32_t, uint32_t);

    // /* vkc::cpu_buffer_t
    // ----------------------------------------------------------------------------------------- */
    VC_REGISTER_MEMBER_FUNCTION(vs, vkc::cpu_buffer_t, data);
    VC_REGISTER_MEMBER_FUNCTION(vs, vkc::cpu_buffer_t, size);

    /* Done objects
    ----------------------------------------------------------------------------------------- */

    /* TODO: Register/Fix all the above builders */
    /* TODO: Registers vulkan enums in the lua library */
    vc::add_lua_flag_mapping(vs, vc::vk_format_from_str);
    vc::add_lua_flag_mapping(vs, vc::vk_vertex_input_rate_from_str);
    vc::add_lua_flag_mapping(vs, vc::vk_shader_stage_flag_bits_from_str);
    vc::add_lua_flag_mapping(vs, vc::vk_descriptor_type_from_str);
    vc::add_lua_flag_mapping(vs, vc::vk_pipeline_stage_flag_bits_from_str);
    vc::add_lua_flag_mapping(vs, vc::vk_index_type_from_str);
    vc::add_lua_flag_mapping(vs, vc::vk_pipeline_bind_point_from_str);
    vc::add_lua_flag_mapping(vs, vc::vk_command_buffer_usage_flag_bits_from_str);
    vc::add_lua_flag_mapping(vs, vc::vk_image_aspect_flag_bits_from_str);
    vc::add_lua_flag_mapping(vs, vc::vk_primitive_topology_from_str);
    vc::add_lua_flag_mapping(vs, vc::vk_memory_property_flag_bits_from_str);
    vc::add_lua_flag_mapping(vs, vc::vk_sharing_mode_from_str);
    vc::add_lua_flag_mapping(vs, vc::vk_buffer_usage_flag_bits_from_str);
    vc::add_lua_flag_mapping(vs, vc::shader_stage_from_string);
    luaw_set_glfw_fields(vs);

    auto ret = add_named_builder_callback(vs,
        "vkc::vertex_input_desc_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            std::vector<VkVertexInputAttributeDescription> attrs;
            for (auto attr : node["m_attrs"].as_seq()) {
                auto m_location = co_await resolve_int(vs, attr["m_location"]);
                auto m_binding = co_await resolve_int(vs, attr["m_binding"]);
                auto m_format = vc::get_enum_val<VkFormat>(attr["m_format"]);
                auto m_offset = co_await resolve_int(vs, attr["m_offset"]);
                attrs.push_back(VkVertexInputAttributeDescription{
                    .location = (uint32_t)m_location,
                    .binding = (uint32_t)m_binding,
                    .format = m_format,
                    .offset = (uint32_t)m_offset
                });
            }
            auto m_binding = co_await resolve_int(vs, node["m_binding"]);
            auto m_stride = co_await resolve_int(vs, node["m_stride"]);
            auto m_in_rate = vc::get_enum_val<VkVertexInputRate>(node["m_in_rate"]);
            auto obj = vertex_input_desc_t::create(vku::vertex_input_desc_t{
                .bind_desc = {
                    .binding = (uint32_t)m_binding,
                    .stride = (uint32_t)m_stride,
                    .inputRate = m_in_rate,
                },
                .attr_desc = attrs,
            });
            mark_dependency_solved(vs, node_name, obj.to_related<vku::object_t>());
            co_return obj.to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    ret = add_named_builder_callback(vs,
        "vkc::binding_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            auto m_binding = co_await resolve_int(vs, node["m_binding"]);
            auto m_stage = vc::get_enum_val<VkShaderStageFlagBits>(node["m_stage"]);
            auto m_desc_type = vc::get_enum_val<VkDescriptorType>(node["m_desc_type"]);
            auto obj = binding_t::create(VkDescriptorSetLayoutBinding{
                .binding = (uint32_t)m_binding,
                .descriptorType = m_desc_type,
                .descriptorCount = 1,
                .stageFlags = m_stage,
                .pImmutableSamplers = nullptr
            });
            mark_dependency_solved(vs, node_name, obj.to_related<vku::object_t>());
            co_return obj.to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    ret = add_named_builder_callback(vs,
        "vkc::cpu_buffer_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            auto m_size = co_await resolve_int(vs, node["m_size"]);
            auto obj = cpu_buffer_t::create(m_size);
            mark_dependency_solved(vs, node_name, obj.to_related<vku::object_t>());
            co_return obj.to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    ret = add_named_builder_callback(vs,
        "vkc::lua_var_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        { /* TODO: not sure how is this type usefull */
            (void)node;
            /* lua_var has the same tag_name as the var name */
            auto obj = lua_var_t::create(node_name);
            mark_dependency_solved(vs, node_name, obj.to_related<vku::object_t>());
            co_return obj.to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    ret = add_named_builder_callback(vs,
        "vku::instance_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            (void)node;
            auto obj = vku::instance_t::create();
            mark_dependency_solved(vs, node_name, obj.to_related<vku::object_t>());
            co_return obj.to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    ret = add_named_builder_callback(vs,
        "vku::window_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            auto w = co_await resolve_int(vs, node["m_width"]);
            auto h = co_await resolve_int(vs, node["m_height"]);
            auto window_name = co_await resolve_str(vs, node["m_name"]);
            auto obj = vku::window_t::create(w, h, window_name);
            mark_dependency_solved(vs, node_name, obj.to_related<vku::object_t>());
            co_return obj.to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    ret = add_named_builder_callback(vs,
        "vku::surface_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            auto window = co_await resolve_obj<vku::window_t>(vs, node["m_window"]);
            auto instance = co_await resolve_obj<vku::instance_t>(vs, node["m_instance"]);
            auto obj = vku::surface_t::create(window, instance);
            mark_dependency_solved(vs, node_name, obj.to_related<vku::object_t>());
            co_return obj.to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    ret = add_named_builder_callback(vs,
        "vku::image_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            auto cp = co_await resolve_obj<vku::cmdpool_t>(vs, node["m_cmdpool"]);
            auto path = co_await resolve_str(vs, node["m_path"]);
            auto obj = load_image(cp, path);
            mark_dependency_solved(vs, node_name, obj.to_related<vku::object_t>());
            co_return obj.to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    ret = add_named_builder_callback(vs,
        "vku::cmdpool_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            auto dev = co_await resolve_obj<vku::device_t>(vs, node["m_device"]);
            auto obj = vku::cmdpool_t::create(dev);
            mark_dependency_solved(vs, node_name, obj.to_related<vku::object_t>());
            co_return obj.to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    ret = add_named_builder_callback(vs,
        "vku::device_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            auto surf = co_await resolve_obj<vku::surface_t>(vs, node["m_surface"]);
            auto obj = vku::device_t::create(surf);
            mark_dependency_solved(vs, node_name, obj.to_related<vku::object_t>());
            co_return obj.to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    ret = add_named_builder_callback(vs,
        "vku::img_sampl_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            auto dev = co_await resolve_obj<vku::device_t>(vs, node["m_device"]);
            auto obj = vku::img_sampl_t::create(dev);
            mark_dependency_solved(vs, node_name, obj.to_related<vku::object_t>());
            co_return obj.to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    ret = add_named_builder_callback(vs,
        "vku::img_view_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            auto img = co_await resolve_obj<vku::image_t>(vs, node["m_image"]);
            auto aspect_mask = vc::get_enum_val<VkImageAspectFlagBits>(node["m_aspect_mask"]);
            auto obj = vku::img_view_t::create(img, aspect_mask);
            mark_dependency_solved(vs, node_name, obj.to_related<vku::object_t>());
            co_return obj.to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);


    ret = add_named_builder_callback(vs,
        "vku::binding_desc_set_t::sampl_binding_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            auto view = co_await resolve_obj<vku::img_view_t>(vs, node["m_view"]);
            auto sampler = co_await resolve_obj<vku::img_sampl_t>(vs, node["m_sampler"]);
            auto desc = co_await resolve_obj<vkc::binding_t>(vs, node["m_desc"]);
            auto obj = vku::binding_desc_set_t::sampl_binding_t::create(desc->bd, view, sampler);
            mark_dependency_solved(vs, node_name, obj.to_related<vku::object_t>());
            co_return obj.to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    ret = add_named_builder_callback(vs,
        "vku::binding_desc_set_t::buff_binding_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            auto buff = co_await resolve_obj<vku::buffer_t>(vs, node["m_buffer"]);
            auto desc = co_await resolve_obj<vkc::binding_t>(vs, node["m_desc"]);
            auto obj = vku::binding_desc_set_t::buff_binding_t::create(desc->bd, buff);
            mark_dependency_solved(vs, node_name, obj.to_related<vku::object_t>());
            co_return obj.to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    ret = add_named_builder_callback(vs,
        "vku::binding_desc_set_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            std::vector<vku::ref_t<vku::binding_desc_set_t::binding_desc_t>> bindings;
            for (auto& subnode : node["m_descriptors"])
                bindings.push_back(
                        co_await resolve_obj<vku::binding_desc_set_t::binding_desc_t>(vs, subnode));
            auto obj = vku::binding_desc_set_t::create(bindings);
            mark_dependency_solved(vs, node_name, obj.to_related<vku::object_t>());
            co_return obj.to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    ret = add_named_builder_callback(vs,
        "vku::buffer_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            auto dev = co_await resolve_obj<vku::device_t>(vs, node["m_device"]);
            size_t sz = co_await resolve_int(vs, node["m_size"]);
            auto usage_flags = vc::get_enum_val<VkBufferUsageFlagBits>(node["m_usage_flags"]);
            auto share_mode = vc::get_enum_val<VkSharingMode>(node["m_sharing_mode"]);
            auto memory_flags = vc::get_enum_val<VkMemoryPropertyFlagBits>(node["m_memory_flags"]);
            auto obj = vku::buffer_t::create(dev, sz, usage_flags, share_mode, memory_flags);
            mark_dependency_solved(vs, node_name, obj.to_related<vku::object_t>());
            co_return obj.to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    ret = add_named_builder_callback(vs,
        "vku::pipeline_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            auto w = co_await resolve_int(vs, node["m_width"]);
            auto h = co_await resolve_int(vs, node["m_height"]);
            auto rp = co_await resolve_obj<vku::renderpass_t>(vs, node["m_renderpass"]);
            std::vector<vku::ref_t<vku::shader_t>> shaders;
            for (auto& sh : node["m_shaders"])
                shaders.push_back(co_await resolve_obj<vku::shader_t>(vs, sh));
            auto topol = vc::get_enum_val<VkPrimitiveTopology>(node["m_topology"]);
            auto indesc = co_await resolve_obj<vkc::vertex_input_desc_t>(vs, node["m_input_desc"]);
            auto binds = co_await resolve_obj<vku::binding_desc_set_t>(vs, node["m_bindings"]);
            auto obj = vku::pipeline_t::create(w, h, rp, shaders, topol, indesc->vid, binds);
            mark_dependency_solved(vs, node_name, obj.to_related<vku::object_t>());
            co_return obj.to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    ret = add_named_builder_callback(vs,
        "vku::renderpass_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            auto swc = co_await resolve_obj<vku::swapchain_t>(vs, node["m_swapchain"]);
            auto obj = vku::renderpass_t::create(swc);
            mark_dependency_solved(vs, node_name, obj.to_related<vku::object_t>());
            co_return obj.to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    ret = add_named_builder_callback(vs,
        "vku::swapchain_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            auto dev = co_await resolve_obj<vku::device_t>(vs, node["m_device"]);
            auto obj = vku::swapchain_t::create(dev);
            mark_dependency_solved(vs, node_name, obj.to_related<vku::object_t>());
            co_return obj.to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    ret = add_named_builder_callback(vs,
        "vku::shader_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            auto dev = co_await resolve_obj<vku::device_t>(vs, node["m_device"]);
            auto spirv = co_await resolve_obj<spirv_t>(vs, node["m_spirv"]);
            auto obj = vku::shader_t::create(dev, spirv->spirv);
            mark_dependency_solved(vs, node_name, obj.to_related<vku::object_t>());
            co_return obj.to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    ret = add_named_builder_callback(vs,
        "vku::fence_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            auto dev = co_await resolve_obj<vku::device_t>(vs, node["m_device"]);
            auto obj = vku::fence_t::create(dev);
            mark_dependency_solved(vs, node_name, obj.to_related<vku::object_t>());
            co_return obj.to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    ret = add_named_builder_callback(vs,
        "vku::sem_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            auto dev = co_await resolve_obj<vku::device_t>(vs, node["m_device"]);
            auto obj = vku::sem_t::create(dev);
            mark_dependency_solved(vs, node_name, obj.to_related<vku::object_t>());
            co_return obj.to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    ret = add_named_builder_callback(vs,
        "vku::framebuffs_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            auto rp = co_await resolve_obj<vku::renderpass_t>(vs, node["m_renderpass"]);
            auto obj = vku::framebuffs_t::create(rp);
            mark_dependency_solved(vs, node_name, obj.to_related<vku::object_t>());
            co_return obj.to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    ret = add_named_builder_callback(vs,
        "vku::desc_set_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            auto descriptor_pool = co_await resolve_obj<vku::desc_pool_t>(vs, node["m_descriptor_pool"]);
            auto pipeline = co_await resolve_obj<vku::pipeline_t>(vs, node["m_pipeline"]);
            auto bindings = co_await resolve_obj<vku::binding_desc_set_t>(vs, node["m_bindings"]);
            auto obj = vku::desc_set_t::create(descriptor_pool, pipeline, bindings);
            mark_dependency_solved(vs, node_name, obj.to_related<vku::object_t>());
            co_return obj.to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    ret = add_named_builder_callback(vs,
        "vku::desc_pool_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            auto dev = co_await resolve_obj<vku::device_t>(vs, node["m_device"]);
            auto binds = co_await resolve_obj<vku::binding_desc_set_t>(vs, node["m_bindings"]);
            int cnt = co_await resolve_int(vs, node["m_cnt"]);
            auto obj = vku::desc_pool_t::create(dev, binds, cnt);
            mark_dependency_solved(vs, node_name, obj.to_related<vku::object_t>());
            co_return obj.to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    ret = add_named_builder_callback(vs,
        "vku::cmdbuff_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            auto cp = co_await resolve_obj<vku::cmdpool_t>(vs, node["m_cmdpool"]);
            auto obj = vku::cmdbuff_t::create(cp);
            mark_dependency_solved(vs, node_name, obj.to_related<vku::object_t>());
            co_return obj.to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    return 0;
}

inline void luaw_set_glfw_fields(vc::virt_state_t *vs) {
    std::vector<std::pair<lua_Integer, std::string>> glfw_mapping = {
        {GLFW_VERSION_MAJOR, "GLFW_VERSION_MAJOR"},
        {GLFW_VERSION_MINOR, "GLFW_VERSION_MINOR"},
        {GLFW_VERSION_REVISION, "GLFW_VERSION_REVISION"},
        {GLFW_TRUE, "GLFW_TRUE"},
        {GLFW_FALSE, "GLFW_FALSE"},
        {GLFW_RELEASE, "GLFW_RELEASE"},
        {GLFW_PRESS, "GLFW_PRESS"},
        {GLFW_REPEAT, "GLFW_REPEAT"},
        {GLFW_HAT_CENTERED, "GLFW_HAT_CENTERED"},
        {GLFW_HAT_UP, "GLFW_HAT_UP"},
        {GLFW_HAT_RIGHT, "GLFW_HAT_RIGHT"},
        {GLFW_HAT_DOWN, "GLFW_HAT_DOWN"},
        {GLFW_HAT_LEFT, "GLFW_HAT_LEFT"},
        {GLFW_HAT_RIGHT_UP, "GLFW_HAT_RIGHT_UP"},
        {GLFW_HAT_RIGHT_DOWN, "GLFW_HAT_RIGHT_DOWN"},
        {GLFW_HAT_LEFT_UP, "GLFW_HAT_LEFT_UP"},
        {GLFW_HAT_LEFT_DOWN, "GLFW_HAT_LEFT_DOWN"},
        {GLFW_KEY_UNKNOWN, "GLFW_KEY_UNKNOWN"},
        {GLFW_KEY_SPACE, "GLFW_KEY_SPACE"},
        {GLFW_KEY_APOSTROPHE, "GLFW_KEY_APOSTROPHE"},
        {GLFW_KEY_COMMA, "GLFW_KEY_COMMA"},
        {GLFW_KEY_MINUS, "GLFW_KEY_MINUS"},
        {GLFW_KEY_PERIOD, "GLFW_KEY_PERIOD"},
        {GLFW_KEY_SLASH, "GLFW_KEY_SLASH"},
        {GLFW_KEY_0, "GLFW_KEY_0"},
        {GLFW_KEY_1, "GLFW_KEY_1"},
        {GLFW_KEY_2, "GLFW_KEY_2"},
        {GLFW_KEY_3, "GLFW_KEY_3"},
        {GLFW_KEY_4, "GLFW_KEY_4"},
        {GLFW_KEY_5, "GLFW_KEY_5"},
        {GLFW_KEY_6, "GLFW_KEY_6"},
        {GLFW_KEY_7, "GLFW_KEY_7"},
        {GLFW_KEY_8, "GLFW_KEY_8"},
        {GLFW_KEY_9, "GLFW_KEY_9"},
        {GLFW_KEY_SEMICOLON, "GLFW_KEY_SEMICOLON"},
        {GLFW_KEY_EQUAL, "GLFW_KEY_EQUAL"},
        {GLFW_KEY_A, "GLFW_KEY_A"},
        {GLFW_KEY_B, "GLFW_KEY_B"},
        {GLFW_KEY_C, "GLFW_KEY_C"},
        {GLFW_KEY_D, "GLFW_KEY_D"},
        {GLFW_KEY_E, "GLFW_KEY_E"},
        {GLFW_KEY_F, "GLFW_KEY_F"},
        {GLFW_KEY_G, "GLFW_KEY_G"},
        {GLFW_KEY_H, "GLFW_KEY_H"},
        {GLFW_KEY_I, "GLFW_KEY_I"},
        {GLFW_KEY_J, "GLFW_KEY_J"},
        {GLFW_KEY_K, "GLFW_KEY_K"},
        {GLFW_KEY_L, "GLFW_KEY_L"},
        {GLFW_KEY_M, "GLFW_KEY_M"},
        {GLFW_KEY_N, "GLFW_KEY_N"},
        {GLFW_KEY_O, "GLFW_KEY_O"},
        {GLFW_KEY_P, "GLFW_KEY_P"},
        {GLFW_KEY_Q, "GLFW_KEY_Q"},
        {GLFW_KEY_R, "GLFW_KEY_R"},
        {GLFW_KEY_S, "GLFW_KEY_S"},
        {GLFW_KEY_T, "GLFW_KEY_T"},
        {GLFW_KEY_U, "GLFW_KEY_U"},
        {GLFW_KEY_V, "GLFW_KEY_V"},
        {GLFW_KEY_W, "GLFW_KEY_W"},
        {GLFW_KEY_X, "GLFW_KEY_X"},
        {GLFW_KEY_Y, "GLFW_KEY_Y"},
        {GLFW_KEY_Z, "GLFW_KEY_Z"},
        {GLFW_KEY_LEFT_BRACKET, "GLFW_KEY_LEFT_BRACKET"},
        {GLFW_KEY_BACKSLASH, "GLFW_KEY_BACKSLASH"},
        {GLFW_KEY_RIGHT_BRACKET, "GLFW_KEY_RIGHT_BRACKET"},
        {GLFW_KEY_GRAVE_ACCENT, "GLFW_KEY_GRAVE_ACCENT"},
        {GLFW_KEY_WORLD_1, "GLFW_KEY_WORLD_1"},
        {GLFW_KEY_WORLD_2, "GLFW_KEY_WORLD_2"},
        {GLFW_KEY_ESCAPE, "GLFW_KEY_ESCAPE"},
        {GLFW_KEY_ENTER, "GLFW_KEY_ENTER"},
        {GLFW_KEY_TAB, "GLFW_KEY_TAB"},
        {GLFW_KEY_BACKSPACE, "GLFW_KEY_BACKSPACE"},
        {GLFW_KEY_INSERT, "GLFW_KEY_INSERT"},
        {GLFW_KEY_DELETE, "GLFW_KEY_DELETE"},
        {GLFW_KEY_RIGHT, "GLFW_KEY_RIGHT"},
        {GLFW_KEY_LEFT, "GLFW_KEY_LEFT"},
        {GLFW_KEY_DOWN, "GLFW_KEY_DOWN"},
        {GLFW_KEY_UP, "GLFW_KEY_UP"},
        {GLFW_KEY_PAGE_UP, "GLFW_KEY_PAGE_UP"},
        {GLFW_KEY_PAGE_DOWN, "GLFW_KEY_PAGE_DOWN"},
        {GLFW_KEY_HOME, "GLFW_KEY_HOME"},
        {GLFW_KEY_END, "GLFW_KEY_END"},
        {GLFW_KEY_CAPS_LOCK, "GLFW_KEY_CAPS_LOCK"},
        {GLFW_KEY_SCROLL_LOCK, "GLFW_KEY_SCROLL_LOCK"},
        {GLFW_KEY_NUM_LOCK, "GLFW_KEY_NUM_LOCK"},
        {GLFW_KEY_PRINT_SCREEN, "GLFW_KEY_PRINT_SCREEN"},
        {GLFW_KEY_PAUSE, "GLFW_KEY_PAUSE"},
        {GLFW_KEY_F1, "GLFW_KEY_F1"},
        {GLFW_KEY_F2, "GLFW_KEY_F2"},
        {GLFW_KEY_F3, "GLFW_KEY_F3"},
        {GLFW_KEY_F4, "GLFW_KEY_F4"},
        {GLFW_KEY_F5, "GLFW_KEY_F5"},
        {GLFW_KEY_F6, "GLFW_KEY_F6"},
        {GLFW_KEY_F7, "GLFW_KEY_F7"},
        {GLFW_KEY_F8, "GLFW_KEY_F8"},
        {GLFW_KEY_F9, "GLFW_KEY_F9"},
        {GLFW_KEY_F10, "GLFW_KEY_F10"},
        {GLFW_KEY_F11, "GLFW_KEY_F11"},
        {GLFW_KEY_F12, "GLFW_KEY_F12"},
        {GLFW_KEY_F13, "GLFW_KEY_F13"},
        {GLFW_KEY_F14, "GLFW_KEY_F14"},
        {GLFW_KEY_F15, "GLFW_KEY_F15"},
        {GLFW_KEY_F16, "GLFW_KEY_F16"},
        {GLFW_KEY_F17, "GLFW_KEY_F17"},
        {GLFW_KEY_F18, "GLFW_KEY_F18"},
        {GLFW_KEY_F19, "GLFW_KEY_F19"},
        {GLFW_KEY_F20, "GLFW_KEY_F20"},
        {GLFW_KEY_F21, "GLFW_KEY_F21"},
        {GLFW_KEY_F22, "GLFW_KEY_F22"},
        {GLFW_KEY_F23, "GLFW_KEY_F23"},
        {GLFW_KEY_F24, "GLFW_KEY_F24"},
        {GLFW_KEY_F25, "GLFW_KEY_F25"},
        {GLFW_KEY_KP_0, "GLFW_KEY_KP_0"},
        {GLFW_KEY_KP_1, "GLFW_KEY_KP_1"},
        {GLFW_KEY_KP_2, "GLFW_KEY_KP_2"},
        {GLFW_KEY_KP_3, "GLFW_KEY_KP_3"},
        {GLFW_KEY_KP_4, "GLFW_KEY_KP_4"},
        {GLFW_KEY_KP_5, "GLFW_KEY_KP_5"},
        {GLFW_KEY_KP_6, "GLFW_KEY_KP_6"},
        {GLFW_KEY_KP_7, "GLFW_KEY_KP_7"},
        {GLFW_KEY_KP_8, "GLFW_KEY_KP_8"},
        {GLFW_KEY_KP_9, "GLFW_KEY_KP_9"},
        {GLFW_KEY_KP_DECIMAL, "GLFW_KEY_KP_DECIMAL"},
        {GLFW_KEY_KP_DIVIDE, "GLFW_KEY_KP_DIVIDE"},
        {GLFW_KEY_KP_MULTIPLY, "GLFW_KEY_KP_MULTIPLY"},
        {GLFW_KEY_KP_SUBTRACT, "GLFW_KEY_KP_SUBTRACT"},
        {GLFW_KEY_KP_ADD, "GLFW_KEY_KP_ADD"},
        {GLFW_KEY_KP_ENTER, "GLFW_KEY_KP_ENTER"},
        {GLFW_KEY_KP_EQUAL, "GLFW_KEY_KP_EQUAL"},
        {GLFW_KEY_LEFT_SHIFT, "GLFW_KEY_LEFT_SHIFT"},
        {GLFW_KEY_LEFT_CONTROL, "GLFW_KEY_LEFT_CONTROL"},
        {GLFW_KEY_LEFT_ALT, "GLFW_KEY_LEFT_ALT"},
        {GLFW_KEY_LEFT_SUPER, "GLFW_KEY_LEFT_SUPER"},
        {GLFW_KEY_RIGHT_SHIFT, "GLFW_KEY_RIGHT_SHIFT"},
        {GLFW_KEY_RIGHT_CONTROL, "GLFW_KEY_RIGHT_CONTROL"},
        {GLFW_KEY_RIGHT_ALT, "GLFW_KEY_RIGHT_ALT"},
        {GLFW_KEY_RIGHT_SUPER, "GLFW_KEY_RIGHT_SUPER"},
        {GLFW_KEY_MENU, "GLFW_KEY_MENU"},
        {GLFW_KEY_LAST, "GLFW_KEY_LAST"},
        {GLFW_MOD_SHIFT, "GLFW_MOD_SHIFT"},
        {GLFW_MOD_CONTROL, "GLFW_MOD_CONTROL"},
        {GLFW_MOD_ALT, "GLFW_MOD_ALT"},
        {GLFW_MOD_SUPER, "GLFW_MOD_SUPER"},
        {GLFW_MOD_CAPS_LOCK, "GLFW_MOD_CAPS_LOCK"},
        {GLFW_MOD_NUM_LOCK, "GLFW_MOD_NUM_LOCK"},
        {GLFW_MOUSE_BUTTON_1, "GLFW_MOUSE_BUTTON_1"},
        {GLFW_MOUSE_BUTTON_2, "GLFW_MOUSE_BUTTON_2"},
        {GLFW_MOUSE_BUTTON_3, "GLFW_MOUSE_BUTTON_3"},
        {GLFW_MOUSE_BUTTON_4, "GLFW_MOUSE_BUTTON_4"},
        {GLFW_MOUSE_BUTTON_5, "GLFW_MOUSE_BUTTON_5"},
        {GLFW_MOUSE_BUTTON_6, "GLFW_MOUSE_BUTTON_6"},
        {GLFW_MOUSE_BUTTON_7, "GLFW_MOUSE_BUTTON_7"},
        {GLFW_MOUSE_BUTTON_8, "GLFW_MOUSE_BUTTON_8"},
        {GLFW_MOUSE_BUTTON_LAST, "GLFW_MOUSE_BUTTON_LAST"},
        {GLFW_MOUSE_BUTTON_LEFT, "GLFW_MOUSE_BUTTON_LEFT"},
        {GLFW_MOUSE_BUTTON_RIGHT, "GLFW_MOUSE_BUTTON_RIGHT"},
        {GLFW_MOUSE_BUTTON_MIDDLE, "GLFW_MOUSE_BUTTON_MIDDLE"},
        {GLFW_JOYSTICK_1, "GLFW_JOYSTICK_1"},
        {GLFW_JOYSTICK_2, "GLFW_JOYSTICK_2"},
        {GLFW_JOYSTICK_3, "GLFW_JOYSTICK_3"},
        {GLFW_JOYSTICK_4, "GLFW_JOYSTICK_4"},
        {GLFW_JOYSTICK_5, "GLFW_JOYSTICK_5"},
        {GLFW_JOYSTICK_6, "GLFW_JOYSTICK_6"},
        {GLFW_JOYSTICK_7, "GLFW_JOYSTICK_7"},
        {GLFW_JOYSTICK_8, "GLFW_JOYSTICK_8"},
        {GLFW_JOYSTICK_9, "GLFW_JOYSTICK_9"},
        {GLFW_JOYSTICK_10, "GLFW_JOYSTICK_10"},
        {GLFW_JOYSTICK_11, "GLFW_JOYSTICK_11"},
        {GLFW_JOYSTICK_12, "GLFW_JOYSTICK_12"},
        {GLFW_JOYSTICK_13, "GLFW_JOYSTICK_13"},
        {GLFW_JOYSTICK_14, "GLFW_JOYSTICK_14"},
        {GLFW_JOYSTICK_15, "GLFW_JOYSTICK_15"},
        {GLFW_JOYSTICK_16, "GLFW_JOYSTICK_16"},
        {GLFW_JOYSTICK_LAST, "GLFW_JOYSTICK_LAST"},
        {GLFW_GAMEPAD_BUTTON_A, "GLFW_GAMEPAD_BUTTON_A"},
        {GLFW_GAMEPAD_BUTTON_B, "GLFW_GAMEPAD_BUTTON_B"},
        {GLFW_GAMEPAD_BUTTON_X, "GLFW_GAMEPAD_BUTTON_X"},
        {GLFW_GAMEPAD_BUTTON_Y, "GLFW_GAMEPAD_BUTTON_Y"},
        {GLFW_GAMEPAD_BUTTON_LEFT_BUMPER, "GLFW_GAMEPAD_BUTTON_LEFT_BUMPER"},
        {GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER, "GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER"},
        {GLFW_GAMEPAD_BUTTON_BACK, "GLFW_GAMEPAD_BUTTON_BACK"},
        {GLFW_GAMEPAD_BUTTON_START, "GLFW_GAMEPAD_BUTTON_START"},
        {GLFW_GAMEPAD_BUTTON_GUIDE, "GLFW_GAMEPAD_BUTTON_GUIDE"},
        {GLFW_GAMEPAD_BUTTON_LEFT_THUMB, "GLFW_GAMEPAD_BUTTON_LEFT_THUMB"},
        {GLFW_GAMEPAD_BUTTON_RIGHT_THUMB, "GLFW_GAMEPAD_BUTTON_RIGHT_THUMB"},
        {GLFW_GAMEPAD_BUTTON_DPAD_UP, "GLFW_GAMEPAD_BUTTON_DPAD_UP"},
        {GLFW_GAMEPAD_BUTTON_DPAD_RIGHT, "GLFW_GAMEPAD_BUTTON_DPAD_RIGHT"},
        {GLFW_GAMEPAD_BUTTON_DPAD_DOWN, "GLFW_GAMEPAD_BUTTON_DPAD_DOWN"},
        {GLFW_GAMEPAD_BUTTON_DPAD_LEFT, "GLFW_GAMEPAD_BUTTON_DPAD_LEFT"},
        {GLFW_GAMEPAD_BUTTON_LAST, "GLFW_GAMEPAD_BUTTON_LAST"},
        {GLFW_GAMEPAD_BUTTON_CROSS, "GLFW_GAMEPAD_BUTTON_CROSS"},
        {GLFW_GAMEPAD_BUTTON_CIRCLE, "GLFW_GAMEPAD_BUTTON_CIRCLE"},
        {GLFW_GAMEPAD_BUTTON_SQUARE, "GLFW_GAMEPAD_BUTTON_SQUARE"},
        {GLFW_GAMEPAD_BUTTON_TRIANGLE, "GLFW_GAMEPAD_BUTTON_TRIANGLE"},
        {GLFW_GAMEPAD_AXIS_LEFT_X, "GLFW_GAMEPAD_AXIS_LEFT_X"},
        {GLFW_GAMEPAD_AXIS_LEFT_Y, "GLFW_GAMEPAD_AXIS_LEFT_Y"},
        {GLFW_GAMEPAD_AXIS_RIGHT_X, "GLFW_GAMEPAD_AXIS_RIGHT_X"},
        {GLFW_GAMEPAD_AXIS_RIGHT_Y, "GLFW_GAMEPAD_AXIS_RIGHT_Y"},
        {GLFW_GAMEPAD_AXIS_LEFT_TRIGGER, "GLFW_GAMEPAD_AXIS_LEFT_TRIGGER"},
        {GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER, "GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER"},
        {GLFW_GAMEPAD_AXIS_LAST, "GLFW_GAMEPAD_AXIS_LAST"},
        {GLFW_NO_ERROR, "GLFW_NO_ERROR"},
        {GLFW_NOT_INITIALIZED, "GLFW_NOT_INITIALIZED"},
        {GLFW_NO_CURRENT_CONTEXT, "GLFW_NO_CURRENT_CONTEXT"},
        {GLFW_INVALID_ENUM, "GLFW_INVALID_ENUM"},
        {GLFW_INVALID_VALUE, "GLFW_INVALID_VALUE"},
        {GLFW_OUT_OF_MEMORY, "GLFW_OUT_OF_MEMORY"},
        {GLFW_API_UNAVAILABLE, "GLFW_API_UNAVAILABLE"},
        {GLFW_VERSION_UNAVAILABLE, "GLFW_VERSION_UNAVAILABLE"},
        {GLFW_PLATFORM_ERROR, "GLFW_PLATFORM_ERROR"},
        {GLFW_FORMAT_UNAVAILABLE, "GLFW_FORMAT_UNAVAILABLE"},
        {GLFW_NO_WINDOW_CONTEXT, "GLFW_NO_WINDOW_CONTEXT"},
        {GLFW_FOCUSED, "GLFW_FOCUSED"},
        {GLFW_ICONIFIED, "GLFW_ICONIFIED"},
        {GLFW_RESIZABLE, "GLFW_RESIZABLE"},
        {GLFW_VISIBLE, "GLFW_VISIBLE"},
        {GLFW_DECORATED, "GLFW_DECORATED"},
        {GLFW_AUTO_ICONIFY, "GLFW_AUTO_ICONIFY"},
        {GLFW_FLOATING, "GLFW_FLOATING"},
        {GLFW_MAXIMIZED, "GLFW_MAXIMIZED"},
        {GLFW_CENTER_CURSOR, "GLFW_CENTER_CURSOR"},
        {GLFW_TRANSPARENT_FRAMEBUFFER, "GLFW_TRANSPARENT_FRAMEBUFFER"},
        {GLFW_HOVERED, "GLFW_HOVERED"},
        {GLFW_FOCUS_ON_SHOW, "GLFW_FOCUS_ON_SHOW"},
        {GLFW_RED_BITS, "GLFW_RED_BITS"},
        {GLFW_GREEN_BITS, "GLFW_GREEN_BITS"},
        {GLFW_BLUE_BITS, "GLFW_BLUE_BITS"},
        {GLFW_ALPHA_BITS, "GLFW_ALPHA_BITS"},
        {GLFW_DEPTH_BITS, "GLFW_DEPTH_BITS"},
        {GLFW_STENCIL_BITS, "GLFW_STENCIL_BITS"},
        {GLFW_ACCUM_RED_BITS, "GLFW_ACCUM_RED_BITS"},
        {GLFW_ACCUM_GREEN_BITS, "GLFW_ACCUM_GREEN_BITS"},
        {GLFW_ACCUM_BLUE_BITS, "GLFW_ACCUM_BLUE_BITS"},
        {GLFW_ACCUM_ALPHA_BITS, "GLFW_ACCUM_ALPHA_BITS"},
        {GLFW_AUX_BUFFERS, "GLFW_AUX_BUFFERS"},
        {GLFW_STEREO, "GLFW_STEREO"},
        {GLFW_SAMPLES, "GLFW_SAMPLES"},
        {GLFW_SRGB_CAPABLE, "GLFW_SRGB_CAPABLE"},
        {GLFW_REFRESH_RATE, "GLFW_REFRESH_RATE"},
        {GLFW_DOUBLEBUFFER, "GLFW_DOUBLEBUFFER"},
        {GLFW_CLIENT_API, "GLFW_CLIENT_API"},
        {GLFW_CONTEXT_VERSION_MAJOR, "GLFW_CONTEXT_VERSION_MAJOR"},
        {GLFW_CONTEXT_VERSION_MINOR, "GLFW_CONTEXT_VERSION_MINOR"},
        {GLFW_CONTEXT_REVISION, "GLFW_CONTEXT_REVISION"},
        {GLFW_CONTEXT_ROBUSTNESS, "GLFW_CONTEXT_ROBUSTNESS"},
        {GLFW_OPENGL_FORWARD_COMPAT, "GLFW_OPENGL_FORWARD_COMPAT"},
        {GLFW_OPENGL_DEBUG_CONTEXT, "GLFW_OPENGL_DEBUG_CONTEXT"},
        {GLFW_OPENGL_PROFILE, "GLFW_OPENGL_PROFILE"},
        {GLFW_CONTEXT_RELEASE_BEHAVIOR, "GLFW_CONTEXT_RELEASE_BEHAVIOR"},
        {GLFW_CONTEXT_NO_ERROR, "GLFW_CONTEXT_NO_ERROR"},
        {GLFW_CONTEXT_CREATION_API, "GLFW_CONTEXT_CREATION_API"},
        {GLFW_SCALE_TO_MONITOR, "GLFW_SCALE_TO_MONITOR"},
        {GLFW_COCOA_RETINA_FRAMEBUFFER, "GLFW_COCOA_RETINA_FRAMEBUFFER"},
        {GLFW_COCOA_FRAME_NAME, "GLFW_COCOA_FRAME_NAME"},
        {GLFW_COCOA_GRAPHICS_SWITCHING, "GLFW_COCOA_GRAPHICS_SWITCHING"},
        {GLFW_X11_CLASS_NAME, "GLFW_X11_CLASS_NAME"},
        {GLFW_X11_INSTANCE_NAME, "GLFW_X11_INSTANCE_NAME"},
        {GLFW_NO_API, "GLFW_NO_API"},
        {GLFW_OPENGL_API, "GLFW_OPENGL_API"},
        {GLFW_OPENGL_ES_API, "GLFW_OPENGL_ES_API"},
        {GLFW_NO_ROBUSTNESS, "GLFW_NO_ROBUSTNESS"},
        {GLFW_NO_RESET_NOTIFICATION, "GLFW_NO_RESET_NOTIFICATION"},
        {GLFW_LOSE_CONTEXT_ON_RESET, "GLFW_LOSE_CONTEXT_ON_RESET"},
        {GLFW_OPENGL_ANY_PROFILE, "GLFW_OPENGL_ANY_PROFILE"},
        {GLFW_OPENGL_CORE_PROFILE, "GLFW_OPENGL_CORE_PROFILE"},
        {GLFW_OPENGL_COMPAT_PROFILE, "GLFW_OPENGL_COMPAT_PROFILE"},
        {GLFW_CURSOR, "GLFW_CURSOR"},
        {GLFW_STICKY_KEYS, "GLFW_STICKY_KEYS"},
        {GLFW_STICKY_MOUSE_BUTTONS, "GLFW_STICKY_MOUSE_BUTTONS"},
        {GLFW_LOCK_KEY_MODS, "GLFW_LOCK_KEY_MODS"},
        {GLFW_RAW_MOUSE_MOTION, "GLFW_RAW_MOUSE_MOTION"},
        {GLFW_CURSOR_NORMAL, "GLFW_CURSOR_NORMAL"},
        {GLFW_CURSOR_HIDDEN, "GLFW_CURSOR_HIDDEN"},
        {GLFW_CURSOR_DISABLED, "GLFW_CURSOR_DISABLED"},
        {GLFW_ANY_RELEASE_BEHAVIOR, "GLFW_ANY_RELEASE_BEHAVIOR"},
        {GLFW_RELEASE_BEHAVIOR_FLUSH, "GLFW_RELEASE_BEHAVIOR_FLUSH"},
        {GLFW_RELEASE_BEHAVIOR_NONE, "GLFW_RELEASE_BEHAVIOR_NONE"},
        {GLFW_NATIVE_CONTEXT_API, "GLFW_NATIVE_CONTEXT_API"},
        {GLFW_EGL_CONTEXT_API, "GLFW_EGL_CONTEXT_API"},
        {GLFW_OSMESA_CONTEXT_API, "GLFW_OSMESA_CONTEXT_API"},
        {GLFW_ARROW_CURSOR, "GLFW_ARROW_CURSOR"},
        {GLFW_IBEAM_CURSOR, "GLFW_IBEAM_CURSOR"},
        {GLFW_CROSSHAIR_CURSOR, "GLFW_CROSSHAIR_CURSOR"},
        {GLFW_HRESIZE_CURSOR, "GLFW_HRESIZE_CURSOR"},
        {GLFW_VRESIZE_CURSOR, "GLFW_VRESIZE_CURSOR"},
        {GLFW_HAND_CURSOR, "GLFW_HAND_CURSOR"},
        {GLFW_CONNECTED, "GLFW_CONNECTED"},
        {GLFW_DISCONNECTED, "GLFW_DISCONNECTED"},
        {GLFW_JOYSTICK_HAT_BUTTONS, "GLFW_JOYSTICK_HAT_BUTTONS"},
        {GLFW_COCOA_CHDIR_RESOURCES, "GLFW_COCOA_CHDIR_RESOURCES"},
        {GLFW_COCOA_MENUBAR, "GLFW_COCOA_MENUBAR"}
    };

    vc::add_lua_flag_mapping(vs, glfw_mapping);
}

}; /* namespace vkc */

namespace virt_composer {

inline std::unordered_map<std::string, VkBufferUsageFlagBits> vk_buffer_usage_flag_bits_from_str = {
    {"VK_BUFFER_USAGE_NONE", (VkBufferUsageFlagBits)0},
    {"VK_BUFFER_USAGE_TRANSFER_SRC_BIT",
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT},
    {"VK_BUFFER_USAGE_TRANSFER_DST_BIT",
            VK_BUFFER_USAGE_TRANSFER_DST_BIT},
    {"VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT",
            VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT},
    {"VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT",
            VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT},
    {"VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT",
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT},
    {"VK_BUFFER_USAGE_STORAGE_BUFFER_BIT",
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT},
    {"VK_BUFFER_USAGE_INDEX_BUFFER_BIT",
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT},
    {"VK_BUFFER_USAGE_VERTEX_BUFFER_BIT",
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT},
    {"VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT",
            VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT},
    {"VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT",
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT},
    // {"VK_BUFFER_USAGE_VIDEO_DECODE_SRC_BIT_KHR",
    //         VK_BUFFER_USAGE_VIDEO_DECODE_SRC_BIT_KHR},
    // {"VK_BUFFER_USAGE_VIDEO_DECODE_DST_BIT_KHR",
    //         VK_BUFFER_USAGE_VIDEO_DECODE_DST_BIT_KHR},
    {"VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT",
            VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT},
    {"VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_COUNTER_BUFFER_BIT_EXT",
            VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_COUNTER_BUFFER_BIT_EXT},
    {"VK_BUFFER_USAGE_CONDITIONAL_RENDERING_BIT_EXT",
            VK_BUFFER_USAGE_CONDITIONAL_RENDERING_BIT_EXT},
#ifdef VK_ENABLE_BETA_EXTENSIONS
    {"VK_BUFFER_USAGE_EXECUTION_GRAPH_SCRATCH_BIT_AMDX",
            VK_BUFFER_USAGE_EXECUTION_GRAPH_SCRATCH_BIT_AMDX},
#endif
    // {"VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR",
    //         VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR},
    // {"VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR",
    //         VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR},
    // {"VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR",
    //         VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR},
    // {"VK_BUFFER_USAGE_VIDEO_ENCODE_DST_BIT_KHR",
    //         VK_BUFFER_USAGE_VIDEO_ENCODE_DST_BIT_KHR},
    // {"VK_BUFFER_USAGE_VIDEO_ENCODE_SRC_BIT_KHR",
    //         VK_BUFFER_USAGE_VIDEO_ENCODE_SRC_BIT_KHR},
    // {"VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT",
    //         VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT},
    // {"VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT",
    //         VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT},
    // {"VK_BUFFER_USAGE_PUSH_DESCRIPTORS_DESCRIPTOR_BUFFER_BIT_EXT",
    //         VK_BUFFER_USAGE_PUSH_DESCRIPTORS_DESCRIPTOR_BUFFER_BIT_EXT},
    // {"VK_BUFFER_USAGE_MICROMAP_BUILD_INPUT_READ_ONLY_BIT_EXT",
    //         VK_BUFFER_USAGE_MICROMAP_BUILD_INPUT_READ_ONLY_BIT_EXT},
    // {"VK_BUFFER_USAGE_MICROMAP_STORAGE_BIT_EXT",
    //         VK_BUFFER_USAGE_MICROMAP_STORAGE_BIT_EXT},
    // {"VK_BUFFER_USAGE_TILE_MEMORY_BIT_QCOM",
    //         VK_BUFFER_USAGE_TILE_MEMORY_BIT_QCOM},
    {"VK_BUFFER_USAGE_RAY_TRACING_BIT_NV",
            VK_BUFFER_USAGE_RAY_TRACING_BIT_NV},
    {"VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_EXT",
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_EXT},
    {"VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR",
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR},
};

template <> inline VkBufferUsageFlagBits get_enum_val<VkBufferUsageFlagBits>(fkyaml::node &n) {
    return get_enum_val(n, vk_buffer_usage_flag_bits_from_str);
}

inline std::unordered_map<std::string, VkSharingMode> vk_sharing_mode_from_str = {
    {"VK_SHARING_MODE_EXCLUSIVE", VK_SHARING_MODE_EXCLUSIVE},
    {"VK_SHARING_MODE_CONCURRENT", VK_SHARING_MODE_CONCURRENT},
};

template <> inline VkSharingMode get_enum_val<VkSharingMode>(fkyaml::node &n) {
    return get_enum_val(n, vk_sharing_mode_from_str);
}

inline std::unordered_map<std::string, VkMemoryPropertyFlagBits>
        vk_memory_property_flag_bits_from_str =
{
    {"VK_MEMORY_PROPERTY_NONE", (VkMemoryPropertyFlagBits)0},
    {"VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT", VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT},
    {"VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT", VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT},
    {"VK_MEMORY_PROPERTY_HOST_COHERENT_BIT", VK_MEMORY_PROPERTY_HOST_COHERENT_BIT},
    {"VK_MEMORY_PROPERTY_HOST_CACHED_BIT", VK_MEMORY_PROPERTY_HOST_CACHED_BIT},
    {"VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT", VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT},
    {"VK_MEMORY_PROPERTY_PROTECTED_BIT", VK_MEMORY_PROPERTY_PROTECTED_BIT},
    {"VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD", VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD},
    {"VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD", VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD},
    // {"VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV", VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV},
};

template <> inline VkMemoryPropertyFlagBits get_enum_val<VkMemoryPropertyFlagBits>(fkyaml::node &n) {
    return get_enum_val(n, vk_memory_property_flag_bits_from_str);
}

inline std::unordered_map<std::string, VkPrimitiveTopology> vk_primitive_topology_from_str = {
    {"VK_PRIMITIVE_TOPOLOGY_POINT_LIST",
            VK_PRIMITIVE_TOPOLOGY_POINT_LIST},
    {"VK_PRIMITIVE_TOPOLOGY_LINE_LIST",
            VK_PRIMITIVE_TOPOLOGY_LINE_LIST},
    {"VK_PRIMITIVE_TOPOLOGY_LINE_STRIP",
            VK_PRIMITIVE_TOPOLOGY_LINE_STRIP},
    {"VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST",
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST},
    {"VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP",
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP},
    {"VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN",
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN},
    {"VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY",
            VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY},
    {"VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY",
            VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY},
    {"VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY",
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY},
    {"VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY",
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY},
    {"VK_PRIMITIVE_TOPOLOGY_PATCH_LIST",
            VK_PRIMITIVE_TOPOLOGY_PATCH_LIST},
};

template <> inline VkPrimitiveTopology get_enum_val<VkPrimitiveTopology>(fkyaml::node &n) {
    return get_enum_val(n, vk_primitive_topology_from_str);
}

inline std::unordered_map<std::string, VkImageAspectFlagBits> vk_image_aspect_flag_bits_from_str = {
    {"VK_IMAGE_ASPECT_NONE", (VkImageAspectFlagBits)0},
    {"VK_IMAGE_ASPECT_COLOR_BIT", VK_IMAGE_ASPECT_COLOR_BIT},
    {"VK_IMAGE_ASPECT_DEPTH_BIT", VK_IMAGE_ASPECT_DEPTH_BIT},
    {"VK_IMAGE_ASPECT_STENCIL_BIT", VK_IMAGE_ASPECT_STENCIL_BIT},
    {"VK_IMAGE_ASPECT_METADATA_BIT", VK_IMAGE_ASPECT_METADATA_BIT},
    {"VK_IMAGE_ASPECT_PLANE_0_BIT", VK_IMAGE_ASPECT_PLANE_0_BIT},
    {"VK_IMAGE_ASPECT_PLANE_1_BIT", VK_IMAGE_ASPECT_PLANE_1_BIT},
    {"VK_IMAGE_ASPECT_PLANE_2_BIT", VK_IMAGE_ASPECT_PLANE_2_BIT},
    {"VK_IMAGE_ASPECT_MEMORY_PLANE_0_BIT_EXT", VK_IMAGE_ASPECT_MEMORY_PLANE_0_BIT_EXT},
    {"VK_IMAGE_ASPECT_MEMORY_PLANE_1_BIT_EXT", VK_IMAGE_ASPECT_MEMORY_PLANE_1_BIT_EXT},
    {"VK_IMAGE_ASPECT_MEMORY_PLANE_2_BIT_EXT", VK_IMAGE_ASPECT_MEMORY_PLANE_2_BIT_EXT},
    {"VK_IMAGE_ASPECT_MEMORY_PLANE_3_BIT_EXT", VK_IMAGE_ASPECT_MEMORY_PLANE_3_BIT_EXT},
    {"VK_IMAGE_ASPECT_PLANE_0_BIT_KHR", VK_IMAGE_ASPECT_PLANE_0_BIT_KHR},
    {"VK_IMAGE_ASPECT_PLANE_1_BIT_KHR", VK_IMAGE_ASPECT_PLANE_1_BIT_KHR},
    {"VK_IMAGE_ASPECT_PLANE_2_BIT_KHR", VK_IMAGE_ASPECT_PLANE_2_BIT_KHR},
};

template <> inline VkImageAspectFlagBits get_enum_val<VkImageAspectFlagBits>(fkyaml::node &n) {
    return get_enum_val(n, vk_image_aspect_flag_bits_from_str);
}

inline std::unordered_map<std::string, VkCommandBufferUsageFlagBits>
        vk_command_buffer_usage_flag_bits_from_str =
{
    {"VK_COMMAND_BUFFER_USAGE_NONE", (VkCommandBufferUsageFlagBits)0},
    {"VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT",
            VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT},
    {"VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT",
            VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT},
    {"VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT",
            VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT},
};

template <> inline VkCommandBufferUsageFlagBits get_enum_val<VkCommandBufferUsageFlagBits>(
        fkyaml::node &n)
{
    return get_enum_val(n, vk_command_buffer_usage_flag_bits_from_str);
}

inline std::unordered_map<std::string, VkPipelineBindPoint> vk_pipeline_bind_point_from_str = {
    {"VK_PIPELINE_BIND_POINT_GRAPHICS",
            VK_PIPELINE_BIND_POINT_GRAPHICS},
    {"VK_PIPELINE_BIND_POINT_COMPUTE",
            VK_PIPELINE_BIND_POINT_COMPUTE},
    {"VK_PIPELINE_BIND_POINT_RAY_TRACING_NV",
            VK_PIPELINE_BIND_POINT_RAY_TRACING_NV},
};

template <> inline VkPipelineBindPoint get_enum_val<VkPipelineBindPoint>(fkyaml::node &n) {
    return get_enum_val(n, vk_pipeline_bind_point_from_str);
}

inline std::unordered_map<std::string, VkIndexType> vk_index_type_from_str = {
    {"VK_INDEX_TYPE_UINT16", VK_INDEX_TYPE_UINT16},
    {"VK_INDEX_TYPE_UINT32", VK_INDEX_TYPE_UINT32},
    {"VK_INDEX_TYPE_NONE_NV", VK_INDEX_TYPE_NONE_NV},
    {"VK_INDEX_TYPE_UINT8_EXT", VK_INDEX_TYPE_UINT8_EXT},
};

template <> inline VkIndexType get_enum_val<VkIndexType>(fkyaml::node &n) {
    return get_enum_val(n, vk_index_type_from_str);
}

inline std::unordered_map<std::string, VkPipelineStageFlagBits>
        vk_pipeline_stage_flag_bits_from_str =
{
    {"VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT",
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT},
    {"VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT",
            VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT},
    {"VK_PIPELINE_STAGE_VERTEX_INPUT_BIT",
            VK_PIPELINE_STAGE_VERTEX_INPUT_BIT},
    {"VK_PIPELINE_STAGE_VERTEX_SHADER_BIT",
            VK_PIPELINE_STAGE_VERTEX_SHADER_BIT},
    {"VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT",
            VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT},
    {"VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT",
            VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT},
    {"VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT",
            VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT},
    {"VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT",
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT},
    {"VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT",
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT},
    {"VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT",
            VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT},
    {"VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT",
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
    {"VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT",
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT},
    {"VK_PIPELINE_STAGE_TRANSFER_BIT",
            VK_PIPELINE_STAGE_TRANSFER_BIT},
    {"VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT",
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT},
    {"VK_PIPELINE_STAGE_HOST_BIT",
            VK_PIPELINE_STAGE_HOST_BIT},
    {"VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT",
            VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT},
    {"VK_PIPELINE_STAGE_ALL_COMMANDS_BIT",
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT},
    {"VK_PIPELINE_STAGE_TRANSFORM_FEEDBACK_BIT_EXT",
            VK_PIPELINE_STAGE_TRANSFORM_FEEDBACK_BIT_EXT},
    {"VK_PIPELINE_STAGE_CONDITIONAL_RENDERING_BIT_EXT",
            VK_PIPELINE_STAGE_CONDITIONAL_RENDERING_BIT_EXT},
    {"VK_PIPELINE_STAGE_FRAGMENT_DENSITY_PROCESS_BIT_EXT",
            VK_PIPELINE_STAGE_FRAGMENT_DENSITY_PROCESS_BIT_EXT},
    {"VK_PIPELINE_STAGE_SHADING_RATE_IMAGE_BIT_NV",
            VK_PIPELINE_STAGE_SHADING_RATE_IMAGE_BIT_NV},
    {"VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV",
            VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV},
    {"VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV",
            VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV},
    {"VK_PIPELINE_STAGE_TASK_SHADER_BIT_NV",
            VK_PIPELINE_STAGE_TASK_SHADER_BIT_NV},
    {"VK_PIPELINE_STAGE_MESH_SHADER_BIT_NV",
            VK_PIPELINE_STAGE_MESH_SHADER_BIT_NV},
};

template <> inline VkPipelineStageFlagBits get_enum_val<VkPipelineStageFlagBits>(fkyaml::node &n) {
    return get_enum_val(n, vk_pipeline_stage_flag_bits_from_str);
}

inline std::unordered_map<std::string, VkFormat> vk_format_from_str = {
    {"VK_FORMAT_UNDEFINED", VK_FORMAT_UNDEFINED},
    {"VK_FORMAT_R4G4_UNORM_PACK8", VK_FORMAT_R4G4_UNORM_PACK8},
    {"VK_FORMAT_R4G4B4A4_UNORM_PACK16", VK_FORMAT_R4G4B4A4_UNORM_PACK16},
    {"VK_FORMAT_B4G4R4A4_UNORM_PACK16", VK_FORMAT_B4G4R4A4_UNORM_PACK16},
    {"VK_FORMAT_R5G6B5_UNORM_PACK16", VK_FORMAT_R5G6B5_UNORM_PACK16},
    {"VK_FORMAT_B5G6R5_UNORM_PACK16", VK_FORMAT_B5G6R5_UNORM_PACK16},
    {"VK_FORMAT_R5G5B5A1_UNORM_PACK16", VK_FORMAT_R5G5B5A1_UNORM_PACK16},
    {"VK_FORMAT_B5G5R5A1_UNORM_PACK16", VK_FORMAT_B5G5R5A1_UNORM_PACK16},
    {"VK_FORMAT_A1R5G5B5_UNORM_PACK16", VK_FORMAT_A1R5G5B5_UNORM_PACK16},
    {"VK_FORMAT_R8_UNORM", VK_FORMAT_R8_UNORM},
    {"VK_FORMAT_R8_SNORM", VK_FORMAT_R8_SNORM},
    {"VK_FORMAT_R8_USCALED", VK_FORMAT_R8_USCALED},
    {"VK_FORMAT_R8_SSCALED", VK_FORMAT_R8_SSCALED},
    {"VK_FORMAT_R8_UINT", VK_FORMAT_R8_UINT},
    {"VK_FORMAT_R8_SINT", VK_FORMAT_R8_SINT},
    {"VK_FORMAT_R8_SRGB", VK_FORMAT_R8_SRGB},
    {"VK_FORMAT_R8G8_UNORM", VK_FORMAT_R8G8_UNORM},
    {"VK_FORMAT_R8G8_SNORM", VK_FORMAT_R8G8_SNORM},
    {"VK_FORMAT_R8G8_USCALED", VK_FORMAT_R8G8_USCALED},
    {"VK_FORMAT_R8G8_SSCALED", VK_FORMAT_R8G8_SSCALED},
    {"VK_FORMAT_R8G8_UINT", VK_FORMAT_R8G8_UINT},
    {"VK_FORMAT_R8G8_SINT", VK_FORMAT_R8G8_SINT},
    {"VK_FORMAT_R8G8_SRGB", VK_FORMAT_R8G8_SRGB},
    {"VK_FORMAT_R8G8B8_UNORM", VK_FORMAT_R8G8B8_UNORM},
    {"VK_FORMAT_R8G8B8_SNORM", VK_FORMAT_R8G8B8_SNORM},
    {"VK_FORMAT_R8G8B8_USCALED", VK_FORMAT_R8G8B8_USCALED},
    {"VK_FORMAT_R8G8B8_SSCALED", VK_FORMAT_R8G8B8_SSCALED},
    {"VK_FORMAT_R8G8B8_UINT", VK_FORMAT_R8G8B8_UINT},
    {"VK_FORMAT_R8G8B8_SINT", VK_FORMAT_R8G8B8_SINT},
    {"VK_FORMAT_R8G8B8_SRGB", VK_FORMAT_R8G8B8_SRGB},
    {"VK_FORMAT_B8G8R8_UNORM", VK_FORMAT_B8G8R8_UNORM},
    {"VK_FORMAT_B8G8R8_SNORM", VK_FORMAT_B8G8R8_SNORM},
    {"VK_FORMAT_B8G8R8_USCALED", VK_FORMAT_B8G8R8_USCALED},
    {"VK_FORMAT_B8G8R8_SSCALED", VK_FORMAT_B8G8R8_SSCALED},
    {"VK_FORMAT_B8G8R8_UINT", VK_FORMAT_B8G8R8_UINT},
    {"VK_FORMAT_B8G8R8_SINT", VK_FORMAT_B8G8R8_SINT},
    {"VK_FORMAT_B8G8R8_SRGB", VK_FORMAT_B8G8R8_SRGB},
    {"VK_FORMAT_R8G8B8A8_UNORM", VK_FORMAT_R8G8B8A8_UNORM},
    {"VK_FORMAT_R8G8B8A8_SNORM", VK_FORMAT_R8G8B8A8_SNORM},
    {"VK_FORMAT_R8G8B8A8_USCALED", VK_FORMAT_R8G8B8A8_USCALED},
    {"VK_FORMAT_R8G8B8A8_SSCALED", VK_FORMAT_R8G8B8A8_SSCALED},
    {"VK_FORMAT_R8G8B8A8_UINT", VK_FORMAT_R8G8B8A8_UINT},
    {"VK_FORMAT_R8G8B8A8_SINT", VK_FORMAT_R8G8B8A8_SINT},
    {"VK_FORMAT_R8G8B8A8_SRGB", VK_FORMAT_R8G8B8A8_SRGB},
    {"VK_FORMAT_B8G8R8A8_UNORM", VK_FORMAT_B8G8R8A8_UNORM},
    {"VK_FORMAT_B8G8R8A8_SNORM", VK_FORMAT_B8G8R8A8_SNORM},
    {"VK_FORMAT_B8G8R8A8_USCALED", VK_FORMAT_B8G8R8A8_USCALED},
    {"VK_FORMAT_B8G8R8A8_SSCALED", VK_FORMAT_B8G8R8A8_SSCALED},
    {"VK_FORMAT_B8G8R8A8_UINT", VK_FORMAT_B8G8R8A8_UINT},
    {"VK_FORMAT_B8G8R8A8_SINT", VK_FORMAT_B8G8R8A8_SINT},
    {"VK_FORMAT_B8G8R8A8_SRGB", VK_FORMAT_B8G8R8A8_SRGB},
    {"VK_FORMAT_A8B8G8R8_UNORM_PACK32", VK_FORMAT_A8B8G8R8_UNORM_PACK32},
    {"VK_FORMAT_A8B8G8R8_SNORM_PACK32", VK_FORMAT_A8B8G8R8_SNORM_PACK32},
    {"VK_FORMAT_A8B8G8R8_USCALED_PACK32", VK_FORMAT_A8B8G8R8_USCALED_PACK32},
    {"VK_FORMAT_A8B8G8R8_SSCALED_PACK32", VK_FORMAT_A8B8G8R8_SSCALED_PACK32},
    {"VK_FORMAT_A8B8G8R8_UINT_PACK32", VK_FORMAT_A8B8G8R8_UINT_PACK32},
    {"VK_FORMAT_A8B8G8R8_SINT_PACK32", VK_FORMAT_A8B8G8R8_SINT_PACK32},
    {"VK_FORMAT_A8B8G8R8_SRGB_PACK32", VK_FORMAT_A8B8G8R8_SRGB_PACK32},
    {"VK_FORMAT_A2R10G10B10_UNORM_PACK32", VK_FORMAT_A2R10G10B10_UNORM_PACK32},
    {"VK_FORMAT_A2R10G10B10_SNORM_PACK32", VK_FORMAT_A2R10G10B10_SNORM_PACK32},
    {"VK_FORMAT_A2R10G10B10_USCALED_PACK32", VK_FORMAT_A2R10G10B10_USCALED_PACK32},
    {"VK_FORMAT_A2R10G10B10_SSCALED_PACK32", VK_FORMAT_A2R10G10B10_SSCALED_PACK32},
    {"VK_FORMAT_A2R10G10B10_UINT_PACK32", VK_FORMAT_A2R10G10B10_UINT_PACK32},
    {"VK_FORMAT_A2R10G10B10_SINT_PACK32", VK_FORMAT_A2R10G10B10_SINT_PACK32},
    {"VK_FORMAT_A2B10G10R10_UNORM_PACK32", VK_FORMAT_A2B10G10R10_UNORM_PACK32},
    {"VK_FORMAT_A2B10G10R10_SNORM_PACK32", VK_FORMAT_A2B10G10R10_SNORM_PACK32},
    {"VK_FORMAT_A2B10G10R10_USCALED_PACK32", VK_FORMAT_A2B10G10R10_USCALED_PACK32},
    {"VK_FORMAT_A2B10G10R10_SSCALED_PACK32", VK_FORMAT_A2B10G10R10_SSCALED_PACK32},
    {"VK_FORMAT_A2B10G10R10_UINT_PACK32", VK_FORMAT_A2B10G10R10_UINT_PACK32},
    {"VK_FORMAT_A2B10G10R10_SINT_PACK32", VK_FORMAT_A2B10G10R10_SINT_PACK32},
    {"VK_FORMAT_R16_UNORM", VK_FORMAT_R16_UNORM},
    {"VK_FORMAT_R16_SNORM", VK_FORMAT_R16_SNORM},
    {"VK_FORMAT_R16_USCALED", VK_FORMAT_R16_USCALED},
    {"VK_FORMAT_R16_SSCALED", VK_FORMAT_R16_SSCALED},
    {"VK_FORMAT_R16_UINT", VK_FORMAT_R16_UINT},
    {"VK_FORMAT_R16_SINT", VK_FORMAT_R16_SINT},
    {"VK_FORMAT_R16_SFLOAT", VK_FORMAT_R16_SFLOAT},
    {"VK_FORMAT_R16G16_UNORM", VK_FORMAT_R16G16_UNORM},
    {"VK_FORMAT_R16G16_SNORM", VK_FORMAT_R16G16_SNORM},
    {"VK_FORMAT_R16G16_USCALED", VK_FORMAT_R16G16_USCALED},
    {"VK_FORMAT_R16G16_SSCALED", VK_FORMAT_R16G16_SSCALED},
    {"VK_FORMAT_R16G16_UINT", VK_FORMAT_R16G16_UINT},
    {"VK_FORMAT_R16G16_SINT", VK_FORMAT_R16G16_SINT},
    {"VK_FORMAT_R16G16_SFLOAT", VK_FORMAT_R16G16_SFLOAT},
    {"VK_FORMAT_R16G16B16_UNORM", VK_FORMAT_R16G16B16_UNORM},
    {"VK_FORMAT_R16G16B16_SNORM", VK_FORMAT_R16G16B16_SNORM},
    {"VK_FORMAT_R16G16B16_USCALED", VK_FORMAT_R16G16B16_USCALED},
    {"VK_FORMAT_R16G16B16_SSCALED", VK_FORMAT_R16G16B16_SSCALED},
    {"VK_FORMAT_R16G16B16_UINT", VK_FORMAT_R16G16B16_UINT},
    {"VK_FORMAT_R16G16B16_SINT", VK_FORMAT_R16G16B16_SINT},
    {"VK_FORMAT_R16G16B16_SFLOAT", VK_FORMAT_R16G16B16_SFLOAT},
    {"VK_FORMAT_R16G16B16A16_UNORM", VK_FORMAT_R16G16B16A16_UNORM},
    {"VK_FORMAT_R16G16B16A16_SNORM", VK_FORMAT_R16G16B16A16_SNORM},
    {"VK_FORMAT_R16G16B16A16_USCALED", VK_FORMAT_R16G16B16A16_USCALED},
    {"VK_FORMAT_R16G16B16A16_SSCALED", VK_FORMAT_R16G16B16A16_SSCALED},
    {"VK_FORMAT_R16G16B16A16_UINT", VK_FORMAT_R16G16B16A16_UINT},
    {"VK_FORMAT_R16G16B16A16_SINT", VK_FORMAT_R16G16B16A16_SINT},
    {"VK_FORMAT_R16G16B16A16_SFLOAT", VK_FORMAT_R16G16B16A16_SFLOAT},
    {"VK_FORMAT_R32_UINT", VK_FORMAT_R32_UINT},
    {"VK_FORMAT_R32_SINT", VK_FORMAT_R32_SINT},
    {"VK_FORMAT_R32_SFLOAT", VK_FORMAT_R32_SFLOAT},
    {"VK_FORMAT_R32G32_UINT", VK_FORMAT_R32G32_UINT},
    {"VK_FORMAT_R32G32_SINT", VK_FORMAT_R32G32_SINT},
    {"VK_FORMAT_R32G32_SFLOAT", VK_FORMAT_R32G32_SFLOAT},
    {"VK_FORMAT_R32G32B32_UINT", VK_FORMAT_R32G32B32_UINT},
    {"VK_FORMAT_R32G32B32_SINT", VK_FORMAT_R32G32B32_SINT},
    {"VK_FORMAT_R32G32B32_SFLOAT", VK_FORMAT_R32G32B32_SFLOAT},
    {"VK_FORMAT_R32G32B32A32_UINT", VK_FORMAT_R32G32B32A32_UINT},
    {"VK_FORMAT_R32G32B32A32_SINT", VK_FORMAT_R32G32B32A32_SINT},
    {"VK_FORMAT_R32G32B32A32_SFLOAT", VK_FORMAT_R32G32B32A32_SFLOAT},
    {"VK_FORMAT_R64_UINT", VK_FORMAT_R64_UINT},
    {"VK_FORMAT_R64_SINT", VK_FORMAT_R64_SINT},
    {"VK_FORMAT_R64_SFLOAT", VK_FORMAT_R64_SFLOAT},
    {"VK_FORMAT_R64G64_UINT", VK_FORMAT_R64G64_UINT},
    {"VK_FORMAT_R64G64_SINT", VK_FORMAT_R64G64_SINT},
    {"VK_FORMAT_R64G64_SFLOAT", VK_FORMAT_R64G64_SFLOAT},
    {"VK_FORMAT_R64G64B64_UINT", VK_FORMAT_R64G64B64_UINT},
    {"VK_FORMAT_R64G64B64_SINT", VK_FORMAT_R64G64B64_SINT},
    {"VK_FORMAT_R64G64B64_SFLOAT", VK_FORMAT_R64G64B64_SFLOAT},
    {"VK_FORMAT_R64G64B64A64_UINT", VK_FORMAT_R64G64B64A64_UINT},
    {"VK_FORMAT_R64G64B64A64_SINT", VK_FORMAT_R64G64B64A64_SINT},
    {"VK_FORMAT_R64G64B64A64_SFLOAT", VK_FORMAT_R64G64B64A64_SFLOAT},
    {"VK_FORMAT_B10G11R11_UFLOAT_PACK32", VK_FORMAT_B10G11R11_UFLOAT_PACK32},
    {"VK_FORMAT_E5B9G9R9_UFLOAT_PACK32", VK_FORMAT_E5B9G9R9_UFLOAT_PACK32},
    {"VK_FORMAT_D16_UNORM", VK_FORMAT_D16_UNORM},
    {"VK_FORMAT_X8_D24_UNORM_PACK32", VK_FORMAT_X8_D24_UNORM_PACK32},
    {"VK_FORMAT_D32_SFLOAT", VK_FORMAT_D32_SFLOAT},
    {"VK_FORMAT_S8_UINT", VK_FORMAT_S8_UINT},
    {"VK_FORMAT_D16_UNORM_S8_UINT", VK_FORMAT_D16_UNORM_S8_UINT},
    {"VK_FORMAT_D24_UNORM_S8_UINT", VK_FORMAT_D24_UNORM_S8_UINT},
    {"VK_FORMAT_D32_SFLOAT_S8_UINT", VK_FORMAT_D32_SFLOAT_S8_UINT},
    {"VK_FORMAT_BC1_RGB_UNORM_BLOCK", VK_FORMAT_BC1_RGB_UNORM_BLOCK},
    {"VK_FORMAT_BC1_RGB_SRGB_BLOCK", VK_FORMAT_BC1_RGB_SRGB_BLOCK},
    {"VK_FORMAT_BC1_RGBA_UNORM_BLOCK", VK_FORMAT_BC1_RGBA_UNORM_BLOCK},
    {"VK_FORMAT_BC1_RGBA_SRGB_BLOCK", VK_FORMAT_BC1_RGBA_SRGB_BLOCK},
    {"VK_FORMAT_BC2_UNORM_BLOCK", VK_FORMAT_BC2_UNORM_BLOCK},
    {"VK_FORMAT_BC2_SRGB_BLOCK", VK_FORMAT_BC2_SRGB_BLOCK},
    {"VK_FORMAT_BC3_UNORM_BLOCK", VK_FORMAT_BC3_UNORM_BLOCK},
    {"VK_FORMAT_BC3_SRGB_BLOCK", VK_FORMAT_BC3_SRGB_BLOCK},
    {"VK_FORMAT_BC4_UNORM_BLOCK", VK_FORMAT_BC4_UNORM_BLOCK},
    {"VK_FORMAT_BC4_SNORM_BLOCK", VK_FORMAT_BC4_SNORM_BLOCK},
    {"VK_FORMAT_BC5_UNORM_BLOCK", VK_FORMAT_BC5_UNORM_BLOCK},
    {"VK_FORMAT_BC5_SNORM_BLOCK", VK_FORMAT_BC5_SNORM_BLOCK},
    {"VK_FORMAT_BC6H_UFLOAT_BLOCK", VK_FORMAT_BC6H_UFLOAT_BLOCK},
    {"VK_FORMAT_BC6H_SFLOAT_BLOCK", VK_FORMAT_BC6H_SFLOAT_BLOCK},
    {"VK_FORMAT_BC7_UNORM_BLOCK", VK_FORMAT_BC7_UNORM_BLOCK},
    {"VK_FORMAT_BC7_SRGB_BLOCK", VK_FORMAT_BC7_SRGB_BLOCK},
    {"VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK", VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK},
    {"VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK", VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK},
    {"VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK", VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK},
    {"VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK", VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK},
    {"VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK", VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK},
    {"VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK", VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK},
    {"VK_FORMAT_EAC_R11_UNORM_BLOCK", VK_FORMAT_EAC_R11_UNORM_BLOCK},
    {"VK_FORMAT_EAC_R11_SNORM_BLOCK", VK_FORMAT_EAC_R11_SNORM_BLOCK},
    {"VK_FORMAT_EAC_R11G11_UNORM_BLOCK", VK_FORMAT_EAC_R11G11_UNORM_BLOCK},
    {"VK_FORMAT_EAC_R11G11_SNORM_BLOCK", VK_FORMAT_EAC_R11G11_SNORM_BLOCK},
    {"VK_FORMAT_ASTC_4x4_UNORM_BLOCK", VK_FORMAT_ASTC_4x4_UNORM_BLOCK},
    {"VK_FORMAT_ASTC_4x4_SRGB_BLOCK", VK_FORMAT_ASTC_4x4_SRGB_BLOCK},
    {"VK_FORMAT_ASTC_5x4_UNORM_BLOCK", VK_FORMAT_ASTC_5x4_UNORM_BLOCK},
    {"VK_FORMAT_ASTC_5x4_SRGB_BLOCK", VK_FORMAT_ASTC_5x4_SRGB_BLOCK},
    {"VK_FORMAT_ASTC_5x5_UNORM_BLOCK", VK_FORMAT_ASTC_5x5_UNORM_BLOCK},
    {"VK_FORMAT_ASTC_5x5_SRGB_BLOCK", VK_FORMAT_ASTC_5x5_SRGB_BLOCK},
    {"VK_FORMAT_ASTC_6x5_UNORM_BLOCK", VK_FORMAT_ASTC_6x5_UNORM_BLOCK},
    {"VK_FORMAT_ASTC_6x5_SRGB_BLOCK", VK_FORMAT_ASTC_6x5_SRGB_BLOCK},
    {"VK_FORMAT_ASTC_6x6_UNORM_BLOCK", VK_FORMAT_ASTC_6x6_UNORM_BLOCK},
    {"VK_FORMAT_ASTC_6x6_SRGB_BLOCK", VK_FORMAT_ASTC_6x6_SRGB_BLOCK},
    {"VK_FORMAT_ASTC_8x5_UNORM_BLOCK", VK_FORMAT_ASTC_8x5_UNORM_BLOCK},
    {"VK_FORMAT_ASTC_8x5_SRGB_BLOCK", VK_FORMAT_ASTC_8x5_SRGB_BLOCK},
    {"VK_FORMAT_ASTC_8x6_UNORM_BLOCK", VK_FORMAT_ASTC_8x6_UNORM_BLOCK},
    {"VK_FORMAT_ASTC_8x6_SRGB_BLOCK", VK_FORMAT_ASTC_8x6_SRGB_BLOCK},
    {"VK_FORMAT_ASTC_8x8_UNORM_BLOCK", VK_FORMAT_ASTC_8x8_UNORM_BLOCK},
    {"VK_FORMAT_ASTC_8x8_SRGB_BLOCK", VK_FORMAT_ASTC_8x8_SRGB_BLOCK},
    {"VK_FORMAT_ASTC_10x5_UNORM_BLOCK", VK_FORMAT_ASTC_10x5_UNORM_BLOCK},
    {"VK_FORMAT_ASTC_10x5_SRGB_BLOCK", VK_FORMAT_ASTC_10x5_SRGB_BLOCK},
    {"VK_FORMAT_ASTC_10x6_UNORM_BLOCK", VK_FORMAT_ASTC_10x6_UNORM_BLOCK},
    {"VK_FORMAT_ASTC_10x6_SRGB_BLOCK", VK_FORMAT_ASTC_10x6_SRGB_BLOCK},
    {"VK_FORMAT_ASTC_10x8_UNORM_BLOCK", VK_FORMAT_ASTC_10x8_UNORM_BLOCK},
    {"VK_FORMAT_ASTC_10x8_SRGB_BLOCK", VK_FORMAT_ASTC_10x8_SRGB_BLOCK},
    {"VK_FORMAT_ASTC_10x10_UNORM_BLOCK", VK_FORMAT_ASTC_10x10_UNORM_BLOCK},
    {"VK_FORMAT_ASTC_10x10_SRGB_BLOCK", VK_FORMAT_ASTC_10x10_SRGB_BLOCK},
    {"VK_FORMAT_ASTC_12x10_UNORM_BLOCK", VK_FORMAT_ASTC_12x10_UNORM_BLOCK},
    {"VK_FORMAT_ASTC_12x10_SRGB_BLOCK", VK_FORMAT_ASTC_12x10_SRGB_BLOCK},
    {"VK_FORMAT_ASTC_12x12_UNORM_BLOCK", VK_FORMAT_ASTC_12x12_UNORM_BLOCK},
    {"VK_FORMAT_ASTC_12x12_SRGB_BLOCK", VK_FORMAT_ASTC_12x12_SRGB_BLOCK},
};

template <> inline VkFormat get_enum_val<VkFormat>(fkyaml::node &n) {
    return get_enum_val(n, vk_format_from_str);
}

inline std::unordered_map<std::string, VkVertexInputRate> vk_vertex_input_rate_from_str = {
    {"VK_VERTEX_INPUT_RATE_VERTEX", VK_VERTEX_INPUT_RATE_VERTEX},
    {"VK_VERTEX_INPUT_RATE_INSTANCE", VK_VERTEX_INPUT_RATE_INSTANCE},
};

template <> inline VkVertexInputRate get_enum_val<VkVertexInputRate>(fkyaml::node &n) {
    return get_enum_val(n, vk_vertex_input_rate_from_str);
}

inline std::unordered_map<std::string, VkShaderStageFlagBits> vk_shader_stage_flag_bits_from_str = {
    {"VK_SHADER_STAGE_VERTEX_BIT", VK_SHADER_STAGE_VERTEX_BIT},
    {"VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT", VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT},
    {"VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT", VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT},
    {"VK_SHADER_STAGE_GEOMETRY_BIT", VK_SHADER_STAGE_GEOMETRY_BIT},
    {"VK_SHADER_STAGE_FRAGMENT_BIT", VK_SHADER_STAGE_FRAGMENT_BIT},
    {"VK_SHADER_STAGE_COMPUTE_BIT", VK_SHADER_STAGE_COMPUTE_BIT},
    {"VK_SHADER_STAGE_ALL_GRAPHICS", VK_SHADER_STAGE_ALL_GRAPHICS},
    {"VK_SHADER_STAGE_ALL", VK_SHADER_STAGE_ALL},
    {"VK_SHADER_STAGE_RAYGEN_BIT_NV", VK_SHADER_STAGE_RAYGEN_BIT_NV},
    {"VK_SHADER_STAGE_ANY_HIT_BIT_NV", VK_SHADER_STAGE_ANY_HIT_BIT_NV},
    {"VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV", VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV},
    {"VK_SHADER_STAGE_MISS_BIT_NV", VK_SHADER_STAGE_MISS_BIT_NV},
    {"VK_SHADER_STAGE_INTERSECTION_BIT_NV", VK_SHADER_STAGE_INTERSECTION_BIT_NV},
    {"VK_SHADER_STAGE_CALLABLE_BIT_NV", VK_SHADER_STAGE_CALLABLE_BIT_NV},
    {"VK_SHADER_STAGE_TASK_BIT_NV", VK_SHADER_STAGE_TASK_BIT_NV},
    {"VK_SHADER_STAGE_MESH_BIT_NV", VK_SHADER_STAGE_MESH_BIT_NV},
};

template <> inline VkShaderStageFlagBits get_enum_val<VkShaderStageFlagBits>(fkyaml::node &n) {
    return get_enum_val(n, vk_shader_stage_flag_bits_from_str);
}

inline std::unordered_map<std::string, VkDescriptorType> vk_descriptor_type_from_str = {
    {"VK_DESCRIPTOR_TYPE_SAMPLER", VK_DESCRIPTOR_TYPE_SAMPLER}, 
    {"VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER", VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER}, 
    {"VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE", VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE}, 
    {"VK_DESCRIPTOR_TYPE_STORAGE_IMAGE", VK_DESCRIPTOR_TYPE_STORAGE_IMAGE}, 
    {"VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER", VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER}, 
    {"VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER", VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER}, 
    {"VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER", VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER}, 
    {"VK_DESCRIPTOR_TYPE_STORAGE_BUFFER", VK_DESCRIPTOR_TYPE_STORAGE_BUFFER}, 
    {"VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC", VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC}, 
    {"VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC", VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC}, 
    {"VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT", VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT}, 
    {"VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV", VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV},
    {"VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT", VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT},
};

template <> inline VkDescriptorType get_enum_val<VkDescriptorType>(fkyaml::node &n) {
    return get_enum_val(n, vk_descriptor_type_from_str);
}

inline std::unordered_map<std::string, vku_shader_stage_e> shader_stage_from_string = {
    {"VKU_SPIRV_VERTEX",    VKU_SPIRV_VERTEX},
    {"VKU_SPIRV_FRAGMENT",  VKU_SPIRV_FRAGMENT},
    {"VKU_SPIRV_COMPUTE",   VKU_SPIRV_COMPUTE},
    {"VKU_SPIRV_GEOMETRY",  VKU_SPIRV_GEOMETRY},
    {"VKU_SPIRV_TESS_CTRL", VKU_SPIRV_TESS_CTRL},
    {"VKU_SPIRV_TESS_EVAL", VKU_SPIRV_TESS_EVAL},
};

template <> inline vku_shader_stage_e get_enum_val<vku_shader_stage_e>(fkyaml::node &n) {
    return get_enum_val(n, shader_stage_from_string);
}


} /* virt_composer */

#endif
