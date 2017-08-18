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

// Description : Terrain MiniMap tool.


#ifndef CRYINCLUDE_EDITOR_TERRAINMINIMAPTOOL_H
#define CRYINCLUDE_EDITOR_TERRAINMINIMAPTOOL_H

#pragma once

#include "Mission.h"
#include <QScopedPointer>
#include <QWidget>

//////////////////////////////////////////////////////////////////////////
class CTerrainMiniMapTool
    : public CEditTool
    , public IEditorNotifyListener
    , public IScreenshotCallback
{
    Q_OBJECT
public:
    Q_INVOKABLE CTerrainMiniMapTool();
    virtual ~CTerrainMiniMapTool();

    //////////////////////////////////////////////////////////////////////////
    // CEditTool
    //////////////////////////////////////////////////////////////////////////
    virtual void BeginEditParams(IEditor* ie, int flags);
    virtual void EndEditParams();
    virtual void Display(DisplayContext& dc);
    virtual bool MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags);
    virtual void DeleteThis() { delete this; };
    //////////////////////////////////////////////////////////////////////////

    // IEditorNotifyListener
    virtual void OnEditorNotifyEvent(EEditorNotifyEvent event);

    SMinimapInfo GetMinimap(){ return m_minimap; }
    QString GetPath(){ return m_path; }
    void SetPath(const QString& path){ m_path = path; }
    QString GetFilename(){ return m_filename; }
    void SetFilename(const QString& filename){ m_filename = filename; }
    void SetOrientation(int orientation) { m_minimap.orientation = orientation; }

    void SetResolution(int nResolution);
    void SetCameraHeight(float fHeight);
    void AddToLevelFolder();
    void Generate(bool bHideProxy = true);

    // IScreenshotCallback
    void SendParameters(void* data, uint32 width, uint32 height, f32 minx, f32 miny, f32 maxx, f32 maxy);

    void LoadSettingsXML();

    virtual void ResetToDefault();

protected:
    SMinimapInfo m_minimap;

private:

    bool m_bDragging;

    QString m_path;
    QString m_filename; // without path and extension

    std::map<string, float> m_ConstClearList;
    bool b_stateScreenShot;
};


#endif // CRYINCLUDE_EDITOR_TERRAINMINIMAPTOOL_H
