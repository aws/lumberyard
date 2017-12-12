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

#include "../../EditorCommon/ManipScene.h"

class QLabel;
class QLineEdit;
class QComboBox;

namespace CharacterTool
{
    class TransformPanel
        : public QWidget
    {
        Q_OBJECT
    public:
        TransformPanel();

        void SetTransform(const QuatT& transform);
        const QuatT& Transform() const{ return m_transform; }

        void SetMode(Manip::ETransformationMode mode);
        void SetEnabled(bool enabled);

        void SetSpace(Manip::ETransformationSpace space);
        Manip::ETransformationSpace Space() const{ return m_space; }

    signals:
        void SignalSpaceChanged();
        void SignalChanged();
        void SignalChangeFinished();

    protected slots:
        void OnSpaceChanged();
        void OnEditXChanged();
        void OnEditXEditingFinished();
        void OnEditYChanged();
        void OnEditYEditingFinished();
        void OnEditZChanged();
        void OnEditZEditingFinished();

    private:
        bool ReadEdit(int axis);
        void WriteEdits();

        Manip::ETransformationSpace m_space;
        Manip::ETransformationMode m_mode;
        QComboBox* m_comboSpace;
        QLabel* m_labelX;
        QLabel* m_labelY;
        QLabel* m_labelZ;
        QLineEdit* m_editX;
        QLineEdit* m_editY;
        QLineEdit* m_editZ;
        QuatT m_transform;
        Ang3 m_rotationAngles;
    };
}
