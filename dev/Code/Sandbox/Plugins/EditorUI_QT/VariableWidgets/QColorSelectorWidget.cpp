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
#include "QColorSelectorWidget.h"

#ifdef EDITOR_QT_UI_EXPORTS
#include <VariableWidgets/QColorSelectorWidget.moc>
#endif

#include "QColorSwatchWidget.h"
#include "qmessagebox.h"
#include "qfile.h"
#include "qxmlstream.h"
#include "qsettings.h"


QColorSelectorWidget::QColorSelectorWidget(QWidget* parent /*= 0*/)
    : QPresetSelectorWidget(parent)
{
    setContentsMargins(0, 0, 0, 0);

    LoadSessionPresets();
    if (libNames.count() == 0 || libNames[0] != DefaultLibraryName())
    {
        m_currentLibrary = AddNewLibrary(DefaultLibraryName(), m_defaultLibraryId);
        BuildDefaultLibrary();
    }
    //default library has been added to presets save default state
    StoreSessionPresets();
    BuildPanelLayout(false, libNames[m_currentLibrary]);
    scrollArea->setMaximumHeight(100);
    scrollArea->adjustSize();
}

QColor QColorSelectorWidget::GetSelectedColor()
{
    if (lastPresetIndexSelected >= 0 && lastPresetIndexSelected < presets.count())
    {
        return ((QColorSwatchWidget*)presets[m_currentLibrary][lastPresetIndexSelected]->GetWidget())->GetValue();
    }
    return QColor(255, 255, 255, 255);
}

void QColorSelectorWidget::SetColor(QColor color)
{
    m_current.color = color;

    addPresetForeground->setStyleSheet(tr("QWidget{background-color: qlineargradient(x1 : 1, y1 : 0, x2 : 0, y2 : 1, stop : 0") + color.name(QColor::HexRgb) +
        tr(", stop: 0.5") + color.name(QColor::HexRgb) +
        tr(", stop: 0.51 ") + color.name(QColor::HexArgb) +
        tr(", stop: 1 ") + color.name(QColor::HexArgb) + tr("); border: none; outline: none;}"));
}

void QColorSelectorWidget::AddColor(QColor color, QString name)
{
    AddPresetToLibrary(new QColorSwatchWidget(color, this), m_currentLibrary, name, false, true);
    BuildPanelLayout(newLibFlag[m_currentLibrary], libNames[m_currentLibrary]);
    DisplayLibrary(m_currentLibrary);
}

void QColorSelectorWidget::SetValueChangedCallback(std::function<void(QColor)> callback)
{
    callback_value_changed = callback;
}

void QColorSelectorWidget::onValueChanged(unsigned int preset)
{
    m_current.color = ((QColorSwatchWidget*)presets[m_currentLibrary][preset]->GetWidget())->GetValue();

    if ((bool)callback_value_changed)
    {
        callback_value_changed(m_current.color);
    }
}

void QColorSelectorWidget::StylePreset(QPresetWidget* widget, bool selected /*= false*/)
{
    QColor color = ((QColorSwatchWidget*)widget->GetWidget())->GetValue();
    if (selected)
    {
        widget->SetForegroundStyle(tr("QWidget{background-color: qlineargradient(x1:1, y1:0, x2: 0, y2: 1, stop: 0") + color.name(QColor::HexRgb) +
            tr(", stop: 0.5") + color.name(QColor::HexRgb) +
            tr(", stop: 0.51 ") + color.name(QColor::HexArgb) +
            tr(", stop: 1 ") + color.name(QColor::HexArgb) + tr("); border: 2px solid white; outline: none;}"));
    }
    else
    {
        widget->SetForegroundStyle(tr("QWidget{background-color: qlineargradient(x1:1, y1:0, x2: 0, y2: 1, stop: 0") + color.name(QColor::HexRgb) +
            tr(", stop: 0.5") + color.name(QColor::HexRgb) +
            tr(", stop: 0.51 ") + color.name(QColor::HexArgb) +
            tr(", stop: 1 ") + color.name(QColor::HexArgb) + tr("); border: none; outline: none;}"));
    }
}

