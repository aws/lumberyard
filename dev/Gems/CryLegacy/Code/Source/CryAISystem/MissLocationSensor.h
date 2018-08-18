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

#ifndef CRYINCLUDE_CRYAISYSTEM_MISSLOCATIONSENSOR_H
#define CRYINCLUDE_CRYAISYSTEM_MISSLOCATIONSENSOR_H
#pragma once

class CMissLocationSensor
{
    struct MissLocation
    {
        enum EType
        {
            Destroyable,
            Lightweight,
            Rope,
            ManuallyBreakable,
            JointStructure,
            Deformable,
            MatBreakable,
            Unbreakable,
            Undefined,
        };

        MissLocation()
            : position(ZERO)
            , type(Undefined)
            , score(0.0f)
        {
        }

        MissLocation(const Vec3& pos, EType typ)
            : position(pos)
            , type(typ)
            , score(0.0f)
        {
        }

        bool operator <(const MissLocation& rhs) const
        {
            return score < rhs.score;
        };

        Vec3 position;
        float score;
        EType type;
    };

    enum EState
    {
        Starting = 0,
        Collecting,
        Filtering,
        Finishing,
    };

    enum
    {
        MaxCollectedCount = 384,
        MaxConsiderCount = 32,
        MaxRopeVertexCount = 4,
        MaxRandomPool = 6,
    };

public:
    CMissLocationSensor(const CAIActor* owner);
    virtual ~CMissLocationSensor();

    void Reset();
    void Update(float timeLimit);
    void Collect(int objTypes);
    bool Filter(float timeLimit);
    bool GetLocation(CAIObject* target, const Vec3& shootPos, const Vec3& shootDir, float maxAngle, Vec3& pos);

    void DebugDraw();

    void AddDestroyableClass(const char* className);
    void ResetDestroyableClasses();

public:
    void ClearEntities();

    typedef std::vector<IPhysicalEntity*> MissEntities;
    MissEntities m_entities;

    typedef std::vector<MissLocation> MissLocations;
    MissLocations m_locations;
    MissLocations m_working;

    MissLocations m_goodies;

    typedef std::vector<IEntityClass*> DestroyableClasses;
    DestroyableClasses m_destroyableEntityClasses;

    uint32 m_updateCount;
    Vec3 m_lastCollectionLocation;

    EState m_state;
    const CAIActor* m_owner;
};

#endif // CRYINCLUDE_CRYAISYSTEM_MISSLOCATIONSENSOR_H
