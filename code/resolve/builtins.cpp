
#include "module.h"

typedef Value* (*BinIntrinsic)(Block*, Id, Value*, Value*);

template<BinIntrinsic F>
static Function* binaryFunction(Context* context, Module* module, Type* type, const char* name, U32 length, Type* returnType = nullptr) {
    auto fun = defineFun(context, module, context->addUnqualifiedName(name, length));
    auto lhs = defineArg(context, fun, 0, type);
    auto rhs = defineArg(context, fun, 0, type);
    fun->returnType = returnType ? returnType : type;

    auto body = block(fun);
    auto result = F(body, 0, lhs, rhs);
    ret(body, result);

    fun->intrinsic = [](FunBuilder* b, Value** args, U32 count, Id instName) -> Value* {
        return F(b->block, instName, args[0], args[1]);
    };

    return fun;
}

template<class Cmp>
using CmpIntrinsic = Value* (*)(Block*, Id, Value*, Value*, Cmp);

template<class Cmp, CmpIntrinsic<Cmp> F, Cmp cmp>
static Function* cmpFunction(Context* context, Module* module, Type* type, const char* name, U32 length) {
    auto fun = defineFun(context, module, context->addUnqualifiedName(name, length));
    auto lhs = defineArg(context, fun, 0, type);
    auto rhs = defineArg(context, fun, 0, type);
    fun->returnType = &intTypes[IntType::Bool];

    auto body = block(fun);
    auto result = F(body, 0, lhs, rhs, cmp);
    ret(body, result);

    fun->intrinsic = [](FunBuilder* b, Value** args, U32 count, Id instName) -> Value* {
        return F(b->block, instName, args[0], args[1], cmp);
    };

    return fun;
}

Module* preludeModule(Context* context) {
    auto module = new Module;
    module->id = context->addUnqualifiedName("Prelude", 7);
    module->name = &context->find(module->id);

    // Define basic operators.
    auto opEq = context->addUnqualifiedName("==", 2);
    auto opNeq = context->addUnqualifiedName("!=", 2);
    auto opLt = context->addUnqualifiedName("<", 1);
    auto opLe = context->addUnqualifiedName("<=", 2);
    auto opGt = context->addUnqualifiedName(">", 1);
    auto opGe = context->addUnqualifiedName(">=", 2);

    auto opPlus = context->addUnqualifiedName("+", 1);
    auto opMinus = context->addUnqualifiedName("-", 1);
    auto opMul = context->addUnqualifiedName("*", 1);
    auto opDiv = context->addUnqualifiedName("/", 1);
    auto opRem = context->addUnqualifiedName("rem", 3);

    auto opAnd = context->addUnqualifiedName("and", 3);
    auto opOr = context->addUnqualifiedName("or", 2);
    auto opXor = context->addUnqualifiedName("xor", 3);
    auto opShl = context->addUnqualifiedName("shl", 3);
    auto opShr = context->addUnqualifiedName("shr", 3);

    module->ops.add(opEq, OpProperties{4, Assoc::Left});
    module->ops.add(opNeq, OpProperties{4, Assoc::Left});
    module->ops.add(opLt, OpProperties{4, Assoc::Left});
    module->ops.add(opLe, OpProperties{4, Assoc::Left});
    module->ops.add(opGt, OpProperties{4, Assoc::Left});
    module->ops.add(opGe, OpProperties{4, Assoc::Left});

    module->ops.add(opPlus, OpProperties{6, Assoc::Left});
    module->ops.add(opMinus, OpProperties{6, Assoc::Left});
    module->ops.add(opMul, OpProperties{7, Assoc::Left});
    module->ops.add(opDiv, OpProperties{7, Assoc::Left});
    module->ops.add(opRem, OpProperties{7, Assoc::Left});

    module->ops.add(opAnd, OpProperties{7, Assoc::Left});
    module->ops.add(opOr, OpProperties{5, Assoc::Left});
    module->ops.add(opXor, OpProperties{6, Assoc::Left});
    module->ops.add(opShl, OpProperties{8, Assoc::Left});
    module->ops.add(opShr, OpProperties{8, Assoc::Left});

    // Define the basic types.
    module->types.add(context->addUnqualifiedName("Bool", 4), &intTypes[IntType::Bool]);
    module->types.add(context->addUnqualifiedName("Int", 3), &intTypes[IntType::Int]);
    module->types.add(context->addUnqualifiedName("Long", 4), &intTypes[IntType::Long]);
    module->types.add(context->addUnqualifiedName("Float", 5), &floatTypes[FloatType::F32]);
    module->types.add(context->addUnqualifiedName("Double", 6), &floatTypes[FloatType::F64]);
    module->types.add(context->addUnqualifiedName("String", 6), &stringType);

    auto orderingType = defineRecord(context, module, context->addUnqualifiedName("Ordering", 8), 3, false);
    orderingType->kind = RecordType::Enum;

    auto ltCon = defineCon(context, module, orderingType, context->addUnqualifiedName("LT", 2), 0, nullptr, 0);
    auto eqCon = defineCon(context, module, orderingType, context->addUnqualifiedName("EQ", 2), 1, nullptr, 0);
    auto gtCon = defineCon(context, module, orderingType, context->addUnqualifiedName("GT", 2), 2, nullptr, 0);

    auto eqClass = defineClass(context, module, context->addUnqualifiedName("Eq", 2));
    {
        auto t = new (module->memory) GenType(0);
    }

    cmpFunction<ICmp, icmp, ICmp::eq>(context, module, &intTypes[IntType::Long], "==", 2);
    cmpFunction<ICmp, icmp, ICmp::neq>(context, module, &intTypes[IntType::Long], "!=", 2);
    cmpFunction<ICmp, icmp, ICmp::gt>(context, module, &intTypes[IntType::Long], ">", 1);
    cmpFunction<ICmp, icmp, ICmp::ge>(context, module, &intTypes[IntType::Long], ">=", 2);
    cmpFunction<ICmp, icmp, ICmp::lt>(context, module, &intTypes[IntType::Long], "<", 1);
    cmpFunction<ICmp, icmp, ICmp::le>(context, module, &intTypes[IntType::Long], "<=", 2);

    binaryFunction<add>(context, module, &intTypes[IntType::Long], "+", 1);
    binaryFunction<sub>(context, module, &intTypes[IntType::Long], "-", 1);
    binaryFunction<mul>(context, module, &intTypes[IntType::Long], "*", 1);
    binaryFunction<div>(context, module, &intTypes[IntType::Long], "/", 1);
    binaryFunction<rem>(context, module, &intTypes[IntType::Long], "rem", 3);

    binaryFunction<sar>(context, module, &intTypes[IntType::Long], "sar", 3);
    binaryFunction<shr>(context, module, &intTypes[IntType::Long], "shr", 4);
    binaryFunction<shl>(context, module, &intTypes[IntType::Long], "shl", 3);

    binaryFunction<and_>(context, module, &intTypes[IntType::Long], "and", 2);
    binaryFunction<or_>(context, module, &intTypes[IntType::Long], "or", 2);
    binaryFunction<xor_>(context, module, &intTypes[IntType::Long], "xor", 2);

    return module;
}

Module* unsafeModule(Context* context, Module* prelude) {
    auto module = new Module;
    module->id = context->addUnqualifiedName("Unsafe", 6);
    module->name = &context->find(module->id);


    return module;
}