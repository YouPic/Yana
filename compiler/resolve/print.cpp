#include "print.h"

void printValue(std::ostream& stream, Context& context, const Value* value) {
    if(value->kind == Value::Arg || value->kind == Value::Global || value->kind >= Value::FirstInst) {
        stream << '%';

        auto name = context.find(value->name);
        if(name.textLength > 0) {
            stream.write(name.text, name.textLength);
        } else if(value->kind >= Value::FirstInst) {
            stream << value->id;
        } else if(value->kind == Value::Arg) {
            stream << 'a';
            stream << ((Arg*)value)->index;
        } else {
            stream << "<unnamed>";
        }
    } else if(value->kind == Value::ConstInt) {
        stream << ((ConstInt*)value)->value;
        stream << ": ";
        printType(stream, context, value->type);
    } else if(value->kind == Value::ConstFloat) {
        stream << ((ConstFloat*)value)->value;
        stream << ": ";
        printType(stream, context, value->type);
    } else if(value->kind == Value::ConstString) {
        auto string = ((ConstString*)value);
        stream << '"';
        stream.write(string->value, string->length);
        stream << '"';
    }
}

void printType(std::ostream& stream, Context& context, const Type* type) {
    switch(type->kind) {
        case Type::Unit:
            stream << "void";
            break;
        case Type::Error:
            stream << "<type error>";
            break;
        case Type::Int:
            stream << 'i';
            stream << ((IntType*)type)->bits;
            break;
        case Type::Float:
            stream << 'f';
            stream << ((FloatType*)type)->bits;
            break;
        case Type::String:
            stream << "String";
            break;
        case Type::Ref:
            stream << '*';
            printType(stream, context, ((RefType*)type)->to);
            break;
        case Type::Array:
            stream << '[';
            printType(stream, context, ((ArrayType*)type)->content);
            stream << ']';
            break;
        case Type::Map:
            stream << '[';
            printType(stream, context, ((MapType*)type)->from);
            stream << " -> ";
            printType(stream, context, ((MapType*)type)->to);
            stream << ']';
            break;
        case Type::Record: {
            auto record = (RecordType*)type;
            auto name = context.find(record->name);
            if(name.textLength > 0) {
                stream.write(name.text, name.textLength);
            }

            if(record->instanceOf) {
                stream << '(';
                for(Size i = 0; i < record->instanceOf->argCount; i++) {
                    printType(stream, context, record->instance[i]);
                    if(i < record->instanceOf->argCount - 1) {
                        stream << ", ";
                    }
                }
                stream << ')';
            }
            break;
        }
        case Type::Tup: {
            auto tup = (TupType*)type;
            stream << '{';
            for(Size i = 0; i < tup->count; i++) {
                printType(stream, context, tup->fields[i].type);
                if(i < tup->count - 1) {
                    stream << ", ";
                }
            }
            stream << '}';
            break;
        }
        case Type::Gen:
            stream << '\'';
            stream << ((GenType*)type)->index;
            break;
        case Type::Fun: {
            auto fun = (FunType*)type;
            stream << '(';
            for(Size i = 0; i < fun->argCount; i++) {
                printType(stream, context, fun->args[i].type);
                if(i < fun->argCount - 1) {
                    stream << ", ";
                }
            }
            stream << ") -> ";
            printType(stream, context, fun->result);
            break;
        }
        case Type::Alias: {
            printType(stream, context, ((AliasType*)type)->to);
            break;
        }
    }
}

void printBlockName(std::ostream& stream, const Block* block) {
    Size index = 0;
    for(Size i = 0; i < block->function->blocks.size(); i++) {
        if(block->function->blocks[i] == block) break;
        index++;
    }

    stream << '#';
    stream << index;
}

void printBlock(std::ostream& stream, Context& context, const Block* block) {
    printBlockName(stream, block);
    stream << ":\n";

    for(auto inst: block->instructions) {
        printInst(stream, context, inst);
    }
}

