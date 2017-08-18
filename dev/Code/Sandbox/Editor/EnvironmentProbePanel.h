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

#ifndef CRYINCLUDE_EDITOR_ENVIRONMENTPROBEPANEL_H
#define CRYINCLUDE_EDITOR_ENVIRONMENTPROBEPANEL_H

#pragma once

#include <QWidget>
#include <QScopedPointer>

class CEnvironementProbeObject;

namespace Ui {
    class CEnvironmentProbePanel;
}

class CEnvironmentProbePanel
    : public QWidget
{
public:
    CEnvironmentProbePanel(QWidget* parent = nullptr);

    void SetEntity(CEnvironementProbeObject* pEntity);
    void OnGenerateCubemap();
    void OnGenerateAllCubemaps();

private:
    CEnvironementProbeObject* m_pEntity;
    QScopedPointer<Ui::CEnvironmentProbePanel> ui;
};

#endif // CRYINCLUDE_EDITOR_ENVIRONMENTPROBEPANEL_H
