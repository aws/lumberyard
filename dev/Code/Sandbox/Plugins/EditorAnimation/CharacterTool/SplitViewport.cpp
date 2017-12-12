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
#include <ISystem.h>
#include <QBoxLayout>

#include <Cry_Camera.h>
#include "SplitViewport.h"
#include "../EditorCommon/QViewport.h"
#include "Serialization.h"
#include "QViewportSettings.h"

namespace CharacterTool
{
    QSplitViewport::QSplitViewport(QWidget* parent)
        : QWidget(parent)
        , m_isSplit(false)
    {
        setContentsMargins(0, 0, 0, 0);
        m_originalViewport = new QViewport(0);
        m_originalViewport->setVisible(false);
        m_originalViewport->SetUseArrowsForNavigation(false);
        connect(m_originalViewport, SIGNAL(SignalCameraMoved(const QuatT&)), this, SLOT(OnCameraMoved(const QuatT&)));

        m_viewport = new QViewport(0);
        connect(m_viewport, SIGNAL(SignalCameraMoved(const QuatT&)), this, SLOT(OnCameraMoved(const QuatT&)));
        m_viewport->SetUseArrowsForNavigation(false);

        m_layout = new QBoxLayout(QBoxLayout::LeftToRight);
        setLayout(m_layout);
        m_layout->setContentsMargins(0, 0, 0, 0);
        m_layout->addWidget(m_originalViewport, 1);
        m_layout->addWidget(m_viewport, 1);
    }

    void QSplitViewport::Serialize(Serialization::IArchive& ar)
    {
        ar(*m_viewport, "viewport", "Viewport");
        ar(*m_originalViewport, "originalViewport", "Original Viewport");
    }

    void QSplitViewport::SetSplit(bool split)
    {
        if (m_isSplit != split)
        {
            m_isSplit = split;

            m_layout->removeWidget(m_originalViewport);
            m_layout->removeWidget(m_viewport);

            if (m_isSplit)
            {
                m_layout->addWidget(m_originalViewport, 1);
            }
            m_layout->addWidget(m_viewport, 1);
            m_originalViewport->setVisible(m_isSplit);
        }
    }

    void QSplitViewport::FixLayout()
    {
        SetSplit(!m_isSplit);
        SetSplit(!m_isSplit);
    }


    void QSplitViewport::OnCameraMoved(const QuatT& qt)
    {
        if (m_viewport)
        {
            SViewportState state = m_viewport->GetState();
            state.cameraTarget = qt;
            m_viewport->SetState(state);
        }

        if (m_originalViewport)
        {
            SViewportState state = m_viewport->GetState();
            state.cameraTarget = qt;
            m_originalViewport->SetState(state);
        }
    }
}

#include <CharacterTool/SplitViewport.moc>
