// any_hash
//
// A single-file library that provides a simple implementation of the
// xxHash xxh32 and xxh64 hashing algorithms.
//
// To use this library you should choose a suitable file to put the
// implementation and define ANY_HASH_IMPLEMENT. For example
//
//    #define ANY_HASH_IMPLEMENT
//    #include "any_hash.h"
//
// Additionally, you can customize the library behavior by defining certain
// macros in the file where you put the implementation. You can see which are
// supported by reading the code guarded by ANY_HASH_IMPLEMENT.
//
// This library is licensed under the terms of the MIT license.
// A copy of the license is included at the end of this file.
//

// Rationale
//
// Why use this library when there is the official xxHash library?
//
// The answer is simplicity: the xxHash library is a single header with
// more than 7000 lines of code. This becomes a burden both for shipping
// and compiling the header file in a (especially small) project.
//
// Meanwhile this any_hash is a few hundreds lines at most.
// This comes at a cost thought: less architecture specific optimizations,
// less hashing algorithms, slightly less performance overall.
//
// Note however that the library is still very fast and the perfomance
// difference will not be noticeable in small projects.
//
// How does this library perform?
//
// As said before, it does far less optimizations compared to xxHash.
// It really makes a difference if you are using an architecture with
// modern instruction sets like AVX2.
//
// However any_hash is still quite fast and performant compared to most
// handcoded hashing solutions and comes in a convenient small package.
//

// Perfomance comparison
//
//   hash function  | large inputs |   small inputs
//                  |              |
//    xxHash XXH3   |  17.07 GB/s  |  129813995 hash/s
//    xxHash XXH32  |   6.02 GB/s  |  87749462 hash/s
//    xxHash XXH64  |  11.72 GB/s  |  81670020 hash/s
//    xxHash XXH128 |  16.71 GB/s  | 112659406 hash/s
//     any_xxh32    |   6.10 GB/s  |  83403749 hash/s
//     any_xxh64    |  11.89 GB/s  |  72925776 hash/s
//
// The speed was measured with the 'benchHash' program provided by
// the xxHash library and without AVX2 enabled.
//

#ifndef ANY_HASH_INCLUDE
#define ANY_HASH_INCLUDE

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifndef ANY_HASH_NO_XXH32

typedef uint32_t any_hash32_t;

any_hash32_t any_hash_xxh32(const uint8_t *data, size_t length, any_hash32_t seed);

#endif

#ifndef ANY_HASH_NO_XXH64

typedef uint64_t any_hash64_t;

any_hash64_t any_hash_xxh64(const uint8_t *data, size_t length, any_hash64_t seed);

#endif

#endif

#ifdef ANY_HASH_IMPLEMENT

#include <string.h>

#define ANY_HASH_ALIGNED 0

#define ANY_HASH_ROTL1 1
#define ANY_HASH_ROTL2 7
#define ANY_HASH_ROTL3 12
#define ANY_HASH_ROTL4 18

#ifndef ANY_HASH_LIKELY
#ifdef __has_builtin
#if __has_builtin(__builtin_expect)
#define ANY_HASH_LIKELY(...) __builtin_expect(!!(__VA_ARGS__), 1)
#endif
#endif

#ifndef ANY_HASH_LIKELY
#define ANY_HASH_LIKELY(...)
#endif
#endif

#ifndef ANY_HASH_RUNTIME_ENDIAN
#ifndef ANY_HASH_LITTLE_ENDIAN
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__ || \
    defined(__BIG_ENDIAN__) || defined(__BIG_ENDIAN) || defined(_BIG_ENDIAN)
#define ANY_HASH_LITTLE_ENDIAN 0
#else
#define ANY_HASH_LITTLE_ENDIAN 1
#endif
#endif
#else
#define ANY_HASH_LITTLE_ENDIAN any_hash_little_endian()

static inline int any_hash_little_endian(void)
{
    const union {
        uint32_t uint;
        char bytes[4];
    } t = { 1 };
    return t.bytes[0];
}
#endif

#ifndef ANY_HASH_NO_XXH32

