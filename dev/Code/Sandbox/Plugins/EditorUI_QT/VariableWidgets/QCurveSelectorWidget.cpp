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
#include "QCurveSelectorWidget.h"
#include "QCurveSwatchWidget.h"

#include <VariableWidgets/QCurveSelectorWidget.moc>
#include "CurveEditorContent.h"
#include "qmessagebox.h"
#include "ISplines.h"
#include "QFileDialog"
#include "qsettings.h"


QCurveSelectorWidget::QCurveSelectorWidget(QWidget* parent /*= 0*/)
    : QPresetSelectorWidget(parent)
{
    presetSizes[0].width = 100;
    presetSizes[0].height = 32;
    presetSizes[1].width = 100;
    presetSizes[1].height = 32;
    presetSizes[2].width = 100;
    presetSizes[2].height = 32;

    swabSizeMenu = nullptr;

    LoadSessionPresets();
    if (libNames.count() == 0 || libNames[0] != DefaultLibraryName())
    {
        m_currentLibrary = AddNewLibrary(DefaultLibraryName(), m_defaultLibraryId);
        BuildDefaultLibrary();
    }
    //default library has been added to presets save default state
    StoreSessionPresets();
    m_menuFlags = PresetMenuFlag(MAIN_MENU_REMOVE_ALL);
    m_layoutType = PresetLayoutType::PRESET_LAYOUT_FIXED_COLUMN_2;
    BuildLibSelectMenu();
    BuildMainMenu();
    BuildPanelLayout(false, libNames[m_currentLibrary]);
    scrollArea->setMinimumHeight(100);
    scrollArea->setMinimumWidth(100 * 2 + 32); //width of 2 column curve icons + content margins + scrollbar ~= 230
    scrollArea->setAlignment(Qt::AlignLeft);
    scrollArea->adjustSize();
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
}

SCurveEditorCurve& QCurveSelectorWidget::GetSelectedCurve()
{
    assert(lastPresetIndexSelected >= 0 && lastPresetIndexSelected < presets.count());
    return ((QCurveSwatchWidget*)presets[m_currentLibrary][lastPresetIndexSelected]->GetWidget())->GetCurve();
}

void QCurveSelectorWidget::SetContent(SCurveEditorContent content)
{
    m_current.content = content;

    addPresetForeground = new QCurveSwatchWidget(content, (QWidget*)this);
}

void QCurveSelectorWidget::SetCurve(SCurveEditorCurve curve)
{
    SCurveEditorContent content = m_current.content;
    content.m_curves.clear();
    content.m_curves.push_back(curve);
    SetContent(content);
}


void QCurveSelectorWidget::onValueChanged(unsigned int preset)
{
    if (static_cast<QCurveSwatchWidget*>(presets[m_currentLibrary][preset]->GetWidget())->GetContent().m_curves.size() == 0)
    {
        return;
    }
    m_current.content = ((QCurveSwatchWidget*)presets[m_currentLibrary][preset]->GetWidget())->GetContent();
    if ((bool)callback_curve_changed)
    {
        callback_curve_changed(m_current.content.m_curves.back());
    }
}

void QCurveSelectorWidget::StylePreset(QPresetWidget* widget, bool selected /*= false*/)
{
    if (selected)
    {
        widget->SetForegroundStyle(tr("QWidget{outline: 2px solid white;}"));
    }
    else
    {
        widget->SetForegroundStyle(tr("QWidget{outline: none;}"));
    }
}

int QCurveSelectorWidget::PresetAlreadyExistsInLibrary(QWidget* value, int lib)
{
    return -1;
}

