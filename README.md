# The Yana Programming Language

Yana is a simple yet fast programming language, supporting both functional and procedural styles.
This repository contains the Yana compiler, standard library and documentation.

## Getting started

Read the [tutorial](https://rimmer.se/yana) or the language reference (coming soon).

## Native target

Yana can be compiled to an efficient native executable. The compiler currently uses LLVM for code generation, allowing it to support many different platforms and advanced optimisations.

## Javascript target

Yana can also be compiled to Javascript, and was designed have an efficient mapping to the language. This means that the generated script is both fast and doesn't require downloading a huge runtime.

## YouPic Blockchain target

This compiler version contains an additional target that allows compiled code to run within YouPic Blockchain. The target consists of a modified Yana IL that supports more efficient execution, as well as a different standard library specific for blockchain operation.
