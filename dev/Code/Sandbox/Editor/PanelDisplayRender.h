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

#ifndef CRYINCLUDE_EDITOR_PANELDISPLAYRENDER_H
#define CRYINCLUDE_EDITOR_PANELDISPLAYRENDER_H

#pragma once
// PanelDisplayRender.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPanelDisplayRender dialog
#include <QWidget>
#include <QScopedPointer>

namespace Ui {
    class CPanelDisplayRender;
}

class CPanelDisplayRender
    : public QWidget
    , public IEditorNotifyListener
{
    Q_OBJECT
public:
    typedef std::map<string, ConsoleVarFunc>     TDVariableNameToConsoleFunction;
    // Construction
public:
    CPanelDisplayRender(QWidget* pParent = nullptr);   // standard constructor
    ~CPanelDisplayRender();

    // Dialog Data
    BOOL    m_roads;
    BOOL    m_decals;
    BOOL    m_detailTex;
    BOOL    m_fog;
    BOOL    m_shadowMaps;
    BOOL    m_staticShadowMaps;
    BOOL    m_skyBox;
    BOOL    m_staticObj;
    BOOL    m_terrain;
    BOOL    m_water;
    BOOL    m_detailObj;
    BOOL    m_particles;
    BOOL    m_geomCaches;
    BOOL    m_clouds;
    BOOL    m_imposters;
    BOOL    m_beams;
    BOOL    m_globalIllum;
    BOOL  m_alphablend;

    BOOL m_dbg_frame_profile;
    BOOL m_dbg_aidebugdraw;
    BOOL m_dbg_aidebugdrawcover;
    BOOL m_dbg_physicsDebugDraw;
    BOOL m_dbg_memory_info;
    BOOL m_dbg_mem_stats;
    BOOL m_dbg_game_token_debugger;
    BOOL m_dbg_renderer_profile_shaders;
    BOOL m_dbg_renderer_overdraw;
    BOOL m_dbg_renderer_resources;
    BOOL m_dbg_drawcalls;
    BOOL m_dbg_highlight_breakable;
    BOOL m_dbg_highlight_missing_surface_type;

protected:
    void UpdateData(bool fromUi = true);

    // Implementation
protected:
    void SetupCallbacks();

    void SetControls();

    virtual void OnEditorNotifyEvent(EEditorNotifyEvent event);

    void showEvent(QShowEvent* event) override;

private slots:
    void OnChangeRenderFlag();
    void OnChangeDebugFlag();
    void OnDisplayAll();
    void OnDisplayNone();
    void OnDisplayInvert();
    void OnFillMode();
    void OnWireframeMode();
    void OnBnClickedToggleHelpers();

private:
    static CPanelDisplayRender*& GetCurrentDisplayRender();

    static void OnDisplayOptionChanged(ICVar* piDisplayModeVariable);
    void OnDisplayOptionChanged();
    void UpdateDisplayOptions();

    static void OnDebugOptionChanged(ICVar* piDisplayModeVariable);
    void OnDebugOptionChanged();
    void UpdateDebugOptions();

    void RegisterChangeCallback(const char* szVariableName, ConsoleVarFunc fnCallbackFunction);







    TDVariableNameToConsoleFunction     m_cVariableNameToConsoleFunction;
    bool                                                            m_boIsUpdatingValues;

    QScopedPointer<Ui::CPanelDisplayRender> ui;
    bool m_initialized;
};

#endif // CRYINCLUDE_EDITOR_PANELDISPLAYRENDER_H
