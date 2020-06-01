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
 * Purpose: File builtin functions.                                                                                   *
 *                                                                                                                    *
 **********************************************************************************************************************/

#ifndef CALAO_FUNC_FILE_HPP
#define CALAO_FUNC_FILE_HPP

#include <calao/file.hpp>
#include <calao/runtime.hpp>

namespace calao {

static Variant file_open1(ArgumentList &args)
{
	auto &path = args.raw_get<String>(0);
	return make_handle<File>(path);
}

static Variant file_open2(ArgumentList &args)
{
	auto &path = args.raw_get<String>(0);
	auto &mode = args.raw_get<String>(1);

	return make_handle<File>(path, mode.data());
}

static Variant file_readline(ArgumentList &args)
{
	auto &f = args.raw_get<File>(0);
	return f.read_line();
}

} // namespace calao

#endif // CALAO_FUNC_FILE_HPP
