/**********************************************************************************************************************
 *                                                                                                                    *
 * Copyright (C) 2019-2020 Julien Eychenne <jeychenne@gmail.com>                                                      *
 *                                                                                                                    *
 * The contents of this file are subject to the Mozilla Public License Version 2.0 (the "License"); you may not use   *
 * this file except in compliance with the License. You may obtain a copy of the License at                           *
 * http://www.mozilla.org/MPL/.                                                                                       *
 *                                                                                                                    *
 * Created: 22/05/2020                                                                                                *
 *                                                                                                                    *
 * Purpose: see header.                                                                                               *
 *                                                                                                                    *
 **********************************************************************************************************************/

#include <calao/class.hpp>

namespace calao {


Class::Class(String name, Class *parent, const std::type_info *info, Index index) :
	_name(std::move(name)), _info(info), _bases(parent ? parent->_bases : std::vector<Class*>()), index(index)
{
	_depth = _bases.size();
	_bases.push_back(this);
}

bool Class::inherits(const Class *base) const
{
	return _bases[base->depth()] == base && base->depth() <= this->depth();
}

int Class::get_distance(const Class *base) const
{
	return _bases[base->depth()] == base ? int(this->depth() - base->depth()) : -1;
}

Handle<Function> Class::get_constructor()
{
	if (!ctor) {
		throw error("[Type errror] Class % is not constructible", name());
	}

	return ctor;
}

void Class::add_initializer(NativeCallback cb, std::initializer_list<Handle<Class>> sig, ParamBitset ref)
{
	static String init("init");
	if (ctor) {
		ctor->add_closure(make_handle<Closure>(std::make_shared<NativeRoutine>(init, std::move(cb), sig, ref)), false);
	}
	else {
		add_initializer(make_handle<Function>(init, std::move(cb), sig, ref));
	}
}


} // namespace calao
