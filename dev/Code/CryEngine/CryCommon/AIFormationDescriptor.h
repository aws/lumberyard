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

#ifndef CRYINCLUDE_CRYCOMMON_AIFORMATIONDESCRIPTOR_H
#define CRYINCLUDE_CRYCOMMON_AIFORMATIONDESCRIPTOR_H
#pragma once


// Unit classes for group behavior.
#define UNIT_CLASS_UNDEFINED        1
#define UNIT_CLASS_LEADER           (1 << 1)
#define UNIT_CLASS_INFANTRY         (1 << 2)
#define UNIT_CLASS_SCOUT            (1 << 3)
#define UNIT_CLASS_ENGINEER         (1 << 4)
#define UNIT_CLASS_MEDIC            (1 << 5)
#define UNIT_CLASS_CIVILIAN         (1 << 6)
#define UNIT_CLASS_COMPANION        (1 << 7)
#define SHOOTING_SPOT_POINT     (1 << 15)
#define SPECIAL_FORMATION_POINT     (1 << 16)
#define UNIT_ALL                    0xffffffff

typedef struct FormationNode
{
    FormationNode()
        : vOffset(0, 0, 0)
        , vSightDirection(0, 0, 0)
        , fFollowDistance(0)
        , fFollowOffset(0)
        , fFollowDistanceAlternate(0)
        , fFollowOffsetAlternate(0)
        , eClass(UNIT_CLASS_UNDEFINED)
        , fFollowHeightOffset(0){}

    Vec3 vOffset;
    Vec3 vSightDirection;

    float fFollowDistance;
    float fFollowOffset;
    float fFollowDistanceAlternate;
    float fFollowOffsetAlternate;
    float fFollowHeightOffset;


    int eClass;
} FormationNode;


class CFormationDescriptor
{
public:
    typedef std::vector<FormationNode> TVectorOfNodes;
    string m_sName;
    // Francesco TODO: the best would be to always use the crc32 and use
    // the string member only to expose the formation names to Sandbox.
    unsigned int m_nNameCRC32;
    TVectorOfNodes m_Nodes;
public:
    CFormationDescriptor() {};
    void AddNode(const FormationNode& nodeDescriptor);
    int GetNodeClass(int i)
    {
        if (i < (int)m_Nodes.size() && i >= 0)
        {
            return m_Nodes[i].eClass;
        }
        return -1;
    }
    void Clear() {m_Nodes.resize(0); };
    float GetNodeDistanceToOwner(const FormationNode& nodeDescriptor) const;

    template<typename Sizer>
    void GetMemoryUsage(Sizer* pSizer) const
    {
        pSizer->AddObject(m_sName);
        pSizer->AddContainer(m_Nodes);
    }
};

#endif // CRYINCLUDE_CRYCOMMON_AIFORMATIONDESCRIPTOR_H
