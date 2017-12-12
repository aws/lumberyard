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

#include <QWidget>
#include "Serialization/Strings.h"

class QLabel;
class QBoxLayout;
class QSlider;
class QDoubleSpinBox;
class QBoxLayout;
class QViewport;

struct SRenderContext;
namespace Serialization
{
    class IArchive;
}

namespace CharacterTool
{
    using Serialization::string;

    class CharacterDocument;

    class BlendSpacePreview
        : public QWidget
    {
        Q_OBJECT
    public:
        BlendSpacePreview(QWidget* parent, CharacterDocument* document);

        void IdleUpdate();
        void Serialize(Serialization::IArchive& ar);
        QViewport* GetViewport() const { return m_viewport; }
    protected slots:

        void OnRender(const SRenderContext& context);
        void OnResetView();
    private:

        QBoxLayout* m_layout;
        CharacterDocument* m_document;
        QAction* m_actionShowGrid;
        QSlider* m_characterScaleSlider;
        QViewport* m_viewport;
    };
}
