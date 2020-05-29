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

#include <calao/function.hpp>
#include <calao/runtime.hpp>

namespace calao {

Callable::Callable(std::vector<Handle<Class>> sig, ParamBitset ref_flags) :
	signature(std::move(sig)), ref_flags(ref_flags)
{

}

NativeRoutine::NativeRoutine(NativeCallback cb, std::vector<Handle<Class>> sig, ParamBitset ref_flags) :
	Callable(std::move(sig), ref_flags), callback(std::move(cb))
{

}

//----------------------------------------------------------------------------------------------------------------------



Routine::Routine() : Callable()
{

}

Routine::Routine(std::vector<Handle<Class>> sig, ParamBitset ref_flags) :
		Callable(std::move(sig), ref_flags)
{

}

Instruction Routine::add_integer_constant(intptr_t i)
{
	return add_constant(integer_pool, i);
}

Instruction Routine::add_float_constant(double n)
{
	return add_constant(float_pool, n);
}

Instruction Routine::add_string_constant(String s)
{
	return add_constant(string_pool, std::move(s));
}

Instruction Routine::add_function(Handle<Function> f)
{
	return add_constant(function_pool, std::move(f));
}

Instruction Routine::add_local(const String &name, int scope, int depth)
{
	for (auto it = locals.rbegin(); it != locals.rend(); it++)
	{
		if (it->scope != scope) {
			break;
		}
		if (it->name == name) {
			throw error("[Name error] Variable \"%\" is already defined in the current scope", name);
		}
	}
	locals.push_back({name, scope, depth});

	return Instruction(locals.size() - 1);
}

std::optional<Instruction> Routine::find_local(const String &name, int scope) const
{
	for (size_t i = locals.size(); i-- > 0; )
	{
		auto &local = locals[i];
		if (local.scope == scope && local.name == name) {
			return Instruction(i);
		}
	}
	return std::optional<Instruction>();
}

int Routine::local_count() const
{
	return int(locals.size());
}

Instruction Routine::add_routine(std::shared_ptr<Routine> r)
{
	return add_constant(routine_pool, std::move(r));
}


//----------------------------------------------------------------------------------------------------------------------

Function::Function(String name, std::shared_ptr<Callable> r) :
	Function(std::move(name))
{
	add_routine(std::move(r));
}

void Function::add_routine(std::shared_ptr<Callable> r)
{
	routines.push_back(std::move(r));
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace calao