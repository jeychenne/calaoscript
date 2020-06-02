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

static Variant file_read_line(ArgumentList &args)
{
	auto &f = args.raw_get<File>(0);
	return f.read_line();
}

static Variant file_write_line(ArgumentList &args)
{
	auto &f = args.raw_get<File>(0);
	auto &text = args.raw_get<String>(1);
	f.write_line(text);

	return Variant();
}

static Variant file_write_lines(ArgumentList &args)
{
	auto &f = args.raw_get<File>(0);
	auto &lines = args.raw_get<List>(1).items();
	for (auto &line : lines) {
		f.write_line(line.to_string());
	}

	return Variant();
}

static Variant file_write(ArgumentList &args)
{
	auto &f = args.raw_get<File>(0);
	auto &text = args.raw_get<String>(1);
	f.write(text);

	return Variant();
}

static Variant file_close(ArgumentList &args)
{
	auto &f = args.raw_get<File>(0);
	f.close();

	return Variant();
}

static Variant file_read_all1(ArgumentList &args)
{
	auto &f = args.raw_get<File>(0);
	String line, text;

	while (!f.at_end())
	{
		text.append(f.read_line());
	}

	return text;
}

static Variant file_read_all2(ArgumentList &args)
{
	auto &path = args.raw_get<String>(0);
	return File::read_all(path);
}

static Variant file_read_lines(ArgumentList &args)
{
	auto &f = args.raw_get<File>(0);
	Array<Variant> result;

	for (auto &ln : f.read_lines()) {
		result.append(std::move(ln));
	}

	return make_handle<List>(&args.runtime(), std::move(result));
}

static Variant file_seek(ArgumentList &args)
{
	auto &f = args.raw_get<File>(0);
	auto pos = args.raw_get<intptr_t>(1);
	f.seek(pos);

	return Variant();
}

static Variant file_tell(ArgumentList &args)
{
	auto &f = args.raw_get<File>(0);
	return f.tell();
}

static Variant file_eof(ArgumentList &args)
{
	auto &f = args.raw_get<File>(0);
	return f.at_end();
}

} // namespace calao

#endif // CALAO_FUNC_FILE_HPP
