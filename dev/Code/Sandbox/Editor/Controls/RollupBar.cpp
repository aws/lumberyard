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

// Description : implementation of the CRollupBar class.


#include "StdAfx.h"
#include "RollupBar.h"
#include "MainWindow.h"
#include "MainTools.h"
#include "PanelDisplayLayer.h"
#include "TerrainPanel.h"
#include "PanelDisplayHide.h"
#include "PanelDisplayRender.h"
#include "PanelDisplayLayer.h"
#include "Controls/QRollupCtrl.h"
#include "Modelling/ModellingToolsPanel.h"
#include "EditMode/SubObjSelectionTypePanel.h"
#include "EditMode/SubObjSelectionPanel.h"
#include "EditMode/SubObjDisplayPanel.h"

#include <QtViewPaneManager.h>

CRollupBar::CRollupBar(QWidget* parent)
    : QTabWidget(parent)
    , m_mainTools(new CMainTools())
{
    m_objectRollupCtrl = new QRollupCtrl;
    insertControl(0, m_objectRollupCtrl, "Create");

    if (!MainWindow::instance()->IsPreview())
    {
        m_terrainRollupCtrl = new QRollupCtrl;
        insertControl(1, m_terrainRollupCtrl, "Terrain Editing");

        m_modellingRollupCtrl = new QRollupCtrl;
        insertControl(2, m_modellingRollupCtrl, "Modeling");

        m_displayRollupCtrl = new QRollupCtrl;
        insertControl(3, m_displayRollupCtrl, "Display Settings");

        //////////////////////////////////////////////////////////////////////////
        // Insert the main rollup
        m_objectRollupCtrl->addItem(m_mainTools, "Objects");

        m_modellingRollupCtrl->addItem(new CModellingToolsPanel, "Modelling");
        m_modellingRollupCtrl->addItem(new CSubObjSelectionTypePanel, "Selection Type");
        m_modellingRollupCtrl->addItem(new CSubObjSelectionPanel, "Selection");
        m_modellingRollupCtrl->addItem(new CSubObjDisplayPanel, "Display Selection");

        m_terrainPanel = new CTerrainPanel();
        m_terrainRollupCtrl->addItem(m_terrainPanel, "Terrain");

        m_displayRollupCtrl->addItem(new CPanelDisplayHide, "Hide by Category");
        m_displayRollupCtrl->addItem(new CPanelDisplayRender, "Render Settings");

        m_pLayerPanel = new CPanelDisplayLayer();
        insertControl(4, m_pLayerPanel, "Layers");

        m_objectRollupCtrl->expandAllPages(true);
        m_terrainRollupCtrl->expandAllPages(true);
        m_modellingRollupCtrl->expandAllPages(true);
        m_displayRollupCtrl->expandAllPages(true);
    }
    else
    {
        // Hide all menus.
    }

    setCurrentIndex(ROLLUP_OBJECTS);
}

void CRollupBar::RegisterViewClass()
{
    AzToolsFramework::ViewPaneOptions opts;
    opts.preferedDockingArea = Qt::RightDockWidgetArea;
    opts.isDeletable = false;
    opts.isStandard = GetIEditor()->IsLegacyUIEnabled();
    opts.showInMenu = true;// doesn't appear in "View->Open View Pane", but will appear in the new menu layout's Tools menu
    opts.canHaveMultipleInstances = true;
    opts.builtInActionId = ID_VIEW_ROLLUPBAR;
    opts.sendViewPaneNameBackToAmazonAnalyticsServers = true;
    opts.isLegacy = true;
    AzToolsFramework::RegisterViewPane<CRollupBar>(LyViewPane::LegacyRollupBar, LyViewPane::CategoryTools, opts);
}

void CRollupBar::insertControl(int index, QWidget* w, const QString& tooltip)
{
    QIcon icon(QStringLiteral(":/RollupBar/icons/rollup_%1.png").arg(index));
    QTabWidget::insertTab(index, w, icon, QString());
    setTabToolTip(index, tooltip);
}

QRollupCtrl* CRollupBar::GetRollUpControl(int rollupBarId) const
{
    switch (rollupBarId)
    {
    case ROLLUP_OBJECTS:
    default:
        return m_objectRollupCtrl;
    case ROLLUP_TERRAIN:
        return m_terrainRollupCtrl;
    case ROLLUP_MODELLING:
        return m_modellingRollupCtrl;
    case ROLLUP_DISPLAY:
        return m_displayRollupCtrl;
    }
}

#include <Controls/RollupBar.moc>
