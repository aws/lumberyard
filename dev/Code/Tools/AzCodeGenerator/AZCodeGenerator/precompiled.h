/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#if defined(AZCG_COMPILER_MSVC)
#pragma once

// We want NO warnings because we run with all warnings and warnings as errors. We are not modifying Clang to make our builds succeed in this configuration.

// Disable all common warnings
#pragma warning(push, 0)

// Visual C compiler refuses to blanket suppress all warnings. Below is a list of them that must be explicitly disabled for clang to stop throwing warnings and erroring our build.

// 4702 - Unreachable code warning
#pragma warning(disable: 4702)

#elif defined(AZCG_COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

#endif

#include "clang/AST/Stmt.h"
#include "clang/AST/Type.h"
#include "clang/AST/Decl.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ExternalASTSource.h"
#include "clang/AST/RecursiveASTVisitor.h"

#include "clang/Basic/Diagnostic.h"

#include "clang/Driver/Compilation.h"

#include "clang/Frontend/FrontendAction.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"

#include "clang/Lex/Preprocessor.h"

#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/CommonOptionsParser.h"

#include "llvm/Support/CommandLine.h"

#include <unordered_map>
#include <functional>

#if defined(AZCG_COMPILER_MSVC)
#pragma warning (pop)
#elif defined(AZCG_COMPILER_CLANG)
#pragma clang diagnostic pop
#endif
