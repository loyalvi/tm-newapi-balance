#include <Windows.h>
#include "Plugin.h"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

#ifdef __cplusplus
extern "C" {
#endif

__declspec(dllexport) ITMPlugin* TMPluginGetInstance()
{
    return &CNewApiPlugin::Instance();
}

#ifdef __cplusplus
}
#endif
