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

#include "pch.h"
#include "../EditorCommon/QPropertyTree/QPropertyTree.h"
#include "Expected.h"
#include "Serialization.h"
#include "DisplayParametersPanel.h"
#include <QBoxLayout>
#include <QViewport.h>
#include <ICryAnimation.h>
#include "CharacterDocument.h"


namespace CharacterTool
{
    DisplayParametersPanel::DisplayParametersPanel(QWidget* parent, CharacterDocument* document, Serialization::SContextLink* context)
        : QWidget(parent)
        , m_displayOptions(document->GetDisplayOptions())
        , m_document(document)
    {
        EXPECTED(connect(document, &CharacterDocument::SignalDisplayOptionsChanged, this, &DisplayParametersPanel::OnDisplayOptionsUpdated));
        EXPECTED(connect(document, &CharacterDocument::SignalCharacterLoaded, this, &DisplayParametersPanel::OnDisplayOptionsUpdated));

        QBoxLayout* layout = new QBoxLayout(QBoxLayout::TopToBottom, this);
        layout->setMargin(0);
        layout->setSpacing(0);

        m_propertyTree = new QPropertyTree(this);
        m_propertyTree->setSizeHint(QSize(220, 100));
        m_propertyTree->setExpandLevels(0);
        m_propertyTree->setSliderUpdateDelay(5);
        m_propertyTree->setAutoRevert(false);
        m_propertyTree->setArchiveContext(context);
        m_propertyTree->setValueColumnWidth(0.6f);
        m_propertyTree->attach(Serialization::SStruct(*m_displayOptions));
        EXPECTED(connect(m_propertyTree, &QPropertyTree::signalChanged, this, &DisplayParametersPanel::OnPropertyTreeChanged));
        EXPECTED(connect(m_propertyTree, &QPropertyTree::signalContinuousChange, this, &DisplayParametersPanel::OnPropertyTreeChanged));
        layout->addWidget(m_propertyTree, 1);
    }

    DisplayParametersPanel::~DisplayParametersPanel()
    {
    }

    void DisplayParametersPanel::Serialize(Serialization::IArchive& ar)
    {
        if (ar.Filter(SERIALIZE_STATE))
        {
            ar(*m_propertyTree, "propertyTree");
        }
    }

    void DisplayParametersPanel::OnDisplayOptionsUpdated()
    {
        IDefaultSkeleton* skeleton = 0;

        m_propertyTree->revertNoninterrupting();
    }

    void DisplayParametersPanel::OnPropertyTreeChanged()
    {
        if (m_document)
        {
            m_document->DisplayOptionsChanged();
        }
    }
}

#include <CharacterTool/DisplayParametersPanel.moc>
