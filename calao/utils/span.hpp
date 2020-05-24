/**********************************************************************************************************************
 *                                                                                                                    *
 * Copyright (C) 2019-2020 Julien Eychenne <jeychenne@gmail.com>                                                      *
 *                                                                                                                    *
 * The contents of this file are subject to the Mozilla Public License Version 2.0 (the "License"); you may not use   *
 * this file except in compliance with the License. You may obtain a copy of the License at                           *
 * http://www.mozilla.org/MPL/.                                                                                       *
 *                                                                                                                    *
 * Created: 11/10/2019                                                                                                *
 *                                                                                                                    *
 * Purpose: an array view similar to std::span in C++20.                                                              *
 *                                                                                                                    *
 **********************************************************************************************************************/

#ifndef CALAO_SPAN_HPP
#define CALAO_SPAN_HPP

#if __has_include(<span>)
#include <span>

#else

#include <calao/third_party/span.hpp>

namespace std {

template<typename T>
using span = nonstd::span<T>;

} // std

#endif // __has_include(<span>)

#endif // CALAO_SPAN_HPP
