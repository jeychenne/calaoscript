/**********************************************************************************************************************
 *                                                                                                                    *
 * Copyright (C) 2019-2020 Julien Eychenne <jeychenne@gmail.com>                                                      *
 *                                                                                                                    *
 * The contents of this file are subject to the Mozilla Public License Version 2.0 (the "License"); you may not use   *
 * this file except in compliance with the License. You may obtain a copy of the License at                           *
 * http://www.mozilla.org/MPL/.                                                                                       *
 *                                                                                                                    *
 * Created: 31/05/2020                                                                                                *
 *                                                                                                                    *
 * Purpose: generic builtin functions.                                                                                *
 *                                                                                                                    *
 **********************************************************************************************************************/

#ifndef CALAO_FUNC_GENERIC_HPP
#define CALAO_FUNC_GENERIC_HPP

#include <calao/runtime.hpp>
#include <calao/file.hpp>
#include <calao/regex.hpp>
#include <calao/internal/func_set.hpp>

namespace calao {

static Variant get_type(Runtime &, std::span<Variant> args)
{
	return args[0].get_class()->object();
}

static Variant get_length(Runtime &, std::span<Variant> args)
{
	auto &v = args[0];

	if (check_type<String>(v)) {
		return raw_cast<String>(v).grapheme_count();
	}
	else if (check_type<List>(v)) {
		return raw_cast<List>(v).size();
	}
	else if (check_type<Table>(v)) {
		return raw_cast<Table>(v).size();
	}
	else if (check_type<File>(v)) {
		return raw_cast<File>(v).size();
	}
	else if (check_type<Regex>(v)) {
		return raw_cast<Regex>(v).count();
	}
	else if (check_type<Set>(v)) {
		return raw_cast<Set>(v).size();
	}

	throw error("[Type error] Cannot get length of % value", v.class_name());
}

static Variant to_string(Runtime &, std::span<Variant> args)
{
	return args[0].to_string();
}

static Variant to_boolean(Runtime &, std::span<Variant> args)
{
	return args[0].to_boolean();
}

static Variant to_integer(Runtime &, std::span<Variant> args)
{
	return args[0].to_integer();
}

static Variant to_float(Runtime &, std::span<Variant> args)
{
	return args[0].to_float();
}

} // namespace calao

#endif // CALAO_FUNC_GENERIC_HPP
