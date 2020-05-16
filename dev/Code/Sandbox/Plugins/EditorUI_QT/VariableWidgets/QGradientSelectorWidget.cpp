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
#include "EditorUI_QT_Precompiled.h"
#include "QGradientSelectorWidget.h"

#include "QCustomGradientWidget.h"
#include "CurveEditorContent.h"
#include "qmessagebox.h"
#include "qfile.h"
#include "qxmlstream.h"
#include "ISplines.h"
#include "QGradientSwatchWidget.h"
#include "qbrush.h"
#include "qsettings.h"


QGradientSelectorWidget::QGradientSelectorWidget(QWidget* parent /*= 0*/)
    : QPresetSelectorWidget(parent)
{
    presetSizes[0].width = 32;
    presetSizes[0].height = 16;
    presetSizes[1].width = 64;
    presetSizes[1].height = 32;
    presetSizes[2].width = 128;
    presetSizes[2].height = 64;

    LoadSessionPresets();
    if (libNames.count() == 0 || libNames[0] != DefaultLibraryName())
    {
        m_currentLibrary = AddNewLibrary(DefaultLibraryName(), m_defaultLibraryId);
        BuildDefaultLibrary();
    }
    //default library has been added to presets save default state
    StoreSessionPresets();
    BuildLibSelectMenu();
    BuildMainMenu();
    BuildPanelLayout(false, libNames[m_currentLibrary]);
    scrollArea->setMinimumHeight(128);
    scrollArea->adjustSize();
    SAFE_DELETE(addPresetForeground);
}

SCurveEditorCurve& QGradientSelectorWidget::GetSelectedCurve()
{
    if (lastPresetIndexSelected >= 0 && lastPresetIndexSelected < presets.count())
    {
        return ((QGradientSwatchWidget*)presets[m_currentLibrary][lastPresetIndexSelected]->GetWidget())->GetCurve();
    }
    return m_DefaultCurve;
}

bool QGradientSelectorWidget::AddPresetToLibrary(QWidget* value, unsigned int lib, QString name /*= QString()*/, bool allowDefault /*= false*/, bool InsertAtTop /*= true*/)
{
    QString usedName = "Preset has no name";
    if (!name.isEmpty())
    {
        usedName = name;
    }


    int index = 0;
    if (InsertAtTop)
    {
        presets[lib].insert(0, new QPresetWidget(usedName, new QGradientSwatchWidget(((QGradientSwatchWidget*)value)->GetContent(), ((QGradientSwatchWidget*)value)->GetStops(), this), this));
    }
    else
    {
        index = presets[lib].count();
        presets[lib].push_back(new QPresetWidget(usedName, new QGradientSwatchWidget(((QGradientSwatchWidget*)value)->GetContent(), ((QGradientSwatchWidget*)value)->GetStops(), this), this));
    }
    int width = presetSizes[currentPresetSize].width;
    int height = presetSizes[currentPresetSize].height;
    presets[lib][index]->setPresetSize(width, height);

    StylePreset(presets[lib][index], false);
    presets[lib][index]->installEventFilter(this);
    presets[lib][index]->setToolTip(usedName);
    unsavedFlag[lib] = true;
    value->close();
    DisplayLibrary(m_currentLibrary);
    return true;
}

void QGradientSelectorWidget::BuildDefaultLibrary()
{
    //default is always first in list
    m_currentLibrary = m_defaultLibraryId;
    while (presets[m_currentLibrary].count() > 0)
    {
        RemovePresetFromLibrary(0, m_currentLibrary);
    }

    //hard code some test presets
    for (int i = 0; i < 6; i++)
    {
        QGradientSwatchWidget preset(SCurveEditorContent(), QGradientStops(), this);
        preset.SetGradientStops({ QGradientStop(0.0, QColor::fromHsl(60 * i, 255, 255, 255)),
                                  QGradientStop(0.5, QColor::fromHsl(60 * i, 255, 127, 255)),
                                  QGradientStop(1.0, QColor::fromHsl(60 * i, 255, 0, 255)) });
        AddPresetToLibrary(&preset, m_currentLibrary, 0, true, false);
        preset.deleteLater();
    }

    DisplayLibrary(m_currentLibrary);
    newLibFlag[m_currentLibrary] = false;
    unsavedFlag[m_currentLibrary] = false;
}

