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

#ifndef CRYINCLUDE_EDITOR_OBJECTS_CAMERAOBJECT_H
#define CRYINCLUDE_EDITOR_OBJECTS_CAMERAOBJECT_H
#pragma once

class QMenu;

#include "EntityObject.h"
#include "QtViewPane.h"

class ICameraObjectListener
{
public:
    virtual void OnFovChange(const float fov) = 0;
    virtual void OnNearZChange(const float nearZ) = 0;
    virtual void OnFarZChange(const float farZ) = 0;
    virtual void OnShakeAmpAChange(const Vec3 amplitude) = 0;
    virtual void OnShakeAmpBChange(const Vec3 amplitude) = 0;
    virtual void OnShakeFreqAChange(const Vec3 frequency) = 0;
    virtual void OnShakeFreqBChange(const Vec3 frequency) = 0;
    virtual void OnShakeMultChange(const float amplitudeAMult, const float amplitudeBMult, const float frequencyAMult, const float frequencyBMult) = 0;
    virtual void OnShakeNoiseChange(const float noiseAAmpMult, const float noiseBAmpMult, const float noiseAFreqMult, const float noiseBFreqMult) = 0;
    virtual void OnShakeWorkingChange(const float timeOffsetA, const float timeOffsetB) = 0;
    virtual void OnCameraShakeSeedChange(const int seed) = 0;
};

/*!
 *  CCameraObject is an object that represent Source or Target of camera.
 *
 */
class SANDBOX_API CCameraObject
    : public CEntityObject
{
    Q_OBJECT
public:

    //////////////////////////////////////////////////////////////////////////
    // Overrides from CBaseObject.
    //////////////////////////////////////////////////////////////////////////
    bool Init(IEditor* ie, CBaseObject* prev, const QString& file);
    void InitVariables();
    void Done();
    QString GetTypeDescription() const { return GetTypeName(); };
    void Display(DisplayContext& disp);

    //////////////////////////////////////////////////////////////////////////
    virtual void SetName(const QString& name);
    bool IsScalable() const override { return false; }

    void BeginEditParams(IEditor* ie, int flags);
    void EndEditParams(IEditor* ie);

    //! Called when object is being created.
    int MouseCreateCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags);
    bool HitTestRect(HitContext& hc);
    bool HitHelperTest(HitContext& hc);
    void Serialize(CObjectArchive& ar);

    void GetBoundBox(AABB& box);
    void GetLocalBounds(AABB& box);
    void OnContextMenu(QMenu* menu);
    //////////////////////////////////////////////////////////////////////////

    void SetFOV(const float fov);
    void SetNearZ(const float nearZ);
    void SetAmplitudeAMult(const float amplitudeAMult);
    void SetAmplitudeBMult(const float amplitudeBMult);
    void SetFrequencyAMult(const float frequencyAMult);
    void SetFrequencyBMult(const float frequencyBMult);

    // FOV and NearZ can be animated by Track View so they check that the loca mv_fov and mv_nearZ
    // CVariables are up to date with the associated camera components before returning
    const float GetFOV();
    const float GetNearZ();
    const float GetFarZ() const {return mv_farZ; }
    const Vec3& GetAmplitudeA() const {return mv_amplitudeA; }
    const float GetAmplitudeAMult() const {return mv_amplitudeAMult; }
    const Vec3& GetFrequencyA() const {return mv_frequencyA; }
    const float GetFrequencyAMult() const {return mv_frequencyAMult; }
    const float GetNoiseAAmpMult() const {return mv_noiseAAmpMult; }
    const float GetNoiseAFreqMult() const {return mv_noiseAFreqMult; }
    const float GetTimeOffsetA() const {return mv_timeOffsetA; }
    const Vec3& GetAmplitudeB() const {return mv_amplitudeB; }
    const float GetAmplitudeBMult() const {return mv_amplitudeBMult; }
    const Vec3& GetFrequencyB() const {return mv_frequencyB; }
    const float GetFrequencyBMult() const {return mv_frequencyBMult; }
    const float GetNoiseBAmpMult() const {return mv_noiseBAmpMult; }
    const float GetNoiseBFreqMult() const {return mv_noiseBFreqMult; }
    const float GetTimeOffsetB() const {return mv_timeOffsetB; }
    const uint GetCameraShakeSeed() const {return mv_cameraShakeSeed; }

    void RegisterCameraListener(ICameraObjectListener* pListener);
    void UnregisterCameraListener(ICameraObjectListener* pListener);

    // overriden from CEntityObject
    void SpawnEntity() override;

    static const GUID& GetClassID()
    {
        // {23612EE3-B568-465d-9B31-0CA32FDE2340}
        static const GUID guid = {
            0x23612ee3, 0xb568, 0x465d, { 0x9b, 0x31, 0xc, 0xa3, 0x2f, 0xde, 0x23, 0x40 }
        };
        return guid;
    }

