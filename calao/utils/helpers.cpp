/**********************************************************************************************************************
 *                                                                                                                    *
 * Copyright (C) 2019-2020 Julien Eychenne <jeychenne@gmail.com>                                                      *
 *                                                                                                                    *
 * The contents of this file are subject to the Mozilla Public License Version 2.0 (the "License"); you may not use   *
 * this file except in compliance with the License. You may obtain a copy of the License at                           *
 * http://www.mozilla.org/MPL/.                                                                                       *
 *                                                                                                                    *
 * Created: 20/02/2019                                                                                                *
 *                                                                                                                    *
 * Purpose: see header.                                                                                               *
 *                                                                                                                    *
 **********************************************************************************************************************/

#include <ctime>
#include <sstream>
#include <calao/string.hpp>

#if CALAO_WINDOWS
#	include <wchar.h>
#else
#	include <cstring>
#endif

#include <calao/utils/helpers.hpp>

namespace calao { namespace utils {

static size_t the_random_seed = 0;

size_t random_seed()
{
	return the_random_seed;
}

void init_random_seed()
{
	srand((unsigned int) time(nullptr));
    the_random_seed = (size_t) rand();
}

FILE *open_file(const String &path, const char *mode)
{
#if CALAO_WINDOWS
	auto wpath = path.to_wide();
    auto wmode = String::to_wide(mode);

    return _wfopen(wpath.data(), wmode.data());
#else
	return fopen(path.data(), mode);
#endif
}

FILE *reopen_file(const String &path, const char *mode, FILE *stream)
{
#if CALAO_WINDOWS
	auto wpath = path.to_wide();
    auto wmode = String::to_wide(mode);

    return _wfreopen(wpath.data(), wmode.data(), stream);
#else
	return freopen(path.data(), mode, stream);
#endif
}

#if !defined(CALAO_ENDIANNES_KNOWN) && !CALAO_WINDOWS
bool is_big_endian()
{
	union {
		uint32_t i;
		char c[4];
	} val = {0x01020304};

	return val.c[0] == 1;
}
#endif // check endianness


String new_uuid(size_t len)
{
	// Credits: https://stackoverflow.com/a/440240
	static const char chars[] = "0123456789_ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	String s(len+1, true);

	for (size_t i = 0; i < len; ++i) {
		char c = chars[rand() % (sizeof(chars) - 1)];
		s.append(c);
	}

	return s;
}

}} // namespace::utils
