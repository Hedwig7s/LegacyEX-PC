#ifndef SOURCE_NN_PIA_COMMON_MEMORY_HPP
#define SOURCE_NN_PIA_COMMON_MEMORY_HPP

#if !defined(_WIN32) && !defined(__unix__) // FIXME: This is a hacky way to get debug building
#include <new>

#include "nn/pia/HeapManager.hpp"
#include "platform/wiiu/pead.hpp"
#endif

namespace nn
{
namespace pia
{
namespace common
{

template <typename T> static T* NewObj()
{
#if !defined(_WIN32) && !defined(__unix__)
    void* buffer = pead::allocateFromSeadHeap(sizeof(T), HeapManager::GetHeap());
    if (buffer == 0)
        return 0;
    return ::new (buffer) T();
#else
    return 0;
#endif
}

template <typename T, typename A1, typename A2> static T* NewObj(A1 a1, A2 a2)
{
#if !defined(_WIN32) && !defined(__unix__)
    void* buffer = pead::allocateFromSeadHeap(sizeof(T), HeapManager::GetHeap());
    if (buffer == 0)
        return 0;
    return ::new (buffer) T(a1, a2);
#else
    return 0;
#endif
}

template <typename T> static void DeleteObj(T* object)
{
#if !defined(_WIN32) && !defined(__unix__)
    if (object != 0)
    {
        object->~T();
        pead::freeToSeadHeap(object);
    }
#endif
}

template <typename T> static T* NewArray(unsigned int count)
{
#if !defined(_WIN32) && !defined(__unix__)
    void* buffer = pead::allocateFromSeadHeap(sizeof(T) * count, HeapManager::GetHeap());
    if (buffer == 0)
        return 0;
    T* array = static_cast<T*>(buffer);
    for (unsigned int index = 0; index < count; ++index)
        ::new (&array[index]) T();
    return array;
#else
    return 0;
#endif
}

template <typename T> static T* NewArray(unsigned int count, int alignment)
{
#if !defined(_WIN32) && !defined(__unix__)
    void* buffer = pead::allocateFromSeadHeap(sizeof(T) * count, HeapManager::GetHeap(), alignment);
    if (buffer == 0)
        return 0;
    T* array = static_cast<T*>(buffer);
    for (unsigned int index = 0; index < count; ++index)
        ::new (&array[index]) T();
    return array;
#else
    return 0;
#endif
}

template <typename T> static void DeleteArray(T* array)
{
#if !defined(_WIN32) && !defined(__unix__)
    if (array != 0)
    {
        unsigned int size = pead::getArraySize(array);
        unsigned int count = size / sizeof(T);
        for (unsigned int index = 0; index < count; ++index)
            array[index].~T();
        pead::freeToSeadHeap(array);
    }
#endif
}

}
}
}

#endif
