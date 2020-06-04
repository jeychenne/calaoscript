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
 * Purpose: manages classes known at compile time.                                                                    *
 *                                                                                                                    *
 **********************************************************************************************************************/

#ifndef CALAO_CLASS_DESCRIPTOR_HPP
#define CALAO_CLASS_DESCRIPTOR_HPP

#include <type_traits>
#include <calao/definitions.hpp>

namespace calao {
class Class;
}

namespace calao::detail {


// A template to keep track of classes known at compile time. This should not be accessed directly: use
// Class::get<T>() instead.
template<typename T>
struct ClassDescriptor
{
	static Class *get()
	{
		// Class is null while we are bootstrapping the class system. The runtime will check that we have a valid pointer for Class.
		assert(isa || (std::is_same_v<T, Class>));
		return isa;
	}

	static void set(Class *cls)
	{
		assert(isa == nullptr);
		isa = cls;
	}

private:

	static Class *isa;
};

template<class T>
Class *ClassDescriptor<T>::isa = nullptr;

} // namespace calao::detail

#endif // CALAO_CLASS_DESCRIPTOR_HPP
