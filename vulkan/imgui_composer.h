#ifndef IMGUI_COMPOSER_H
#define IMGUI_COMPOSER_H

/* TODO: this...
I need some sort of script that will take imgui.h and create all the bindings
becase writing them myself is not going to work */

namespace imgui_composer
{

// namespace vo = virt_object;
// namespace vc = virt_composer;
// namespace imc = imgui_composer;

// inline int register_meta(vc::virt_state_t *vs) {
// 	DBG_SCOPE();

//     std::vector<luaL_Reg> ast_tab_funcs = {
//         // {"create_ast_node", vc::luaw_function_wrapper<create_ast_node, vc::bm_t<ast_node_e>>},
//         // {"create_ast_var",  vc::luaw_function_wrapper<create_ast_var,  const char *>},
//         // {"create_ast_int",  vc::luaw_function_wrapper<create_ast_int,  int64_t>},
//     };

//     ASSERT_FN(add_lua_tab_funcs(vs, ast_tab_funcs));
// }

}

#endif
