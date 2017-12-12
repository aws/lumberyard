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

// Description : Implements a part for vehicles which is the an attachment
//               of a parent Animated part


#ifndef CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEPARTANIMATEDJOINT_H
#define CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEPARTANIMATEDJOINT_H
#pragma once

#include "VehicleSystem/VehiclePartBase.h"

#define VEH_USE_RPM_JOINT

class CVehicle;

class CVehiclePartAnimatedJoint
    : public CVehiclePartBase
{
    IMPLEMENT_VEHICLEOBJECT
public:

    CVehiclePartAnimatedJoint();
    virtual ~CVehiclePartAnimatedJoint();

    // IVehiclePart
    virtual bool Init(IVehicle* pVehicle, const CVehicleParams& table, IVehiclePart* parent, CVehicle::SPartInitInfo& initInfo, int partType);
    virtual void InitGeometry(const CVehicleParams& table);
    virtual void PostInit();
    virtual void Reset();
    virtual bool ChangeState(EVehiclePartState state, int flags);
    virtual void SetMaterial(_smart_ptr<IMaterial> pMaterial);
    virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params);

    virtual void OnEvent(const SVehiclePartEvent& event);

    virtual const Matrix34& GetLocalTM(bool relativeToParentPart, bool forced = false);
    virtual const Matrix34& GetWorldTM();
    virtual const AABB& GetLocalBounds();
    virtual const Matrix34& GetLocalInitialTM() { return m_initialTM; }

    virtual void Physicalize();
    virtual void Update(const float frameTime);

    virtual void InvalidateTM(bool invalidate);
    virtual void SetMoveable(bool allowTranslationMovement = false);

    virtual void GetMemoryUsage(ICrySizer* s) const;
    // ~IVehiclePart

    virtual void SetLocalTM(const Matrix34& localTM);
    virtual void ResetLocalTM(bool recursive);

    virtual int GetJointId() { return m_jointId; }

    virtual IStatObj* GetStatObj();
    virtual bool SetStatObj(IStatObj* pStatObj);

    virtual _smart_ptr<IMaterial> GetMaterial();

    virtual const Matrix34& GetLocalBaseTM() { return m_baseTM; }
    virtual void SetLocalBaseTM(const Matrix34& tm);

    virtual void SerMatrix(TSerialize ser, Matrix34& mat);
    virtual void Serialize(TSerialize ser, EEntityAspects aspects);

    virtual IStatObj* GetExternalGeometry(bool destroyed) { return destroyed ? m_pDestroyedGeometry.get() : m_pGeometry.get(); }

protected:
    Matrix34 m_baseTM;
    Matrix34 m_initialTM;
    Matrix34 m_worldTM;
    Matrix34 m_localTM;
    AABB m_localBounds;

    // if using external geometry
    _smart_ptr<IStatObj> m_pGeometry;
    _smart_ptr<IStatObj> m_pDestroyedGeometry;
    _smart_ptr<IMaterial> m_pMaterial;

    _smart_ptr<ICharacterInstance> m_pCharInstance;
    CVehiclePartAnimated* m_pAnimatedPart;
    int m_jointId;

#ifdef VEH_USE_RPM_JOINT
    float m_dialsRotMax;
    float m_initialRotOfs;
#endif

    bool m_localTMInvalid;
    bool m_isMoveable;
    bool m_isTransMoveable;
    bool m_bUsePaintMaterial;

    IAnimationOperatorQueuePtr m_operatorQueue;

    friend class CVehiclePartAnimated;
};

#endif // CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEPARTANIMATEDJOINT_H
