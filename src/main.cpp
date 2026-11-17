// Identity Mask: hash editing; model restoration only after a game reset.
// Independent implementation; no copied mod code or SDK redistribution.
#include <windows.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <string>
#include <type_traits>
#include "Compatibility.h"

// This actual import makes Windows load ScriptHookV before our DllMain.
__declspec(dllimport) void scriptRegister(HMODULE, void(*)());
void startupLog(HMODULE instance, const char* line) {
    wchar_t path[MAX_PATH]{};
    if(!GetModuleFileNameW(instance,path,MAX_PATH)) return;
    wchar_t* slash = wcsrchr(path,L'\\');
    if(!slash) return;
    if(wcscpy_s(slash+1, MAX_PATH-static_cast<size_t>(slash+1-path), L"IdentityMask.log")) return;
    HANDLE f=CreateFileW(path,FILE_APPEND_DATA,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(f==INVALID_HANDLE_VALUE) return;
    DWORD written=0;
    WriteFile(f,line,static_cast<DWORD>(strlen(line)),&written,nullptr);
    WriteFile(f,"\r\n",2,&written,nullptr);
    CloseHandle(f);
}

namespace shv {
using U64 = std::uint64_t;
void (*unreg)(HMODULE) = nullptr;
void (*wait)(DWORD) = nullptr;
void (*init)(U64) = nullptr;
void (*push)(U64) = nullptr;
U64* (*call)() = nullptr;
unsigned char* (*entity)(int) = nullptr;
HMODULE owner = nullptr;
template<class T> bool bind(HMODULE m, T& fn, const char* decorated, const char* plain) {
    fn = reinterpret_cast<T>(GetProcAddress(m, decorated));
    if (!fn) fn = reinterpret_cast<T>(GetProcAddress(m, plain));
    if(!fn) { startupLog(owner,"Missing ScriptHookV export:"); startupLog(owner,decorated); }
    return fn != nullptr;
}
bool load() {
    auto m = GetModuleHandleW(L"ScriptHookV.dll");
    if (!m) { startupLog(owner,"ScriptHookV.dll not loaded"); return false; }
    // Memory editing is optional at startup: its absence must not hide the menu.
    bind(m, entity, "?getScriptHandleBaseAddress@@YAPEAEH@Z", "getScriptHandleBaseAddress");
    return bind(m, unreg, "?scriptUnregister@@YAXPEAUHINSTANCE__@@@Z", "scriptUnregister")
        && bind(m, wait, "?scriptWait@@YAXK@Z", "scriptWait")
        && bind(m, init, "?nativeInit@@YAX_K@Z", "nativeInit")
        && bind(m, push, "?nativePush64@@YAX_K@Z", "nativePush64")
        && bind(m, call, "?nativeCall@@YAPEA_KXZ", "nativeCall");
}
template<class T> U64 pack(T v) {
    if constexpr (std::is_pointer_v<T>) return reinterpret_cast<U64>(v);
    else if constexpr (std::is_same_v<T, float>) {
        std::uint32_t b; std::memcpy(&b, &v, 4); return b;
    } else return static_cast<U64>(v);
}
template<class R = void, class... A> R invoke(U64 hash, A... args) {
    init(hash); (push(pack(args)), ...); auto p = call();
    if constexpr (!std::is_void_v<R>) { R r{}; if(p) std::memcpy(&r, p, sizeof(R)); return r; }
}
}
using Hash = std::uint32_t;
constexpr Hash identities[] = {0x0D7114C9, 0x9B22DBAF, 0x9B810FA2};
constexpr const char* labels[] = {"Michael", "Franklin", "Trevor", "Restore original identity"};
HMODULE module;
std::wstring folder;
std::string status = "Choose the identity read by the game.";
int openKey = VK_F6;
bool menu = false;
int selection = 0;
std::array<bool, 256> previous{};

void log(const char* text) {
    FILE* f = nullptr;
    _wfopen_s(&f, (folder + L"IdentityMask.log").c_str(), L"a");
    if (f) { std::fprintf(f, "%llu %s\n", GetTickCount64(), text); std::fclose(f); }
}
void message(const char* text) { status = text; log(text); }
bool readMemory(std::uintptr_t address, void* out, SIZE_T size) {
    SIZE_T done = 0;
    return ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<void*>(address), out, size, &done) && done == size;
}
bool writeHash(std::uintptr_t address, Hash expected, Hash value) {
    // Never change protection of an unexpected/read-only address.
    MEMORY_BASIC_INFORMATION m{};
    if (!VirtualQuery(reinterpret_cast<void*>(address), &m, sizeof(m))) return false;
    if (m.State != MEM_COMMIT || (m.Protect & (PAGE_GUARD | PAGE_NOACCESS))) return false;
    if (!(m.Protect & (PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY))) return false;
    Hash current = 0;
    if (!readMemory(address, &current, sizeof(current)) || current != expected) return false;
    SIZE_T done = 0;
    return WriteProcessMemory(GetCurrentProcess(), reinterpret_cast<void*>(address), &value, sizeof(value), &done) && done == sizeof(value);
}
struct Patch {
    std::uintptr_t info = 0, vtable = 0, ped = 0;
    int handle = 0;
    Hash original = 0, applied = 0;
} patch;
bool monitorPatch = true;
bool restore() {
    if (!patch.info) return true;
    std::uintptr_t vt = 0;
    Hash current = 0;
    if (!readMemory(patch.info, &vt, sizeof(vt)) || vt != patch.vtable ||
        !readMemory(patch.info + compatibility::modelHashOffset, &current, sizeof(current))) {
        monitorPatch = false;
        message("Cannot restore: model memory is no longer valid. Restart GTA."); return false;
    }
    if (current != patch.original && !writeHash(patch.info + compatibility::modelHashOffset, patch.applied, patch.original)) {
        monitorPatch = false;
        message("Another script changed the hash. Restore stopped; restart GTA."); return false;
    }
    patch = {};
    monitorPatch = true;
    message("Original identity restored.");
    return true;
}
int playerPed() { return shv::invoke<int>(0xD80958FC74E988A6ULL); }
Hash model(int ped) { return shv::invoke<Hash>(0x9F47B058362C84B5ULL, ped); }
bool apply(Hash target) {
    if(!shv::entity) { message("Memory export unavailable. Check IdentityMask.log."); return false; }
    if (!restore()) return false;
    const int handle = playerPed();
    if (!handle || !shv::invoke<int>(0x7239B21A38F536BAULL, handle)) { message("Player unavailable."); return false; }
    const auto ped = reinterpret_cast<std::uintptr_t>(shv::entity(handle));
    std::uintptr_t info = 0, vt = 0;
    Hash original = 0;
    // Legacy and Enhanced: entity + 0x20 -> CBaseModelInfo; nameHash + 0x18.
    // Validate against the native result before any write; never scan/write guessed offsets.
    if (!ped || !readMemory(ped + compatibility::modelInfoOffset, &info, sizeof(info)) || !info ||
        !readMemory(info, &vt, sizeof(vt)) || !vt ||
        !readMemory(info + compatibility::modelHashOffset, &original, sizeof(original)) || !original || original != model(handle)) {
        message("Unrecognized layout: no changes applied."); return false;
    }
    if (original == target) { message("The model already has this identity."); return true; }
    if (!writeHash(info + compatibility::modelHashOffset, original, target)) { message("Hash write refused."); return false; }
    patch = {info, vt, ped, handle, original, target};
    if (model(handle) != target) {
        if (restore()) message("Hash verification failed: change reverted.");
        return false;
    }
    message("Identity applied. Mission model retention is active.");
    return true;
}
// Remember the real model BEFORE editing its hash. The retention profile is
// independent of Patch, because replay loading can destroy the old player ped.
struct Retention {
    bool active = false;
    Hash visual = 0, identity = 0;
    std::array<int,12> drawable{}, texture{}, palette{};
    std::array<int,8> prop{}, propTexture{};
    std::uintptr_t candidate = 0;
    ULONGLONG stableSince = 0, lastAttempt = 0, windowStart = 0;
    unsigned attempts = 0;
} retention;

