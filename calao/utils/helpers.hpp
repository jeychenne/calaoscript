/**********************************************************************************************************************
 *                                                                                                                    *
 * Copyright (C) 2019-2020 Julien Eychenne <jeychenne@gmail.com>                                                      *
 *                                                                                                                    *
 * The contents of this file are subject to the Mozilla Public License Version 2.0 (the "License"); you may not use   *
 * this file except in compliance with the License. You may obtain a copy of the License at                           *
 * http://www.mozilla.org/MPL/.                                                                                       *
 *                                                                                                                    *
 * Created: 20/02/2019                                                                                                *
 *                                                                                                                    *
 * Purpose: helper routines.                                                                                          *
 *                                                                                                                    *
 **********************************************************************************************************************/

#ifndef CALAO_HELPERS_HPP
#define CALAO_HELPERS_HPP

#include <cstdio>
#include <string>
#include <calao/definitions.hpp>

/* Use byte swapping primitives if available, otherwise default to generic versions */
#if defined(_WIN32) && !defined(CALAO_MINGW)
#include <intrin.h>
#   define CALAO_BYTESWAP16(x) _byteswap_ushort(x)
#   define CALAO_BYTESWAP32(x) _byteswap_ulong(x)
#   define CALAO_HAS_BYTESWAP
#elif defined(__APPLE__)
#    include <libkern/OSByteOrder.h>
#   define CALAO_BYTESWAP16(x) OSSwapInt16(x)
#   define CALAO_BYTESWAP32(x) OSSwapInt32(x)
#   define CALAO_HAS_BYTESWAP
#   define CALAO_ENDIANNES_KNOWN
#elif (defined(__GNUC__) || defined(__GNUG__)) && !defined(CALAO_MINGW)
#    include <byteswap.h>

#   define CALAO_BYTESWAP16(x) __bswap_16(x)
#   define CALAO_BYTESWAP32(x) __bswap_32(x)
#   define CALAO_HAS_BYTESWAP
#   define CALAO_ENDIANNES_KNOWN
#else
#   define CALAO_BYTESWAP16(x) ((x >> 8) | (x << 8))
#   define CALAO_BYTESWAP32(x) \
    ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) | \
    (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))
#endif

namespace calao
{
    class String;
}


namespace calao { namespace utils {



// Check whether the platform is big endian (most likely false)
// Use compile-time check if possible.
#if CALAO_WINDOWS
static inline bool is_big_endian()
{
	return false;
}

#elif defined(CALAO_ENDIANNES_KNOWN)

static inline bool is_big_endian()
{
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	return true;
#else
	return false;
#endif
}

#else
bool is_big_endian();
#endif // end check endianness




void init_random_seed();

size_t random_seed();

static inline
intptr_t next_capacity(intptr_t n)
{
	assert(n != 0);
	return (n < 32) ? (n << 1) : (n + (n >> 1));
}

static inline
intptr_t find_capacity(intptr_t requested, intptr_t capacity = 8)
{
	// Find a capacity which is at least what is requested.
	while (capacity < requested) capacity = utils::next_capacity(capacity);

	return capacity;
}

String new_uuid(size_t len = 20);


FILE *open_file(const String &path, const char *mode);

FILE *reopen_file(const String &path, const char *mode, FILE *stream);


template<class T>
T minimum(T x, T y, T z)
{
	return (std::min)((std::min)(x, y), z);
}


template<class T>
T maximum(T x, T y, T z)
{
	return (std::max)((std::max)(x, y), z);
}

}} // namespace::utils

#endif // CALAO_HELPERS_HPP
