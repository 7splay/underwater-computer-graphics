#define STB_IMAGE_IMPLEMENTATION
#ifdef _WIN32
#define STBI_WINDOWS_UTF8
#endif
#include "stb_image.h"

namespace {
struct StbFlipOnLoad {
  StbFlipOnLoad() { stbi_set_flip_vertically_on_load(1); }
};
const StbFlipOnLoad kStbFlipOnLoad;
}
