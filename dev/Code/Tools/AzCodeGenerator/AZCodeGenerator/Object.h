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
#include "Output.h"

namespace CodeGenerator
{
    class IWriter;

    template <typename DeclType>
    bool ContainsIgnoreAnnotation(DeclType* decl)
    {
        bool shouldBeIgnored = false;
        bool hasOtherAnnotations = false;
        for (auto attrIt = decl->template specific_attr_begin<clang::AnnotateAttr>(); attrIt != decl->template specific_attr_end<clang::AnnotateAttr>(); ++attrIt)
        {
            const clang::AnnotateAttr* attr = clang::dyn_cast<clang::AnnotateAttr>(*attrIt);
            if (!attr)
            {
                continue;
            }

            // Always skip AZCG_IGNORE
            if (attr->getAnnotation().contains("AZCG_IGNORE"))
            {
                shouldBeIgnored = true;
            }
            else
            {
                hasOtherAnnotations = true;
            }
        }

        // We should not have ignored items with other annotations (likely a double annotation)
        if (shouldBeIgnored && hasOtherAnnotations)
        {
            auto& sourceManager = decl->getASTContext().getSourceManager();
            auto presumedLocation = sourceManager.getPresumedLoc(decl->getLocation());
            auto fileName = presumedLocation.getFilename();
            auto lineNumber = presumedLocation.getLine();
            Output::Error("\n%s(%d): Error - Found annotations on an item marked as AZCG_IGNORE. This likely means there are multiple annotation macros used for one item which is unsupported.", fileName, lineNumber);
        }
        return shouldBeIgnored;
    }

    //! Object refers to either a class or a struct.
    //! This class will write out all the object's details: base classes, fields, methods, etc.
    template <typename T>
    class Object
        : public Declaration<T>
    {
    public:
        using Declaration<T>::WriteNameType;
        using Declaration<T>::WriteName;

        void WriteBases(T* decl, IWriter* writer)
        {
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

                    // Deep dive to get full base chains
                    WriteBases(baseDecl, writer);

                    writer->End();
                }
            }

            writer->EndArray();
        }

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
                WriteBases(decl, writer);

                // Fields
                writer->WriteString("fields");
                writer->BeginArray();
                {
                    for (auto fieldIt = decl->field_begin(); fieldIt != decl->field_end(); ++fieldIt)
                    {
                        // Skip ignored fields
                        if (ContainsIgnoreAnnotation(*fieldIt))
                        {
                            continue;
                        }
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
                        if (ContainsIgnoreAnnotation(*methodIt))
                        {
                            continue;
                        }
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