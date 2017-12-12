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

#include "Writer.h"

namespace CodeGenerator
{
    class IWriter;

    //! Base class for different declaration, provides some shared functionality, expects declarations to implement
    //! the Write function.
    template <typename T>
    class Declaration
    {
    public:
        virtual bool Write(T* decl, IWriter* writer) = 0;

    protected:

        virtual void WriteNameType(T* decl, IWriter* writer)
        {
            WriteName(decl, writer);
            WriteType(decl, writer);
        }

        virtual void WriteName(T* decl, IWriter* writer)
        {
            writer->WriteString("name");
            writer->WriteString(GetNameForDecl(decl, false).c_str());
            writer->WriteString("qualified_name");
            writer->WriteString(GetNameForDecl(decl, true).c_str());
        }

        std::string GetTemplateArgumentAsString(const clang::TemplateArgument& templateArgument, bool useQualifiedName, clang::ASTContext& astContext)
        {
            switch (templateArgument.getKind())
            {
            case clang::TemplateArgument::ArgKind::Type:
                if (templateArgument.getAsType()->getAsCXXRecordDecl())
                {
                    return GetNameForDecl(templateArgument.getAsType()->getAsCXXRecordDecl(), useQualifiedName);
                }
                return templateArgument.getAsType().getAsString();

            case clang::TemplateArgument::ArgKind::Declaration:
                return GetNameForDecl(templateArgument.getAsDecl(), useQualifiedName);

            case clang::TemplateArgument::ArgKind::NullPtr:
                return "nullptr";

            case clang::TemplateArgument::ArgKind::Integral:
                return templateArgument.getAsIntegral().toString(10);

            case clang::TemplateArgument::ArgKind::Template:
            case clang::TemplateArgument::ArgKind::TemplateExpansion:
                return useQualifiedName ? templateArgument.getAsTemplate().getAsTemplateDecl()->getQualifiedNameAsString() : templateArgument.getAsTemplate().getAsTemplateDecl()->getNameAsString();

            case clang::TemplateArgument::ArgKind::Expression:
                return templateArgument.getAsExpr()->getExprLoc().printToString(astContext.getSourceManager());

            case clang::TemplateArgument::ArgKind::Pack:
                return GetTemplateArgumentsArrayAsString(templateArgument.getPackAsArray(), useQualifiedName, astContext);
                
            case clang::TemplateArgument::ArgKind::Null:
            default:
                return "<Unknown>";
            }
        }

        std::string GetTemplateArgumentsArrayAsString(const llvm::ArrayRef<clang::TemplateArgument>& templateArguments, bool useQualifiedName, clang::ASTContext& astContext)
        {
            std::string templateString = "";
            for (const auto& templateArgument : templateArguments)
            {
                templateString.append(GetTemplateArgumentAsString(templateArgument, useQualifiedName, astContext));
                templateString.push_back(',');
            }

            // Remove that last comma if there were arguments
            if (templateArguments.size())
            {
                templateString.pop_back();
            }

            return templateString;
        }

        template <class DeclType>
        std::string GetNameForDecl(DeclType* decl, bool useQualifiedName)
        {
            std::string name = useQualifiedName ? decl->getQualifiedNameAsString() : decl->getNameAsString();
            if (!clang::isa<clang::ClassTemplateSpecializationDecl>(decl))
            {
                // If this isn't a template specialization, the name is all we need
                return name;
            }

            // Template specializations are special and should output the full template arg list with the name
            clang::ClassTemplateSpecializationDecl* templateDecl = clang::cast<clang::ClassTemplateSpecializationDecl>(decl);
            name.push_back('<');
            name.append(GetTemplateArgumentsArrayAsString(templateDecl->getTemplateArgs().asArray(), useQualifiedName, decl->getASTContext()));
            name.push_back('>');
        
            return name;
        }

        virtual void WriteType(T* /*decl*/, IWriter* /*writer*/)
        {
        }
    };
}