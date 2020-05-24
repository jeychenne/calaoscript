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

#include <cfenv>
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

void Runtime::push_int(intptr_t n)
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
	auto limit = top - n;

	if (unlikely(limit < stack.begin()))
	{
		// Clean up stack
		while (top > stack.begin())
		{
			(--top)->~Variant();
		}
		throw error("[Internal error] Stack underflow");
	}

	while (top > limit)
	{
		(--top)->~Variant();
	}
}

Variant & Runtime::get_top(int n)
{
	return *(top + n);
}

void Runtime::negate()
{
	auto &var = get_top();

	if (var.is_integer())
	{
		intptr_t value = - unsafe_cast<intptr_t>(var);
		pop();
		push_int(value);
	}
	else if (var.is_float())
	{
		double value = - unsafe_cast<double>(var);
		pop();
		push(value);
	}
	else
	{
		throw error("[Type error] Negation operator expected a Number, got a %", var.class_name());
	}
}

void Runtime::math_op(char op)
{
	auto &v1 = get_top(-2);
	auto &v2 = get_top(-1);
	std::feclearexcept(FE_ALL_EXCEPT);

	if (v1.is_number() && v2.is_number())
	{
		switch (op)
		{
			case '+':
			{
				if (v1.is_integer() && v2.is_integer())
				{
					auto x = cast<intptr_t>(v1);
					auto y = cast<intptr_t>(v2);
					pop(2);
					auto result = x + y;
					check_math_error();
					push_int(result);
				}
				else
				{
					auto x = v1.get_number();
					auto y = v2.get_number();
					pop(2);
					auto result = x + y;
					check_math_error();
					push(result);
				}
				return;
			}
			case '-':
			{
				if (v1.is_integer() && v2.is_integer())
				{
					auto x = cast<intptr_t>(v1);
					auto y = cast<intptr_t>(v2);
					pop(2);
					auto result = x - y;
					check_math_error();
					push_int(result);
				}
				else
				{
					auto x = v1.get_number();
					auto y = v2.get_number();
					pop(2);
					auto result = x - y;
					check_math_error();
					push(result);
				}
				return;
			}
			case '*':
			{
				if (v1.is_integer() && v2.is_integer())
				{
					auto x = cast<intptr_t>(v1);
					auto y = cast<intptr_t>(v2);
					pop(2);
					auto result = x * y;
					check_math_error();
					push_int(result);
				}
				else
				{
					auto x = v1.get_number();
					auto y = v2.get_number();
					pop(2);
					auto result = x * y;
					check_math_error();
					push(result);
				}
				return;
			}
			case '/':
			{
				auto x = v1.get_number();
				auto y = v2.get_number();
				pop(2);
				auto result = x / y;
				check_math_error();
				push(result);
				return;
			}
			default:
				break;
		}
	}

	pop(2);
	throw error("[Type error] Cannot apply math operator to values which are not numbers");
}

void Runtime::check_math_error()
{
	if (fetestexcept(FE_OVERFLOW | FE_UNDERFLOW | FE_DIVBYZERO | FE_INVALID))
	{
		if (fetestexcept(FE_OVERFLOW)) {
			throw error("[Math error] Number overflow");
		}
		if (fetestexcept(FE_UNDERFLOW)) {
			throw error("[Math error] Number underflow");
		}
		if (fetestexcept(FE_DIVBYZERO)) {
			throw error("[Math error] Division by zero");
		}
		if (fetestexcept(FE_INVALID)) {
			throw error("[Math error] Undefined number");
		}
	}
}

void Runtime::interpret(const Code &code)
{
	ip = code.data();

	while (true)
	{
		auto op = static_cast<Opcode>(*ip++);

		switch (op)
		{
			case Opcode::Add:
			{
				math_op('+');
				break;
			}
			case Opcode::Divide:
			{
				math_op('/');
				break;
			}
			case Opcode::Multiply:
			{
				math_op('*');
				break;
			}
			case Opcode::Negate:
			{
				negate();
				break;
			}
			case Opcode::PushBoolean:
			{
				bool value = bool(*ip++);
				push(value);
				break;
			}
			case Opcode::PushFloat:
			{
				double value = code.get_float(*ip++);
				push(value);
				break;
			}
			case Opcode::PushInteger:
			{
				intptr_t value = code.get_integer(*ip++);
				push_int(value);
				break;
			}
			case Opcode::PushSmallInt:
			{
				push_int(*ip++);
				break;
			}
			case Opcode::PushString:
			{
				String value = code.get_string(*ip++);
				push(std::move(value));
				break;
			}
			case Opcode::Return:
				return;
			case Opcode::Subtract:
			{
				math_op('-');
				break;
			}
			default:
				throw error("[Internal error] Invalid opcode: %", (int)op);
		}
	}
}

void Runtime::disassemble(const Code &code, const String &name)
{
	printf("==================== %s ====================\n", name.data());
	printf("offset    line   instruction    operands   comments\n");
	size_t size = code.size();

	for (size_t offset = 0; offset < size; )
	{
		offset += disassemble_instruction(code, offset);
	}
}

size_t Runtime::disassemble_instruction(const Code &code, size_t offset)
{
	auto op = static_cast<Opcode>(code[offset]);
	printf("%6zu   %5d   ", offset, code.get_line(offset));

	switch (op)
	{
		case Opcode::Add:
		{
			printf("ADD\n");
			return 1;
		}
		case Opcode::Divide:
		{
			printf("DIVIDE\n");
			return 1;
		}
		case Opcode::Multiply:
		{
			printf("MULTIPLY\n");
			return 1;
		}
		case Opcode::Negate:
		{
			printf("NEGATE\n");
			return 1;
		}
		case Opcode::PushBoolean:
		{
			int value = code[offset+1];
			auto str = value ? "true" : "false";
			printf("PUSH_BOOLEAN   %-5d      ; %s\n", value, str);
			return 2;
		}
		case Opcode::PushFloat:
		{
			int index = code[offset+1];
			double value = code.get_float(index);
			printf("PUSH_FLOAT     %-5d      ; %f\n", index, value);
			return 2;
		}
		case Opcode::PushInteger:
		{
			int index = code[offset+1];
			intptr_t value = code.get_integer(index);
			printf("PUSH_INTEGER   %-5d      ; %" PRIdPTR "\n", index, value);
			return 2;
		}
		case Opcode::PushSmallInt:
		{
			int value = code[offset+1];
			printf("PUSH_SMALL_INT %-5d\n", value);
			return 2;
		}
		case Opcode::PushString:
		{
			int index = code[offset+1];
			String value = code.get_string(index);
			printf("PUSH_STRING    %-5d      ; \"%s\"\n", index, value.data());
			return 2;
		}
		case Opcode::Return:
		{
			printf("RETURN\n");
			return 1;
		}
		case Opcode::Subtract:
		{
			printf("SUBTRACT\n");
			return 1;
		}
		default:
			printf("Unknown opcode %d", static_cast<int>(op));
	}

	return 1;
}


} // namespace calao
