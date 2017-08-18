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
#pragma once

#include "Consumer.h"

namespace CodeGenerator
{
    class IWriter;

    //! Used to format and print fully processed diagnostics.
    class MocDiagConsumer
        : public clang::DiagnosticConsumer
    {
    public:
        MocDiagConsumer(clang::DiagnosticConsumer* Previous);
        MocDiagConsumer(std::unique_ptr<clang::DiagnosticConsumer> previousConsumer);

#if CLANG_VERSION_MAJOR == 3 && CLANG_VERSION_MINOR < 2
        DiagnosticConsumer* clone(clang::DiagnosticsEngine& Diags) const override
        {
            return new MocDiagConsumer {
                       Proxy->clone(Diags)
            };
        }
#endif
        void BeginSourceFile(const clang::LangOptions& LangOpts, const clang::Preprocessor* PP = nullptr) override;
        void EndSourceFile() override;

        void HandleDiagnostic(clang::DiagnosticsEngine::Level DiagLevel, const clang::Diagnostic& Info) override;

        void finish() override;
        void clear() override;

    private:
        std::unique_ptr<clang::DiagnosticConsumer> Proxy;
    };

    //! Front-end action, will invoke our consumer.
    class Action
        : public clang::SyntaxOnlyAction
    {
    public:
        Action(IWriter* writer);

        bool hasCodeCompletionSupport() const override;
        std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance& Compiler, llvm::StringRef /*InFile*/) override;

    private:
        IWriter* m_writer;
    };
}