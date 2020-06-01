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

#include <algorithm>
#include <random>
#include <calao/runtime.hpp>

namespace calao {

static Variant list_contains(ArgumentList &args)
{
	auto &lst = args.raw_get<List>(0).items();
	return lst.contains(args[1]);
}

static Variant list_first(ArgumentList &args)
{
	auto &lst = args.raw_get<List>(0).items();
	if (lst.empty()) {
		throw error("[Index error] Cannot get first element in empty list");
	}
	return lst.first();
}

static Variant list_find1(ArgumentList &args)
{
	auto &lst = args.raw_get<List>(0).items();
	return lst.find(args[1]);
}

static Variant list_find2(ArgumentList &args)
{
	auto &lst = args.raw_get<List>(0).items();
	intptr_t i = args.raw_get<intptr_t>(1);
	return lst.find(args[1], i);
}

static Variant list_rfind1(ArgumentList &args)
{
	auto &lst = args.raw_get<List>(0).items();
	return lst.rfind(args[1]);
}

static Variant list_rfind2(ArgumentList &args)
{
	auto &lst = args.raw_get<List>(0).items();
	intptr_t i = args.raw_get<intptr_t>(1);
	return lst.rfind(args[1], i);
}

static Variant list_last(ArgumentList &args)
{
	auto &lst = args.raw_get<List>(0).items();
	if (lst.empty()) {
		throw error("[Index error] Cannot get last element in empty list");
	}
	return lst.last();
}

static Variant list_left(ArgumentList &args)
{
	auto &lst = args.raw_get<List>(0).items();
	intptr_t count = args.raw_get<intptr_t>(1);
	Array<Variant> result;

	for (intptr_t i = 1; i <= count; i++) {
		result.append(lst.at(i));
	}

	return make_handle<List>(&args.runtime(), std::move(result));
}

static Variant list_right(ArgumentList &args)
{
	auto &lst = args.raw_get<List>(0).items();
	intptr_t count = args.raw_get<intptr_t>(1);
	Array<Variant> result;
	intptr_t limit = lst.size() - count;

	for (intptr_t i = lst.size(); i > limit; i--) {
		result.append(lst.at(i));
	}

	return make_handle<List>(&args.runtime(), std::move(result));
}

static Variant list_join(ArgumentList &args)
{
	auto &lst = args.raw_get<List>(0).items();
	auto &delim = args.raw_get<String>(1);
	String result;

	for (intptr_t i = 1; i <= lst.size(); i++)
	{
		result.append(lst[i].to_string());
		if (i != lst.size()) {
			result.append(delim);
		}
	}

	return result;
}

static Variant list_clear(ArgumentList &args)
{
	auto &lst = args.raw_get<List>(0).items();
	lst.clear();

	return Variant();
}

static Variant list_append1(ArgumentList &args)
{
	auto &lst = args.raw_get<List>(0).items();
	lst.append(args[1]);

	return Variant();
}

static Variant list_append2(ArgumentList &args)
{
	auto &lst1 = args.raw_get<List>(0).items();
	auto &lst2 = args.raw_get<List>(0).items();

	for (auto &item : lst2) {
		lst1.append(item.resolve());
	}

	return Variant();
}

static Variant list_prepend1(ArgumentList &args)
{
	auto &lst = args.raw_get<List>(0).items();
	lst.prepend(args[1]);

	return Variant();
}

static Variant list_prepend2(ArgumentList &args)
{
	auto &lst1 = args.raw_get<List>(0).items();
	auto &lst2 = args.raw_get<List>(0).items();

	for (intptr_t i = lst2.size(); i > 0; i--) {
		lst1.prepend(lst2[i].resolve());
	}

	return Variant();
}

static Variant list_is_empty(ArgumentList &args)
{
	auto &lst = args.raw_get<List>(0).items();

	return lst.empty();
}

static Variant list_pop(ArgumentList &args)
{
	auto &lst = args.raw_get<List>(0).items();

	return lst.take_last().resolve();
}

static Variant list_shift(ArgumentList &args)
{
	auto &lst = args.raw_get<List>(0).items();

	return lst.take_first().resolve();
}

static Variant list_sort(ArgumentList &args)
{
	auto &lst = args.raw_get<List>(0).items();
	std::sort(lst.begin(), lst.end());

	return Variant();
}

static Variant list_reverse(ArgumentList &args)
{
	auto &lst = args.raw_get<List>(0).items();
	std::reverse(lst.begin(), lst.end());

	return Variant();
}

static Variant list_remove(ArgumentList &args)
{
	auto &lst = args.raw_get<List>(0).items();
	lst.remove(args[1].resolve());

	return Variant();
}

static Variant list_remove_first(ArgumentList &args)
{
	auto &lst = args.raw_get<List>(0).items();
	lst.remove_first(args[1].resolve());

	return Variant();
}

static Variant list_remove_last(ArgumentList &args)
{
	auto &lst = args.raw_get<List>(0).items();
	lst.remove_last(args[1].resolve());

	return Variant();
}

static Variant list_remove_at(ArgumentList &args)
{
	auto &lst = args.raw_get<List>(0).items();
	intptr_t pos = args.raw_get<intptr_t>(1);
	lst.remove_at(pos);

	return Variant();
}

static Variant list_shuffle(ArgumentList &args)
{
	auto &lst = args.raw_get<List>(0).items();
	std::random_device rd;
	std::mt19937 g(rd());
	std::shuffle(lst.begin(), lst.end(), g);

	return Variant();
}

static Variant list_sample(ArgumentList &args)
{
	auto &lst = args.raw_get<List>(0).items();
	intptr_t n = args.raw_get<intptr_t>(1);
	Array<Variant> result;
	std::random_device rd;
	std::mt19937 g(rd());
	std::sample(lst.begin(), lst.end(), std::back_inserter(result), n, g);

	return make_handle<List>(&args.runtime(), std::move(result));
}

static Variant list_insert(ArgumentList &args)
{
	auto &lst = args.raw_get<List>(0).items();
	intptr_t pos = args.raw_get<intptr_t>(1);
	lst.insert(pos, args[2]);

	return Variant();
}



} // namespace calao

#endif // CALAO_FUNC_LIST_HPP
