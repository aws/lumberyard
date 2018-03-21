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
#include "StdAfx.h"
#include "PropertyColorCtrl.hxx"
#include "PropertyQTConstants.h"
#include <QtWidgets/QSlider>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QHBoxLayout>

#include "../UICore/ColorPickerDelegate.hxx"

namespace AzToolsFramework
{
    PropertyColorCtrl::PropertyColorCtrl(QWidget* pParent)
        : QWidget(pParent)
    {
        // create the gui, it consists of a layout, and in that layout, a text field for the value
        // and then a slider for the value.
        QHBoxLayout* pLayout = new QHBoxLayout(this);
        pLayout->setAlignment(Qt::AlignLeft);

        m_colorEdit = new QLineEdit();
        m_colorEdit->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        m_colorEdit->setMinimumWidth(PropertyQTConstant_MinimumWidth);
        m_colorEdit->setFixedHeight(PropertyQTConstant_DefaultHeight);
        /*Use regex to validate the input
        *\d\d?    Match 0-99
        *1\d\d    Match 100 - 199
        *2[0-4]\d Match 200-249
        *25[0-5]  Match 250 - 255
        *(25[0-5]|2[0-4]\d|1\d\d|\d\d?)\s*,\s*){2} Match the first two "0-255,"
        *(25[0-5]|2[0-4]\d|1\d\d|\d\d?) Match the last "0-255"
        */
        m_colorEdit->setValidator(new QRegExpValidator(QRegExp("^\\s*((25[0-5]|2[0-4]\\d|1\\d\\d|\\d\\d?)\\s*,\\s*){2}(25[0-5]|2[0-4]\\d|1\\d\\d|\\d\\d?)\\s*$")));

        m_pDefaultButton = new QToolButton(this);
        m_pDefaultButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        QSize fixedSize = QSize(PropertyQTConstant_DefaultHeight, PropertyQTConstant_DefaultHeight);
        // The icon size needs to be smaller than the fixed size to make sure it visually aligns properly.
        QSize iconSize = QSize(fixedSize.width() - 2, fixedSize.height() - 2);
        m_pDefaultButton->setIconSize(iconSize);
        m_pDefaultButton->setFixedSize(fixedSize);

        setLayout(pLayout);

        pLayout->setContentsMargins(0, 0, 0, 0);
        pLayout->addWidget(m_pDefaultButton);
        pLayout->addWidget(m_colorEdit);

        m_pColorDialog = nullptr;
        this->setFocusProxy(m_pDefaultButton);
        setFocusPolicy(m_pDefaultButton->focusPolicy());

        connect(m_pDefaultButton, SIGNAL(clicked()), this, SLOT(openColorDialog()));

        connect(m_colorEdit, SIGNAL(editingFinished()), this, SLOT(OnEditingFinished()));
        connect(m_colorEdit, SIGNAL(returnPressed()), this, SLOT(OnEditingFinished()));
        connect(m_colorEdit, SIGNAL(textEdited(const QString&)), this, SLOT(OnTextEdited(const QString&)));
    }

    PropertyColorCtrl::~PropertyColorCtrl()
    {
    }

    QColor PropertyColorCtrl::value() const
    {
        return m_color;
    }

    QWidget* PropertyColorCtrl::GetFirstInTabOrder()
    {
        return m_pDefaultButton;
    }
    QWidget* PropertyColorCtrl::GetLastInTabOrder()
    {
        return m_pDefaultButton;
    }

    void PropertyColorCtrl::UpdateTabOrder()
    {
        // there is only one focusable element in this, the color dialog button.
    }

    void PropertyColorCtrl::SetColor(QColor color, bool updateDialogColor)
    {
        if (color.isValid())
        {
            m_color = color;
            QPixmap pixmap(m_pDefaultButton->iconSize());
            pixmap.fill(color);
            QIcon newIcon(pixmap);
            m_pDefaultButton->setIcon(newIcon);
            if (m_pColorDialog && updateDialogColor)
            {
                m_pColorDialog->setCurrentColor(m_color);
            }

            int R, G, B;
            m_color.getRgb(&R, &G, &B);
            m_colorEdit->setText(QStringLiteral("%1,%2,%3").arg(R).arg(G).arg(B));
        }
    }

    void PropertyColorCtrl::setValue(QColor val)
    {
        SetColor(val);
    }

    void PropertyColorCtrl::openColorDialog()
    {
        CreateColorDialog();

        if (m_pColorDialog != nullptr)
        {
            m_pColorDialog->setCurrentColor(m_color);
            m_pColorDialog->exec();
        }
    }

    void PropertyColorCtrl::CreateColorDialog()
    {
        // Don't need to create a dialog if it already exists.
        if (m_pColorDialog != nullptr)
        {
            return;
        }

        m_pColorDialog = new QColorDialog(m_color, this);
        m_pColorDialog->setOption(QColorDialog::NoButtons);
        connect(m_pColorDialog, SIGNAL(currentColorChanged(QColor)), this, SLOT(onSelected(QColor)));

        // Position the picker around cursor.
        QLayout* layout = m_pColorDialog->layout();
        if (layout)
        {
            QSize halfSize = (layout->sizeHint() / 2);
            m_pColorDialog->move(QCursor::pos() - QPoint(halfSize.width(), halfSize.height()));
        }
    }

    void PropertyColorCtrl::onSelected(QColor color)
    {
        // Update the color but don't need to update the dialog color since
        // this signal came from the dialog
        SetColor(color, false);

        emit valueChanged(m_color);
    }

