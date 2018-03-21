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

#include "StdAfx.h"
#include <assert.h>               // assert()
#include "UIBumpmapPanel.h"       // CUIBumpmapPanel
#include "resource.h"
#include "ImageProperties.h"      // CBumpProperties
#include "ImageUserDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QLineEdit>
#include <QSlider>
#include <QTabWidget>

CUIBumpmapPanel::CUIBumpmapPanel()
    : m_hTab_Normalmapgen(0)
{
    m_pDlg = NULL;
    // Reflect slider bump notifications back to the control.
    this->reflectIDs.insert(IDC_NM_FILTERTYPE);
    this->reflectIDs.insert(IDC_NM_SLIDERBUMPBLUR);
    this->reflectIDs.insert(IDC_EDIT_BUMPBLUR);
    this->reflectIDs.insert(IDC_NM_SLIDERBUMPSTRENGTH);
    this->reflectIDs.insert(IDC_EDIT_BUMPSTRENGTH);
    this->reflectIDs.insert(IDC_NM_BUMPINVERT);

    memset(&m_GenIndexToControl, 0, sizeof(m_GenIndexToControl));
    memset(&m_GenControlToIndex, 0, sizeof(m_GenControlToIndex));
}

template<typename T = QWidget*>
T findWidget(QWidget* parent, const QString& name)
{
    QTabWidget* tw = parent->window()->findChild<QTabWidget*>();
    QString suffix = tw->tabText(tw->indexOf(parent)).contains("RGB") ? "RGB" : "Alpha";
    QString n = QStringLiteral("%1_%2").arg(name, suffix);
    return parent->findChild<T>(n);
}

void CUIBumpmapPanel::InitDialog(QWidget* hWndParent, const bool bShowBumpmapFileName)
{
    // Initialise the values.
    m_bumpBlurValue.SetValue(0.0f);
    m_bumpBlurValue.SetMin(0.0f);
    m_bumpBlurValue.SetMax(10.0f);
    m_bumpStrengthValue.SetValue(0.0f);
    m_bumpStrengthValue.SetMin(0.0f);
    m_bumpStrengthValue.SetMax(15.0f);
    m_bumpInvertValue.SetValue(false);

    m_hTab_Normalmapgen = hWndParent;

    {
        QWidget* hwnd = findWidget(hWndParent, "srfoAddBumpText0");
        assert(hwnd);

        hwnd->setVisible(bShowBumpmapFileName);

        hwnd = findWidget(hWndParent, "srfoAddBumpName");
        assert(hwnd);

        hwnd->setVisible(bShowBumpmapFileName);

        hwnd = findWidget(hWndParent, "srfoAddBumpChoose");
        assert(hwnd);

        hwnd->setVisible(bShowBumpmapFileName);
        QObject::connect(qobject_cast<QAbstractButton*>(hwnd), &QAbstractButton::clicked, [&](){ ChooseAddBump(m_hTab_Normalmapgen->window()); });
    }

    {
        auto hwnd = findWidget<QSlider*>(hWndParent, "nmSliderBumpStrength");
        assert(hwnd);
        m_bumpStrengthSlider.SetDocument(&m_bumpStrengthValue);
        m_bumpStrengthSlider.SubclassWidget(hwnd);
    }

    {
        auto hwnd = findWidget<QSlider*>(hWndParent, "nmSliderBumpBlur");
        assert(hwnd);
        m_bumpBlurSlider.SetDocument(&m_bumpBlurValue);
        m_bumpBlurSlider.SubclassWidget(hwnd);
    }

    {
        auto hwnd = findWidget<QLineEdit*>(hWndParent, "editBumpBlur");
        assert(hwnd);
        m_bumpBlurEdit.SetDocument(&m_bumpBlurValue);
        m_bumpBlurEdit.SubclassWidget(hwnd);
    }

    {
        auto hwnd = findWidget<QLineEdit*>(hWndParent, "editBumpStrength");
        assert(hwnd);
        m_bumpStrengthEdit.SetDocument(&m_bumpStrengthValue);
        m_bumpStrengthEdit.SubclassWidget(hwnd);
    }

    {
        auto hwnd = findWidget<QCheckBox*>(hWndParent, "nmBumpInvert");
        assert(hwnd);
        m_bumpInvertCheckBox.SetDocument(&m_bumpInvertValue);
        m_bumpInvertCheckBox.SubclassWidget(hwnd);
    }

    // bump to normal map conversion
    {
        auto hwnd = findWidget<QComboBox*>(hWndParent, "nmFilterType");
        assert(hwnd);
        for (int i = 0, c = 0; i < CBumpProperties::GetBumpToNormalFilterCount(); ++i)
        {
            const char* const genStr = (i == 0 ? "None (off)" : CBumpProperties::GetBumpToNormalFilterName(i));
            if (genStr && genStr[0])
            {
                hwnd->addItem(genStr);

                m_GenIndexToControl[i] = c;
                m_GenControlToIndex[c] = i;

                ++c;
            }
        }
    }
}