bool QGradientSelectorWidget::SavePreset(QString filepath)
{
    XmlNodeRef node = GetIEditor()->GetSystem()->CreateXmlNode("library");
    node->setAttr("name", libNames[m_currentLibrary].toStdString().c_str());

    for (unsigned int i = 0; i < presets[m_currentLibrary].count(); i++)
    {
        QGradientSwatchWidget* widget = ((QGradientSwatchWidget*)presets[m_currentLibrary][i]->GetWidget());
        XmlNodeRef child = GetIEditor()->GetSystem()->CreateXmlNode("preset");
        node->addChild(child);
        child->setAttr("name", presets[m_currentLibrary][i]->GetName().toStdString().c_str());

        //gradient
        XmlNodeRef gradient = GetIEditor()->GetSystem()->CreateXmlNode("gradient");
        child->addChild(gradient);
        QGradientStops stops = widget->GetStops();
        for (unsigned int s = 0; s < stops.size(); s++)
        {
            XmlNodeRef stopNode = GetIEditor()->GetSystem()->CreateXmlNode("stop");
            gradient->addChild(stopNode);
            stopNode->setAttr("time", stops[s].first);
            stopNode->setAttr("value", stops[s].second.name(QColor::HexArgb).toStdString().c_str());
        }

        //curve
        XmlNodeRef contentNode = GetIEditor()->GetSystem()->CreateXmlNode("content");
        child->addChild(contentNode);
        SCurveEditorContent content = widget->GetContent();
        for (unsigned int c = 0; c < content.m_curves.size(); c++)
        {
            XmlNodeRef curveNode = GetIEditor()->GetSystem()->CreateXmlNode("curve");
            contentNode->addChild(curveNode);
            SCurveEditorCurve curve = content.m_curves[c];
            curveNode->setAttr("value", curve.m_defaultValue);
            //keys
            for (unsigned int k = 0; k < curve.m_keys.size(); k++)
            {
                XmlNodeRef keyNode = GetIEditor()->GetSystem()->CreateXmlNode("key");
                curveNode->addChild(keyNode);
                SCurveEditorKey key = curve.m_keys[k];
                keyNode->setAttr("time", key.m_time);
                keyNode->setAttr("value", key.m_value);
                keyNode->setAttr("intype", key.m_inTangentType);
                keyNode->setAttr("intan", key.m_inTangent);
                keyNode->setAttr("outtype", key.m_outTangentType);
                keyNode->setAttr("outtan", key.m_outTangent);
            }
        }
    }
    newLibFlag[m_currentLibrary] = false;
    unsavedFlag[m_currentLibrary] = false;
    BuildPanelLayout(false, libNames[m_currentLibrary]);
    return node->saveToFile(filepath.toStdString().c_str());
}

