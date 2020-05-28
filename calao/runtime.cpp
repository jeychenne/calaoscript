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
#include <calao/utils/helpers.hpp>

namespace calao {

static const int STACK_SIZE = 1024;

bool Runtime::initialized = false;


Runtime::Runtime() :
		stack(STACK_SIZE, Variant()), parser(this)
{
	srand(time(nullptr));

	if (! initialized)
	{
		utils::init_random_seed();
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
	if (this->top == this->limit) {
		resize_stack();
	}
}

void Runtime::resize_stack()
{
	auto size = stack.size();
	auto new_size = size + STACK_SIZE;
	this->stack.resize(new_size);
	top = stack.begin() + size;
	limit = stack.end();
}

void Runtime::ensure_capacity(int n)
{
	if (top + n >= limit) {
		resize_stack();
	}
}


void Runtime::check_underflow()
{
	if (this->top == this->stack.begin())
	{
		throw RuntimeError(get_current_line(), "[Internal error] Stack underflow");
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
		throw RuntimeError(get_current_line(), "[Internal error] Stack underflow");
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
	char opstring[2] = { op, '\0' };
	throw RuntimeError(get_current_line(), "[Type error] Cannot apply math operator '%' to % and %", opstring, v1.class_name(), v2.class_name());
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

void Runtime::interpret(const Routine &routine)
{
	this->code = &routine.code;
	ip = routine.code.data();

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
			case Opcode::Assert:
			{
				int narg = *ip++;
				bool value = peek(-narg).to_boolean();
				if (!value)
				{
					auto msg = (narg == 2) ? utils::format("Assertion failed: %", peek(-1).to_string()) : std::string("Assertion failed");
					throw RuntimeError(get_current_line(), msg);
				}
				break;
			}
			case Opcode::Compare:
			{
				auto &v1 = peek(-2);
				auto &v2 = peek(-1);
				int result = v1.compare(v2);
				pop(2);
				push_int(result);
				break;
			}
			case Opcode::Concat:
			{
				int narg = *ip++;
				String s;
				for (int i = narg; i > 0; i--) {
					s.append(peek(-i).to_string());
				}
				pop(narg);
				push(std::move(s));
				break;
			}
			case Opcode::DecrementLocal:
			{
				int index = *ip++;
				auto &v = current_frame->locals[index];
				assert(v.is_integer());
				unsafe_cast<intptr_t>(v)--;
				break;
			}
			case Opcode::DefineGlobal:
			{
				auto name = routine.get_string(*ip++);
				if (globals.find(name) != globals.end()) {
					throw RuntimeError(get_current_line(), "Global variable \"%\" is already defined", name);
				}
				globals.insert({name, std::move(peek())});
				pop();
				break;
			}
			case Opcode::DefineLocal:
			{
				Variant &local = current_frame->locals[*ip++];
				local = std::move(peek());
				pop();
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
			case Opcode::GetGlobal:
			{
				auto name = routine.get_string(*ip++);
				auto it = globals.find(name);
				if (it == globals.end()) {
					throw RuntimeError(get_current_line(), "Undefined variable \"%\"", name);
				}
				push(it->second);
				break;
			}
			case Opcode::GetLocal:
			{
				const Variant &v = current_frame->locals[*ip++];
				push(v);
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
			case Opcode::IncrementLocal:
			{
				int index = *ip++;
				auto &v = current_frame->locals[index];
				assert(v.is_integer());
				unsafe_cast<intptr_t>(v)++;
				break;
			}
			case Opcode::Jump:
			{
				int addr = Code::read_integer(ip);
				ip = code->data() + addr;
				break;
			}
			case Opcode::JumpFalse:
			{
				int addr = Code::read_integer(ip);
				bool value = peek().to_boolean();
				pop();
				if (!value) ip = code->data() + addr;
				break;
			}
			case Opcode::JumpTrue:
			{
				int addr = Code::read_integer(ip);
				bool value = peek().to_boolean();
				pop();
				if (value) ip = code->data() + addr;
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
			case Opcode::NewFrame:
			{
				push_stack_frame(*ip++);
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
			case Opcode::Pop:
			{
				pop();
				break;
			}
			case Opcode::Power:
			{
				math_op('^');
				break;
			}
			case Opcode::Print:
			{
				auto s = peek().to_string();
				utils::printf(s);
				pop();
				break;
			}
			case Opcode::PrintLine:
			{
				auto s = peek().to_string();
				utils::printf(s);
				printf("\n");
				pop();
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
				double value = routine.get_float(*ip++);
				push(value);
				break;
			}
			case Opcode::PushInteger:
			{
				intptr_t value = routine.get_integer(*ip++);
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
				String value = routine.get_string(*ip++);
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
				pop_stack_frame();
				this->code = nullptr;
				return;
			}
			case Opcode::SetGlobal:
			{
				auto name = routine.get_string(*ip++);
				auto it = globals.find(name);
				if (it == globals.end()) {
					throw RuntimeError(get_current_line(), "Undefined variable \"%\"", name);
				}
				it->second = std::move(peek());
				pop();
				break;
			}
			case Opcode::SetLocal:
			{
				Variant &v = current_frame->locals[*ip++];
				v = std::move(peek());
				pop();
				break;
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

void Runtime::disassemble(const Routine &routine, const String &name)
{
	printf("==================== %s ====================\n", name.data());
	printf("offset    line   instruction    operands   comments\n");
	size_t size = routine.code.size();

	for (size_t offset = 0; offset < size; )
	{
		offset += disassemble_instruction(routine, offset);
	}
}

size_t Runtime::print_simple_instruction(const char *name)
{
	printf("%s\n", name);
	return 1;
}

size_t Runtime::disassemble_instruction(const Routine &routine, size_t offset)
{
	auto op = static_cast<Opcode>(routine.code[offset]);
	printf("%6zu   %5d   ", offset, routine.code.get_line(offset));

	switch (op)
	{
		case Opcode::Add:
		{
			return print_simple_instruction("ADD");
		}
		case Opcode::Assert:
		{
			int narg = routine.code[offset + 1];
			printf("ASSERT         %-5d\n", narg);
			return 2;
		}
		case Opcode::Compare:
		{
			return print_simple_instruction("COMPARE");
		}
		case Opcode::Concat:
		{
			int narg = routine.code[offset+1];
			printf("CONCAT         %-5d\n", narg);
			return 2;
		}
		case Opcode::DecrementLocal:
		{
			int index = routine.code[offset + 1];
			printf("DEC_LOCAL      %-5d\n", index);
			return 2;
		}
		case Opcode::DefineGlobal:
		{
			int index = routine.code[offset + 1];
			String value = routine.get_string(index);
			printf("DEFINE_GLOBAL  %-5d      ; %s\n", index, value.data());
			return 2;
		}
		case Opcode::DefineLocal:
		{
			int index = routine.code[offset + 1];
			String value = routine.get_local_name(index);
			printf("DEFINE_LOCAL   %-5d      ; %s\n", index, value.data());
			return 2;
		}
		case Opcode::Divide:
		{
			return print_simple_instruction("DIVIDE");
		}
		case Opcode::Equal:
		{
			return print_simple_instruction("EQUAL");
		}
		case Opcode::GetGlobal:
		{
			int index = routine.code[offset + 1];
			String value = routine.get_string(index);
			printf("GET_GLOBAL     %-5d      ; %s\n", index, value.data());
			return 2;
		}
		case Opcode::GetLocal:
		{
			int index = routine.code[offset + 1];
			String value = routine.get_local_name(index);
			printf("GET_LOCAL      %-5d      ; %s\n", index, value.data());
			return 2;
		}
		case Opcode::Greater:
		{
			return print_simple_instruction("GREATER");
		}
		case Opcode::GreaterEqual:
		{
			return print_simple_instruction("GREATER_EQUAL");
		}
		case Opcode::IncrementLocal:
		{
			int index = routine.code[offset + 1];
			printf("INC_LOCAL      %-5d\n", index);
			return 2;
		}
		case Opcode::Jump:
		{
			auto ptr = routine.code.data() + offset + 1;
			int addr = Code::read_integer(ptr);
			printf("JUMP           %-5d\n", addr);
			return 1 + Code::IntSerializer::IntSize;
		}
		case Opcode::JumpFalse:
		{
			auto ptr = routine.code.data() + offset + 1;
			int addr = Code::read_integer(ptr);
			printf("JUMP_FALSE     %-5d\n", addr);
			return 1 + Code::IntSerializer::IntSize;
		}
		case Opcode::JumpTrue:
		{
			auto ptr = routine.code.data() + offset + 1;
			int addr = Code::read_integer(ptr);
			printf("JUMP_TRUE      %-5d\n", addr);
			return 1 + Code::IntSerializer::IntSize;
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
		case Opcode::NewFrame:
		{
			int nlocal = routine.code[offset+1];
			printf("NEW_FRAME      %-5d\n", nlocal);
			return 2;
		}
		case Opcode::Not:
		{
			return print_simple_instruction("NOT");
		}
		case Opcode::NotEqual:
		{
			return print_simple_instruction("NOT_EQUAL");
		}
		case Opcode::Pop:
		{
			return print_simple_instruction("POP");
		}
		case Opcode::Power:
		{
			return print_simple_instruction("POWER");
		}
		case Opcode::Print:
		{
			return print_simple_instruction("PRINT");
		}
		case Opcode::PrintLine:
		{
			return print_simple_instruction("PRINT_LINE");
		}
		case Opcode::PushBoolean:
		{
			int value = routine.code[offset + 1];
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
			int index = routine.code[offset + 1];
			double value = routine.get_float(index);
			printf("PUSH_FLOAT     %-5d      ; %f\n", index, value);
			return 2;
		}
		case Opcode::PushInteger:
		{
			int index = routine.code[offset + 1];
			intptr_t value = routine.get_integer(index);
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
			int value = (int16_t) routine.code[offset + 1];
			printf("PUSH_SMALL_INT %-5d\n", value);
			return 2;
		}
		case Opcode::PushString:
		{
			int index = routine.code[offset + 1];
			String value = routine.get_string(index);
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
		case Opcode::SetGlobal:
		{
			int index = routine.code[offset + 1];
			String value = routine.get_string(index);
			printf("SET_GLOBAL     %-5d      ; %s\n", index, value.data());
			return 2;
		}
		case Opcode::SetLocal:
		{
			int index = routine.code[offset + 1];
			String value = routine.get_local_name(index);
			printf("SET_LOCAL      %-5d      ; %s\n", index, value.data());
			return 2;
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
	auto ast = parser.parse_file(path);
	auto routine = compiler.compile(std::move(ast));
	disassemble(*routine, "test");
	CallInfo call;
	routine->call(*this, call);
}

int Runtime::get_current_line() const
{
	auto offset = int(ip - 1 - code->data());
	return code->get_line(offset);
}

String Runtime::intern_string(const String &s)
{
	auto result = strings.insert(s);
	return *result.first;
}

void Runtime::push_stack_frame(int nlocal)
{
	current_frame = frame_pool.newElement();
	frames.push_back(current_frame);
	// Add 1 for the return value, which will sit at the beginning of the frame
	ensure_capacity(nlocal + 1);
	current_frame->base = top;
	current_frame->locals = top + 1;
	// Push null for the return value (we've already checked the capacity).
	new (top++) Variant;
	top += nlocal;
}

void Runtime::pop_stack_frame()
{
	// Discard everything except the return value.
	auto n = int(top - current_frame->locals);
	assert(n >= 0);
	pop(n);
	frame_pool.deleteElement(current_frame);
	frames.pop_back();
	current_frame = frames.empty() ? nullptr : frames.back();
}


} // namespace calao
