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
#include "Writer.h"
#include "Annotation.h"
#include "Field.h"
#include "Method.h"
#include "Namespace.h"

namespace CodeGenerator
{
    class IWriter;

    //! Object refers to either a class or a struct.
    //! This class will write out all the object's details: base classes, fields, methods, etc.
    template <typename T>
    class Object
        : public Declaration<T>
    {
    public:
        using Declaration<T>::WriteNameType;
        using Declaration<T>::WriteName;
        bool Write(T* decl, IWriter* writer) override
        {
            writer->Begin();
            {
                WriteNameType(decl, writer);

                Annotation<T> annotation;
                annotation.Write(decl, writer);

                Namespace<T> namespaces;
                namespaces.Write(decl, writer);

                // Base objects
                writer->WriteString("bases");
                writer->BeginArray();

                for (auto baseIt = decl->bases_begin(); baseIt != decl->bases_end(); ++baseIt)
                {
                    clang::CXXRecordDecl* baseDecl = baseIt->getType()->getAsCXXRecordDecl();
                    if (baseDecl)
                    {
                        // Store both the class name and the qualified name.
                        writer->Begin();
                        WriteName(baseDecl, writer);
                        writer->End();
                    }
                }
                writer->EndArray();

                // Fields
                writer->WriteString("fields");
                writer->BeginArray();
                {
                    for (auto fieldIt = decl->field_begin(); fieldIt != decl->field_end(); ++fieldIt)
                    {
                        Field<clang::FieldDecl> field;
                        field.Write(*fieldIt, writer);
                    }
                }
                writer->EndArray();

                // Methods
                writer->WriteString("methods");
                writer->BeginArray();
                {
                    for (auto methodIt = decl->method_begin(); methodIt != decl->method_end(); ++methodIt)
                    {
                        Method<clang::CXXMethodDecl> method;
                        method.Write(*methodIt, writer);
                    }
                }
                writer->EndArray();

                // Traits
                writer->WriteString("traits");
                writer->Begin();
                {
                    writer->WriteString("isAbstract");
                    writer->WriteBool(decl->isAbstract());
                    writer->WriteString("isPolymorphic");
                    writer->WriteBool(decl->isPolymorphic());
                    writer->WriteString("isPOD");
                    writer->WriteBool(decl->isPOD());
                }
                writer->End();

                // Meta
                writer->WriteString("meta");
                writer->Begin();
                {
                    writer->WriteString("path");
                    auto fileID = decl->getASTContext().getSourceManager().getFileID(decl->getLocation());
                    auto fileEntry = decl->getASTContext().getSourceManager().getFileEntryForID(fileID);
                    writer->WriteString(fileEntry ? fileEntry->getName() : "");
                }
                writer->End();
            }
            writer->End();

            return true;
        }

    protected:

        void WriteType(T* decl, IWriter* writer) override
        {
            writer->WriteString("type");
            writer->WriteString(decl->isClass() ? "class" : "struct");
        }
    };
}