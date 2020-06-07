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

#include <phon/runtime/object.hpp>
#include <phon/runtime/recycler.hpp>

namespace phonometrica {


void Recycler::add_candidate(Collectable *obj)
{
	auto old_root = this->root;
	obj->next = old_root;
	if (old_root) old_root->previous = obj;
	this->root = obj;
}

void Recycler::remove_candidate(Collectable *obj)
{
	if (obj == this->root)
	{
		this->root = obj->next;
	}
	if (obj->previous)
	{
		obj->previous->next = obj->next;
	}
	if (obj->next)
	{
		obj->next->previous = obj->previous;
	}
}

} // namespace phonometrica