void CUIBumpmapPanel::UpdateBumpStrength(const CBumpProperties& rBumpProp)
{
    const float fValue = rBumpProp.GetBumpStrengthAmount();

    this->m_bumpStrengthValue.SetValue(abs(fValue));
}


void CUIBumpmapPanel::UpdateBumpBlur(const CBumpProperties& rBumpProp)
{
    this->m_bumpBlurValue.SetValue(rBumpProp.GetBumpBlurAmount());
}


void CUIBumpmapPanel::UpdateBumpInvert(const CBumpProperties& rBumpProp)
{
    const float fValue = rBumpProp.GetBumpStrengthAmount();

    this->m_bumpInvertValue.SetValue(fValue < 0);
}


void CUIBumpmapPanel::GetDataFromDialog(CBumpProperties& rBumpProp)
{
    {
        const bool bInverted = this->m_bumpInvertValue.GetValue();
        const float fValue = abs(this->m_bumpStrengthValue.GetValue());

        rBumpProp.SetBumpStrengthAmount(bInverted ? -fValue : fValue);
    }

    rBumpProp.SetBumpBlurAmount(this->m_bumpBlurValue.GetValue());


    {
        const auto hwnd = findWidget<QComboBox*>(m_hTab_Normalmapgen, "nmFilterType");
        assert(hwnd);
        rBumpProp.SetBumpToNormalFilterIndex(m_GenControlToIndex[hwnd->currentIndex()]);
    }

    {
        QString str;

        auto dlgitem = findWidget<QLineEdit*>(m_hTab_Normalmapgen, "srfoAddBumpName");

        str = dlgitem->text();

        rBumpProp.SetBumpmapName(str.toUtf8().data());
    }
}


void CUIBumpmapPanel::ChooseAddBump(QWidget* hParentWnd)
{
    findWidget<QLineEdit*>(m_hTab_Normalmapgen, "srfoAddBumpName")->setText(QFileDialog::getOpenFileName(hParentWnd, "Open optional Bumpmap (.TIF)", QString(), "TIF Files (*.TIF)"));
}

void CUIBumpmapPanel::SetDataToDialog(const CBumpProperties& rBumpProp, const bool bInitalUpdate)
{
    // sub window normalmapgen
    QWidget* hwnd = findWidget(m_hTab_Normalmapgen, "nmFilterType");
    assert(hwnd);
    const int iSel = m_GenIndexToControl[rBumpProp.GetBumpToNormalFilterIndex()];
    qobject_cast<QComboBox*>(hwnd)->setCurrentIndex(iSel);
    rBumpProp.EnableWindowBumpToNormalFilter(hwnd);

    if (bInitalUpdate)
    {
        UpdateBumpStrength(rBumpProp);
        UpdateBumpBlur(rBumpProp);
        UpdateBumpInvert(rBumpProp);
    }

    hwnd = findWidget(m_hTab_Normalmapgen, "editBumpStrength");
    assert(hwnd);
    rBumpProp.EnableWindowBumpStrengthAmount(hwnd);
    hwnd = findWidget(m_hTab_Normalmapgen, "nmSliderBumpStrength");
    assert(hwnd);
    rBumpProp.EnableWindowBumpStrengthAmount(hwnd);
    hwnd = findWidget(m_hTab_Normalmapgen, "nmBumpInvert");
    assert(hwnd);
    rBumpProp.EnableWindowBumpStrengthAmount(hwnd);

    hwnd = findWidget(m_hTab_Normalmapgen, "nmSliderBumpBlur");
    assert(hwnd);
    rBumpProp.EnableWindowBumpBlurAmount(hwnd);
    hwnd = findWidget(m_hTab_Normalmapgen, "editBumpBlur");
    assert(hwnd);
    rBumpProp.EnableWindowBumpBlurAmount(hwnd);


    {
        QLineEdit* dlgitem = findWidget<QLineEdit*>(m_hTab_Normalmapgen, "srfoAddBumpName");

        dlgitem->setText(rBumpProp.GetBumpmapName().c_str());
    }
}
