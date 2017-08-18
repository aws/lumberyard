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

#pragma once

#include <memory>
#include <QWidget>
#include "DisplayParameters.h"
#include "../EditorCommon/QPropertyTree/ContextList.h"

class QPropertyTree;

namespace CharacterTool
{
    class CharacterDocument;

    class DisplayParametersPanel
        : public QWidget
    {
        Q_OBJECT
    public:
        DisplayParametersPanel(QWidget* parent, CharacterDocument* document, Serialization::SContextLink* context);
        ~DisplayParametersPanel();
        QSize sizeHint() const override { return QSize(240, 100); }
        void Serialize(Serialization::IArchive& ar);

    public slots:
        void OnDisplayOptionsUpdated();
        void OnPropertyTreeChanged();

    private:
        QPropertyTree* m_propertyTree;
        std::shared_ptr<DisplayOptions> m_displayOptions;
        CharacterDocument* m_document;
    };
}