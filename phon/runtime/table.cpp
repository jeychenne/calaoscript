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

#include <phon/runtime/table.hpp>

namespace phonometrica {

Variant &Table::get(const Variant &key)
{
	auto it = _map.find(key);

	if (it == _map.end()) {
		throw error("[Index error] Missing key in table: %", key.to_string(true));
	}

	return it->second;
}

Table::Table(const Table &other)
{
	_map.reserve(other.size());

	for (auto &pair : other._map) {
		_map.insert({ pair.first.resolve(), pair.second.resolve() });
	}
}

String Table::to_string() const
{
	if (this->seen)
	{
		return "{...}";
	}

	bool flag = this->seen;
	String s("{");

	for (auto &pair : _map)
	{
		s.append(pair.first.to_string(true));
		s.append(": ");
		s.append(pair.second.to_string(true));
		s.append(", ");
	}
	s.remove_last(", ");
	s.append('}');
	this->seen = flag;

	return s;
}

void Table::traverse(const GCCallback &callback)
{
	for (auto &pair : _map)
	{
#ifdef PHON_STD_UNORDERED_MAP
		const_cast<Variant&>(pair.first).traverse(callback);
#else
		pair.first.traverse(callback);
#endif
		pair.second.traverse(callback);
	}
}

String Table::to_json() const
{
	if (this->seen)
	{
		throw error("[JSON error] Cannot convert recursive table to JSON");
	}

	bool flag = this->seen;
	String s("{");
	auto keys = this->keys();
	// Sort the keys so that we have a predictable order.
	std::sort(keys.begin(), keys.end());

	for (intptr_t i = 1; i <= keys.size(); i++)
	{
		auto &key = keys[i];
		s.append(key.to_string(true));
		s.append(": ");
		auto it = _map.find(key);
		s.append(it->second.to_string(true));
		if (i != keys.size()) {
			s.append(", ");
		}
	}
	s.append('}');
	this->seen = flag;

	return s;
}

} // namespace phonometrica