void printGlobal(std::ostream& stream, Context& context, const Global* global) {
    stream << "global ";
    auto name = context.find(global->name);
    if(name.textLength > 0) {
        stream << '%';
        stream.write(name.text, name.textLength);
    } else {
        stream << "<unnamed>";
    }

    stream << ": ";
    printType(stream, context, global->type);
    stream << '\n';
}

void printTypeClass(std::ostream& stream, Context& context, const InstanceMap* map) {
    for(U32 i = 0; i < map->instances.size(); i++) {
        stream << "instance ";

        ClassInstance* instance = map->instances[i];
        auto argCount = instance->typeClass->argCount;
        auto funCount = instance->typeClass->funCount;

        auto name = context.find(instance->typeClass->name);
        if(name.textLength > 0) {
            stream.write(name.text, name.textLength);
        } else {
            stream << "<unnamed>";
        }

        stream << '(';
        for(U32 j = 0; j < argCount; j++) {
            printType(stream, context, instance->forTypes[j]);
            if(j < argCount - 1) {
                stream << ", ";
            }
        }
        stream << ")\n";

        for(U32 j = 0; j < funCount; j++) {
            printFunction(stream, context, instance->instances[j], instance->typeClass->functions[j].name);
            stream << '\n';
        }

        stream << "end instance\n";
    }
}

void printModule(std::ostream& stream, Context& context, const Module* module) {
    for(auto& global: module->globals) {
        printGlobal(stream, context, &global);
    }

    stream << '\n';

    for(auto& fun: module->functions) {
        printFunction(stream, context, &fun);
        stream << '\n';
    }

    for(auto& list: module->typeInstances) {
        for(auto& fun: list->functions) {
            printFunction(stream, context, &fun);
            stream << '\n';
        }
    }

    for(const InstanceMap& instance: module->classInstances) {
        printTypeClass(stream, context, &instance);
        stream << '\n';
    }
}

void printGenEnv(std::ostream& stream, Context& context, const GenEnv* env) {
    if(env->typeCount > 0) {
        stream << '[';
        for(Size i = 0; i < env->typeCount; i++) {
            printType(stream, context, env->types[i]);
            if(i < env->typeCount - 1) {
                stream << ", ";
            }
        }

        if(env->classCount > 0) {
            stream << ", ";
            stream << '{';

            for(Size i = 0; i < env->classCount; i++) {
                auto c = env->classes[i];
                auto name = context.find(c.classType->name);
                if(name.textLength > 0) {
                    stream.write(name.text, name.textLength);
                } else {
                    stream << "<unnamed>";
                }

                stream << '(';
                for(Size j = 0; j < c.forTypes.length; j++) {
                    printType(stream, context, c.forTypes.ptr[j]);
                    if(j < c.forTypes.length - 1) {
                        stream << ", ";
                    }
                }
                stream << ')';

                if(i < env->classCount - 1) {
                    stream << ", ";
                }
            }

            stream << '}';
        }

        if(env->funCount > 0) {
            stream << ", ";
            stream << '{';

            for(Size i = 0; i < env->funCount; i++) {
                auto fun = env->funs[i];
                auto name = context.find(fun.name);
                if(name.textLength > 0) {
                    stream.write(name.text, name.textLength);
                } else {
                    stream << "<unnamed>";
                }

                stream << ": ";
                printType(stream, context, fun.type);

                if(i < env->funCount - 1) {
                    stream << ", ";
                }
            }

            stream << '}';
        }

        if(env->fieldCount > 0) {
            stream << ", ";
            stream << '{';

            for(Size i = 0; i < env->fieldCount; i++) {
                auto field = env->fields[i];
                printType(stream, context, field.container);
                stream << '.';

                auto name = context.find(field.fieldName);
                if(name.textLength > 0) {
                    stream.write(name.text, name.textLength);
                } else {
                    stream << "<unnamed>";
                }

                stream << ": ";
                printType(stream, context, field.fieldType);

                if(i < env->fieldCount - 1) {
                    stream << ", ";
                }
            }

            stream << '}';
        }

        stream << ']';
    }
}

