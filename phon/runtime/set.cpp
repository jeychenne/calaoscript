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
 * Purpose: see header.                                                                                               *
 *                                                                                                                    *
 **********************************************************************************************************************/

#include <phon/runtime/set.hpp>

namespace phonometrica {

Set::Set(const Set &other)
{
	for (auto &val : other._items) {
		_items.insert(val.resolve());
	}
}

String Set::to_string() const
{
	if (this->seen)
	{
		return "{...}";
	}

	bool flag = this->seen;
	String s("{");

	for (auto &val : _items)
	{
		s.append(val.to_string(true));
		s.append(", ");
	}
	s.remove_last(", ");
	s.append('}');
	this->seen = flag;

	return s;
}

void Set::traverse(const GCCallback &callback)
{
	for (auto &val : _items) {
		const_cast<Variant&>(val).traverse(callback);
	}
}

} // namespace phonometrica