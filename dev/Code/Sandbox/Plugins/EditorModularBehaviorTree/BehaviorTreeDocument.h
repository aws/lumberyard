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

#ifndef BehaviorTreeDescriptor_h
#define BehaviorTreeDescriptor_h

#pragma once

#include <BehaviorTree/IBehaviorTree.h>

class BehaviorTreeDocument
{
public:
    BehaviorTreeDocument();

    void Reset();
    bool Loaded();

    bool Changed();
    void SetChanged();

    void Serialize(Serialization::IArchive& archive);

    void NewFile(const char* behaviorTreeName, const char* absoluteFilePath);
    bool OpenFile(const char* behaviorTreeName, const char* absoluteFilePath);
    bool Save();
    bool SaveToFile(const char* behaviorTreeName, const char* absoluteFilePath);

private:
    XmlNodeRef GenerateBehaviorTreeXml();

    bool m_changed;
    string m_behaviorTreeName;
    string m_absoluteFilePath;
    BehaviorTree::BehaviorTreeTemplatePtr m_behaviorTreeTemplate;
};

#endif // BehaviorTreeDescriptor_h
