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

namespace calao {

static Variant get_type(ArgumentList &args)
{
	return args[0].get_class()->object();
}

static Variant get_length(ArgumentList &args)
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

	throw error("[Type error] Cannot calculate length of % value", v.class_name());
}

} // namespace calao

#endif // CALAO_FUNC_GENERIC_HPP
