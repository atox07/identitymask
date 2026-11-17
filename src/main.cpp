// Identity Mask: identity hash editing, without SET_PLAYER_MODEL.
// Independent implementation; no copied mod code or SDK redistribution.
#include <windows.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <string>
#include <type_traits>

namespace shv {
using U64 = std::uint64_t;
void (*reg)(HMODULE, void(*)()) = nullptr;
void (*unreg)(HMODULE) = nullptr;
void (*wait)(DWORD) = nullptr;
void (*init)(U64) = nullptr;
void (*push)(U64) = nullptr;
U64* (*call)() = nullptr;
unsigned char* (*entity)(int) = nullptr;
template<class T> bool bind(HMODULE m, T& fn, const char* decorated, const char* plain) {
    fn = reinterpret_cast<T>(GetProcAddress(m, decorated));
    if (!fn) fn = reinterpret_cast<T>(GetProcAddress(m, plain));
    return fn != nullptr;
}
bool load() {
    auto m = GetModuleHandleW(L"ScriptHookV.dll");
    if (!m) return false;
    return bind(m, reg, "?scriptRegister@@YAXPEAUHINSTANCE__@@P6AXXZ@Z", "scriptRegister")
        && bind(m, unreg, "?scriptUnregister@@YAXPEAUHINSTANCE__@@@Z", "scriptUnregister")
        && bind(m, wait, "?scriptWait@@YAXK@Z", "scriptWait")
        && bind(m, init, "?nativeInit@@YAX_K@Z", "nativeInit")
        && bind(m, push, "?nativePush64@@YAX_K@Z", "nativePush64")
        && bind(m, call, "?nativeCall@@YAPEA_KXZ", "nativeCall")
        && bind(m, entity, "?getScriptHandleBaseAddress@@YAPEAEH@Z", "getScriptHandleBaseAddress");
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
constexpr const char* labels[] = {"Michael", "Franklin", "Trevor", "Ripristina identita originale"};
HMODULE module;
std::wstring folder;
std::string status = "Scegli l'identita letta dal gioco.";
int openKey = VK_F8;
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
        !readMemory(patch.info + 0x18, &current, sizeof(current))) {
        monitorPatch = false;
        message("Ripristino impossibile: memoria modello non piu valida. Riavvia GTA."); return false;
    }
    if (current != patch.original && !writeHash(patch.info + 0x18, patch.applied, patch.original)) {
        monitorPatch = false;
        message("Hash cambiato da un altro script: ripristino sospeso. Riavvia GTA."); return false;
    }
    patch = {};
    monitorPatch = true;
    message("Identita originale ripristinata.");
    return true;
}
int playerPed() { return shv::invoke<int>(0xD80958FC74E988A6ULL); }
Hash model(int ped) { return shv::invoke<Hash>(0x9F47B058362C84B5ULL, ped); }
void apply(Hash target) {
    if (!restore()) return;
    const int handle = playerPed();
    if (!handle || !shv::invoke<int>(0x7239B21A38F536BAULL, handle)) { message("Giocatore non disponibile."); return; }
    const auto ped = reinterpret_cast<std::uintptr_t>(shv::entity(handle));
    std::uintptr_t info = 0, vt = 0;
    Hash original = 0;
    // Legacy layout: entity + 0x20 -> CBaseModelInfo; nameHash + 0x18.
    // Validate against the native result before any write; never scan/write guessed offsets.
    if (!ped || !readMemory(ped + 0x20, &info, sizeof(info)) || !info ||
        !readMemory(info, &vt, sizeof(vt)) || !vt ||
        !readMemory(info + 0x18, &original, sizeof(original)) || !original || original != model(handle)) {
        message("Layout non riconosciuto: nessuna modifica applicata."); return;
    }
    if (original == target) { message("Il modello ha gia questa identita."); return; }
    if (!writeHash(info + 0x18, original, target)) { message("Scrittura hash rifiutata."); return; }
    patch = {info, vt, ped, handle, original, target};
    if (model(handle) != target) {
        if (restore()) message("Verifica hash fallita: modifica annullata.");
        return;
    }
    message("Identita applicata. Il modello visibile non viene sostituito.");
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
    std::snprintf(buffer,sizeof(buffer),"Hash letto: 0x%08X", model(playerPed()));
    text(.032f,.34f,.30f,buffer);
    rect(.27f,.43f,.50f,.095f,15,18,25,235);
    text(.032f,.39f,.27f,status.c_str());
    text(.032f,.44f,.27f,"Frecce / Num8-2: scegli | Invio / Num5: applica | Esc: chiudi");
}
void ScriptMain() {
    wchar_t path[MAX_PATH]{}; GetModuleFileNameW(module,path,MAX_PATH);
    folder = path; folder = folder.substr(0,folder.find_last_of(L"\\/")+1);
    openKey = static_cast<int>(GetPrivateProfileIntW(L"Menu",L"OpenKey",VK_F8,(folder+L"IdentityMask.ini").c_str()));
    if(openKey < VK_F1 || openKey > VK_F24) openKey = VK_F8;
    log("Identity Mask 0.1 started; Legacy experimental layout.");
    for (;;) {
        // Never carry the old mask silently to a different ped or a loaded save.
        if(patch.info && monitorPatch) {
            const auto current = reinterpret_cast<std::uintptr_t>(shv::entity(playerPed()));
            std::uintptr_t info = 0;
            if(current != patch.ped || !readMemory(current+0x20,&info,sizeof(info)) || info != patch.info)
                restore();
        }
        const bool toggle=pressed(openKey);
        const bool up=pressed(VK_UP) | pressed(VK_NUMPAD8);
        const bool down=pressed(VK_DOWN) | pressed(VK_NUMPAD2);
        const bool enter=pressed(VK_RETURN) | pressed(VK_NUMPAD5);
        const bool escape=pressed(VK_ESCAPE);
        if(toggle) menu=!menu;
        if(menu) {
            // Disable game input only while menu is open (not keyboard polling).
            for(int control : {24,25,27,172,173,176,177,199,200,201,202})
                shv::invoke(0xFE99B66D079CF6BCULL,0,control,true);
            if(up) selection=(selection+3)%4;
            if(down) selection=(selection+1)%4;
            if(enter) { if(selection==3) restore(); else apply(identities[selection]); }
            if(escape) menu=false;
            if(menu) draw();
        }
        shv::wait(0);
    }
}
BOOL APIENTRY DllMain(HMODULE instance,DWORD reason,LPVOID) {
    if(reason==DLL_PROCESS_ATTACH) {
        module=instance; DisableThreadLibraryCalls(instance);
        if(!shv::load()) { OutputDebugStringA("Identity Mask: missing ScriptHookV exports\n"); return FALSE; }
        shv::reg(instance,ScriptMain);
    } else if(reason==DLL_PROCESS_DETACH && shv::unreg) {
        // Loader-lock path: only unregister. Restore via menu before hot-unloading.
        shv::unreg(instance);
    }
    return TRUE;
}
