#include "type.h"
#include "codegen.h"
#include "symbols.h"
#include "image.h"
#include "rtti.h"

// #define DEBUG_MTABLES

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

    if (!ie)
    {
        printf("Struct calcAddress called with null field name!\n");
        return;
    }

    StructType * st = this;
    do
    {
        for (unsigned int loopc = 0; loopc < st->members.size(); loopc++)
        {
            if (st->members[loopc].name == ie->getString())
            {
                c->block()->add(Insn(ADD, a, a,
                                 Operand::usigc(st->members[loopc].offset / 8)));
                return;
            }
        }
        st = parent;
	} while(st);

    printf("Unable to find offset for %s!\n", ie->getString().c_str());
}

void StructType::calc()
{
	siz = 0;
	if (!is_union && parent)
	{
        if (parent->size() == 0)
        {
            parent->calc();
        }
        
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
		return parent->fieldType(n);
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

StructType::StructType(std::string n, bool u, StructType * p, bool rtti)
{
	nam = n;
	siz = 0;
	is_union = u;
	parent = p;
    has_rtti = p ? p->hasRtti() : rtti;
	if (!p && rtti)
	{
		addMember("class", register_type);
		if (is_union)
		{
			addMember("current_type", register_type);
		}
	}
    else if (p)
    {
        p->registerChild(this);
    }

    std::string pname = name()+"^";
        // Shouldn't be able to make pointer to this type before it's defined
    assert(!types->lookup(pname));
        // So for each struct there is a pointer to the struct with
        // the class id of the struct + 1
    PointerType * pt = new PointerType(this);
    types->add(pt, pname);
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

Type * Types::lookup(uint64 id)
{
	std::map<std::string, Type *>::iterator it;
    for (it = types.begin(); it != types.end(); it++)
    {
        if ((*it).second->classId() == id)
        {
            return (*it).second;
        }
    }

    return 0;
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
        // Avoid double delete
    types.erase("Uint");
    types.erase("Int");

    if (types["Uint64"] == types["Uint32"])
    {
            // Convert-uint64 hack as above; avoid double delete
        types.erase("Uint64");
        types.erase("Int64");
    }
    
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

    if (is_signed && size() < 65)
    {
        int64 * val = ((int64 *)addr);
        char buf[4096];
        sprintf(buf, "%lld", *val);
        ret = buf;
        return ret;
    }
    
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
    if (!hasRtti())
    {
        ret += " raw";
    }
    
	return ret;
}

// a) get function address
// b) get and init stack frame
// c) call stack frame
// d) get result
// e) dispose of stack frame

// frame is dynamic, ip, static link, return, args, locals

Value * FunctionType::generateFuncall(Codegen * c, Funcall * f, Value * sl,
                                      Value * fp, std::vector<Value *> & args)
{
	Type * ret = c->getScope()->getType()->getReturn();
	Value * to_add = 0;
	Value * new_frame = allocStackFrame(c, fp, to_add, f, ret_type == 0 ? register_type : ret_type);

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

    if (sl)
    {
        c->block()->add(Insn(STORE, new_frame,
                             Operand::sigc(assembler->staticLinkOffset()), sl));
    }

	int current_offset = assembler->returnOffset();

	if (ret == 0)
	{
		printf("Function has no return type!\n");
	}
    else
	{
		int align = ret->align() / 8;
		while (current_offset % align)
		{
			current_offset++;
		}
		current_offset += ret->size() / 8;
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
		Type * expectedtype = params.size() > loopc ? params[loopc].type : 0;

        if (expectedtype) // not available for generics yet!
        {
            if (intype && !expectedtype->canActivate() && intype->canActivate())
            {
                arg = intype->getActivatedValue(c, arg);
            }
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

    std::string nam = "@"+rettype->name();
    Type * at = types->lookup(nam);
    if (!at)
    {
        at = new ActivationType(rettype);
        types->add(at, nam);
    }
    
	Value * new_ptr = c->getTemporary(at, "new_ptr");
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

Value * ExternalFunctionType::generateFuncall(Codegen * c, Funcall * f, Value *,
                                              Value * fp, std::vector<Value *> & args)
{
    return convention->generateCall(c, fp, args);   
}

GenericFunctionType::~GenericFunctionType()
{
}

static bool cmp_func(FunctionScope* &a, FunctionScope* &b)
{
    return (a->getType()->getSignature()) >
        (b->getType()->getSignature());
}

//  Per generic function:
//    Per candidate actual function, sorted in descending order of specifity:
//      Offset to next candidate
//      Number of arguments
//      Per argument, prefixed with length code:
//        Type code for a potential match
//      Function pointer
//
//  Go to first candidate, then:
//    If the function pointer is 0 we have no more candidates; die
//    Calculate address of next candidate
//    Go through each arg. If any of the type codes match, go to the next arg,
//    otherwise go to the next candidate.
//    If all the args have matched, load the function pointer at the end, then call it

Value * GenericFunctionType::generateFuncall(Codegen * c, Funcall * f,
                                             Value * sl,Value * fp,
                                             std::vector<Value *> & args)
{   
    uint64 wordsize = register_type->size() / 8;
    
    BasicBlock * no_functions_found = c->newBlock("no_functions_found");
    no_functions_found->add(Insn(BREAKP));

    BasicBlock * start_candidate = c->newBlock("start_candidate");
    c->setBlock(start_candidate);
    
    BasicBlock * arguments_loop_header = c->newBlock("arguments_loop_header");
    c->setBlock(arguments_loop_header);

    BasicBlock * args_count_check = c->newBlock("args_count_check");
    BasicBlock * arguments_loop_body = c->newBlock("arguments_loop_body");
    
    Value * pointer = c->getTemporary(register_type, "pointer");
    start_candidate->add(Insn(MOVE, pointer, fp));
    
    Value * next_candidate_offset = c->getTemporary(register_type, "next_candidate_offset");
    Value * next_candidate = c->getTemporary(register_type, "next_candidate");
    arguments_loop_header->add(Insn(LOAD, next_candidate_offset, pointer));
    arguments_loop_header->add(Insn(ADD, next_candidate, next_candidate_offset, pointer));
    arguments_loop_header->add(Insn(ADD, pointer, pointer, Operand::usigc(wordsize)));
    arguments_loop_header->add(Insn(CMP, next_candidate_offset, Operand::usigc(0)));
    arguments_loop_header->add(Insn(BEQ, no_functions_found, args_count_check));

    BasicBlock * no_we_did_not = c->newBlock("no_we_did_not");
    
    c->setBlock(args_count_check);
    Value * no_args = c->getTemporary(register_type, "no_args");
    args_count_check->add(Insn(LOAD, no_args, pointer));
    args_count_check->add(Insn(ADD, pointer, pointer, Operand::usigc(wordsize)));
    args_count_check->add(Insn(CMP, no_args, Operand::usigc(args.size())));
    args_count_check->add(Insn(BEQ, arguments_loop_body, no_we_did_not));
    
    Value * possible_matches = c->getTemporary(register_type, "possible_matches");
    Value * matched = c->getTemporary(register_type, "matched");

    c->setBlock(arguments_loop_body);
    BasicBlock * arguments_loop_tail = c->newBlock("arguments_loop_tail");

    no_we_did_not->add(Insn(MOVE, pointer, next_candidate));
    no_we_did_not->add(Insn(BRA, arguments_loop_header));
    arguments_loop_header->add(Insn(BRA, arguments_loop_body));
    
    BasicBlock * possible_matches_header = c->newBlock("possible_matches_header_0");
    
        // Reuse for each arg to reduce naive stack usage
    Value * candidate_type = c->getTemporary(register_type, "candidate_type");    
    Value * expected_type = c->getTemporary(register_type, "expected_type");
    Value * classptr = c->getTemporary(register_type, "classptr");
            
    for (unsigned int loopc=0; loopc<args.size(); loopc++)
    {
        c->setBlock(possible_matches_header);
        char buf[4096];
        sprintf(buf, "%d", loopc+1);
        std::string argnum(buf);
        
        if (loopc == 0)
        {
            arguments_loop_body->add(Insn(BRA, possible_matches_header));
        }

        possible_matches_header->add(Insn(MOVE, matched, Operand::usigc(0)));
        possible_matches_header->add(Insn(LOAD, possible_matches, pointer));
        possible_matches_header->add(Insn(ADD, pointer, pointer, 
                                        Operand::usigc(wordsize)));
        
        BasicBlock * possible_matches_body = c->newBlock("possible_matches_body"+argnum);
        c->setBlock(possible_matches_body);
        possible_matches_body->add(Insn(LOAD, candidate_type,  pointer));
        possible_matches_body->add(Insn(ADD, pointer, pointer, 
                                        Operand::usigc(wordsize)));
        
            // If it's a ptr-to-struct, we load the class id from the pointer
        bool deref = false;
        if (args[loopc]->type->canDeref())
        {
            int count = 0;
            Type * base = args[loopc]->type->baseDeref(count);
            if (count == 1 && base->hasRtti())
            {
                deref = true;
            }
                // Not sure how to handle Foo^^ yet
        }

        if (deref)
        {
            possible_matches_body->add(Insn(LOAD, classptr, args[loopc]));
            possible_matches_body->add(Insn(LOAD, expected_type, classptr));
            possible_matches_body->add(Insn(ADD, expected_type, expected_type, Operand::usigc(1)));
        }
        else
        {
            possible_matches_body->add(Insn(MOVE, expected_type, Operand::usigc(args[loopc]->type->classId())));
        }
        
        possible_matches_body->add(Insn(CMP, candidate_type, expected_type));
        possible_matches_body->add(Insn(SELEQ, matched, Operand::usigc(1), matched));
        
        BasicBlock * did_we_find_a_match = c->newBlock("did_we_find_a_match");
        possible_matches_body->add(Insn(SUB, possible_matches, possible_matches,
                                        Operand::usigc(1)));
        possible_matches_body->add(Insn(CMP, possible_matches, Operand::usigc(0)));
        possible_matches_body->add(Insn(BEQ, did_we_find_a_match, possible_matches_body));

        c->setBlock(did_we_find_a_match);
        did_we_find_a_match->add(Insn(CMP, matched, Operand::usigc(0)));
        possible_matches_header = c->newBlock("possible_matches_header_"+argnum);
        did_we_find_a_match->add(Insn(BEQ, no_we_did_not, possible_matches_header));
    }

    c->setBlock(possible_matches_header);  // blank
    possible_matches_header->add(Insn(BRA, arguments_loop_tail));
    c->setBlock(arguments_loop_tail);
        // We got to the end of all the arguments without noping out,
        // thus we have a match
    BasicBlock * found_match = c->newBlock("found_match");
    arguments_loop_tail->add(Insn(BRA, found_match));
    c->setBlock(no_functions_found);  // Jam it in any old where
    c->setBlock(no_we_did_not);  // Ditto
    c->setBlock(found_match);
    Value * ptr = c->getTemporary(register_type, "genfunptr");
    c->block()->add(Insn(LOAD, ptr, pointer));
    BasicBlock * genfuncall = c->newBlock("genfuncall");
    c->setBlock(genfuncall);
    Value * ret = FunctionType::generateFuncall(c, f, sl, ptr, args);
    BasicBlock * endgenfuncall = c->newBlock("endgenfuncall");
    c->block()->add(Insn(BRA, endgenfuncall));
    c->setBlock(endgenfuncall);
    return ret;
}

std::vector<FunctionScope *> & GenericFunctionType::getSpecialisations()
{
    std::sort(specialisations.begin(), specialisations.end(), cmp_func);
    return specialisations;
}

void Mtables::processFunction(FunctionScope * fs)
{
    MtableEntry me(fs);
     
    std::vector<StructElement> & params = fs->getType()->getParams();

    me.table.push_back(params.size());
    
    for (unsigned int loopc=0; loopc<params.size(); loopc++)
    {
        Type * t = params[loopc].type;
            // Probably don't want to keep dynamic_cast indefinitely
        StructType * st = dynamic_cast<StructType *>(t);
        if (st)
        {
            std::vector<StructType *> c = st->getChildren();
            me.table.push_back(1+c.size());
            me.table.push_back(t->classId());
            for (unsigned int loopc2=0; loopc2<c.size(); loopc2++)
            {
                me.table.push_back(c[loopc]->classId());
            }
        }
        else if (t->canDeref() && t->derefType()->canField())
        {
            StructType * st = dynamic_cast<StructType *>(t->derefType());
            if (st)
            {
                std::vector<StructType *> c = st->getChildren();
                me.table.push_back(1+c.size());
                me.table.push_back(t->classId());
                for (unsigned int loopc2=0; loopc2<c.size(); loopc2++)
                {
                    me.table.push_back(c[loopc]->classId()+1);
                }
            }
        }
        else
        {
            me.table.push_back(1);
            me.table.push_back(t->classId());
        }
    }
    data.push_back(me);
    offset += me.table.size() + 2;
}

void MtableEntry::print()
{
    printf("%s %ld ", target ? target->name().c_str() : "<null!>", offset);
    for (int loopc=0; loopc<table.size(); loopc++)
    {
        printf("[%d]", table[loopc]);
    }
    puts("");
}

void Mtables::generateTables()
{
    for (std::list<FunctionScope *>::iterator it = entries.begin(); it != entries.end(); it++)
    {
        GenericFunctionType * gft = (GenericFunctionType *)(*it)->getType();
        offsets[(*it)] = offset;
#ifdef DEBUG_MTABLES
        printf("Stored offset %llx for %s\n", offset, gft ? gft->name().c_str() : "<null!>");
#endif        
        std::vector<FunctionScope *> specialisations = gft->getSpecialisations();
        for (std::vector<FunctionScope *>::iterator it2 = specialisations.begin();
             it2 != specialisations.end(); it2++)
        {
            processFunction(*it2);
        }
        MtableEntry term(0);  // Empty table endicates end of entry
        data.push_back(term);
        offset += 2;
    } 
}

void Mtables::createSection(Image * i, Assembler * a)
{
    sf_bit = (a->pointerSize() == 64);
    bool le = a->littleEndian();
    i->setSectionSize(IMAGE_MTABLES, (offset+data.size()) * (sf_bit ? 8 : 4));
    unsigned char * ptr = i->getPtr(IMAGE_MTABLES);
    unsigned char * orig = ptr;

    for (unsigned int loopc=0; loopc<data.size(); loopc++)
    {
        MtableEntry & me = data[loopc];
        size_t len = me.table.size() == 0 ? 0 : me.table.size() + 2;
        len *= sf_bit ? 8 : 4;

        me.offset = (ptr-orig);
#ifdef DEBUG_MTABLES
        me.print();
#endif
        if (sf_bit)
        {
#ifdef DEBUG_MTABLES
			printf("(%llx) length %lld", ptr-orig, len);
#endif
            wee64(le, ptr, len);
			uint64 count = 0;
            for (unsigned int loopc2=0; loopc2<me.table.size(); loopc2++)
            {
				if (count == 0)
				{
					count = me.table[loopc2];
#ifdef DEBUG_MTABLES
					printf(" ");
#endif
				}
				else
				{
					count--;
				}
#ifdef DEBUG_MTABLES
				printf("(%llx)[%lld]", ptr-orig, me.table[loopc2]);
#endif
                wee64(le, ptr, me.table[loopc2]);
            }

            if (me.target)
            {
                MtableRelocation * mr = new MtableRelocation(i, me.target, ptr-orig);
                mr->add64();
#ifdef DEBUG_MTABLES
				printf(" (%llx)ptr\n", ptr-orig);
#endif
                wee64(le, ptr, 0xfeedbeeffeedbeefLL);
            }
			else
			{
#ifdef DEBUG_MTABLES
				printf(" (%llx)end\n", ptr-orig);
#endif
                wee64(le, ptr, 0x0);
			}
        }
        else
        {
#ifdef DEBUG_MTABLES
			printf("(%llx) length %lld", ptr-orig, len);
#endif
            wee32(le, ptr, checked_32(len));
			uint64 count = 0;
            for (unsigned int loopc2=0; loopc2<me.table.size(); loopc2++)
            {
				if (count == 0)
				{
					count = me.table[loopc2];
#ifdef DEBUG_MTABLES
					printf(" ");
#endif
				}
				else
				{
					count--;
				}
#ifdef DEBUG_MTABLES
				printf("(%llx)[%ld]", ptr-orig, me.table[loopc2]);
#endif
                wee32(le, ptr, checked_32(me.table[loopc2]));
            }
            if (me.target)
            {
                MtableRelocation * mr = new MtableRelocation(i, me.target, ptr-orig);
                mr->add32();
#ifdef DEBUG_MTABLES
				printf(" (%llx)ptr\n", ptr-orig);
#endif
                wee32(le, ptr, 0xfeedbeef);
            }
            else
            {
#ifdef DEBUG_MTABLES
				printf(" (%llx)end\n", ptr-orig);
#endif
                wee32(le, ptr, 0x0);
            }                
        }   
    }
}
