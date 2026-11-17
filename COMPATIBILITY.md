# Legacy / Enhanced compatibility notes — Identity Mask 0.4

## Verified source facts

1. Alexander Blade announced Enhanced support for ScriptHookV:
   https://www.dev-c.com/post/scripthookv-enhanced
   Use a ScriptHookV runtime/ASI Loader compatible with the installed edition and game build. The runtime is not included in this project.

2. ScriptHookVDotNet Enhanced supports both editions and contains explicit implementations of the two memory reads used by Identity Mask. The reference below is pinned, not a moving branch:
   https://github.com/Chiheb-Bacha/ScriptHookVDotNetEnhanced/blob/b5314686ab534d9cec8914625a0f5ab2dde528ea/source/core/NativeMemory.cs#L5906

   - GetModelInfo: read model-info pointer at entity address + 0x20.
   - GetModelHashFromFwArcheType: read hash at model-info address + 0x18.
   - Both methods are explicitly marked "Unchanged in Enhanced" in that source.
   - The same file identifies Enhanced using GTA5_Enhanced.exe (line 109).

These facts support retaining the same two offset values for both builds. They do not establish compatibility with every game update or prove that shared model-hash mutation and mission replay behavior work on every Enhanced build. No unrelated offsets, hooks, script globals or memory patterns from that project are used.

## What this project changes

- CMake builds IdentityMask_Legacy and IdentityMask_Enhanced from the same implementation.
- A compile-time edition definition selects the required executable name. Each script logs its edition and rejects the wrong game executable before calling gameplay natives.
- Memory offsets are named constants in Compatibility.h, shared because the reference confirms they are unchanged.
- Every initial hash edit still validates the memory value against GET_ENTITY_MODEL before writing. Unknown layouts are refused rather than scanned or guessed.
- Windows loads ScriptHookV via the same explicit import already used by the working Legacy version. No new SDK or .NET dependency is introduced.
- One manual GitHub workflow builds both binaries and uploads two artifacts. They use the same installed filename in separate packages to prevent accidental double loading.

## Validation status

The user confirmed the 0.2 Legacy menu/basic identity swap worked. The 0.4 packaging, edition selection and source changes have been reviewed locally, but this environment has neither a usable C++ compiler nor a running GTA installation. Both GitHub builds and in-game Enhanced behavior must still be verified. This is an initial Enhanced port, not a claim of tested universal support.

To check Enhanced: install the Enhanced artifact, open F6, apply another identity without changing the visible ped, confirm the displayed hash changes, then restore it. Check the log reports Enhanced. Test replay retention separately; all replay/cutscene limitations in README.md remain.
