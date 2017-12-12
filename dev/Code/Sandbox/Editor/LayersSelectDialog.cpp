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

#include "StdAfx.h"
#include "LayersSelectDialog.h"
#include "Objects/ObjectLayerManager.h"
#include "Controls/LayersListBox.h"
#include <QtUtil.h>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QItemSelectionModel>

// CLayersSelectDialog dialog

CLayersSelectDialog::CLayersSelectDialog(const QPoint& origin, QWidget* parent /*=NULL*/)
    : QDialog(parent)
    , m_selectedLayer("")
    , m_origin(origin)
    , m_layers(nullptr)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Popup);
    m_layers = new CLayersListBox();
    m_layers->header()->hide();

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_layers);

    ReloadLayers();

    connect(m_layers, &CLayersListBox::itemSelected, this, &CLayersSelectDialog::OnLbnSelchangeLayers);
}

CLayersSelectDialog::~CLayersSelectDialog()
{
}

// CLayersSelectDialog message handlers

//////////////////////////////////////////////////////////////////////////
void CLayersSelectDialog::OnLbnSelchangeLayers()
{
    CObjectLayer* pLayer = m_layers->GetCurrentLayer();
    if (pLayer)
    {
        m_selectedLayer = pLayer->GetName();
    }
    else
    {
        m_selectedLayer = "";
    }

    // Layer selected.
    accept();
}

//////////////////////////////////////////////////////////////////////////
void CLayersSelectDialog::ReloadLayers()
{
    move(m_origin.x(), m_origin.y());
    m_layers->ReloadLayers();
    m_layers->SelectLayer(m_selectedLayer);
}

void CLayersSelectDialog::focusOutEvent(QFocusEvent* e)
{
    reject();
}
