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
#include "RoadPanel.h"

#include "Objects/RoadObject.h"
#include <ui_RoadPanel.h>

//////////////////////////////////////////////////////////////////////////
CRoadPanel::CRoadPanel(QWidget* pParent /* = nullptr */)
    : QWidget(pParent)
    , ui(new Ui::CRoadPanel)
{
    ui->setupUi(this);
    connect(ui->ALIGN_HM, &QPushButton::clicked, this, &CRoadPanel::OnAlignHeightMap);
    connect(ui->ROAD_ERASEVEG, &QPushButton::clicked, this, &CRoadPanel::OnEraseVegetation);
}

//////////////////////////////////////////////////////////////////////////
void CRoadPanel::OnAlignHeightMap()
{
    if (m_pRoad->GetPointCount() > 0)
    {
        m_pRoad->AlignHeightMap();
    }
}

//////////////////////////////////////////////////////////////////////////
void CRoadPanel::OnEraseVegetation()
{
    m_pRoad->EraseVegetation();
}