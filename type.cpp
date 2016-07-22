#include "type.h"
#include "codegen.h"
#include "symbols.h"
#include "image.h"
#include "rtti.h"

Type * register_type = 0;
Type * signed_register_type = 0;
Type * byte_type = 0;
uint64 class_id_counter = 1;  // 0 is unknown class

void IntegerType::copy(Codegen * c, Value * a, Value * v)
{
	uint64 store = STORE;
	if (bits < 9)
	{
		store = STORE8;
	}
	else if (bits < 17)
	{
		store = STORE16;
	}
	else if (bits < 33)
	{
		store = STORE32;
	}
	else if (bits < 65)
	{
		store = STORE64;
	}
	else
	{
		assert(false);
	}

	if (v->is_number)
	{
		Value * v2 = c->getTemporary(register_type, "intcopy");
		c->block()->add(Insn(MOVE, v2, v));
		c->block()->add(Insn(store, a, v2));
	}
	else
	{
		c->block()->add(Insn(store, a, v));
	}
}

bool IntegerType::construct(Codegen * c, Value * t, Value * v)
{
	if (v)
	{
		if (v->type)
		{
			if (!v->type->inRegister())
			{
				return false;
			}
		}
		c->block()->add(Insn(MOVE, t, v));
	}
	return true;
}

bool PointerType::construct(Codegen * c, Value * t, Value * v)
{
	if (v)
	{
		if (v->type)
		{
			if (!v->type->inRegister())
			{
				return false;
			}
		}
		c->block()->add(Insn(MOVE, t, v));
	}
	return true;
}

bool ActivationType::construct(Codegen * c, Value * t, Value * v)
{
	if (v)
	{
		if (v->type)
		{
			if (!v->type->inRegister())
			{
				return false;
			}
		}
		c->block()->add(Insn(MOVE, t, v));
	}
	return true;
}

Value * ActivationType::getActivatedValue(Codegen * c, Value * act)
{
	Value * ret = c->getTemporary(activatedType(), "activation_return");
	c->block()->add(Insn(LOAD, ret, act, Operand::sigc(assembler->returnOffset())));
	return ret;
}

void ActivationType::copy(Codegen * c, Value * a, Value * v)
{
	Value * tmp = c->getTemporary(pointed_type, "actcopy");
	c->block()->add(Insn(LOAD, tmp, v, Operand::sigc(assembler->returnOffset())));
	c->block()->add(Insn(STORE, a, tmp));
}

void PointerType::copy(Codegen * c, Value * a, Value * v)
{
	c->block()->add(Insn(STORE, a, v));
}

void PointerType::calcAddress(Codegen * c, Value * a, Expr *)
{
	c->block()->add(Insn(LOAD, a, a));
}

void FunctionType::copy(Codegen * c, Value * a, Value * v)
{
	c->block()->add(Insn(STORE, a, v));
}

void ArrayType::calcAddress(Codegen * c, Value * a, Expr * i)
{
	Value * v = c->getTemporary(register_type, "arrayaddr");
	Value * ss = i->codegen(c);

	if (!ss)
	{
		fprintf(log_file, "Got null value on codegen\n");
		return;
	}

	if (ss->is_number)
	{
		c->block()->add(Insn(MOVE, v,
			Operand::usigc(pointed_type->size() / 8 * ss->val)));
	}
	else
	{
		c->block()->add(Insn(MUL, v, Operand::usigc(pointed_type->size() / 8), ss));
	}

	c->block()->add(Insn(ADD, a, a, v));
}

void StructType::calcAddress(Codegen * c, Value * a, Expr * i)
{
	IdentifierExpr * ie = (IdentifierExpr *)i;
	if (siz == 0)
	{
		calc();
	}

	if (is_union)
	{
		return;
	}

	for (unsigned int loopc = 0; loopc < members.size(); loopc++)
	{
		if (members[loopc].name == ie->getString())
		{
			c->block()->add(Insn(ADD, a, a,
				Operand::usigc(members[loopc].offset / 8)));
		}
	}
}