bool QColorSelectorWidget::AddPresetToLibrary(QWidget* value, unsigned int lib, QString name /*= QString()*/, bool allowDefault /*= false*/, bool InsertAtTop /*= true*/)
{
    QColor color = ((QColorSwatchWidget*)value)->GetValue();
    QString usedName;
    QString R, G, B, A, AA;
    int r, g, b, a;
    color.getRgb(&r, &g, &b, &a);
    R.setNum(r);
    G.setNum(g);
    B.setNum(b);
    A.setNum(a);
    AA.setNum(int(a / 2.55));
    if (name.length() > 0)
    {
        usedName = name;
    }
    else
    {
        usedName = tr("R: ") + R + tr("	G: ") + G + tr("	B: ") + B + tr("	A: ") + AA;
    }
    if (!color.isValid())
    {
        return false;
    }

    int index = 0;
    if (InsertAtTop)
    {
        presets[lib].insert(0, new QPresetWidget(usedName, value, this));
    }
    else
    {
        index = presets[lib].count();
        presets[lib].push_back(new QPresetWidget(usedName, value, this));
    }
    int width = presetSizes[currentPresetSize].width;
    int height = presetSizes[currentPresetSize].height;
    presets[lib][index]->setPresetSize(width, height);

    StylePreset(presets[lib][index], false);
    presets[lib][index]->installEventFilter(this);

    if (name.length() > 0 && name.compare(tr("R: ") + R + tr("	G: ") + G + tr("	B: ") + B + tr("	A: ") + AA, Qt::CaseInsensitive) != 0)
    {
        presets[lib][index]->setToolTip(name + tr("\n") + tr("R: ") + R + tr("	G: ") + G + tr("	B: ") + B + tr("	A: ") + AA);
    }
    else
    {
        presets[lib][index]->setToolTip(usedName);
    }
    unsavedFlag[lib] = true;
    DisplayLibrary(m_currentLibrary);
    return true;
}

int QColorSelectorWidget::PresetAlreadyExistsInLibrary(QWidget* value, int lib)
{
    QColor colorInTest = ((QColorSwatchWidget*)value)->GetValue();
    QString colorInTestName = colorInTest.name(QColor::HexArgb);
    if (presets.count() <= lib || lib < 0)
    {
        return -1;
    }

    for (int i = 0; i < presets[lib].count(); i++)
    {
        QColor colorTest = ((QColorSwatchWidget*)presets[lib][i]->GetWidget())->GetValue();
        QString colorTestName = colorTest.name(QColor::HexArgb);
        if (colorInTestName.compare(colorTestName, Qt::CaseInsensitive) == 0)
        {
            return i;
        }
    }
    return -1;
}

bool QColorSelectorWidget::LoadPreset(QString filepath)
{
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly))
    {
        return false;
    }
    QFileInfo info(file);
    QString libName = GetUniqueLibraryName(info.baseName());

    QXmlStreamReader stream(&file);
    bool result = false;

    int libIndex = 0;
    while (!stream.isEndDocument())
    {
        if (stream.isStartElement())
        {
            if (stream.name() == "library")
            {
                libIndex = libNames.count();
                QXmlStreamAttributes att = stream.attributes();
                for (QXmlStreamAttribute attr : att)
                {
                    if (attr.name().compare(QLatin1String("name"), Qt::CaseInsensitive) == 0)
                    {
                        if ((libIndex = AddNewLibrary(libName)) != m_currentLibrary && libIndex != -1 && libIndex < libFilePaths.count()) //make sure we're not resetting
                        {
                            libFilePaths[libIndex] = filepath;
                            newLibFlag[libIndex] = true;
                        }
                        else if (libIndex >= libFilePaths.count())
                        {
                            libFilePaths.push_back(filepath);
                            newLibFlag.push_back(true);
                        }
                        else if (libIndex == -1)
                        {
                            BuildPanelLayout(false, libNames[m_currentLibrary]);
                            file.close();
                            return false;
                        }
                        else
                        {
                            // Remove all current presets. And presets will be re-added in next loop.
                            while (presets[m_currentLibrary].count() > 0)
                            {
                                RemovePresetFromLibrary(0, m_currentLibrary);
                            }
                        }
                        break;
                    }
                }
            }
            else if (stream.name() == "color" && libIndex < libNames.count()) //checks for color element and that library exists
            {
                QString name = stream.name().toString();
                QColor color;
                QXmlStreamAttributes attributes = stream.attributes();
                for (QXmlStreamAttribute attr : attributes)
                {
                    if (attr.name().compare(QLatin1String("name"), Qt::CaseInsensitive) == 0)
                    {
                        name = attr.value().toString();
                    }
                    if (attr.name().compare(QLatin1String("value"), Qt::CaseInsensitive) == 0)
                    {
                        color.setNamedColor(attr.value().toString());
                    }
                }
                AddPresetToLibrary(new QColorSwatchWidget(color, this), libIndex, name, false, true);
                result = true;
            }
        }
        stream.readNext();
    }

    file.close();
    BuildLibSelectMenu();


    m_currentLibrary = libIndex;
    DisplayLibrary(libIndex);
    BuildPanelLayout(false, libNames[m_currentLibrary]);
    newLibFlag[libIndex] = false;
    unsavedFlag[libIndex] = false;
    //update the presets
    SaveDefaultPreset(libIndex);
    return result;
}

