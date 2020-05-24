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
 * Purpose: standard exceptions.                                                                                      *
 *                                                                                                                    *
 **********************************************************************************************************************/

#ifndef CALAO_ERROR_HPP
#define CALAO_ERROR_HPP

#include <stdexcept>
#include <calao/utils/print.hpp>

namespace calao {

// Forward declaration.
class String;

template<typename T, typename... Args>
std::runtime_error error(const char *fmt, const T &value, Args... args)
{
	auto msg = utils::format(fmt, value, args...);
	return std::runtime_error(msg);
}

static inline
std::runtime_error error(const std::string &msg)
{
	return std::runtime_error(msg);
}

static inline
std::runtime_error error(const char *msg)
{
	return std::runtime_error(msg);
}

std::runtime_error error(const String &msg);

} // namespace calao

#endif // CALAO_ERROR_HPP