void forgetRetention() {
    if(retention.active)
        shv::invoke(0xE532F5D78798DAABULL, retention.visual); // SET_MODEL_AS_NO_LONGER_NEEDED
    retention = {};
}
void selectIdentity(Hash target) {
    const int ped = playerPed();
    if(!ped || !shv::entity) { message("Player or memory export unavailable."); return; }
    // Undo the previous spoof before capturing/streaming the real model.
    if(!restore()) return;
    Retention next;
    next.visual = model(ped); next.identity = target;
    for(int i=0;i<12;++i) {
        next.drawable[i]=shv::invoke<int>(0x67F3780DD425D4FCULL,ped,i);
        next.texture[i]=shv::invoke<int>(0x04A355E041E004E6ULL,ped,i);
        next.palette[i]=shv::invoke<int>(0xE3DD5F2A84B42281ULL,ped,i);
    }
    for(int i=0;i<8;++i) {
        next.prop[i]=shv::invoke<int>(0x898CC20EA75BACD8ULL,ped,i);
        next.propTexture[i]=shv::invoke<int>(0xE131A28626F81AB2ULL,ped,i);
    }
    forgetRetention();
    // Keep the original model resident across a replay's loading screen.
    shv::invoke(0x963D27A58DF860ACULL,next.visual); // REQUEST_MODEL
    if(apply(target)) { next.active=true; retention=next; }
    else shv::invoke(0xE532F5D78798DAABULL,next.visual);
}

