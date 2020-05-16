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

#include "../EditorCommon/EditorCommonAPI.h"
class EDITOR_COMMON_API QViewport;

class QBoxLayout;
template<class T>
struct QuatT_tpl;
typedef QuatT_tpl<float> QuatT;

namespace Serialization {
    class IArchive;
}

namespace CharacterTool
{
    class QSplitViewport
        : public QWidget
    {
        Q_OBJECT
    public:
        QSplitViewport(QWidget* parent);

        QViewport* OriginalViewport() { return m_originalViewport; }
        QViewport* CompressedViewport() { return m_viewport; }

        bool IsSplit() const{ return m_isSplit; }
        void SetSplit(bool setSplit);
        void Serialize(Serialization::IArchive& ar);
        void FixLayout();
    protected slots:
        void OnCameraMoved(const QuatT& m);
    private:
        QViewport* m_originalViewport;
        QViewport* m_viewport;
        QBoxLayout* m_layout;
        bool m_isSplit;
    };
}
