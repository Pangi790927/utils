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
 * 
 *
 * vkc::lua_function_t
 * -------------------
 *
 * Description: Represents a C++ function exposed to Lua. Can be called from Lua scripts 
 * using a Lua state, allowing integration of native C++ callbacks into Lua code.
 *
 * Members:
 * - m_name: Name of the function as seen in Lua.
 * - m_source: Source or context of the function (e.g., shared object path or C++ module).
 *
 * Member functions:
 * - call(L): Executes the C++ callback using a given Lua state.
 *
 * Init: create(name, source)
 *   - Parameters:
 *     - name: Name of the function in Lua.
 *     - source: Source or context of the function.
 * 
 * 
 * vkc::lua_script_t
 * -----------------
 *
 * Description: Holds a Lua script as a string. Can be loaded or executed from C++ 
 * code using a Lua state, enabling scripting functionality.
 *
 * Members:
 * - content: The text of the Lua script.
 *
 * Init: create(content)
 *   - Parameters:
 *     - content: The Lua script source code as a string.
 * 
 * 
 * vkc::integer_t
 * --------------
 *
 * Description: Wraps a 64-bit integer for bookkeeping or parameter storage within the 
 * Vulkan wrapper framework. Useful for tracking values in a reference-managed system.
 *
 * Members:
 * - value: The stored 64-bit integer.
 *
 * Init: create(value)
 *   - Parameters:
 *     - value: Initial integer value.
 * 
 * 
 * vkc::float_t
 * ------------
 *
 * Description: Wraps a double-precision floating-point value for bookkeeping or 
 * parameter storage within the Vulkan wrapper framework. Useful for tracking values 
 * in a reference-managed system.
 *
 * Members:
 * - value: The stored double-precision floating-point number.
 *
 * Member functions:
 * (none specific; access value directly)
 *
 * Init: create(value)
 *   - Parameters:
 *     - value: Initial floating-point value.
 * 
 * 
 * vkc::string_t
 * -------------
 *
 * Description: Wraps a standard string for bookkeeping or parameter storage within 
 * the Vulkan wrapper framework. Useful for managing text values in a reference-managed 
 * system.
 *
 * Members:
 * - value: The stored string.
 *
 * Init: create(value)
 *   - Parameters:
 *     - value: Initial string content.
 * 
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
 * 
 */

/*! @file This file will be used to auto-initialize Vulkan. This file will describe the structure
 * of the Vulkan objects and create an vulkan_obj_t with the specific type and an associated name.
 * In this way you can configure all the semaphores and buffers from here and ask for them inside
 * the program. The objective is to have the entire Vulkan pipeline described from outside of the
 * source code. */

/* TODO: figure out what to do with this: (The LUA_IMPL part, I think most of the things in
vulkan_composer would stay better in a CPP file) */
#define LUA_IMPL

#include "vulkan_utils.h"
#include "yaml.h"
#include "tinyexpr.h"
#include "minilua.h"
#include "demangle.h"
#include "co_utils.h"

#include <coroutine>
#include <filesystem>

/* TODO: this also needs to stay in an implementation file */
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>


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

namespace vulkan_composer {

namespace vo = virt_object;
namespace vku = vulkan_utils;
namespace vkc = vulkan_composer;

VULKAN_UTILS_REGISTER_TYPE(VKC_TYPE_SPIRV);
VULKAN_UTILS_REGISTER_TYPE(VKC_TYPE_STRING);
VULKAN_UTILS_REGISTER_TYPE(VKC_TYPE_FLOAT);
VULKAN_UTILS_REGISTER_TYPE(VKC_TYPE_CPU_BUFFER);
VULKAN_UTILS_REGISTER_TYPE(VKC_TYPE_INTEGER);
VULKAN_UTILS_REGISTER_TYPE(VKC_TYPE_LUA_SCRIPT);
VULKAN_UTILS_REGISTER_TYPE(VKC_TYPE_LUA_VARIABLE);
VULKAN_UTILS_REGISTER_TYPE(VKC_TYPE_LUA_FUNCTION);
VULKAN_UTILS_REGISTER_TYPE(VKC_TYPE_VERTEX_INPUT_DESC);
VULKAN_UTILS_REGISTER_TYPE(VKC_TYPE_BINDING_DESC);

/*! OBS: This is static not inline because we want to make sure this won't conflict with another's
 * translation unit variables */
/* Max number of named references That are concomitent */
static constexpr const int MAX_NUMBER_OF_OBJECTS = 16384;

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
    virtual VkResult _init() override { return VK_SUCCESS; }
    virtual VkResult _uninit() override { return VK_SUCCESS; }
};

/* Does this really have any irl usage? */
struct lua_function_t : public vku::object_t {
    std::string m_name;
    std::string m_source;

    static vku::object_type_e type_id_static() { return VKC_TYPE_LUA_FUNCTION; }
    static vku::ref_t<lua_function_t> create(std::string name, std::string source) {
        auto ret = vku::ref_t<lua_function_t>::create_obj_ref(
                std::make_unique<lua_function_t>(), {});
        ret->m_name = name;
        ret->m_source = source;
        return ret;
    }

    virtual vku::object_type_e type_id() const override { return VKC_TYPE_LUA_FUNCTION; }

    int call(lua_State *L) {
        DBG("Calling %s from %s", m_name.c_str(), m_source.c_str());
        (void)L;
        return 0;
    }

    inline std::string to_string() const override {
        return std::format("vkc::lua_var[{}]: m_name={} m_source={}", (void*)this, m_name, m_source);
    }


private:
    virtual VkResult _init() override { return VK_SUCCESS; }
    virtual VkResult _uninit() override { return VK_SUCCESS; }
};

struct lua_script_t : public vku::object_t {
    std::string content;

    static vku::object_type_e type_id_static() { return VKC_TYPE_LUA_SCRIPT; }
    static vku::ref_t<lua_script_t> create(std::string content) {
        auto ret = vku::ref_t<lua_script_t>::create_obj_ref(std::make_unique<lua_script_t>(), {});
        ret->content = content;
        return ret;
    }

    virtual vku::object_type_e type_id() const override { return VKC_TYPE_LUA_SCRIPT; }

    inline std::string to_string() const override {
        return std::format("vkc::lua_script[{}]: m_content=\n{}", (void*)this, content);
    }

private:
    virtual VkResult _init() override { return VK_SUCCESS; }
    virtual VkResult _uninit() override { return VK_SUCCESS; }
};


struct integer_t : public vku::object_t {
    int64_t value = 0;

    static vku::object_type_e type_id_static() { return VKC_TYPE_INTEGER; }
    static vku::ref_t<integer_t> create(int64_t value) {
        auto ret = vku::ref_t<integer_t>::create_obj_ref(std::make_unique<integer_t>(), {});
        ret->value = value;
        return ret;
    }

    virtual vku::object_type_e type_id() const override { return VKC_TYPE_INTEGER; }

    inline std::string to_string() const override {
        return std::format("vkc::integer[{}]: value={} ", (void*)this, value);
    }

private:
    virtual VkResult _init() override { return VK_SUCCESS; }
    virtual VkResult _uninit() override { return VK_SUCCESS; }
};

struct float_t : public vku::object_t {
    double value = 0;

    static vku::object_type_e type_id_static() { return VKC_TYPE_FLOAT; }
    static vku::ref_t<float_t> create(double value) {
        auto ret = vku::ref_t<float_t>::create_obj_ref(std::make_unique<float_t>(), {});
        ret->value = value;
        return ret;
    }

    virtual vku::object_type_e type_id() const override { return VKC_TYPE_FLOAT; }

    inline std::string to_string() const override {
        return std::format("vkc::float[{}]: value={} ", (void*)this, value);
    }

private:
    virtual VkResult _init() override { return VK_SUCCESS; }
    virtual VkResult _uninit() override { return VK_SUCCESS; }
};

/* This does the following: creates a buffer in cpu-memory space that can be used to copy data to
from it */
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
    virtual VkResult _init() override { return VK_SUCCESS; }
    virtual VkResult _uninit() override { return VK_SUCCESS; }

    std::vector<uint8_t> _data;
};


struct string_t : public vku::object_t {
    std::string value;

    static vku::object_type_e type_id_static() { return VKC_TYPE_STRING; }
    static vku::ref_t<string_t> create(const std::string& value) {
        auto ret = vku::ref_t<string_t>::create_obj_ref(std::make_unique<string_t>(), {});
        ret->value = value;
        return ret;
    }

    virtual vku::object_type_e type_id() const override { return VKC_TYPE_STRING; }

    inline std::string to_string() const override {
        return std::format("vkc::string[{}]: value={} ", (void*)this, value);
    }

private:
    virtual VkResult _init() override { return VK_SUCCESS; }
    virtual VkResult _uninit() override { return VK_SUCCESS; }
};

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
    virtual VkResult _init() override { return VK_SUCCESS; }
    virtual VkResult _uninit() override { return VK_SUCCESS; }
};

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
    virtual VkResult _init() override { return VK_SUCCESS; }
    virtual VkResult _uninit() override { return VK_SUCCESS; }
};

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
    virtual VkResult _init() override { return VK_SUCCESS; }
    virtual VkResult _uninit() override { return VK_SUCCESS; }
};

/*! IMPLEMENTATION
 ***************************************************************************************************
 ***************************************************************************************************
 ***************************************************************************************************
 */

static std::string app_path = std::filesystem::canonical("./");

enum luaw_member_e {
    LUAW_MEMBER_FUNCTION,
    LUAW_MEMBER_OBJECT,
};

struct luaw_member_t {
    lua_CFunction fn;
    luaw_member_e member_type;
};

extern int VKU_TYPE_CNT;
extern std::unordered_map<std::string, luaw_member_t> *lua_class_members;
extern std::unordered_map<std::string, lua_CFunction> *lua_class_member_setters;


/* This holds the obhects reference such that lua can use them */
struct object_ref_t {
    vku::ref_t<vku::object_t> obj;  /* The actual reference to the vku/vkc object */

    std::string name;               /* this is required such that when this object gets removed it
                                       also gets removed from objects_map */
};

/* Holds all the references to the objects, it is held as on object so it can be initialized by
the lua code and upon success, merged */
struct ref_state_t {
    std::vector<object_ref_t> objects;
    std::map<std::string, int> objects_map;
    std::map<std::string, std::vector<co::state_t *>> wanted_objects;
    std::deque<std::coroutine_handle<void>> work;
    std::vector<int> free_objects;

    ref_state_t() : objects(MAX_NUMBER_OF_OBJECTS) {
        for (int i = MAX_NUMBER_OF_OBJECTS-1; i >= 1; i--)
            free_objects.push_back(i);
    }

    /* TODO: this is stupid slow, must be made faster (create a clear interface of adding and
    removing objects and make get_new and append return the internal held update list) */

    std::vector<int> get_new(const ref_state_t &oth) {
        std::set<int> free_objects_this(free_objects.begin(), free_objects.end());
        std::set<int> free_objects_other(oth.free_objects.begin(), oth.free_objects.end());

        std::vector<int> new_other;
        std::set_difference(
            free_objects_this.begin(), free_objects_this.end(),
            free_objects_other.begin(), free_objects_other.end(),
            std::back_inserter(new_other)
        );

        std::vector<int> new_this;
        std::set_difference(
            free_objects_other.begin(), free_objects_other.end(),
            free_objects_this.begin(), free_objects_this.end(),
            std::back_inserter(new_this)
        );

        if (new_this.size())
            throw vku::err_t(
                    "How could the state change while we where creating a new object? huh?");
        return new_other;
    }

    void append(const ref_state_t &oth) {
        std::set<int> free_objects_this(free_objects.begin(), free_objects.end());
        std::set<int> free_objects_other(oth.free_objects.begin(), oth.free_objects.end());

        std::vector<int> new_other;
        std::set_difference(
            free_objects_this.begin(), free_objects_this.end(),
            free_objects_other.begin(), free_objects_other.end(),
            std::back_inserter(new_other)
        );

        std::vector<int> new_this;
        std::set_difference(
            free_objects_other.begin(), free_objects_other.end(),
            free_objects_this.begin(), free_objects_this.end(),
            std::back_inserter(new_this)
        );

        for (int idx : new_other) {
            this->objects[idx] = oth.objects[idx];
            this->objects_map[oth.objects[idx].name] = idx;
        }
        this->free_objects = std::vector<int>(free_objects_other.begin(), free_objects_other.end());
    }
};

/* The state is initially created to have  */
static ref_state_t g_rs;
static int g_lua_table;

template <typename T, typename K>
constexpr auto has(T&& data_struct, K&& key) {
    return std::forward<T>(data_struct).find(std::forward<K>(key))
            != std::forward<T>(data_struct).end();
}

template <typename VkuT>
struct depend_resolver_t {
    /* We save the searched dependency */
    depend_resolver_t(ref_state_t *rs, std::string required_depend)
    : required_depend(required_depend), rs(rs) {}

    /* If we already have the dependency we can already retur */
    bool await_ready() noexcept { return has(rs->objects_map, required_depend); }

    /* Else we place ourselves on the waiting queue */
    template <typename P>
    co::handle<void> await_suspend(co::handle<P> caller) noexcept {
        auto state = co::external_on_suspend(caller);
        /* We place ourselves on the waiting queue: */
        rs->wanted_objects[required_depend].push_back(state);

        /* Else we return the next work in line that can be done */
        return co::external_wait_next_task(state->pool);
    }

    vku::ref_t<VkuT> await_resume() {
        if (!has(rs->objects_map, required_depend)) {
            DBG("Object not found");
            throw vku::err_t(std::format("Object not found, {}", required_depend));
        }
        if (!rs->objects[rs->objects_map[required_depend]].obj) {
            DBG("For some reason this object now holds a nullptr...");
            throw vku::err_t("nullptr object");
        }
        auto ret = rs->objects[rs->objects_map[required_depend]].obj.to_related<VkuT>();
        if (!ret) {
            DBG("Invalid ref...");
            throw vku::err_t(sformat("Invalid reference, maybe cast doesn't work?: [cast: %s to: %s]",
                    demangle<4>(typeid(
                        rs->objects[rs->objects_map[required_depend]].obj.get()).name()).c_str(),
                    demangle<VkuT, 4>().c_str()));
        }
        return ret;
    }

    std::string required_depend;
    ref_state_t *rs;
};

void mark_dependency_solved(ref_state_t *rs,
        std::string depend_name, vku::ref_t<vku::object_t> depend)
{
    /* First remember the dependency: */
    if (!depend) {
        DBG("Object into nullptr");
        throw vku::err_t{std::format("Object turned into nullptr: {}", depend_name)};
    }
    if (has(rs->objects_map, depend_name)) {
        DBG("Name taken");
        throw vku::err_t{std::format("Tag name already exists: {}", depend_name)};
    }
    int new_id = rs->free_objects.back();
    rs->free_objects.pop_back();

    DBG("Adding object: %s [%d]", depend->to_string().c_str(), new_id);
    rs->objects_map[depend_name] = new_id;
    depend->cbks = std::make_shared<vo::object_cbks_t<vku::vulkan_traits_t>>();
    depend->cbks->usr_ptr = std::shared_ptr<void>((void *)(intptr_t)new_id, [](void *){});
    rs->objects[new_id].obj = depend;
    rs->objects[new_id].name = depend_name;

    /* Second, awake all the ones waiting for the respective dependency */
    if (vkc::has(rs->wanted_objects, depend_name)) {
        for (auto s : rs->wanted_objects[depend_name])
            co::external_sched_resume(s);
        rs->wanted_objects.erase(depend_name);
    }
}

inline bool starts_with(const std::string& a, const std::string& b) {
    return a.size() >= b.size() && a.compare(0, b.size(), b) == 0;
}

static std::unordered_map<std::string, vku_shader_stage_e> shader_stage_from_string = {
    {"VKU_SPIRV_VERTEX",    VKU_SPIRV_VERTEX},
    {"VKU_SPIRV_FRAGMENT",  VKU_SPIRV_FRAGMENT},
    {"VKU_SPIRV_COMPUTE",   VKU_SPIRV_COMPUTE},
    {"VKU_SPIRV_GEOMETRY",  VKU_SPIRV_GEOMETRY},
    {"VKU_SPIRV_TESS_CTRL", VKU_SPIRV_TESS_CTRL},
    {"VKU_SPIRV_TESS_EVAL", VKU_SPIRV_TESS_EVAL},
};

template <typename T>
inline T get_enum_val(fkyaml::node &node, const std::unordered_map<std::string, T>& enum_vals) {
    if (node.is_string()) {
        if (!has(enum_vals, node.as_str()))
            throw vku::err_t(std::format("Unknown enum({}) value: {}", demangle<T>(), node.as_str()));
        return enum_vals.find(node.as_str())->second;
    }
    if (node.is_sequence()) {
        uint32_t ret = 0;
        for (auto &val : node.as_seq())
            ret |= (uint32_t)get_enum_val(val, enum_vals);
        return (T)ret;
    }
    throw vku::err_t{std::format("Node({}), can't be converted to an enum of type ({})",
            fkyaml::node::serialize(node), demangle<T>())};
}