void retainMissionModel() {
    if(!retention.active || !shv::entity) return;
    const int ped=playerPed();
    if(!ped || !shv::invoke<int>(0x7239B21A38F536BAULL,ped)) return;
    const auto address=reinterpret_cast<std::uintptr_t>(shv::entity(ped));
    std::uintptr_t info=0;
    if(!address || !readMemory(address+compatibility::modelInfoOffset,&info,sizeof(info))) return;
    Hash current=model(ped);
    if(retention.visual==retention.identity && current==retention.visual) return;
    if(patch.info==info && patch.ped==address && current==patch.applied) {
        retention.candidate=0; return; // Already wearing the retained model/mask.
    }
    // Do not turn another protagonist into the selected character on a switch.
    if(current!=retention.identity && current!=retention.visual) {
        retention.candidate=0; return;
    }
    // Never replace a ped while the engine is loading/switching or in a cutscene.
    if(shv::invoke<int>(0xD9D2CFFF49FAB35FULL) || // IS_PLAYER_SWITCH_IN_PROGRESS
       shv::invoke<int>(0x991251AFC3981F84ULL) || // IS_CUTSCENE_ACTIVE
       shv::invoke<int>(0x10D0A8F259E93EC9ULL) || // GET_IS_LOADING_SCREEN_ACTIVE
       shv::invoke<int>(0x5A859503B0C08678ULL,ped,true)) { // IS_PED_DEAD_OR_DYING
        retention.candidate=0; return;
    }
    const auto now=GetTickCount64();
    if(retention.candidate!=address) {
        retention.candidate=address; retention.stableSince=now; return;
    }
    if(now-retention.stableSince<500 || now-retention.lastAttempt<2000) return;
    if(now-retention.windowStart>30000) { retention.windowStart=now; retention.attempts=0; }
    if(retention.attempts>=3) {
        forgetRetention();
        message("Mission keeps resetting the model. Retention stopped; select again to retry.");
        return;
    }
    retention.lastAttempt=now; ++retention.attempts;
    if(!restore()) { forgetRetention(); return; }
    current=model(ped);
    if(current!=retention.visual) {
        shv::invoke(0x963D27A58DF860ACULL,retention.visual);
        if(!shv::invoke<int>(0x98A4EB5D89A0C952ULL,retention.visual)) return; // HAS_MODEL_LOADED
        // Replay recreated the canonical protagonist. Restore the saved visual
        // model, reacquire its new handle, restore outfit, then spoof its hash.
        const int player=shv::invoke<int>(0x4F8644AF03D0E0D6ULL); // PLAYER_ID
        shv::invoke(0x00A1CADD00108836ULL,player,retention.visual); // SET_PLAYER_MODEL
        const int replacement=playerPed();
        if(!replacement || model(replacement)!=retention.visual) {
            forgetRetention(); message("Mission model restoration failed."); return;
        }
        for(int i=0;i<12;++i)
            shv::invoke(0x262B14F48D29DE80ULL,replacement,i,retention.drawable[i],retention.texture[i],retention.palette[i]);
        shv::invoke(0xCD8A7537A9B52F06ULL,replacement); // CLEAR_ALL_PED_PROPS
        for(int i=0;i<8;++i) if(retention.prop[i]>=0)
            shv::invoke(0x93376B65A266EB5FULL,replacement,i,retention.prop[i],retention.propTexture[i],true);
    }
    if(!apply(retention.identity)) { forgetRetention(); return; }
    retention.candidate=0;
    log("Mission model and identity reapplied.");
}

