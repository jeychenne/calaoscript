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

#ifndef CALAO_DEFINITIONS_HPP
#define CALAO_DEFINITIONS_HPP

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <functional>

#define CALAO_UNUSED(x) (void)(x)

#ifdef __GNUC__
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)
#else
#define likely(x)       x
#define unlikely(x)     x
#endif

// File extension
#ifndef CALAO_FILE_EXTENSION
#	define CALAO_FILE_EXTENSION ".calao"
#endif

namespace calao {

namespace meta {

static constexpr size_t pointer_size = sizeof(void*);
static constexpr bool is_arch32 = (pointer_size == 4);
static constexpr bool is_arch64 = (pointer_size == 8);

} // namespace calao::meta


// Largest and smallest integers that can be safely stored in a double.
static constexpr double largest_integer = 9007199254740992;
static constexpr double smallest_integer = -9007199254740992;

// Forward declarations.
class Object;
class Collectable;

// Callback for the garbage collector.
using GCCallback = std::function<void(Collectable*)>;

} // namespace calao

#endif // CALAO_DEFINITIONS_HPP
