#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <RE/Fallout.h>
#include <F4SE/F4SE.h>
#include <REX/REX.h>
#include <Scaleform/Scaleform.h>

#include <winsock2.h>
#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>

#ifdef ERROR
#undef ERROR
#endif
#ifdef GetObject
#undef GetObject
#endif
#ifdef GetMessage
#undef GetMessage
#endif

#include <algorithm>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <format>
#include <filesystem>
#include <functional>

#define MAKE_EXE_VERSION_EX(major, minor, build, sub) ((((major) & 0xFF) << 24) | (((minor) & 0xFF) << 16) | (((build) & 0xFFF) << 4) | ((sub) & 0xF))
#define MAKE_EXE_VERSION(major, minor, build)         MAKE_EXE_VERSION_EX(major, minor, build, 0)

using namespace std::literals;
using namespace REL;
using namespace REX;
using namespace F4SE;