#define ANY_HASH_BYTES32 sizeof(any_hash32_t)
#define ANY_HASH_DATALEN32 16

#define ANY_HASH_ALIGN32 3
#define ANY_HASH_ROUND32 13

#define ANY_HASH_AV32_1 15
#define ANY_HASH_AV32_2 13
#define ANY_HASH_AV32_3 16

#define ANY_HASH_PROC32_1 17
#define ANY_HASH_PROC32_2 11

#define ANY_HASH_PRIME32_1 2654435761u
#define ANY_HASH_PRIME32_2 2246822519u
#define ANY_HASH_PRIME32_3 3266489917u
#define ANY_HASH_PRIME32_4 668265263u
#define ANY_HASH_PRIME32_5 374761393u

#ifdef __has_builtin
#if __has_builtin(__builtin_bswap32)
#define ANY_HASH_SWAP32 __builtin_bswap32
#endif

#if __has_builtin(__builtin_rotateleft32)
#define ANY_HASH_ROTL32 __builtin_rotateleft32
#endif
#endif

#ifndef ANY_HASH_SWAP32
#define ANY_HASH_SWAP32 any_hash_swap32

static inline any_hash32_t any_hash_swap32(any_hash32_t hash)
{
    return ((hash << 24) & 0xff000000) |
           ((hash <<  8) & 0x00ff0000) |
           ((hash <<  8) & 0x0000ff00) |
           ((hash << 24) & 0x000000ff);
}
#endif

#ifndef ANY_HASH_ROTL32
#define ANY_HASH_ROTL32 any_hash_rotl32

static inline any_hash32_t any_hash_rotl32(any_hash32_t hash, uint32_t bits)
{
    return (hash << bits) | (hash >> (32 - bits));
}
#endif


static inline any_hash32_t any_hash_fetch32(const uint8_t *data, int_fast32_t align)
{
    if (ANY_HASH_LIKELY(align == ANY_HASH_ALIGNED))
        return ANY_HASH_LITTLE_ENDIAN
            ? *(const any_hash32_t *)data
            : ANY_HASH_SWAP32(*(const any_hash32_t *)data);

    any_hash32_t hash;
    memcpy(&hash, data, 4);

    return ANY_HASH_LITTLE_ENDIAN
        ? hash
        : ANY_HASH_SWAP32(hash);
}

static inline any_hash32_t any_hash_avalanche32(any_hash32_t hash)
{
    hash ^= hash >> ANY_HASH_AV32_1;
    hash *= ANY_HASH_PRIME32_2;
    hash ^= hash >> ANY_HASH_AV32_2;
    hash *= ANY_HASH_PRIME32_3;
    hash ^= hash >> ANY_HASH_AV32_3;
    return hash;
}

static any_hash32_t any_hash_round32(any_hash32_t hash, any_hash32_t next)
{
    hash += next * ANY_HASH_PRIME32_2;
    hash = ANY_HASH_ROTL32(hash, ANY_HASH_ROUND32);
    hash *= ANY_HASH_PRIME32_1;
    return hash;
}

