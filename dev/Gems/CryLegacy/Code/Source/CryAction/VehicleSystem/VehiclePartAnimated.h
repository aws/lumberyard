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

// Description : Implements a part for vehicles which uses CGA files


#ifndef CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEPARTANIMATED_H
#define CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEPARTANIMATED_H
#pragma once

#include "VehiclePartBase.h"

class CScriptBind_VehiclePart;
class CVehicle;
class CVehiclePartAnimatedJoint;

class CVehiclePartAnimated
    : public CVehiclePartBase
{
    IMPLEMENT_VEHICLEOBJECT
public:

    CVehiclePartAnimated();
    ~CVehiclePartAnimated();

    // IVehiclePart
    virtual bool Init(IVehicle* pVehicle, const CVehicleParams& table, IVehiclePart* parent, CVehicle::SPartInitInfo& initInfo, int partType);
    virtual void Release();
    virtual void Reset();

    virtual void SetMaterial(_smart_ptr<IMaterial> pMaterial);

    virtual void OnEvent(const SVehiclePartEvent& event);

    virtual bool ChangeState(EVehiclePartState state, int flags = 0);
    virtual void Physicalize();

    virtual void Update(const float frameTime);

    virtual void Serialize(TSerialize serialize, EEntityAspects);
    virtual void PostSerialize();

    virtual void GetMemoryUsage(ICrySizer* s) const;
    // ~IVehiclePart

    virtual IStatObj* GetSubGeometry(CVehiclePartBase* pPart, EVehiclePartState state, Matrix34& position, bool removeFromParent);
    IStatObj* GetGeometryForState(CVehiclePartAnimatedJoint* pPart, EVehiclePartState state);
    IStatObj* GetDestroyedGeometry(const char* pJointName, unsigned int index = 0);
    Matrix34 GetDestroyedGeometryTM(const char* pJointName, unsigned int index);

    virtual void SetDrivingProxy(bool bDrive);

    void RotationChanged(CVehiclePartAnimatedJoint* pJoint);

    int AssignAnimationLayer() { return ++m_lastAnimLayerAssigned; }

#if ENABLE_VEHICLE_DEBUG
    void Dump();
#endif

    int GetNextFreeLayer();
    bool ChangeChildState(CVehiclePartAnimatedJoint* pPart, EVehiclePartState state, int flags);

protected:

    virtual void InitGeometry();
    void FlagSkeleton(ISkeletonPose* pSkeletonPose, IDefaultSkeleton& rIDefaultSkeleton);
    virtual EVehiclePartState GetStateForDamageRatio(float ratio);

    typedef std::map <string, /*_smart_ptr<*/ IStatObj* > TStringStatObjMap;
    TStringStatObjMap m_intactStatObjs;

    typedef std::map<string, IVehiclePart*> TStringVehiclePartMap;
    TStringVehiclePartMap m_jointParts;

    _smart_ptr<ICharacterInstance> m_pCharInstance;
    ICharacterInstance* m_pCharInstanceDestroyed;

    int m_hullMatId[2];

    int m_lastAnimLayerAssigned;
    int m_iRotChangedFrameId;
    bool m_serializeForceRotationUpdate;
    bool m_initialiseOnChangeState;
    bool m_ignoreDestroyedState;
};

#endif // CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEPARTANIMATED_H
