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
#ifndef CRYINCLUDE__EXAMPLEQMLVIEWPANE_EXAMPLEQTOBJECT_H
#define CRYINCLUDE__EXAMPLEQMLVIEWPANE_EXAMPLEQTOBJECT_H
#pragma once


#include <QObject>
#include <QTimer>

// this serves as an example Qt Object that can be used by the QML to talk to the
// rest of the engine/editor.  in this particular case, its not a singleton type
// but instead, QML makes it.
class ExampleQtObject
    : public QObject
{
    Q_OBJECT

    Q_PROPERTY(float exampleProperty READ exampleProperty WRITE setExampleProperty NOTIFY examplePropertyChanged)

    // an example of a function which can display something from the engine:
    Q_PROPERTY(float fpsValue READ fpsValue NOTIFY fpsValueChanged)

public:
    explicit ExampleQtObject(QObject* parent = 0);

    Q_INVOKABLE void exampleInvokableFunction(float value);
    float exampleProperty() const { return m_propertyValue; }
    float fpsValue() const { return m_lastFPSValue; }

signals:
    void exampleSignal(int value);
    void examplePropertyChanged();
    void fpsValueChanged();

public slots:
    void exampleSlot(QString value);
    void setExampleProperty(float value);

private slots:
    void onTimerElapse();
private:
    float m_propertyValue;
    float m_lastFPSValue;
    QTimer m_fpsSampleTimer;
};


#endif // CRYINCLUDE__EXAMPLEQMLVIEWPANE_EXAMPLEQTOBJECT_H