bool pressed(int key) {
    bool down = (GetAsyncKeyState(key) & 0x8000) != 0;
    bool edge = down && !previous[key]; previous[key] = down; return edge;
}
void text(float x, float y, float scale, const char* value, int r=240, int g=240, int b=245) {
    using shv::invoke;
    invoke(0x66E0276CC5F6B9DAULL, 0); // SET_TEXT_FONT
    invoke(0x07C837F9A01C34C9ULL, scale, scale);
    invoke(0xBE6B23FFA53FB442ULL, r,g,b,255);
    invoke(0x25FBB336DF1804CBULL, "STRING");
    invoke(0x6C188BE134E074AAULL, value);
    invoke(0xCD015E5BB0D96A57ULL, x,y,0);
}
void rect(float x,float y,float w,float h,int r,int g,int b,int a) {
    shv::invoke(0x3A618A217E5154F0ULL,x,y,w,h,r,g,b,a,0);
}
void draw() {
    rect(.19f,.12f,.34f,.065f,38,95,175,245);
    text(.032f,.097f,.55f,"IDENTITY MASK");
    rect(.19f,.265f,.34f,.225f,15,18,25,235);
    for (int i=0;i<4;++i) {
        float y=.16f+i*.042f;
        if(i==selection) rect(.19f,y+.017f,.33f,.04f,40,90,150,230);
        text(.032f,y,.36f,labels[i]);
    }
    char buffer[100];
    std::snprintf(buffer,sizeof(buffer),"Current hash: 0x%08X", model(playerPed()));
    text(.032f,.34f,.30f,buffer);
    rect(.27f,.43f,.50f,.095f,15,18,25,235);
    text(.032f,.39f,.27f,status.c_str());
    text(.032f,.44f,.27f,"Arrows / Num8-2: select | Enter / Num5: apply | Esc: close");
}
void ScriptMain() {
    wchar_t path[MAX_PATH]{}; GetModuleFileNameW(module,path,MAX_PATH);
    folder = path; folder = folder.substr(0,folder.find_last_of(L"\\/")+1);
    wchar_t gamePath[MAX_PATH]{};
    if(!GetModuleFileNameW(nullptr,gamePath,MAX_PATH)) {
        log("Unable to identify the game executable. Script stopped."); return;
    }
    const wchar_t* gameName=wcsrchr(gamePath,L'\\');
    gameName=gameName ? gameName+1 : gamePath;
    log(compatibility::edition);
    if(_wcsicmp(gameName,compatibility::executable)!=0) {
        log("Wrong edition package or renamed game executable. Install the matching Identity Mask ASI.");
        return;
    }
    openKey = static_cast<int>(GetPrivateProfileIntW(L"Menu",L"OpenKey",VK_F6,(folder+L"IdentityMask.ini").c_str()));
    if(openKey < VK_F1 || openKey > VK_F24) openKey = VK_F6;
    log("Identity Mask 0.4: ScriptMain running.");
    char keyLog[80]; std::snprintf(keyLog,sizeof(keyLog),"Menu virtual key: %d (F6 = 117)",openKey); log(keyLog);
    for (;;) {
        // Restore the old allocation first; the retention profile survives it.
        if(patch.info && monitorPatch) {
            const auto current = reinterpret_cast<std::uintptr_t>(shv::entity(playerPed()));
            std::uintptr_t info = 0;
            if(current != patch.ped || !readMemory(current+compatibility::modelInfoOffset,&info,sizeof(info)) || info != patch.info)
                restore();
        }
        retainMissionModel();
        const bool toggle=pressed(openKey);
        const bool up=pressed(VK_UP) | pressed(VK_NUMPAD8);
        const bool down=pressed(VK_DOWN) | pressed(VK_NUMPAD2);
        const bool enter=pressed(VK_RETURN) | pressed(VK_NUMPAD5);
        const bool escape=pressed(VK_ESCAPE);
        if(toggle) { menu=!menu; log(menu ? "Menu opened" : "Menu closed"); }
        if(menu) {
            // Disable game input only while menu is open (not keyboard polling).
            for(int control : {24,25,27,172,173,176,177,199,200,201,202})
                shv::invoke(0xFE99B66D079CF6BCULL,0,control,true);
            if(up) selection=(selection+3)%4;
            if(down) selection=(selection+1)%4;
            if(enter) {
                if(selection==3) { if(restore()) forgetRetention(); }
                else selectIdentity(identities[selection]);
            }
            if(escape) menu=false;
            if(menu) draw();
        }
        shv::wait(0);
    }
}
BOOL APIENTRY DllMain(HMODULE instance,DWORD reason,LPVOID) {
    if(reason==DLL_PROCESS_ATTACH) {
        module=instance; DisableThreadLibraryCalls(instance);
        shv::owner=instance;
        startupLog(instance,"Identity Mask 0.4: DLL attached");
        if(!shv::load()) { startupLog(instance,"Initialization failed; see missing export above"); return FALSE; }
        scriptRegister(instance,ScriptMain);
        startupLog(instance,"Script registered; waiting for ScriptMain");
    } else if(reason==DLL_PROCESS_DETACH && shv::unreg) {
        // Loader-lock path: only unregister. Restore via menu before hot-unloading.
        shv::unreg(instance);
    }
    return TRUE;
}
