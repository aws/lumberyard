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
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011
// -------------------------------------------------------------------------
//  File name:   AreaSolidObject.h
//  Created:     13/9/2011 by Jaesik
//  Description: AreaSolid object definition
//
////////////////////////////////////////////////////////////////////////////
#include "Objects/EntityObject.h"
#include "Objects/AreaBox.h"
#include "DesignerBaseObject.h"

class IDesignerEventHandler;
class CPickEntitiesPanel;
class AreaSolidPanel;
struct IComponentArea;
class ICrySizer;

class AreaSolidObject
    : public DesignerBaseObject<CAreaObjectBase>
{
    Q_OBJECT
public:
    ~AreaSolidObject();

    bool CreateGameObject();
    virtual void InitVariables();
    void GetBoundBox(AABB& box);
    void GetLocalBounds(AABB& box);
    bool HitTest(HitContext& hc);
    void Display(DisplayContext& dc);
    void DisplayMemoryUsage(DisplayContext& dc);

    bool Init(IEditor* ie, CBaseObject* prev, const QString& file) override;

    void BeginEditParams(IEditor* ie, int flags);
    void EndEditParams(IEditor* ie);

    void SetMaterial(CMaterial* mtl);

    CD::ModelCompiler* GetCompiler() const override;

    virtual void PostLoad(CObjectArchive& ar) { UpdateGameArea(); }
    virtual void InvalidateTM(int nWhyFlags);

    void Serialize(CObjectArchive& ar);
    XmlNodeRef Export(const QString& levelPath, XmlNodeRef& xmlNode) override;

    void SetAreaId(int nAreaId);
    int GetAreaId() const{ return m_areaId; }

    void UpdateGameArea() override;
    void UpdateGameResource() override { UpdateGameArea(); };

    void GenerateGameFilename(QString& generatedFileName) const;

    AreaSolidPanel* GetAreaSolidPanel() const { return m_pActivateEditAreaSolidPanel; }

    void OnEvent(ObjectEvent event) override;

protected:
    friend class CTemplateObjectClassDesc<AreaSolidObject>;
    //! Dtor must be protected.
    AreaSolidObject();

    static const GUID& GetClassID()
    {
        // {672811ea-da24-4586-befb-742a4ed96d0c}
        static const GUID guid = {
            0x672811ea, 0xda24, 0x4586, { 0xbe, 0xfb, 0x74, 0x2a, 0x4e, 0xd9, 0x6d, 0x0c }
        };
        return guid;
    }

    void AddConvexhullToEngineArea(IComponentArea* pArea, std::vector<std::vector<Vec3> >& faces, bool bObstructrion);
    void DeleteThis() { delete this; };

    void OnAreaChange(IVariable* pVar) override { UpdateGameArea(); }
    void OnSizeChange(IVariable* pVar) { UpdateGameArea(); }

    bool m_bAreaModified;

    CVariable<int> m_areaId;
    CVariable<float> m_edgeWidth;
    CVariable<int> mv_groupId;
    CVariable<int> mv_priority;
    CVariable<bool> mv_filled;

    static int s_nGlobalFileAreaSolidID;
    int m_nUniqFileIdForExport;

    int m_nActivateEditAreaSolidRollUpID;
    AreaSolidPanel* m_pActivateEditAreaSolidPanel;
    ICrySizer* m_pSizer;
};