void StructType::calc()
{
	siz = 0;
	if (!is_union && parent)
	{
		siz = parent->size();
	}

	for (unsigned int loopc = 0; loopc < members.size(); loopc++)
	{
		members[loopc].offset = siz;
		if (is_union)
		{
			siz = (siz < members[loopc].type->size()) ?
				members[loopc].type->size() : siz;
		}
		else
		{
			siz += members[loopc].type->size();
		}
	}

	if (is_union)
	{
		siz = parent->size() > siz ? parent->size() : siz;
	}
}

Type * StructType::fieldType(std::string n)
{
	for (unsigned int loopc = 0; loopc < members.size(); loopc++)
	{
		if (members[loopc].name == n)
		{
			return members[loopc].type;
		}
	}

	if (parent)
	{
		return fieldType(n);
	}

	return 0;
}

void FunctionType::calc()
{
}

void StructType::copy(Codegen * c, Value * to, Value * from)
{
	Value * fa = c->getTemporary(register_type, "structaddr");
	c->block()->add(Insn(GETADDR, fa, from));
	Value * v = c->getTemporary(byte_type, "copy");
	for (int loopc = 0; loopc < siz / 8; loopc++)
	{
		// Needs more better        
		c->block()->add(Insn(LOAD8, v, fa));
		c->block()->add(Insn(STORE8, to, v));
		c->block()->add(Insn(ADD, fa, Operand::usigc(1)));
		c->block()->add(Insn(ADD, to, Operand::usigc(1)));
	}
}

bool StructType::canCopy(Type * t)
{
	if (t == this)
	{
		for (unsigned int loopc = 0; loopc < members.size(); loopc++)
		{
			if (!members[loopc].type->canCopy(members[loopc].type))
			{
				return false;
			}
		}
		return true;
	}

	return false;
}

StructType::StructType(std::string n, bool u, StructType * p)
{
	nam = n;
	siz = 0;
	is_union = u;
	parent = p;

	if (!p)
	{
		addMember("class", register_type);
		if (is_union)
		{
			addMember("current_type", register_type);
		}
	}
    else
    {
        p->registerChild(this);
    }

        // Shouldn't be able to make pointer to this type before it's defined
    assert(!types->lookup(name()+"^"));
        // So for each struct there is a pointer to the struct with
        // the class id of the struct + 1
    new PointerType(this);
}

bool StructType::construct(Codegen * c, Value * t, Value *)
{
	Value * v = c->getTemporary(register_type, "struct_addr");
	c->block()->add(Insn(GETADDR, v, t, Operand::usigc(0)));
	Value * ra = c->getTemporary(register_type, "rtti_addr");
	uint64 offs = rtti->lookup(classId());

	c->block()->add(Insn(MOVE, ra,
		Operand::section(IMAGE_RTTI, offs)));
	c->block()->add(Insn(STORE, v, ra));
	return true;
}

std::map<std::string, Type *> & Types::get()
{
	return types;
}

void Types::add(Type * t, std::string n)
{
	types[n] = t;
}

Type * Types::lookup(std::string n)
{
	std::map<std::string, Type *>::iterator it = types.find(n);
	if (it != types.end())
	{
		return (*it).second;
	}

	if (n.find("^") == std::string::npos && n.find("[") == std::string::npos)
	{

		// Special case for integers
		int size;
		if (sscanf(n.c_str(), "Uint%d", &size) == 1)
		{
			fprintf(log_file, "Uint match %d\n", size);
			Type * t = new IntegerType(false, size);
			types[n] = t;
			return t;
		}

		if (sscanf(n.c_str(), "Int%d", &size) == 1)
		{
			fprintf(log_file, "Int match %d\n", size);
			Type * t = new IntegerType(true, size);
			types[n] = t;
			return t;
		}
	}

	fprintf(log_file, "No idea what type [%s] is\n", n.c_str());
	return 0;
}

Types::Types()
{
	types["Bool"] = new BoolType();
	types["Void"] = new VoidType();
	byte_type = new IntegerType(true, 8);
	types["Byte"] = byte_type;
	register_type = new IntegerType(false, assembler->pointerSize());
	signed_register_type = new IntegerType(true, assembler->pointerSize());
	if (assembler->pointerSize() == 64)
	{
		types["Uint64"] = register_type;
		types["Int64"] = signed_register_type;
	}
	else
	{
		types["Uint32"] = register_type;
		types["Int32"] = signed_register_type;
	}

	types["Uint"] = register_type;
	types["Int"] = signed_register_type;

	if (assembler->convertUint64())
	{
		types["Uint64"] = register_type;
		types["Int64"] = signed_register_type;
	}

	types["Byte^"] = new PointerType(byte_type);
}

