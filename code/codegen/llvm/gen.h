#pragma once

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include "../../compiler/context.h"
#include "../../resolve/module.h"

struct LLVMTypes {
    llvm::Type* stringData;
    llvm::Type* stringRef;
};

struct Gen {
    llvm::LLVMContext* llvm;
    llvm::Module* module;
    llvm::IRBuilder<>* builder;
    LLVMTypes types;
    Arena* mem;

    Context* context;
};

llvm::Module* genModule(llvm::LLVMContext* llvm, Context* context, Module* module);
