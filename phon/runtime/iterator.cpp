/**********************************************************************************************************************
 *                                                                                                                    *
 * Copyright (C) 2019-2020 Julien Eychenne <jeychenne@gmail.com>                                                      *
 *                                                                                                                    *
 * The contents of this file are subject to the Mozilla Public License Version 2.0 (the "License"); you may not use   *
 * this file except in compliance with the License. You may obtain a copy of the License at                           *
 * http://www.mozilla.org/MPL/.                                                                                       *
 *                                                                                                                    *
 * Created: 02/06/2020                                                                                                *
 *                                                                                                                    *
 * Purpose: see header.                                                                                               *
 *                                                                                                                    *
 **********************************************************************************************************************/

#include <phon/runtime/iterator.hpp>
#include <phon/regex.hpp>
#include <phon/file.hpp>

namespace phonometrica {

Variant Iterator::get_value()
{
	throw error("[Type error] Type % only supports iteration over keys", object.class_name());
}


//---------------------------------------------------------------------------------------------------------------------

ListIterator::ListIterator(Variant v, bool ref_val) : Iterator(std::move(v), ref_val), pos(1)
{
	lst = &raw_cast<List>(object.resolve()).items();
}

Variant ListIterator::get_key()
{
	return pos;
}

Variant ListIterator::get_value()
{
	auto &value = (*lst)[pos++];
	if (ref_val) value.make_alias();

	return value;
}

bool ListIterator::at_end() const
{
	return pos > lst->size();
}


//---------------------------------------------------------------------------------------------------------------------

TableIterator::TableIterator(Variant v, bool ref_val) :
	Iterator(v, ref_val), it(raw_cast<Table>(v.resolve()).map().begin())
{
	map = &raw_cast<Table>(object.resolve()).map();
}

Variant TableIterator::get_key()
{
	return it->first;
}

Variant TableIterator::get_value()
{
	auto &value = (it++)->second;
	if (ref_val) value.make_alias();

	return value;
}

bool TableIterator::at_end() const
{
	return it == raw_cast<Table>(object).map().end();
}


//---------------------------------------------------------------------------------------------------------------------

StringIterator::StringIterator(Variant v, bool ref_val) : Iterator(std::move(v), ref_val)
{
	str = &raw_cast<String>(object.resolve());
}

Variant StringIterator::get_key()
{
	return pos;
}

Variant StringIterator::get_value()
{
	if (ref_val) {
		throw error("[Reference error] Cannot take a reference to a character in a string.\nHint: take the second loop variable by value, not by reference");
	}
	return str->next_grapheme(pos++);
}

bool StringIterator::at_end() const
{
	return pos > str->grapheme_count();
}


//---------------------------------------------------------------------------------------------------------------------

RegexIterator::RegexIterator(Variant v, bool ref_val) : Iterator(std::move(v), ref_val)
{
	re = &raw_cast<Regex>(object.resolve());
}

Variant RegexIterator::get_key()
{
	return pos;
}

Variant RegexIterator::get_value()
{
	if (ref_val) {
		throw error("[Reference error] Cannot take a reference to a group in a regular expression.\nHint: take the second loop variable by value, not by reference");
	}
	return re->capture(pos++);
}

bool RegexIterator::at_end() const
{
	return pos > re->count();
}


//---------------------------------------------------------------------------------------------------------------------

FileIterator::FileIterator(Variant v, bool ref_val) : Iterator(std::move(v), ref_val)
{
	file = &raw_cast<File>(object.resolve());

	if (!file->readable()) {
		throw error("[Iterator error] Cannot iterate File object: the file is not readable");
	}
}

Variant FileIterator::get_key()
{
	return pos++;
}

Variant FileIterator::get_value()
{
	if (ref_val) {
		throw error("[Reference error] Cannot take a reference to a line in a file.\nHint: take the second loop variable by value, not by reference");
	}

	return file->read_line();
}

bool FileIterator::at_end() const
{
	return file->at_end();
}
} // namespace phonometrica