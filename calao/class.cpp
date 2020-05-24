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


Class::Class(String name, Class *parent, const std::type_info *info) :
	Atomic(get<Class>()), _name(std::move(name)), _info(info), _bases(parent ? parent->_bases : std::vector<Class*>())
{
	_depth = _bases.size();
	_bases.push_back(this);
}

bool Class::inherits(const Class *base) const
{
	return base->depth() <= this->depth() && _bases[base->depth()] == base;
}
} // namespace calao
