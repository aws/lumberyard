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

#include "Declaration.h"

namespace CodeGenerator
{
    class IWriter;

    //! Writes out member functions.
    template <typename T>
    class Method
        : public Declaration<T>
    {
    public:
        using Declaration<T>::WriteName;
        bool Write(T* decl, IWriter* writer) override
        {
            // Don't support deleted or implicit functions
            clang::FunctionDecl* functionDecl = decl;
            if (functionDecl->isDeleted()
                || functionDecl->isImplicit())
            {
                return true;
            }

            // Write out list of function parameters
            writer->Begin();
            {
                WriteName(decl, writer);

                clang::QualType returnType = functionDecl->getReturnType();
                writer->WriteString("return_type");
                writer->WriteString(returnType.getAsString().c_str());

                writer->WriteString("is_virtual");
                writer->WriteBool(functionDecl->isVirtualAsWritten() || functionDecl->isPure() || functionDecl->hasInheritedPrototype());

                writer->WriteString("uses_override");
                writer->WriteBool(functionDecl->getAttr<clang::OverrideAttr>() != nullptr);

                writer->WriteString("access");
                if (functionDecl->getAccess() == clang::AccessSpecifier::AS_public)
                {
                    writer->WriteString("public");
                }
                else if (functionDecl->getAccess() == clang::AccessSpecifier::AS_protected)
                {
                    writer->WriteString("protected");
                }
                else if (functionDecl->getAccess() == clang::AccessSpecifier::AS_private ||
                    functionDecl->getAccess() == clang::AccessSpecifier::AS_none)
                {
                    writer->WriteString("private");
                }

                if (functionDecl->getNumParams() > 0)
                {
                    writer->WriteString("params");
                    writer->BeginArray();
                    for (unsigned int i = 0; i < functionDecl->getNumParams(); ++i)
                    {
                        clang::ParmVarDecl* paramDecl = functionDecl->getParamDecl(i);

                        writer->Begin();
                        {
                            writer->WriteString("name");
                            writer->WriteString(paramDecl->getNameAsString());
                            writer->WriteString("type");
                            writer->WriteString(paramDecl->getType().getAsString());
                            writer->WriteString("canonical_type");
                            writer->WriteString(paramDecl->getType().getCanonicalType().getAsString());
                        }
                        writer->End();
                    }
                    writer->EndArray();
                }

                Annotation<T> annotation;
                annotation.Write(decl, writer);
            }
            writer->End();

            return true;
        }

        void WriteType(T* decl, IWriter* writer) override
        {
            writer->WriteString("type");
            writer->WriteString(decl->getType().getAsString());
            writer->WriteString("canonical_type");
            writer->WriteString(decl->getType().getCanonicalType().getAsString());
        }
    };
}