/**********************************************************************************************************************
 *                                                                                                                    *
 * Copyright (C) 2019-2020 Julien Eychenne <jeychenne@gmail.com>                                                      *
 *                                                                                                                    *
 * The contents of this file are subject to the Mozilla Public License Version 2.0 (the "License"); you may not use   *
 * this file except in compliance with the License. You may obtain a copy of the License at                           *
 * http://www.mozilla.org/MPL/.                                                                                       *
 *                                                                                                                    *
 * Created: 22/05/2020                                                                                                *
 *                                                                                                                    *
 * Purpose: common definitions.                                                                                       *
 *                                                                                                                    *
 **********************************************************************************************************************/

#ifndef PHONOMETRICA_DEFINITIONS_HPP
#define PHONOMETRICA_DEFINITIONS_HPP

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <functional>

#define PHON_UNUSED(x) (void)(x)

#ifdef __GNUC__
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)
#else
#define likely(x)       x
#define unlikely(x)     x
#endif

// File extension
#ifndef PHON_FILE_EXTENSION
#	define PHON_FILE_EXTENSION ".phon"
#endif

namespace phonometrica {

namespace meta {

static constexpr size_t pointer_size = sizeof(void*);
static constexpr bool is_arch32 = (pointer_size == 4);
static constexpr bool is_arch64 = (pointer_size == 8);

} // namespace phonometrica::meta


// Largest and smallest integers that can be safely stored in a double.
static constexpr double largest_integer = 9007199254740992;
static constexpr double smallest_integer = -9007199254740992;

// Forward declarations.
class Object;
class Collectable;

// Callback for the garbage collector.
using GCCallback = std::function<void(Collectable*)>;

} // namespace phonometrica

#endif // PHONOMETRICA_DEFINITIONS_HPP
