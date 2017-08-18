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

namespace clang
{
    class ASTContext;
}

// Hash specialization for clang::FileID, used by unordered_map below
template<>
struct std::hash<clang::FileID>
{
    size_t operator() (const clang::FileID& fileID) const
    {
        return fileID.getHashValue();
    }
};

namespace CodeGenerator
{
    class IWriter;

    //! RecursiveASTVisitor does preorder depth-first traversal on the entire Clang AST and visits each node.
    //! This visitor is used to write out the generated CXX data.
    class CXXVisitor
        : public clang::RecursiveASTVisitor < CXXVisitor >
    {
    public:

        CXXVisitor(IWriter* writer)
            : m_writer(writer)
        {}

        //! Writes general use data regarding the current translation unit.
        void WriteMetaData(clang::ASTContext& context);

        //! Implementation to visit CXX record declarations.
        //! \param decl The declaration record being visited.
        //! \return bool The return value indicates whether we want the visitation to proceed. Return false to stop the traversal of the AST.
        bool VisitCXXRecordDecl(clang::CXXRecordDecl* decl);

        //! Implementation to visit Enum record declarations.
        //! \param decl The Enum declaration record being visited.
        bool VisitEnumDecl(clang::EnumDecl* decl);

        //! Implementation to visit Value record declarations (global variables)
        //! \param decl The Value declaration record being visited.
        bool VisitVarDecl(clang::VarDecl* decl);

    protected:

        bool IsClassOrStruct(clang::CXXRecordDecl* decl);
        bool IsGlobalVariable(clang::VarDecl* decl);

        bool IsInFileOfInterest(clang::Decl* decl);

        IWriter* m_writer;

        std::unordered_map<clang::FileID, bool> m_shouldVisitFileCache;
    };
}