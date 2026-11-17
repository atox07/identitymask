# Identity Mask 0.4 — GTA V

An ASI menu that changes the identity hash read by the game while keeping your current visible ped. Includes automatic model and identity retention after a mission replay recreates the player.

## Build and install

1. Upload this folder's CONTENTS to the repository root, including the hidden .github folder.
2. Open Actions > Build Identity Mask > Run workflow. The workflow runs manually only.
3. One manual workflow run compiles BOTH editions. Download IdentityMask-Legacy-Windows-x64 for Legacy or IdentityMask-Enhanced-Windows-x64 for Enhanced.
4. Place the chosen package's IdentityMask.asi and IdentityMask.ini beside GTA5.exe (Legacy) or GTA5_Enhanced.exe (Enhanced). Install one ASI per game directory; do not rename/install both builds together. Requires a compatible ScriptHookV.dll and ASI Loader.

Targets: GTA V Legacy and Enhanced x64 Story Mode. Each build checks the running executable against its edition and stops with a log message if the wrong package is installed. Enhanced support is implemented from source references but still requires in-game verification on your installation. See COMPATIBILITY.md. The menu and basic identity change in 0.2 were confirmed working by the user. The retention behavior and new dual-edition build in 0.4 have not been compiled or tested in GTA in this environment.

## Controls

| Action | Key |
| --- | --- |
| Open / close | F6 |
| Select | Up / Down or Num8 / Num2 |
| Apply | Enter or Num5 |
| Close | Esc |

OpenKey in IdentityMask.ini uses Windows virtual-key codes (F6 = 117). The menu contains Michael, Franklin, Trevor, and Restore original identity.

## Keeping a model in a replay

1. Use your trainer to choose the visible ped/outfit you want.
2. Open Identity Mask with F6 and select the identity required for the mission (for example, Michael).
3. Start the mission replay. Identity Mask remembers the real model hash, the selected identity, the 12 clothing components and 8 prop slots in RAM.
4. If the game recreates the matching protagonist, the plugin waits for loading, character switching and active cutscenes to finish. It restores the remembered visible model/outfit and reapplies the identity hash. If only the hash was reset, it only reapplies the hash.

There is no extra toggle: selecting an identity arms retention. Restore original identity cancels retention and restores the real hash; it does not switch your visible appearance back to a canonical protagonist. Select a different identity to replace the remembered profile. There is one active profile, not a separate replacement for every mission actor. A switch to a different protagonist does not automatically replace that protagonist.

Retention also reacts to matching model resets outside replay: there is no version-specific replay global or mission-script patch. It does not survive a game restart. Restore before using a trainer to choose a different model.

## Limits of mission retention

Mission and cutscene compatibility is experimental. The first cutscene can still show the game's forced model, because the plugin does not replace the player during an active cutscene. Separate cutscene actors are not replaced. Custom freemode face blends, tattoos and overlays are not captured: only the model, clothing components and props are retained.

When a replay replaces the visual model, restoring it requires SET_PLAYER_MODEL, which recreates the player ped. Mission scripts holding the previous ped reference, tasks and inventory can be affected. The initial menu selection still edits only the hash. The plugin stops retention if a mission repeatedly forces a reset (three attempts within 30 seconds) rather than replacing the ped continuously. Choose the identity again to retry. Compatibility with every mission is not guaranteed.

The hash is edited in shared CBaseModelInfo data (entity + 0x20, name hash + 0x18), with runtime validation against GET_ENTITY_MODEL before writing. Other peds using the same model can therefore see the changed hash. The mask does not replace every protagonist variable, voice, stat or special ability. Do not combine it with another hash editor or the original Character Swap.

## Diagnostics

IdentityMask.log is written beside the ASI. It records DLL loading, script registration, ScriptMain startup, menu input and retention outcomes. If absent, inspect asiloader.log and ScriptHookV.log. The GTA directory must allow log writes.

The 0.2 loader fix remains: an explicit scriptRegister import makes Windows load ScriptHookV before DllMain. Other exports are resolved dynamically. The model-memory export is optional for menu startup; if missing, applying an identity reports an error.

## Identity values

| Character | Model name | Hash |
| --- | --- | --- |
| Michael | player_zero | 0x0D7114C9 |
| Franklin | player_one | 0x9B22DBAF |
| Trevor | player_two | 0x9B810FA2 |

## Technical references

- https://www.dev-c.com/gtav/scripthookv/
- https://github.com/JupiterKasparov/GTAV_YOS/blob/main/scripthookv.pas
- https://github.com/cpast/SirenSetting_Limit_Remover/blob/master/ASI_lib/RageStructs.h
- https://github.com/citizenfx/fivem/blob/master/code/components/extra-natives-five/src/PedExtraNatives.cpp
- https://github.com/citizenfx/natives/blob/master/PLAYER/SetPlayerModel.md

Independent implementation. No original Character Swap code, binaries or game assets included. Build uses CMake/MSVC x64 on windows-2022, without an SDK download.

## Building locally

Configure once with `cmake -S . -B build -A x64`, then run `cmake --build build --config Release --parallel` to build both editions. Individual targets are `IdentityMask_Legacy` and `IdentityMask_Enhanced`.

The resulting files are `build/Release/Legacy/IdentityMask.asi` and `build/Release/Enhanced/IdentityMask.asi`. Menu controls, identity choices, model retention and manual-only Actions triggering are shared. No ScriptHookVDotNet installation is required; its public source was used only as a compatibility reference.
