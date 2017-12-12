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

#ifndef CRYINCLUDE_EDITOR_OBJECTS_SPLINEDISTRIBUTOR_H
#define CRYINCLUDE_EDITOR_OBJECTS_SPLINEDISTRIBUTOR_H
#pragma once


#include "SplineObject.h"



class CSplineDistributor
    : public CSplineObject
{
    Q_OBJECT
protected:
    friend class CSplineDistributorClassDesc;
    CSplineDistributor();

public:
    // from CSplineObject
    void OnUpdate() override;
    void SetLayerId(uint16 nLayerId) override;

protected:

    //overrided from CBaseObject.
    void Done() override;
    bool CreateGameObject() override;
    void UpdateVisibility(bool visible) override;
    XmlNodeRef Export(const QString& levelPath, XmlNodeRef& xmlNode) override;
    void InvalidateTM(int nWhyFlags) override;
    void DeleteThis() override { delete this; }
    // Ignore default draw highlight.
    void DrawHighlight(DisplayContext& dc) override {}

    //! Called when variable changes.
    void OnParamChange(IVariable* pVariable);
    void OnGeometryChange(IVariable* pVariable);

    void FreeGameData();
    void LoadGeometry(const QString& filename);
    void UpdateEngineNode(bool bOnlyTransform = false);

private:
    void SetObjectCount(int num);


protected:
    CVariable<QString> mv_geometryFile;
    CVariable<float> mv_step;
    CVariable<bool> mv_follow;
    CVariable<float> mv_zAngle;
    CVariable<bool> mv_outdoor;
    CVariable<bool> mv_castShadowMaps;
    CVariable<bool> mv_rainOccluder;
    CVariable<bool> mv_registerByBBox;
    CVariableEnum<int> mv_hideable;
    CVariable<int> mv_ratioLOD;
    CVariable<float> mv_viewDistanceMultiplier;
    CVariable<bool> mv_excludeFromTriangulation;
    // keep for future implementation if we need
    //CVariable<float> mv_aiRadius;
    CVariable<bool> mv_noDecals;
    CVariable<bool> mv_recvWind;
    CVariable<float> mv_integrQuality;
    CVariable<bool> mv_Occluder;

    _smart_ptr<CEdMesh> m_pGeometry;
    std::vector<IRenderNode*> m_renderNodes;

    bool m_bGameObjectCreated;
};
#endif // CRYINCLUDE_EDITOR_OBJECTS_SPLINEDISTRIBUTOR_H