bool QGradientSelectorWidget::LoadPreset(QString filepath)
{
    XmlNodeRef _root = GetIEditor()->GetSystem()->LoadXmlFromFile(filepath.toStdString().c_str());
    if (!_root)
    {
        return false;         //failed to load
    }
    bool result = false;

    QFileInfo info(filepath);
    QString libName = GetUniqueLibraryName(info.baseName());

    int libIndex = -1;

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
        return false;
    }
    else // The library is already in presets list.
    {
        // Remove all current presets. And presets will be re-added in next loop.
        while (presets[m_currentLibrary].count() > 0)
        {
            RemovePresetFromLibrary(0, m_currentLibrary);
        }
    }

    for (unsigned int i = 0; i < _root->getChildCount(); i++)
    {
        QGradientStops stops;
        SCurveEditorContent content;
        XmlNodeRef _child = _root->getChild(i); //get first child should be "preset"
        QString presetName = _child->getAttr("name"); //get preset name

        for (unsigned int n = 0; n < _child->getChildCount(); n++)
        {
            XmlNodeRef _node = _child->getChild(n);
            QString typeString = _node->getTag();
            if (typeString.compare("gradient") == 0) //get gradient stops
            {
                for (unsigned int g = 0; g < _node->getChildCount(); g++)
                {
                    XmlNodeRef _gradient = _node->getChild(g);
                    float time;
                    QColor color;
                    if (_gradient->getAttr("time", time))
                    {
                        QString colorName;
                        colorName = _gradient->getAttr("value");
                        color.setNamedColor(colorName);
                        if (color.isValid())
                        {
                            stops.push_back(QGradientStop(time, color));
                        }
                    }
                }
            }
            if (typeString.compare("content") == 0) //get curve content
            {
                for (unsigned int c = 0; c < _node->getChildCount(); c++)
                {
                    XmlNodeRef _curve = _node->getChild(c);
                    content.m_curves.push_back(SCurveEditorCurve());
                    _curve->getAttr("value", content.m_curves.back().m_defaultValue);
                    SCurveEditorCurve curve = content.m_curves.back();
                    for (unsigned int k = 0; k < _curve->getChildCount(); k++)
                    {
                        curve.m_keys.push_back(SCurveEditorKey());
                        XmlNodeRef _key = _curve->getChild(k);
                        SCurveEditorKey key = curve.m_keys.back();
                        float time, value;
                        Vec2 intan, outtan;
                        int intype, outtype;

                        if (_key->getAttr("time", time))
                        {
                            key.m_time = time;
                        }
                        if (_key->getAttr("value", value))
                        {
                            key.m_value = value;
                        }
                        if (_key->getAttr("intan", intan))
                        {
                            key.m_inTangent = intan;
                        }
                        if (_key->getAttr("outtan", outtan))
                        {
                            key.m_outTangent = outtan;
                        }
                        if (_key->getAttr("intype", intype))
                        {
                            key.m_inTangentType = (SCurveEditorKey::ETangentType)intype;
                        }
                        if (_key->getAttr("outtype", outtype))
                        {
                            key.m_outTangentType = (SCurveEditorKey::ETangentType)outtype;
                        }
                        content.m_curves.back().m_keys.push_back(key);
                    }
                }
            }
        }
        AddPresetToLibrary(new QGradientSwatchWidget(content, stops, this), libIndex, presetName);
    }
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

int QGradientSelectorWidget::PresetAlreadyExistsInLibrary(QWidget* value, int lib)
{
    QGradientSwatchWidget* preset = ((QGradientSwatchWidget*)value);
    SCurveEditorCurve curve = preset->GetCurve();
    QGradientStops stops = preset->GetStops();
    float amin = preset->GetMinAlpha();
    float amax = preset->GetMaxAlpha();
    int result = -1;
    bool failCheck = false;

    for (unsigned int index = 0; index < presets[lib].count(); index++)
    {
        QGradientSwatchWidget* current = (QGradientSwatchWidget*)presets[lib][index]->GetWidget();

        //early rejections
        if (amin != current->GetMinAlpha())
        {
            continue;
        }
        if (amax != current->GetMaxAlpha())
        {
            continue;
        }
        if (stops.count() != current->GetStops().count())
        {
            continue;
        }
        if (curve.m_keys.size() != current->GetCurve().m_keys.size())
        {
            continue;
        }

        //test gradient stops
        QGradientStops currentStops = current->GetStops();
        for (int i = 0; i < currentStops.count(); i++)
        {
            if (currentStops[i].first != stops[i].first)
            {
                failCheck = true;
            }
            if (currentStops[i].second != stops[i].second)
            {
                failCheck = true;
            }
        }
        if (failCheck)
        {
            continue;
        }

        //test curve keys
        SCurveEditorCurve currentCurve = current->GetCurve();
        for (int i = 0; i < currentCurve.m_keys.size(); i++)
        {
            if (currentCurve.m_keys[i].m_value != curve.m_keys[i].m_value)
            {
                failCheck = true;
            }
            if (currentCurve.m_keys[i].m_time != curve.m_keys[i].m_time)
            {
                failCheck = true;
            }
            if (currentCurve.m_keys[i].m_inTangent != curve.m_keys[i].m_inTangent)
            {
                failCheck = true;
            }
            if (currentCurve.m_keys[i].m_outTangent != curve.m_keys[i].m_outTangent)
            {
                failCheck = true;
            }
        }
        if (failCheck)
        {
            continue;
        }
        result = index;
        return result;
    }
    return result;
}

