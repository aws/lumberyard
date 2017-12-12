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

#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2012 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   DesignerObject.h
//  Created:     12/Oct/2011 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "DesignerBaseObject.h"

class IDesignerPanel;
class IDesignerSubPanel;
class DesignerObject;

namespace CD
{
    class ModelCompiler;

    struct SDesignerEngineFlags
    {
        bool outdoor;
        bool castShadows;
        bool supportSecVisArea;
        bool rainOccluder;
        bool hideable;
        float viewDistanceMultiplier;
        bool excludeFromTriangulation;
        bool noDynWater;
        bool noStaticDecals;
        bool excludeCollision;
        bool occluder;

        DesignerObject* m_pObj;

        SDesignerEngineFlags();
        void Set();
        void Update();
        void Serialize(Serialization::IArchive& ar);
    };
};

class DesignerObject
    : public DesignerBaseObject<CBaseObject>
{
    Q_OBJECT
public:
    DesignerObject();
    ~DesignerObject(){}

    static const GUID& GetClassID()
    {
        static const GUID guid = {
            0xcafbc2d2, 0x3784, 0x4de4, { 0x8f, 0x1f, 0x69, 0x2c, 0xa7, 0x8d, 0x68, 0x65 }
        };
        return guid;
    }

    bool Init(IEditor* ie, CBaseObject* prev, const QString& file) override;
    void Display(DisplayContext& dc) override;

    void GetBoundBox(AABB& box) override;
    void GetLocalBounds(AABB& box) override;

    bool HitTest(HitContext& hc) override;
    virtual IPhysicalEntity* GetCollisionEntity() const override;

    void BeginEditParams(IEditor* ie, int flags) override;
    void BeginEditMultiSelParams(bool bAllOfSameType) override;

    void Serialize(CObjectArchive& ar) override;

    void SetMaterial(CMaterial* mtl) override;
    void SetMaterial(const QString& materialName) override;

    void SetMaterialLayersMask(uint32 nLayersMask);
    void SetMinSpec(uint32 nSpec, bool bSetChildren = true);

    bool IsSimilarObject(CBaseObject* pObject) const;
    void OnEvent(ObjectEvent event);

    XmlNodeRef Export(const QString& levelPath, XmlNodeRef& xmlNode) { return 0; }

    void GenerateGameFilename(QString& generatedFileName) const;

    void OnMaterialChanged(MaterialChangeFlags change) override;

    IRenderNode* GetEngineNode() const;

    void UpdateEngineFlags(){ m_EngineFlags.Update(); }
    void SwitchToDesignerEditTool();

    CD::SDesignerEngineFlags& GetEngineFlags() { return m_EngineFlags; }

protected:

    void UpdateVisibility(bool visible);
    void WorldToLocalRay(Vec3& raySrc, Vec3& rayDir) const;

    void DeleteThis() { delete this; };
    void InvalidateTM(int nWhyFlags);

    void DrawDimensions(DisplayContext& dc, AABB* pMergedBoundBox);

private:

    Matrix34 m_invertTM;
    uint32 m_nBrushUniqFileId;

    CD::SDesignerEngineFlags m_EngineFlags;
};

class SolidObjectClassDesc
    : public CObjectClassDesc
{
public:
    REFGUID ClassID() override
    {
        static const GUID guid = {
            0xe58b34c2, 0x5ed2, 0x4538, { 0x88, 0x96, 0x47, 0x47, 0xfa, 0x2b, 0x28, 0x9 }
        };
        return guid;
    }
    ObjectType GetObjectType() { return OBJTYPE_SOLID; };
    QString ClassName() { return "Solid"; };
    QString Category() { return ""; };
    QString GetFileSpec() { return ""; };
    int GameCreationOrder() { return 150; };
    QObject* CreateQObject() const override { return new DesignerObject; }
};
