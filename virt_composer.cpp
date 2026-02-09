#include "virt_composer.h"

#include "co_utils.h"
#include "yaml.h"
#include "tinyexpr.h"

#define LUA_IMPL
#include "minilua.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <filesystem>

/* Max number of named references That are concomitent */
static constexpr const int MAX_NUMBER_OF_OBJECTS = 16384;

/* This should be the path of the application */
static std::string app_path = std::filesystem::canonical("./");


