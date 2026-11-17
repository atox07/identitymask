#pragma once
#include <cstddef>

// Independent layout declarations, verified against NativeMemory.cs in
// Chiheb-Bacha/ScriptHookVDotNetEnhanced, commit
// b5314686ab534d9cec8914625a0f5ab2dde528ea:
// GetModelInfo and GetModelHashFromFwArcheType explicitly say
// "Unchanged in Enhanced". Keep separate edition build targets, not guesses.
namespace compatibility {
inline constexpr std::size_t modelInfoOffset = 0x20;
inline constexpr std::size_t modelHashOffset = 0x18;
#if defined(IDENTITYMASK_ENHANCED)
inline constexpr const char* edition = "Enhanced";
inline constexpr const wchar_t* executable = L"GTA5_Enhanced.exe";
#elif defined(IDENTITYMASK_LEGACY)
inline constexpr const char* edition = "Legacy";
inline constexpr const wchar_t* executable = L"GTA5.exe";
#else
#error Select an Identity Mask edition through the CMake targets
#endif
}
