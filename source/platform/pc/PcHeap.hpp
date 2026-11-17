#ifndef PLATFORM_PC_PCHEAP_HPP
#define PLATFORM_PC_PCHEAP_HPP
#include <stddef.h>
extern char* gHeapStart;
extern char* gHeapEnd;
extern char* gHeapTop;

#if __SIZEOF_POINTER__ == 8
extern char* gLowHeapStart;
extern char* gLowHeapEnd;

extern "C" void PcCreateLowHeap(void* base, size_t size);
#endif

extern "C" void PcStartupLog(const char* text);
extern "C" int PcHeapCommit(char* top);
#endif