any_hash32_t any_hash_xxh32(const uint8_t *data, size_t length, any_hash32_t seed)
{
    any_hash32_t hash;
    const int_fast32_t align = (uintptr_t)data & ANY_HASH_ALIGN32;

    if (length >= ANY_HASH_DATALEN32) {
        const uint8_t *const end = data + length - ANY_HASH_DATALEN32;

        any_hash32_t st1 = seed + ANY_HASH_PRIME32_1 + ANY_HASH_PRIME32_2;
        any_hash32_t st2 = seed + ANY_HASH_PRIME32_2;
        any_hash32_t st3 = seed;
        any_hash32_t st4 = seed - ANY_HASH_PRIME32_1;

        do {
            st1 = any_hash_round32(st1, any_hash_fetch32(data, align));
            data += ANY_HASH_BYTES32;
            st2 = any_hash_round32(st2, any_hash_fetch32(data, align));
            data += ANY_HASH_BYTES32;
            st3 = any_hash_round32(st3, any_hash_fetch32(data, align));
            data += ANY_HASH_BYTES32;
            st4 = any_hash_round32(st4, any_hash_fetch32(data, align));
            data += ANY_HASH_BYTES32;
        } while (data <= end);

        hash = ANY_HASH_ROTL32(st1, ANY_HASH_ROTL1) + ANY_HASH_ROTL32(st2, ANY_HASH_ROTL2)
             + ANY_HASH_ROTL32(st3, ANY_HASH_ROTL3) + ANY_HASH_ROTL32(st4, ANY_HASH_ROTL4);
    } else
        hash = seed + ANY_HASH_PRIME32_5;

    hash += length;
    length &= (ANY_HASH_DATALEN32 - 1);

    while (length >= 4) {
        hash += any_hash_fetch32(data, align) * ANY_HASH_PRIME32_3;
        hash = ANY_HASH_ROTL32(hash, ANY_HASH_PROC32_1) * ANY_HASH_PRIME32_4;
        data += ANY_HASH_BYTES32;
        length -= ANY_HASH_BYTES32;
    }

    while (length > 0) {
        hash += (*data++) * ANY_HASH_PRIME32_5;
        hash = ANY_HASH_ROTL32(hash, ANY_HASH_PROC32_2) * ANY_HASH_PRIME32_1;
        --length;
    }

    return any_hash_avalanche32(hash);
}

#endif

#ifndef ANY_HASH_NO_XXH64

#define ANY_HASH_BYTES64 sizeof(any_hash64_t)
#define ANY_HASH_DATALEN64 32

#define ANY_HASH_ALIGN64 7
#define ANY_HASH_ROUND64 31

#define ANY_HASH_AV64_1 33
#define ANY_HASH_AV64_2 29
#define ANY_HASH_AV64_3 32

#define ANY_HASH_PROC64_1 27
#define ANY_HASH_PROC64_2 23
#define ANY_HASH_PROC64_3 11

#define ANY_HASH_PRIME64_1 11400714785074694791ull
#define ANY_HASH_PRIME64_2 14029467366897019727ull
#define ANY_HASH_PRIME64_3 1609587929392839161ull
#define ANY_HASH_PRIME64_4 9650029242287828579ull
#define ANY_HASH_PRIME64_5 2870177450012600261ull

#ifdef __has_builtin
#if __has_builtin(__builtin_bswap64)
#define ANY_HASH_SWAP64 __builtin_bswap64
#endif

#if __has_builtin(__builtin_rotateleft64)
#define ANY_HASH_ROTL64 __builtin_rotateleft64
#endif
#endif

#ifndef ANY_HASH_SWAP64
#define ANY_HASH_SWAP64 any_hash_swap64

static inline any_hash64_t any_hash_swap64(any_hash64_t hash)
{
    return ((hash << 56) & 0xff00000000000000ull) |
           ((hash << 40) & 0x00ff000000000000ull) |
           ((hash << 24) & 0x0000ff0000000000ull) |
           ((hash <<  8) & 0x000000ff00000000ull) |
           ((hash >>  8) & 0x00000000ff000000ull) |
           ((hash >> 24) & 0x0000000000ff0000ull) |
           ((hash >> 40) & 0x000000000000ff00ull) |
           ((hash >> 56) & 0x00000000000000ffull);
}
#endif

#ifndef ANY_HASH_ROTL64
#define ANY_HASH_ROTL64 any_hash_rotl64

static inline any_hash64_t any_hash_rotl64(any_hash64_t hash, uint32_t bits)
{
    return (hash << bits) | (hash >> (64 - bits));
}
#endif

static inline any_hash64_t any_hash_fetch64(const uint8_t *data, int_fast32_t align)
{
    if (ANY_HASH_LIKELY(align == ANY_HASH_ALIGNED))
        return ANY_HASH_LITTLE_ENDIAN
            ? *(const any_hash64_t *)data
            : ANY_HASH_SWAP64(*(const any_hash64_t *)data);

    any_hash64_t hash;
    memcpy(&hash, data, 8);

    return ANY_HASH_LITTLE_ENDIAN
        ? hash
        : ANY_HASH_SWAP64(hash);
}

