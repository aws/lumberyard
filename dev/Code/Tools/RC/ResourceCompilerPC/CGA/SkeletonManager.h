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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGA_SKELETONMANAGER_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGA_SKELETONMANAGER_H
#pragma once


#include <set>
#include "SkeletonLoader.h"

struct IPakSystem;
class ICryXML;
struct IResourceCompiler;

class SkeletonManager
{
public:
    SkeletonManager(IPakSystem* pPakSystem, ICryXML* pXMLParser, IResourceCompiler* pRc);

    bool LoadSkeletonList(const string& filename, const string& rootPath, const std::set<string>& skeletons);
    const CSkeletonInfo* FindSkeleton(const string& name) const;
    const CSkeletonInfo* LoadSkeleton(const string& name);

private:
    const CSkeletonInfo* LoadSkeletonInfo(const string& name, const string& file);

private:
    IPakSystem* m_pPakSystem;
    ICryXML* m_pXmlParser;

    string m_rootPath;
    string m_tmpPath;

    typedef std::map<string, string> TNameToFileMap;
    TNameToFileMap m_nameToFile;

    typedef std::map<string, SkeletonLoader> TNameToSkeletonMap;
    TNameToSkeletonMap m_nameToSkeletonInfo;
};

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGA_SKELETONMANAGER_H