bool QCurveSelectorWidget::LoadPreset(QString filepath)
{
    XmlNodeRef _root = GetIEditor()->GetSystem()->LoadXmlFromFile(filepath.toStdString().c_str());
    if (!_root)
    {
        return false; //failed to load
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
        SCurveEditorContent content;
        XmlNodeRef _child = _root->getChild(i); //get first child should be "preset"
        QString presetName = _child->getAttr("name"); //get preset name

        for (unsigned int n = 0; n < _child->getChildCount(); n++)
        {
            XmlNodeRef _node = _child->getChild(n);
            QString typeString = _node->getTag();
            if (typeString.compare("content") == 0) //get curve content
            {
                for (unsigned int c = 0; c < _node->getChildCount(); c++)
                {
                    XmlNodeRef _curve = _node->getChild(c);
                    content.m_curves.push_back(SCurveEditorCurve());
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
        AddPresetToLibrary(new QCurveSwatchWidget(content, this), libIndex, presetName);
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

bool QCurveSelectorWidget::SavePreset(QString filepath)
{
    XmlNodeRef node = GetIEditor()->GetSystem()->CreateXmlNode("library");
    node->setAttr("name", libNames[m_currentLibrary].toStdString().c_str());

    for (unsigned int i = 0; i < presets[m_currentLibrary].count(); i++)
    {
        QCurveSwatchWidget* widget = ((QCurveSwatchWidget*)presets[m_currentLibrary][i]->GetWidget());
        XmlNodeRef child = GetIEditor()->GetSystem()->CreateXmlNode("preset");
        node->addChild(child);
        child->setAttr("name", presets[m_currentLibrary][i]->GetName().toStdString().c_str());

        //curve
        XmlNodeRef contentNode = GetIEditor()->GetSystem()->CreateXmlNode("content");
        child->addChild(contentNode);
        SCurveEditorContent content = widget->GetContent();
        for (unsigned int c = 0; c < content.m_curves.size(); c++)
        {
            XmlNodeRef curveNode = GetIEditor()->GetSystem()->CreateXmlNode("curve");
            contentNode->addChild(curveNode);
            SCurveEditorCurve curve = content.m_curves[c];
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

void QCurveSelectorWidget::BuildDefaultLibrary()
{
    //default is always first in list
    m_currentLibrary = m_defaultLibraryId;
    while (presets[m_currentLibrary].count() > 0)
    {
        RemovePresetFromLibrary(0, m_currentLibrary);
    }

    SCurveEditorKey defaultKey;
    defaultKey.m_time = 0.0f;
    defaultKey.m_value = 1.0f;
    defaultKey.m_bModified = true;
    defaultKey.m_bAdded = true;
    defaultKey.m_inTangentType = SCurveEditorKey::eTangentType_Linear;
    defaultKey.m_outTangentType = SCurveEditorKey::eTangentType_Linear;
    SCurveEditorKey defaultFlatKey = defaultKey;
    defaultFlatKey.m_inTangentType = SCurveEditorKey::eTangentType_Flat;
    defaultFlatKey.m_outTangentType = SCurveEditorKey::eTangentType_Flat;

    int curveNr = 0;
#define BEGIN_CURVE(name, defaultValue)                                 \
    {                                                                   \
        SCurveEditorContent content;                                    \
        content.m_curves.push_back(SCurveEditorCurve());                \
        content.m_curves.back().m_defaultValue = (float)(defaultValue); \
        const char* curveName = #name;                                  \
        curveNr++;
#define ADD_KEY(t, v, isFlat)                                                         \
    content.m_curves.back().m_keys.push_back((isFlat) ? defaultFlatKey : defaultKey); \
    content.m_curves.back().m_keys.back().m_time = (float)(t);                        \
    content.m_curves.back().m_keys.back().m_value = (float)(v);
#define END_CURVE()                                                                                                                       \
    AddPresetToLibrary(new QCurveSwatchWidget(content, this), m_currentLibrary, QString("Curve ") + QString::number(curveNr), true, false); \
    }


    BEGIN_CURVE("Ramp Up", 0.5)
    ADD_KEY(0, 0, false);
    ADD_KEY(1, 1, false);
    END_CURVE()
    BEGIN_CURVE("Ramp Down", 0.5)
    ADD_KEY(0, 1, false);
    ADD_KEY(1, 0, false);
    END_CURVE()
    BEGIN_CURVE("Ease in-out Up", 0.5)
    ADD_KEY(0, 0, true);
    ADD_KEY(1, 1, true);
    END_CURVE()
    BEGIN_CURVE("Ease in-out Down", 0.5)
    ADD_KEY(0, 1, true);
    ADD_KEY(1, 0, true);
    END_CURVE()
    BEGIN_CURVE("Ease out Up", 0.5)
    ADD_KEY(0, 0, false);
    ADD_KEY(1, 1, true);
    END_CURVE()
    BEGIN_CURVE("Ease in Down", 0.5)
    ADD_KEY(0, 1, true);
    ADD_KEY(1, 0, false);
    END_CURVE()
    BEGIN_CURVE("Ease in Up", 0.5)
    ADD_KEY(0, 0, true);
    ADD_KEY(1, 1, false);
    END_CURVE()
    BEGIN_CURVE("Ease out Down", 0.5)
    ADD_KEY(0, 1, false);
    ADD_KEY(1, 0, true);
    END_CURVE()
    BEGIN_CURVE("Bell", 0.5)
    ADD_KEY(0, 0, true);
    ADD_KEY(0.5, 1, true);
    ADD_KEY(1, 0, true);
    END_CURVE()
    BEGIN_CURVE("Bell flipped", 0.5)
    ADD_KEY(0, 1, true);
    ADD_KEY(0.5, 0, true);
    ADD_KEY(1, 1, true);
    END_CURVE()
    BEGIN_CURVE("Beta", 0.5)
    ADD_KEY(0, 0, false);
    ADD_KEY(0.5, 1, true);
    ADD_KEY(1, 0, false);
    END_CURVE()
    BEGIN_CURVE("Beta flipped", 0.5)
    ADD_KEY(0, 1, false);
    ADD_KEY(0.5, 0, true);
    ADD_KEY(1, 1, false);
    END_CURVE()

#undef BEGIN_CURVE
#undef ADD_KEY
#undef END_CURVE

    DisplayLibrary(m_currentLibrary);
    newLibFlag[m_currentLibrary] = false;
    unsavedFlag[m_currentLibrary] = false;
}

bool QCurveSelectorWidget::AddPresetToLibrary(QWidget* value, unsigned int lib, QString name /*= QString()*/, bool allowDefault /*= false*/, bool InsertAtTop /*= true*/)
{
    QCurveSwatchWidget* preset = ((QCurveSwatchWidget*)value);

    QString usedName = "Preset has no name";
    if (!name.isEmpty())
    {
        usedName = name;
    }

    int index = 0;
    if (InsertAtTop)
    {
        presets[lib].insert(0, new QPresetWidget(usedName, (new QCurveSwatchWidget(preset->GetContent(), this)), this));
    }
    else
    {
        index = presets[lib].count();
        presets[lib].push_back(new QPresetWidget(usedName, (new QCurveSwatchWidget(preset->GetContent(), this)), this));
    }
    int width = presetSizes[currentPresetSize].width;
    int height = presetSizes[currentPresetSize].height;
    presets[lib][index]->setPresetSize(width, height);

    StylePreset(presets[lib][index], false);
    presets[lib][index]->SetBackgroundStyle("background: none;");
    presets[lib][index]->installEventFilter(this);
    presets[lib][index]->setToolTip(usedName);
    unsavedFlag[lib] = true;
    DisplayLibrary(m_currentLibrary);
    return true;
}

QWidget* QCurveSelectorWidget::CreateSelectedWidget()
{
    return new QCurveSwatchWidget(m_current.content, this);
}

bool QCurveSelectorWidget::LoadSessionPresetFromName(QSettings* settings, const QString& libName)
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
        settings->beginGroup(preset); //start of this preset
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
                settings->beginGroup(key);
                _key.m_time = settings->value("time", "0").toFloat();
                _key.m_value = settings->value("value").toFloat();
                _p = settings->value("intan").toPointF();
                _key.m_inTangent = Vec2(_p.x(), _p.y());
                _p = settings->value("outtan").toPointF();
                _key.m_outTangent = Vec2(_p.x(), _p.y());
                _key.m_inTangentType = (SCurveEditorKey::ETangentType)settings->value("intype").toInt();
                _key.m_outTangentType = (SCurveEditorKey::ETangentType)settings->value("outtype").toInt();
                _curve.m_keys.push_back(_key);
                settings->endGroup();
            }
            SCurveEditorContent content;
            content.m_curves.push_back(_curve);
            AddPresetToLibrary(new QCurveSwatchWidget(content, this), index, curveName, true, false);
            settings->endGroup(); //end of this curve
        }
        settings->endGroup(); //end of this preset
    }
    settings->endGroup(); //end of this library
    newLibFlag[index] = false;
    unsavedFlag[index] = false;
    return success;
}

void QCurveSelectorWidget::SavePresetToSession(QSettings* settings, const int& lib, const int& preset)
{
    int numDigits = 3;
    int numericBase = 10;
    QChar fillerChar = '0';
    //assigning a path of 000-999 to preserve preset order, and prevent cases where duplicate names would create conflict
    settings->beginGroup(QString("%1").arg(preset, numDigits, numericBase, fillerChar));
    SCurveEditorContent _content = static_cast<QCurveSwatchWidget*>(presets[lib][preset]->GetWidget())->GetContent();

    for (unsigned int c = 0; c < _content.m_curves.size(); c++) //this preset
    {
        //assigning a path of 000-999 to preserve curve order, and prevent cases where duplicate names would create conflict
        settings->beginGroup(QString("%1").arg(c, numDigits, numericBase, fillerChar));
        SCurveEditorCurve _curve = _content.m_curves[c];
        settings->setValue("defValue", _curve.m_defaultValue);
        settings->setValue("name", presets[lib][preset]->GetName());
        for (unsigned int k = 0; k < _curve.m_keys.size(); k++)
        {
            //assigning a path of 000-999 to preserve key order, and prevent cases where duplicate names would create conflict
            settings->beginGroup(QString("%1").arg(k, numDigits, numericBase, fillerChar));
            SCurveEditorKey _key = _curve.m_keys[k];
            settings->setValue("time", _key.m_time);
            settings->setValue("value", _key.m_value);
            settings->setValue("intan", QPointF(_key.m_inTangent.x, _key.m_inTangent.y));
            settings->setValue("outtan", QPointF(_key.m_outTangent.x, _key.m_outTangent.y));
            settings->setValue("intype", _key.m_inTangentType);
            settings->setValue("outtype", _key.m_outTangentType);
            settings->endGroup();//end key
        }
        settings->endGroup();//end curve
    }
    settings->endGroup();//end preset
}

QString QCurveSelectorWidget::SettingsBaseGroup() const
{
    return "Curve Presets/";
}
