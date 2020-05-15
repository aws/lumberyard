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
#include "TransformPanel.h"
#include <QBoxLayout>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include "../../EditorCommon/ManipScene.h"
#include "../Expected.h"

namespace CharacterTool
{
    TransformPanel::TransformPanel()
        : m_mode(Manip::MODE_TRANSLATE)
        , m_transform(IDENTITY)
        , m_space(Manip::SPACE_WORLD)
    {
        setContentsMargins(0, 0, 0, 0);

        QBoxLayout* layout = new QBoxLayout(QBoxLayout::LeftToRight);

        layout->setSpacing(2);
        m_comboSpace = new QComboBox();
        m_comboSpace->addItem("Global");
        m_comboSpace->addItem("Local");
        m_comboSpace->setCurrentIndex(0);
        EXPECTED(connect(m_comboSpace, SIGNAL(currentIndexChanged(int)), this, SLOT(OnSpaceChanged())));
        layout->addWidget(m_comboSpace);

        m_labelX = new QLabel("X:");
        layout->addWidget(m_labelX);
        m_editX = new QLineEdit();
        m_editX->setAlignment(Qt::AlignRight);
        m_editX->setFixedWidth(60);
        m_editX->setValidator(new QDoubleValidator());
        EXPECTED(connect(m_editX, SIGNAL(textEdited(const QString&)), this, SLOT(OnEditXChanged())));
        EXPECTED(connect(m_editX, SIGNAL(editingFinished()), this, SLOT(OnEditXEditingFinished())));
        layout->addWidget(m_editX);

        m_labelY = new QLabel("Y:");
        layout->addWidget(m_labelY);
        m_editY = new QLineEdit();
        m_editY->setAlignment(Qt::AlignRight);
        m_editY->setFixedWidth(60);
        m_editY->setValidator(new QDoubleValidator());
        EXPECTED(connect(m_editY, SIGNAL(textEdited(const QString&)), this, SLOT(OnEditYChanged())));
        EXPECTED(connect(m_editY, SIGNAL(editingFinished()), this, SLOT(OnEditYEditingFinished())));
        layout->addWidget(m_editY);

        m_labelZ = new QLabel("Z:");
        layout->addWidget(m_labelZ);
        m_editZ = new QLineEdit();
        m_editZ->setAlignment(Qt::AlignRight);
        m_editZ->setValidator(new QDoubleValidator());
        m_editZ->setFixedWidth(60);
        EXPECTED(connect(m_editZ, SIGNAL(textEdited(const QString&)), this, SLOT(OnEditZChanged())));
        EXPECTED(connect(m_editZ, SIGNAL(editingFinished()), this, SLOT(OnEditZEditingFinished())));
        layout->addWidget(m_editZ);
        setLayout(layout);
    }

    void TransformPanel::OnSpaceChanged()
    {
        if (m_comboSpace->currentIndex() == 0)
        {
            m_space = Manip::SPACE_WORLD;
        }
        else
        {
            m_space = Manip::SPACE_LOCAL;
        }
        SignalSpaceChanged();
    }

    void TransformPanel::OnEditXChanged()
    {
    }

    void TransformPanel::OnEditYChanged()
    {
    }

    void TransformPanel::OnEditZChanged()
    {
    }

    void TransformPanel::OnEditXEditingFinished()
    {
        if (ReadEdit(0))
        {
            SignalChangeFinished();
        }
    }

    void TransformPanel::OnEditYEditingFinished()
    {
        if (ReadEdit(1))
        {
            SignalChangeFinished();
        }
    }

    void TransformPanel::OnEditZEditingFinished()
    {
        if (ReadEdit(2))
        {
            SignalChangeFinished();
        }
    }

    void TransformPanel::SetTransform(const QuatT& transform)
    {
        m_transform = transform;

        WriteEdits();
    }

    void TransformPanel::SetMode(Manip::ETransformationMode mode)
    {
        m_mode = mode;
        if (m_mode == Manip::MODE_TRANSLATE)
        {
            m_transform.q.SetIdentity();
        }
        else
        {
            m_transform.t = ZERO;
        }
    }

    void TransformPanel::SetEnabled(bool enabled)
    {
        m_comboSpace->setEnabled(enabled);
        m_editX->setEnabled(enabled);
        m_editY->setEnabled(enabled);
        m_editZ->setEnabled(enabled);
        m_labelX->setEnabled(enabled);
        m_labelY->setEnabled(enabled);
        m_labelZ->setEnabled(enabled);
    }

    static QString FixLeadingMinus(const QString& str)
    {
        if (str == "-0.000")
        {
            return "0.000";
        }
        if (str == "-0.00")
        {
            return "0.00";
        }
        return str;
    }

    void TransformPanel::WriteEdits()
    {
        QString x, y, z;

        if (m_mode == Manip::MODE_TRANSLATE)
        {
            x.sprintf("%.3f", m_transform.t.x);
            y.sprintf("%.3f", m_transform.t.y);
            z.sprintf("%.3f", m_transform.t.z);
        }
        else if (m_mode == Manip::MODE_ROTATE)
        {
            m_rotationAngles = Ang3::GetAnglesXYZ(m_transform.q);

            x.sprintf("%.2f", RAD2DEG(m_rotationAngles.x));
            y.sprintf("%.2f", RAD2DEG(m_rotationAngles.y));
            z.sprintf("%.2f", RAD2DEG(m_rotationAngles.z));
        }

        m_editX->setText(FixLeadingMinus(x));
        m_editY->setText(FixLeadingMinus(y));
        m_editZ->setText(FixLeadingMinus(z));
    }

    bool TransformPanel::ReadEdit(int axis)
    {
        QLineEdit* edits[3] = { m_editX, m_editY, m_editZ };

        float value = aznumeric_cast<float>(atof(edits[axis]->text().toLocal8Bit().data()));

        QuatT oldTransform = m_transform;

        if (m_mode == Manip::MODE_TRANSLATE)
        {
            m_transform.t[axis] = value;
        }
        else if (m_mode == Manip::MODE_ROTATE)
        {
            m_rotationAngles[axis] = DEG2RAD(value);
            m_transform = QuatT(Quat(m_rotationAngles), m_transform.t);
        }

        return !QuatT::IsEquivalent(oldTransform, m_transform);
    }

    void TransformPanel::SetSpace(Manip::ETransformationSpace space)
    {
        m_space = space;
        m_comboSpace->setCurrentIndex(space == Manip::SPACE_WORLD ? 0 : 1);
    }
}

#include <CharacterTool/TransformPanel.moc>