bool QColorSelectorWidget::SavePreset(QString filepath)
{
    QFile file(filepath);
    file.open(QIODevice::WriteOnly);
    QXmlStreamWriter stream(&file);

    stream.setAutoFormatting(true);
    stream.writeStartDocument();

    stream.writeStartElement(tr("library"));
    stream.writeAttribute("name", libNames[m_currentLibrary]);
    for (unsigned int i = 0; i < presets[m_currentLibrary].count(); i++)
    {
        QColor color = ((QColorSwatchWidget*)presets[m_currentLibrary][i]->GetWidget())->GetValue();
        int alpha = color.alpha();
        QString A;
        A.setNum(alpha);
        stream.writeStartElement("color");
        stream.writeAttribute("name", presets[m_currentLibrary][i]->GetName());
        stream.writeAttribute("value", color.name(QColor::HexArgb));
        stream.writeEndElement(); //color
    }
    stream.writeEndElement(); // current library
    stream.writeEndDocument();
    file.close();
    newLibFlag[m_currentLibrary] = false;
    unsavedFlag[m_currentLibrary] = false;
    BuildPanelLayout(false, libNames[m_currentLibrary]);

    return true;
}

void QColorSelectorWidget::BuildDefaultLibrary()
{
    //default is always first in list
    m_currentLibrary = m_defaultLibraryId;
    while (presets[m_currentLibrary].count() > 0)
    {
        RemovePresetFromLibrary(0, m_currentLibrary);
    }

    for (unsigned int i = 2; i < 20; i++)
    {
        QColor color((Qt::GlobalColor)i);
        AddPresetToLibrary(new QColorSwatchWidget(color, this), m_currentLibrary, 0, true, false);
    }

    DisplayLibrary(m_currentLibrary);
    newLibFlag[m_currentLibrary] = false;
    unsavedFlag[m_currentLibrary] = false;
}

QWidget* QColorSelectorWidget::CreateSelectedWidget()
{
    return new QColorSwatchWidget(m_current.color, this);
}

QString QColorSelectorWidget::SettingsBaseGroup() const
{
    return "Color Presets/";
}

bool QColorSelectorWidget::LoadSessionPresetFromName(QSettings* settings, const QString& libName)
{
    bool success = false;
    settings->beginGroup(libName);
    QString value = settings->value("name", "").toString();
    //ensure a library exists
    int index = AddNewLibrary(value);
    value = settings->value("path", "").toString();
    libFilePaths[index] = value;

    QStringList keys = settings->childGroups();
    for (QString key : keys)
    {
        success = true;
        settings->beginGroup(key);//start of preset
        QColor color;
        value = settings->value("color", "#00000000").toString();
        color.setNamedColor(value);
        value = settings->value("name", "").toString();
        AddPresetToLibrary(new QColorSwatchWidget(color, this), index, value, true, false);
        settings->endGroup(); //end of this preset
    }
    settings->endGroup(); //end of this library
    newLibFlag[index] = false;
    unsavedFlag[index] = false;
    return success;
}

void QColorSelectorWidget::SavePresetToSession(QSettings* settings, const int& lib, const int& preset)
{
    CRY_ASSERT(presets[lib][preset]);
    int digitsToDisplay = 3;
    int numericalBase = 10;
    QChar fillerCharacter = '0';
    //assigning a path of 000-999 to preserve preset order, and prevent cases where duplicate names would create conflict
    settings->beginGroup(QString("%1").arg(preset, digitsToDisplay, numericalBase, fillerCharacter));
    QString _color = static_cast<QColorSwatchWidget*>(presets[lib][preset]->GetWidget())->GetValue().name(QColor::HexArgb);
    settings->setValue("color", _color);
    settings->setValue("name", presets[lib][preset]->GetName());
    settings->endGroup();//end of preset
}
