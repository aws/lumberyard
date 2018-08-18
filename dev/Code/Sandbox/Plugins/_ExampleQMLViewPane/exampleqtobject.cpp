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
#include "stdafx.h"
#include "exampleqtobject.h"
// manually include moc file:
#include "I3DEngine.h"
#include <exampleqtobject.moc>


ExampleQtObject::ExampleQtObject(QObject* parent)
    : QObject(parent)
{
    m_lastFPSValue = 0.0f;
    m_propertyValue = 0.0f;
    m_fpsSampleTimer.start(250);
    connect(&m_fpsSampleTimer, SIGNAL(timeout()), this, SLOT(onTimerElapse()));
}

void ExampleQtObject::exampleInvokableFunction(float value)
{
    exampleSignal((int)value);
}

void ExampleQtObject::exampleSlot(QString value)
{
    GetIEditor()->GetSystem()->GetILog()->Log("\nValue logged: %s", value.toUtf8().data());
}


void ExampleQtObject::setExampleProperty(float value)
{
    if (m_propertyValue != value)
    {
        m_propertyValue = value;
        examplePropertyChanged();
    }
}

void ExampleQtObject::onTimerElapse()
{
    if (IEditor* pEditor = GetIEditor())
    {
        if (I3DEngine* pEngine = pEditor->Get3DEngine())
        {
            SDebugFPSInfo newDebugInfo;
            pEngine->FillDebugFPSInfo(newDebugInfo);
            if (m_lastFPSValue != newDebugInfo.fAverageFPS)
            {
                m_lastFPSValue = newDebugInfo.fAverageFPS;
                fpsValueChanged();
            }
        }
    }
}
