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

#include <cstdio>
#include <ctime>
#include <calao/runtime.hpp>
#include <calao/regex.hpp>
#include <calao/file.hpp>

namespace calao {

static const int STACK_SIZE = 1024;

size_t Runtime::hash_seed = 0;
bool Runtime::initialized = false;


Runtime::Runtime() :
	stack(STACK_SIZE, Variant())
{
	srand(time(nullptr));

	if (! initialized)
	{
		hash_seed = (size_t) rand();
		initialized = true;
	}

	create_builtins();
	this->top = this->stack.begin();
	this->limit = this->stack.end();
}

calao::Runtime::~Runtime()
{
	for (auto i = classes.size(); i-- > 0; )
	{
		delete classes[i];
	}
}

void calao::Runtime::add_candidate(Collectable *obj)
{
	gc.add_candidate(obj);
}

void calao::Runtime::remove_candidate(Collectable *obj)
{
	gc.remove_candidate(obj);
}

size_t Runtime::random_seed()
{
	return hash_seed;
}

void Runtime::create_builtins()
{
	// We need to boostrap the class system, since we are creating class instances but the class type doesn't exist
	// yet. We can't create it first because it inherits from Object.
	auto object_class = create_type<Object>("Object", nullptr);

	auto class_class = create_type<Class>("Class", object_class);
	assert(class_class->inherits(object_class));

	assert(object_class->get_class() == nullptr);
	assert(class_class->get_class() == nullptr);

	object_class->set_class(class_class);
	class_class->set_class(class_class);

	// Create other builtin types.
	create_type<bool>("Boolean", object_class);
	auto num_class = create_type<Number>("Number", object_class);
	create_type<intptr_t>("Integer", num_class);
	create_type<double>("Float", num_class);
	create_type<String>("String", object_class);
	create_type<Regex>("Regex", object_class);
	create_type<List>("List", object_class);
	create_type<Table>("Table", object_class);
	create_type<File>("File", object_class);
//	create_type<Module>("Module", object_class);

	// Sanity checks
	assert(object_class->get_class() != nullptr);
	assert(class_class->get_class() != nullptr);
	assert((Class::get<Class>()) != nullptr);
}

Variant &Runtime::push()
{
	return *(new(var()) Variant());
}

void Runtime::push(double n)
{
	new(var()) Variant(n);
}

void Runtime::push(bool b)
{
	new(var()) Variant(b);
}

void Runtime::push(const Variant &v)
{
	new(var()) Variant(v);
}

void Runtime::push(Variant &&v)
{
	new(var()) Variant(std::move(v));
}

void Runtime::push(String s)
{
	new(var()) Variant(std::move(s));
}

Variant *Runtime::var()
{
	check_capacity();
	return this->top++;
}

void Runtime::check_capacity()
{
	if (this->top == this->limit)
	{
		// FIXME: if we hold pointers to the stack, they should be updated.
		auto size = stack.size();
		auto new_size = size + STACK_SIZE;
		this->stack.resize(new_size);
		top = stack.begin() + size;
		limit = stack.end();
	}
}

void Runtime::check_underflow()
{
	if (this->top == this->stack.begin())
	{
		throw error("[Internal error] Stack underflow");
	}
}

void Runtime::pop(int n)
{
	while (n-- > 0)
	{
		check_underflow();
		--top;
	}
}

void Runtime::disassemble(const Code &code, const String &name)
{
	printf("== %s ==", name.data());
	size_t size = code.size();

	for (size_t offset = 0; offset < size; offset++)
	{
		disassemble_instruction(code, offset);
	}

}

void Runtime::disassemble_instruction(const Code &code, size_t offset)
{
	auto op = static_cast<Opcode>(code[offset]);
	printf("%04zu ", offset);

	switch (op)
	{
		case Opcode::Return:
		{
			printf("RETURN");
			break;
		}
		default:
			printf("Unknown opcode %d", static_cast<int>(op));
	}
}


} // namespace calao
