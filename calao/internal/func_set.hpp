/**********************************************************************************************************************
 *                                                                                                                    *
 * Copyright (C) 2019-2020 Julien Eychenne <jeychenne@gmail.com>                                                      *
 *                                                                                                                    *
 * The contents of this file are subject to the Mozilla Public License Version 2.0 (the "License"); you may not use   *
 * this file except in compliance with the License. You may obtain a copy of the License at                           *
 * http://www.mozilla.org/MPL/.                                                                                       *
 *                                                                                                                    *
 * Created: 05/06/2019                                                                                                *
 *                                                                                                                    *
 * Purpose: Set builtin functions.                                                                                    *
 *                                                                                                                    *
 **********************************************************************************************************************/

#ifndef CALAO_FUNC_SET_HPP
#define CALAO_FUNC_SET_HPP

#include <calao/set.hpp>
#include <calao/runtime.hpp>

namespace calao {

static Variant set_init(Runtime &rt, std::span<Variant>)
{
	return make_handle<Set>(&rt);
}

static Variant set_contains(Runtime &, std::span<Variant> args)
{
	auto &set = raw_cast<Set>(args[0]);
	return set.contains(args[1].resolve());
}

static Variant set_insert(Runtime &, std::span<Variant> args)
{
	auto &set = raw_cast<Set>(args[0]).items();
	set.insert(args[1].resolve());

	return Variant();
}

static Variant set_remove(Runtime &, std::span<Variant> args)
{
	auto &set = raw_cast<Set>(args[0]).items();
	set.erase(args[1].resolve());

	return Variant();
}

static Variant set_is_empty(Runtime &, std::span<Variant> args)
{
	auto &set = raw_cast<Set>(args[0]).items();
	return set.empty();
}

static Variant set_clear(Runtime &, std::span<Variant> args)
{
	auto &set = raw_cast<Set>(args[0]).items();
	set.clear();

	return Variant();
}

} // namespace calao

#endif // CALAO_FUNC_SET_HPP
