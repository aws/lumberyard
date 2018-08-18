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

#include <platform.h>

#include "stdafx.h"
#include <assert.h>               // assert()
#include "UIMIPControlPanel.h"    // CUIMIPControlPanel
#include "resource.h"
#include "ImageProperties.h"      // CBumpProperties

#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QSlider>

CUIMIPControlPanel::CUIMIPControlPanel()
    : m_hTab(0)
{
    memset(&m_GenIndexToControl, 0, sizeof(m_GenIndexToControl));
    memset(&m_GenControlToIndex, 0, sizeof(m_GenControlToIndex));
}

void CUIMIPControlPanel::InitDialog(QWidget* hWndParent, const bool bShowBumpmapFileName)
{
    m_hTab = hWndParent;

    // Initialise the values.
    for (int i = 0; i < NUM_CONTROLLED_MIP_MAPS; ++i)
    {
        m_MIPAlphaControl[i].SetRange(0.0f, 100.0f);
        m_MIPAlphaControl[i].SetValue(50.0f);
    }

    m_MIPBlurringControl.SetRange(-16.0f, 16.0f);
    m_MIPBlurringControl.SetValue(0.0f);

    // Subclass the sliders and edit controls.
    for (int i = 0; i < NUM_CONTROLLED_MIP_MAPS; ++i)
    {
        QLineEdit* hwndEdit = hWndParent->findChild<QLineEdit*>(QStringLiteral("editMipAlpha%1").arg(i));
        assert(hwndEdit);
        QSlider* hwndSlider = hWndParent->findChild<QSlider*>(QStringLiteral("sliderMipAlpha%1").arg(i));
        assert(hwndSlider);
        m_MIPAlphaControl[i].SubclassWidgets(hwndSlider, hwndEdit);
    }

    {
        QLineEdit* hwndEdit = hWndParent->findChild<QLineEdit*>("editMipBlur");
        assert(hwndEdit);
        QSlider* hwndSlider = hWndParent->findChild<QSlider*>("sliderMipBlur");
        assert(hwndSlider);
        m_MIPBlurringControl.SubclassWidgets(hwndSlider, hwndEdit);
    }

    {
        QComboBox* hwnd = hWndParent->findChild<QComboBox*>("mipMaps");
        assert(hwnd);
        hwnd->addItem("none (0)");
        hwnd->addItem("max, tiled (1)");
        //hwnd->addItem("max, mirrored (2)");
    }

    {
        QComboBox* hwnd = hWndParent->findChild<QComboBox*>("mipMethod");
        assert(hwnd);
        for (int i = 0, c = 0; i < CImageProperties::GetMipGenerationMethodCount(); ++i)
        {
            const char* genStr = (i == 0 ? NULL : CImageProperties::GetMipGenerationMethodName(i));
            if (genStr && genStr[0])
            {
                hwnd->addItem(genStr);

                m_GenIndexToControl[i] = c;
                m_GenControlToIndex[c] = i;

                ++c;
            }
        }
    }

    {
        QComboBox* hwnd = hWndParent->findChild<QComboBox*>("mipOp");
        assert(hwnd);
        for (int i = 0; i < CImageProperties::GetMipGenerationEvalCount(); ++i)
        {
            const char* genStr = CImageProperties::GetMipGenerationEvalName(i);
            if (genStr && genStr[0])
            {
                hwnd->addItem(genStr);
            }
        }
    }
}

void CUIMIPControlPanel::GetDataFromDialog(CImageProperties& rProp)
{
    for (int i = 0; i < NUM_CONTROLLED_MIP_MAPS; ++i)
    {
        rProp.SetMIPAlpha(i, (int)(this->m_MIPAlphaControl[i].GetValue() + 0.5f));
    }

    rProp.SetMipBlurring(this->m_MIPBlurringControl.GetValue());

    {
        QCheckBox* hwnd = m_hTab->findChild<QCheckBox*>("maintainAlphaCoverage");
        assert(hwnd);
        rProp.SetMaintainAlphaCoverage(hwnd->isChecked());
    }

    {
        QComboBox* hwnd = m_hTab->findChild<QComboBox*>("mipMaps");
        assert(hwnd);
        rProp.SetMipMaps(hwnd->currentIndex() > 0 ? 1 : 0);
    }

    {
        QComboBox* hwnd = m_hTab->findChild<QComboBox*>("mipMethod");
        assert(hwnd);
        rProp.SetMipGenerationMethodIndex(m_GenControlToIndex[hwnd->currentIndex()]);
    }

    {
        QComboBox* hwnd = m_hTab->findChild<QComboBox*>("mipOp");
        assert(hwnd);
        rProp.SetMipGenerationEvalIndex(hwnd->currentIndex());
    }
}

void CUIMIPControlPanel::SetDataToDialog(const IConfig* pConfig, const CImageProperties& rProp, const bool bInitalUpdate)
{
    int iMIPAlpha[NUM_CONTROLLED_MIP_MAPS];

    rProp.GetMIPAlphaArray(iMIPAlpha);

    for (int i = 0; i < NUM_CONTROLLED_MIP_MAPS; ++i)
    {
        this->m_MIPAlphaControl[i].SetValue((float)iMIPAlpha[i]);
    }

    this->m_MIPBlurringControl.SetValue(rProp.GetMipBlurring());

    {
        QCheckBox* hwnd = m_hTab->findChild<QCheckBox*>("maintainAlphaCoverage");
        assert(hwnd);
        hwnd->setChecked(rProp.GetMaintainAlphaCoverage());
        hwnd->setEnabled(pConfig->GetAsInt("mc", -12345, -12345, eCP_PriorityAll & (~eCP_PriorityFile)) == -12345);
    }

    {
        QComboBox* hwnd = m_hTab->findChild<QComboBox*>("mipMaps");
        assert(hwnd);
        const int iSel = rProp.GetMipMaps() ? 1 : 0;
        hwnd->setCurrentIndex(iSel);
        hwnd->setEnabled(pConfig->GetAsInt("mipmaps", -12345, -12345, eCP_PriorityAll & (~eCP_PriorityFile)) == -12345);
    }

    {
        QComboBox* hwnd = m_hTab->findChild<QComboBox*>("mipMethod");
        assert(hwnd);
        const int iSel = m_GenIndexToControl[rProp.GetMipGenerationMethodIndex()];
        hwnd->setCurrentIndex(iSel);
        hwnd->setEnabled(azstricmp(pConfig->GetAsString("mipgentype", "<>", "<>", eCP_PriorityAll & (~eCP_PriorityFile)).c_str(), "<>")  == 0);
    }

    {
        QComboBox* hwnd = m_hTab->findChild<QComboBox*>("mipOp");
        assert(hwnd);
        const int iSel = rProp.GetMipGenerationEvalIndex();
        hwnd->setCurrentIndex(iSel);
        hwnd->setEnabled(azstricmp(pConfig->GetAsString("mipgeneval", "<>", "<>", eCP_PriorityAll & (~eCP_PriorityFile)).c_str(), "<>")  == 0);
    }
}