    QColor PropertyColorCtrl::convertFromString(const QString& string)
    {
        QStringList strList = string.split(",", QString::SkipEmptyParts);
        int R = 0, G = 0, B = 0;
        AZ_Assert(strList.size() == 3, "Invalid input string for RGB field!");
        R = strList[0].trimmed().toInt();
        G = strList[1].trimmed().toInt();
        B = strList[2].trimmed().toInt();
        return QColor(R, G, B);
    }

    ////////////////////////////////////////////////////////////////
    QWidget* AZColorPropertyHandler::CreateGUI(QWidget* pParent)
    {
        PropertyColorCtrl* newCtrl = aznew PropertyColorCtrl(pParent);
        connect(newCtrl, &PropertyColorCtrl::valueChanged, this, [newCtrl]()
        {
            EBUS_EVENT(PropertyEditorGUIMessages::Bus, RequestWrite, newCtrl);
        });
        return newCtrl;
    }

    // called when the user pressed enter, tab or we lost focus
    void PropertyColorCtrl::OnEditingFinished()
    {

        int pos = 0;
        QString text = m_colorEdit->text();
        QValidator::State validateState = m_colorEdit->validator()->validate(text, pos);

        if (validateState == QValidator::State::Acceptable)
        {
            QColor newValue = convertFromString(text);
            if (newValue != m_color)
            {
                setValue(newValue);
                emit valueChanged(newValue);
            }
        }
        else
        {   
            //If the text is not valid, set back to old value
            setValue(m_color);
        }
    }

    // called each time when a new character has been entered or removed, this doesn't update the value yet
    void PropertyColorCtrl::OnTextEdited(const QString& newText)
    {
        int pos = 0;
        QString text = newText;
        QValidator::State validateState = m_colorEdit->validator()->validate(text, pos);

        if (validateState == QValidator::State::Invalid)
        {
            m_colorEdit->setStyleSheet("color: red;");
        }
        else
        {
            m_colorEdit->setStyleSheet("");
        }
    }

    void AZColorPropertyHandler::ConsumeAttribute(PropertyColorCtrl* /*GUI*/, AZ::u32 /*attrib*/, PropertyAttributeReader* /*attrValue*/, const char* /*debugName*/)
    {
    }

    void AZColorPropertyHandler::WriteGUIValuesIntoProperty(size_t index, PropertyColorCtrl* GUI, property_t& instance, InstanceDataNode* node)
    {
        (int)index;
        (void)node;
        QColor val = GUI->value();
        AZ::Color asAZColor((float)val.redF(), (float)val.greenF(), (float)val.blueF(), (float)val.alphaF());
        instance = static_cast<property_t>(asAZColor);
    }

    bool AZColorPropertyHandler::ReadValuesIntoGUI(size_t index, PropertyColorCtrl* GUI, const property_t& instance, InstanceDataNode* node)
    {
        (int)index;
        (void)node;
        AZ::Vector4 asVector4 = static_cast<AZ::Vector4>(instance);
        QColor asQColor;
        asQColor.setRedF((qreal)asVector4.GetX());
        asQColor.setGreenF((qreal)asVector4.GetY());
        asQColor.setBlueF((qreal)asVector4.GetZ());
        asQColor.setAlphaF((qreal)asVector4.GetW());
        GUI->setValue(asQColor);
        return false;
    }

    QWidget* Vector3ColorPropertyHandler::CreateGUI(QWidget* pParent)
    {
        PropertyColorCtrl* newCtrl = aznew PropertyColorCtrl(pParent);
        connect(newCtrl, &PropertyColorCtrl::valueChanged, this, [newCtrl]()
            {
                EBUS_EVENT(PropertyEditorGUIMessages::Bus, RequestWrite, newCtrl);
            });
        // note:  Qt automatically disconnects objects from each other when either end is destroyed, no need to worry about delete.

        return newCtrl;
    }
    void Vector3ColorPropertyHandler::ConsumeAttribute(PropertyColorCtrl* /*GUI*/, AZ::u32 /*attrib*/, PropertyAttributeReader* /*attrValue*/, const char* /*debugName*/)
    {
    }
    void Vector3ColorPropertyHandler::WriteGUIValuesIntoProperty(size_t index, PropertyColorCtrl* GUI, property_t& instance, InstanceDataNode* node)
    {
        (int)index;
        (void)node;
        QColor val = GUI->value();
        AZ::Vector3 asVector3((float)val.redF(), (float)val.greenF(), (float)val.blueF());
        instance = static_cast<property_t>(asVector3);
    }

    bool Vector3ColorPropertyHandler::ReadValuesIntoGUI(size_t index, PropertyColorCtrl* GUI, const property_t& instance, InstanceDataNode* node)
    {
        (int)index;
        (void)node;
        AZ::Vector3 asVector3 = static_cast<AZ::Vector3>(instance);
        QColor asQColor;
        asQColor.setRedF((qreal)asVector3.GetX());
        asQColor.setGreenF((qreal)asVector3.GetY());
        asQColor.setBlueF((qreal)asVector3.GetZ());
        GUI->setValue(asQColor);
        return false;
    }

    ////////////////////////////////////////////////////////////////
    void RegisterColorPropertyHandlers()
    {
        EBUS_EVENT(PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew Vector3ColorPropertyHandler());
        EBUS_EVENT(PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew AZColorPropertyHandler());
    }
}

#include <UI/PropertyEditor/PropertyColorCtrl.moc>
