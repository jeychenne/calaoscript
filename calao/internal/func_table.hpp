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
 * Purpose: Table builtin functions.                                                                                  *
 *                                                                                                                    *
 **********************************************************************************************************************/

#ifndef CALAO_FUNC_TABLE_HPP
#define CALAO_FUNC_TABLE_HPP

#include <calao/table.hpp>
#include <calao/runtime.hpp>

namespace calao {

static Variant table_contains(ArgumentList &args)
{
	auto &map = args.raw_get<Table>(0).map();
	return map.contains(args[1]);
}

static Variant table_is_empty(ArgumentList &args)
{
	auto &map = args.raw_get<Table>(0).map();
	return map.empty();
}

static Variant table_clear(ArgumentList &args)
{
	auto &map = args.raw_get<Table>(0).map();
	map.clear();

	return Variant();
}

static Variant table_get_keys(ArgumentList &args)
{
	auto &tab = args.raw_get<Table>(0);
	return make_handle<List>(&args.runtime(), tab.keys());
}

static Variant table_get_values(ArgumentList &args)
{
	auto &tab = args.raw_get<Table>(0);
	return make_handle<List>(&args.runtime(), tab.values());
}

static Variant table_remove(ArgumentList &args)
{
	auto &map = args.raw_get<Table>(0).map();
	map.erase(args[1]);

	return Variant();
}

static Variant table_get1(ArgumentList &args)
{
	auto &map = args.raw_get<Table>(0).map();
	auto it = map.find(args[1]);

	return (it != map.end()) ? it->second : Variant();
}

static Variant table_get2(ArgumentList &args)
{
	auto &map = args.raw_get<Table>(0).map();
	auto it = map.find(args[1]);

	return (it != map.end()) ? it->second : args[2];
}

} // namespace calao

#endif // CALAO_FUNC_TABLE_HPP