static inline any_hash64_t any_hash_avalanche64(any_hash64_t hash)
{
    hash ^= hash >> ANY_HASH_AV64_1;
    hash *= ANY_HASH_PRIME64_2;
    hash ^= hash >> ANY_HASH_AV64_2;
    hash *= ANY_HASH_PRIME64_3;
    hash ^= hash >> ANY_HASH_AV64_3;
    return hash;
}

static any_hash64_t any_hash_round64(any_hash64_t hash, any_hash64_t next)
{
    hash += next * ANY_HASH_PRIME64_2;
    hash = ANY_HASH_ROTL64(hash, ANY_HASH_ROUND64);
    hash *= ANY_HASH_PRIME64_1;
    return hash;
}

static any_hash64_t any_hash_round64_merge(any_hash64_t hash, any_hash64_t next)
{
    hash ^= any_hash_round64(0, next);
    hash = hash * ANY_HASH_PRIME64_1 + ANY_HASH_PRIME64_4;
    return hash;
}

any_hash64_t any_hash_xxh64(const uint8_t *data, size_t length, any_hash64_t seed)
{
    any_hash64_t hash;
    const int_fast32_t align = (uintptr_t)data & ANY_HASH_ALIGN64;

    if (length >= ANY_HASH_DATALEN64) {
        const uint8_t *const end = data + length - ANY_HASH_DATALEN64;

        any_hash64_t st1 = seed + ANY_HASH_PRIME64_1 + ANY_HASH_PRIME64_2;
        any_hash64_t st2 = seed + ANY_HASH_PRIME64_2;
        any_hash64_t st3 = seed;
        any_hash64_t st4 = seed - ANY_HASH_PRIME64_1;

        do {
            st1 = any_hash_round64(st1, any_hash_fetch64(data, align));
            data += ANY_HASH_BYTES64;
            st2 = any_hash_round64(st2, any_hash_fetch64(data, align));
            data += ANY_HASH_BYTES64;
            st3 = any_hash_round64(st3, any_hash_fetch64(data, align));
            data += ANY_HASH_BYTES64;
            st4 = any_hash_round64(st4, any_hash_fetch64(data, align));
            data += ANY_HASH_BYTES64;
        } while (data <= end);

        hash = ANY_HASH_ROTL64(st1, ANY_HASH_ROTL1) + ANY_HASH_ROTL64(st2, ANY_HASH_ROTL2)
             + ANY_HASH_ROTL64(st3, ANY_HASH_ROTL3) + ANY_HASH_ROTL64(st4, ANY_HASH_ROTL4);

        hash = any_hash_round64_merge(hash, st1);
        hash = any_hash_round64_merge(hash, st2);
        hash = any_hash_round64_merge(hash, st3);
        hash = any_hash_round64_merge(hash, st4);
    } else
        hash = seed + ANY_HASH_PRIME64_5;

    hash += length;
    length &= (ANY_HASH_DATALEN64 - 1);

    while (length >= 8) {
        hash ^= any_hash_round64(0, any_hash_fetch64(data, align));
        hash = ANY_HASH_ROTL64(hash, ANY_HASH_PROC64_1) * ANY_HASH_PRIME64_1 + ANY_HASH_PRIME64_4;
        data += ANY_HASH_BYTES64;
        length -= ANY_HASH_BYTES64;
    }

    while (length >= 4) {
        hash ^= (any_hash64_t)(any_hash_fetch32(data, align)) * ANY_HASH_PRIME64_1;
        hash = ANY_HASH_ROTL64(hash, ANY_HASH_PROC64_2) * ANY_HASH_PRIME64_2 + ANY_HASH_PRIME64_3;
        data += ANY_HASH_BYTES32;
        length -= ANY_HASH_BYTES32;
    }

    while (length > 0) {
        hash ^= (*data++) * ANY_HASH_PRIME64_5;
        hash = ANY_HASH_ROTL64(hash, ANY_HASH_PROC64_3) * ANY_HASH_PRIME64_1;
        --length;
    }

    return any_hash_avalanche64(hash);
}

#endif

#endif

// MIT License
//
// Copyright (c) 2024 Federico Angelilli
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
