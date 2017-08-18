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

#pragma once

#include <QVBoxLayout>
#include <AzCore/std/functional.h>

template<class StateEnum>
class StatefulLayout
    : public QVBoxLayout
{
public:

    StatefulLayout(QWidget* parent)
    {
        setSpacing(0);
        setMargin(0);
        parent->setLayout(this);
    }

    void SetWidget(StateEnum newState, AZStd::function<QWidget* ()> factory)
    {
        if (m_widget && newState == m_currentState)
        {
            return;
        }

        if (m_widget)
        {
            removeWidget(m_widget);
            delete m_widget;
        }

        m_widget = factory();

        if (m_widget)
        {
            addWidget(m_widget);
        }

        m_currentState = newState;
    }

    StateEnum CurrentState()
    {
        return m_currentState;
    }

private:

    QWidget* m_widget {
        nullptr
    };
    StateEnum m_currentState;
};

