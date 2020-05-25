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
		Token::initialize();
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

void Runtime::push_null()
{
	new(var()) Variant();
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

Variant & Runtime::peek(int n)
{
	return *(top + n);
}

void Runtime::negate()
{
	auto &var = peek();

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
	auto &v1 = peek(-2);
	auto &v2 = peek(-1);
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
					if ((x < 0.0) == (y < 0.0) && std::abs(y) > (std::numeric_limits<intptr_t>::max)() - std::abs(x)) {
						throw RuntimeError(get_current_line(), "[Math error] Integer overflow");
					}
					push_int(x+y);
				}
				else
				{
					auto x = v1.get_number();
					auto y = v2.get_number();
					pop(2);
					auto result = x + y;
					check_float_error();
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
					push_int(x - y);
				}
				else
				{
					auto x = v1.get_number();
					auto y = v2.get_number();
					pop(2);
					auto result = x - y;
					check_float_error();
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
					push_int(x * y);
				}
				else
				{
					auto x = v1.get_number();
					auto y = v2.get_number();
					pop(2);
					auto result = x * y;
					check_float_error();
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
				check_float_error();
				push(result);
				return;
			}
			case '^':
			{
				auto x = v1.get_number();
				auto y = v2.get_number();
				pop(2);
				auto result = pow(x, y);
				check_float_error();
				push(result);
				return;
			}
			case '%':
			{
				if (v1.is_integer() && v2.is_integer())
				{
					auto x = cast<intptr_t>(v1);
					auto y = cast<intptr_t>(v2);
					pop(2);
					push_int(x % y);
				}
				else
				{
					auto x = v1.get_number();
					auto y = v2.get_number();
					pop(2);
					push(std::fmod(x, y));
				}
				return;
			}
			default:
				break;
		}
	}

	pop(2);
	throw error("[Type error] Cannot apply math operator to values which are not numbers");
}

void Runtime::check_float_error()
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
	this->code = &code;
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
			case Opcode::Concat:
			{
				auto s = peek(-2).to_string();
				s.append(peek(-1).to_string());
				pop(2);
				push(std::move(s));
				break;
			}
			case Opcode::Divide:
			{
				math_op('/');
				break;
			}
			case Opcode::Equal:
			{
				auto &v2 = peek(-1);
				auto &v1 = peek(-2);
				bool value = (v1 == v2);
				pop(2);
				push(value);
				break;
			}
			case Opcode::Greater:
			{
				auto &v2 = peek(-1);
				auto &v1 = peek(-2);
				bool value = (v1.compare(v2) > 0);
				pop(2);
				push(value);
				break;
			}
			case Opcode::GreaterEqual:
			{
				auto &v2 = peek(-1);
				auto &v1 = peek(-2);
				bool value = (v1.compare(v2) >= 0);
				pop(2);
				push(value);
				break;
			}
			case Opcode::Less:
			{
				auto &v2 = peek(-1);
				auto &v1 = peek(-2);
				bool value = (v1.compare(v2) < 0);
				pop(2);
				push(value);
				break;
			}
			case Opcode::LessEqual:
			{
				auto &v2 = peek(-1);
				auto &v1 = peek(-2);
				bool value = (v1.compare(v2) <= 0);
				pop(2);
				push(value);
				break;
			}
			case Opcode::Modulus:
			{
				math_op('%');
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
			case Opcode::Not:
			{
				bool value = peek().to_boolean();
				pop();
				push(!value);
				break;
			}
			case Opcode::NotEqual:
			{
				auto &v2 = peek(-1);
				auto &v1 = peek(-2);
				bool value = (v1 != v2);
				pop(2);
				push(value);
				break;
			}
			case Opcode::Power:
			{
				math_op('^');
				break;
			}
			case Opcode::PushBoolean:
			{
				bool value = bool(*ip++);
				push(value);
				break;
			}
			case Opcode::PushFalse:
			{
				push(false);
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
			case Opcode::PushNan:
			{
				push(std::nan(""));
				break;
			}
			case Opcode::PushNull:
			{
				push_null();
				break;
			}
			case Opcode::PushSmallInt:
			{
				push_int((int16_t) *ip++);
				break;
			}
			case Opcode::PushString:
			{
				String value = code.get_string(*ip++);
				push(std::move(value));
				break;
			}
			case Opcode::PushTrue:
			{
				push(true);
				break;
			}
			case Opcode::Return:
			{
				this->code = nullptr;
				return;
			}
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

size_t Runtime::print_simple_instruction(const char *name)
{
	printf("%s\n", name);
	return 1;
}

size_t Runtime::disassemble_instruction(const Code &code, size_t offset)
{
	auto op = static_cast<Opcode>(code[offset]);
	printf("%6zu   %5d   ", offset, code.get_line(offset));

	switch (op)
	{
		case Opcode::Add:
		{
			return print_simple_instruction("ADD");
		}
		case Opcode::Concat:
		{
			return print_simple_instruction("CONCAT");
		}
		case Opcode::Divide:
		{
			return print_simple_instruction("DIVIDE");
		}
		case Opcode::Equal:
		{
			return print_simple_instruction("EQUAL");
		}
		case Opcode::Greater:
		{
			return print_simple_instruction("GREATER");
		}
		case Opcode::GreaterEqual:
		{
			return print_simple_instruction("GREATER_EQUAL");
		}
		case Opcode::Less:
		{
			return print_simple_instruction("LESS");
		}
		case Opcode::LessEqual:
		{
			return print_simple_instruction("LESS_EQUAL");
		}
		case Opcode::Modulus:
		{
			return print_simple_instruction("MODULUS");
		}
		case Opcode::Multiply:
		{
			return print_simple_instruction("MULTIPLY");
		}
		case Opcode::Negate:
		{
			return print_simple_instruction("NEGATE");
		}
		case Opcode::Not:
		{
			return print_simple_instruction("NOT");
		}
		case Opcode::NotEqual:
		{
			return print_simple_instruction("NOT_EQUAL");
		}
		case Opcode::Power:
		{
			return print_simple_instruction("POWER");
		}
		case Opcode::PushBoolean:
		{
			int value = code[offset+1];
			auto str = value ? "true" : "false";
			printf("PUSH_BOOLEAN   %-5d      ; %s\n", value, str);
			return 2;
		}
		case Opcode::PushFalse:
		{
			return print_simple_instruction("PUSH_FALSE");
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
		case Opcode::PushNan:
		{
			return print_simple_instruction("PUSH_NAN");
		}
		case Opcode::PushNull:
		{
			return print_simple_instruction("PUSH_NULL");
		}
		case Opcode::PushSmallInt:
		{
			int value = (int16_t) code[offset+1];
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
		case Opcode::PushTrue:
		{
			return print_simple_instruction("PUSH_TRUE");
		}
		case Opcode::Return:
		{
			return print_simple_instruction("RETURN");
		}
		case Opcode::Subtract:
		{
			return print_simple_instruction("SUBTRACT");
		}
		default:
			printf("Unknown opcode %d", static_cast<int>(op));
	}

	return 1;
}

void Runtime::do_file(const String &path)
{
	auto code = compiler.do_file(path);
	disassemble(*code, "test");
	interpret(*code);
	auto &v = peek();
	auto s = v.to_string(check_type<String>(v));
	printf("------------------------\nValue on the stack: %s\n", s.data());
}

int Runtime::get_current_line() const
{
	auto offset = int(ip - 1 - code->data());
	return code->get_line(offset);
}


} // namespace calao