void QGradientSelectorWidget::StylePreset(QPresetWidget* widget, bool selected /*= false*/)
{
    if (selected)
    {
        widget->SetForegroundStyle(tr("QWidget{border: 2px solid white; outline: none;}"));
    }
    else
    {
        widget->SetForegroundStyle(tr("QWidget{border: none; outline: none;}"));
    }
}

void QGradientSelectorWidget::onValueChanged(unsigned int preset)
{
    SetContent(((QGradientSwatchWidget*)presets[m_currentLibrary][preset]->GetWidget())->GetContent());
    SetGradientStops(((QGradientSwatchWidget*)presets[m_currentLibrary][preset]->GetWidget())->GetStops());
    if (bool(m_callbackCurveChanged))
    {
        m_callbackCurveChanged(m_current.content.m_curves.back());
    }
    if (bool(m_callbackGradientChanged))
    {
        m_callbackGradientChanged(m_current.stops);
    }
}

QGradientStops& QGradientSelectorWidget::GetSelectedStops()
{
    return m_current.stops;
}

float QGradientSelectorWidget::GetSelectedAlphaMin()
{
    float result = 255; //maximum alpha in integer form
    for (auto key : m_current.content.m_curves.back().m_keys)
    {
        result = qMin(result, key.m_value);
    }
    return result;
}

float QGradientSelectorWidget::GetSelectedAlphaMax()
{
    float result = 0; //minimum alpha in integer form
    for (auto key : m_current.content.m_curves.back().m_keys)
    {
        result = qMax(result, key.m_value);
    }
    return result;
}

void QGradientSelectorWidget::SetContent(SCurveEditorContent content)
{
    m_current.content = content;

    SAFE_DELETE(addPresetForeground);
    addPresetForeground = new QGradientSwatchWidget(m_current.content,
            m_current.stops,
            this);
    DisplayLibrary(m_currentLibrary);
}

void QGradientSelectorWidget::SetGradientStops(QGradientStops stops)
{
    m_current.stops = stops;

    SAFE_DELETE(addPresetForeground);

    addPresetForeground = new QGradientSwatchWidget(m_current.content,
            m_current.stops,
            this);
    DisplayLibrary(m_currentLibrary);
}

QWidget* QGradientSelectorWidget::CreateSelectedWidget()
{
    return new QGradientSwatchWidget(m_current.content, m_current.stops, this);
}

bool QGradientSelectorWidget::LoadSessionPresetFromName(QSettings* settings, const QString& libName)
{
    bool success = false;
    settings->beginGroup(libName);
    QString value = settings->value("name", "").toString();
    //ensure a library exists
    int index = AddNewLibrary(value);
    value = settings->value("path", "").toString();
    libFilePaths[index] = value;

    QStringList presets = settings->childGroups();
    for (QString preset : presets)
    {
        QString curveName = "";
        SCurveEditorContent content;
        QGradientStops stops;
        settings->beginGroup(preset); //start of this preset
        settings->beginGroup("Curves");//start of curves
        QStringList curves = settings->childGroups();
        for (QString curve : curves)
        {
            float f;
            settings->beginGroup(curve);//start of curve
            SCurveEditorCurve _curve;
            f = settings->value("defValue").toFloat();
            _curve.m_defaultValue = f;
            QString curveName = settings->value("name").toString();
            QStringList keys = settings->childGroups();
            for (QString key : keys)
            {
                SCurveEditorKey _key;
                QPointF _p;
                success = true;
                settings->beginGroup(key);//start of key
                _key.m_time = settings->value("time", "0").toFloat();
                _key.m_value = settings->value("value").toFloat();
                _p = settings->value("intan").toPointF();
                _key.m_inTangent = Vec2(_p.x(), _p.y());
                _p = settings->value("outtan").toPointF();
                _key.m_outTangent = Vec2(_p.x(), _p.y());
                _key.m_inTangentType = (SCurveEditorKey::ETangentType)settings->value("intype").toInt();
                _key.m_outTangentType = (SCurveEditorKey::ETangentType)settings->value("outtype").toInt();
                _curve.m_keys.push_back(_key);
                settings->endGroup();//end of key
            }
            content.m_curves.push_back(_curve);
            settings->endGroup(); //end of this curve
        }
        settings->endGroup(); // end of curves
        settings->beginGroup("Stops"); //start of stops
        QStringList hues = settings->childGroups();
        for (QString hue : hues)
        {
            QGradientStop _stop;
            settings->beginGroup(hue);//start of this stop
            success = true;
            _stop.first = settings->value("Time", "0").toFloat();
            _stop.second = settings->value("Value").toString();
            settings->endGroup(); //end of this stop
            stops.append(_stop);
        }
        settings->endGroup(); // end of stops
        AddPresetToLibrary(new QGradientSwatchWidget(content, stops, this), index, curveName, true, false);
        settings->endGroup(); //end of this preset
    }
    settings->endGroup(); //end of this library
    newLibFlag[index] = false;
    unsavedFlag[index] = false;
    return success;
}

