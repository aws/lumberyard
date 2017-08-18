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
#include "Namespace.h"

namespace CodeGenerator
{
    class IWriter;

    //! Fields are class member variables.
    template <typename T>
    class Enum
        : public Declaration<T>
    {
    public:
        using Declaration<T>::WriteNameType;
        bool Write(T* decl, IWriter* writer) override
        {
            if (clang::EnumDecl* enumDecl = clang::dyn_cast<clang::EnumDecl>(decl))
            {
                writer->Begin();
                {
                    this->WriteName(enumDecl, writer);

                    Annotation<T> annotation;
                    annotation.Write(enumDecl, writer);

                    writer->WriteString("type");
                    writer->WriteString("enum");

                    writer->WriteString("elements");
                    writer->BeginArray();

                    for (auto it = enumDecl->enumerator_begin(); it != enumDecl->enumerator_end(); ++it)
                    {
                        writer->WriteString(it->getNameAsString());
                    }

                    writer->EndArray();
                }
                writer->End();
            }

            return true;
        }
    };
}
