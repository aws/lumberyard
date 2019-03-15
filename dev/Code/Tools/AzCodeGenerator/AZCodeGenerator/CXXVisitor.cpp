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
#include "CXXVisitor.h"
#include "Object.h"
#include "Enum.h"

namespace CodeGenerator
{
    namespace Configuration
    {
        using namespace llvm;

        static cl::OptionCategory codeFilteringCategory("Code Filtering Options");
        static cl::opt<std::string> s_inclusionFilter("inclusion-filter", cl::desc("Using a wildcard filter, allow more than just the specified input-files to be parsed by clang into intermediate data"), cl::cat(codeFilteringCategory), cl::Optional);
    }

    bool CXXVisitor::VisitCXXRecordDecl(clang::CXXRecordDecl* decl)
    {
        if (IsClassOrStruct(decl))
        {
            // At this point, we either have a class or a struct.
            Object<clang::CXXRecordDecl> object;
            object.Write(decl, m_writer);
        }

        return true;
    }

    namespace
    {
        bool PatternMatchesString(const char* pattern, const char* string)
        {
            switch (*pattern)
            {
            case '\0':
            case ':':  // Either ':' or '\0' marks the end of the pattern.
                return *string == '\0';
            case '?':  // Matches any single character.
                return *string != '\0' && PatternMatchesString(pattern + 1, string + 1);
            case '*':  // Matches any string (possibly empty) of characters.
                return (*string != '\0' && PatternMatchesString(pattern, string + 1)) || PatternMatchesString(pattern + 1, string);
            default:  // Non-special character.  Matches itself.
                return *pattern == *string && PatternMatchesString(pattern + 1, string + 1);
            }
        }
    }

    bool CXXVisitor::IsInFileOfInterest(clang::Decl* decl)
    {
        // Check for AZCG_IGNORE annotation first, skip anything that has it
        if (ContainsIgnoreAnnotation(decl))
        {
            return false;
        }

        auto& sourceManager = decl->getASTContext().getSourceManager();

        // Always accept decls from the main file
        if (sourceManager.isInMainFile(decl->getLocation()))
        {
            return true;
        }

        if (Configuration::s_inclusionFilter.empty())
        {
            return false;
        }

        // Inclusion filtering
        clang::SourceLocation loc = decl->getLocation();
        bool isMacro = loc.isMacroID();
        if (isMacro)
        {
            loc = sourceManager.getExpansionLoc(loc);
        }
        std::string declFileName = sourceManager.getBufferName(loc);
        auto fileID = sourceManager.getFileID(loc);

        // Caching to prevent loads of duplicate pattern matching checks
        auto fileCacheEntry = m_shouldVisitFileCache.find(fileID);
        if (fileCacheEntry != m_shouldVisitFileCache.end())
        {
            return fileCacheEntry->second;
        }

        // Not in the cache, make a call and then add it
        auto fileEntry = sourceManager.getFileEntryForID(fileID);

        // No entry for the file, it doesn't match
        if (!fileEntry)
        {
            m_shouldVisitFileCache.insert(std::make_pair(fileID, false));
            return false;
        }

        std::string fileName = fileEntry->getName();
        bool patternMatched = PatternMatchesString(Configuration::s_inclusionFilter.c_str(), fileName.c_str());
        m_shouldVisitFileCache.insert(std::make_pair(fileID, patternMatched));
        return patternMatched;
    }


    bool CXXVisitor::IsClassOrStruct(clang::CXXRecordDecl* decl)
    {
        // We are only interested in visiting records for the main file.
        bool isValidLocation = IsInFileOfInterest(decl);

        return isValidLocation
               && decl->isCompleteDefinition()
               && (decl->isClass() || decl->isStruct())
               && !decl->isDependentType();
    }

    bool CXXVisitor::IsGlobalVariable(clang::VarDecl* decl)
    {
        // We are only interested in visiting records for the main file.
        bool isValidLocation = IsInFileOfInterest(decl);

        return isValidLocation &&
            decl->hasGlobalStorage() && 
            !decl->isStaticLocal(); // variables inside static functions
    }

    bool CXXVisitor::VisitEnumDecl(clang::EnumDecl* decl)
    {
        bool isValidLocation = IsInFileOfInterest(decl);
        if (isValidLocation)
        {
            Enum<clang::EnumDecl> enumDecl;
            enumDecl.Write(decl, m_writer);
        }
        return true;
    }

    bool CXXVisitor::VisitVarDecl(clang::VarDecl* decl)
    {
        if (IsGlobalVariable(decl))
        {
            Field<clang::VarDecl> object;
            object.Write(decl, m_writer);
        }
        return true;
    }

    void CXXVisitor::WriteMetaData(clang::ASTContext& context)
    {
        m_writer->WriteString("meta");
        m_writer->Begin();
        {
            std::string fileName = context.getSourceManager().getFileEntryForID(context.getSourceManager().getMainFileID())->getName();
            m_writer->WriteString("path");
            std::replace(fileName.begin(), fileName.end(), '\\', '/');
            m_writer->WriteString(fileName.c_str());
        }
        m_writer->End();
    }
}