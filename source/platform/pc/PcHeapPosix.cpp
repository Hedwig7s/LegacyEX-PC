#ifndef _WIN32

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include "PcHeap.hpp"

char* gHeapStart = 0;
char* gHeapEnd = 0;
char* gHeapTop = 0;
#if __SIZEOF_POINTER__ == 8
char* gLowHeapStart = 0;
char* gLowHeapEnd = 0;
#endif

namespace
{
const size_t kMegabyte = static_cast<size_t>(1024) * 1024;
#if __SIZEOF_POINTER__ == 8
const size_t kArenaSizes[] = {16384 * kMegabyte, 8192 * kMegabyte, 4096 * kMegabyte, 2048 * kMegabyte,
                              1536 * kMegabyte,  1024 * kMegabyte, 512 * kMegabyte,  384 * kMegabyte};
const size_t kLowHeapSize = 256 * kMegabyte;
const size_t kLowHeapLimit = static_cast<size_t>(4096) * kMegabyte;
#else
const size_t kArenaSizes[] = {1536 * kMegabyte, 1024 * kMegabyte, 768 * kMegabyte, 512 * kMegabyte, 384 * kMegabyte};
#endif
const size_t kCommitStep = 32 * kMegabyte;
const size_t kInitialCommit = 1024 * kMegabyte;
char* s_committedEnd = 0;

bool ExeSiblingPath(const char* name, char* path, size_t capacity) // This'll become a problem for any form of packaging
{
    ssize_t length = readlink("/proc/self/exe", path, capacity - 1);
    if (length <= 0)
        return false;
    while (length > 0 && path[length - 1] != '/')
        --length;
    if (static_cast<size_t>(length) + strlen(name) + 1 > capacity)
        return false;
    strcpy(path + length, name);
    return true;
}


bool WriteAll(int fd, const char* data, size_t length)
{
    while (length != 0)
    {
        ssize_t written = write(fd, data, length);

        if (written < 0)
        {
            if (errno == EINTR)
                continue;
            return false;
        }

        if (written == 0)
            return false;

        data += written;
        length -= static_cast<size_t>(written);
    }

    return true;
}

void StartupLog(const char* text, bool truncate)
{
    char path[4096];

    if (!ExeSiblingPath("startup.log", path, sizeof(path)))
        return;

    int flags = O_WRONLY | O_CREAT | (truncate ? O_TRUNC : O_APPEND);
    int fd = open(path, flags, 0666);

    if (fd < 0)
        return;

    WriteAll(fd, text, strlen(text));
    WriteAll(fd, "\n", 1);

    close(fd);
}

bool ReadHeapSettings(size_t* megabytes, bool* commitAll)
{
    char text[128];

    const char* environment = getenv("MCLCE_HEAP");

    if (environment != 0 && strlen(environment) < sizeof(text))
    {
        memcpy(text, environment, strlen(environment) + 1);
    }
    else
    {
        char path[4096];

        if (!ExeSiblingPath("heap.txt", path, sizeof(path)))
            return false;

        int fd = open(path, O_RDONLY);

        if (fd < 0)
            return false;

        size_t total = 0;

        while (total < sizeof(text) - 1)
        {
            ssize_t count = read(fd, text + total, sizeof(text) - 1 - total);

            if (count < 0)
            {
                if (errno == EINTR)
                    continue;

                close(fd);
                return false;
            }

            if (count == 0)
                break;

            total += static_cast<size_t>(count);
        }

        close(fd);
        text[total] = 0;
    }

    *megabytes = static_cast<size_t>(strtoul(text + strcspn(text, "0123456789"), 0, 10));

    *commitAll = strstr(text, "commit") != 0;

    return true;
}

void LogAddressSpace()
{
    FILE* maps = fopen("/proc/self/maps", "r");
    if (maps == 0)
        return;
#if __SIZEOF_POINTER__ == 8
    const unsigned long long userLimit = 1ULL << 47;
#else
    const unsigned long long userLimit = 0xC0000000ULL;
#endif
    unsigned long long freeTotal = 0;
    unsigned long long freeLargest = 0;
    unsigned long long previousEnd = 0;
    char line[512];
    while (fgets(line, sizeof(line), maps) != 0)
    {
        unsigned long long start = 0;
        unsigned long long end = 0;
        if (sscanf(line, "%llx-%llx", &start, &end) != 2 || start >= userLimit)
            continue;
        if (start > previousEnd)
        {
            freeTotal += start - previousEnd;
            if (start - previousEnd > freeLargest)
                freeLargest = start - previousEnd;
        }
        previousEnd = end;
    }
    if (previousEnd < userLimit)
    {
        freeTotal += userLimit - previousEnd;
        if (userLimit - previousEnd > freeLargest)
            freeLargest = userLimit - previousEnd;
    }
    fclose(maps);
    snprintf(line, sizeof(line), "address space: %u MB free between mappings, largest free block %u MB",
             static_cast<unsigned int>(freeTotal / kMegabyte), static_cast<unsigned int>(freeLargest / kMegabyte));
    StartupLog(line, false);
}

#if __SIZEOF_POINTER__ == 8
char* MapLowBlock(size_t size)
{
    const int protection = PROT_READ | PROT_WRITE;
    const int flags = MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE;
#ifdef MAP_32BIT
    void* block = mmap(0, size, protection, flags | MAP_32BIT, -1, 0);
    if (block != MAP_FAILED)
    {
        if (reinterpret_cast<size_t>(block) + size <= kLowHeapLimit)
            return static_cast<char*>(block);
        munmap(block, size);
    }
#endif
#ifdef MAP_FIXED_NOREPLACE
    for (size_t address = 64 * kMegabyte; address + size <= kLowHeapLimit; address += size)
    {
        void* block = mmap(reinterpret_cast<void*>(address), size, protection, flags | MAP_FIXED_NOREPLACE, -1, 0);
        if (block == reinterpret_cast<void*>(address))
            return static_cast<char*>(block);
        if (block != MAP_FAILED)
            munmap(block, size);
    }
#endif
    return 0;
}

void InitialiseLowHeap()
{
    gLowHeapStart = MapLowBlock(kLowHeapSize);
    if (gLowHeapStart == 0)
    {
        StartupLog("heap: could not reserve the 256 MB sound heap below 4 GB; stopping", false);
        abort();
    }
    gLowHeapEnd = gLowHeapStart + kLowHeapSize;
    PcCreateLowHeap(gLowHeapStart, kLowHeapSize);
    char line[128];
    snprintf(line, sizeof(line), "heap: %u MB sound heap at %p (below 4 GB, for 32-bit Miles handles)",
             static_cast<unsigned int>(kLowHeapSize / kMegabyte), static_cast<void*>(gLowHeapStart));
    StartupLog(line, false);
}
#endif

char* ReserveArena(size_t size, bool commit)
{
    const int protection = commit ? PROT_READ | PROT_WRITE : PROT_NONE;
    void* block = mmap(0, size, protection, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    return block == MAP_FAILED ? 0 : static_cast<char*>(block);
}

void InitialiseArena()
{
    if (gHeapStart != 0)
        return;
    size_t requested = 0;
    bool commitAll = false;
    const bool configured = ReadHeapSettings(&requested, &commitAll);
    if (configured && requested >= 128)
    {
        gHeapStart = ReserveArena(requested * kMegabyte, commitAll);
        if (gHeapStart != 0)
            gHeapEnd = gHeapStart + requested * kMegabyte;
    }
    for (size_t index = 0; index < sizeof(kArenaSizes) / sizeof(kArenaSizes[0]) && gHeapStart == 0; ++index)
    {
        gHeapStart = ReserveArena(kArenaSizes[index], commitAll);
        if (gHeapStart != 0)
            gHeapEnd = gHeapStart + kArenaSizes[index];
    }
    char line[160];
    if (gHeapStart == 0)
    {
        StartupLog("heap: could not reserve even 384 MB of address space; stopping", true);
        abort();
    }
    gHeapTop = gHeapStart;
    s_committedEnd = commitAll ? gHeapEnd : gHeapStart;
    snprintf(line, sizeof(line), "heap: %u MB %s at %p%s",
             static_cast<unsigned int>((gHeapEnd - gHeapStart) / kMegabyte),
             commitAll ? "reserved and committed" : "reserved", static_cast<void*>(gHeapStart),
             configured ? " (heap.txt or MCLCE_HEAP)" : "");
    StartupLog(line, true);
    if (!commitAll)
    {
        size_t initial = kInitialCommit;
        if (initial > static_cast<size_t>(gHeapEnd - gHeapStart))
            initial = static_cast<size_t>(gHeapEnd - gHeapStart);
        if (mprotect(gHeapStart, initial, PROT_READ | PROT_WRITE) == 0)
        {
            s_committedEnd = gHeapStart + initial;
            snprintf(line, sizeof(line), "heap: first %u MB committed", static_cast<unsigned int>(initial / kMegabyte));
        }
        else
            snprintf(line, sizeof(line), "heap: committing the first %u MB failed (errno %d)",
                     static_cast<unsigned int>(initial / kMegabyte), errno);
        StartupLog(line, false);
    }
#if __SIZEOF_POINTER__ == 8
    InitialiseLowHeap();
#endif
    LogAddressSpace();
}
}

__attribute__((constructor(101))) static void InitialiseArenaAtStartup()
{
    InitialiseArena();
}

__attribute__((constructor(102))) static void LogConstructorsStarting()
{
    StartupLog("startup: global constructors starting", false);
}

extern "C" void PcStartupLog(const char* text)
{
    StartupLog(text, false);
}

extern "C" int PcHeapCommit(char* top)
{
    if (top <= s_committedEnd)
        return 1;
    size_t wanted = static_cast<size_t>(top - s_committedEnd);
    wanted = (wanted + kCommitStep - 1) / kCommitStep * kCommitStep;
    if (wanted > static_cast<size_t>(gHeapEnd - s_committedEnd))
        wanted = static_cast<size_t>(gHeapEnd - s_committedEnd);
    if (mprotect(s_committedEnd, wanted, PROT_READ | PROT_WRITE) != 0)
    {
        char line[160];
        snprintf(line, sizeof(line), "heap: committing %u MB at %u MB into the heap failed (errno %d)",
                 static_cast<unsigned int>(wanted / kMegabyte),
                 static_cast<unsigned int>((s_committedEnd - gHeapStart) / kMegabyte), errno);
        StartupLog(line, false);
        return 0;
    }
    s_committedEnd += wanted;
    return top <= s_committedEnd;
}

#endif
