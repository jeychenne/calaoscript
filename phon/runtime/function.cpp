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

#include <phon/runtime/function.hpp>
#include <phon/runtime/runtime.hpp>

namespace phonometrica {

Callable::Callable(const String &name, std::vector<Handle<Class>> sig, ParamBitset ref_flags) :
	signature(std::move(sig)), ref_flags(ref_flags), _name(name)
{

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

Variant NativeRoutine::operator()(Runtime &rt, std::span<Variant> args)
{
	return callback(rt, args);
}


//----------------------------------------------------------------------------------------------------------------------

Routine::Routine(Routine *parent, const String &name) : Callable(name), parent(parent)
{

}

Routine::Routine(Routine *parent, const String &name, std::vector<Handle<Class>> sig, ParamBitset ref_flags) :
		Callable(name, std::move(sig), ref_flags), parent(parent), is_sealed(true)
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

Instruction Routine::add_local(const String &name, int scope, int depth)
{
	for (auto it = locals.rbegin(); it != locals.rend(); it++)
	{
		if (it->scope != scope) {
			break;
		}
		if (it->name == name) {
			throw error("[Name error] Variable \"%\" is already defined in this scope", name);
		}
	}
	locals.push_back({name, scope, depth});

	return Instruction(locals.size() - 1);
}

std::optional<Instruction> Routine::find_local(const String &name, int scope_depth) const
{
	for (size_t i = locals.size(); i-- > 0; )
	{
		auto &local = locals[i];
		if (local.depth <= scope_depth  && local.name == name) {
			return Instruction(i);
		}
	}
	return std::optional<Instruction>();
}

std::optional<Instruction> Routine::resolve_upvalue(const String &name, int scope_depth)
{
	if (parent)
	{
		auto index = parent->find_local(name, scope_depth);
		if (index) {
			return add_upvalue(*index, true);
		}

		index = parent->resolve_upvalue(name, scope_depth);
		if (index) {
			return add_upvalue(*index, false);
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

Instruction Routine::add_upvalue(Instruction index, bool local)
{
	if (unlikely(upvalues.size()) > (std::numeric_limits<Instruction>::max)()) {
		throw error("[Compiler error] Maximum number of upvalues exceeded in the current function");
	}
	auto it = std::find(upvalues.begin(), upvalues.end(), Upvalue{index, local});
	if (it != upvalues.end()) {
		return std::distance(upvalues.begin(), it);
	}
	upvalues.push_back({index, local});

	return Instruction(upvalues.size() - 1);
}

//----------------------------------------------------------------------------------------------------------------------

Function::Function(String name, Handle<Closure> c) :
	Function(std::move(name))
{
	add_closure(std::move(c));
}

void Function::add_closure(Handle<Closure> c)
{
	if (std::find(closures.begin(), closures.end(), c) == closures.end())
	{
		auto r = c->routine.get();
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
		for (auto &cand : closures)
		{
			if (r->signature == cand->routine->signature) {
				throw error("[Name error] Function % is already defined", r->get_definition());
			}
		}

		// If the current routine accepts more arguments than all currently known routines, update the reference flag. Incoming routines
		// will have to be consistent with this routine and the others.
		if (r->arg_count() > max_argc)
		{
			this->ref_flags = rflags;
			max_argc = r->arg_count();
		}

		// Sort routines by number of parameters
		argc = c->routine->arg_count();
		for (auto it = closures.begin(); it != closures.end(); it++)
		{
			if (argc < (*it)->routine->arg_count())
			{
				closures.insert(it, std::move(c));
				return;
			}
		}
		closures.push_back(std::move(c));
	}
}

Handle<Closure> Function::find_closure(std::span<Variant> args)
{
	// We use the simplest implementation possible. It finds the routine with the cheapest cost, where cost is defined
	// as the sum of the distances between each argument's type and the expected parameter type.
	int best_cost = (std::numeric_limits<int>::max)() - 1; // We use INT_MAX for signatures that don't match
	Handle<Closure> candidate;
	bool conflict = false;
	assert(!closures.empty());

	for (auto &c : closures)
	{
		auto r = c->routine.get();
		if (r->arg_count() < args.size()) {
			continue;
		}
		else if (r->arg_count() > args.size()) {
			break; // routines are sorted by their number of arguments, so we won't find a better match at this point.
		}
		int cost = r->get_cost(args);

		if (cost == 0) {
			return c; // there can only be 1 exact match, so we're done.
		}
		if (cost <= best_cost)
		{
			conflict = (cost == best_cost);
			best_cost = cost;
			candidate = c;
		}
	}

	if (conflict)
	{
		Array<String> types;
		for (auto &arg : args) {
			types.append(arg.class_name());
		}

		Array<String> signatures;

		for (auto &c : closures)
		{
			auto r = c->routine.get();
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
	auto r = std::make_shared<NativeRoutine>(name, std::move(cb), sig, ref_flags);

	add_closure(make_handle<Closure>(std::move(r)));
}

//----------------------------------------------------------------------------------------------------------------------
Variant Closure::operator()(Runtime &rt, std::span<Variant> args)
{
	if (routine->is_native()) {
		return call_native(rt, args);
	}
	else {
		return call_user(rt, args);
	}
}

Variant Closure::call_native(Runtime &rt, std::span<Variant> args)
{
	return (*reinterpret_cast<NativeRoutine*>(routine.get()))(rt, args);
}

Variant Closure::call_user(Runtime &rt, std::span<Variant> args)
{
	return rt.interpret(*this, args);
}
} // namespace phonometrica