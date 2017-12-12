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

    //! Fields are class member variables.
    template <typename T>
    class Namespace
        : public Declaration<T>
    {
    public:

        using Declaration<T>::WriteNameType;
        using Declaration<T>::WriteType;
        bool Write(T* decl, IWriter* writer) override
        {
            if (clang::NamespaceDecl* namespaceDecl = clang::dyn_cast<clang::NamespaceDecl>(decl))
            {
                writer->WriteString("namespaces");
                writer->BeginArray();
                std::vector<std::string> namespaceArray;

                // Collect the namespaces into array
                auto it = namespaceDecl;
                while (it)
                {
                    namespaceArray.push_back(it->getNameAsString());
                    it = clang::dyn_cast<clang::NamespaceDecl>(it->getParent());
                }

                // Write out the namespaces from outermost to innermost.
                for (auto it = namespaceArray.rbegin(); it != namespaceArray.rend(); ++it)
                {
                    writer->WriteString(*it);
                }

                writer->EndArray();
            }

            return true;
        }

    protected:

        void WriteType(T* /*decl*/, IWriter* /*writer*/) override
        {
        }
    };
}