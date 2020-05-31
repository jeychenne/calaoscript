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
 * Purpose: String builtin functions.                                                                                 *
 *                                                                                                                    *
 **********************************************************************************************************************/

#ifndef CALAO_FUNC_STRING_HPP
#define CALAO_FUNC_STRING_HPP

#include <calao/runtime.hpp>

namespace calao {

static Variant string_contains(Runtime &, std::span<Variant> args)
{
	auto &s1 = raw_cast<String>(args[0]);
	auto &s2 = raw_cast<String>(args[1]);

	return s1.contains(s2);
}

static Variant string_startswith(Runtime &, std::span<Variant> args)
{
	auto &s1 = raw_cast<String>(args[0]);
	auto &s2 = raw_cast<String>(args[1]);

	return s1.starts_with(s2);
}

static Variant string_endswith(Runtime &, std::span<Variant> args)
{
	auto &s1 = raw_cast<String>(args[0]);
	auto &s2 = raw_cast<String>(args[1]);

	return s1.ends_with(s2);
}

static Variant string_find1(Runtime &, std::span<Variant> args)
{
	auto &s1 = raw_cast<String>(args[0]);
	auto &s2 = raw_cast<String>(args[1]);

	return s1.find(s2);
}

static Variant string_find2(Runtime &, std::span<Variant> args)
{
	auto &s1 = raw_cast<String>(args[0]);
	auto &s2 = raw_cast<String>(args[1]);
	intptr_t i = raw_cast<intptr_t>(args[2]);

	return s1.find(s2, i);
}

static Variant string_rfind1(Runtime &, std::span<Variant> args)
{
	auto &s1 = raw_cast<String>(args[0]);
	auto &s2 = raw_cast<String>(args[1]);

	return s1.rfind(s2);
}

static Variant string_rfind2(Runtime &, std::span<Variant> args)
{
	auto &s1 = raw_cast<String>(args[0]);
	auto &s2 = raw_cast<String>(args[1]);
	intptr_t i = raw_cast<intptr_t>(args[2]);

	return s1.rfind(s2, i);
}

static Variant string_left(Runtime &, std::span<Variant> args)
{
	auto &s1 = raw_cast<String>(args[0]);
	auto count = raw_cast<intptr_t>(args[1]);

	return s1.left(count);
}

static Variant string_right(Runtime &, std::span<Variant> args)
{
	auto &s1 = raw_cast<String>(args[0]);
	auto count = raw_cast<intptr_t>(args[1]);

	return s1.right(count);
}

static Variant string_mid1(Runtime &, std::span<Variant> args)
{
	auto &s1 = raw_cast<String>(args[0]);
	auto from = raw_cast<intptr_t>(args[1]);

	return s1.mid(from);
}

static Variant string_mid2(Runtime &, std::span<Variant> args)
{
	auto &s1 = raw_cast<String>(args[0]);
	auto from = raw_cast<intptr_t>(args[1]);
	auto count = raw_cast<intptr_t>(args[2]);

	return s1.mid(from, count);
}

static Variant string_first(Runtime &, std::span<Variant> args)
{
	auto &s1 = raw_cast<String>(args[0]);
	return s1.left(1);
}

static Variant string_last(Runtime &, std::span<Variant> args)
{
	auto &s1 = raw_cast<String>(args[0]);
	return s1.right(1);
}

static Variant string_count(Runtime &, std::span<Variant> args)
{
	auto &s1 = raw_cast<String>(args[0]);
	auto &s2 = raw_cast<String>(args[1]);

	return s1.count(s2);
}

static Variant string_toupper(Runtime &, std::span<Variant> args)
{
	auto &s = raw_cast<String>(args[0]);
	return s.to_upper();
}

static Variant string_tolower(Runtime &, std::span<Variant> args)
{
	auto &s = raw_cast<String>(args[0]);
	return s.to_lower();
}

static Variant string_reverse(Runtime &, std::span<Variant> args)
{
	auto &s = raw_cast<String>(args[0]);
	return s.reverse();
}

static Variant string_isempty(Runtime &, std::span<Variant> args)
{
	auto &s = raw_cast<String>(args[0]);
	return s.empty();
}

static Variant string_char(Runtime &, std::span<Variant> args)
{
	auto &s = raw_cast<String>(args[0]);
	intptr_t i = raw_cast<intptr_t>(args[1]);

	return s.next_grapheme(i);
}

static Variant string_split(Runtime &rt, std::span<Variant> args)
{
	auto &s = raw_cast<String>(args[0]);
	auto &delim = raw_cast<String>(args[1]);
	List result;
	for (auto &p : s.split(delim)) {
		result.append(std::move(p));
	}

	return make_handle<List>(&rt, std::move(result));
}

} // namespace calao

#endif // CALAO_FUNC_STRING_HPP
