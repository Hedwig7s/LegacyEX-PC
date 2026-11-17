#ifndef _WIN32
#include "platform/pc/PcFunctionHooks.hpp"

#include <stdio.h>

int PcHookFunction(const char* name, void*, void**)
{
    printf("[pc] native: could not hook \"%s\": native function hooks are unavailable on this platform\n",
           name != 0 ? name : "");
    return 0;
}
#endif
