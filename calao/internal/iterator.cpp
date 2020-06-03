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

#include <calao/internal/iterator.hpp>

namespace calao {

Variant Iterator::get_value()
{
	throw error("[Type error] Type % only supports iteration over keys", object.class_name());
}


//---------------------------------------------------------------------------------------------------------------------

ListIterator::ListIterator(Variant v, bool with_val, bool ref_val) : Iterator(std::move(v), with_val, ref_val), pos(1)
{
	lst = &raw_cast<List>(object.resolve()).items();
}

Variant ListIterator::get_key()
{
	return with_val ? pos : pos++;
}

Variant ListIterator::get_value()
{
	// We always increment pos without checking with_val because if with_val is false, this function will never be called.
	assert(with_val);
	auto &value = (*lst)[pos++];
	if (ref_val) value.make_alias();

	return value;
}

bool ListIterator::at_end() const
{
	return pos > lst->size();
}


//---------------------------------------------------------------------------------------------------------------------

TableIterator::TableIterator(Variant v, bool with_val, bool ref_val) :
	Iterator(v, with_val, ref_val), it(raw_cast<Table>(v.resolve()).map().begin())
{
	map = &raw_cast<Table>(object.resolve()).map();
}

Variant TableIterator::get_key()
{
	return with_val ? it->first : (it++)->first;
}

Variant TableIterator::get_value()
{
	assert(with_val);
	auto &value = (it++)->second;
	if (ref_val) value.make_alias();

	return value;
}

bool TableIterator::at_end() const
{
	return it == raw_cast<Table>(object).map().end();
}


//---------------------------------------------------------------------------------------------------------------------

StringIterator::StringIterator(Variant v, bool with_val, bool ref_val) : Iterator(std::move(v), with_val, ref_val)
{
	str = &raw_cast<String>(object.resolve());
}

Variant StringIterator::get_key()
{
	return with_val ? pos : pos++;
}

Variant StringIterator::get_value()
{
	assert(with_val);
	if (ref_val) {
		throw error("[Reference error] Cannot take a reference to a character in a string.\nHint: take the value by value, not by reference");
	}
	return str->next_grapheme(pos++);
}

bool StringIterator::at_end() const
{
	return pos > str->grapheme_count();
}
} // namespace calao