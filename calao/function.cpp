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

Callable::Callable(const String &name, std::vector<Handle<Class>> sig, ParamBitset ref_flags) :
	signature(std::move(sig)), ref_flags(ref_flags), _name(name)
{
	_argc = int(signature.size());
}

int Callable::get_cost(std::span<Variant> args) const
{
	int cost = 0;

	for (int i = 0; i < args.size(); i++)
	{
		auto &v = args[i];
		// Null variables are compatible with any type
		if (v.is_null()) continue;

		auto derived = v.get_class();
		auto base = signature[i].get();
		int dist = derived->get_distance(base);
		// If the argument doesn't match, this can't be a candidate.
		if (dist < 0) return (std::numeric_limits<int>::max)();
		cost += dist;
	}

	return cost;
}

String Callable::get_definition() const
{
	String def = name();
	Array<String> types;

	for (size_t i = 0; i < signature.size(); i++)
	{
		auto &cls = signature[i];
		if (ref_flags[i]) {
			types.append(String("ref ").append(cls->name()));
		}
		else {
			types.append(cls->name());
		}
	}
	def.append('(');
	def.append(String::join(types, ", "));
	def.append(')');

	return def;
}

//----------------------------------------------------------------------------------------------------------------------

NativeRoutine::NativeRoutine(const String &name, NativeCallback cb, std::initializer_list<Handle<Class>> sig, ParamBitset ref_flags) :
	Callable(name, std::move(sig), ref_flags), callback(std::move(cb))
{

}

Variant NativeRoutine::call(ArgumentList &args)
{
	return callback(args);
}


//----------------------------------------------------------------------------------------------------------------------

Routine::Routine(const String &name, int argc) : Callable(name, argc)
{

}

Routine::Routine(const String &name, std::vector<Handle<Class>> sig, ParamBitset ref_flags) :
		Callable(name, std::move(sig), ref_flags)
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

std::optional<Instruction> Routine::find_local(const String &name) const
{
	for (size_t i = locals.size(); i-- > 0; )
	{
		auto &local = locals[i];
		if (local.name == name) {
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

Variant Routine::call(ArgumentList &args)
{
	return args.runtime().interpret(*this, args);
}

//----------------------------------------------------------------------------------------------------------------------

Function::Function(String name, std::shared_ptr<Callable> r) :
	Function(std::move(name))
{
	add_routine(std::move(r), true);
}

void Function::add_routine(std::shared_ptr<Callable> r, bool create)
{
	// For user-defined functions, we don't have
	if (std::find(routines.begin(), routines.end(), r) == routines.end())
	{
		// Check for *reference consistency*; that is, ensure that positional argument n is consistently passed by value
		// or by reference for all routines in this function.
		int argc = (std::min)(max_argc, r->arg_count());
		auto rflags = r->ref_flags;
		for (int i = 0; i < argc; i++)
		{
			if (rflags[i] != ref_flags[i]) {
				throw error("[Reference error] This overload of function \"%\" is not consistent with previous definitions: parameter % must be passed by %",
						name(), i+1, ref_flags[i] ? "reference" : "value");
			}
		}
		for (auto &cand : routines)
		{
			// FIXME: this doesn't work when a function with the same signature is redefined.
			if (!create && r->signature == cand->signature) {
				throw error("[Type error] Function % is already defined in this scope", r->get_definition());
			}
		}

		// If the current routine accepts more arguments than all currently known routines, update the reference flag. Incoming routines
		// will have to be consistent with this routine and the others.
		if (r->arg_count() > max_argc)
		{
			this->ref_flags = rflags;
			max_argc = r->arg_count();
		}
		routines.push_back(std::move(r));
	}
}

std::shared_ptr<Callable> Function::find_routine(std::span<Variant> args)
{
	// We use the simplest implementation possible. It finds the routine with the cheapest cost, where cost is defined
	// as the sum of the distances between each argument's type and the expected parameter type.
	int best_cost = (std::numeric_limits<int>::max)() - 1; // We use INT_MAX for signatures that don't match
	std::shared_ptr<Callable> candidate;
	bool conflict = false;
	assert(!routines.empty());

	for (auto &r : routines)
	{
		if (!r->check_arg_count(args.size())) continue;
		int cost = r->get_cost(args);

		if (cost <= best_cost)
		{
			conflict = (cost == best_cost);
			best_cost = cost;
			candidate = r;
		}
	}

	if (conflict)
	{
		Array<String> types;
		for (auto &arg : args) {
			types.append(arg.class_name());
		}

		Array<String> signatures;

		for (auto &r : routines)
		{
			if (r->get_cost(args) == best_cost) {
				signatures.append(r->get_definition());
			}
		}
		throw error("[Runtime error] Cannot resolve call to function '%' with the following argument types: (%).\nCandidates are:\n%",
				name(), String::join(types, ", "), String::join(signatures, "\n"));
	}

	return candidate;
}

Function::Function(const String &name, NativeCallback cb, std::initializer_list<Handle<Class>> sig, ParamBitset ref_flags) :
	Function(name)
{
	add_routine(std::make_shared<NativeRoutine>(name, std::move(cb), sig, ref_flags), true);
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace calao