template <typename T>
inline T get_enum_val(fkyaml::node &n) {
    throw vku::err_t{std::format("Type {} not implemented", demangle<T>())};
}

static std::unordered_map<std::string, VkBufferUsageFlagBits> vk_buffer_usage_flag_bits_from_str = {
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

static std::unordered_map<std::string, VkSharingMode> vk_sharing_mode_from_str = {
    {"VK_SHARING_MODE_EXCLUSIVE", VK_SHARING_MODE_EXCLUSIVE},
    {"VK_SHARING_MODE_CONCURRENT", VK_SHARING_MODE_CONCURRENT},
};

template <> inline VkSharingMode get_enum_val<VkSharingMode>(fkyaml::node &n) {
    return get_enum_val(n, vk_sharing_mode_from_str);
}

static std::unordered_map<std::string, VkMemoryPropertyFlagBits>
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

static std::unordered_map<std::string, VkPrimitiveTopology> vk_primitive_topology_from_str = {
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

static std::unordered_map<std::string, VkImageAspectFlagBits> vk_image_aspect_flag_bits_from_str = {
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

static std::unordered_map<std::string, VkCommandBufferUsageFlagBits>
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

static std::unordered_map<std::string, VkPipelineBindPoint> vk_pipeline_bind_point_from_str = {
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

static std::unordered_map<std::string, VkIndexType> vk_index_type_from_str = {
    {"VK_INDEX_TYPE_UINT16", VK_INDEX_TYPE_UINT16},
    {"VK_INDEX_TYPE_UINT32", VK_INDEX_TYPE_UINT32},
    {"VK_INDEX_TYPE_NONE_NV", VK_INDEX_TYPE_NONE_NV},
    {"VK_INDEX_TYPE_UINT8_EXT", VK_INDEX_TYPE_UINT8_EXT},
};

template <> inline VkIndexType get_enum_val<VkIndexType>(fkyaml::node &n) {
    return get_enum_val(n, vk_index_type_from_str);
}

static std::unordered_map<std::string, VkPipelineStageFlagBits>
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

static std::unordered_map<std::string, VkFormat> vk_format_from_str = {
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

static std::unordered_map<std::string, VkVertexInputRate> vk_vertex_input_rate_from_str = {
    {"VK_VERTEX_INPUT_RATE_VERTEX", VK_VERTEX_INPUT_RATE_VERTEX},
    {"VK_VERTEX_INPUT_RATE_INSTANCE", VK_VERTEX_INPUT_RATE_INSTANCE},
};

template <> inline VkVertexInputRate get_enum_val<VkVertexInputRate>(fkyaml::node &n) {
    return get_enum_val(n, vk_vertex_input_rate_from_str);
}

static std::unordered_map<std::string, VkShaderStageFlagBits> vk_shader_stage_flag_bits_from_str = {
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

static std::unordered_map<std::string, VkDescriptorType> vk_descriptor_type_from_str = {
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

inline auto get_from_map(auto &m, const std::string& str) {
    if (!has(m, str))
        throw vku::err_t(std::format("Failed to get object: {} from: {}",
                str, demangle<decltype(m), 2>()));
    return m[str];
}

inline std::string get_file_string_content(const std::string& file_path_relative) {
    std::string file_path = std::filesystem::canonical(file_path_relative);

    if (!starts_with(file_path, app_path)) {
        DBG("The path is restricted to the application main directory");
        throw vku::err_t(std::format("File_error [{} vs {}]", file_path, app_path));
    }

    std::ifstream ifs(file_path.c_str());

    if (!ifs.good()) {
        DBG("Failed to open path: %s", file_path.c_str());
        throw std::runtime_error("File_error");
    }

    return std::string((std::istreambuf_iterator<char>(ifs)),
                       (std::istreambuf_iterator<char>()));
}

inline auto load_image(auto cp, std::string path) {
    int w, h, chans;
    stbi_uc* pixels = stbi_load(path.c_str(), &w, &h, &chans, STBI_rgb_alpha);

    /* TODO: some more logs around here */
    VkDeviceSize imag_sz = w*h*4;
    if (!pixels) {
        throw vku::err_t("Failed to load image");
    }

    auto img = vku::image_t::create(cp->m_device, w, h, VK_FORMAT_R8G8B8A8_SRGB);
    img->set_data(cp, pixels, imag_sz);

    stbi_image_free(pixels);

    return img;
}

static std::vector<
    std::pair<
        std::function<bool(fkyaml::node& node)>,
        std::function<co::task<vku::ref_t<vku::object_t>> (ref_state_t *,
                const std::string&, fkyaml::node&)>
    >
> build_psudo_object_cbks;

co::task_t build_pseudo_object(ref_state_t *rs, const std::string& name, fkyaml::node& node) {
    for (auto &[match, cbk] : build_psudo_object_cbks)
        if (match(node))
            co_return co_await cbk(rs, name, node);

    if (node.is_integer()) {
        auto obj = integer_t::create(node.as_int());
        mark_dependency_solved(rs, name, obj.to_related<vku::object_t>());
        co_return 0;
    }

    if (node.is_string()) {
        auto obj = string_t::create(node.as_str());
        mark_dependency_solved(rs, name, obj.to_related<vku::object_t>());
        co_return 0;
    }

    if (node.is_mapping() && node.contains("m_shader_type")) {
        vku::spirv_t spirv;

        if (node.contains("m_source")) {
            spirv = vku::spirv_compile(
                    get_from_map(shader_stage_from_string, node["m_shader_type"].as_str()),
                    node["m_source"].as_str().c_str());
        }

        if (node.contains("m_source_path")) {
            if (spirv.content.size()) {
                DBG("Trying to initialize spirv from 2 sources (only one of source, "
                        "source-path, or spirv-path allowed)");
                co_return -1;
            }
            spirv = vku::spirv_compile(
                    get_from_map(shader_stage_from_string, node["m_shader_type"].as_str()),
                    get_file_string_content(node["m_source_path"].as_str()).c_str());
        }

        if (node.contains("m_spirv_path")) {
            if (spirv.content.size()) {
                DBG("Trying to initialize spirv from 2 sources (only one of source, "
                        "source-path, or spirv-path allowed)");
                co_return -1;
            }

            spirv.type = get_from_map(shader_stage_from_string, node["m_shader_type"].as_str());
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
        mark_dependency_solved(rs, name, obj.to_related<vku::object_t>());

        co_return 0;
    }

    if (name == "lua_script") {
        if (!(node.contains("m_source") || node.contains("source_path"))) {
            DBG("lua-script must be a node that has either source or source-path")
            co_return -1;
        }

        if (node.contains("m_source") && node.contains("m_source_path")) {
            DBG("lua-script can be either loaded from inline script or from a specified path, not"
                    "from both!");
            co_return -1;
        }

        if (node.contains("m_source")) {
            auto obj = lua_script_t::create(node["m_source"].as_str());
            mark_dependency_solved(rs, name, obj.to_related<vku::object_t>());
            co_return 0;
        }

        if (node.contains("m_source_path")) {
            std::string source = get_file_string_content(node["m_source_path"].as_str());

            auto obj = lua_script_t::create(source);
            mark_dependency_solved(rs, name, obj.to_related<vku::object_t>());
            co_return 0;
        }
    }

    DBG("Failed to build anything from this object[%s], so the object is invalid", name.c_str());
    co_return -1;
}

static std::vector<std::pair<std::string, double>> vkc_constant_sizes_from_string = {
    {"SIZEOF_INT16", sizeof(int16_t)},
    {"SIZEOF_INT32", sizeof(int32_t)},
    {"SIZEOF_INT64", sizeof(int64_t)},
    {"SIZEOF_UINT16", sizeof(uint16_t)},
    {"SIZEOF_UINT32", sizeof(int32_t)},
    {"SIZEOF_UINT64", sizeof(int64_t)},
    {"SIZEOF_FLOAT", sizeof(float)},
    {"SIZEOF_DOUBLE", sizeof(double)},
    {"SIZEOF_VEC_2F", sizeof(float)*2},
    {"SIZEOF_VEC_3F", sizeof(float)*3},
    {"SIZEOF_VEC_4F", sizeof(float)*4},
    {"SIZEOF_VEC_2D", sizeof(double)*2},
    {"SIZEOF_VEC_3D", sizeof(double)*3},
    {"SIZEOF_VEC_4D", sizeof(double)*4},
    {"SIZEOF_MAT_2x2F", sizeof(float)*2*2},
    {"SIZEOF_MAT_3x3F", sizeof(float)*3*3},
    {"SIZEOF_MAT_4x4F", sizeof(float)*4*4},
    {"SIZEOF_MAT_2x2D", sizeof(double)*2*2},
    {"SIZEOF_MAT_3x3D", sizeof(double)*3*3},
    {"SIZEOF_MAT_4x4D", sizeof(double)*4*4},
};

/*! This either follows a reference to an integer or it returns the direct value if available */
co::task<int64_t> resolve_int(ref_state_t *rs, fkyaml::node& node) {
    if (node.has_tag_name() && node.get_tag_name() == "!ref")
        co_return (co_await depend_resolver_t<integer_t>(rs, node.as_str()))->value;
    if (node.is_string()) {
        /* Try to resolve an expression resulting in an integer: */
        std::string expr_str = node.as_str();

        /* TODO: Try to resolve user-defined vars */
        
        std::vector<texpr::te_variable> vars;
        for (auto &ct : vkc_constant_sizes_from_string)
            vars.push_back(texpr::te_variable{
                .name = ct.first.c_str(),
                .address = (void *)&ct.second,
                .type = texpr::TE_VARIABLE,
                .context = nullptr,
            });

        int err;
        texpr::te_expr *expr = texpr::te_compile(expr_str.c_str(), vars.data(), vars.size(), &err);

        if (expr) {
            double expr_result = texpr::te_eval(expr);
            texpr::te_free(expr);
            co_return std::round(expr_result);
        } else {
            DBG("Parse error at %d\n", err);
        }
    }
    else
        co_return node.as_int();
}

/*! This either follows a reference to an integer or it returns the direct value if available */
co::task<double> resolve_float(ref_state_t *rs, fkyaml::node& node) {
    if (node.has_tag_name() && node.get_tag_name() == "!ref")
        co_return (co_await depend_resolver_t<float_t>(rs, node.as_str()))->value;
    co_return node.as_float();
}

/*! This either follows a reference to a string or it returns the direct value if available */
co::task<std::string> resolve_str(ref_state_t *rs, fkyaml::node& node) {
    if (node.has_tag_name() && node.get_tag_name() == "!ref")
        co_return (co_await depend_resolver_t<string_t>(rs, node.as_str()))->value;
    co_return node.as_str();
}

co::task<vku::ref_t<vku::object_t>> build_object(ref_state_t *rs,
        const std::string& name, fkyaml::node& node);

static int64_t anonymous_increment = 0;
inline std::string new_anon_name() {
    return "anonymous_" + std::to_string(anonymous_increment++);
}

template <typename VkuT>
co::task<vku::ref_t<VkuT>> resolve_obj(ref_state_t *rs, fkyaml::node& node) {
    /* Check -- How objects work in the configuration file -- */

    if (node.has_tag_name() && node.get_tag_name() == "!ref") {
        /* This is simply a reference to an object m_field: !ref tag_name*/
        co_return co_await depend_resolver_t<VkuT>(rs, node.as_str());
    }
    else if (node.is_mapping() && node.as_map().size() == 1
            && node.as_map().begin()->second.contains("m_type"))
    {
        /* This is in the form m_field: tag_name: m_type: "..." */
        std::string tag = node.as_map().begin()->first.as_str();
        auto ref = co_await build_object(rs, tag, node.as_map().begin()->second);
        co_return ref.template to_related<VkuT>();
    }
    else if (node.contains("m_type")) {
        /* This is in the form m_field: m_type: "...", ie, inlined object */
        std::string tag = node.contains("m_tag") ?
                node["m_tag"].as_str() : new_anon_name();
        auto ref = co_await build_object(rs, tag, node);
        co_return ref.template to_related<VkuT>();
    }

    /* None of the above */
    throw vku::err_t{std::format("node:{} is invalid in this contex",
            fkyaml::node::serialize(node))};
}

static std::vector<
    std::pair<
        std::function<bool(fkyaml::node& node)>,
        std::function<co::task<vku::ref_t<vku::object_t>> (ref_state_t *,
                const std::string&, fkyaml::node&)>
    >
> build_object_cbks;

co::task<vku::ref_t<vku::object_t>> build_object(ref_state_t *rs,
        const std::string& name, fkyaml::node& node)
{
    if (!node.is_mapping()) {
        DBG("Error node: %s not a mapping", fkyaml::node::serialize(node).c_str());
        co_return nullptr;
    }
    for (auto &[match, cbk] : build_object_cbks)
        if (match(node))
            co_return co_await cbk(rs, name, node);
    if (false);
    else if (node["m_type"] == "vkc::vertex_input_desc_t") {
        std::vector<VkVertexInputAttributeDescription> attrs;
        for (auto attr : node["m_attrs"].as_seq()) {
            auto m_location = co_await resolve_int(rs, attr["m_location"]);
            auto m_binding = co_await resolve_int(rs, attr["m_binding"]);
            auto m_format = get_enum_val<VkFormat>(attr["m_format"]);
            auto m_offset = co_await resolve_int(rs, attr["m_offset"]);
            attrs.push_back(VkVertexInputAttributeDescription{
                .location = (uint32_t)m_location,
                .binding = (uint32_t)m_binding,
                .format = m_format,
                .offset = (uint32_t)m_offset
            });
        }
        auto m_binding = co_await resolve_int(rs, node["m_binding"]);
        auto m_stride = co_await resolve_int(rs, node["m_stride"]);
        auto m_in_rate = get_enum_val<VkVertexInputRate>(node["m_in_rate"]);
        auto obj = vertex_input_desc_t::create(vku::vertex_input_desc_t{
            .bind_desc = {
                .binding = (uint32_t)m_binding,
                .stride = (uint32_t)m_stride,
                .inputRate = m_in_rate,
            },
            .attr_desc = attrs,
        });
        mark_dependency_solved(rs, name, obj.to_related<vku::object_t>());
        co_return obj.to_related<vku::object_t>();
    }
    else if (node["m_type"] == "vkc::binding_t") {
        auto m_binding = co_await resolve_int(rs, node["m_binding"]);
        auto m_stage = get_enum_val<VkShaderStageFlagBits>(node["m_stage"]);
        auto m_desc_type = get_enum_val<VkDescriptorType>(node["m_desc_type"]);
        auto obj = binding_t::create(VkDescriptorSetLayoutBinding{
            .binding = (uint32_t)m_binding,
            .descriptorType = m_desc_type,
            .descriptorCount = 1,
            .stageFlags = m_stage,
            .pImmutableSamplers = nullptr
        });
        mark_dependency_solved(rs, name, obj.to_related<vku::object_t>());
        co_return obj.to_related<vku::object_t>();
    }
    else if (node["m_type"] == "vkc::cpu_buffer_t") {
        auto m_size = co_await resolve_int(rs, node["m_size"]);
        auto obj = cpu_buffer_t::create(m_size);
        mark_dependency_solved(rs, name, obj.to_related<vku::object_t>());
        co_return obj.to_related<vku::object_t>();
    }
    else if (node["m_type"] == "vkc::lua_var_t") { /* TODO: not sure how is this type usefull */
        /* lua_var has the same tag_name as the var name */
        auto obj = lua_var_t::create(name);
        mark_dependency_solved(rs, name, obj.to_related<vku::object_t>());
        co_return obj.to_related<vku::object_t>();
    }
    else if (node["m_type"] == "vkc::lua_function_t") {
        /* lua_function has the same tag_name as the function name */
        auto src = co_await resolve_str(rs, node["m_source"]);
        auto obj = lua_function_t::create(name, src);
        mark_dependency_solved(rs, name, obj.to_related<vku::object_t>());
        co_return obj.to_related<vku::object_t>();
    }
    else if (node["m_type"] == "vku::instance_t") {
        auto obj = vku::instance_t::create();
        mark_dependency_solved(rs, name, obj.to_related<vku::object_t>());
        co_return obj.to_related<vku::object_t>();
    }
    else if (node["m_type"] == "vku::window_t") {
        auto w = co_await resolve_int(rs, node["m_width"]);
        auto h = co_await resolve_int(rs, node["m_height"]);
        auto window_name = co_await resolve_str(rs, node["m_name"]);
        auto obj = vku::window_t::create(w, h, window_name);
        mark_dependency_solved(rs, name, obj.to_related<vku::object_t>());
        co_return obj.to_related<vku::object_t>();
    }
    else if (node["m_type"] == "vku::surface_t") {
        auto window = co_await resolve_obj<vku::window_t>(rs, node["m_window"]);
        auto instance = co_await resolve_obj<vku::instance_t>(rs, node["m_instance"]);
        auto obj = vku::surface_t::create(window, instance);
        mark_dependency_solved(rs, name, obj.to_related<vku::object_t>());
        co_return obj.to_related<vku::object_t>();
    }
    else if (node["m_type"] == "vku::device_t") {
        auto surf = co_await resolve_obj<vku::surface_t>(rs, node["m_surface"]);
        auto obj = vku::device_t::create(surf);
        mark_dependency_solved(rs, name, obj.to_related<vku::object_t>());
        co_return obj.to_related<vku::object_t>();
    }
    else if (node["m_type"] == "vku::cmdpool_t") {
        auto dev = co_await resolve_obj<vku::device_t>(rs, node["m_device"]);
        auto obj = vku::cmdpool_t::create(dev);
        mark_dependency_solved(rs, name, obj.to_related<vku::object_t>());
        co_return obj.to_related<vku::object_t>();
    }
    else if (node["m_type"] == "vku::image_t") {
        auto cp = co_await resolve_obj<vku::cmdpool_t>(rs, node["m_cmdpool"]);
        auto path = co_await resolve_str(rs, node["m_path"]);
        auto obj = load_image(cp, path);
        mark_dependency_solved(rs, name, obj.to_related<vku::object_t>());
        co_return obj.to_related<vku::object_t>();
    }
    else if (node["m_type"] == "vku::img_view_t") {
        auto img = co_await resolve_obj<vku::image_t>(rs, node["m_image"]);
        auto aspect_mask = get_enum_val<VkImageAspectFlagBits>(node["m_aspect_mask"]);
        auto obj = vku::img_view_t::create(img, aspect_mask);
        mark_dependency_solved(rs, name, obj.to_related<vku::object_t>());
        co_return obj.to_related<vku::object_t>();
    }
    else if (node["m_type"] == "vku::img_sampl_t") {
        auto dev = co_await resolve_obj<vku::device_t>(rs, node["m_device"]);
        auto obj = vku::img_sampl_t::create(dev);
        mark_dependency_solved(rs, name, obj.to_related<vku::object_t>());
        co_return obj.to_related<vku::object_t>();
    }
    else if (node["m_type"] == "vku::buffer_t") {
        auto dev = co_await resolve_obj<vku::device_t>(rs, node["m_device"]);
        size_t sz = co_await resolve_int(rs, node["m_size"]);
        auto usage_flags = get_enum_val<VkBufferUsageFlagBits>(node["m_usage_flags"]);
        auto share_mode = get_enum_val<VkSharingMode>(node["m_sharing_mode"]);
        auto memory_flags = get_enum_val<VkMemoryPropertyFlagBits>(node["m_memory_flags"]);
        auto obj = vku::buffer_t::create(dev, sz, usage_flags, share_mode, memory_flags);
        mark_dependency_solved(rs, name, obj.to_related<vku::object_t>());
        co_return obj.to_related<vku::object_t>();
    }
    else if (node["m_type"] == "vku::binding_desc_set_t") {
        std::vector<vku::ref_t<vku::binding_desc_set_t::binding_desc_t>> bindings;
        for (auto& subnode : node["m_descriptors"])
            bindings.push_back(
                    co_await resolve_obj<vku::binding_desc_set_t::binding_desc_t>(rs, subnode));
        auto obj = vku::binding_desc_set_t::create(bindings);
        mark_dependency_solved(rs, name, obj.to_related<vku::object_t>());
        co_return obj.to_related<vku::object_t>();
    }
    else if (node["m_type"] == "vku::binding_desc_set_t::buff_binding_t") {
        auto buff = co_await resolve_obj<vku::buffer_t>(rs, node["m_buffer"]);
        auto desc = co_await resolve_obj<vkc::binding_t>(rs, node["m_desc"]);
        auto obj = vku::binding_desc_set_t::buff_binding_t::create(desc->bd, buff);
        mark_dependency_solved(rs, name, obj.to_related<vku::object_t>());
        co_return obj.to_related<vku::object_t>();
    }
    else if (node["m_type"] == "vku::binding_desc_set_t::sampl_binding_t") {
        auto view = co_await resolve_obj<vku::img_view_t>(rs, node["m_view"]);
        auto sampler = co_await resolve_obj<vku::img_sampl_t>(rs, node["m_sampler"]);
        auto desc = co_await resolve_obj<vkc::binding_t>(rs, node["m_desc"]);
        auto obj = vku::binding_desc_set_t::sampl_binding_t::create(desc->bd, view, sampler);
        mark_dependency_solved(rs, name, obj.to_related<vku::object_t>());
        co_return obj.to_related<vku::object_t>();
    }
    else if (node["m_type"] == "vku::shader_t") {
        auto dev = co_await resolve_obj<vku::device_t>(rs, node["m_device"]);
        auto spirv = co_await resolve_obj<spirv_t>(rs, node["m_spirv"]);
        auto obj = vku::shader_t::create(dev, spirv->spirv);
        mark_dependency_solved(rs, name, obj.to_related<vku::object_t>());
        co_return obj.to_related<vku::object_t>();
    }
    else if (node["m_type"] == "vku::swapchain_t") {
        auto dev = co_await resolve_obj<vku::device_t>(rs, node["m_device"]);
        auto obj = vku::swapchain_t::create(dev);
        mark_dependency_solved(rs, name, obj.to_related<vku::object_t>());
        co_return obj.to_related<vku::object_t>();
    }
    else if (node["m_type"] == "vku::renderpass_t") {
        auto swc = co_await resolve_obj<vku::swapchain_t>(rs, node["m_swapchain"]);
        auto obj = vku::renderpass_t::create(swc);
        mark_dependency_solved(rs, name, obj.to_related<vku::object_t>());
        co_return obj.to_related<vku::object_t>();
    }
    else if (node["m_type"] == "vku::pipeline_t") {
        auto w = co_await resolve_int(rs, node["m_width"]);
        auto h = co_await resolve_int(rs, node["m_height"]);
        auto rp = co_await resolve_obj<vku::renderpass_t>(rs, node["m_renderpass"]);
        std::vector<vku::ref_t<vku::shader_t>> shaders;
        for (auto& sh : node["m_shaders"])
            shaders.push_back(co_await resolve_obj<vku::shader_t>(rs, sh));
        auto topol = get_enum_val<VkPrimitiveTopology>(node["m_topology"]);
        auto indesc = co_await resolve_obj<vkc::vertex_input_desc_t>(rs, node["m_input_desc"]);
        auto binds = co_await resolve_obj<vku::binding_desc_set_t>(rs, node["m_bindings"]);
        auto obj = vku::pipeline_t::create(w, h, rp, shaders, topol, indesc->vid, binds);
        mark_dependency_solved(rs, name, obj.to_related<vku::object_t>());
        co_return obj.to_related<vku::object_t>();
    }
    else if (node["m_type"] == "vku::framebuffs_t") {
        auto rp = co_await resolve_obj<vku::renderpass_t>(rs, node["m_renderpass"]);
        auto obj = vku::framebuffs_t::create(rp);
        mark_dependency_solved(rs, name, obj.to_related<vku::object_t>());
        co_return obj.to_related<vku::object_t>();
    }
    else if (node["m_type"] == "vku::sem_t") {
        auto dev = co_await resolve_obj<vku::device_t>(rs, node["m_device"]);
        auto obj = vku::sem_t::create(dev);
        mark_dependency_solved(rs, name, obj.to_related<vku::object_t>());
        co_return obj.to_related<vku::object_t>();
    }
    else if (node["m_type"] == "vku::fence_t") {
        auto dev = co_await resolve_obj<vku::device_t>(rs, node["m_device"]);
        auto obj = vku::fence_t::create(dev);
        mark_dependency_solved(rs, name, obj.to_related<vku::object_t>());
        co_return obj.to_related<vku::object_t>();
    }
    else if (node["m_type"] == "vku::cmdbuff_t") {
        auto cp = co_await resolve_obj<vku::cmdpool_t>(rs, node["m_cmdpool"]);
        auto obj = vku::cmdbuff_t::create(cp);
        mark_dependency_solved(rs, name, obj.to_related<vku::object_t>());
        co_return obj.to_related<vku::object_t>();
    }
    else if (node["m_type"] == "vku::desc_pool_t") {
        auto dev = co_await resolve_obj<vku::device_t>(rs, node["m_device"]);
        auto binds = co_await resolve_obj<vku::binding_desc_set_t>(rs, node["m_bindings"]);
        int cnt = co_await resolve_int(rs, node["m_cnt"]);
        auto obj = vku::desc_pool_t::create(dev, binds, cnt);
        mark_dependency_solved(rs, name, obj.to_related<vku::object_t>());
        co_return obj.to_related<vku::object_t>();
    }
    else if (node["m_type"] == "vku::desc_set_t") {
        auto descriptor_pool = co_await resolve_obj<vku::desc_pool_t>(rs, node["m_descriptor_pool"]);
        auto pipeline = co_await resolve_obj<vku::pipeline_t>(rs, node["m_pipeline"]);
        auto bindings = co_await resolve_obj<vku::binding_desc_set_t>(rs, node["m_bindings"]);
        auto obj = vku::desc_set_t::create(descriptor_pool, pipeline, bindings);
        mark_dependency_solved(rs, name, obj.to_related<vku::object_t>());
        co_return obj.to_related<vku::object_t>();
    }

    DBG("Object m_type is not known: %s", node["m_type"].as_str().c_str());
    throw vku::err_t{std::format("Invalid object type: {}", node["m_type"].as_str())};
}


co::task_t build_schema(ref_state_t *rs, fkyaml::node& root) {
    ASSERT_COFN(CHK_BOOL(root.is_mapping()));

    for (auto &[name, node] : root.as_map()) {
        if (!node.contains("m_type")) {
            co_await co::sched(build_pseudo_object(rs, name.as_str(), node));
        }
        else {
            co_await co::sched(build_object(rs, name.as_str(), node));
        }
    }

    co_return 0;
}

inline vkc_error_e parse_config(const char *path) {
    std::ifstream file(path);

    try {
        auto config = fkyaml::node::deserialize(file);

        auto pool = co::create_pool();
        pool->sched(build_schema(&g_rs, config));

        if (pool->run() != co::RUN_OK) {
            DBG("Failed to create the schema");
            return VKC_ERROR_GENERIC;
        }

        if (g_rs.wanted_objects.size()) {
            for (auto &[k, v]: g_rs.wanted_objects) {
                DBG("Unknown Object: %s", k.c_str());
            }
            return VKC_ERROR_PARSE_YAML;
        }
    }
    catch (fkyaml::exception &e) {
        DBG("fkyaml::exception: %s", e.what());
        return VKC_ERROR_PARSE_YAML;
    }
    catch (std::exception &e) {
        DBG("Exception: %s", e.what());
        return VKC_ERROR_GENERIC;
    }

    return VKC_ERROR_OK;
}

inline void* luaw_to_user_data(int index) { return (void*)(intptr_t)(index); }
inline int luaw_from_user_data(void *val) { return (int)(intptr_t)(val); }

void luaw_push_error(lua_State *L, std::string err_str) {
    lua_Debug ar;
    std::string context;
    int i = 2;
    auto line_source = [](const char *src, int N) -> std::string {
        if (!src)
            return "<unknown>";
        std::istringstream stream(src);
        std::string line;
        int current = 1;

        while (std::getline(stream, line)) {
            if (current == N)
                return line;
            current++;
        }
        return "<unknown>";
    };

    while (lua_getstack(L, i, &ar)) {
        lua_getinfo(L, "nSl", &ar);
        context = std::format("      at {:>20}:{:<4}, in '{}':\n",
                ar.short_src, ar.currentline, line_source(ar.source, ar.currentline)) + context;
        i++;
    }
    if (lua_getstack(L, 1, &ar)) {
        lua_getinfo(L, "nSl", &ar);
        context += std::format("Error at {:>20}:{:<4}, in '{}':\n",
                ar.short_src, ar.currentline, line_source(ar.source, ar.currentline));
    }
    context += err_str;
    lua_pushstring(L, context.c_str());
}

/* This is here just to hold the diverse bitmaps */
template <typename T>
struct bm_t {
    using type = T;
};

template <typename Param, ssize_t index>
struct luaw_param_t{
    void luaw_single_param(lua_State *L) {
        /* What a parameter can be:
        1. vku::ref_t of some object
        2. a std::string
        3. an integer bitmap
        4. an integer */

        /* If this is resolved to a void it will error out, which is ok, because this case is either
        way an error */
        demangle_static_assert<false, Param>(" - Is not a valid parameter type");
    }
};

/* This resolves userdata(void *) received from lua to an vku parameter */
template <ssize_t index>
struct luaw_param_t<void *, index> {
    void *luaw_single_param(lua_State *L) {
        if (lua_isnil(L, index))
            return NULL;
        return lua_touserdata(L, index);
    }
};

/* This resolves userdata(vku::ref) received from lua to an vku parameter */
template <typename T, ssize_t index>
struct luaw_param_t<vku::ref_t<T>, index> {
    vku::ref_t<T> luaw_single_param(lua_State *L) {
        if (lua_isnil(L, index))
            return vku::ref_t<T>{}; /* if the user intended to pass a nill, we give it as a nullptr */
        int obj_index = luaw_from_user_data(lua_touserdata(L, index));
        if (obj_index == 0) {
            luaw_push_error(L, std::format("Invalid parameter at index {} of expected type {} but "
                    "got [{}] instead",
                    index, demangle<T>(), lua_typename(L, lua_type(L, index))));
            lua_error(L);
        }
        return g_rs.objects[obj_index].obj.to_related<T>();
    }
};

/* This resolves bitmasks received from lua to an vku parameter */
template <typename T, ssize_t index>
struct luaw_param_t<bm_t<T>, index> {
    T luaw_single_param(lua_State *L) {
        /* There are 2 options here (maybe later we will also add numbers, but not for now):
            1. This is a string that converts to the respective type bitmask
            2. An integer, this will be converted to T
            3. This is an enum value, either like 1. or 2. */
        auto from_string = [](lua_State *L, int idx) -> T {
            const char *val = lua_tostring(L, idx);
            if (!val) {
                luaw_push_error(L, std::format(
                        "Invalid parameter at index {}, failed conversion to [vku-bitmask] "
                        "object is an invalid string: [{}]",
                        idx, lua_typename(L, lua_type(L, idx))));
                lua_error(L);
            }
            fkyaml::node str_enum_val{val};
            return get_enum_val<T>(str_enum_val);
        };
        auto from_integer = [](lua_State *L, int idx) -> T {
            int valid = 0;
            uint32_t val = lua_tointegerx(L, idx, &valid);
            if (!valid) {
                luaw_push_error(L, std::format(
                        "Invalid parameter at index {}, failed conversion to [vku-bitmask] "
                        "object is an invalid integer: [{}]",
                        idx, lua_typename(L, lua_type(L, idx))));
                lua_error(L);
            }
            return (T)val;
        };
        if (lua_isinteger(L, index)) {
            return from_integer(L, (int)index);
        }
        else if (lua_isstring(L, index)) {
            return from_string(L, (int)index);
        } 
        else if (lua_istable(L, index)) {
            int len = lua_rawlen(L, index);
            T ret = (T)0;
            for (int i = 1; i <= len; i++) {
                lua_rawgeti(L, index, i);
                if (lua_isinteger(L, -1))
                    ret = (T)(ret | from_integer(L, -1));
                else if (lua_isstring(L, -1))
                    ret = (T)(ret | from_string(L, -1));
                else {
                    luaw_push_error(L, std::format(
                            "Invalid parameter at index {}, failed conversion to [vku-bitmask] "
                            "object is an invalid string or integer: [{}]",
                            index, lua_typename(L, lua_type(L, index))));
                    lua_error(L);
                }
                lua_pop(L, 1);
            }
            return ret;
        }
        else {
            luaw_push_error(L, std::format(
                    "Invalid parameter at index {}, failed conversion to [vku-bitmask] "
                    "object is neither table, integer or string: [{}]",
                    index, lua_typename(L, lua_type(L, index))));
            lua_error(L);
            return (T)0;
        }
    }
};

/* This resolves integers received from lua to an vku parameter */
template <std::integral Integer, ssize_t index>
struct luaw_param_t<Integer, index> {
    Integer luaw_single_param(lua_State *L) {
        int valid = 0;
        Integer ret = lua_tointegerx(L, index, &valid);
        if (!valid) {
            luaw_push_error(L,
                    std::format("Invalid parameter at index {}, failed conversion to integer from "
                    "[{}]",
                    index, lua_typename(L, lua_type(L, index))));
            lua_error(L);
        }
        return ret;
    }
};

/* This resolves floats received from lua to an vku parameter */
template <std::floating_point Float, ssize_t index>
struct luaw_param_t<Float, index> {
    Float luaw_single_param(lua_State *L) {
        int valid = 0;
        Float ret = lua_tonumberx(L, index, &valid);
        if (!valid) {
            luaw_push_error(L,
                    std::format("Invalid parameter at index {}, failed conversion to integer from "
                    "[{}]",
                    index, lua_typename(L, lua_type(L, index))));
            lua_error(L);
        }
        return ret;
    }
};

/* This resolves strings received from lua to an vku parameter */
template <ssize_t index>
struct luaw_param_t<char *, index> {
    char *luaw_single_param(lua_State *L) {
        char *ret = lua_tostring(L, index);
        if (!ret) {
            luaw_push_error(L,
                    std::format("Invalid parameter at index {}, failed conversion to string from "
                    "[{}]",
                    index, lua_typename(L, lua_type(L, index))));
            lua_error(L);
        }
        return ret;
    }
};

template <typename T>
struct de_bitmaptizize { using Type = T; }; 

template <typename T>
struct de_bitmaptizize<bm_t<T>> { using Type = T; };

template <typename ...Args>
struct de_bitmaptizize<std::tuple<Args...>> {
    using Type = std::tuple<typename de_bitmaptizize<Args>::Type...>;
};

template <typename T, typename U>
struct de_bitmaptizize<std::pair<T, U>> {
    using Type = std::pair<typename de_bitmaptizize<T>::Type, typename de_bitmaptizize<U>::Type>;
};

template <typename T>
struct de_bitmaptizize<std::vector<T>> {
    using Type = std::vector<typename de_bitmaptizize<T>::Type>;
};

template <typename ...Args, ssize_t index>
struct luaw_param_t<std::tuple<Args...>, index> {
    template <size_t ...I>
    auto _luaw_single_param_impl(lua_State *L, std::index_sequence<I...>) {
        typename de_bitmaptizize<std::tuple<Args...>>::Type ret;
        if (lua_isnil(L, index))
            return ret;
        if (!lua_istable(L, index)) {
            luaw_push_error(L, std::format("Invalid object of type: {} at index {}",
                    lua_typename(L, lua_type(L, index)), index));
            lua_error(L);
        }
        int abs_idx = lua_absindex(L, index);
        int len = lua_rawlen(L, index);
        for (int i = len; i >= 1; i--)
            lua_rawgeti(L, abs_idx, i);
        ret = typename de_bitmaptizize<std::tuple<Args...>>::Type{
                luaw_param_t<Args, -ssize_t(I)-1>{}.luaw_single_param(L)...};
        lua_pop(L, len);
        return ret;
    }

    auto luaw_single_param(lua_State *L) {
        return _luaw_single_param_impl(L, std::index_sequence_for<Args...>{});
    }
};

template <typename Arg1, typename Arg2, ssize_t index>
struct luaw_param_t<std::pair<Arg1, Arg2>, index> {
    auto luaw_single_param(lua_State *L) {
        auto tuple = luaw_param_t<std::tuple<Arg1, Arg2>, index>{}
                .luaw_single_param(L);
        typename de_bitmaptizize<std::pair<Arg1, Arg2>>::Type ret =
                {std::get<0>(tuple), std::get<1>(tuple)};
        return ret;
    }
};

template <typename T, ssize_t index>
struct luaw_param_t<std::vector<T>, index> {
    auto luaw_single_param(lua_State *L) {
        typename de_bitmaptizize<std::vector<T>>::Type ret;
        if (lua_isnil(L, index))
            return ret;
        if (!lua_istable(L, index)) {
            luaw_push_error(L, std::format("Invalid object of type: {} at index {}",
                    lua_typename(L, lua_type(L, index)), index));
            lua_error(L);
        }
        int len = lua_rawlen(L, index);
        for (int i = 1; i <= len; i++) {
            lua_rawgeti(L, index, i);
            ret.push_back(luaw_param_t<T, -1>{}.luaw_single_param(L));
            lua_pop(L, 1);
        }
        return ret;
    }
};

template <typename T>
struct luaw_returner_t {
    void luaw_ret_push(lua_State *L, T&& t) {
        (void)t;
        demangle_static_assert<false, T>(" - Is not a valid return type");
    }
};

template <std::integral Integer>
struct luaw_returner_t<Integer> {
    void luaw_ret_push(lua_State *L, Integer&& x) {
        lua_pushinteger(L, x);
    }
};

template <>
struct luaw_returner_t<void *> {
    void luaw_ret_push(lua_State *L, void *rawptr) {
        lua_pushlightuserdata(L, rawptr);
    }
};

template <auto function, typename ...Params, size_t ...I>
int luaw_function_wrapper_impl(lua_State *L, std::index_sequence<I...>) {
    using RetType = decltype(function(
            luaw_param_t<Params, I + 1>{}.luaw_single_param(L)...));

    if constexpr (std::is_void_v<RetType>) {
        function(luaw_param_t<Params, I + 1>{}.luaw_single_param(L)...);
        return 0;
    }
    else {
        luaw_returner_t<RetType>{}.luaw_ret_push(L, function(
                luaw_param_t<Params, I + 1>{}.luaw_single_param(L)...));
        return 1;
    }    
}

template <typename VkuT, auto member_ptr, typename ...Params, size_t ...I>
int luaw_member_function_wrapper_impl(lua_State *L, std::index_sequence<I...>) {
    int index = luaw_from_user_data(lua_touserdata(L, 1));
    if (index == 0) {
        luaw_push_error(L, "Nil user object can't call member function! (check if : used)");
        lua_error(L);
    }
    auto &o = g_rs.objects[index];
    if (!o.obj) {
        luaw_push_error(L, "internal_error: Nil user object can't call member function!");
        lua_error(L);
    }
    auto obj = o.obj.to_related<VkuT>();

    using RetType = decltype((obj.get()->*member_ptr)(
            luaw_param_t<Params, I + 2>{}.luaw_single_param(L)...));

    if constexpr (std::is_void_v<RetType>) {
        (obj.get()->*member_ptr)(luaw_param_t<Params, I + 2>{}.luaw_single_param(L)...);
        return 0;
    }
    else {
        luaw_returner_t<RetType>{}.luaw_ret_push(L, (obj.get()->*member_ptr)(
                luaw_param_t<Params, I + 2>{}.luaw_single_param(L)...));
        return 1;
    }    
}

inline vkc_error_e luaw_execute_window_resize(int width, int height);
int luaw_catch_exception(lua_State *L) {
    /* We don't let errors get out of the call because we don't want to break lua. As such, we catch
    any error and propagate it as a lua error. */
    try {
        throw ; // re-throw the current exception
    }
    catch (vku::err_t &vkerr) {
        if (vkerr.vk_err == VK_SUBOPTIMAL_KHR) {
            DBG("TODO: resize? Somehow...");
            ASSERT_FN(luaw_execute_window_resize(800, 600));
        }
        else {
            luaw_push_error(L, std::format("Invalid call: {}", vkerr.what()));
            lua_error(L);
        }
    }
    catch (fkyaml::exception &e) {
        luaw_push_error(L, std::format("fkyaml::exception: {}", e.what()));
        lua_error(L);
    }
    catch (std::exception &e) {
        luaw_push_error(L, std::format("std::exception: {}", e.what()));
        lua_error(L);
    }
    catch (...) {
        throw ; /* most probably the lua string */
    }

    return 0;
}

template <auto function, typename ...Params>
int luaw_function_wrapper(lua_State *L) {
    try {
        return luaw_function_wrapper_impl<function, Params...>(
                L, std::index_sequence_for<Params...>{});
    }
    catch (...) { return luaw_catch_exception(L); }
}

template <typename VkuT, auto member_ptr, typename ...Params>
int luaw_member_function_wrapper(lua_State *L) {
    try {
        return luaw_member_function_wrapper_impl<VkuT, member_ptr, Params...>(
                L, std::index_sequence_for<Params...>{});
    }
    catch (...) { return luaw_catch_exception(L); }
}


// helper to detect if a type is vku::ref_t<...>
template <typename>
struct is_vku_ref_t : std::false_type {};

template <typename T>
struct is_vku_ref_t<vku::ref_t<T>> : std::true_type {};

// helper to detect if a type is vku::ref_t<...>
template <typename T>
concept is_vku_enum = requires(fkyaml::node n) {
    get_enum_val<T>(n);
};

/* getter */
template <typename VkuT, auto member_ptr>
int luaw_member_object_wrapper(lua_State *L) {
    try {
        int index = luaw_from_user_data(lua_touserdata(L, -2)); /* an int, ok on unwind */
        if (index == 0) {
            luaw_push_error(L, "Nil user object can't get member!");
            lua_error(L);
        }
        auto &o = g_rs.objects[index];
        if (!o.obj) {
            luaw_push_error(L, "internal_error: Nil user object can't get member!");
            lua_error(L);
        }
        auto obj = o.obj.to_related<VkuT>();
        auto &member = obj.get()->*member_ptr;

        using member_type = std::decay_t<decltype(member)>;

        if constexpr (std::is_same_v<member_type, std::string>) {
            lua_pushstring(L, member.c_str());
            return 1;
        }
        else if constexpr (std::is_integral_v<member_type>) {
            lua_pushinteger(L, member);
            return 1;
        }
        else if constexpr (std::is_floating_point_v<member_type>) {
            lua_pushnumber(L, member);
            return 1;
        }
        else if constexpr (std::is_same_v<member_type, std::vector<std::string>>) {
            lua_createtable(L, member.size(), 0);
            for (size_t i = 1; auto &str : member) {
                lua_pushstring(L, str.c_str());
                lua_rawseti(L, -2, i++);
            }
            return 1;
        }
        else if constexpr (is_vku_enum<member_type>) {
            lua_pushnumber(L, (int)member);
            return 1;
        }
        else if constexpr (is_vku_ref_t<member_type>::value) {
            if (!member) {
                lua_pushnil(L);
                return 1;
            }
            if (!member->cbks) {
                luaw_push_error(L, "internal_error: How did this object get known to lua ?!");
                lua_error(L);
            }
            if (!member->cbks->usr_ptr) {
                /* So this object was no longer known by the lua side, we must resurect it */

                /* We first get it a new id */
                int new_id = g_rs.free_objects.back();
                g_rs.free_objects.pop_back();

                /* make it reference it's own id */
                member->cbks->usr_ptr = std::shared_ptr<void>((void *)(intptr_t)new_id, [](void *){});

                /* add it's lua-name-mapping and it's lua-id-mapping */
                std::string name = new_anon_name();
                g_rs.objects_map[name] = new_id;
                g_rs.objects[new_id].obj = member;
                g_rs.objects[new_id].name = name;
            }
            int member_id = (intptr_t)member->cbks->usr_ptr.get();
            if (member_id >= g_rs.objects.size() || member_id < 0) {
                luaw_push_error(L, "internal_error: Integrity check failed");
                lua_error(L);
            }
            lua_pushlightuserdata(L, luaw_to_user_data(member_id));
            luaL_setmetatable(L, "__vku_metatable");
            return 1;
        }
        else {
            demangle_static_assert<false, decltype(member)>(" - Is not a valid member type");
            return 0;
        }
    }
    catch (...) { return luaw_catch_exception(L); }
}

template <typename VkuT, auto member_ptr>
int luaw_member_setter_object_wrapper(lua_State *L) {
    int index = luaw_from_user_data(lua_touserdata(L, -3)); /* an int, ok on unwind */
    if (index == 0) {
        luaw_push_error(L, "Nil user object can't set member!");
        lua_error(L);
    }
    auto &o = g_rs.objects[index];
    if (!o.obj) {
        luaw_push_error(L, "internal_error: Nil user object can't set member!");
        lua_error(L);
    }
    auto obj = o.obj.to_related<VkuT>();
    auto &member = obj.get()->*member_ptr;

    using member_type = std::decay_t<decltype(member)>;

    if constexpr (std::is_same_v<member_type, std::string>) {
        const char *str = lua_tostring(L, -1);
        member = str ? str : "";
        return 0;
    }
    else if constexpr (std::is_integral_v<member_type>) {
        uint64_t val = lua_tointeger(L, -1);
        member = (member_type)val;
        return 0;
    }
    else if constexpr (std::is_floating_point_v<member_type>) {
        double val = lua_tonumber(L, -1);
        member = (member_type)val;
        return 0;
    }
    else if constexpr (std::is_same_v<member_type, std::vector<std::string>>) {
        if (!lua_istable(L, -1)) {
            luaw_push_error(L, "You need a table for this assignment!");
            lua_error(L);
        }
        int len = lua_rawlen(L, -1);
        std::vector<std::string> to_asign;
        for (int i = 1; i <= len; i++) {
            lua_rawgeti(L, -1, i);
            const char *str = lua_tostring(L, -1);
            to_asign.push_back(str ? str : "");
            lua_pop(L, 1);
        }
        member = to_asign;
        return 0;
    }
    else if constexpr (is_vku_enum<member_type>) {
        uint64_t val = lua_tointeger(L, -1);
        member = (member_type)val;
        return 0;
    }
    else if constexpr (is_vku_ref_t<member_type>::value) {
        int index = luaw_from_user_data(lua_touserdata(L, -1)); /* an int, ok on unwind */
        if (index == 0) {
            member = nullptr;
            return 0;
        }
        member = g_rs.objects[index].obj;
        return 0;
    }
    else {
        demangle_static_assert<false, decltype(member)>(" - Is not a valid member type");
        return 0;
    }
}

template <typename VkuT, auto member_ptr, typename ...Params>
void luaw_register_member_function(const char *function_name) {
    lua_class_members[VkuT::type_id_static()][function_name] = luaw_member_t{
        .fn = &luaw_member_function_wrapper<VkuT, member_ptr, Params...>,
        .member_type = LUAW_MEMBER_FUNCTION
    };
}

template <typename VkuT, auto member_ptr>
void luaw_register_member_object(const char *member_name) {
    lua_class_members[VkuT::type_id_static()][member_name] = luaw_member_t{
        .fn = &luaw_member_object_wrapper<VkuT, member_ptr>,
        .member_type = LUAW_MEMBER_OBJECT
    };

    lua_class_member_setters[VkuT::type_id_static()][member_name] =
            &luaw_member_setter_object_wrapper<VkuT, member_ptr>;
}

#define VKC_REG_MEMB(obj_type, memb)            \
vulkan_composer::luaw_register_member_object<   \
/* self        */   obj_type,                   \
/* member      */   &obj_type::memb>            \
/* member name */   (#memb)

#define VKC_REG_FN(obj_type, fn, ...)           \
vulkan_composer::luaw_register_member_function< \
/* self     */  obj_type,                       \
/* function */  &obj_type::fn,                  \
/* params   */  ##__VA_ARGS__>                  \
/* fn name  */  (#fn)

inline void glfw_pool_events() {
    glfwPollEvents();
}

inline uint32_t glfw_get_key(vku::ref_t<vku::window_t> window, uint32_t key) {
    if (!window)
        throw vku::err_t("Window parameter can't be null");
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

/* Lua is interesting... It seems that I can use next(#t) or next(nil) to check if the table is an
array or a dictionary + lua_rawlen to check for both. yadayada, I need to write it in code */
fkyaml::node create_yaml_from_lua_object(lua_State *L, int index) {
    index = lua_absindex(L, index);
    if (lua_isboolean(L, index)) {
        fkyaml::node ret(fkyaml::node_type::BOOLEAN);
        ret.as_bool() = lua_toboolean(L, index);
        return ret;
    }
    else if (lua_isinteger(L, index)) {
        fkyaml::node ret(fkyaml::node_type::INTEGER);
        ret.as_int() = lua_tointeger(L, index);
        return ret;
    }
    else if (lua_isnumber(L, index)) {
        fkyaml::node ret(fkyaml::node_type::FLOAT);
        ret.as_float() = lua_tonumber(L, index);
        return ret;
    }
    else if (lua_isstring(L, index)) {
        fkyaml::node ret(fkyaml::node_type::STRING);
        ret.as_str() = lua_tostring(L, index) ? lua_tostring(L, index) : "";
        return ret;
    }
    else if (lua_isnil(L, index)) {
        fkyaml::node ret(fkyaml::node_type::NULL_OBJECT);
        return ret;
    }
    else if (lua_istable(L, index)) {
        ; /* we continue bellow */
    }
    else {
        luaw_push_error(L, std::format("Unknown conversion from type: {} to yaml object",
                lua_typename(L, lua_type(L, index))));
        lua_error(L);
    }

    bool array_detected = false;
    bool dict_detected = false;
    int array_len;

    /* AFAIK only arrays have a rawlen */
    if ((array_len = lua_rawlen(L, index)) != 0)
        array_detected = true;

    /* Assuming that lua_next is continuous for arrays (next(t, k) -> k+1), we must do two things:
    First check if the first key is in the array, if not, than this table also has dict keys, else
    any potential dictionary key will be placed after the array. (continued bellow...) */
    lua_pushnil(L);
    if (lua_next(L, index) != 0) {
        if ((lua_type(L, -2) != LUA_TNUMBER || lua_tointeger(L, -2) < 1 ||
                lua_tointeger(L, -2) >= array_len))
        {
            dict_detected = true;
        }
        lua_pop(L, 2);
    }
    else return fkyaml::node{fkyaml::node_type::MAPPING}; /* If empty we return an empty table */

    /* (...continuation from above) As such, second we now check if any dictionary key exists after
    the array part. */
    if (array_detected) {
        lua_pushinteger(L, array_len);
        if (lua_next(L, index) != 0) {
            if ((lua_type(L, -2) != LUA_TNUMBER || lua_tointeger(L, -2) < 1 ||
                    lua_tointeger(L, -2) > array_len))
            {
                dict_detected = true;
            }
            lua_pop(L, 2);
        }
    }

    if (array_detected && dict_detected) {
        luaw_push_error(L, "Create object doesn't support tables with both a hash part and "
                "an array part");
        lua_error(L);
    }

    if (array_detected) {
        int len = lua_rawlen(L, index);
        fkyaml::node to_ret(fkyaml::node_type::SEQUENCE);
        for (int i = 1; i <= len; i++) {
            lua_rawgeti(L, index, i);
            auto to_add = create_yaml_from_lua_object(L, -1);
            to_ret.as_seq().push_back(to_add);
            lua_pop(L, 1);
        }
        return to_ret;
    }

    if (dict_detected) {
        lua_pushnil(L);
        fkyaml::node to_ret(fkyaml::node_type::MAPPING);
        while (lua_next(L, index) != 0) {
            const char *key = lua_tostring(L, -2);
            if (key) {
                auto to_add = create_yaml_from_lua_object(L, -1);
                to_ret[key] = to_add;
            }
            lua_pop(L, 1);
        }
        return to_ret;
    }

    luaw_push_error(L, "internal_error: shouldn't reach here");
    lua_error(L);
    return fkyaml::node{};
}

inline int internal_create_object(lua_State *L) {
    const char *name = lua_tostring(L, 1);
    if (!name) {
        luaw_push_error(L, "Error at index 1: first parameter must be a string, the tag of the "
                "object");
        lua_error(L);
    }
    auto object_description = create_yaml_from_lua_object(L, 2);

    /* We copy the whole objects ref state, such that for now we have an exact copy of the global
    vku namespace and we can reference it's objects. If we error out, the only references that will
    remain alive are those that where backed up by g_rs and if we don't error out, at the end we
    append the differences to g_rs. */
    ref_state_t ref_state = g_rs;

    DBG("create_object: %s", fkyaml::node::serialize(object_description).c_str());
    auto pool = co::create_pool();

    if (!object_description.contains("m_type")) {
        pool->sched(build_pseudo_object(&ref_state, name, object_description));
    }
    else {
        pool->sched(build_object(&ref_state, name, object_description));
    }

    if (pool->run() != co::RUN_OK) {
        luaw_push_error(L, "CO_OJECT_CREATOR: Failed to create the object");
        lua_error(L);
    }

    if (ref_state.wanted_objects.size()) {
        std::string unknown_objects = "[";
        for (auto &[k, v]: ref_state.wanted_objects) {
            unknown_objects += std::format("{}, ", k);
        }
        unknown_objects += "]";
        luaw_push_error(L, std::format("unknown objects: {}", unknown_objects));
        lua_error(L);
    }

    if (!has(ref_state.objects_map, name)) {
        luaw_push_error(L, "internal_error: Object is not found after creation");
        lua_error(L);
    }

    auto new_idx = g_rs.get_new(ref_state);

    DBG("Getting lua table...");

    /* Get back the vulkan_utils table */
    lua_rawgeti(L, LUA_REGISTRYINDEX, g_lua_table);

    for (int id : new_idx) {
        if (!ref_state.objects[id].obj) {
            DBG("Null user object?");
        }
        DBG("Registering object: %s with id: %d", ref_state.objects[id].name.c_str(), id);
        /* this makes vulkan_utils.key = object_id and sets it's metadata */
        lua_pushlightuserdata(L, luaw_to_user_data(id));
        luaL_setmetatable(L, "__vku_metatable");
        lua_setfield(L, -2, ref_state.objects[id].name.c_str());
    }

    lua_getfield(L, -1, name);
    lua_remove(L, -2); /* pops vulkan_utils table */

    /* actualize the global state */
    g_rs.append(ref_state);

    /* Eventual errors are catched outside of this function */
    return 1;
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

static std::vector<luaL_Reg> vku_tab_funcs = {
    {"glfw_pool_events",    luaw_function_wrapper<glfw_pool_events>},
    {"get_key",             luaw_function_wrapper<glfw_get_key,
            vku::ref_t<vku::window_t>, uint32_t>},
    {"signal_close",        luaw_function_wrapper<internal_signal_close>},
    {"aquire_next_img",     luaw_function_wrapper<internal_aquire_next_img,
            vku::ref_t<vku::swapchain_t>, vku::ref_t<vku::sem_t>>},
    {"submit_cmdbuff",      luaw_function_wrapper<vku::submit_cmdbuff,
            std::vector<std::pair<vku::ref_t<vku::sem_t>, bm_t<VkPipelineStageFlagBits>>>,
            vku::ref_t<vku::cmdbuff_t>, vku::ref_t<vku::fence_t>,
            std::vector<vku::ref_t<vku::sem_t>>>},
    {"present",             luaw_function_wrapper<vku::present,
            vku::ref_t<vku::swapchain_t>,
            std::vector<vku::ref_t<vku::sem_t>>,
            uint32_t>},
    {"wait_fences",         luaw_function_wrapper<vku::wait_fences,
            std::vector<vku::ref_t<vku::fence_t>>>},
    {"reset_fences",        luaw_function_wrapper<vku::reset_fences,
            std::vector<vku::ref_t<vku::fence_t>>>},
    {"device_wait_handle",  luaw_function_wrapper<internal_device_wait_handle,
            vku::ref_t<vku::device_t>>},
    {"create_object", internal_create_object},
    {"copy_from_cpu_to_gpu",luaw_function_wrapper<copy_from_cpu_to_gpu,
            vku::ref_t<vku::buffer_t>, void *, size_t, size_t>},
    {"copy_from_gpu_to_cpu",luaw_function_wrapper<copy_from_gpu_to_cpu,
            void *, vku::ref_t<vku::buffer_t>, size_t, size_t>},
};

inline void luaw_set_glfw_fields(lua_State *L);

void register_flag_mapping(lua_State *L, auto &mapping) {
    for (auto& [k, v] : mapping) {
        lua_pushinteger(L, (uint32_t)v);
        lua_setfield(L, -2, k.c_str());
    }
};

static std::vector<std::function<void(lua_State *L)>> cbk_register_mapping;
static std::vector<std::function<void(void)>> cbk_register_members;

inline int luaopen_vku(lua_State *L) {
    int top = lua_gettop(L);

    {
        /* This metatable describes a generic vkc/vku object inside lua. Practically, it expososes
        member objects and functions to lua. */
        luaL_newmetatable(L, "__vku_metatable");

        lua_pushcfunction(L, [](lua_State *L) {
            int id = luaw_from_user_data(lua_touserdata(L, -1)); /* an int, ok on unwind */
            auto &o = g_rs.objects[id]; /* a reference, ok on unwind? (if err) */
            if (!o.obj) {
                luaw_push_error(L, std::format("invalid object id: {}", id));
                lua_error(L);
            }
            lua_pushstring(L, o.obj->to_string().c_str());
            return 1;
        });
        lua_setfield(L, -2, "__tostring");

        /* params: 1.usrptr, 2.key -> returns: 1.value */
        lua_pushcfunction(L, [](lua_State *L) {
            // DBG("__index: %d", lua_gettop(L));
            int id = luaw_from_user_data(lua_touserdata(L, -2)); /* an int, ok on unwind */
            const char *member_name = lua_tostring(L, -1); /* an const char *, ok on unwind */

            // DBG("usr_id: %d", id);
            // DBG("member_name: %s", member_name);

            auto &o = g_rs.objects[id]; /* a reference, ok on unwind? (if err) */
            if (!o.obj) {
                luaw_push_error(L, std::format("invalid object id: {}", id));
                lua_error(L);
            }
            vku::object_type_e class_id = o.obj->type_id(); /* an int, still ok on unwind */
            if (class_id < 0 || class_id >= VKU_TYPE_CNT) {
                luaw_push_error(L, std::format("invalid class id: {}", vku::to_string(class_id)));
                lua_error(L);
            }
            if (!has(lua_class_members[class_id], member_name)) {
                if (strcmp(member_name, "rebuild") == 0) try {
                    o.obj.rebuild();
                    return 0;
                } catch (...) { return luaw_catch_exception(L); }
                luaw_push_error(L, std::format("class id {} doesn't have member: {}",
                        vku::to_string(class_id), member_name));
                lua_error(L);
            }
            auto &member = lua_class_members[class_id][member_name];
            if (member.member_type == LUAW_MEMBER_FUNCTION) {
                lua_pushcfunction(L, member.fn);
                return 1;
            }
            else if (member.member_type == LUAW_MEMBER_OBJECT) {
                return member.fn(L);
            } else {
                luaw_push_error(L, std::format("NOT IMPLEMENTED YET: non-function member access"));
                lua_error(L);
            }
            luaw_push_error(L, std::format("INTERNAL ERROR: shouldn't reach here"));
            lua_error(L);
            return 0;
        });
        lua_setfield(L, -2, "__index");

        /* params: 1.usrptr, 2.key, 3.value  */
        lua_pushcfunction(L, [](lua_State *L) {
            DBG("__newindex: %d", lua_gettop(L));
            int id = luaw_from_user_data(lua_touserdata(L, -3)); /* an int, ok on unwind */
            const char *member_name = lua_tostring(L, -2); /* an const char *, ok on unwind */

            DBG("usr_id: %d", id);
            DBG("member_name: %s", member_name);

            auto &o = g_rs.objects[id]; /* a reference, ok on unwind? (if err) */
            if (!o.obj) {
                luaw_push_error(L, std::format("invalid object id: {}", id));
                lua_error(L);
            }
            vku::object_type_e class_id = o.obj->type_id(); /* an int, still ok on unwind */
            if (class_id < 0 || class_id >= VKU_TYPE_CNT) {
                luaw_push_error(L, std::format("invalid class id: {}", vku::to_string(class_id)));
                lua_error(L);
            }
            if (!has(lua_class_member_setters[class_id], member_name)) {
                luaw_push_error(L, std::format("class id {} doesn't have member: {}",
                        vku::to_string(class_id), member_name));
                lua_error(L);
            }
            auto &member = lua_class_member_setters[class_id][member_name];
            return member(L);
        });
        lua_setfield(L, -2, "__newindex");

        /* params: 1.usrptr [... rest of params] */
        lua_pushcfunction(L, [](lua_State *L) {
            DBG("__call: %d", lua_gettop(L));
            int id = luaw_from_user_data(lua_touserdata(L, 1)); /* an int, ok on unwind */

            DBG("usr_id: %d", id);

            auto &o = g_rs.objects[id]; /* a reference, ok on unwind? (if err) */
            if (!o.obj) {
                luaw_push_error(L, std::format("invalid object id: {}", id));
                lua_error(L);
            }
            DBG("tostr: %s", o.obj->to_string().c_str());
            vku::object_type_e class_id = o.obj->type_id(); /* an int, still ok on unwind */
            if (class_id != VKC_TYPE_LUA_FUNCTION) {
                luaw_push_error(L, std::format("invalid class id: {} is not VKC_TYPE_LUA_FUNCTION",
                        vku::to_string(class_id)));
                lua_error(L);
            }
            return o.obj.to_related<lua_function_t>()->call(L);
        });
        lua_setfield(L, -2, "__call");

        /* params: 1.usrptr */
        lua_pushcfunction(L, [](lua_State *L) {
            DBG("__gc");

            int id = luaw_from_user_data(lua_touserdata(L, -1)); /* an int, ok on unwind */
            auto &o = g_rs.objects[id]; /* a reference, ok on unwind? (if err) */
            if (!o.obj) {
                return 0;
            }

            /* The object is no longer known to lua, as such we also delete it's slot. Obs: It may
            still be alive, meaning, it is known by the c++ side, just not by the lua side.
            !!! It will also loose it's name with this operation (Is that really ok?) */
            o.obj->cbks->usr_ptr = nullptr;

            /* we clean it's name mapping, it's reference and free it's id */
            g_rs.objects_map.erase(o.name);
            o = object_ref_t{};
            g_rs.free_objects.push_back(id);
            return 0;
        });
        lua_setfield(L, -2, "__gc");

        /* vku::window_t
        ----------------------------------------------------------------------------------------- */

        VKC_REG_MEMB(vku::window_t, m_name);
        VKC_REG_MEMB(vku::window_t, m_width);
        VKC_REG_MEMB(vku::window_t, m_height);

        /* vku::instance_t
        ----------------------------------------------------------------------------------------- */

        VKC_REG_MEMB(vku::instance_t, m_app_name);
        VKC_REG_MEMB(vku::instance_t, m_engine_name);
        VKC_REG_MEMB(vku::instance_t, m_extensions);
        VKC_REG_MEMB(vku::instance_t, m_layers);

        /* vku::cmdbuff_t
        ----------------------------------------------------------------------------------------- */

        VKC_REG_FN(vku::cmdbuff_t, begin, bm_t<VkCommandBufferUsageFlagBits>);
        VKC_REG_FN(vku::cmdbuff_t, begin_rpass, vku::ref_t<vku::framebuffs_t>, uint32_t);
        VKC_REG_FN(vku::cmdbuff_t, bind_vert_buffs,
                uint32_t, std::vector<std::pair<vku::ref_t<vku::buffer_t>, VkDeviceSize>>);
        VKC_REG_FN(vku::cmdbuff_t, bind_desc_set,
                bm_t<VkPipelineBindPoint>, vku::ref_t<vku::pipeline_t>, vku::ref_t<vku::desc_set_t>);
        VKC_REG_FN(vku::cmdbuff_t, bind_idx_buff,
                vku::ref_t<vku::buffer_t>, uint64_t, bm_t<VkIndexType>);
        VKC_REG_FN(vku::cmdbuff_t, draw, vku::ref_t<vku::pipeline_t>, uint64_t);
        VKC_REG_FN(vku::cmdbuff_t, draw_idx, vku::ref_t<vku::pipeline_t>, uint64_t);
        VKC_REG_FN(vku::cmdbuff_t, end_rpass);
        VKC_REG_FN(vku::cmdbuff_t, end);
        luaw_register_member_function<vku::cmdbuff_t, &vku::cmdbuff_t::end>("end_begin");
        VKC_REG_FN(vku::cmdbuff_t, reset);
        VKC_REG_FN(vku::cmdbuff_t, bind_compute, vku::ref_t<vku::compute_pipeline_t>);
        VKC_REG_FN(vku::cmdbuff_t, dispatch_compute, uint32_t, uint32_t, uint32_t);

        /* vkc::cpu_buffer_t
        ----------------------------------------------------------------------------------------- */
        VKC_REG_FN(vkc::cpu_buffer_t, data);
        VKC_REG_FN(vkc::cpu_buffer_t, size);

        /* all others:
        ----------------------------------------------------------------------------------------- */
        for (auto &cbk : cbk_register_members)
            cbk();

        /* Done objects
        ----------------------------------------------------------------------------------------- */

        lua_pushstring(L, "locked");
        lua_setfield(L, -2, "__metatable");

        lua_pop(L, 1); /* pop luaL_newmetatable */
    }

    DBG("top: %d gettop: %d", top, lua_gettop(L));
    ASSERT_FN(CHK_BOOL(top == lua_gettop(L))); /* sanity check */

    {
        /* Registers the vulkan_utils library and some standalone functions from vku(vulkan utils)
        or vkc(vulkan composer) */
        vku_tab_funcs.push_back({NULL, NULL});
        luaL_checkversion(L);
        lua_createtable(L, 0, vku_tab_funcs.size() - 1);
        luaL_setfuncs(L, vku_tab_funcs.data(), 0);

        /* Registers this lua table for later use */
        g_lua_table = luaL_ref(L, LUA_REGISTRYINDEX);
        lua_rawgeti(L, LUA_REGISTRYINDEX, g_lua_table);

        /* Registers glfw enum in this library */
        luaw_set_glfw_fields(L); /* This adds all glfw enums tokens */

        /* Registers objects loaded from the yaml confing as objects in the library */
        for (auto &[k, id] : g_rs.objects_map) {
            if (!g_rs.objects[id].obj) {
                DBG("Null user object?");
            }
            DBG("Registering object: %s with id: %d", k.c_str(), id);
            /* this makes vulkan_utils.key = object_id and sets it's metadata */
            lua_pushlightuserdata(L, luaw_to_user_data(id));
            luaL_setmetatable(L, "__vku_metatable");
            lua_setfield(L, -2, k.c_str());
        }

        /* Registers vulkan enums in the lua library */
        register_flag_mapping(L, vk_format_from_str);
        register_flag_mapping(L, vk_vertex_input_rate_from_str);
        register_flag_mapping(L, vk_shader_stage_flag_bits_from_str);
        register_flag_mapping(L, vk_descriptor_type_from_str);
        register_flag_mapping(L, vk_pipeline_stage_flag_bits_from_str);
        register_flag_mapping(L, vk_index_type_from_str);
        register_flag_mapping(L, vk_pipeline_bind_point_from_str);
        register_flag_mapping(L, vk_command_buffer_usage_flag_bits_from_str);
        register_flag_mapping(L, vk_image_aspect_flag_bits_from_str);
        register_flag_mapping(L, vk_primitive_topology_from_str);
        register_flag_mapping(L, vk_memory_property_flag_bits_from_str);
        register_flag_mapping(L, vk_sharing_mode_from_str);
        register_flag_mapping(L, vk_buffer_usage_flag_bits_from_str);
        register_flag_mapping(L, shader_stage_from_string);
        register_flag_mapping(L, vkc_constant_sizes_from_string);

        for (auto &cbk : cbk_register_mapping)
            cbk(L);
    }

    DBG("top: %d gettop: %d", top, lua_gettop(L));
    ASSERT_FN(CHK_BOOL(top + 1 == lua_gettop(L))); /* sanity check */

    return 1;
}


inline lua_State *lua_state;
inline vkc_error_e luaw_init() {
    lua_state = luaL_newstate();
    if (lua_state == NULL) {
        DBG("Failed to init lua");
        return VKC_ERROR_FAILED_LUA_INIT;
    }

    luaL_requiref(lua_state, "vulkan_utils", luaopen_vku, 1);       lua_pop(lua_state, 1);
    luaL_requiref(lua_state, LUA_GNAME, luaopen_base, 1);           lua_pop(lua_state, 1);
    luaL_requiref(lua_state, LUA_LOADLIBNAME, luaopen_package, 1);  lua_pop(lua_state, 1);
    luaL_requiref(lua_state, LUA_COLIBNAME, luaopen_coroutine, 1);  lua_pop(lua_state, 1);
    luaL_requiref(lua_state, LUA_TABLIBNAME, luaopen_table, 1);     lua_pop(lua_state, 1);
    luaL_requiref(lua_state, LUA_STRLIBNAME, luaopen_string, 1);    lua_pop(lua_state, 1);
    luaL_requiref(lua_state, LUA_MATHLIBNAME, luaopen_math, 1);     lua_pop(lua_state, 1);
    luaL_requiref(lua_state, LUA_UTF8LIBNAME, luaopen_utf8, 1);     lua_pop(lua_state, 1);
    luaL_requiref(lua_state, LUA_DBLIBNAME, luaopen_debug, 1);      lua_pop(lua_state, 1);

    /* We don't want lua to access our system, so we intentionally don't include those */
    // luaL_requiref(L, LUA_IOLIBNAME, luaopen_io, 1); lua_pop(L, 1);
    // luaL_requiref(L, LUA_OSLIBNAME, luaopen_os, 1); lua_pop(L, 1);

    if (!has(g_rs.objects_map, "lua_script")) {
        DBG("Config didn't provide a starter script to execute");
        return VKC_ERROR_GENERIC;
    }
    auto start_script = g_rs.objects[g_rs.objects_map["lua_script"]].obj.to_related<lua_script_t>();
    if (luaL_loadstring(lua_state, start_script->content.c_str()) != LUA_OK) {
        DBG("LUA Load Failed: \n%s", lua_tostring(lua_state, -1));
        lua_close(lua_state);
        return VKC_ERROR_FAILED_LUA_LOAD;
    }
    if (lua_pcall(lua_state, 0, 0, 0) != LUA_OK) {
        DBG("LUA Exec Failed: \n%s", lua_tostring(lua_state, -1));
        lua_close(lua_state);
        return VKC_ERROR_FAILED_LUA_EXEC;
    }
 
    return VKC_ERROR_OK;
}

inline vkc_error_e luaw_uninit() {
    lua_close(lua_state);
    return VKC_ERROR_OK;
}

inline vkc_error_e luaw_execute_loop_run() {
    lua_getglobal(lua_state, "on_loop_run");
    if (lua_pcall(lua_state, 0, 0, 0) != LUA_OK) {
        DBG("LUA luaw_execute_loop_run Failed: \n%s", lua_tostring(lua_state, -1));
        return VKC_ERROR_FAILED_CALL;
    }
    return VKC_ERROR_OK;
}

inline vkc_error_e luaw_execute_window_resize(int width, int height) {
    lua_getglobal(lua_state, "on_window_resize");
    lua_pushinteger(lua_state, width);
    lua_pushinteger(lua_state, height);
    if (lua_pcall(lua_state, 2, 0, 0) != LUA_OK) {
        DBG("LUA luaw_execute_window_resize Failed: \n%s", lua_tostring(lua_state, -1));
        return VKC_ERROR_FAILED_CALL;
    }
    return VKC_ERROR_OK;
}

inline void luaw_set_glfw_fields(lua_State *L) {
    lua_pushinteger(L, GLFW_VERSION_MAJOR);
    lua_setfield(L, -2, "GLFW_VERSION_MAJOR"); 
    lua_pushinteger(L, GLFW_VERSION_MINOR);
    lua_setfield(L, -2, "GLFW_VERSION_MINOR"); 
    lua_pushinteger(L, GLFW_VERSION_REVISION);
    lua_setfield(L, -2, "GLFW_VERSION_REVISION"); 
    lua_pushinteger(L, GLFW_TRUE);
    lua_setfield(L, -2, "GLFW_TRUE"); 
    lua_pushinteger(L, GLFW_FALSE);
    lua_setfield(L, -2, "GLFW_FALSE"); 
    lua_pushinteger(L, GLFW_RELEASE);
    lua_setfield(L, -2, "GLFW_RELEASE"); 
    lua_pushinteger(L, GLFW_PRESS);
    lua_setfield(L, -2, "GLFW_PRESS"); 
    lua_pushinteger(L, GLFW_REPEAT);
    lua_setfield(L, -2, "GLFW_REPEAT"); 
    lua_pushinteger(L, GLFW_HAT_CENTERED);
    lua_setfield(L, -2, "GLFW_HAT_CENTERED"); 
    lua_pushinteger(L, GLFW_HAT_UP);
    lua_setfield(L, -2, "GLFW_HAT_UP"); 
    lua_pushinteger(L, GLFW_HAT_RIGHT);
    lua_setfield(L, -2, "GLFW_HAT_RIGHT"); 
    lua_pushinteger(L, GLFW_HAT_DOWN);
    lua_setfield(L, -2, "GLFW_HAT_DOWN"); 
    lua_pushinteger(L, GLFW_HAT_LEFT);
    lua_setfield(L, -2, "GLFW_HAT_LEFT"); 
    lua_pushinteger(L, GLFW_HAT_RIGHT_UP);
    lua_setfield(L, -2, "GLFW_HAT_RIGHT_UP"); 
    lua_pushinteger(L, GLFW_HAT_RIGHT_DOWN);
    lua_setfield(L, -2, "GLFW_HAT_RIGHT_DOWN"); 
    lua_pushinteger(L, GLFW_HAT_LEFT_UP);
    lua_setfield(L, -2, "GLFW_HAT_LEFT_UP"); 
    lua_pushinteger(L, GLFW_HAT_LEFT_DOWN);
    lua_setfield(L, -2, "GLFW_HAT_LEFT_DOWN"); 
    lua_pushinteger(L, GLFW_KEY_UNKNOWN);
    lua_setfield(L, -2, "GLFW_KEY_UNKNOWN"); 
    lua_pushinteger(L, GLFW_KEY_SPACE);
    lua_setfield(L, -2, "GLFW_KEY_SPACE"); 
    lua_pushinteger(L, GLFW_KEY_APOSTROPHE);
    lua_setfield(L, -2, "GLFW_KEY_APOSTROPHE"); 
    lua_pushinteger(L, GLFW_KEY_COMMA);
    lua_setfield(L, -2, "GLFW_KEY_COMMA"); 
    lua_pushinteger(L, GLFW_KEY_MINUS);
    lua_setfield(L, -2, "GLFW_KEY_MINUS"); 
    lua_pushinteger(L, GLFW_KEY_PERIOD);
    lua_setfield(L, -2, "GLFW_KEY_PERIOD"); 
    lua_pushinteger(L, GLFW_KEY_SLASH);
    lua_setfield(L, -2, "GLFW_KEY_SLASH"); 
    lua_pushinteger(L, GLFW_KEY_0);
    lua_setfield(L, -2, "GLFW_KEY_0"); 
    lua_pushinteger(L, GLFW_KEY_1);
    lua_setfield(L, -2, "GLFW_KEY_1"); 
    lua_pushinteger(L, GLFW_KEY_2);
    lua_setfield(L, -2, "GLFW_KEY_2"); 
    lua_pushinteger(L, GLFW_KEY_3);
    lua_setfield(L, -2, "GLFW_KEY_3"); 
    lua_pushinteger(L, GLFW_KEY_4);
    lua_setfield(L, -2, "GLFW_KEY_4"); 
    lua_pushinteger(L, GLFW_KEY_5);
    lua_setfield(L, -2, "GLFW_KEY_5"); 
    lua_pushinteger(L, GLFW_KEY_6);
    lua_setfield(L, -2, "GLFW_KEY_6"); 
    lua_pushinteger(L, GLFW_KEY_7);
    lua_setfield(L, -2, "GLFW_KEY_7"); 
    lua_pushinteger(L, GLFW_KEY_8);
    lua_setfield(L, -2, "GLFW_KEY_8"); 
    lua_pushinteger(L, GLFW_KEY_9);
    lua_setfield(L, -2, "GLFW_KEY_9"); 
    lua_pushinteger(L, GLFW_KEY_SEMICOLON);
    lua_setfield(L, -2, "GLFW_KEY_SEMICOLON"); 
    lua_pushinteger(L, GLFW_KEY_EQUAL);
    lua_setfield(L, -2, "GLFW_KEY_EQUAL"); 
    lua_pushinteger(L, GLFW_KEY_A);
    lua_setfield(L, -2, "GLFW_KEY_A"); 
    lua_pushinteger(L, GLFW_KEY_B);
    lua_setfield(L, -2, "GLFW_KEY_B"); 
    lua_pushinteger(L, GLFW_KEY_C);
    lua_setfield(L, -2, "GLFW_KEY_C"); 
    lua_pushinteger(L, GLFW_KEY_D);
    lua_setfield(L, -2, "GLFW_KEY_D"); 
    lua_pushinteger(L, GLFW_KEY_E);
    lua_setfield(L, -2, "GLFW_KEY_E"); 
    lua_pushinteger(L, GLFW_KEY_F);
    lua_setfield(L, -2, "GLFW_KEY_F"); 
    lua_pushinteger(L, GLFW_KEY_G);
    lua_setfield(L, -2, "GLFW_KEY_G"); 
    lua_pushinteger(L, GLFW_KEY_H);
    lua_setfield(L, -2, "GLFW_KEY_H"); 
    lua_pushinteger(L, GLFW_KEY_I);
    lua_setfield(L, -2, "GLFW_KEY_I"); 
    lua_pushinteger(L, GLFW_KEY_J);
    lua_setfield(L, -2, "GLFW_KEY_J"); 
    lua_pushinteger(L, GLFW_KEY_K);
    lua_setfield(L, -2, "GLFW_KEY_K"); 
    lua_pushinteger(L, GLFW_KEY_L);
    lua_setfield(L, -2, "GLFW_KEY_L"); 
    lua_pushinteger(L, GLFW_KEY_M);
    lua_setfield(L, -2, "GLFW_KEY_M"); 
    lua_pushinteger(L, GLFW_KEY_N);
    lua_setfield(L, -2, "GLFW_KEY_N"); 
    lua_pushinteger(L, GLFW_KEY_O);
    lua_setfield(L, -2, "GLFW_KEY_O"); 
    lua_pushinteger(L, GLFW_KEY_P);
    lua_setfield(L, -2, "GLFW_KEY_P"); 
    lua_pushinteger(L, GLFW_KEY_Q);
    lua_setfield(L, -2, "GLFW_KEY_Q"); 
    lua_pushinteger(L, GLFW_KEY_R);
    lua_setfield(L, -2, "GLFW_KEY_R"); 
    lua_pushinteger(L, GLFW_KEY_S);
    lua_setfield(L, -2, "GLFW_KEY_S"); 
    lua_pushinteger(L, GLFW_KEY_T);
    lua_setfield(L, -2, "GLFW_KEY_T"); 
    lua_pushinteger(L, GLFW_KEY_U);
    lua_setfield(L, -2, "GLFW_KEY_U"); 
    lua_pushinteger(L, GLFW_KEY_V);
    lua_setfield(L, -2, "GLFW_KEY_V"); 
    lua_pushinteger(L, GLFW_KEY_W);
    lua_setfield(L, -2, "GLFW_KEY_W"); 
    lua_pushinteger(L, GLFW_KEY_X);
    lua_setfield(L, -2, "GLFW_KEY_X"); 
    lua_pushinteger(L, GLFW_KEY_Y);
    lua_setfield(L, -2, "GLFW_KEY_Y"); 
    lua_pushinteger(L, GLFW_KEY_Z);
    lua_setfield(L, -2, "GLFW_KEY_Z"); 
    lua_pushinteger(L, GLFW_KEY_LEFT_BRACKET);
    lua_setfield(L, -2, "GLFW_KEY_LEFT_BRACKET"); 
    lua_pushinteger(L, GLFW_KEY_BACKSLASH);
    lua_setfield(L, -2, "GLFW_KEY_BACKSLASH"); 
    lua_pushinteger(L, GLFW_KEY_RIGHT_BRACKET);
    lua_setfield(L, -2, "GLFW_KEY_RIGHT_BRACKET"); 
    lua_pushinteger(L, GLFW_KEY_GRAVE_ACCENT);
    lua_setfield(L, -2, "GLFW_KEY_GRAVE_ACCENT"); 
    lua_pushinteger(L, GLFW_KEY_WORLD_1);
    lua_setfield(L, -2, "GLFW_KEY_WORLD_1"); 
    lua_pushinteger(L, GLFW_KEY_WORLD_2);
    lua_setfield(L, -2, "GLFW_KEY_WORLD_2"); 
    lua_pushinteger(L, GLFW_KEY_ESCAPE);
    lua_setfield(L, -2, "GLFW_KEY_ESCAPE"); 
    lua_pushinteger(L, GLFW_KEY_ENTER);
    lua_setfield(L, -2, "GLFW_KEY_ENTER"); 
    lua_pushinteger(L, GLFW_KEY_TAB);
    lua_setfield(L, -2, "GLFW_KEY_TAB"); 
    lua_pushinteger(L, GLFW_KEY_BACKSPACE);
    lua_setfield(L, -2, "GLFW_KEY_BACKSPACE"); 
    lua_pushinteger(L, GLFW_KEY_INSERT);
    lua_setfield(L, -2, "GLFW_KEY_INSERT"); 
    lua_pushinteger(L, GLFW_KEY_DELETE);
    lua_setfield(L, -2, "GLFW_KEY_DELETE"); 
    lua_pushinteger(L, GLFW_KEY_RIGHT);
    lua_setfield(L, -2, "GLFW_KEY_RIGHT"); 
    lua_pushinteger(L, GLFW_KEY_LEFT);
    lua_setfield(L, -2, "GLFW_KEY_LEFT"); 
    lua_pushinteger(L, GLFW_KEY_DOWN);
    lua_setfield(L, -2, "GLFW_KEY_DOWN"); 
    lua_pushinteger(L, GLFW_KEY_UP);
    lua_setfield(L, -2, "GLFW_KEY_UP"); 
    lua_pushinteger(L, GLFW_KEY_PAGE_UP);
    lua_setfield(L, -2, "GLFW_KEY_PAGE_UP"); 
    lua_pushinteger(L, GLFW_KEY_PAGE_DOWN);
    lua_setfield(L, -2, "GLFW_KEY_PAGE_DOWN"); 
    lua_pushinteger(L, GLFW_KEY_HOME);
    lua_setfield(L, -2, "GLFW_KEY_HOME"); 
    lua_pushinteger(L, GLFW_KEY_END);
    lua_setfield(L, -2, "GLFW_KEY_END"); 
    lua_pushinteger(L, GLFW_KEY_CAPS_LOCK);
    lua_setfield(L, -2, "GLFW_KEY_CAPS_LOCK"); 
    lua_pushinteger(L, GLFW_KEY_SCROLL_LOCK);
    lua_setfield(L, -2, "GLFW_KEY_SCROLL_LOCK"); 
    lua_pushinteger(L, GLFW_KEY_NUM_LOCK);
    lua_setfield(L, -2, "GLFW_KEY_NUM_LOCK"); 
    lua_pushinteger(L, GLFW_KEY_PRINT_SCREEN);
    lua_setfield(L, -2, "GLFW_KEY_PRINT_SCREEN"); 
    lua_pushinteger(L, GLFW_KEY_PAUSE);
    lua_setfield(L, -2, "GLFW_KEY_PAUSE"); 
    lua_pushinteger(L, GLFW_KEY_F1);
    lua_setfield(L, -2, "GLFW_KEY_F1"); 
    lua_pushinteger(L, GLFW_KEY_F2);
    lua_setfield(L, -2, "GLFW_KEY_F2"); 
    lua_pushinteger(L, GLFW_KEY_F3);
    lua_setfield(L, -2, "GLFW_KEY_F3"); 
    lua_pushinteger(L, GLFW_KEY_F4);
    lua_setfield(L, -2, "GLFW_KEY_F4"); 
    lua_pushinteger(L, GLFW_KEY_F5);
    lua_setfield(L, -2, "GLFW_KEY_F5"); 
    lua_pushinteger(L, GLFW_KEY_F6);
    lua_setfield(L, -2, "GLFW_KEY_F6"); 
    lua_pushinteger(L, GLFW_KEY_F7);
    lua_setfield(L, -2, "GLFW_KEY_F7"); 
    lua_pushinteger(L, GLFW_KEY_F8);
    lua_setfield(L, -2, "GLFW_KEY_F8"); 
    lua_pushinteger(L, GLFW_KEY_F9);
    lua_setfield(L, -2, "GLFW_KEY_F9"); 
    lua_pushinteger(L, GLFW_KEY_F10);
    lua_setfield(L, -2, "GLFW_KEY_F10"); 
    lua_pushinteger(L, GLFW_KEY_F11);
    lua_setfield(L, -2, "GLFW_KEY_F11"); 
    lua_pushinteger(L, GLFW_KEY_F12);
    lua_setfield(L, -2, "GLFW_KEY_F12"); 
    lua_pushinteger(L, GLFW_KEY_F13);
    lua_setfield(L, -2, "GLFW_KEY_F13"); 
    lua_pushinteger(L, GLFW_KEY_F14);
    lua_setfield(L, -2, "GLFW_KEY_F14"); 
    lua_pushinteger(L, GLFW_KEY_F15);
    lua_setfield(L, -2, "GLFW_KEY_F15"); 
    lua_pushinteger(L, GLFW_KEY_F16);
    lua_setfield(L, -2, "GLFW_KEY_F16"); 
    lua_pushinteger(L, GLFW_KEY_F17);
    lua_setfield(L, -2, "GLFW_KEY_F17"); 
    lua_pushinteger(L, GLFW_KEY_F18);
    lua_setfield(L, -2, "GLFW_KEY_F18"); 
    lua_pushinteger(L, GLFW_KEY_F19);
    lua_setfield(L, -2, "GLFW_KEY_F19"); 
    lua_pushinteger(L, GLFW_KEY_F20);
    lua_setfield(L, -2, "GLFW_KEY_F20"); 
    lua_pushinteger(L, GLFW_KEY_F21);
    lua_setfield(L, -2, "GLFW_KEY_F21"); 
    lua_pushinteger(L, GLFW_KEY_F22);
    lua_setfield(L, -2, "GLFW_KEY_F22"); 
    lua_pushinteger(L, GLFW_KEY_F23);
    lua_setfield(L, -2, "GLFW_KEY_F23"); 
    lua_pushinteger(L, GLFW_KEY_F24);
    lua_setfield(L, -2, "GLFW_KEY_F24"); 
    lua_pushinteger(L, GLFW_KEY_F25);
    lua_setfield(L, -2, "GLFW_KEY_F25"); 
    lua_pushinteger(L, GLFW_KEY_KP_0);
    lua_setfield(L, -2, "GLFW_KEY_KP_0"); 
    lua_pushinteger(L, GLFW_KEY_KP_1);
    lua_setfield(L, -2, "GLFW_KEY_KP_1"); 
    lua_pushinteger(L, GLFW_KEY_KP_2);
    lua_setfield(L, -2, "GLFW_KEY_KP_2"); 
    lua_pushinteger(L, GLFW_KEY_KP_3);
    lua_setfield(L, -2, "GLFW_KEY_KP_3"); 
    lua_pushinteger(L, GLFW_KEY_KP_4);
    lua_setfield(L, -2, "GLFW_KEY_KP_4"); 
    lua_pushinteger(L, GLFW_KEY_KP_5);
    lua_setfield(L, -2, "GLFW_KEY_KP_5"); 
    lua_pushinteger(L, GLFW_KEY_KP_6);
    lua_setfield(L, -2, "GLFW_KEY_KP_6"); 
    lua_pushinteger(L, GLFW_KEY_KP_7);
    lua_setfield(L, -2, "GLFW_KEY_KP_7"); 
    lua_pushinteger(L, GLFW_KEY_KP_8);
    lua_setfield(L, -2, "GLFW_KEY_KP_8"); 
    lua_pushinteger(L, GLFW_KEY_KP_9);
    lua_setfield(L, -2, "GLFW_KEY_KP_9"); 
    lua_pushinteger(L, GLFW_KEY_KP_DECIMAL);
    lua_setfield(L, -2, "GLFW_KEY_KP_DECIMAL"); 
    lua_pushinteger(L, GLFW_KEY_KP_DIVIDE);
    lua_setfield(L, -2, "GLFW_KEY_KP_DIVIDE"); 
    lua_pushinteger(L, GLFW_KEY_KP_MULTIPLY);
    lua_setfield(L, -2, "GLFW_KEY_KP_MULTIPLY"); 
    lua_pushinteger(L, GLFW_KEY_KP_SUBTRACT);
    lua_setfield(L, -2, "GLFW_KEY_KP_SUBTRACT"); 
    lua_pushinteger(L, GLFW_KEY_KP_ADD);
    lua_setfield(L, -2, "GLFW_KEY_KP_ADD"); 
    lua_pushinteger(L, GLFW_KEY_KP_ENTER);
    lua_setfield(L, -2, "GLFW_KEY_KP_ENTER"); 
    lua_pushinteger(L, GLFW_KEY_KP_EQUAL);
    lua_setfield(L, -2, "GLFW_KEY_KP_EQUAL"); 
    lua_pushinteger(L, GLFW_KEY_LEFT_SHIFT);
    lua_setfield(L, -2, "GLFW_KEY_LEFT_SHIFT"); 
    lua_pushinteger(L, GLFW_KEY_LEFT_CONTROL);
    lua_setfield(L, -2, "GLFW_KEY_LEFT_CONTROL"); 
    lua_pushinteger(L, GLFW_KEY_LEFT_ALT);
    lua_setfield(L, -2, "GLFW_KEY_LEFT_ALT"); 
    lua_pushinteger(L, GLFW_KEY_LEFT_SUPER);
    lua_setfield(L, -2, "GLFW_KEY_LEFT_SUPER"); 
    lua_pushinteger(L, GLFW_KEY_RIGHT_SHIFT);
    lua_setfield(L, -2, "GLFW_KEY_RIGHT_SHIFT"); 
    lua_pushinteger(L, GLFW_KEY_RIGHT_CONTROL);
    lua_setfield(L, -2, "GLFW_KEY_RIGHT_CONTROL"); 
    lua_pushinteger(L, GLFW_KEY_RIGHT_ALT);
    lua_setfield(L, -2, "GLFW_KEY_RIGHT_ALT"); 
    lua_pushinteger(L, GLFW_KEY_RIGHT_SUPER);
    lua_setfield(L, -2, "GLFW_KEY_RIGHT_SUPER"); 
    lua_pushinteger(L, GLFW_KEY_MENU);
    lua_setfield(L, -2, "GLFW_KEY_MENU"); 
    lua_pushinteger(L, GLFW_KEY_LAST);
    lua_setfield(L, -2, "GLFW_KEY_LAST"); 
    lua_pushinteger(L, GLFW_MOD_SHIFT);
    lua_setfield(L, -2, "GLFW_MOD_SHIFT"); 
    lua_pushinteger(L, GLFW_MOD_CONTROL);
    lua_setfield(L, -2, "GLFW_MOD_CONTROL"); 
    lua_pushinteger(L, GLFW_MOD_ALT);
    lua_setfield(L, -2, "GLFW_MOD_ALT"); 
    lua_pushinteger(L, GLFW_MOD_SUPER);
    lua_setfield(L, -2, "GLFW_MOD_SUPER"); 
    lua_pushinteger(L, GLFW_MOD_CAPS_LOCK);
    lua_setfield(L, -2, "GLFW_MOD_CAPS_LOCK"); 
    lua_pushinteger(L, GLFW_MOD_NUM_LOCK);
    lua_setfield(L, -2, "GLFW_MOD_NUM_LOCK"); 
    lua_pushinteger(L, GLFW_MOUSE_BUTTON_1);
    lua_setfield(L, -2, "GLFW_MOUSE_BUTTON_1"); 
    lua_pushinteger(L, GLFW_MOUSE_BUTTON_2);
    lua_setfield(L, -2, "GLFW_MOUSE_BUTTON_2"); 
    lua_pushinteger(L, GLFW_MOUSE_BUTTON_3);
    lua_setfield(L, -2, "GLFW_MOUSE_BUTTON_3"); 
    lua_pushinteger(L, GLFW_MOUSE_BUTTON_4);
    lua_setfield(L, -2, "GLFW_MOUSE_BUTTON_4"); 
    lua_pushinteger(L, GLFW_MOUSE_BUTTON_5);
    lua_setfield(L, -2, "GLFW_MOUSE_BUTTON_5"); 
    lua_pushinteger(L, GLFW_MOUSE_BUTTON_6);
    lua_setfield(L, -2, "GLFW_MOUSE_BUTTON_6"); 
    lua_pushinteger(L, GLFW_MOUSE_BUTTON_7);
    lua_setfield(L, -2, "GLFW_MOUSE_BUTTON_7"); 
    lua_pushinteger(L, GLFW_MOUSE_BUTTON_8);
    lua_setfield(L, -2, "GLFW_MOUSE_BUTTON_8"); 
    lua_pushinteger(L, GLFW_MOUSE_BUTTON_LAST);
    lua_setfield(L, -2, "GLFW_MOUSE_BUTTON_LAST"); 
    lua_pushinteger(L, GLFW_MOUSE_BUTTON_LEFT);
    lua_setfield(L, -2, "GLFW_MOUSE_BUTTON_LEFT"); 
    lua_pushinteger(L, GLFW_MOUSE_BUTTON_RIGHT);
    lua_setfield(L, -2, "GLFW_MOUSE_BUTTON_RIGHT"); 
    lua_pushinteger(L, GLFW_MOUSE_BUTTON_MIDDLE);
    lua_setfield(L, -2, "GLFW_MOUSE_BUTTON_MIDDLE"); 
    lua_pushinteger(L, GLFW_JOYSTICK_1);
    lua_setfield(L, -2, "GLFW_JOYSTICK_1"); 
    lua_pushinteger(L, GLFW_JOYSTICK_2);
    lua_setfield(L, -2, "GLFW_JOYSTICK_2"); 
    lua_pushinteger(L, GLFW_JOYSTICK_3);
    lua_setfield(L, -2, "GLFW_JOYSTICK_3"); 
    lua_pushinteger(L, GLFW_JOYSTICK_4);
    lua_setfield(L, -2, "GLFW_JOYSTICK_4"); 
    lua_pushinteger(L, GLFW_JOYSTICK_5);
    lua_setfield(L, -2, "GLFW_JOYSTICK_5"); 
    lua_pushinteger(L, GLFW_JOYSTICK_6);
    lua_setfield(L, -2, "GLFW_JOYSTICK_6"); 
    lua_pushinteger(L, GLFW_JOYSTICK_7);
    lua_setfield(L, -2, "GLFW_JOYSTICK_7"); 
    lua_pushinteger(L, GLFW_JOYSTICK_8);
    lua_setfield(L, -2, "GLFW_JOYSTICK_8"); 
    lua_pushinteger(L, GLFW_JOYSTICK_9);
    lua_setfield(L, -2, "GLFW_JOYSTICK_9"); 
    lua_pushinteger(L, GLFW_JOYSTICK_10);
    lua_setfield(L, -2, "GLFW_JOYSTICK_10"); 
    lua_pushinteger(L, GLFW_JOYSTICK_11);
    lua_setfield(L, -2, "GLFW_JOYSTICK_11"); 
    lua_pushinteger(L, GLFW_JOYSTICK_12);
    lua_setfield(L, -2, "GLFW_JOYSTICK_12"); 
    lua_pushinteger(L, GLFW_JOYSTICK_13);
    lua_setfield(L, -2, "GLFW_JOYSTICK_13"); 
    lua_pushinteger(L, GLFW_JOYSTICK_14);
    lua_setfield(L, -2, "GLFW_JOYSTICK_14"); 
    lua_pushinteger(L, GLFW_JOYSTICK_15);
    lua_setfield(L, -2, "GLFW_JOYSTICK_15"); 
    lua_pushinteger(L, GLFW_JOYSTICK_16);
    lua_setfield(L, -2, "GLFW_JOYSTICK_16"); 
    lua_pushinteger(L, GLFW_JOYSTICK_LAST);
    lua_setfield(L, -2, "GLFW_JOYSTICK_LAST"); 
    lua_pushinteger(L, GLFW_GAMEPAD_BUTTON_A);
    lua_setfield(L, -2, "GLFW_GAMEPAD_BUTTON_A"); 
    lua_pushinteger(L, GLFW_GAMEPAD_BUTTON_B);
    lua_setfield(L, -2, "GLFW_GAMEPAD_BUTTON_B"); 
    lua_pushinteger(L, GLFW_GAMEPAD_BUTTON_X);
    lua_setfield(L, -2, "GLFW_GAMEPAD_BUTTON_X"); 
    lua_pushinteger(L, GLFW_GAMEPAD_BUTTON_Y);
    lua_setfield(L, -2, "GLFW_GAMEPAD_BUTTON_Y"); 
    lua_pushinteger(L, GLFW_GAMEPAD_BUTTON_LEFT_BUMPER);
    lua_setfield(L, -2, "GLFW_GAMEPAD_BUTTON_LEFT_BUMPER"); 
    lua_pushinteger(L, GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER);
    lua_setfield(L, -2, "GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER"); 
    lua_pushinteger(L, GLFW_GAMEPAD_BUTTON_BACK);
    lua_setfield(L, -2, "GLFW_GAMEPAD_BUTTON_BACK"); 
    lua_pushinteger(L, GLFW_GAMEPAD_BUTTON_START);
    lua_setfield(L, -2, "GLFW_GAMEPAD_BUTTON_START"); 
    lua_pushinteger(L, GLFW_GAMEPAD_BUTTON_GUIDE);
    lua_setfield(L, -2, "GLFW_GAMEPAD_BUTTON_GUIDE"); 
    lua_pushinteger(L, GLFW_GAMEPAD_BUTTON_LEFT_THUMB);
    lua_setfield(L, -2, "GLFW_GAMEPAD_BUTTON_LEFT_THUMB"); 
    lua_pushinteger(L, GLFW_GAMEPAD_BUTTON_RIGHT_THUMB);
    lua_setfield(L, -2, "GLFW_GAMEPAD_BUTTON_RIGHT_THUMB"); 
    lua_pushinteger(L, GLFW_GAMEPAD_BUTTON_DPAD_UP);
    lua_setfield(L, -2, "GLFW_GAMEPAD_BUTTON_DPAD_UP"); 
    lua_pushinteger(L, GLFW_GAMEPAD_BUTTON_DPAD_RIGHT);
    lua_setfield(L, -2, "GLFW_GAMEPAD_BUTTON_DPAD_RIGHT"); 
    lua_pushinteger(L, GLFW_GAMEPAD_BUTTON_DPAD_DOWN);
    lua_setfield(L, -2, "GLFW_GAMEPAD_BUTTON_DPAD_DOWN"); 
    lua_pushinteger(L, GLFW_GAMEPAD_BUTTON_DPAD_LEFT);
    lua_setfield(L, -2, "GLFW_GAMEPAD_BUTTON_DPAD_LEFT"); 
    lua_pushinteger(L, GLFW_GAMEPAD_BUTTON_LAST);
    lua_setfield(L, -2, "GLFW_GAMEPAD_BUTTON_LAST"); 
    lua_pushinteger(L, GLFW_GAMEPAD_BUTTON_CROSS);
    lua_setfield(L, -2, "GLFW_GAMEPAD_BUTTON_CROSS"); 
    lua_pushinteger(L, GLFW_GAMEPAD_BUTTON_CIRCLE);
    lua_setfield(L, -2, "GLFW_GAMEPAD_BUTTON_CIRCLE"); 
    lua_pushinteger(L, GLFW_GAMEPAD_BUTTON_SQUARE);
    lua_setfield(L, -2, "GLFW_GAMEPAD_BUTTON_SQUARE"); 
    lua_pushinteger(L, GLFW_GAMEPAD_BUTTON_TRIANGLE);
    lua_setfield(L, -2, "GLFW_GAMEPAD_BUTTON_TRIANGLE"); 
    lua_pushinteger(L, GLFW_GAMEPAD_AXIS_LEFT_X);
    lua_setfield(L, -2, "GLFW_GAMEPAD_AXIS_LEFT_X"); 
    lua_pushinteger(L, GLFW_GAMEPAD_AXIS_LEFT_Y);
    lua_setfield(L, -2, "GLFW_GAMEPAD_AXIS_LEFT_Y"); 
    lua_pushinteger(L, GLFW_GAMEPAD_AXIS_RIGHT_X);
    lua_setfield(L, -2, "GLFW_GAMEPAD_AXIS_RIGHT_X"); 
    lua_pushinteger(L, GLFW_GAMEPAD_AXIS_RIGHT_Y);
    lua_setfield(L, -2, "GLFW_GAMEPAD_AXIS_RIGHT_Y"); 
    lua_pushinteger(L, GLFW_GAMEPAD_AXIS_LEFT_TRIGGER);
    lua_setfield(L, -2, "GLFW_GAMEPAD_AXIS_LEFT_TRIGGER"); 
    lua_pushinteger(L, GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER);
    lua_setfield(L, -2, "GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER"); 
    lua_pushinteger(L, GLFW_GAMEPAD_AXIS_LAST);
    lua_setfield(L, -2, "GLFW_GAMEPAD_AXIS_LAST"); 
    lua_pushinteger(L, GLFW_NO_ERROR);
    lua_setfield(L, -2, "GLFW_NO_ERROR"); 
    lua_pushinteger(L, GLFW_NOT_INITIALIZED);
    lua_setfield(L, -2, "GLFW_NOT_INITIALIZED"); 
    lua_pushinteger(L, GLFW_NO_CURRENT_CONTEXT);
    lua_setfield(L, -2, "GLFW_NO_CURRENT_CONTEXT"); 
    lua_pushinteger(L, GLFW_INVALID_ENUM);
    lua_setfield(L, -2, "GLFW_INVALID_ENUM"); 
    lua_pushinteger(L, GLFW_INVALID_VALUE);
    lua_setfield(L, -2, "GLFW_INVALID_VALUE"); 
    lua_pushinteger(L, GLFW_OUT_OF_MEMORY);
    lua_setfield(L, -2, "GLFW_OUT_OF_MEMORY"); 
    lua_pushinteger(L, GLFW_API_UNAVAILABLE);
    lua_setfield(L, -2, "GLFW_API_UNAVAILABLE"); 
    lua_pushinteger(L, GLFW_VERSION_UNAVAILABLE);
    lua_setfield(L, -2, "GLFW_VERSION_UNAVAILABLE"); 
    lua_pushinteger(L, GLFW_PLATFORM_ERROR);
    lua_setfield(L, -2, "GLFW_PLATFORM_ERROR"); 
    lua_pushinteger(L, GLFW_FORMAT_UNAVAILABLE);
    lua_setfield(L, -2, "GLFW_FORMAT_UNAVAILABLE"); 
    lua_pushinteger(L, GLFW_NO_WINDOW_CONTEXT);
    lua_setfield(L, -2, "GLFW_NO_WINDOW_CONTEXT"); 
    lua_pushinteger(L, GLFW_FOCUSED);
    lua_setfield(L, -2, "GLFW_FOCUSED"); 
    lua_pushinteger(L, GLFW_ICONIFIED);
    lua_setfield(L, -2, "GLFW_ICONIFIED"); 
    lua_pushinteger(L, GLFW_RESIZABLE);
    lua_setfield(L, -2, "GLFW_RESIZABLE"); 
    lua_pushinteger(L, GLFW_VISIBLE);
    lua_setfield(L, -2, "GLFW_VISIBLE"); 
    lua_pushinteger(L, GLFW_DECORATED);
    lua_setfield(L, -2, "GLFW_DECORATED"); 
    lua_pushinteger(L, GLFW_AUTO_ICONIFY);
    lua_setfield(L, -2, "GLFW_AUTO_ICONIFY"); 
    lua_pushinteger(L, GLFW_FLOATING);
    lua_setfield(L, -2, "GLFW_FLOATING"); 
    lua_pushinteger(L, GLFW_MAXIMIZED);
    lua_setfield(L, -2, "GLFW_MAXIMIZED"); 
    lua_pushinteger(L, GLFW_CENTER_CURSOR);
    lua_setfield(L, -2, "GLFW_CENTER_CURSOR"); 
    lua_pushinteger(L, GLFW_TRANSPARENT_FRAMEBUFFER);
    lua_setfield(L, -2, "GLFW_TRANSPARENT_FRAMEBUFFER"); 
    lua_pushinteger(L, GLFW_HOVERED);
    lua_setfield(L, -2, "GLFW_HOVERED"); 
    lua_pushinteger(L, GLFW_FOCUS_ON_SHOW);
    lua_setfield(L, -2, "GLFW_FOCUS_ON_SHOW"); 
    lua_pushinteger(L, GLFW_RED_BITS);
    lua_setfield(L, -2, "GLFW_RED_BITS"); 
    lua_pushinteger(L, GLFW_GREEN_BITS);
    lua_setfield(L, -2, "GLFW_GREEN_BITS"); 
    lua_pushinteger(L, GLFW_BLUE_BITS);
    lua_setfield(L, -2, "GLFW_BLUE_BITS"); 
    lua_pushinteger(L, GLFW_ALPHA_BITS);
    lua_setfield(L, -2, "GLFW_ALPHA_BITS"); 
    lua_pushinteger(L, GLFW_DEPTH_BITS);
    lua_setfield(L, -2, "GLFW_DEPTH_BITS"); 
    lua_pushinteger(L, GLFW_STENCIL_BITS);
    lua_setfield(L, -2, "GLFW_STENCIL_BITS"); 
    lua_pushinteger(L, GLFW_ACCUM_RED_BITS);
    lua_setfield(L, -2, "GLFW_ACCUM_RED_BITS"); 
    lua_pushinteger(L, GLFW_ACCUM_GREEN_BITS);
    lua_setfield(L, -2, "GLFW_ACCUM_GREEN_BITS"); 
    lua_pushinteger(L, GLFW_ACCUM_BLUE_BITS);
    lua_setfield(L, -2, "GLFW_ACCUM_BLUE_BITS"); 
    lua_pushinteger(L, GLFW_ACCUM_ALPHA_BITS);
    lua_setfield(L, -2, "GLFW_ACCUM_ALPHA_BITS"); 
    lua_pushinteger(L, GLFW_AUX_BUFFERS);
    lua_setfield(L, -2, "GLFW_AUX_BUFFERS"); 
    lua_pushinteger(L, GLFW_STEREO);
    lua_setfield(L, -2, "GLFW_STEREO"); 
    lua_pushinteger(L, GLFW_SAMPLES);
    lua_setfield(L, -2, "GLFW_SAMPLES"); 
    lua_pushinteger(L, GLFW_SRGB_CAPABLE);
    lua_setfield(L, -2, "GLFW_SRGB_CAPABLE"); 
    lua_pushinteger(L, GLFW_REFRESH_RATE);
    lua_setfield(L, -2, "GLFW_REFRESH_RATE"); 
    lua_pushinteger(L, GLFW_DOUBLEBUFFER);
    lua_setfield(L, -2, "GLFW_DOUBLEBUFFER"); 
    lua_pushinteger(L, GLFW_CLIENT_API);
    lua_setfield(L, -2, "GLFW_CLIENT_API"); 
    lua_pushinteger(L, GLFW_CONTEXT_VERSION_MAJOR);
    lua_setfield(L, -2, "GLFW_CONTEXT_VERSION_MAJOR"); 
    lua_pushinteger(L, GLFW_CONTEXT_VERSION_MINOR);
    lua_setfield(L, -2, "GLFW_CONTEXT_VERSION_MINOR"); 
    lua_pushinteger(L, GLFW_CONTEXT_REVISION);
    lua_setfield(L, -2, "GLFW_CONTEXT_REVISION"); 
    lua_pushinteger(L, GLFW_CONTEXT_ROBUSTNESS);
    lua_setfield(L, -2, "GLFW_CONTEXT_ROBUSTNESS"); 
    lua_pushinteger(L, GLFW_OPENGL_FORWARD_COMPAT);
    lua_setfield(L, -2, "GLFW_OPENGL_FORWARD_COMPAT"); 
    lua_pushinteger(L, GLFW_OPENGL_DEBUG_CONTEXT);
    lua_setfield(L, -2, "GLFW_OPENGL_DEBUG_CONTEXT"); 
    lua_pushinteger(L, GLFW_OPENGL_PROFILE);
    lua_setfield(L, -2, "GLFW_OPENGL_PROFILE"); 
    lua_pushinteger(L, GLFW_CONTEXT_RELEASE_BEHAVIOR);
    lua_setfield(L, -2, "GLFW_CONTEXT_RELEASE_BEHAVIOR"); 
    lua_pushinteger(L, GLFW_CONTEXT_NO_ERROR);
    lua_setfield(L, -2, "GLFW_CONTEXT_NO_ERROR"); 
    lua_pushinteger(L, GLFW_CONTEXT_CREATION_API);
    lua_setfield(L, -2, "GLFW_CONTEXT_CREATION_API"); 
    lua_pushinteger(L, GLFW_SCALE_TO_MONITOR);
    lua_setfield(L, -2, "GLFW_SCALE_TO_MONITOR"); 
    lua_pushinteger(L, GLFW_COCOA_RETINA_FRAMEBUFFER);
    lua_setfield(L, -2, "GLFW_COCOA_RETINA_FRAMEBUFFER"); 
    lua_pushinteger(L, GLFW_COCOA_FRAME_NAME);
    lua_setfield(L, -2, "GLFW_COCOA_FRAME_NAME"); 
    lua_pushinteger(L, GLFW_COCOA_GRAPHICS_SWITCHING);
    lua_setfield(L, -2, "GLFW_COCOA_GRAPHICS_SWITCHING"); 
    lua_pushinteger(L, GLFW_X11_CLASS_NAME);
    lua_setfield(L, -2, "GLFW_X11_CLASS_NAME"); 
    lua_pushinteger(L, GLFW_X11_INSTANCE_NAME);
    lua_setfield(L, -2, "GLFW_X11_INSTANCE_NAME"); 
    lua_pushinteger(L, GLFW_NO_API);
    lua_setfield(L, -2, "GLFW_NO_API"); 
    lua_pushinteger(L, GLFW_OPENGL_API);
    lua_setfield(L, -2, "GLFW_OPENGL_API"); 
    lua_pushinteger(L, GLFW_OPENGL_ES_API);
    lua_setfield(L, -2, "GLFW_OPENGL_ES_API"); 
    lua_pushinteger(L, GLFW_NO_ROBUSTNESS);
    lua_setfield(L, -2, "GLFW_NO_ROBUSTNESS"); 
    lua_pushinteger(L, GLFW_NO_RESET_NOTIFICATION);
    lua_setfield(L, -2, "GLFW_NO_RESET_NOTIFICATION"); 
    lua_pushinteger(L, GLFW_LOSE_CONTEXT_ON_RESET);
    lua_setfield(L, -2, "GLFW_LOSE_CONTEXT_ON_RESET"); 
    lua_pushinteger(L, GLFW_OPENGL_ANY_PROFILE);
    lua_setfield(L, -2, "GLFW_OPENGL_ANY_PROFILE"); 
    lua_pushinteger(L, GLFW_OPENGL_CORE_PROFILE);
    lua_setfield(L, -2, "GLFW_OPENGL_CORE_PROFILE"); 
    lua_pushinteger(L, GLFW_OPENGL_COMPAT_PROFILE);
    lua_setfield(L, -2, "GLFW_OPENGL_COMPAT_PROFILE"); 
    lua_pushinteger(L, GLFW_CURSOR);
    lua_setfield(L, -2, "GLFW_CURSOR"); 
    lua_pushinteger(L, GLFW_STICKY_KEYS);
    lua_setfield(L, -2, "GLFW_STICKY_KEYS"); 
    lua_pushinteger(L, GLFW_STICKY_MOUSE_BUTTONS);
    lua_setfield(L, -2, "GLFW_STICKY_MOUSE_BUTTONS"); 
    lua_pushinteger(L, GLFW_LOCK_KEY_MODS);
    lua_setfield(L, -2, "GLFW_LOCK_KEY_MODS"); 
    lua_pushinteger(L, GLFW_RAW_MOUSE_MOTION);
    lua_setfield(L, -2, "GLFW_RAW_MOUSE_MOTION"); 
    lua_pushinteger(L, GLFW_CURSOR_NORMAL);
    lua_setfield(L, -2, "GLFW_CURSOR_NORMAL"); 
    lua_pushinteger(L, GLFW_CURSOR_HIDDEN);
    lua_setfield(L, -2, "GLFW_CURSOR_HIDDEN"); 
    lua_pushinteger(L, GLFW_CURSOR_DISABLED);
    lua_setfield(L, -2, "GLFW_CURSOR_DISABLED"); 
    lua_pushinteger(L, GLFW_ANY_RELEASE_BEHAVIOR);
    lua_setfield(L, -2, "GLFW_ANY_RELEASE_BEHAVIOR"); 
    lua_pushinteger(L, GLFW_RELEASE_BEHAVIOR_FLUSH);
    lua_setfield(L, -2, "GLFW_RELEASE_BEHAVIOR_FLUSH"); 
    lua_pushinteger(L, GLFW_RELEASE_BEHAVIOR_NONE);
    lua_setfield(L, -2, "GLFW_RELEASE_BEHAVIOR_NONE"); 
    lua_pushinteger(L, GLFW_NATIVE_CONTEXT_API);
    lua_setfield(L, -2, "GLFW_NATIVE_CONTEXT_API"); 
    lua_pushinteger(L, GLFW_EGL_CONTEXT_API);
    lua_setfield(L, -2, "GLFW_EGL_CONTEXT_API"); 
    lua_pushinteger(L, GLFW_OSMESA_CONTEXT_API);
    lua_setfield(L, -2, "GLFW_OSMESA_CONTEXT_API"); 
    lua_pushinteger(L, GLFW_ARROW_CURSOR);
    lua_setfield(L, -2, "GLFW_ARROW_CURSOR"); 
    lua_pushinteger(L, GLFW_IBEAM_CURSOR);
    lua_setfield(L, -2, "GLFW_IBEAM_CURSOR"); 
    lua_pushinteger(L, GLFW_CROSSHAIR_CURSOR);
    lua_setfield(L, -2, "GLFW_CROSSHAIR_CURSOR"); 
    lua_pushinteger(L, GLFW_HRESIZE_CURSOR);
    lua_setfield(L, -2, "GLFW_HRESIZE_CURSOR"); 
    lua_pushinteger(L, GLFW_VRESIZE_CURSOR);
    lua_setfield(L, -2, "GLFW_VRESIZE_CURSOR"); 
    lua_pushinteger(L, GLFW_HAND_CURSOR);
    lua_setfield(L, -2, "GLFW_HAND_CURSOR"); 
    lua_pushinteger(L, GLFW_CONNECTED);
    lua_setfield(L, -2, "GLFW_CONNECTED"); 
    lua_pushinteger(L, GLFW_DISCONNECTED);
    lua_setfield(L, -2, "GLFW_DISCONNECTED"); 
    lua_pushinteger(L, GLFW_JOYSTICK_HAT_BUTTONS);
    lua_setfield(L, -2, "GLFW_JOYSTICK_HAT_BUTTONS"); 
    lua_pushinteger(L, GLFW_COCOA_CHDIR_RESOURCES);
    lua_setfield(L, -2, "GLFW_COCOA_CHDIR_RESOURCES"); 
    lua_pushinteger(L, GLFW_COCOA_MENUBAR);
    lua_setfield(L, -2, "GLFW_COCOA_MENUBAR"); 
}

}; /* namespace vkc */

#endif