void printFunction(std::ostream& stream, Context& context, const Function* fun, Id forceName) {
    stream << "fn ";
    auto name = context.find(forceName ? forceName : fun->name);
    if(name.textLength > 0) {
        stream.write(name.text, name.textLength);
    } else {
        stream << "<unnamed>";
    }

    printGenEnv(stream, context, &fun->gen);

    stream << '(';
    for(Size i = 0; i < fun->args.size(); i++) {
        printValue(stream, context, fun->args[i]);
        stream << ": ";
        printType(stream, context, fun->args[i]->type);
        if(i < fun->args.size() - 1) {
            stream << ", ";
        }
    }
    stream << ") -> ";
    printType(stream, context, fun->returnType);
    stream << " {\n";

    for(auto& block : fun->blocks) {
        printBlock(stream, context, block);
    }

    stream << "}\n";
}

void printInst(std::ostream& stream, Context& context, const Inst* inst) {
    stream << "  ";

    const char* name = "";
    switch(inst->kind) {
        case Inst::InstNop:
            name = "nop";
            break;
        case Inst::InstTrunc:
            name = "trunc";
            break;
        case Inst::InstFTrunc:
            name = "truncf";
            break;
        case Inst::InstZExt:
            name = "zext";
            break;
        case Inst::InstSExt:
            name = "sext";
            break;
        case Inst::InstFExt:
            name = "fext";
            break;
        case Inst::InstFToI:
            name = "ftoi";
            break;
        case Inst::InstFToUI:
            name = "ftoui";
            break;
        case Inst::InstIToF:
            name = "itof";
            break;
        case Inst::InstUIToF:
            name = "uitof";
            break;
        case Inst::InstAdd:
            name = "add";
            break;
        case Inst::InstSub:
            name = "sub";
            break;
        case Inst::InstMul:
            name = "mul";
            break;
        case Inst::InstDiv:
            name = "div";
            break;
        case Inst::InstIDiv:
            name = "idiv";
            break;
        case Inst::InstRem:
            name = "rem";
            break;
        case Inst::InstIRem:
            name = "irem";
            break;
        case Inst::InstFAdd:
            name = "fadd";
            break;
        case Inst::InstFSub:
            name = "fsub";
            break;
        case Inst::InstFMul:
            name = "fmul";
            break;
        case Inst::InstFDiv:
            name = "fdiv";
            break;
        case Inst::InstICmp:
            name = "icmp";
            break;
        case Inst::InstFCmp:
            name = "fcmp";
            break;
        case Inst::InstShl:
            name = "shl";
            break;
        case Inst::InstShr:
            name = "shr";
            break;
        case Inst::InstSar:
            name = "sar";
            break;
        case Inst::InstAnd:
            name = "and";
            break;
        case Inst::InstOr:
            name = "or";
            break;
        case Inst::InstXor:
            name = "xor";
            break;
        case Inst::InstAddRef:
            name = "addref";
            break;
        case Inst::InstJe:
            name = "je";
            break;
        case Inst::InstRecord:
            name = "record";
            break;
        case Inst::InstTup:
            name = "tup";
            break;
        case Inst::InstFun:
            name = "fun";
            break;
        case Inst::InstAlloc:
            name = "alloc";
            break;
        case Inst::InstAllocArray:
            name = "allocarray";
            break;
        case Inst::InstLoad:
            name = "load";
            break;
        case Inst::InstLoadField:
            name = "loadfield";
            break;
        case Inst::InstLoadArray:
            name = "loadarray";
            break;
        case Inst::InstStore:
            name = "store";
            break;
        case Inst::InstStoreField:
            name = "storefield";
            break;
        case Inst::InstStoreArray:
            name = "storearray";
            break;
        case Inst::InstGetField:
            name = "getfield";
            break;
        case Inst::InstUpdateField:
            name = "updatefield";
            break;
        case Inst::InstArrayLength:
            name = "arraylength";
            break;
        case Inst::InstArrayCopy:
            name = "arraycopy";
            break;
        case Inst::InstArraySlice:
            name = "arrayslice";
            break;
        case Inst::InstStringLength:
            name = "stringlength";
            break;
        case Inst::InstStringData:
            name = "stringdata";
            break;
        case Inst::InstCall:
            name = "call";
            break;
        case Inst::InstCallDyn:
            name = "call dyn";
            break;
        case Inst::InstCallForeign:
            name = "call foreign";
            break;
        case Inst::InstJmp:
            name = "jmp";
            break;
        case Inst::InstRet:
            name = "ret";
            break;
        case Inst::InstPhi:
            name = "phi";
            break;
    }

    if(inst->type->kind != Type::Unit && inst->kind != Inst::InstRet) {
        printValue(stream, context, inst);
        stream << " = ";
    }

    stream << name;

    if(inst->kind == Inst::InstAlloc) {
        if(((InstAlloc*)inst)->mut) stream << "<mut>";
    }

    stream << '(';

    if(inst->kind == Inst::InstCall) {
        auto fun = context.find(((InstCall*)inst)->fun->name);
        if(fun.textLength > 0) {
            stream.write(fun.text, fun.textLength);
        } else {
            stream << "<unnamed>";
        }

        if(inst->usedCount > 0) {
            stream << ", ";
        }
    } else if(inst->kind == Inst::InstCallForeign) {
        auto fun = context.find(((InstCallForeign*)inst)->fun->name);
        if(fun.textLength > 0) {
            stream.write(fun.text, fun.textLength);
        } else {
            stream << "<unnamed>";
        }

        if(inst->usedCount > 0) {
            stream << ", ";
        }
    } else if(inst->kind == Inst::InstCallDyn) {
        if(((InstCallDyn*)inst)->isIntrinsic) {
            stream << "<intrinsic> ";
        }
    } else if(inst->kind == Inst::InstRecord) {
        auto record = (InstRecord*)inst;
        auto con = context.findName(record->con->name);
        stream.write(con.text(), con.size());

        if(inst->usedCount > 0) {
            stream << ", ";
        }
    }

    if(inst->kind == Inst::InstPhi) {
        auto phi = (InstPhi*)inst;
        for(U32 i = 0; i < phi->altCount; i++) {
            stream << '[';
            printValue(stream, context, phi->alts[i].value);
            stream << ", ";
            printBlockName(stream, phi->alts[i].fromBlock);
            stream << ']';

            if(i < phi->altCount - 1) {
                stream << ", ";
            }
        }
    } else if(inst->kind == Inst::InstJe) {
        auto je = (InstJe*)inst;
        stream << '[';
        printValue(stream, context, je->cond);
        stream << ", ";
        printBlockName(stream, je->then);
        stream << ", ";
        printBlockName(stream, je->otherwise);
        stream << ']';
    } else {
        for(U32 i = 0; i < inst->usedCount; i++) {
            printValue(stream, context, inst->usedValues[i]);
            if(i < inst->usedCount - 1) {
                stream << ", ";
            }
        }
    }

    if(inst->kind == Inst::InstJmp) {
        printBlockName(stream, ((const InstJmp*)inst)->to);
    } else if(inst->kind == Inst::InstGetField) {
        auto get = (InstGetField*)inst;
        for(Size i = 0; i < get->chainLength; i++) {
            stream << ", ";
            stream << get->indexChain[i];
        }
    } else if(inst->kind == Inst::InstLoadField) {
        auto get = (InstLoadField*)inst;
        for(Size i = 0; i < get->chainLength; i++) {
            stream << ", ";
            stream << get->indexChain[i];
        }
    }

    stream << "): ";
    printType(stream, context, inst->type);
    stream << '\n';
}