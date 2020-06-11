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
 * Purpose: Module builtin functions.                                                                                 *
 *                                                                                                                    *
 **********************************************************************************************************************/

#ifndef PHONSCRIPT_FUNC_MODULE_HPP
#define PHONSCRIPT_FUNC_MODULE_HPP

#include <phon/runtime/module.hpp>

namespace phonometrica {

static Variant module_init(Runtime &rt, std::span<Variant>args)
{
	auto &name = raw_cast<String>(args[0]);
	return make_handle<Module>(&rt, name);
}

static Variant module_get_attr(Runtime &, std::span<Variant> args)
{
	auto &m = raw_cast<Module>(args[0]);
	auto key = raw_cast<String>(args[1]);

	return m.get(key);
}

static Variant module_set_attr(Runtime &, std::span<Variant> args)
{
	auto &m = raw_cast<Module>(args[0]);
	auto key = raw_cast<String>(args[1]);
	m[key] = args[2].resolve();

	return Variant();
}

} // namespace phonometrica

#endif // PHONSCRIPT_FUNC_MODULE_HPP
