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
#include "precompiled.h"
#include "Action.h"
#include "Configuration.h"

#include <memory>

namespace CodeGenerator
{
    namespace Configuration
    {
        using namespace llvm;

        // AST Traversal settings
        static cl::OptionCategory ASTTraversalCategory("AST Traversal Settings");
        static cl::opt<bool> SkipFunctionBodies("SkipFunctionBodies", cl::desc("Do not traverse into function bodies."), cl::cat(ASTTraversalCategory), cl::Optional, cl::init(true));
        static cl::opt<bool> EnableIncrementalProcessing("EnableIncrementalProcessing", cl::desc("Enables the incremental processing."), cl::cat(ASTTraversalCategory), cl::Optional, cl::init(true));
        static cl::opt<bool> SuppressIncludeNotFoundError("SuppressIncludeNotFoundError", cl::desc("Suppresses #include not found errors"), cl::cat(ASTTraversalCategory), cl::Optional, cl::init(true));
        static cl::opt<bool> DelayedTemplateParsing("DelayedTemplateParsing", cl::desc("Template tokens are consumed and stored for parsing at the end of the translation unit."), cl::cat(ASTTraversalCategory), cl::Optional, cl::init(true));

        // Clang Compilation Settings (Not the same as actual compiler settings)
        static cl::OptionCategory CompilationCategory("Clang Compilation Settings");
        static cl::opt<bool> SuppressDiagnostics("SuppressDiagnostics", cl::desc("Hides clang compilation diagnostic information."), cl::cat(CompilationCategory), cl::Optional, cl::init(true));
        static cl::opt<bool> SuppressErrorsAsWarnings("SuppressErrorsAsWarnings", cl::desc("Suppress compilation errors during parsing as warnings"), cl::cat(CompilationCategory), cl::Optional, cl::init(true));
        static cl::opt<bool> OnlyRunDiagnosticsOnMainFile("OnlyRunDiagnosticsOnMainFile", cl::desc("Only runs diagnostics (error and warning checking) on the main file being compiled, ignores errors and warnings from all other files"), cl::cat(CompilationCategory), cl::Optional, cl::init(true));
    }

    class PPCallbacks
        : public clang::PPCallbacks
    {
    public:
        void InclusionDirective(clang::SourceLocation HashLoc, const clang::Token &IncludeTok, clang::StringRef FileName, bool IsAngled, 
            clang::CharSourceRange FilenameRange, const clang::FileEntry *File, clang::StringRef SearchPath, clang::StringRef RelativePath, const clang::Module *Imported) override
        {
            if (File)
            {
                std::string filename = File->getName();
                std::replace(filename.begin(), filename.end(), '\\', '/');
                Output::DependencyFile(filename);
            }
        }
    };

    MocDiagConsumer::MocDiagConsumer(clang::DiagnosticConsumer* Previous)
        : Proxy(Previous)
    {
    }

    MocDiagConsumer::MocDiagConsumer(std::unique_ptr<clang::DiagnosticConsumer> previousConsumer)
    {
        Proxy = std::move(previousConsumer);
    }

    void MocDiagConsumer::BeginSourceFile(const clang::LangOptions& LangOpts, const clang::Preprocessor* PP /*= nullptr*/)
    {
        Proxy->BeginSourceFile(LangOpts, PP);
    }

    void MocDiagConsumer::clear()
    {
        Proxy->clear();
    }

    void MocDiagConsumer::EndSourceFile()
    {
        Proxy->EndSourceFile();
    }

    void MocDiagConsumer::finish()
    {
        Proxy->finish();
    }

    void MocDiagConsumer::HandleDiagnostic(clang::DiagnosticsEngine::Level DiagLevel, const clang::Diagnostic& Info)
    {
        bool ignoreDiagnostic = Configuration::SuppressDiagnostics;

        if (Configuration::OnlyRunDiagnosticsOnMainFile && !Info.getSourceManager().isInMainFile(Info.getLocation()))
        {
            // Ignore anything outside of the main file
            ignoreDiagnostic = true;
        }

        if (!ignoreDiagnostic)
        {
            if (Configuration::SuppressErrorsAsWarnings)
            {
                if (DiagLevel >= clang::DiagnosticsEngine::Error)
                {
                    DiagLevel = clang::DiagnosticsEngine::Warning;
                }
            }

            clang::DiagnosticConsumer::HandleDiagnostic(DiagLevel, Info);
            Proxy->HandleDiagnostic(DiagLevel, Info);
        }

        // This prevents errors/warnings from halting further compilation
        const_cast<clang::DiagnosticsEngine*>(Info.getDiags())->Reset();
    }

    Action::Action(IWriter* writer)
        : m_writer(writer)
    {
    }

    bool Action::hasCodeCompletionSupport() const
    {
        return false;
    }

    std::unique_ptr<clang::ASTConsumer> Action::CreateASTConsumer(clang::CompilerInstance& Compiler, llvm::StringRef /*InFile*/)
    {
        Compiler.getPreprocessor().enableIncrementalProcessing(Configuration::EnableIncrementalProcessing);
        Compiler.getPreprocessor().SetSuppressIncludeNotFoundError(Configuration::SuppressIncludeNotFoundError);
        Compiler.getPreprocessor().addPPCallbacks(std::make_unique<PPCallbacks>());
        Compiler.getFrontendOpts().SkipFunctionBodies = Configuration::SkipFunctionBodies;
        Compiler.getLangOpts().DelayedTemplateParsing = Configuration::DelayedTemplateParsing;

        Compiler.getDiagnostics().setClient(new MocDiagConsumer {Compiler.getDiagnostics().takeClient()});

        return std::unique_ptr<clang::ASTConsumer>(new Consumer(m_writer));
    }
}