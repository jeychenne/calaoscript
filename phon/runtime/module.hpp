/**********************************************************************************************************************
 *                                                                                                                    *
 * Copyright (C) 2019-2020 Julien Eychenne <jeychenne@gmail.com>                                                      *
 *                                                                                                                    *
 * The contents of this file are subject to the Mozilla Public License Version 2.0 (the "License"); you may not use   *
 * this file except in compliance with the License. You may obtain a copy of the License at                           *
 * http://www.mozilla.org/MPL/.                                                                                       *
 *                                                                                                                    *
 * Created: 04/06/2020                                                                                                *
 *                                                                                                                    *
 * Purpose: a module provides a namespace for Phonometrica code.                                                      *
 *                                                                                                                    *
 **********************************************************************************************************************/

#ifndef PHONOMETRICA_MODULE_HPP
#define PHONOMETRICA_MODULE_HPP

#include <phon/runtime/variant.hpp>
#include <phon/dictionary.hpp>

namespace phonometrica {

class Module final
{
public:

	using Storage = Dictionary<Variant>;
	using iterator = Storage::iterator;
	using value_type = Storage::value_type;

	explicit Module(const String &name) : _name(name) { }

	Module(const Module &) = delete;


	String name() const { return _name; }

	Variant &operator[](const String &key) { return members[key]; }

	iterator find(const String &key) { return members.find(key); }

	iterator begin() { return members.begin(); }

	iterator end() { return members.end(); }

	void insert(value_type v) { members.insert(std::move(v)); }

	Variant &get(const String &key);

private:

	friend class Runtime;

	String _name;

	Storage members;
};

} // namespace phonometrica

#endif // PHONOMETRICA_MODULE_HPP
