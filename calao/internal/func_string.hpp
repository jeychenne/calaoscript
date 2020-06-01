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

static Variant string_contains(ArgumentList &args)
{
	auto &s1 = args.raw_get<String>(0);
	auto &s2 = args.raw_get<String>(1);

	return s1.contains(s2);
}

static Variant string_startswith(ArgumentList &args)
{
	auto &s1 = args.raw_get<String>(0);
	auto &s2 = args.raw_get<String>(1);

	return s1.starts_with(s2);
}

static Variant string_endswith(ArgumentList &args)
{
	auto &s1 = args.raw_get<String>(0);
	auto &s2 = args.raw_get<String>(1);

	return s1.ends_with(s2);
}

static Variant string_find1(ArgumentList &args)
{
	auto &s1 = args.raw_get<String>(0);
	auto &s2 = args.raw_get<String>(1);

	return s1.find(s2);
}

static Variant string_find2(ArgumentList &args)
{
	auto &s1 = args.raw_get<String>(0);
	auto &s2 = args.raw_get<String>(1);
	intptr_t i = args.raw_get<intptr_t>(2);

	return s1.find(s2, i);
}

static Variant string_rfind1(ArgumentList &args)
{
	auto &s1 = args.raw_get<String>(0);
	auto &s2 = args.raw_get<String>(1);

	return s1.rfind(s2);
}

static Variant string_rfind2(ArgumentList &args)
{
	auto &s1 = args.raw_get<String>(0);
	auto &s2 = args.raw_get<String>(1);
	intptr_t i = args.raw_get<intptr_t>(2);

	return s1.rfind(s2, i);
}

static Variant string_left(ArgumentList &args)
{
	auto &s1 = args.raw_get<String>(0);
	auto count = args.raw_get<intptr_t>(1);

	return s1.left(count);
}

static Variant string_right(ArgumentList &args)
{
	auto &s1 = args.raw_get<String>(0);
	auto count = args.raw_get<intptr_t>(1);

	return s1.right(count);
}

static Variant string_mid1(ArgumentList &args)
{
	auto &s1 = args.raw_get<String>(0);
	auto from = args.raw_get<intptr_t>(1);

	return s1.mid(from);
}

static Variant string_mid2(ArgumentList &args)
{
	auto &s1 = args.raw_get<String>(0);
	auto from = args.raw_get<intptr_t>(1);
	auto count = args.raw_get<intptr_t>(2);

	return s1.mid(from, count);
}

static Variant string_first(ArgumentList &args)
{
	auto &s1 = args.raw_get<String>(0);
	return s1.left(1);
}

static Variant string_last(ArgumentList &args)
{
	auto &s1 = args.raw_get<String>(0);
	return s1.right(1);
}

static Variant string_count(ArgumentList &args)
{
	auto &s1 = args.raw_get<String>(0);
	auto &s2 = args.raw_get<String>(1);

	return s1.count(s2);
}

static Variant string_toupper(ArgumentList &args)
{
	auto &s = args.raw_get<String>(0);
	return s.to_upper();
}

static Variant string_tolower(ArgumentList &args)
{
	auto &s = args.raw_get<String>(0);
	return s.to_lower();
}

static Variant string_reverse(ArgumentList &args)
{
	auto &s = args.raw_get<String>(0);
	return s.reverse();
}

static Variant string_isempty(ArgumentList &args)
{
	auto &s = args.raw_get<String>(0);
	return s.empty();
}

static Variant string_char(ArgumentList &args)
{
	auto &s = args.raw_get<String>(0);
	intptr_t i = args.raw_get<intptr_t>(1);

	return s.next_grapheme(i);
}

static Variant string_split(ArgumentList &args)
{
	auto &s = args.raw_get<String>(0);
	auto &delim = args.raw_get<String>(1);
	List result;
	for (auto &p : s.split(delim)) {
		result.append(std::move(p));
	}

	return make_handle<List>(&args.runtime(), std::move(result));
}

static Variant string_append(ArgumentList &args)
{
	auto &s1 = args.raw_get<String>(0);
	auto &s2 = args.raw_get<String>(1);
	s1.append(s2);

	return Variant();
}

} // namespace calao

#endif // CALAO_FUNC_STRING_HPP
