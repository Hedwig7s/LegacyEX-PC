#ifndef _WIN32
#include "platform/pc/PcNativeMods.hpp"

#include <stddef.h>

void PcNativeModsSetEnabled(bool)
{
}

void PcNativeModsRegister()
{
}

void PcNativeModsLoad()
{
}

const char* PcNativeModsIdFor(void*)
{
    return 0;
}

std::string PcNativeModsDirectoryOf(const char*)
{
    return std::string();
}

const char* PcNativeModsCurrentId()
{
    return "";
}

bool PcNativeModsHudHidden(int)
{
    return false;
}

size_t PcNativeModsCurrent()
{
    return static_cast<size_t>(-1);
}

void PcNativeModsSetCurrent(size_t)
{
}

void PcNativeModsFrame()
{
}

void PcNativeModsHostStart()
{
}

void PcNativeModsHostTick()
{
}

void PcNativeModsHostStop()
{
}

void* PcNativeModsFindFunction(const char*, std::string* found, std::string* err)
{
    if (found != 0)
        found->clear();
    if (err != 0)
        *err = "native mods are only supported on Windows";
    return 0;
}

bool PcNativeModsHideMenus()
{
    return false;
}

bool PcNativeModsWantHud()
{
    return false;
}

void PcNativeModsHud(std::vector<PcScriptHudItem>*)
{
}

bool PcNativeModsWantViews()
{
    return false;
}

void PcNativeModsViewFrame(PcNativeViews*)
{
}

void PcNativeModsViewDone(PcNativeViews*)
{
}

bool PcNativeModsWantViewDraw()
{
    return false;
}

void PcNativeModsViewDraw(PcNativeViews*)
{
}
#endif