private:
    friend class CTemplateObjectClassDesc<CCameraObject>;
    //! Dtor must be protected.
    CCameraObject();

    // overrided from IAnimNodeCallback
    //void OnNodeAnimated( IAnimNode *pNode );

    virtual void DrawHighlight(DisplayContext& dc) {};

    // return world position for the entity targeted by look at.
    Vec3 GetLookAtEntityPos() const;

    void OnFovChange(IVariable* var);
    void OnNearZChange(IVariable* var);
    void OnFarZChange(IVariable* var);

    void UpdateCameraEntity();

    void OnShakeAmpAChange(IVariable* var);
    void OnShakeAmpBChange(IVariable* var);
    void OnShakeFreqAChange(IVariable* var);
    void OnShakeFreqBChange(IVariable* var);
    void OnShakeMultChange(IVariable* var);
    void OnShakeNoiseChange(IVariable* var);
    void OnShakeWorkingChange(IVariable* var);
    void OnCameraShakeSeedChange(IVariable* var);

    void OnMenuSetAsViewCamera();

    // Arguments
    //   fAspectRatio - e.g. 4.0/3.0
    void GetConePoints(Vec3 q[4], float dist, const float fAspectRatio);
    void DrawCone(DisplayContext& dc, float dist, float fScale = 1);
    void CreateTarget();
    //////////////////////////////////////////////////////////////////////////
    //! Field of view.
    CVariable<float> mv_fov;

    CVariable<float> mv_nearZ;
    CVariable<float> mv_farZ;

    //////////////////////////////////////////////////////////////////////////
    //! Camera shake.
    CVariableArray mv_shakeParamsArray;
    CVariable<Vec3> mv_amplitudeA;
    CVariable<float> mv_amplitudeAMult;
    CVariable<Vec3> mv_frequencyA;
    CVariable<float> mv_frequencyAMult;
    CVariable<float> mv_noiseAAmpMult;
    CVariable<float> mv_noiseAFreqMult;
    CVariable<float> mv_timeOffsetA;
    CVariable<Vec3> mv_amplitudeB;
    CVariable<float> mv_amplitudeBMult;
    CVariable<Vec3> mv_frequencyB;
    CVariable<float> mv_frequencyBMult;
    CVariable<float> mv_noiseBAmpMult;
    CVariable<float> mv_noiseBFreqMult;
    CVariable<float> mv_timeOffsetB;
    CVariable<int> mv_cameraShakeSeed;

    //////////////////////////////////////////////////////////////////////////
    // Mouse callback.
    int m_creationStep;

    CListenerSet<ICameraObjectListener*> m_listeners;
};

/*!
 *  CCameraObjectTarget is a target object for Camera.
 *
 */
class CCameraObjectTarget
    : public CEntityObject
{
    Q_OBJECT
public:
    //////////////////////////////////////////////////////////////////////////
    // Ovverides from CBaseObject.
    //////////////////////////////////////////////////////////////////////////
    bool Init(IEditor* ie, CBaseObject* prev, const QString& file);
    void InitVariables();
    QString GetTypeDescription() const { return GetTypeName(); };
    void Display(DisplayContext& disp);
    bool HitTest(HitContext& hc);
    void GetBoundBox(AABB& box);
    bool IsScalable() const override { return false; }
    bool IsRotatable() const override { return false; }
    void Serialize(CObjectArchive& ar);
    //////////////////////////////////////////////////////////////////////////

protected:
    friend class CTemplateObjectClassDesc<CCameraObjectTarget>;

    //! Dtor must be protected.
    CCameraObjectTarget();

    static const GUID& GetClassID()
    {
        // {1AC4CF4E-9614-4de8-B791-C0028D0010D2}
        static const GUID guid = {
            0x1ac4cf4e, 0x9614, 0x4de8, { 0xb7, 0x91, 0xc0, 0x2, 0x8d, 0x0, 0x10, 0xd2 }
        };
        return guid;
    }

    virtual void DrawHighlight(DisplayContext& dc) {};
};

#endif // CRYINCLUDE_EDITOR_OBJECTS_CAMERAOBJECT_H
