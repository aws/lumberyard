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

#ifndef CRYINCLUDE_CRYAISYSTEM_GOALPIPEXMLREADER_H
#define CRYINCLUDE_CRYAISYSTEM_GOALPIPEXMLREADER_H
#pragma once

#include <IGoalPipe.h>

class CPipeManager;

class CGoalPipeXMLReader
{
public:
    CGoalPipeXMLReader();

    bool LoadGoalPipesFromXmlFile(const char* filename);
    bool LoadGoalPipesFromXmlNode(const XmlNodeRef& root);

    void ParseGoalPipe(const char* szGoalPipeName, const XmlNodeRef goalPipeNode, CPipeManager::ActionToTakeWhenDuplicateFound actionToTakeWhenDuplicateFound = CPipeManager::ReplaceDuplicateAndReportError);

private:
    void ParseGoalOps(IGoalPipe* pGoalPipe, const XmlNodeRef& root,
        string sIfLabel = string(), bool b_IfElseEnd_Halves_ReverseOrder = true);
    void ParseGoalOp(IGoalPipe* pGoalPipe, const XmlNodeRef& goalOpNode);

    string GenerateUniqueLabelName();

private:
    class CXMLAttrReader
    {
        typedef std::pair<const char*, int> TRecord;
        std::vector<TRecord> m_dictionary;

    public:
        void Add(const char* szKey, int nValue) { m_dictionary.push_back(TRecord(szKey, nValue)); }
        bool Get(const char* szKey, int& nValue);
    };

    CXMLAttrReader m_dictBranchType;

    IGoalPipe::EGroupType m_currentGrouping;

    uint32 m_nextUniqueLabelID;
};


#endif // CRYINCLUDE_CRYAISYSTEM_GOALPIPEXMLREADER_H
