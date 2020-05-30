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
 * Purpose: builtin functions.                                                                                        *
 *                                                                                                                    *
 **********************************************************************************************************************/

#include <calao/runtime.hpp>

#define CLS(T) get_class<T>()

namespace calao {

static Variant get_type(Runtime &, std::span<Variant> args)
{
	return args[0].get_class()->object();
}

//---------------------------------------------------------------------------------------------------------------------

// String type

static Variant string_contains(Runtime &, std::span<Variant> args)
{
	auto &s1 = unsafe_cast<String>(args[0]);
	auto &s2 = unsafe_cast<String>(args[1]);

	return s1.contains(s2);
}

static Variant string_startswith(Runtime &, std::span<Variant> args)
{
	auto &s1 = unsafe_cast<String>(args[0]);
	auto &s2 = unsafe_cast<String>(args[1]);

	return s1.starts_with(s2);
}

static Variant string_endswith(Runtime &, std::span<Variant> args)
{
	auto &s1 = unsafe_cast<String>(args[0]);
	auto &s2 = unsafe_cast<String>(args[1]);

	return s1.ends_with(s2);
}

static Variant string_find(Runtime &, std::span<Variant> args)
{
	auto &s1 = unsafe_cast<String>(args[0]);
	auto &s2 = unsafe_cast<String>(args[1]);

	return s1.find(s2);
}

static Variant string_rfind(Runtime &, std::span<Variant> args)
{
	auto &s1 = unsafe_cast<String>(args[0]);
	auto &s2 = unsafe_cast<String>(args[1]);

	return s1.rfind(s2);
}

static Variant string_left(Runtime &, std::span<Variant> args)
{
	auto &s1 = unsafe_cast<String>(args[0]);
	auto count = unsafe_cast<intptr_t>(args[1]);

	return s1.left(count);
}

static Variant string_right(Runtime &, std::span<Variant> args)
{
	auto &s1 = unsafe_cast<String>(args[0]);
	auto count = unsafe_cast<intptr_t>(args[1]);

	return s1.right(count);
}

static Variant string_mid1(Runtime &, std::span<Variant> args)
{
	auto &s1 = unsafe_cast<String>(args[0]);
	auto from = unsafe_cast<intptr_t>(args[1]);

	return s1.mid(from);
}

static Variant string_mid2(Runtime &, std::span<Variant> args)
{
	auto &s1 = unsafe_cast<String>(args[0]);
	auto from = unsafe_cast<intptr_t>(args[1]);
	auto count = unsafe_cast<intptr_t>(args[2]);

	return s1.mid(from, count);
}

//---------------------------------------------------------------------------------------------------------------------

void Runtime::set_global_namespace()
{
	add_global("type", get_type, { CLS(Object) });
	add_global("contains", string_contains, { CLS(String), CLS(String) });
	add_global("starts_with", string_startswith, { CLS(String), CLS(String) });
	add_global("ends_with", string_endswith, { CLS(String), CLS(String) });
	add_global("find", string_find, { CLS(String), CLS(String) });
	add_global("rfind", string_rfind, { CLS(String), CLS(String) });
	add_global("left", string_left, { CLS(String), CLS(intptr_t) });
	add_global("right", string_right, { CLS(String), CLS(intptr_t) });
	add_global("mid", string_mid1, { CLS(String), CLS(intptr_t) });
	add_global("mid", string_mid2, { CLS(String), CLS(intptr_t), CLS(intptr_t) });
}

} // namespace calao

#undef CLS