Types::~Types()
{
	for (std::map<std::string, Type *>::iterator it = types.begin();
	it != types.end(); it++)
	{
		delete (*it).second;
	}
}

std::string BoolType::display(unsigned char * addr)
{
	uint64 val = *((uint64 *)addr);
	if (val == 0)
	{
		return "false";
	}
	else
	{
		return "true";
	}
}

std::string IntegerType::display(unsigned char * addr)
{
	std::string ret;

	// Assume little endian unsigned for now
	for (int loopc = 0; loopc < size() / 8; loopc++)
	{
		unsigned char val = *addr;
		addr++;
		char buf[4096];
		sprintf(buf, "%02x", val);
		ret = buf + ret;
	}

	int idx = -1;
	for (unsigned int loopc = 0; loopc < ret.size(); loopc++)
	{
		if (ret[loopc] != '0')
		{
			break;
		}
		idx++;
	}

	if (idx != -1)
	{
		ret = ret.substr(idx + 1);
	}

	if (ret == "")
	{
		ret = "0";
	}

	return ret;
}

std::string PointerType::display(unsigned char * addr)
{
	uint64 val = *((uint64 *)addr);
	char buf[4096];
	sprintf(buf, "%llx", val);
	return buf;
}

std::string ArrayType::display(unsigned char * addr)
{
	std::string ret = "[";
	for (int loopc = 0; loopc < siz; loopc++)
	{
		if (loopc > 0)
		{
			ret += ",";
		}

		ret += pointed_type->display(addr);
		addr += pointed_type->size() / 8;
	}
	ret += "]";
	return ret;
}

std::string StructType::display(unsigned char * addr)
{
	std::string ret = "{";

	for (unsigned int loopc = 0; loopc < members.size(); loopc++)
	{
		if (loopc > 0)
		{
			ret += ",";
		}

		ret += members[loopc].type->display(addr + (members[loopc].offset / 8));
	}
	ret += "}";
	return ret;
}

// a) get function address
// b) get and init stack frame
// c) call stack frame
// d) get result
// e) dispose of stack frame

// frame is dynamic, ip, static link, return, args, locals

Value * FunctionType::generateFuncall(Codegen * c, Funcall * f, Value * fp,
	std::vector<Value *> & args)
{
	std::vector<Type *> rets = c->getScope()->getType()->getReturns();
	Value * to_add = 0;
	Value * new_frame = allocStackFrame(c, fp, to_add, f, rets.size() == 0 ? register_type : rets[0]);

	Value * fp_holder = c->getTemporary(register_type, "fp_holder");
	c->block()->add(Insn(MOVE, fp_holder, fp));
	// Get to the actual code
	c->block()->add(Insn(ADD, fp_holder, fp_holder,
		Operand::usigc(assembler->pointerSize() / 8)));
	c->block()->add(Insn(STORE, new_frame,
		Operand::reg(assembler->framePointer())));

	// Set initial stored ip to start of function
	c->block()->add(Insn(STORE, new_frame,
		Operand::sigc(assembler->ipOffset()), fp_holder));

	// Needs expanding
	if (c->getScope() == f->getScope())
	{
		// Recursive
		Value * static_link = c->getStaticLink();
		assert(static_link);

		c->block()->add(Insn(STORE, new_frame,
			Operand::sigc(assembler->staticLinkOffset()),
			static_link));
	}
	else
	{
		c->block()->add(Insn(STORE, new_frame,
			Operand::sigc(assembler->staticLinkOffset()),
			Operand::reg(assembler->framePointer())));
	}

	int current_offset = assembler->returnOffset();

	if (rets.size() == 0)
	{
		printf("Function has no return types!\n");
	}

	for (unsigned int loopc = 0; loopc < rets.size(); loopc++)
	{
		int align = rets[loopc]->align() / 8;
		while (current_offset % align)
		{
			current_offset++;
		}
		current_offset += rets[loopc]->size() / 8;
	}

	for (unsigned int loopc = 0; loopc < args.size(); loopc++)
	{
		int align = args[loopc]->type->align() / 8;
		while (current_offset % align)
		{
			current_offset++;
		}

		Value * arg = args[loopc];
		Type * intype = arg->type;
		Type * expectedtype = params[loopc].type;

		if (intype && !expectedtype->canActivate() && intype->canActivate())
		{
			arg = intype->getActivatedValue(c, arg);
		}

		c->block()->add(Insn(STORE, new_frame, Operand::sigc(current_offset), arg));
		current_offset += args[loopc]->type->size() / 8;
	}

	new_frame->type->activate(c, new_frame);
	return new_frame;
}

