/**********************************************************************************************************************
 *                                                                                                                    *
 * Copyright (C) 2019-2020 Julien Eychenne <jeychenne@gmail.com>                                                      *
 *                                                                                                                    *
 * The contents of this file are subject to the Mozilla Public License Version 2.0 (the "License"); you may not use   *
 * this file except in compliance with the License. You may obtain a copy of the License at                           *
 * http://www.mozilla.org/MPL/.                                                                                       *
 *                                                                                                                    *
 * Created: 04/06/2020                                                                                                *
 *                                                                                                                    *
 * Purpose: Regex builtin functions.                                                                                  *
 *                                                                                                                    *
 **********************************************************************************************************************/

#ifndef CALAO_FUNC_REGEX_HPP
#define CALAO_FUNC_REGEX_HPP

#include <calao/regex.hpp>
#include <calao/runtime.hpp>

namespace calao {

static Variant regex_new1(Runtime &, std::span<Variant> args)
{
	auto &pattern = raw_cast<String>(args[0]);
	return make_handle<Regex>(pattern);
}

static Variant regex_new2(Runtime &, std::span<Variant> args)
{
	auto &pattern = raw_cast<String>(args[0]);
	auto &flags = raw_cast<String>(args[1]);


	return make_handle<Regex>(pattern, flags);
}

static Variant regex_match1(Runtime &, std::span<Variant> args)
{
	auto &regex = raw_cast<Regex>(args[0]);
	auto &subject = raw_cast<String>(args[1]);


	return regex.match(subject);
}

static Variant regex_match2(Runtime &, std::span<Variant> args)
{
	auto &regex = raw_cast<Regex>(args[0]);
	auto &subject = raw_cast<String>(args[1]);
	intptr_t pos = raw_cast<intptr_t>(args[2]);

	return regex.match(subject, pos);
}

static Variant regex_has_match(Runtime &, std::span<Variant> args)
{
	auto &regex = raw_cast<Regex>(args[0]);
	return regex.has_match();
}

static Variant regex_count(Runtime &, std::span<Variant> args)
{
	auto &regex = raw_cast<Regex>(args[0]);
	return regex.count();
}

static Variant regex_group(Runtime &, std::span<Variant> args)
{
	auto &regex = raw_cast<Regex>(args[0]);
	intptr_t i = raw_cast<intptr_t>(args[1]);
	if (!regex.has_match() || i < 0 || i > regex.count()) {
		throw error("[Index error] Invalid group index in regular expression: %", i);
	}

	return regex.capture(i);
}

static Variant regex_get_start(Runtime &, std::span<Variant> args)
{
	auto &regex = raw_cast<Regex>(args[0]);
	intptr_t i = raw_cast<intptr_t>(args[1]);
	if (!regex.has_match() || i < 0 || i > regex.count()) {
		throw error("[Index error] Invalid group index in regular expression: %", i);
	}

	return regex.capture_start(i);
}

static Variant regex_get_end(Runtime &, std::span<Variant> args)
{
	auto &regex = raw_cast<Regex>(args[0]);
	intptr_t i = raw_cast<intptr_t>(args[1]);
	if (!regex.has_match() || i < 0 || i > regex.count()) {
		throw error("[Index error] Invalid group index in regular expression: %", i);
	}

	return regex.capture_end(i);
}

} // namespace calao

#endif // CALAO_FUNC_REGEX_HPP
