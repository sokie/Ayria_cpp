/*
    Initial author: Convery (tcn@ayria.se)
    Started: 14-03-2019
    License: MIT
*/

#pragma once
#include <memory>
#include <cassert>
#include <cstdarg>
#include <string_view>

namespace Internal
{
    // Truncate the string if needed.
    #if defined(VA_SIZE)
    constexpr int32_t Defaultsize = VA_SIZE;
    #else
    constexpr int32_t Defaultsize = 512;
    #endif

    inline int32_t va(char *Buffer, int32_t Size, const std::string_view Format, std::va_list Varlist)
    {
        return std::vsnprintf(Buffer, Size, Format.data(), Varlist);
    }
}

inline std::string va(std::string_view Format, ...)
{
    auto Buffer{ std::make_unique<char[]>(Internal::Defaultsize) };
    std::va_list Varlist;
    int32_t Size{};

    // Parse the argument-list.
    va_start(Varlist, Format);
    {
        // Try using the default size as it should work 99% of the time.
        Size = Internal::va(Buffer.get(), Internal::Defaultsize, Format, Varlist);

        // If the size is larger, we need to allocate again =(
        if (Size > Internal::Defaultsize)
        {
            Buffer = std::make_unique<char[]>(Size);
            Size = Internal::va(Buffer.get(), Size, Format, Varlist);
        }
    }
    va_end(Varlist);

    // Negative result on error.
    if (Size < 0) { assert(false); return {}; }
    assert(Size);

    // Take the memory with us.
    return { std::move(Buffer.get()), static_cast<size_t>(Size) };
}
inline std::string va(const char *Format, ...)
{
    auto Buffer{ std::make_unique<char[]>(Internal::Defaultsize) };
    std::va_list Varlist;
    int32_t Size{};

    // Parse the argument-list.
    va_start(Varlist, Format);
    {
        // Try using the default size as it should work 99% of the time.
        Size = Internal::va(Buffer.get(), Internal::Defaultsize, Format, Varlist);

        // If the size is larger, we need to allocate again =(
        if (Size > Internal::Defaultsize)
        {
            Buffer = std::make_unique<char[]>(Size);
            Size = Internal::va(Buffer.get(), Size, Format, Varlist);
        }
    }
    va_end(Varlist);

    // Negative result on error.
    if (Size < 0) { assert(false); return {}; }
    assert(Size);

    // Take the memory with us.
    return { std::move(Buffer.get()), static_cast<size_t>(Size) };
}
inline std::string va(const std::string Format, ...)
{
    auto Buffer{ std::make_unique<char[]>(Internal::Defaultsize) };
    std::va_list Varlist;
    int32_t Size{};

    // Parse the argument-list.
    va_start(Varlist, Format);
    {
        // Try using the default size as it should work 99% of the time.
        Size = Internal::va(Buffer.get(), Internal::Defaultsize, Format, Varlist);

        // If the size is larger, we need to allocate again =(
        if (Size > Internal::Defaultsize)
        {
            Buffer = std::make_unique<char[]>(Size);
            Size = Internal::va(Buffer.get(), Size, Format, Varlist);
        }
    }
    va_end(Varlist);

    // Negative result on error.
    if (Size < 0) { assert(false); return {}; }
    assert(Size);

    // Take the memory with us.
    return { std::move(Buffer.get()), static_cast<size_t>(Size) };
}
