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

#include <AzQtComponents/AzQtComponentsAPI.h>

#include <QWidget>
#include <QVector3D>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QStringList>
#include <QColor>

class QPixmap;

namespace AzQtComponents
{
    class AZ_QT_COMPONENTS_API VectorEditElement
        : public QWidget
    {
        Q_OBJECT

    public:
        // TODO: maybe share them somewhere (in a gadget)
        enum Flavor
        {
            Plain = 0,
            Information,
            Question,
            Invalid,
            Valid
        };
        Q_ENUM(Flavor)
        explicit VectorEditElement(const QString& label, QWidget* parent = nullptr);

        float value() const;
        void setValue(float);
        QString label() const;
        void setLabel(const QString&);
        QColor color() const;
        void setColor(const QColor&);
        void setFlavor(VectorEditElement::Flavor flavor);

    Q_SIGNALS:
        void valueChanged();

    private:
        void updateColor();
        QLineEdit* const m_lineEdit;
        QLabel* const m_label;
        QColor m_color;
        Flavor m_flavor;
    };

    class AZ_QT_COMPONENTS_API VectorEdit
        : public QWidget
    {
        Q_OBJECT
        Q_PROPERTY(QVector3D vector READ vector WRITE setVector NOTIFY vectorChanged)
        Q_PROPERTY(AzQtComponents::VectorEditElement::Flavor flavor READ flavor WRITE setFlavor NOTIFY flavorChanged)

    public:
        explicit VectorEdit(QWidget* parent = 0);

        QVector3D vector() const;
        float x() const;
        float y() const;
        float z() const;

        QString xLabel() const;
        QString yLabel() const;
        QString zLabel() const;
        void setLabels(const QString& xLabel, const QString& yLabel, const QString& zLabel);
        void setLabels(const QStringList& labels);

        QColor xColor() const;
        QColor yColor() const;
        QColor zColor() const;
        void setColors(const QColor& xColor, const QColor& yColor, const QColor& zColor);
        void setXColor(const QColor& color);
        void setYColor(const QColor& color);
        void setZColor(const QColor& color);

        VectorEditElement::Flavor flavor() const;
        void setFlavor(VectorEditElement::Flavor flavor);
        void setPixmap(const QPixmap&);

    public Q_SLOTS:
        void setVector(QVector3D vec);
        void setVector(float xValue, float yValue, float zValue);
        void setX(float xValue);
        void setY(float yValue);
        void setZ(float zValue);

    Q_SIGNALS:
        void vectorChanged();
        void flavorChanged();

    private:
        QList<VectorEditElement*> m_editElements;
        VectorEditElement::Flavor m_flavor;
        QLabel* const m_iconLabel;
    };
} // namespace AzQtComponents


