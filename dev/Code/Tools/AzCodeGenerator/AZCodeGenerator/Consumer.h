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

#include "CXXVisitor.h"

namespace CodeGenerator
{
    class Consumer
        : public clang::ASTConsumer
    {
    public:

        Consumer(IWriter* writer)
            : clang::ASTConsumer()
            , m_visitor(writer)
            , m_writer(writer)
        {}

        virtual void HandleTranslationUnit(clang::ASTContext& context)
        {
            m_visitor.WriteMetaData(context);

            m_writer->WriteString("objects");
            m_writer->BeginArray();
            {
                // Preorder depth-first traversal on the entire Clang AST.
                m_visitor.TraverseDecl(context.getTranslationUnitDecl());
            }
            m_writer->EndArray();
        }
    private:

        CXXVisitor m_visitor;
        IWriter* m_writer;
    };
}