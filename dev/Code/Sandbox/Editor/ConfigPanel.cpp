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
#include "ConfigPanel.h"
#include "ConfigGroup.h"
#include <QtUtil.h>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>

//-----------------------------------------------------------------------------

CConfigPanel::CItem::CItem(Config::IConfigVar* pVar, QWidget* widget)
    : m_pVar(pVar)
    , m_widget(widget)
{
}

CConfigPanel::CItem::~CItem()
{
}

//-----------------------------------------------------------------------------

CConfigPanel::CConfigPanel(QWidget* pParent /*= nullptr*/)
    : QWidget(pParent)
{
    GetIEditor()->RegisterNotifyListener(this);

    new QVBoxLayout(this);
}

CConfigPanel::~CConfigPanel()
{
    GetIEditor()->UnregisterNotifyListener(this);
}

void CConfigPanel::DisplayGroup(Config::CConfigGroup* pGroup, const char* szGroupName)
{
    QGroupBox* pGroupBox = new QGroupBox();
    pGroupBox->setTitle(szGroupName);

    layout()->addWidget(pGroupBox);
    QVBoxLayout* groupLayout = new QVBoxLayout(pGroupBox);

    // Create the items
    const uint32 count = pGroup->GetVarCount();
    for (uint32 i = 0; i < count; ++i)
    {
        Config::IConfigVar* pVar = pGroup->GetVar(i);

        // allocate ID
        const uint32 id = m_currentId++;

        // create control
        switch (pVar->GetType())
        {
        case Config::IConfigVar::eType_BOOL:
        {
            QCheckBox* checkBox = new QCheckBox(QtUtil::ToQString(pVar->GetDescription()));
            connect(checkBox, &QCheckBox::clicked, this, &CConfigPanel::OnOptionChanged);
            groupLayout->addWidget(checkBox, 0, Qt::AlignLeft);

            // get value
            bool bIsChecked = false;
            pVar->Get(&bIsChecked);
            checkBox->setChecked(bIsChecked);

            CItem* pItem = new CItem(pVar, checkBox);
            m_items[checkBox].reset(pItem);

            break;
        }

        case Config::IConfigVar::eType_INT:
        case Config::IConfigVar::eType_STRING:
        case Config::IConfigVar::eType_FLOAT:
        {
            QLabel* label = new QLabel(QtUtil::ToQString(pVar->GetDescription() + ":"));
            QLineEdit* lineEdit = new QLineEdit;
            connect(lineEdit, &QLineEdit::textChanged, this, &CConfigPanel::OnTextChanged);

            QHBoxLayout* hLayout = new QHBoxLayout;
            groupLayout->addLayout(hLayout);
            hLayout->addWidget(label);
            hLayout->addWidget(lineEdit);

            // get value
            if (pVar->GetType() == Config::IConfigVar::eType_INT)
            {
                int iValue = 0;
                pVar->Get(&iValue);
                lineEdit->setText(QString::number(i));
            }
            else if (pVar->GetType() == Config::IConfigVar::eType_FLOAT)
            {
                float fValue = 0.0f;
                pVar->Get(&fValue);
                lineEdit->setText(QString::number(i));
            }
            else if (pVar->GetType() == Config::IConfigVar::eType_STRING)
            {
                string sValue;
                pVar->Get(&sValue);
                lineEdit->setText(sValue.c_str());
            }

            CItem* pItem = new CItem(pVar, lineEdit);
            m_items[lineEdit].reset(pItem);

            break;
        }
        }
    }
}

void CConfigPanel::OnOptionChanged(bool checked)
{
    TItemMap::const_iterator it = m_items.find(sender());
    if (it != m_items.end())
    {
        CItem* pItem = it->second.data();
        if (pItem->m_pVar->GetType() == Config::IConfigVar::eType_BOOL)
        {
            pItem->m_pVar->Set(&checked);
            OnConfigValueChanged(pItem->m_pVar);
        }
    }
}

void CConfigPanel::OnTextChanged(const QString& text)
{
    TItemMap::const_iterator it = m_items.find(sender());
    if (it != m_items.end())
    {
        CItem* pItem = it->second.data();
        switch (pItem->m_pVar->GetType())
        {
        case Config::IConfigVar::eType_FLOAT:
        {
            bool ok = false;
            float newValue = text.toFloat(&ok);
            if (ok)
            {
                pItem->m_pVar->Set(&newValue);
                OnConfigValueChanged(pItem->m_pVar);
            }

            break;
        }

        case Config::IConfigVar::eType_INT:
        {
            // get text from the window
            bool ok = false;
            int newValue = text.toInt(&ok);
            if (ok)
            {
                pItem->m_pVar->Set(&newValue);
                OnConfigValueChanged(pItem->m_pVar);
            }

            break;
        }

        case Config::IConfigVar::eType_STRING:
        {
            // get text from the window
            const string newValue(text.toUtf8().constData());
            pItem->m_pVar->Set(&newValue);
            OnConfigValueChanged(pItem->m_pVar);

            break;
        }
        }
    }
}


void CConfigPanel::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
}

