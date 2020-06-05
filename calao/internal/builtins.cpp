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

#include <calao/internal/func_generic.hpp>
#include <calao/internal/func_file.hpp>
#include <calao/internal/func_string.hpp>
#include <calao/internal/func_list.hpp>
#include <calao/internal/func_table.hpp>
#include <calao/internal/func_regex.hpp>
#include <calao/internal/func_set.hpp>

#define CLS(T) get_class<T>()
#define REF(bits) ParamBitset(bits)

namespace calao {

void Runtime::set_global_namespace()
{
	// Generic functions
	add_global("type", get_type, { CLS(Object) });
	add_global("len", get_length, { CLS(Object) });
	add_global("str", to_string, { CLS(Object) });
	add_global("bool", to_boolean, { CLS(Object) });
	add_global("int", to_integer, { CLS(Object) });
	add_global("float", to_float, { CLS(Object) });

	// String
	add_global("contains", string_contains, { CLS(String), CLS(String) });
	add_global("starts_with", string_starts_with, {CLS(String), CLS(String)});
	add_global("ends_with", string_ends_with, {CLS(String), CLS(String)});
	add_global("find", string_find1, { CLS(String), CLS(String) });
	add_global("find", string_find2, { CLS(String), CLS(String), CLS(intptr_t) });
	add_global("rfind", string_rfind1, { CLS(String), CLS(String) });
	add_global("rfind", string_rfind2, { CLS(String), CLS(String), CLS(intptr_t) });
	add_global("left", string_left, { CLS(String), CLS(intptr_t) });
	add_global("right", string_right, { CLS(String), CLS(intptr_t) });
	add_global("mid", string_mid1, { CLS(String), CLS(intptr_t) });
	add_global("mid", string_mid2, { CLS(String), CLS(intptr_t), CLS(intptr_t) });
	add_global("first", string_first, {CLS(String)});
	add_global("last", string_last, {CLS(String)});
	add_global("count", string_count,  { CLS(String), CLS(String) });
	add_global("to_upper", string_to_upper, {CLS(String)});
	add_global("to_lower", string_to_lower, {CLS(String)});
	add_global("reverse", string_reverse, { CLS(String) }, REF("1"));
	add_global("is_empty", string_is_empty, {CLS(String)});
	add_global("char", string_char, { CLS(String), CLS(intptr_t) });
	add_global("split", string_split, { CLS(String), CLS(String) });
	add_global("append", string_append, { CLS(String), CLS(String) }, REF("01"));
	add_global("prepend", string_prepend, { CLS(String), CLS(String) }, REF("01"));
	add_global("insert", string_insert, { CLS(String), CLS(intptr_t), CLS(String) }, REF("001"));
	add_global("trim", string_trim, { CLS(String) }, REF("1"));
	add_global("ltrim", string_ltrim, { CLS(String) }, REF("1"));
	add_global("rtrim", string_rtrim, { CLS(String) }, REF("1"));
	add_global("remove", string_remove, { CLS(String), CLS(String) }, REF("01"));
	add_global("remove_first", string_remove_first, {CLS(String), CLS(String)}, REF("01"));
	add_global("remove_last", string_remove_last, {CLS(String), CLS(String)}, REF("01"));
	add_global("remove_at", string_remove_at, {CLS(String), CLS(intptr_t), CLS(intptr_t)}, REF("001"));
	add_global("replace", string_replace, { CLS(String), CLS(String), CLS(String) }, REF("001"));
	add_global("replace_first", string_replace_first, {CLS(String), CLS(String), CLS(String)}, REF("001"));
	add_global("replace_last", string_replace_last, {CLS(String), CLS(String), CLS(String)}, REF("001"));
	add_global("replace_at", string_replace_at, {CLS(String), CLS(intptr_t), CLS(intptr_t), CLS(String)}, REF("0001"));

	// List
	add_global("contains", list_contains, { CLS(List), CLS(Object) });
	add_global("first", list_first, { CLS(List) });
	add_global("last", list_last, { CLS(List) });
	add_global("find", list_find1, { CLS(List), CLS(Object) });
	add_global("find", list_find2, { CLS(List), CLS(Object), CLS(intptr_t) });
	add_global("rfind", list_rfind1, { CLS(List), CLS(Object) });
	add_global("rfind", list_rfind2, { CLS(List), CLS(Object), CLS(intptr_t) });
	add_global("left", list_left, { CLS(List), CLS(intptr_t) });
	add_global("right", list_right, { CLS(List), CLS(intptr_t) });
	add_global("join", list_join, { CLS(List), CLS(String) });
	add_global("clear", list_clear, { CLS(List) }, REF("1"));
	add_global("append", list_append1, { CLS(List), CLS(Object) }, REF("01"));
	add_global("append", list_append2, { CLS(List), CLS(List) }, REF("01"));
	add_global("prepend", list_prepend1, { CLS(List), CLS(Object) }, REF("01"));
	add_global("prepend", list_prepend2, { CLS(List), CLS(List) }, REF("01"));
	add_global("is_empty", list_is_empty, {CLS(List)});
	add_global("pop", list_pop, { CLS(List) }, REF("1"));
	add_global("shift", list_shift, { CLS(List) }, REF("1"));
	add_global("sort", list_sort, { CLS(List) }, REF("1"));
	add_global("reverse", list_reverse, { CLS(List) }, REF("1"));
	add_global("remove", list_remove, { CLS(List), CLS(Object) }, REF("01"));
	add_global("remove_first", list_remove_first, { CLS(List), CLS(Object) }, REF("01"));
	add_global("remove_last", list_remove_last, { CLS(List), CLS(Object) }, REF("01"));
	add_global("remove_at", list_remove_at, { CLS(List), CLS(intptr_t) }, REF("01"));
	add_global("shuffle", list_shuffle, { CLS(List) }, REF("1"));
	add_global("sample", list_sample, { CLS(List), CLS(intptr_t) });
	add_global("insert", list_insert, { CLS(List), CLS(intptr_t), CLS(Object) }, REF("001"));

	// File
	add_global("open", file_open1, { CLS(String) });
	add_global("open", file_open2, { CLS(String), CLS(String) });
	add_global("read_line", file_read_line, { CLS(File) });
	add_global("read_lines", file_read_lines, { CLS(File) });
	add_global("write_line", file_write_line, {CLS(File), CLS(String) });
	add_global("write_lines", file_write_lines, {CLS(File), CLS(List) });
	add_global("write", file_write, {CLS(File), CLS(String) });
	add_global("close", file_close, {CLS(File) });
	add_global("read", file_read_all1, { CLS(File) });
	add_global("read", file_read_all2, { CLS(String) });
	add_global("tell", file_tell, { CLS(File) });
	add_global("seek", file_seek, { CLS(File), CLS(intptr_t) });
	add_global("eof", file_eof, { CLS(File) });
	auto file_class = Class::get<File>();
	auto &v = (*globals)["open"];
	file_class->add_initializer(v.handle<Function>());

	// Table
	add_global("contains", table_contains, { CLS(Table), CLS(Object) });
	add_global("is_empty", table_is_empty, { CLS(Table) });
	add_global("clear", table_clear, { CLS(Table) }, REF("1"));
	add_global("get_keys", table_get_keys, { CLS(Table) });
	add_global("get_values", table_get_values, { CLS(Table) });
	add_global("remove", table_remove, { CLS(Table), CLS(Object) }, REF("01"));
	add_global("get", table_get1, { CLS(Table), CLS(Object) });
	add_global("get", table_get2, { CLS(Table), CLS(Object), CLS(Object) });

	// Regex
	auto regex_class = Class::get<Regex>();
	regex_class->add_initializer(regex_new1, {CLS(String)});
	regex_class->add_initializer(regex_new2, {CLS(String), CLS(String)});
	add_global("match", regex_match1, { CLS(Regex), CLS(String) });
	add_global("match", regex_match2, { CLS(Regex), CLS(String), CLS(intptr_t) });
	add_global("has_match", regex_has_match, { CLS(Regex) });
	add_global("count", regex_count, { CLS(Regex) });
	add_global("group", regex_group, { CLS(Regex), CLS(intptr_t) });
	add_global("get_start", regex_get_start, {CLS(Regex), CLS(intptr_t)});
	add_global("get_end", regex_get_end, {CLS(Regex), CLS(intptr_t)});

	// Set
	add_global("contains", set_contains, { CLS(Set), CLS(Object) });
	add_global("insert", set_insert, { CLS(Set), CLS(Object) }, REF("01"));
	add_global("remove", set_remove, { CLS(Set), CLS(Object) }, REF("01"));
	add_global("is_empty", set_is_empty, { CLS(Set) });
	add_global("clear", set_clear, { CLS(Set) }, REF("1"));
}

} // namespace calao

#undef CLS
#undef REF
