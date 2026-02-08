#pragma once

#define FRAME_LIMITER 1

#define SHOW_NAMETABLE 1 // Doesn't work with WINDOWS_GDI -> 1

// If none of the renderer's below are 1, SDL is used by default. (IDK why the f*ck I designed SDL as default, as everything I had built upon was GDI.)
#define WINDOWS_GDI 1
#define GLFW 0