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
 * Purpose: Table type (dynamic hash table).                                                                          *
 *                                                                                                                    *
 **********************************************************************************************************************/

#ifndef PHONOMETRICA_TABLE_HPP
#define PHONOMETRICA_TABLE_HPP

#include <phon/hashmap.hpp>
#include <phon/runtime/variant.hpp>

namespace phonometrica {


class Table final
{
public:

	using Storage = Hashmap<Variant, Variant>;
	using iterator = Storage::iterator;
	using const_iterator = Storage::const_iterator;

	Table() = default;

	Table(Storage dat) : _map(std::move(dat)) { }

	Table(Table &&) = default;

	Table(const Table &other);

	bool operator==(const Table &other) const;

	Storage &data() { return _map; }

	intptr_t size() const { return intptr_t(_map.size()); }

	Variant &get(const Variant &key);

	String to_string() const;

	String to_json() const;

	void traverse(const GCCallback &callback);

	Array<Variant> keys() const
	{
		Array<Variant> result;
		result.reserve(_map.size());

		for (auto &it : _map) {
			result.append(it.first);
		}

		return result;
	}

	Array<Variant> values() const
	{
		Array<Variant> result;
		result.reserve(_map.size());

		for (auto &it : _map) {
			result.append(it.second);
		}

		return result;
	}

	Storage &map() { return _map; }

	const Storage &map() const { return _map; }

private:

	Storage _map;
	mutable bool seen = false;
};


//---------------------------------------------------------------------------------------------------------------------

namespace meta {

static inline void traverse(Table &tab, const GCCallback &callback)
{
	tab.traverse(callback);
}


static inline String to_string(const Table &tab)
{
	return tab.to_string();
}
} // namespace phonometrica::meta
} // namespace phonometrica

#endif // PHONOMETRICA_TABLE_HPP
