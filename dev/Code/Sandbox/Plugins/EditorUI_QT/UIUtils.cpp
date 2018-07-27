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

#include "UIUtils.h"

#include <QDialog>
#include <QInputDialog>
#include <QFormLayout>
#include "VariableWidgets/QAmazonLineEdit.h"
#include <QDialogButtonBox>
#include <QValidator>
#include <QtMath>

QString UIUtils::GetStringDialog(const QString& title, const QString& valueName, QWidget* parent)
{
    return QInputDialog::getText(parent, title, valueName);
}

bool UIUtils::GetMultiFieldDialog(QWidget* parent, const QString& title, FieldContainer& container, const unsigned int& width)
{
    QDialog dialog(parent);
    dialog.setWindowTitle(title);

    QFormLayout form(&dialog);

    // Make sure the dialog is not resizeble
    form.setSizeConstraint(QLayout::SetFixedSize);

    std::vector<QAmazonLineEdit*> edits;

    for (unsigned int i = 0; i < container.m_fieldList.size(); i++)
    {
        BaseType* base = container.m_fieldList[i];
        QAmazonLineEdit* edit = new QAmazonLineEdit(&dialog);

        edit->setMinimumWidth(width);
        QObject::connect(edit, &QAmazonLineEdit::textChanged, edit, [edit, width](const QString& text)
            {
                const unsigned int currWidth = (unsigned int)edit->fontMetrics().boundingRect(text).width();
                edit->setMinimumWidth(qMax(currWidth + 32, width));
            });

        switch (base->GetType())
        {
        case TYPE_STRING:
            break;
        case TYPE_INT:
            edit->setValidator(new QIntValidator(edit));
            break;
        case TYPE_FLOAT:
            edit->setValidator(new QDoubleValidator(edit));
            break;
        }
        edit->setText(base->ToString());

        edits.push_back(edit);
        form.addRow(base->GetValueName(), edit);
    }

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);

    QObject::connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    bool ok = dialog.exec() == QDialog::Accepted;

    // If ok, set container values to the specified values
    if (ok)
    {
        for (unsigned int i = 0; i < container.m_fieldList.size(); i++)
        {
            BaseType* base = container.m_fieldList[i];
            QAmazonLineEdit* edit = edits[i];

            base->FromString(edit->text());

            // Cleanup
            delete edit;
        }
    }
    else
    {
        // Cleanup
        for (unsigned int i = 0; i < edits.size(); i++)
        {
            delete edits[i];
        }
    }

    return ok;
}

QColor UIUtils::StringToColor(const QString& value)
{
    std::string str = value.toStdString();
    float r, g, b, a = 255.0f;
    int res = 0;
    if (res != 4)
    {
        res = azsscanf(str.c_str(), "%f,%f,%f,%f", &r, &g, &b, &a);
    }
    if (res != 4)
    {
        res = azsscanf(str.c_str(), "R:%f,G:%f,B:%f,A:%f", &r, &g, &b, &a);
    }
    if (res != 4)
    {
        res = azsscanf(str.c_str(), "R:%f G:%f B:%f A:%f", &r, &g, &b, &a);
    }
    if (res != 4)
    {
        res = azsscanf(str.c_str(), "%f %f %f %f", &r, &g, &b, &a);
    }

    // if no alpha is passed in as part of the string we still need to support a valid RGB string.
    if (res < 3)
    {
        res = azsscanf(str.c_str(), "%f,%f,%f", &r, &g, &b);
        a = 255.0f;
    }
    if (res < 3)
    {
        res = azsscanf(str.c_str(), "R:%f,G:%f,B:%f", &r, &g, &b);
        a = 255.0f;
    }
    if (res < 3)
    {
        res = azsscanf(str.c_str(), "R:%f G:%f B:%f", &r, &g, &b);
        a = 255.0f;
    }
    if (res < 3)
    {
        res = azsscanf(str.c_str(), "%f %f %f", &r, &g, &b);
        a = 255.0f;
    }

    //If we have not been able to match any of the formates then we just assume one value and use it as the R.
    if (res < 3)
    {
        azsscanf(str.c_str(), "%f", &r);
        return r;
    }

    return ColorLinearToGamma(r , g, b, a);
}

QString UIUtils::ColorToString(const QColor& color)
{
    float r, g, b, a;
    ColorGammaToLinear(color, r, g, b, a);
    return QString().sprintf("%f,%f,%f", r, g, b);
}

void UIUtils::ColorGammaToLinear(const QColor& color, float& r, float& g, float& b, float& a)
{
    r = color.redF();
    g = color.greenF();
    b = color.blueF();
    a = color.alphaF();

    r = (float)(r <= 0.04045 ? (r / 12.92) : pow(((double)r + 0.055) / 1.055, 2.4)),
    g = (float)(g <= 0.04045 ? (g / 12.92) : pow(((double)g + 0.055) / 1.055, 2.4)),
    b = (float)(b <= 0.04045 ? (b / 12.92) : pow(((double)b + 0.055) / 1.055, 2.4));
    a = (float)(a <= 0.04045 ? (a / 12.92) : pow(((double)a + 0.055) / 1.055, 2.4));
}

QColor UIUtils::ColorLinearToGamma(float r, float g, float b, float a)
{
    r = clamp_tpl(r, 0.0f, 1.0f);
    g = clamp_tpl(g, 0.0f, 1.0f);
    b = clamp_tpl(b, 0.0f, 1.0f);
    a = clamp_tpl(a, 0.0f, 1.0f);

    r = (float)(r <= 0.0031308 ? (12.92 * r) : (1.055 * pow((double)r, 1.0 / 2.4) - 0.055));
    g = (float)(g <= 0.0031308 ? (12.92 * g) : (1.055 * pow((double)g, 1.0 / 2.4) - 0.055));
    b = (float)(b <= 0.0031308 ? (12.92 * b) : (1.055 * pow((double)b, 1.0 / 2.4) - 0.055));
    a = (float)(a <= 0.0031308 ? (12.92 * a) : (1.055 * pow((double)a, 1.0 / 2.4) - 0.055));

    return QColor(FtoI(r * 255.0f), FtoI(g * 255.0f), FtoI(b * 255.0f), FtoI(a * 255.0f));
}

bool UIUtils::FileExistsOutsideIgnoreCache(const QString& filepath)
{
    if (filepath.isEmpty())
    {
        return false;
    }
    QFileInfo info(filepath);
    return info.exists();
}
