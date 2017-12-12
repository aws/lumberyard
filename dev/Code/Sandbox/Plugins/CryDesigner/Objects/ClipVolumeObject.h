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

#include "Objects/EntityObject.h"
#include "Objects/DesignerBaseObject.h"

class ClipVolumeObjectPanel;

class ClipVolumeObject
    : public DesignerBaseObject<CEntityObject>
{
    Q_OBJECT
public:
    ClipVolumeObject();
    virtual ~ClipVolumeObject();

    static const GUID& GetClassID()
    {
        // {C91100AA-1DBF-4410-B25A-0325853480C4}
        static const GUID guid = {
            0xc91100aa, 0x1dbf, 0x4410, { 0xb2, 0x5a, 0x3, 0x25, 0x85, 0x34, 0x80, 0xc4 }
        };
        return guid;
    }

    void Display(DisplayContext& dc) override;

    bool Init(IEditor* ie, CBaseObject* prev, const QString& file) override;
    void InitVariables() override {}
    void Serialize(CObjectArchive& ar) override;

    bool CreateGameObject() override;
    void EntityLinked(const QString& name, GUID targetEntityId);

    void GetLocalBounds(AABB& box) override;
    bool HitTest(HitContext& hc) override;

    void BeginEditParams(IEditor* ie, int flags);
    void EndEditParams(IEditor* ie);

    void UpdateGameResource() override;
    QString GenerateGameFilename() const;

    void ExportBspTree(IChunkFile* pChunkFile) const;

private:
    void UpdateCollisionData(const DynArray<Vec3>& meshFaces);
    IVariable* GetLinkedLightClipVolumeVar(GUID targetEntityId) const;

    IStatObj* GetIStatObj() override;
    void DeleteThis() override { delete this; }

    CVariable<bool> mv_filled;
    std::vector<Lineseg> m_MeshEdges;

    int m_nUniqFileIdForExport;
    ClipVolumeObjectPanel* m_pEditClipVolumePanel;
    int m_nEditClipVolumeRollUpID;
};
