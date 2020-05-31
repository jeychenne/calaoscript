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
 * Purpose: List builtin functions.                                                                                   *
 *                                                                                                                    *
 **********************************************************************************************************************/

#ifndef CALAO_FUNC_LIST_HPP
#define CALAO_FUNC_LIST_HPP

#include <calao/runtime.hpp>

namespace calao {

static Variant list_contains(Runtime &, std::span<Variant> args)
{
	auto &lst = raw_cast<List>(args[0]);
	return lst.contains(args[1]);
}

static Variant list_first(Runtime &, std::span<Variant> args)
{
	auto &lst = raw_cast<List>(args[0]);
	if (lst.empty()) {
		throw error("[Index error] Cannot get first element in empty list");
	}
	return lst.first();
}

static Variant list_find1(Runtime &, std::span<Variant> args)
{
	auto &lst = raw_cast<List>(args[0]);
	return lst.find(args[1]);
}

static Variant list_find2(Runtime &, std::span<Variant> args)
{
	auto &lst = raw_cast<List>(args[0]);
	intptr_t i = raw_cast<intptr_t>(args[1]);
	return lst.find(args[1], i);
}

static Variant list_rfind1(Runtime &, std::span<Variant> args)
{
	auto &lst = raw_cast<List>(args[0]);
	return lst.rfind(args[1]);
}

static Variant list_rfind2(Runtime &, std::span<Variant> args)
{
	auto &lst = raw_cast<List>(args[0]);
	intptr_t i = raw_cast<intptr_t>(args[1]);
	return lst.rfind(args[1], i);
}

static Variant list_last(Runtime &, std::span<Variant> args)
{
	auto &lst = raw_cast<List>(args[0]);
	if (lst.empty()) {
		throw error("[Index error] Cannot get last element in empty list");
	}
	return lst.last();
}

static Variant list_left(Runtime &rt, std::span<Variant> args)
{
	auto &lst = raw_cast<List>(args[0]);
	intptr_t count = raw_cast<intptr_t>(args[1]);
	List result;

	for (intptr_t i = 1; i <= count; i++) {
		result.append(lst.at(i));
	}

	return make_handle<List>(&rt, std::move(result));
}

static Variant list_right(Runtime &rt, std::span<Variant> args)
{
	auto &lst = raw_cast<List>(args[0]);
	intptr_t count = raw_cast<intptr_t>(args[1]);
	List result;
	intptr_t limit = lst.size() - count;

	for (intptr_t i = lst.size(); i > limit; i--) {
		result.append(lst.at(i));
	}

	return make_handle<List>(&rt, std::move(result));
}

} // namespace calao

#endif // CALAO_FUNC_LIST_HPP