void QGradientSelectorWidget::SavePresetToSession(QSettings* settings, const int& lib, const int& preset)
{
    int digitsToDisplay = 3;
    int numericalBase = 10;
    QChar fillerCharacter = '0';
    //assigning a path of 000-999 to preserve preset order, and prevent cases where duplicate names would create conflict
    settings->beginGroup(QString("%1").arg(preset, digitsToDisplay, numericalBase, fillerCharacter));
    QGradientSwatchWidget* _preset = static_cast<QGradientSwatchWidget*>(presets[lib][preset]->GetWidget());
    settings->setValue("/name", presets[lib][preset]->GetName());
    //CURVE///////////////////////////////////////////////////////////////////
    SCurveEditorContent _content = _preset->GetContent();

    for (unsigned int curve = 0; curve < _content.m_curves.size(); curve++) //this preset
    {
        //assigning a path of 000-999 to preserve curve order, and prevent cases where duplicate names would create conflict
        settings->beginGroup("Curves/" + QString("%1").arg(curve, digitsToDisplay, numericalBase, fillerCharacter));
        SCurveEditorCurve _curve = _content.m_curves[curve];
        settings->setValue("defValue", _curve.m_defaultValue);
        for (unsigned int key = 0; key < _curve.m_keys.size(); key++)
        {
            //assigning a path of 000-999 to preserve key order, and prevent cases where duplicate names would create conflict
            settings->beginGroup(QString("%1").arg(key, digitsToDisplay, numericalBase, fillerCharacter));
            SCurveEditorKey _key = _curve.m_keys[key];
            settings->setValue("time", _key.m_time);
            settings->setValue("value", _key.m_value);
            settings->setValue("intan", QPointF(_key.m_inTangent.x, _key.m_inTangent.y));
            settings->setValue("outtan", QPointF(_key.m_outTangent.x, _key.m_outTangent.y));
            settings->setValue("intype", _key.m_inTangentType);
            settings->setValue("outtype", _key.m_outTangentType);
            settings->endGroup();//end of key
        }
        settings->endGroup();//end of curve
    }
    //GRADIENT////////////////////////////////////////////////////////////////
    QGradientStops _stops = _preset->GetStops();
    for (unsigned int stop = 0; stop < _stops.count(); stop++) //this preset
    {
        //assigning a path of 000-999 to preserve stop order, and prevent cases where duplicate names would create conflict
        settings->beginGroup("Stops/" + QString("%1").arg(stop, digitsToDisplay, numericalBase, fillerCharacter));
        settings->setValue("Time", _stops[stop].first);
        settings->setValue("Value", _stops[stop].second.name(QColor::HexRgb));
        settings->endGroup();//end of stop
    }
    settings->endGroup();//end of preset
}

QString QGradientSelectorWidget::SettingsBaseGroup() const
{
    return "Gradient Presets/";
}

void QGradientSelectorWidget::SetPresetName(QString name)
{
    addPresetName.setText(name);
}

void QGradientSelectorWidget::SetCallbackGradientChanged(std::function<void(const QGradientStops&)> callback)
{
    m_callbackGradientChanged = callback;
}

void QGradientSelectorWidget::SetCallbackCurveChanged(std::function<void(const struct SCurveEditorCurve&)> callback)
{
    m_callbackCurveChanged = callback;
}

#include <VariableWidgets/QGradientSelectorWidget.moc>

