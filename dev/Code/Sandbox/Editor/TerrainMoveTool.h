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

// Description : Terrain modification tool.


#ifndef CRYINCLUDE_EDITOR_TERRAINMOVETOOL_H
#define CRYINCLUDE_EDITOR_TERRAINMOVETOOL_H

#pragma once


struct SMTBox
{
    bool isShow;
    bool isSelected;
    bool isCreated;
    Vec3 pos;

    SMTBox()
    {
        isShow = false;
        isSelected = false;
        isCreated = false;
        pos = Vec3(0, 0, 0);
    }
};


//////////////////////////////////////////////////////////////////////////
class CTerrainMoveTool
    : public CEditTool
{
    Q_OBJECT
public:
    Q_INVOKABLE CTerrainMoveTool();
    virtual ~CTerrainMoveTool();

    virtual void BeginEditParams(IEditor* ie, int flags);
    virtual void EndEditParams();

    virtual void Display(DisplayContext& dc);

    // Ovverides from CEditTool
    bool MouseCallback(CViewport* pView, EMouseEvent event, QPoint& point, int flags);

    // Key down.
    bool OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags);
    bool OnKeyUp(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags);

    void OnManipulatorDrag(CViewport* view, ITransformManipulator* pManipulator, QPoint& p0, QPoint& p1, const Vec3& value) override;

    // Delete itself.
    void DeleteThis() { delete this; };

    void Move(bool isCopy = false, bool bOnlyVegetation = true, bool bOnlyTerrain = true);

    void SetDym(Vec3 dym);
    Vec3 GetDym()   {   return m_dym; }

    void SetTargetRot(ImageRotationDegrees targetRot);
    ImageRotationDegrees GetTargetRot()  {   return m_targetRot; }

    void SetSyncHeight(bool isSyncHeight);
    bool GetSyncHeight() { return m_isSyncHeight; }

    // 0 - unselect all, 1 - select source, 2 - select target
    void Select(int nBox);

    void SetArchive(CXmlArchive* ar);

    bool IsNeedMoveTool() { return true; };

private:
    CXmlArchive* m_archive;

    // !!!WARNING
    QRect m_srcRect;

    //static SMTBox m_source;
    //static SMTBox m_target;
    SMTBox m_source;
    SMTBox m_target;

    bool m_isSyncHeight;

    static Vec3 m_dym;
    static ImageRotationDegrees m_targetRot;

    int m_panelId;
    class CTerrainMoveToolPanel* m_panel;

    IEditor* m_ie;

    friend class CUndoTerrainMoveTool;
    friend class CTerrainMoveToolPanel;

    void OffsetTrackViewPositionKeys(const Vec3& offset);
    void UpdateOffsetDisplay();
};


#endif // CRYINCLUDE_EDITOR_TERRAINMOVETOOL_H
