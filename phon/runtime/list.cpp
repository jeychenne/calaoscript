/**********************************************************************************************************************
 *                                                                                                                    *
 * Copyright (C) 2019-2020 Julien Eychenne <jeychenne@gmail.com>                                                      *
 *                                                                                                                    *
 * The contents of this file are subject to the Mozilla Public License Version 2.0 (the "License"); you may not use   *
 * this file except in compliance with the License. You may obtain a copy of the License at                           *
 * http://www.mozilla.org/MPL/.                                                                                       *
 *                                                                                                                    *
 * Created: 01/06/2020                                                                                                *
 *                                                                                                                    *
 * Purpose: see header.                                                                                               *
 *                                                                                                                    *
 **********************************************************************************************************************/

#include <phon/runtime/list.hpp>

namespace phonometrica {

List::List(const List &other) : _items(other._items)
{
	// When we clone a list, we need to make sure that aliases are resolved, otherwise both lists may get mutated if
	// we mutate a reference in one of them.
	for (auto &item : _items) {
		item.unalias();
	}
}

void List::traverse(const GCCallback &callback)
{
	for (auto &item : _items) {
		item.traverse(callback);
	}
}

String List::to_string() const
{
	if (this->seen)
	{
		return "[...]";
	}

	bool flag = this->seen;
	String s("[");
	for (intptr_t i = 1; i <= _items.size(); i++)
	{
		s.append(_items[i].to_string(true));
		if (i < _items.size()) {
			s.append(", ");
		}
	}
	s.append(']');
	this->seen = flag;

	return s;
}

} // namespace phonometrica