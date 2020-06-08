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
#include <iomanip>
#include <phon/runtime/runtime.hpp>
#include <phon/regex.hpp>
#include <phon/file.hpp>
#include <phon/utils/helpers.hpp>

#define CATCH_ERROR catch (std::runtime_error &e) { RUNTIME_ERROR(e.what()); }
#define RUNTIME_ERROR(...) throw RuntimeError(get_current_line(), __VA_ARGS__)

#if 0
#	define trace_op() std::cerr << std::setw(6) << std::left << (ip-1-code->data()) << "\t" << std::setw(15) << Code::get_opcode_name(*(ip-1)) << "stack size = " << intptr_t(top - stack.data()) << std::endl;
#else
#	define trace_op()
#endif


namespace phonometrica {

static const int STACK_SIZE = 1024;
bool Runtime::initialized = false;


Runtime::Runtime() :
		stack(STACK_SIZE, Variant()), parser(this), compiler(this)
{
	srand(time(nullptr));

	if (! initialized)
	{
		utils::init_random_seed();
		Token::initialize();
		initialized = true;
	}

	get_item_string = intern_string("get_item");
	set_item_string = intern_string("set_item");
	create_builtins();
	set_global_namespace();
	this->top = this->stack.begin();
	this->limit = this->stack.end();
}

phonometrica::Runtime::~Runtime()
{
	// Make sure we don't double free variants that have been destructed but are not null.
	for (auto var = top; var < stack.end(); var++) {
		new (var) Variant;
	}
	stack.clear();
	globals.drop()->release();

	// Finalize classes manually: this is necessary because we must finalize Class last.
	for (auto &cls : classes) {
		cls->finalize();
	}
	for (size_t i = classes.size(); i-- > 2; )
	{
		auto ptr = classes[i].drop();
		ptr->release();
	}
	classes[0].drop()->release();
	classes[1].drop()->release();
}

void phonometrica::Runtime::add_candidate(Collectable *obj)
{
	gc.add_candidate(obj);
}

void phonometrica::Runtime::remove_candidate(Collectable *obj)
{
	gc.remove_candidate(obj);
}

void Runtime::create_builtins()
{
	// We need to boostrap the class system, since we are creating class instances but the class type doesn't exist
	// yet. We can't create it first because it inherits from Object.
	auto object_class = create_type<Object>("Object", nullptr, Class::Index::Object);
	auto raw_object_class = object_class.get();

	auto class_class = create_type<Class>("Class", raw_object_class, Class::Index::Class);
	assert(class_class->inherits(raw_object_class));

	assert(object_class.object()->get_class() == nullptr);
	assert(class_class.object()->get_class() == nullptr);

	object_class.object()->set_class(class_class.get());
	class_class.object()->set_class(class_class.get());

	// Create other builtin types.
	auto null_type = create_type<nullptr_t>("Null", raw_object_class, Class::Index::Null);
	auto bool_class = create_type<bool>("Boolean", raw_object_class, Class::Index::Boolean);
	auto num_class = create_type<Number>("Number", raw_object_class, Class::Index::Number);
	auto int_class = create_type<intptr_t>("Integer", num_class.get(), Class::Index::Integer);
	auto float_class = create_type<double>("Float", num_class.get(), Class::Index::Float);
	auto string_class = create_type<String>("String", raw_object_class, Class::Index::String);
	auto regex_class = create_type<Regex>("Regex", raw_object_class, Class::Index::Regex);
	auto list_class = create_type<List>("List", raw_object_class, Class::Index::List);
	auto array_class = create_type<Array<double>>("Array", raw_object_class, Class::Index::Array);
	auto table_class = create_type<Table>("Table", raw_object_class, Class::Index::Table);
	auto file_class = create_type<File>("File", raw_object_class, Class::Index::File);
	auto module_class = create_type<Module>("Module", raw_object_class, Class::Index::Module);
	// Function and Closure have the same name because the difference is an implementation detail.
	create_type<Function>("Function", raw_object_class, Class::Index::Function);
	auto func_class = create_type<Closure>("Function", raw_object_class, Class::Index::Closure);
	auto set_class = create_type<Set>("Set", raw_object_class, Class::Index::Set);

	// Iterators are currently not exposed to users.
	create_type<Iterator>("Iterator", raw_object_class, Class::Index::Iterator);
	create_type<ListIterator>("Iterator", raw_object_class, Class::Index::ListIterator);
	create_type<TableIterator>("Iterator", raw_object_class, Class::Index::TableIterator);
	create_type<StringIterator>("Iterator", raw_object_class, Class::Index::StringIterator);
	create_type<FileIterator>("Iterator", raw_object_class, Class::Index::FileIterator);
	create_type<RegexIterator>("Iterator", raw_object_class, Class::Index::RegexIterator);

	// Sanity checks
	assert(object_class.object()->get_class() != nullptr);
	assert(class_class.object()->get_class() != nullptr);
	assert((Class::get<Class>()) != nullptr);

	globals = make_handle<Module>(this, "global");

#define GLOB(T, h) add_global(Class::get_name<T>(), std::move(h));
	GLOB(Object, object_class);
	GLOB(bool, bool_class);
	GLOB(Number, num_class);
	GLOB(intptr_t, int_class);
	GLOB(double, float_class);
	GLOB(String, string_class);
	GLOB(Regex, regex_class);
	GLOB(List, list_class);
	GLOB(Array<double>, array_class);
	GLOB(Table, table_class);
	GLOB(File, file_class);
	GLOB(Closure, func_class);
	GLOB(Module, module_class);
	GLOB(Set, set_class);
#undef GLOB
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
		RUNTIME_ERROR("[Internal error] Stack underflow");
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
		RUNTIME_ERROR("[Internal error] Stack underflow");
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
		intptr_t value = -raw_cast<intptr_t>(var);
		pop();
		push_int(value);
	}
	else if (var.is_float())
	{
		double value = -raw_cast<double>(var);
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
	auto &v1 = peek(-2).resolve();
	auto &v2 = peek(-1).resolve();
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
						RUNTIME_ERROR("[Math error] Integer overflow");
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
	RUNTIME_ERROR("[Type error] Cannot apply math operator '%' to % and %", opstring, v1.class_name(), v2.class_name());
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

Variant Runtime::interpret(Closure &closure)
{
	if (current_frame) {
		current_frame->previous_routine = current_routine;
	}
	assert(!closure.routine->is_native());
	Routine &routine = *(reinterpret_cast<Routine*>(closure.routine.get()));
	current_routine = &routine;
	code = &routine.code;
	ip = routine.code.data();

	while (true)
	{
		auto op = static_cast<Opcode>(*ip++);

		switch (op)
		{
			case Opcode::Add:
			{
				trace_op();
				math_op('+');
				break;
			}
			case Opcode::Assert:
			{
				trace_op();
				int narg = *ip++;
				bool value = peek(-narg).to_boolean();
				if (!value)
				{
					auto msg = (narg == 2) ? utils::format("Assertion failed: %", peek(-1).to_string()) : std::string("Assertion failed");
					RUNTIME_ERROR(msg);
				}
				break;
			}
			case Opcode::Call:
			{
				trace_op();
				Instruction flags = *ip++;
				// TODO: handle return by reference
				needs_ref = flags & (1<<9);
				int narg = flags & 255;

				auto &v = peek(-narg - 1);
				// Precall already checked that we have a function object.
				auto &func = raw_cast<Function>(v);
				std::span<Variant> args(top - narg, narg);

				try 
				{
					auto c = func.find_closure(args);
					if (!c) {
						report_call_error(func, args);
					}

					if (c->routine->is_native())
					{
						try {
							auto result = (*c)(*this, args);
							pop(narg + 1);
							push(std::move(result));
						}
						CATCH_ERROR
					}
					else
					{
						current_frame->ip = ip;
						push((*c)(*this, args));
					}
				}
				CATCH_ERROR
				needs_ref = false;
				break;
			}
			case Opcode::ClearLocal:
			{
				trace_op();
				auto &v = current_frame->locals[*ip++];
				v.clear();
				break;
			}
			case Opcode::Compare:
			{
				trace_op();
				auto &v1 = peek(-2);
				auto &v2 = peek(-1);
				int result = v1.compare(v2);
				pop(2);
				push_int(result);
				break;
			}
			case Opcode::Concat:
			{
				trace_op();
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
				trace_op();
				int index = *ip++;
				auto &v = current_frame->locals[index];
				assert(v.is_integer());
				raw_cast<intptr_t>(v)--;
				break;
			}
			case Opcode::DefineGlobal:
			{
				trace_op();
				auto name = routine.get_string(*ip++);
				if (globals->find(name) != globals->end())
				{
					RUNTIME_ERROR("Global variable \"%\" is already defined", name);
				}
				globals->insert({name, std::move(peek())});
				pop();
				break;
			}
			case Opcode::DefineLocal:
			{
				trace_op();
				Variant &local = current_frame->locals[*ip++];
				local = std::move(peek());
				pop();
				break;
			}
			case Opcode::Divide:
			{
				trace_op();
				math_op('/');
				break;
			}
			case Opcode::Equal:
			{
				trace_op();
				auto &v2 = peek(-1);
				auto &v1 = peek(-2);
				bool value = (v1 == v2);
				pop(2);
				push(value);
				break;
			}
			case Opcode::GetGlobal:
			{
				trace_op();
				auto name = routine.get_string(*ip++);
				auto it = globals->find(name);
				if (it == globals->end()) {
					RUNTIME_ERROR("[Symbol error] Undefined variable \"%\"", name);
				}
				push(it->second.resolve());
				break;
			}
			case Opcode::GetGlobalArg:
			{
				trace_op();
				auto name = routine.get_string(*ip++);
				bool by_ref = current_frame->ref_flags[*ip++];
				auto it = globals->find(name);
				if (it == globals->end()) {
					RUNTIME_ERROR("[Symbol error] Undefined variable \"%\"", name);
				}
				if (by_ref)
				{
					it->second.unshare();
					push(it->second.make_alias());
				}
				else
				{
					push(it->second.resolve());
				}
				break;
			}
			case Opcode::GetGlobalRef:
			{
				trace_op();
				auto name = routine.get_string(*ip++);
				auto it = globals->find(name);
				if (it == globals->end()) {
					RUNTIME_ERROR("[Symbol error] Undefined variable \"%\"", name);
				}
				it->second.unshare();
				push(it->second.make_alias());
				break;
			}
			case Opcode::GetIndex:
			{
				trace_op();
				get_index(*ip++, false);
				break;
			}
			case Opcode::GetIndexArg:
			{
				trace_op();
				int count = *ip++;
				bool by_ref = current_frame->ref_flags[*ip++];
				if (by_ref) {
					RUNTIME_ERROR("Passing indexed expression as an argument by reference is not yet supported");
				}
				get_index(count, by_ref);
				break;
			}
			case Opcode::GetIndexRef:
			{
				trace_op();
				get_index(*ip++, true);
				break;
			}
			case Opcode::GetLocal:
			{
				trace_op();
				auto &v = current_frame->locals[*ip++];
				push(v.resolve());
				break;
			}
			case Opcode::GetLocalArg:
			{
				trace_op();
				auto &v = current_frame->locals[*ip++];
				bool by_ref = current_frame->ref_flags[*ip++];
				if (by_ref) {
					push(v.make_alias());
				}
				else {
					push(v.resolve());
				}
				break;
			}
			case Opcode::GetLocalRef:
			{
				trace_op();
				Variant &v = current_frame->locals[*ip++];
				push(v.make_alias());
				break;
			}
			case Opcode::GetUniqueGlobal:
			{
				trace_op();
				auto name = routine.get_string(*ip++);
				auto it = globals->find(name);
				if (it == globals->end()) {
					RUNTIME_ERROR("[Symbol error] Undefined variable \"%\"", name);
				}
				push(it->second.unshare());
				break;
			}
			case Opcode::GetUniqueLocal:
			{
				trace_op();
				push(current_frame->locals[*ip++].unshare());
				break;
			}
			case Opcode::Greater:
			{
				trace_op();
				auto &v2 = peek(-1);
				auto &v1 = peek(-2);
				bool value = (v1.compare(v2) > 0);
				pop(2);
				push(value);
				break;
			}
			case Opcode::GreaterEqual:
			{
				trace_op();
				auto &v2 = peek(-1);
				auto &v1 = peek(-2);
				bool value = (v1.compare(v2) >= 0);
				pop(2);
				push(value);
				break;
			}
			case Opcode::IncrementLocal:
			{
				trace_op();
				int index = *ip++;
				auto &v = current_frame->locals[index];
				assert(v.is_integer());
				raw_cast<intptr_t>(v)++;
				break;
			}
			case Opcode::Jump:
			{
				trace_op();
				int addr = Code::read_integer(ip);
				ip = code->data() + addr;
				break;
			}
			case Opcode::JumpFalse:
			{
				trace_op();
				int addr = Code::read_integer(ip);
				bool value = peek().to_boolean();
				pop();
				if (!value) ip = code->data() + addr;
				break;
			}
			case Opcode::JumpTrue:
			{
				trace_op();
				int addr = Code::read_integer(ip);
				bool value = peek().to_boolean();
				pop();
				if (value) ip = code->data() + addr;
				break;
			}
			case Opcode::Less:
			{
				trace_op();
				auto &v2 = peek(-1);
				auto &v1 = peek(-2);
				bool value = (v1.compare(v2) < 0);
				pop(2);
				push(value);
				break;
			}
			case Opcode::LessEqual:
			{
				trace_op();
				auto &v2 = peek(-1);
				auto &v1 = peek(-2);
				bool value = (v1.compare(v2) <= 0);
				pop(2);
				push(value);
				break;
			}
			case Opcode::Modulus:
			{
				trace_op();
				math_op('%');
				break;
			}
			case Opcode::Multiply:
			{
				trace_op();
				math_op('*');
				break;
			}
			case Opcode::Negate:
			{
				trace_op();
				negate();
				break;
			}
			case Opcode::NewArray:
			{
				trace_op();
				int nrow = *ip++;
				int ncol = *ip++;
				int narg = nrow * ncol;
				if (nrow == 1)
				{
					Array<double> array(ncol, 0.0);
					for (int i = 1; i <= ncol; i++)
					{
						array(i) = peek(-ncol + i - 1).to_float();
					}
					pop(narg);
					push(make_handle<Array<double>>(std::move(array)));
				}
				else
				{
					int k = narg;
					Array<double> array(nrow, ncol, 0.0);
					for (int i = 1; i <= nrow; i++)
					{
						for (int j = 1; j <= ncol; j++)
						{
							array(i,j) = peek(-k).to_float();
							k--;
						}
					}
					pop(narg);
					push(make_handle<Array<double>>(std::move(array)));
				}
				break;
			}
			case Opcode::NewClosure:
			{
				trace_op();
				const int index = *ip++;
				const int narg = *ip++;
				auto r = routine.get_routine(index);
				if (!r->sealed())
				{
					for (int i = narg; i > 0; i--)
					{
						auto &v = peek(-i);
						if (!check_type<Class>(v)) {
							RUNTIME_ERROR("Expected a Class object as type of parameter %", (narg + 1 - i));
						}
						r->add_parameter_type(v.handle<Class>());
					}
					r->seal();
				}
				pop(narg);
				push(make_handle<Function>(r->name(), make_handle<Closure>(r)));
				break;
			}
			case Opcode::NewFrame:
			{
				trace_op();
				push_call_frame(*ip++);
				break;
			}
			case Opcode::NewIterator:
			{
				trace_op();
				bool ref_val = bool(*ip++);
				auto v = std::move(peek());
				pop();
				if (check_type<List>(v)) {
					push(make_handle<ListIterator>(std::move(v), ref_val));
				}
				else if (check_type<Table>(v)) {
					push(make_handle<TableIterator>(std::move(v), ref_val));
				}
				else if (check_type<File>(v)) {
					push(make_handle<FileIterator>(std::move(v), ref_val));
				}
				else if (check_type<Regex>(v)) {
					push(make_handle<RegexIterator>(std::move(v), ref_val));
				}
				else if (check_type<String>(v)) {
					push(make_handle<StringIterator>(std::move(v), ref_val));
				}
				else {
					RUNTIME_ERROR("Type % is not iterable", v.class_name());
				}
				break;
			}
			case Opcode::NewList:
			{
				trace_op();
				int narg = *ip++;
				List lst(narg);
				for (int i = narg; i > 0; i--) {
					lst[narg+1-i] = std::move(peek(-i));
				}
				pop(narg);
				push(make_handle<List>(this, std::move(lst)));
				break;
			}
			case Opcode::NewTable:
			{
				trace_op();
				int narg = *ip++ * 2;
				Table::Storage tab;
				for (int i = narg; i > 0; i -= 2)
				{
					auto &key = peek(-i).resolve();
					auto &val = peek(-i+1).resolve();
					try {
						tab.insert({ std::move(key), std::move(val) });
					}
					CATCH_ERROR
				}
				pop(narg);
				push(make_handle<Table>(this, std::move(tab)));
				break;
			}
			case Opcode::NewSet:
			{
				trace_op();
				int narg = *ip++;
				Set::Storage set;
				for (int i = narg; i > 0; i--) {
					set.insert(std::move(peek(-i)));
				}
				pop(narg);
				push(make_handle<Set>(this, std::move(set)));
				break;
			}
			case Opcode::NextKey:
			{
				trace_op();
				try {
					auto v = std::move(peek());
					pop();
					auto &it = raw_cast<Iterator>(v);
					push(it.get_key());
				}
				CATCH_ERROR
				break;
			}
			case Opcode::NextValue:
			{
				trace_op();
				try {
					auto v = std::move(peek());
					pop();
					auto &it = raw_cast<Iterator>(v);
					push(it.get_value());
				}
				CATCH_ERROR

				break;
			}
			case Opcode::Not:
			{
				trace_op();
				bool value = peek().to_boolean();
				pop();
				push(!value);
				break;
			}
			case Opcode::NotEqual:
			{
				trace_op();
				auto &v2 = peek(-1);
				auto &v1 = peek(-2);
				bool value = (v1 != v2);
				pop(2);
				push(value);
				break;
			}
			case Opcode::Pop:
			{
				trace_op();
				pop();
				break;
			}
			case Opcode::Power:
			{
				trace_op();
				math_op('^');
				break;
			}
			case Opcode::Precall:
			{
				trace_op();
				auto &v = peek();
				Handle<Function> func;
				if (check_type<Function>(v))
				{
					func = v.resolve().handle<Function>();
				}
				else if (check_type<Class>(v))
				{
					// Replace the class with its constructor.
					auto cls = v.handle<Class>();
					pop();
					try {
						func = cls->get_constructor();
						push(func);
					}
					CATCH_ERROR
				}
				else
				{
					RUNTIME_ERROR("Expected a Function or a Class, got a %", v.class_name());
				}

				current_frame->ref_flags = func->ref_flags;
				break;
			}
			case Opcode::Print:
			{
				trace_op();
				int narg = *ip++;
				for (int i = narg; i > 0; i--)
				{
					auto s = peek(-i).to_string();
					utils::printf(s);
				}
				pop(narg);
				break;
			}
			case Opcode::PrintLine:
			{
				trace_op();
				int narg = *ip++;
				for (int i = narg; i > 0; i--)
				{
					auto s = peek(-i).to_string();
					utils::printf(s);
				}
				printf("\n");
				pop(narg);
				break;
			}
			case Opcode::PushBoolean:
			{
				trace_op();
				bool value = bool(*ip++);
				push(value);
				break;
			}
			case Opcode::PushFalse:
			{
				trace_op();
				push(false);
				break;
			}
			case Opcode::PushFloat:
			{
				trace_op();
				double value = routine.get_float(*ip++);
				push(value);
				break;
			}
			case Opcode::PushInteger:
			{
				trace_op();
				intptr_t value = routine.get_integer(*ip++);
				push_int(value);
				break;
			}
			case Opcode::PushNan:
			{
				trace_op();
				push(std::nan(""));
				break;
			}
			case Opcode::PushNull:
			{
				trace_op();
				push_null();
				break;
			}
			case Opcode::PushSmallInt:
			{
				trace_op();
				push_int((int16_t) *ip++);
				break;
			}
			case Opcode::PushString:
			{
				trace_op();
				String value = routine.get_string(*ip++);
				push(std::move(value));
				break;
			}
			case Opcode::PushTrue:
			{
				trace_op();
				push(true);
				break;
			}
			case Opcode::Return:
			{
				trace_op();
				return pop_call_frame();
			}
			case Opcode::SetGlobal:
			{
				trace_op();
				auto name = routine.get_string(*ip++);
				auto it = globals->find(name);
				auto &v = peek();
				if (it == globals->end())
				{
					if (check_type<Function>(v)) {
						globals->insert({name, std::move(v)});
					}
					else {
						RUNTIME_ERROR("[Symbol error] Undefined variable \"%\"", name);
					}
				}
				else
				{
					try {
						it->second = std::move(v);
					}
					CATCH_ERROR
				}
				pop();
				break;
			}
			case Opcode::SetIndex:
			{
				trace_op();
				int count = *ip++ + 2; // add indexed expression and value
				auto &v = peek(-count);
				auto cls = v.get_class();
				std::span<Variant> args(&v, count);
				auto method = cls->get_method(set_item_string);
				if (!method) {
					RUNTIME_ERROR("[Type error] % type is not index-assignable");
				}
				auto c = method->find_closure(args);
				if (!c) {
					report_call_error(*method, args);
				}
				try {
					(*c)(*this, args);
				}
				CATCH_ERROR
				pop(count);
				break;
			}
			case Opcode::SetLocal:
			{
				trace_op();
				Variant &v = current_frame->locals[*ip++];
				try {
					v = std::move(peek());
				}
				CATCH_ERROR
				pop();
				break;
			}
			case Opcode::Subtract:
			{
				trace_op();
				math_op('-');
				break;
			}
			case Opcode::TestIterator:
			{
				trace_op();
				auto v = std::move(peek());
				pop();
				auto &it = raw_cast<Iterator>(v);
				push(!it.at_end());
				break;
			}
			case Opcode::Throw:
			{
				String msg;
				try {
					msg = peek().to_string();
					pop();
				}
				CATCH_ERROR
				RUNTIME_ERROR("[Runtime error] %", msg);
			}
			default:
				throw error("[Internal error] Invalid opcode: %", (int)op);
		}
	}

	return Variant();
}

void Runtime::disassemble(const Routine &routine, const String &name)
{
	printf("========================= %s =========================\n", name.data());
	printf("strings: %d, large integers: %d, floats: %d, routines: %d\n", (int)routine.string_pool.size(), (int) routine.integer_pool.size(),
		   (int) routine.float_pool.size(), (int) routine.routine_pool.size());
	printf("offset    line   instruction    operands   comments\n");
	size_t size = routine.code.size();

	for (size_t offset = 0; offset < size; )
	{
		offset += disassemble_instruction(routine, offset);
	}

	for (auto &r : routine.routine_pool)
	{
		printf("\n");
		disassemble(*r, r->name());
	}

}

void Runtime::disassemble(const Closure &closure, const String &name)
{
	auto &routine = *(reinterpret_cast<Routine*>(closure.routine.get()));
	disassemble(routine, name);
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
		case Opcode::Call:
		{
			int narg = routine.code[offset + 1];
			printf("CALL           %-5d\n", narg);
			return 2;
		}
		case Opcode::ClearLocal:
		{
			int index = routine.code[offset + 1];
			String value = routine.get_local_name(index);
			printf("CLEAR_LOCAL    %-5d     ; %s\n", index, value.data());
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
		case Opcode::GetGlobalArg:
		{
			int index = routine.code[offset + 1];
			int narg = routine.code[offset + 2];
			String value = routine.get_string(index);
			printf("GET_GLOBAL_ARG %-5d %-5d; %s\n", index, narg, value.data());
			return 3;
		}
		case Opcode::GetGlobalRef:
		{
			int index = routine.code[offset + 1];
			String value = routine.get_string(index);
			printf("GET_GLOBAL_REF %-5d      ; %s\n", index, value.data());
			return 2;
		}
		case Opcode::GetIndex:
		{
			int count = routine.code[offset + 1];
			printf("GET_INDEX      %-5d\n", count);
			return 2;
		}
		case Opcode::GetIndexArg:
		{
			int count = routine.code[offset + 1];
			int index = routine.code[offset + 2];
			printf("GET_INDEX_ARG %-5d %-5d\n", count, index);
			return 3;
		}
		case Opcode::GetIndexRef:
		{
			int count = routine.code[offset + 1];
			printf("GET_INDEX_REF  %-5d\n", count);
			return 2;
		}
		case Opcode::GetLocal:
		{
			int index = routine.code[offset + 1];
			String value = routine.get_local_name(index);
			printf("GET_LOCAL      %-5d      ; %s\n", index, value.data());
			return 2;
		}
		case Opcode::GetLocalArg:
		{
			int index = routine.code[offset + 1];
			int narg = routine.code[offset + 2];
			String value = routine.get_local_name(index);
			printf("GET_LOCAL_ARG  %-5d %-5d; %s\n", index, narg, value.data());
			return 3;
		}
		case Opcode::GetLocalRef:
		{
			int index = routine.code[offset + 1];
			String value = routine.get_local_name(index);
			printf("GET_LOCAL_REF  %-5d      ; %s\n", index, value.data());
			return 2;
		}
		case Opcode::GetUniqueGlobal:
		{
			int index = routine.code[offset + 1];
			String value = routine.get_string(index);
			printf("GET_UNIQUE_GLOBAL %-5d   ; %s\n", index, value.data());
			return 2;
		}
		case Opcode::GetUniqueLocal:
		{
			int index = routine.code[offset + 1];
			String value = routine.get_local_name(index);
			printf("GET_UNIQUE_LOCAL  %-5d   ; %s\n", index, value.data());
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
		case Opcode::NewArray:
		{
			int nrow = routine.code[offset+1];
			int ncol = routine.code[offset+2];
			printf("NEW_ARRAY      %-5d %-5d\n", nrow, ncol);
			return 3;
		}
		case Opcode::NewClosure:
		{
			int index = routine.code[offset + 1];
			int narg = routine.code[offset + 2];
			auto r = routine.get_routine(index);
			printf("NEW_CLOSURE    %-3d %-5d  ; <%p>\n", index, narg, r.get());
			return 3;
		}
		case Opcode::NewFrame:
		{
			int nlocal = routine.code[offset+1];
			printf("NEW_FRAME      %-5d\n", nlocal);
			return 2;
		}
		case Opcode::NewIterator:
		{
			bool ref_val = bool(routine.code[offset+1]);
			printf("NEW_ITER       %-5d\n", ref_val);
			return 2;
		}
		case Opcode::NewList:
		{
			int nlocal = routine.code[offset+1];
			printf("NEW_LIST       %-5d\n", nlocal);
			return 2;
		}
		case Opcode::NewTable:
		{
			int len = routine.code[offset+1];
			printf("NEW_TABLE      %-5d\n", len);
			return 2;
		}
		case Opcode::NewSet:
		{
			int nlocal = routine.code[offset+1];
			printf("NEW_SET        %-5d\n", nlocal);
			return 2;
		}
		case Opcode::NextKey:
		{
			return print_simple_instruction("NEXT_KEY");
		}
		case Opcode::NextValue:
		{
			return print_simple_instruction("NEXT_VALUE");
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
		case Opcode::Precall:
		{
			return print_simple_instruction("PRECALL");
		}
		case Opcode::Print:
		{
			int narg = routine.code[offset+1];
			printf("PRINT         %-5d\n", narg);
			return 2;
		}
		case Opcode::PrintLine:
		{
			int narg = routine.code[offset+1];
			printf("PRINT_LINE     %-5d\n", narg);
			return 2;

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
		case Opcode::SetIndex:
		{
			int count = routine.code[offset + 1];
			printf("SET_INDEX      %-5d\n", count);
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
		case Opcode::TestIterator:
		{
			return print_simple_instruction("TEST_ITER");
		}
		case Opcode::Throw:
		{
			return print_simple_instruction("THROW");
		}
		default:
			printf("Unknown opcode %d", static_cast<int>(op));
	}

	return 1;
}

Handle<Closure> Runtime::compile_file(const String &path)
{
	this->clear();
	auto ast = parser.parse_file(path);

	return compiler.compile(std::move(ast));
}

Variant Runtime::do_file(const String &path)
{
	auto closure = compile_file(path);
	return interpret(*closure);
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

void Runtime::push_call_frame(int nlocal)
{
	// FIXME: valgrind complains about a fishy size passed to malloc here when PHON_STD_UNORDERED_MAP is defined.
	frames.push_back(std::make_unique<CallFrame>());
	current_frame = frames.back().get();
	ensure_capacity(nlocal);
	current_frame->locals = top;
	current_frame->nlocal = nlocal;
	int argc = current_routine->arg_count();
	top += argc;
	for (int i = argc; i < nlocal; i++) {
		new (top++) Variant;
	}
}

Variant Runtime::pop_call_frame()
{
	Variant result;

	// If there's a value left on top of the stack, it is taken as the return value.
	auto locals_end = current_frame->locals + current_frame->nlocal;
	if (top > locals_end) {
		result = std::move(*--top);
	}

	// Clean current frame.
	auto n = int(top - current_frame->locals);
	assert(n >= 0);
	pop(n + 1); // pop the frame + the function that was left on the stack.

	// Restore previous frame.
	frames.pop_back();
	current_frame = frames.empty() ? nullptr : frames.back().get();

	if (frames.empty())
	{
		current_frame = nullptr;
		code = nullptr;
		current_routine = nullptr;
		ip = nullptr;
	}
	else
	{
		current_frame = frames.back().get();
		code = &current_frame->previous_routine->code;
		current_routine = current_frame->previous_routine;
		ip = current_frame->ip;
	}

	return result;
}

void Runtime::add_global(String name, Variant value)
{
	globals->insert({ std::move(name), std::move(value) });
}

Variant Runtime::interpret(Closure &closure, std::span<Variant> args)
{
	// The arguments are on top of the stack. We adjust the top of the stack accordingly.
	top -= args.size();

	// Now we can just interpret the routine. The first opcode wil be NewFrame.
	return interpret(closure);
}

void Runtime::add_global(const String &name, NativeCallback cb, std::initializer_list<Handle<Class>> sig, ParamBitset ref)
{
	(*globals)[name] = make_handle<Function>(name, std::move(cb), sig, ref);
}

void Runtime::get_index(int count, bool by_ref)
{
	needs_ref = by_ref;
	count++; // add indexed expression
	auto &v = peek(-count);
	Variant result;
	std::span<Variant> args(&v, count);
	auto cls = v.get_class();
	auto method = cls->get_method(get_item_string);
	if (!method) {
		RUNTIME_ERROR("[Type error] % type is not indexable");
	}
	auto closure = method->find_closure(args);
	if (!closure) {
		report_call_error(*method, args);
	}
	try {
		result = (*closure)(*this, args);
	}
	CATCH_ERROR
	pop(count);
	push(std::move(result));
	needs_ref = false;
}

bool Runtime::needs_reference() const
{
	return needs_ref;
}

void Runtime::clear()
{
	needs_ref = false;
}

Variant &Runtime::operator[](const String &key)
{
	return (*globals)[key];
}

bool Runtime::debug_mode() const
{
	return debugging;
}

void Runtime::set_debug_mode(bool value)
{
	debugging = value;
}

void Runtime::report_call_error(const Function &func, std::span<Variant> args)
{
	Array<String> types;

	for (auto &arg : args) {
		types.append(arg.class_name());
	}
	String candidates;
	for (auto &c : func.closures) {
		candidates.append(c->routine->get_definition());
		candidates.append('\n');
	}
	RUNTIME_ERROR("Cannot resolve call to function '%' with the following argument types: (%).\nCandidates are:\n%",
				  func.name(), String::join(types, ", "), candidates);
}


} // namespace phonometrica

#undef CATCH_ERROR
#undef RUNTIME_ERROR
#undef trace_op