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

// Description : WaterWave object definition.


#ifndef CRYINCLUDE_EDITOR_OBJECTS_WATERWAVEOBJECT_H
#define CRYINCLUDE_EDITOR_OBJECTS_WATERWAVEOBJECT_H
#pragma once


#include "RoadObject.h"

/*!
 *  CWaterWaveObject used to create shore waves
 *
 */

class CWaterWaveObject
    : public CRoadObject
{
    Q_OBJECT
public:

    CWaterWaveObject();

    virtual void Serialize(CObjectArchive& ar);
    virtual XmlNodeRef Export(const QString& levelPath, XmlNodeRef& xmlNode);

    //! CRoadObject override: Needed for snapping correctly to water plane - If there's better solution feel free to change
    virtual void SetPoint(int index, const Vec3& pos);
    //! CRoadObject override: Needed for snapping correctly to water plane - If there's better solution feel free to change
    virtual int MouseCreateCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags);

protected:
    //! CRoadObject override: Needed for snapping correctly to water plane - If there's better solution feel free to change
    Vec3 MapViewToCP(CViewport* view, const QPoint& point);

    virtual bool Init(IEditor* ie, CBaseObject* prev, const QString& file);
    virtual void InitVariables();
    virtual void UpdateSectors();

    virtual void OnWaterWaveParamChange(IVariable* var);
    virtual void OnWaterWavePhysicsParamChange(IVariable* var);

    void Physicalize();
    bool IsValidSector(const CRoadSector& pSector);

    //////////////////////////////////////////////////////////////////////////
    // Overrides from CBaseObject.
    //////////////////////////////////////////////////////////////////////////

    virtual void Done();

protected:

    uint64 m_nID;

    CVariable<float> mv_surfaceUScale;
    CVariable<float> mv_surfaceVScale;

    CVariable<float> mv_speed;
    CVariable<float> mv_speedVar;

    CVariable<float> mv_lifetime;
    CVariable<float> mv_lifetimeVar;

    CVariable<float> mv_height;
    CVariable<float> mv_heightVar;

    CVariable<float> mv_posVar;

    IRenderNode* m_pRenderNode;
};
#endif // CRYINCLUDE_EDITOR_OBJECTS_WATERWAVEOBJECT_H