void ActivationType::activate(Codegen * c, Value * frame)
{
	BasicBlock * return_block = c->newBlock("return");
	Value * ip_holder = c->getTemporary(register_type, "ip_holder");
	c->block()->add(Insn(MOVE, ip_holder, return_block));
	c->block()->add(Insn(STORE, Operand::reg(assembler->framePointer()),
		Operand::sigc(assembler->ipOffset()), ip_holder));

	RegSet res;
	res.set(0);
	c->block()->setReservedRegs(res);
	c->block()->add(Insn(LOAD, Operand::reg(0), frame, Operand::sigc(assembler->ipOffset())));
	c->block()->add(Insn(MOVE, Operand::reg(assembler->framePointer()),
		frame));
	// Oops this doesn't work because frame pointer just got clobbered

	c->block()->add(Insn(BRA, Operand::reg(0)));
	// Call returns here
	c->setBlock(return_block);
}

Value * FunctionType::allocStackFrame(Codegen * c, Value * faddr,
	Value * & to_add, Funcall * f, Type * rettype)
{
	to_add = c->getTemporary(register_type, "to_add");
	c->block()->add(Insn(LOAD, to_add, faddr));

	int depth = 0;
	Value * next_frame_ptr = c->getScope()->lookup("__stackptr", depth);
	assert(next_frame_ptr);

	Value * addrof = c->getTemporary(register_type, "addr_of_stackptr");
	c->block()->add(Insn(GETADDR, addrof, next_frame_ptr, Operand::usigc(depth)));

	Value * new_ptr = c->getTemporary(new ActivationType(rettype), "new_ptr");
	c->block()->add(Insn(LOAD, new_ptr, addrof));

	Value * adder = c->getTemporary(register_type, "stackptr_add");
	c->block()->add(Insn(LOAD, adder, addrof));
	c->block()->add(Insn(ADD, adder, adder, to_add));
	c->block()->add(Insn(STORE, addrof, Operand::sigc(0), adder));
	return new_ptr;
}

bool FunctionType::validArgList(std::vector<Value *> & args, std::string & reason)
{
	reason = "";
	if (args.size() != params.size())
	{
		char buf[4096];
		sprintf(buf, "expected %ld arguments, got %ld", params.size(), args.size());
		reason = buf;
		return false;
	}

	for (unsigned int loopc = 0; loopc < args.size(); loopc++)
	{
		if (args[loopc]->type->size() != params[loopc].type->size())
		{
			char buf[4096];
			sprintf(buf, "%s - expected %d bit argument, got %d",
				params[loopc].name.c_str(),
				params[loopc].type->size(),
				args[loopc]->type->size());
			reason = buf;
			return false;
		}
	}

	return true;
}

Value * ExternalFunctionType::generateFuncall(Codegen * c, Funcall * f, Value * fp,
	std::vector<Value *> & args)
{
	return convention->generateCall(c, fp, args);
}

static bool cmp_func(FunctionScope* &a, FunctionScope* &b)
{
    return (a->getType()->getSignature()) >
        (b->getType()->getSignature());
}

Value * GenericFunctionType::generateFuncall(Codegen * c, Funcall * f, Value * fp,
	std::vector<Value *> & args)
{
    std::map<FunctionSignature, FunctionScope *> already_seen;
    
    std::sort(specialisations.begin(), specialisations.end(), cmp_func);
	printf("Generate generic funcall! Candidates:\n");
	for (std::vector<FunctionScope *>::iterator it = specialisations.begin(); it != specialisations.end(); it++)
	{
		printf("%s\n", (*it)->getType()->name().c_str());
	}

	return 0;
}
