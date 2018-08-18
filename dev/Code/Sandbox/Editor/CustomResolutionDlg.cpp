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

// Description : A dialog for getting a resolution info from users
// Notice      : Refer to ViewportTitleDlg cpp for a use case


#include "StdAfx.h"
#include "CustomResolutionDlg.h"
#include <QTextStream>

#include <ui_CustomResolutionDlg.h>

#define MIN_RES 64

CCustomResolutionDlg::CCustomResolutionDlg(int w, int h, QWidget* pParent /*=NULL*/)
    : QDialog(pParent)
    , m_wDefault(w)
    , m_hDefault(h)
    , m_ui(new Ui::CustomResolutionDlg)
{
    m_ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    OnInitDialog();
}

CCustomResolutionDlg::~CCustomResolutionDlg()
{
}

void CCustomResolutionDlg::OnInitDialog()
{
    int maxRes = GetIEditor()->GetRenderer()->GetMaxSquareRasterDimension();
    m_ui->m_width->setRange(MIN_RES, maxRes);
    m_ui->m_width->setValue(m_wDefault);

    m_ui->m_height->setRange(MIN_RES, maxRes);
    m_ui->m_height->setValue(m_hDefault);

    QString maxDimensionString;
    QTextStream(&maxDimensionString) << "Maximum Dimension: " << maxRes;
    m_ui->m_maxDimension->setText(maxDimensionString);
}

int CCustomResolutionDlg::GetWidth() const
{
    return m_ui->m_width->value();
}

int CCustomResolutionDlg::GetHeight() const
{
    return m_ui->m_height->value();
}

#include <CustomResolutionDlg.moc>