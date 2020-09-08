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
#include "GraphicsSettingsDialog.h"

//QT
#include <QVector>
#include <QMessageBox>
#include <QComboBox>
#include <QLabel>
#include <QFile>
#include <QLineEdit>
#include <QGridLayout>
#include <QDebug>
#include <QFontMetrics>

//Editor
#include <Util/AutoDirectoryRestoreFileDialog.h>
#include "IEditor.h"
#include "ILogFile.h"
#include "XConsole.h"
#include "ISourceControl.h"
#include <Util/FileUtil.h>
#include "XConsoleVariable.h"

#include <AzFramework/IO/LocalFileIO.h>

#include "ui_graphicssettingsdialog.h"

GraphicsSettingsDialog::GraphicsSettingsDialog(QWidget* parent /* = nullptr */)
    : QDialog(parent)
    , m_ui(new Ui::GraphicsSettingsDialog())
{
    //Update qlabel color when disabled
    setStyleSheet("QLabel::disabled{color: gray;}");

    // Start initialization the dialog
    m_isLoading = true;
    m_ui->setupUi(this);
    setWindowTitle("Graphics Settings");

    /////////////////////////////////////////////

    m_currentPlatform = GetISystem()->GetConfigPlatform();
    m_currentSpecIndex = 0;
    m_dirtyCVarCount = 0;

    m_showCustomSpec = true;
    ShowCustomSpecOption(false);

    // Show categories, disable apply button
    m_showCategories = true;
    m_ui->ApplyButton->setEnabled(false);

    m_iconRemove = QIcon(":/PropertyEditor/Resources/cross-small.png");

    m_cfgFiles[CONFIG_PC].push_back("pc_low.cfg");
    m_cfgFiles[CONFIG_PC].push_back("pc_medium.cfg");
    m_cfgFiles[CONFIG_PC].push_back("pc_high.cfg");
    m_cfgFiles[CONFIG_PC].push_back("pc_veryhigh.cfg");
    m_cfgFiles[CONFIG_OSX_METAL].push_back("osx_metal_low.cfg");
    m_cfgFiles[CONFIG_OSX_METAL].push_back("osx_metal_medium.cfg");
    m_cfgFiles[CONFIG_OSX_METAL].push_back("osx_metal_high.cfg");
    m_cfgFiles[CONFIG_OSX_METAL].push_back("osx_metal_veryhigh.cfg");
    m_cfgFiles[CONFIG_ANDROID].push_back("android_low.cfg");
    m_cfgFiles[CONFIG_ANDROID].push_back("android_medium.cfg");
    m_cfgFiles[CONFIG_ANDROID].push_back("android_high.cfg");
    m_cfgFiles[CONFIG_ANDROID].push_back("android_veryhigh.cfg");
    m_cfgFiles[CONFIG_IOS].push_back("ios_low.cfg");
    m_cfgFiles[CONFIG_IOS].push_back("ios_medium.cfg");
    m_cfgFiles[CONFIG_IOS].push_back("ios_high.cfg");
    m_cfgFiles[CONFIG_IOS].push_back("ios_veryhigh.cfg");
#if defined(AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS)
#define AZ_RESTRICTED_PLATFORM_EXPANSION(CodeName, CODENAME, codename, PrivateName, PRIVATENAME, privatename, PublicName, PUBLICNAME, publicname, PublicAuxName1, PublicAuxName2, PublicAuxName3)\
    m_cfgFiles[CONFIG_##CODENAME].push_back(#publicname "_low.cfg");\
    m_cfgFiles[CONFIG_##CODENAME].push_back(#publicname "_medium.cfg");\
    m_cfgFiles[CONFIG_##CODENAME].push_back(#publicname "_high.cfg");
    AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS
#undef AZ_RESTRICTED_PLATFORM_EXPANSION
#endif
    m_cfgFiles[CONFIG_APPLETV].push_back("appletv.cfg");

    m_cvarGroupData["Config/CVarGroups/sys_spec_gameeffects.cfg"].m_layout = m_ui->GameEffectsLayout;
    m_cvarGroupData["Config/CVarGroups/sys_spec_light.cfg"].m_layout = m_ui->LightLayout;
    m_cvarGroupData["Config/CVarGroups/sys_spec_objectdetail.cfg"].m_layout = m_ui->ObjectDetailLayout;
    m_cvarGroupData["Config/CVarGroups/sys_spec_particles.cfg"].m_layout = m_ui->ParticlesLayout;
    m_cvarGroupData["Config/CVarGroups/sys_spec_physics.cfg"].m_layout = m_ui->PhysicsLayout;
    m_cvarGroupData["Config/CVarGroups/sys_spec_postprocessing.cfg"].m_layout = m_ui->PostProcessingLayout;
    m_cvarGroupData["Config/CVarGroups/sys_spec_quality.cfg"].m_layout = m_ui->QualityLayout;
    m_cvarGroupData["Config/CVarGroups/sys_spec_shading.cfg"].m_layout = m_ui->ShadingLayout;
    m_cvarGroupData["Config/CVarGroups/sys_spec_shadows.cfg"].m_layout = m_ui->ShadowsLayout;
    m_cvarGroupData["Config/CVarGroups/sys_spec_sound.cfg"].m_layout = m_ui->SoundLayout;
    m_cvarGroupData["Config/CVarGroups/sys_spec_texture.cfg"].m_layout = m_ui->TextureLayout;
    m_cvarGroupData["Config/CVarGroups/sys_spec_textureresolution.cfg"].m_layout = m_ui->TextureResolutionLayout;
    m_cvarGroupData["Config/CVarGroups/sys_spec_volumetriceffects.cfg"].m_layout = m_ui->VolumetricEffectsLayout;
    m_cvarGroupData["Config/CVarGroups/sys_spec_water.cfg"].m_layout = m_ui->WaterLayout;
    m_cvarGroupData["SpecFile"].m_layout = m_ui->SpecFileHeaderLayout;
    m_cvarGroupData["miscellaneous"].m_layout = m_ui->MiscellaneousLayout;

    // Since the layout order is set by the .ui file we need to match the order here so that tabbing works correctly.
    m_cvarGroupOrder.push_back("Config/CVarGroups/sys_spec_gameeffects.cfg");
    m_cvarGroupOrder.push_back("Config/CVarGroups/sys_spec_light.cfg");
    m_cvarGroupOrder.push_back("Config/CVarGroups/sys_spec_objectdetail.cfg");
    m_cvarGroupOrder.push_back("Config/CVarGroups/sys_spec_particles.cfg");
    m_cvarGroupOrder.push_back("Config/CVarGroups/sys_spec_physics.cfg");
    m_cvarGroupOrder.push_back("Config/CVarGroups/sys_spec_postprocessing.cfg");
    m_cvarGroupOrder.push_back("Config/CVarGroups/sys_spec_quality.cfg");
    m_cvarGroupOrder.push_back("Config/CVarGroups/sys_spec_shading.cfg");
    m_cvarGroupOrder.push_back("Config/CVarGroups/sys_spec_shadows.cfg");
    m_cvarGroupOrder.push_back("Config/CVarGroups/sys_spec_sound.cfg");
    m_cvarGroupOrder.push_back("Config/CVarGroups/sys_spec_texture.cfg");
    m_cvarGroupOrder.push_back("Config/CVarGroups/sys_spec_textureresolution.cfg");
    m_cvarGroupOrder.push_back("Config/CVarGroups/sys_spec_volumetriceffects.cfg");
    m_cvarGroupOrder.push_back("Config/CVarGroups/sys_spec_water.cfg");
    m_cvarGroupOrder.push_back("miscellaneous");

    m_platformStrings.push_back(AZStd::make_pair("PC", CONFIG_PC));
    m_platformStrings.push_back(AZStd::make_pair("OSX Metal", CONFIG_OSX_METAL));
    m_platformStrings.push_back(AZStd::make_pair("Android", CONFIG_ANDROID));
    m_platformStrings.push_back(AZStd::make_pair("iOS", CONFIG_IOS));
#if defined(AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS)
#define AZ_RESTRICTED_PLATFORM_EXPANSION(CodeName, CODENAME, codename, PrivateName, PRIVATENAME, privatename, PublicName, PUBLICNAME, publicname, PublicAuxName1, PublicAuxName2, PublicAuxName3)\
    m_platformStrings.push_back(AZStd::make_pair(PublicAuxName2, CONFIG_##CODENAME));
    AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS
#undef AZ_RESTRICTED_PLATFORM_EXPANSION
#endif
    m_platformStrings.push_back(AZStd::make_pair("Apple TV", CONFIG_APPLETV));
    m_platformStrings.push_back(AZStd::make_pair("Custom", CONFIG_INVALID_PLATFORM));

    for (auto& platformString : m_platformStrings)
    {
        QString platform = platformString.first.c_str();
        m_ui->PlatformEntry->addItem(platform);
    }

    QSettings settings("Amazon", "Lumberyard");
    settings.beginGroup("GraphicsSettingsDialog");

    if (settings.contains("Platform"))
    {
        QString platformName = settings.value("Platform").toString();
        AZStd::string azPlatform = platformName.toUtf8().data();
        m_currentPlatform = GetConfigPlatformFromName(azPlatform);
        if (m_currentPlatform == CONFIG_INVALID_PLATFORM)
        {
            ShowCustomSpecOption(true);
        }
    }

    settings.endGroup();

    SetPlatformEntry(m_currentPlatform);

    BuildUI();

    m_isLoading = false;
}

GraphicsSettingsDialog::~GraphicsSettingsDialog()
{
    QSettings settings("Amazon", "Lumberyard");
    settings.beginGroup("GraphicsSettingsDialog");

    auto platformCheck = [this](AZStd::pair<AZStd::string, ESystemConfigPlatform>& stringConfigPair) { return stringConfigPair.second == m_currentPlatform; };
    auto platformStringsIterator = AZStd::find_if(m_platformStrings.begin(), m_platformStrings.end(), platformCheck);
    if (platformStringsIterator != m_platformStrings.end())
    {
        settings.setValue("Platform", platformStringsIterator->first.c_str());
    }
    else
    {
        settings.remove("Platform");
    }

    settings.beginGroup("cvarGroup");

    for (auto& collapseIterator : m_uiCollapseGroup)
    {
        settings.setValue(collapseIterator->m_groupName, collapseIterator->m_isCollapsed);
    }

    settings.endGroup();
    settings.endGroup();
    settings.sync();

    for (CollapseGroup* group : m_uiCollapseGroup)
    {
        // Destructor of CollapseGroup will set the members
        // pointing to nullptr. The actually destruction of the
        // widgets will be done on destruction of m_ui
        delete group;
    }

    for (ParameterWidget* widget : m_parameterWidgets)
    {
        // Destructor of ParameterWidget will set the members
        // pointing to nullptr. The actually destruction of the
        // widgets will be done on destruction of m_ui
        delete widget;
    }

    // Delete m_ui will destruct all the UI elements with in the ui file
    // Since m_ui is a QScopedPointer which will cleanup itself on the end
    // of the scope.
}

void GraphicsSettingsDialog::OnLoadConfigurationEntry(const char* strKey, const char* strValue, const char* strGroup)
{
    AZStd::string key = strKey;
    ICVar* cvar = gEnv->pConsole->GetCVar(strKey);

    if (cvar)
    {
        AZStd::transform(key.begin(), key.end(), key.begin(), tolower);
        if (azstricmp(key.c_str(), "sys_spec_full") == 0 || key.find("sys_spec_") == key.npos)
        {
            int type = cvar->GetType();
            AZStd::any val;
            if (type == CVAR_INT)
            {
                val = atoi(strValue);
            }
            else if (type == CVAR_FLOAT)
            {
                val = static_cast<float>(atof(strValue));
            }
            else
            {
                val = AZStd::string(strValue);
            }

            // Platform cfg file (ex. pc_veryhigh.cfg)
            if (strGroup[0] == '\0')
            {
                // New cvar loaded into map
                if (m_cVarTracker.find(key) == m_cVarTracker.end())
                {
                    m_cVarTracker[key].type = type;
                    m_cVarTracker[key].cvarGroup = "miscellaneous";
                    AZStd::any empty;
                    if (type == CVAR_INT)
                    {
                        empty = 0;
                    }
                    else if (type == CVAR_FLOAT)
                    {
                        empty = 0.0f;
                    }
                    else
                    {
                        empty = AZStd::string("");
                    }
                    m_cVarTracker[key].fileVals.resize(m_numSpecLevels, CVarFileStatus(empty, empty, empty));
                }

                m_cVarTracker[key].fileVals[m_currentSpecIndex].editedValue = val;
                m_cVarTracker[key].fileVals[m_currentSpecIndex].overwrittenValue = val;
            }
            // default group in sys_spec cfg file
            else if (azstricmp(strGroup, "default") == 0)
            {
                CVarFileStatus defaultVal(val, val, val);
                if (m_cVarTracker.find(key) == m_cVarTracker.end())
                {
                    // New cvar loaded into map
                    CVarInfo& currentCVar = m_cVarTracker[key];
                    currentCVar.type = cvar->GetType();
                    currentCVar.fileVals.resize(m_numSpecLevels, defaultVal);
                }
                else
                {
                    // Reset values, if there's a platform override it always follows the sys_spec_*.cfg files.
                    // Resetting avoids the issue where some spec levels are never set because of an extra platform
                    // override load happening earlier just to store the value of sys_spec_full.
                    for (int specLevel = 0; specLevel < m_numSpecLevels; ++specLevel)
                    {
                        m_cVarTracker[key].fileVals[specLevel] = defaultVal;
                    }
                }
                // Overwrite miscellaneous if mentioned in platform config file
                m_cVarTracker[key].cvarGroup = m_currentConfigFilename;
            }
            // specific index in sys_spec cfg file
            else
            {
                int group = 0;

                if (azsscanf(strGroup, "%d", &group) == 1)
                {
                    auto sysSpecFull = m_cVarTracker.find("sys_spec_full");
                    if (sysSpecFull != m_cVarTracker.end())
                    {
                        CVarFileStatus indexAssignment(val, val, val);
                        for (int specLevel = 0; specLevel < m_numSpecLevels; ++specLevel)
                        {
                            // Only apply cvar change to configurations with sys_spec_Full matching the index
                            int overwrittenValue = 0;
                            if (AZStd::any_numeric_cast<int>(&sysSpecFull->second.fileVals[specLevel].overwrittenValue, overwrittenValue) && group == overwrittenValue)
                            {
                                m_cVarTracker[key].fileVals[specLevel] = indexAssignment;
                            }
                        }
                    }                    
                }
            }
        }
    }
}

// Loads UI column for specific cfg file (ex. pc_low.cfg)
void GraphicsSettingsDialog::BuildColumn(int specLevel)
{
    if (specLevel < 0 || specLevel >= m_numSpecLevels)
    {
        return;
    }

    int row = CVAR_ROW_OFFSET;
    for (auto& it : m_cVarTracker)
    {
        QString str = it.first.c_str();
        QWidget* input = nullptr;
        if (it.second.type == CVAR_INT)
        {
            AzQtComponents::SpinBox* intval = new AzQtComponents::SpinBox(nullptr);
            intval->setFocusPolicy(Qt::StrongFocus);
            intval->setMaximum(INT32_MAX);
            intval->setMinimum(INT32_MIN);
            int editedValue;
            if (AZStd::any_numeric_cast<int>(&it.second.fileVals[specLevel].editedValue, editedValue))
            {
                intval->setValue(editedValue);
            }
            m_cvarGroupData[it.second.cvarGroup].m_cvarSpinBoxes.push_back(intval);
            connect(intval, SIGNAL(valueChanged(int)), this, SLOT(CVarChanged(int)));
            input = intval;
        }
        else if (it.second.type == CVAR_FLOAT)
        {
            AzQtComponents::DoubleSpinBox* doubleval = new AzQtComponents::DoubleSpinBox(nullptr);
            doubleval->setFocusPolicy(Qt::StrongFocus);
            doubleval->setMaximum(FLT_MAX);
            doubleval->setMinimum(-FLT_MAX);
            float editedValue;
            if (AZStd::any_numeric_cast<float>(&it.second.fileVals[specLevel].editedValue, editedValue))
            {
                doubleval->setValue(editedValue);
            }
            m_cvarGroupData[it.second.cvarGroup].m_cvarDoubleSpinBoxes.push_back(doubleval);
            connect(doubleval, SIGNAL(valueChanged(double)), this, SLOT(CVarChanged(double)));
            input = doubleval;
        }
        else
        {
            QLineEdit* stringval = new QLineEdit(nullptr);
            AZStd::string* editedValue = AZStd::any_cast<AZStd::string>(&it.second.fileVals[specLevel].editedValue);
            stringval->setText(editedValue->c_str());
            m_cvarGroupData[it.second.cvarGroup].m_cvarLineEdits.push_back(stringval);
            connect(stringval, SIGNAL(textChanged(const QString&)), this, SLOT(CVarChanged(const QString&)));
            input = stringval;
        }

        if (input)
        {
            input->setObjectName(str);
            input->setProperty("specLevel", specLevel);
            QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            sizePolicy.setHeightForWidth(input->sizePolicy().hasHeightForWidth());
            input->setSizePolicy(sizePolicy);
            input->setMinimumSize(QSize(INPUT_MIN_WIDTH, INPUT_MIN_HEIGHT));
            m_cvarGroupData[it.second.cvarGroup].m_layout->addWidget(input, row, specLevel + CVAR_VALUE_COLUMN_OFFSET, INPUT_ROW_SPAN, INPUT_COLUMN_SPAN);
            m_parameterWidgets.push_back(new ParameterWidget(input, it.first.c_str()));
            m_cvarGroupData[it.second.cvarGroup].m_widgetInsertOrder.push_back(input);
        }

        row++;
    }
}

void GraphicsSettingsDialog::LoadPlatformConfigurations()
{
    for (ParameterWidget* widget : m_parameterWidgets)
    {
        delete widget;
    }
    m_parameterWidgets.clear();

    m_ui->ApplyButton->setEnabled(false);
    m_dirtyCVarCount = 0;

    // Load platform cfg files to load in sys_spec_Full
    for (size_t cfgFileIndex = 0; cfgFileIndex < m_cfgFiles[m_currentPlatform].size(); ++cfgFileIndex)
    {
        m_currentSpecIndex = cfgFileIndex;
        m_currentConfigFilename = m_cfgFiles[m_currentPlatform][cfgFileIndex];
        GetISystem()->LoadConfiguration(m_currentConfigFilename.c_str(), this, true);
    }

    if (m_cVarTracker.find("sys_spec_full") == m_cVarTracker.end())
    {
        m_cVarTracker.clear();
        CleanUI();
        ShowCategories(false);
        QMessageBox::warning(this, "Warning", "Invalid custom spec file (missing sys_spec_full).",
            QMessageBox::Ok);
        return;
    }

    // Load sys_spec cfgs based on sys_spec_Full values
    LoadCVarGroupDirectory(m_cvarGroupsFolder);

    setUpdatesEnabled(false);

    // Reload platform cfg files to override sys_spec index assignments and load rows with filenames of given platform
    for (size_t cfgFileIndex = 0; cfgFileIndex < m_cfgFiles[m_currentPlatform].size(); ++cfgFileIndex)
    {
        m_currentSpecIndex = cfgFileIndex;
        m_currentConfigFilename = m_cfgFiles[m_currentPlatform][cfgFileIndex];
        GetISystem()->LoadConfiguration(m_currentConfigFilename.c_str(), this, true);

        CVarGroupInfo& specFileGroup = m_cvarGroupData["SpecFile"];

        QPushButton* invisibleButton = new QPushButton(nullptr);
        invisibleButton->setMinimumWidth(INPUT_MIN_WIDTH);
        invisibleButton->setMinimumHeight(INPUT_MIN_HEIGHT);
        specFileGroup.m_layout->addWidget(invisibleButton, PLATFORM_LABEL_ROW - 1, cfgFileIndex + CVAR_VALUE_COLUMN_OFFSET, INPUT_ROW_SPAN, INPUT_COLUMN_SPAN);
        
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        invisibleButton->setSizePolicy(sizePolicy);
        invisibleButton->setStyleSheet("border: none; background-color: transparent; padding: 0ex;");
        invisibleButton->setProperty("SpecConfigFileIndex", static_cast<int>(cfgFileIndex));
        invisibleButton->installEventFilter(this);
        specFileGroup.m_specFileArea.push_back(invisibleButton);
        
        QHBoxLayout* specFileLayout = new QHBoxLayout(invisibleButton);
        specFileLayout->setContentsMargins(0, 0, 0, 0);
        specFileLayout->setSpacing(5);
        
        QLabel* platformLabel = new QLabel(nullptr);
        QString str = m_cfgFiles[m_currentPlatform][cfgFileIndex].c_str();
        platformLabel->setObjectName(str.append("Label"));
        sizePolicy.setHeightForWidth(platformLabel->sizePolicy().hasHeightForWidth());
        platformLabel->setSizePolicy(sizePolicy);
        platformLabel->setMinimumHeight(INPUT_MIN_HEIGHT);
        platformLabel->setAlignment(Qt::AlignLeading | Qt::AlignLeft | Qt::AlignVCenter);
            
        specFileLayout->addWidget(platformLabel);
        platformLabel->setText(QApplication::translate("GraphicsSettingsDialog", m_cfgFiles[m_currentPlatform][cfgFileIndex].c_str(), 0));
        specFileGroup.m_platformLabels.push_back(platformLabel);

        if (m_showCustomSpec)
        {
            QPushButton* customSpecUnloadButton = new QPushButton(nullptr);
            customSpecUnloadButton->setVisible(false);
            customSpecUnloadButton->setFlat(true);
            customSpecUnloadButton->setStyleSheet("border: none; background-color: transparent; padding: 0ex;");
            customSpecUnloadButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            customSpecUnloadButton->setFixedSize(QSize(16, 16));
            customSpecUnloadButton->setIcon(m_iconRemove);
            customSpecUnloadButton->setToolTip(tr("Unload this spec file."));

            specFileLayout->addWidget(customSpecUnloadButton);
            specFileLayout->addStretch(1);
            connect(customSpecUnloadButton, &QPushButton::clicked, this, [this, cfgFileIndex]() {UnloadCustomSpec(cfgFileIndex); });
            specFileGroup.m_customSpecUnloadButtons.push_back(customSpecUnloadButton);
        }
    }

    // Loads column of cvar names
    int row = CVAR_ROW_OFFSET;
    for (auto& it : m_cVarTracker)
    {
        QLabel* cVarLabel = new QLabel(nullptr);
        QString str = it.first.c_str();
        QString strLabel = str + "Label";
        cVarLabel->setObjectName(strLabel);
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        sizePolicy.setHeightForWidth(cVarLabel->sizePolicy().hasHeightForWidth());
        cVarLabel->setSizePolicy(sizePolicy);
        cVarLabel->setMinimumSize(QSize(INPUT_MIN_WIDTH, INPUT_MIN_HEIGHT));
        cVarLabel->setAlignment(Qt::AlignLeading | Qt::AlignLeft | Qt::AlignVCenter);
        m_cvarGroupData[it.second.cvarGroup].m_layout->addWidget(cVarLabel, row, CVAR_LABEL_COLUMN, INPUT_ROW_SPAN, INPUT_COLUMN_SPAN);
        cVarLabel->setText(QApplication::translate("GraphicsSettingsDialog", it.first.c_str(), 0));
        m_cvarGroupData[it.second.cvarGroup].m_cvarLabels.push_back(cVarLabel);

        if (ICVar* cvar = gEnv->pConsole->GetCVar(it.first.c_str()))
        {
            cVarLabel->setToolTip(cvar->GetHelp());
        }

        row++;
    }

    // Loads columns of cvar values for each platform cfg file
    for (int cfgFileIndex = 0; cfgFileIndex < m_cfgFiles[m_currentPlatform].size(); ++cfgFileIndex)
    {
        BuildColumn(cfgFileIndex);
    }

    // Hide any section that has no visible controls
    for (auto& it : m_cvarGroupData)
    {
        int totalControlCount = 0;
        CVarGroupInfo& currentCvarGroupInfo = it.second;
        totalControlCount += currentCvarGroupInfo.m_cvarDoubleSpinBoxes.size();
        totalControlCount += currentCvarGroupInfo.m_cvarSpinBoxes.size();
        totalControlCount += currentCvarGroupInfo.m_cvarLineEdits.size();
        totalControlCount += currentCvarGroupInfo.m_platformLabels.size();

        if (totalControlCount == 0)
        {
            // disable labels
            for (auto& label : currentCvarGroupInfo.m_platformLabels)
            {
                if (label)
                {
                    label->setVisible(false);
                }
            }

            // disable widgets
            QGridLayout* currentLayout = currentCvarGroupInfo.m_layout;
            for (int currentLayoutItem = 0; currentLayoutItem < currentLayout->count(); ++currentLayoutItem)
            {
                QWidget* widget = currentLayout->itemAt(currentLayoutItem)->widget();
                if (widget)
                {
                    widget->setVisible(false);
                }
            }
        }
    }

    SetCVarWidgetTabOrder();

    setUpdatesEnabled(true);

    QSettings settings("Amazon", "Lumberyard");
    settings.beginGroup("GraphicsSettingsDialog");
    settings.beginGroup("cvarGroup");

    for (auto& collapseIterator : m_uiCollapseGroup)
    {
        bool groupCollapsed = settings.value(collapseIterator->m_groupName, collapseIterator->m_isCollapsed).toBool();
        if (collapseIterator->m_isCollapsed != groupCollapsed)
        {
            collapseIterator->ToggleCollpased();
        }
    }

    settings.endGroup();
    settings.endGroup();
}

void GraphicsSettingsDialog::SetCVarWidgetTabOrder()
{
    int numColumns = m_cfgFiles[m_currentPlatform].size();
    if (numColumns == 0)
    {
        return;
    }

    QWidget* lastGroupWidget = nullptr;
    for (auto& it : m_cvarGroupOrder)
    {
        QVector<QWidget*>& widgetTabVector = m_cvarGroupData[it].m_widgetInsertOrder;
        int numWidgets = widgetTabVector.size();
        int rowsInGroup = numWidgets / numColumns;

        if (numWidgets)
        {
            if (lastGroupWidget)
            {
                // set the last widget of the last group to tab to the first of this one
                setTabOrder(lastGroupWidget, widgetTabVector.first());
            }
            lastGroupWidget = widgetTabVector.back();
        }

        for (int currentRow = 0; currentRow < rowsInGroup; ++currentRow)
        {
            for (int currentColumn = 0; currentColumn < numColumns - 1; ++currentColumn)
            {
                setTabOrder(widgetTabVector[currentRow + (rowsInGroup*currentColumn)], widgetTabVector[currentRow + (rowsInGroup*(currentColumn + 1))]);
            }
            if (currentRow < rowsInGroup - 1)
            {
                // Set the last input on this row to the first of the next row
                setTabOrder(widgetTabVector[currentRow + (rowsInGroup*(numColumns - 1))], widgetTabVector[currentRow + 1]);
            }
        }
    }
}

bool GraphicsSettingsDialog::eventFilter(QObject *obj, QEvent *event)
{
    // This event filter is currently only used for the inivisible button
    // in which case we still want to pass the events to the other Qt objects
    // even when the Enter/Leave is handled here.
    if ((event->type() == QEvent::Enter) || (event->type() == QEvent::Leave))
    {
        CVarGroupInfo& fileSpecGroup = m_cvarGroupData["SpecFile"];
        QVariant cfgFileIndexVariant = obj->property("SpecConfigFileIndex");
        int cfgFileIndex = cfgFileIndexVariant.toInt();
        if (cfgFileIndex >= 0 && cfgFileIndex < fileSpecGroup.m_customSpecUnloadButtons.size())
        {
            bool unloadButtonVisible = (event->type() == QEvent::Enter);
            fileSpecGroup.m_customSpecUnloadButtons[cfgFileIndex]->setVisible(unloadButtonVisible);
        }
    }

    return QWidget::eventFilter(obj, event);
}

void GraphicsSettingsDialog::LoadCVarGroupDirectory(const AZStd::string& path)
{
    AZ::IO::LocalFileIO::FindFilesCallbackType fileFinderCb;
    fileFinderCb = [&](const char* fullPath) -> bool
    {
        if (gEnv->pFileIO->IsDirectory(fullPath))
        {
            // recurse into subdirectory
            gEnv->pFileIO->FindFiles(fullPath, "*.cfg", fileFinderCb);
        }
        else
        {
            m_currentConfigFilename = fullPath;
            GetISystem()->LoadConfiguration(fullPath, this, false);
        }

        return true; // keep searching
    };

    gEnv->pFileIO->FindFiles(path.c_str(), "*.cfg", fileFinderCb);
}

//Build UI, link signals and set the data for device list.
void GraphicsSettingsDialog::BuildUI()
{
    //Collpase Buttons
    SetCollapsedLayout("GameEffects", m_ui->GameEffectsDropdown, m_ui->GameEffectsLayout);
    SetCollapsedLayout("Light", m_ui->LightDropdown, m_ui->LightLayout);
    SetCollapsedLayout("ObjectDetail", m_ui->ObjectDetailDropdown, m_ui->ObjectDetailLayout);
    SetCollapsedLayout("Particles", m_ui->ParticlesDropdown, m_ui->ParticlesLayout);
    SetCollapsedLayout("Physics", m_ui->PhysicsDropdown, m_ui->PhysicsLayout);
    SetCollapsedLayout("PostProcessing", m_ui->PostProcessingDropdown, m_ui->PostProcessingLayout);
    SetCollapsedLayout("Quality", m_ui->QualityDropdown, m_ui->QualityLayout);
    SetCollapsedLayout("Shading", m_ui->ShadingDropdown, m_ui->ShadingLayout);
    SetCollapsedLayout("Shadows", m_ui->ShadowsDropdown, m_ui->ShadowsLayout);
    SetCollapsedLayout("Sound", m_ui->SoundDropdown, m_ui->SoundLayout);
    SetCollapsedLayout("Texture", m_ui->TextureDropdown, m_ui->TextureLayout);
    SetCollapsedLayout("TextureResolution", m_ui->TextureResolutionDropdown, m_ui->TextureResolutionLayout);
    SetCollapsedLayout("VolumetricEffects", m_ui->VolumetricEffectsDropdown, m_ui->VolumetricEffectsLayout);
    SetCollapsedLayout("Water", m_ui->WaterDropdown, m_ui->WaterLayout);
    SetCollapsedLayout("Miscellaneous", m_ui->MiscellaneousDropdown, m_ui->MiscellaneousLayout);

    connect(m_ui->CancelButton, &QPushButton::clicked, this, &GraphicsSettingsDialog::reject);
    connect(m_ui->ApplyButton, &QPushButton::clicked, this, &GraphicsSettingsDialog::accept);
    connect(m_ui->PlatformEntry, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(PlatformChanged(const QString&)));
    connect(m_ui->SelectCustomSpecButton, &QPushButton::clicked, this, &GraphicsSettingsDialog::OpenCustomSpecDialog);

    if (!m_cfgFiles[m_currentPlatform].empty())
    {
        LoadPlatformConfigurations();
    }
    else
    {
        m_cVarTracker.clear();
        CleanUI();
        ShowCategories(false);
    }
}

template<typename T>
void GraphicsSettingsDialog::RemoveWidgetsFromLayout(QVector<T*>& widgetVector, QGridLayout* layout)
{
    if (layout == nullptr)
    {
        AZ_Error("GraphicsSettingsDialog", false, "Trying to remove widgets from a layout that doesn't exist.");
        return;
    }

    for (auto& it : widgetVector)
    {
        layout->removeWidget(it);
        delete it;
    }
    widgetVector.clear();
}

void GraphicsSettingsDialog::CleanUI()
{
    setUpdatesEnabled(false);

    QSettings settings("Amazon", "Lumberyard");
    settings.beginGroup("GraphicsSettingsDialog");
    settings.beginGroup("cvarGroup");

    for (auto& collapseIterator : m_uiCollapseGroup)
    {
        settings.setValue(collapseIterator->m_groupName, collapseIterator->m_isCollapsed);
    }

    settings.endGroup();
    settings.endGroup();
    settings.sync();

    for (auto& cvarGroupIt : m_cvarGroupData)
    {
        CVarGroupInfo& currentGroup = cvarGroupIt.second;
        RemoveWidgetsFromLayout(currentGroup.m_platformLabels, currentGroup.m_layout);
        RemoveWidgetsFromLayout(currentGroup.m_cvarLabels, currentGroup.m_layout);
        RemoveWidgetsFromLayout(currentGroup.m_cvarSpinBoxes, currentGroup.m_layout);
        RemoveWidgetsFromLayout(currentGroup.m_cvarDoubleSpinBoxes, currentGroup.m_layout);
        RemoveWidgetsFromLayout(currentGroup.m_cvarLineEdits, currentGroup.m_layout);
        RemoveWidgetsFromLayout(currentGroup.m_customSpecUnloadButtons, currentGroup.m_layout);
        RemoveWidgetsFromLayout(currentGroup.m_specFileArea, currentGroup.m_layout);
        currentGroup.m_widgetInsertOrder.clear();
    }

    m_cVarTracker.clear();

    // Uncollapse groups
    for (auto& collapseIt : m_uiCollapseGroup)
    {
        if (collapseIt->m_isCollapsed)
        {
            collapseIt->ToggleCollpased();
        }
    }

    setUpdatesEnabled(true);
}

void GraphicsSettingsDialog::ShowCategories(bool show)
{
    if (m_showCategories == show)
    {
        return;
    }

    m_showCategories = show;

    m_ui->GameEffectsDropdown->setVisible(m_showCategories);
    m_ui->LightDropdown->setVisible(m_showCategories);
    m_ui->ObjectDetailDropdown->setVisible(m_showCategories);
    m_ui->ParticlesDropdown->setVisible(m_showCategories);
    m_ui->PhysicsDropdown->setVisible(m_showCategories);
    m_ui->PostProcessingDropdown->setVisible(m_showCategories);
    m_ui->QualityDropdown->setVisible(m_showCategories);
    m_ui->ShadingDropdown->setVisible(m_showCategories);
    m_ui->ShadowsDropdown->setVisible(m_showCategories);
    m_ui->SoundDropdown->setVisible(m_showCategories);
    m_ui->TextureDropdown->setVisible(m_showCategories);
    m_ui->TextureResolutionDropdown->setVisible(m_showCategories);
    m_ui->VolumetricEffectsDropdown->setVisible(m_showCategories);
    m_ui->WaterDropdown->setVisible(m_showCategories);
    m_ui->MiscellaneousDropdown->setVisible(m_showCategories);

    m_ui->SpecFileHeaderLabel->setVisible(m_showCategories);
    m_ui->GameEffectsLabel->setVisible(m_showCategories);
    m_ui->LightLabel->setVisible(m_showCategories);
    m_ui->ObjectDetailLabel->setVisible(m_showCategories);
    m_ui->ParticlesLabel->setVisible(m_showCategories);
    m_ui->PhysicsLabel->setVisible(m_showCategories);
    m_ui->PostProcessingLabel->setVisible(m_showCategories);
    m_ui->QualityLabel->setVisible(m_showCategories);
    m_ui->ShadingLabel->setVisible(m_showCategories);
    m_ui->ShadowsLabel->setVisible(m_showCategories);
    m_ui->SoundLabel->setVisible(m_showCategories);
    m_ui->TextureLabel->setVisible(m_showCategories);
    m_ui->TextureResolutionLabel->setVisible(m_showCategories);
    m_ui->VolumetricEffectsLabel->setVisible(m_showCategories);
    m_ui->WaterLabel->setVisible(m_showCategories);
    m_ui->MiscellaneousLabel->setVisible(m_showCategories);
}

void GraphicsSettingsDialog::ShowCustomSpecOption(bool show)
{
    if (m_showCustomSpec == show)
    {
        return;
    }

    m_showCustomSpec = show;

    m_ui->SelectCustomSpecButton->setVisible(m_showCustomSpec);
}

void GraphicsSettingsDialog::PlatformChanged(const QString& platform)
{
    bool change = true;
    if (m_dirtyCVarCount > 0 && !SendUnsavedChangesWarning(false))
    {
        change = false;
    }

    if (change)
    {
        AZStd::string azPlatform = platform.toStdString().c_str();

        m_currentPlatform = GetConfigPlatformFromName(azPlatform);
        if (m_currentPlatform == CONFIG_INVALID_PLATFORM) // "Custom" selected
        {
            ShowCustomSpecOption(true);
            CleanUI();
            if (m_cfgFiles[CONFIG_INVALID_PLATFORM].empty()) // if we don't have a custom spec
            {
                ShowCategories(false);
                m_ui->ApplyButton->setEnabled(false);
                m_dirtyCVarCount = 0;
            }
            else
            {
                ShowCategories(true);
                LoadPlatformConfigurations();
            }
        }
        else
        {
            ShowCustomSpecOption(false);
            ShowCategories(true);
            CleanUI();
            LoadPlatformConfigurations();
        }
    }
    else
    {
        m_ui->PlatformEntry->blockSignals(true);
        SetPlatformEntry(m_currentPlatform);
        m_ui->PlatformEntry->blockSignals(false);
    }
}

bool GraphicsSettingsDialog::SendUnsavedChangesWarning(bool cancel)
{
    int result = QMessageBox::Yes;

    if (cancel)
    {
        result = QMessageBox::question(this, "Warning", "There are currently unsaved changed. Are you sure you want to cancel?",
            QMessageBox::Yes, QMessageBox::No);
    }
    else
    {
        result = QMessageBox::question(this, "Warning", "There are currently unsaved changed. Are you sure you want to change configurations?",
            QMessageBox::Yes, QMessageBox::No);
    }

    return (result == QMessageBox::Yes);
}

bool GraphicsSettingsDialog::CVarChanged(AZStd::any val, const char* cvarName, int specLevel)
{
    // Checking if the edited value (before change) is equal to the overwritten value
    bool dirtyBefore = false;
    AZStd::string azcvarName = cvarName;
    AZStd::pair<AZStd::string, CVarInfo> cvarInfo = AZStd::make_pair(azcvarName, m_cVarTracker[cvarName]);
    if (CheckCVarStatesForDiff(&cvarInfo, specLevel, EDITED_OVERWRITTEN_COMPARE))
    {
        dirtyBefore = true;
    }

    if (azstricmp(cvarName, "sys_spec_full") == 0)
    {
        //Pop out the warning dialog for sys_spec_Full since all cvars will be changed
        int result = QMessageBox::Ok;

        result = QMessageBox::question(this, "Warning", "Modifying sys_spec_full will override any unsaved changes.",
            QMessageBox::Ok, QMessageBox::Cancel);

        // Cancel - Change sys_spec_full qspinbox value back
        if (result == QMessageBox::Cancel)
        {
            return false;
        }
        else // OK - reload column
        {
            // Updating sys_spec_full for when adding cvargroup directory
            m_cVarTracker[cvarName].fileVals[specLevel].editedValue = val;
            gEnv->pSystem->AddCVarGroupDirectory(m_cvarGroupsFolder);
            m_currentConfigFilename = m_cfgFiles[m_currentPlatform][specLevel];
            m_currentSpecIndex = specLevel;
            GetISystem()->LoadConfiguration(m_currentConfigFilename.c_str(), this, true);
            // Updating sys_spec_full since overwritten from loading platform cfg
            m_cVarTracker[cvarName].fileVals[specLevel].editedValue = val;
            BuildColumn(specLevel);
        }
    }
    else
    {
        m_cVarTracker[cvarName].fileVals[specLevel].editedValue = val;
    }

    // If changing cvar from the platform cfg file currently running, set cvar
    if (GetISystem()->GetConfigPlatform() == m_currentPlatform && GetISystem()->GetConfigSpec() == specLevel + 1)
    {
        if (ICVar* cvar = gEnv->pConsole->GetCVar(cvarName))
        {
            int type = cvar->GetType();
            if (type == CVAR_INT)
            {
                int newValue;
                if (AZStd::any_numeric_cast<int>(&val, newValue))
                {
                    cvar->Set(newValue);
                }
            }
            else if (type == CVAR_FLOAT)
            {
                float newValue;
                if (AZStd::any_numeric_cast<float>(&val, newValue))
                {
                    cvar->Set(newValue);
                }
            }
            else
            {
                AZStd::string* currValue = AZStd::any_cast<AZStd::string>(&val);
                cvar->Set(currValue->c_str());
            }
        }
    }

    // Checking if the newly edited value is equal to the overwritten value
    cvarInfo = AZStd::make_pair(azcvarName, m_cVarTracker[cvarName]);
    if (CheckCVarStatesForDiff(&cvarInfo, specLevel, EDITED_OVERWRITTEN_COMPARE))
    {
        if (!dirtyBefore)
        {
            m_ui->ApplyButton->setEnabled(true);
            m_dirtyCVarCount++;
        }
    }
    else
    {
        if (dirtyBefore)
        {
            m_dirtyCVarCount--;
            if (m_dirtyCVarCount == 0)
            {
                m_ui->ApplyButton->setEnabled(false);
            }
        }
    }

    return true;
}

void GraphicsSettingsDialog::CVarChanged(int i)
{
    AzQtComponents::SpinBox* box = static_cast<AzQtComponents::SpinBox*>(sender());
    QString str = box->objectName();
    QByteArray ba = str.toUtf8();
    const char* cvarName = ba.data();

    int specLevel = box->property("specLevel").toInt();

    AZStd::any val;
    val = i;

    if (!CVarChanged(val, cvarName, specLevel))
    {
        // sys_spec_full warning cancelled
        box->blockSignals(true);
        int editedValue;
        if (AZStd::any_numeric_cast<int>(&m_cVarTracker[cvarName].fileVals[specLevel].editedValue, editedValue))
        {
            box->setValue(editedValue);
        }
        box->blockSignals(false);
    }
}

void GraphicsSettingsDialog::CVarChanged(double d)
{
    AzQtComponents::DoubleSpinBox* box = static_cast<AzQtComponents::DoubleSpinBox*>(sender());
    QString str = box->objectName();
    QByteArray ba = str.toUtf8();
    const char* cvarName = ba.data();

    AZStd::any val;
    val = d;

    int specLevel = box->property("specLevel").toInt();

    if (!CVarChanged(val, cvarName, specLevel))
    {
        // only can return false from sys_spec_full, which is an int cvar
    }
}

void GraphicsSettingsDialog::CVarChanged(const QString& s)
{
    QLineEdit* box = qobject_cast<QLineEdit*>(sender());
    QString str = box->objectName();
    QByteArray ba = str.toUtf8();
    const char* cvarName = ba.data();

    AZStd::any val;
    val = AZStd::string(s.toStdString().c_str());

    int specLevel = box->property("specLevel").toInt();

    if (!CVarChanged(val, cvarName, specLevel))
    {
        // only can return false from sys_spec_full, which is an int cvar
    }
}

// Returns true if there is a difference between the two cvar states
bool GraphicsSettingsDialog::CheckCVarStatesForDiff(AZStd::pair<AZStd::string, CVarInfo>* it, int cfgFileIndex, CVarStateComparison cmp)
{
    if (it->second.type == CVAR_INT)
    {
        int editedVal, overwrittenVal, originalVal;
        if (AZStd::any_numeric_cast<int>(&it->second.fileVals[cfgFileIndex].editedValue, editedVal) &&
            AZStd::any_numeric_cast<int>(&it->second.fileVals[cfgFileIndex].overwrittenValue, overwrittenVal) &&
            AZStd::any_numeric_cast<int>(&it->second.fileVals[cfgFileIndex].originalValue, originalVal))
        {
            if ((cmp == EDITED_OVERWRITTEN_COMPARE && editedVal != overwrittenVal) ||
                (cmp == EDITED_ORIGINAL_COMPARE && editedVal != originalVal) ||
                (cmp == OVERWRITTEN_ORIGINAL_COMPARE && overwrittenVal != originalVal))
            {
                return true;
            }
        }
    }
    else if (it->second.type == CVAR_FLOAT)
    {
        float editedVal, overwrittenVal, originalVal;
        if (AZStd::any_numeric_cast<float>(&it->second.fileVals[cfgFileIndex].editedValue, editedVal) &&
            AZStd::any_numeric_cast<float>(&it->second.fileVals[cfgFileIndex].overwrittenValue, overwrittenVal) &&
            AZStd::any_numeric_cast<float>(&it->second.fileVals[cfgFileIndex].originalValue, originalVal))
        {
            if ((cmp == EDITED_OVERWRITTEN_COMPARE && editedVal != overwrittenVal) ||
                (cmp == EDITED_ORIGINAL_COMPARE && editedVal != originalVal) ||
                (cmp == OVERWRITTEN_ORIGINAL_COMPARE && overwrittenVal != originalVal))
            {
                return true;
            }
        }
    }
    else
    {
        AZStd::string editedVal = *AZStd::any_cast<AZStd::string>(&it->second.fileVals[cfgFileIndex].editedValue);
        AZStd::string overwrittenVal = *AZStd::any_cast<AZStd::string>(&it->second.fileVals[cfgFileIndex].overwrittenValue);
        AZStd::string originalVal = *AZStd::any_cast<AZStd::string>(&it->second.fileVals[cfgFileIndex].originalValue);
        if ((cmp == EDITED_OVERWRITTEN_COMPARE && editedVal.compare(overwrittenVal) != 0) ||
            (cmp == EDITED_ORIGINAL_COMPARE && editedVal.compare(originalVal) != 0) ||
            (cmp == OVERWRITTEN_ORIGINAL_COMPARE && overwrittenVal.compare(originalVal) != 0))
        {
            return true;
        }
    }
    return false;
}

///////////////////////////////////////////////////////////////////////
// settings file management
void GraphicsSettingsDialog::reject()
{
    if (m_dirtyCVarCount > 0)
    {
        if (SendUnsavedChangesWarning(true))
        {
            QDialog::reject();
        }
    }
    else
    {
        QDialog::reject();
    }
}

void GraphicsSettingsDialog::accept()
{
    int result = QMessageBox::Yes;

    //Pop out the warning dialog for customized setting
    result = QMessageBox::question(this, "Warning", "A non-tested setting could potientially crash the game if the setting does not match the device. Are you sure you want to apply the customized setting?",
            QMessageBox::Yes, QMessageBox::No);

    //Save and exit
    if (result == QMessageBox::Yes)
    {
        SaveSystemSettings();
    }
}

void GraphicsSettingsDialog::OpenCustomSpecDialog()
{
    QString projectName = GetIEditor()->GetProjectName();
    QString settingsPath = projectName + "/" + SETTINGS_FILE_PATH;

    CAutoDirectoryRestoreFileDialog importCustomSpecDialog(QFileDialog::AcceptOpen, QFileDialog::ExistingFile, ".cfg", settingsPath, CFG_FILEFILTER, {}, {}, this);

    if (importCustomSpecDialog.exec())
    {
        QString file = importCustomSpecDialog.selectedFiles().first();
        if (!file.isEmpty())
        {
            ApplyCustomSpec(file);
        }
    }
}

void GraphicsSettingsDialog::ApplyCustomSpec(const QString& customFilePath)
{
    if (customFilePath.isEmpty())
    {
        return;
    }

    QFile customFile(customFilePath);
    if (!customFile.exists())
    {
        QMessageBox::warning(this, "Warning", "Could not find custom spec file.",
            QMessageBox::Ok);
        return;
    }

    bool change = true;
    if (m_dirtyCVarCount > 0 && !SendUnsavedChangesWarning(false))
    {
        change = false;
    }

    if (change)
    {
        AZStd::string filename = customFilePath.toStdString().c_str();
        filename = filename.substr(filename.find_last_of('/') + 1);

        m_currentPlatform = CONFIG_INVALID_PLATFORM;

        bool alreadyLoaded = IsCustomSpecAlreadyLoaded(filename);
        if (m_cfgFiles[CONFIG_INVALID_PLATFORM].size() < m_numSpecLevels && !alreadyLoaded)
        {
            m_cfgFiles[CONFIG_INVALID_PLATFORM].push_back(filename);
        }
        else if (alreadyLoaded)
        {
            QMessageBox::warning(this, "Warning", "The selected custom spec file is already loaded. No changes have been made.",
                QMessageBox::Ok);
            return;
        }

        if (m_cfgFiles[CONFIG_INVALID_PLATFORM].size() == m_numSpecLevels)
        {
            m_ui->SelectCustomSpecButton->setEnabled(false);
        }

        ShowCategories(true);

        CleanUI();

        LoadPlatformConfigurations();
    }
}

void GraphicsSettingsDialog::UnloadCustomSpec(int specLevel)
{
    if (specLevel < 0 || specLevel >= m_cfgFiles[CONFIG_INVALID_PLATFORM].size())
    {
        return;
    }

    bool shouldUnload = m_dirtyCVarCount <= 0 || SendUnsavedChangesWarning(false);

    if (shouldUnload)
    {
        m_cfgFiles[CONFIG_INVALID_PLATFORM].erase(m_cfgFiles[CONFIG_INVALID_PLATFORM].begin() + specLevel);
        m_ui->SelectCustomSpecButton->setEnabled(true);

        CleanUI();

        if (!m_cfgFiles[CONFIG_INVALID_PLATFORM].empty())
        {
            LoadPlatformConfigurations();
        }
        else
        {
            ShowCategories(false);
        }
    }
}

bool GraphicsSettingsDialog::IsCustomSpecAlreadyLoaded(const AZStd::string& filename) const
{
    auto& customConfigFilenames = m_cfgFiles.at(CONFIG_INVALID_PLATFORM);
    const auto& findResult = AZStd::find(customConfigFilenames.begin(), customConfigFilenames.end(), filename.c_str());

    return findResult != customConfigFilenames.end();
}

void GraphicsSettingsDialog::SetCollapsedLayout(const QString& groupName, QPushButton* togglebutton, QGridLayout* layout)
{
    CollapseGroup* cgroup = new CollapseGroup();
    cgroup->m_groupName = groupName;
    cgroup->m_dropDownButton = togglebutton;
    cgroup->m_gridlayout = layout;
    cgroup->m_isCollapsed = false;
    cgroup->m_dropDownButton->setStyleSheet("QPushButton{ border: 0px solid transparent;}");
    connect(cgroup->m_dropDownButton, &QPushButton::clicked, this, [=]() { cgroup->ToggleCollpased(); });
    cgroup->m_dropDownButton->setIcon(cgroup->m_iconOpen);

    m_uiCollapseGroup.push_back(cgroup);
}

void GraphicsSettingsDialog::SetPlatformEntry(ESystemConfigPlatform platform)
{
    auto platformCheck = [&platform](AZStd::pair<AZStd::string, ESystemConfigPlatform>& stringConfigPair) { return stringConfigPair.second == platform; };
    auto platformStringsIterator = AZStd::find_if(m_platformStrings.begin(), m_platformStrings.end(), platformCheck);
    if (platformStringsIterator != m_platformStrings.end())
    {
        int platformIndex = m_ui->PlatformEntry->findText(platformStringsIterator->first.c_str());
        m_ui->PlatformEntry->setCurrentIndex(platformIndex);
    }
    else
    {
        AZ_Assert(false, "Platform not found in platform strings vector.");
    }
}

ESystemConfigPlatform GraphicsSettingsDialog::GetConfigPlatformFromName(const AZStd::string& platformName)
{
    ESystemConfigPlatform configPlatform = CONFIG_INVALID_PLATFORM;

    auto platformNameCheck = [&platformName](AZStd::pair<AZStd::string, ESystemConfigPlatform>& stringConfigPair) { return (stringConfigPair.first == platformName); };
    auto configPlatformIterator = AZStd::find_if(m_platformStrings.begin(), m_platformStrings.end(), platformNameCheck);
    if (configPlatformIterator != m_platformStrings.end())
    {
        configPlatform = configPlatformIterator->second;
    }
    else
    {
        AZ_Assert(false, "Platform name not found in platform strings vector.");
    }

    return configPlatform;
}

// Save the current UI options to system qsettings
void GraphicsSettingsDialog::SaveSystemSettings()
{
    AZStd::vector<QString> successFiles;
    AZStd::vector<QString> nochangeFiles;

    for (int cfgFileIndex = 0; cfgFileIndex < m_cfgFiles[m_currentPlatform].size(); ++cfgFileIndex)
    {
        const QString eq = " = ";
        const QString cvarGroupString1 = "\n------------------------\n-- ";
        const QString cvarGroupString2 = "\n------------------------\n";
        QString commandList = "";

        int sysSpecFull;
        if (AZStd::any_numeric_cast<int>(&m_cVarTracker["sys_spec_full"].fileVals[cfgFileIndex].editedValue, sysSpecFull))
        {
            commandList += "sys_spec_full" + eq + QString::number(sysSpecFull) + '\n';
        }

        QMap<QString, QString> cvarGroupStrings;

        // Set to true as soon as cvar is found which is unique from current cfg file and index assignment
        bool saveOut = false;

        // Adding any dirty cvars not equal to sys_spec_full or any cvar in miscellaneous to command list
        for (auto& it : m_cVarTracker)
        {
            if ((CheckCVarStatesForDiff(&it, cfgFileIndex, EDITED_ORIGINAL_COMPARE) || it.second.cvarGroup == "miscellaneous") && azstricmp(it.first.c_str(), "sys_spec_full"))
            {
                AZStd::string cvarGroup = it.second.cvarGroup;
                if (azstricmp(cvarGroup.c_str(), "miscellaneous") != 0)
                {
                    int fileIndex = cvarGroup.find_last_of('/') + 1;
                    cvarGroup = cvarGroup.substr(fileIndex, cvarGroup.size() - fileIndex - 4);
                }
                QString qcvarGroup = cvarGroup.c_str();
                if (cvarGroupStrings.find(qcvarGroup) == cvarGroupStrings.end())
                {
                    cvarGroupStrings[qcvarGroup] = cvarGroupString1 + qcvarGroup + cvarGroupString2;
                }

                if (it.second.type == CVAR_INT)
                {
                    int val;
                    if (AZStd::any_numeric_cast<int>(&it.second.fileVals[cfgFileIndex].editedValue, val))
                    {
                        cvarGroupStrings[qcvarGroup] += it.first.c_str() + eq + QString::number(val) + '\n';
                    }
                }
                else if (it.second.type == CVAR_FLOAT)
                {
                    float val;
                    if (AZStd::any_numeric_cast<float>(&it.second.fileVals[cfgFileIndex].editedValue, val))
                    {
                        cvarGroupStrings[qcvarGroup] += it.first.c_str() + eq + QString::number(val) + '\n';
                    }
                }
                else
                {
                    cvarGroupStrings[qcvarGroup] += it.first.c_str() + eq + AZStd::any_cast<AZStd::string>(it.second.fileVals[cfgFileIndex].editedValue).c_str() + '\n';
                }
            }

            if (!saveOut)
            {
                saveOut = CheckCVarStatesForDiff(&it, cfgFileIndex, EDITED_OVERWRITTEN_COMPARE);
            }
        }

        // Adding the project name to the path so that the file is created there if it doesn't already exist
        // as we don't want to modify the version in Engine/config.
        QString projectName = GetIEditor()->GetProjectName();
        QString settingsPath = projectName + "/" + SETTINGS_FILE_PATH;

        QString settingsFile = settingsPath + m_cfgFiles[m_currentPlatform][cfgFileIndex].c_str();

        // Check if current settings differ from existing cfg
        if (saveOut)
        {
            // Adding any dirty cvars not equal to sys_spec_full to command list
            for (auto& it : cvarGroupStrings)
            {
                commandList += it;
            }

            if (CFileUtil::OverwriteFile(settingsFile.toStdString().c_str()))
            {
                if (!CFileUtil::CreateDirectory(settingsPath.toStdString().c_str()))
                {
                    QMessageBox::warning(this, "Warning", "Could not create the directory for file \"" + settingsFile + "\". Failed to apply Graphics Setting.",
                        QMessageBox::Ok);
                    continue;
                }

                QByteArray data = commandList.toUtf8().data();

                QFile file(settingsFile);
                if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size())
                {
                    QMessageBox::warning(this, "Warning", "Could not write settings to file \"" + settingsFile + ". Failed to apply Graphics Setting.",
                        QMessageBox::Ok);
                    file.close();
                    continue;
                }

                successFiles.push_back(settingsFile);

                // Update platform cvars to reflect new values
                for (auto& it : m_cVarTracker)
                {
                    it.second.fileVals[cfgFileIndex].overwrittenValue = it.second.fileVals[cfgFileIndex].editedValue;
                }

                file.close();
            }
            else
            {
                QMessageBox::warning(this, "Warning", "Could not check out or make file writable: \"" + settingsFile + "\". Failed to apply Graphics Setting.",
                    QMessageBox::Ok);
            }
        }
        else
        {
            nochangeFiles.push_back(settingsFile);
        }
    }

    // Print list of files which had no changes made
    if (nochangeFiles.size() > 0)
    {
        QString message = "No changes have been made to the following files:\n";
        for (int i = 0; i < nochangeFiles.size(); ++i)
        {
            message += nochangeFiles[i] + "\n";
        }
        QMessageBox::information(this, "Log", message, QMessageBox::Ok);
    }

    // Print list of files which were successfully saved
    if (successFiles.size() > 0)
    {
        QString message = "Updated the graphics setting correctly for the following files:\n";
        for (int i = 0; i < successFiles.size(); ++i)
        {
            message += successFiles[i] + "\n";
        }
        QMessageBox::information(this, "Log", message, QMessageBox::Ok);
    }

    // if we saved all of the files that we needed to then disable the save button again
    if (nochangeFiles.size() + successFiles.size() == m_cfgFiles[m_currentPlatform].size())
    {
        m_dirtyCVarCount = 0;
        m_ui->ApplyButton->setEnabled(false);
    }
}

/////////////////////////////////////////////////////////////////////////
// Collapse Group
GraphicsSettingsDialog::CollapseGroup::CollapseGroup()
    : m_dropDownButton(nullptr)
    , m_gridlayout(nullptr)
    , m_isCollapsed(false)
{
    m_iconOpen = QIcon(":/PropertyEditor/Resources/group_open.png");
    m_iconClosed = QIcon(":/PropertyEditor/Resources/group_closed.png");
}

void GraphicsSettingsDialog::CollapseGroup::ToggleButton(QPushButton* button, QGridLayout* layout)
{
    if (button && m_gridlayout)
    {
        button->setIcon(m_isCollapsed ? m_iconClosed : m_iconOpen);
        for (int row = 1; row < layout->rowCount(); ++row)
        {
            for (int col = 0; col < layout->columnCount(); ++col)
            {
                QLayoutItem* item = layout->itemAtPosition(row, col);
                if (item)
                {
                    QWidget* itemWidget = item->widget();
                    if (itemWidget)
                    {
                        itemWidget->setVisible(!m_isCollapsed);
                    }
                }
            }
        }
    }
}

void GraphicsSettingsDialog::CollapseGroup::ToggleCollpased()
{
    m_isCollapsed = !m_isCollapsed;
    ToggleButton(m_dropDownButton, m_gridlayout);
}

///////////////////////////////////////////////////////////////////
// ParameterWidget
GraphicsSettingsDialog::ParameterWidget::ParameterWidget(QWidget* widget, QString parameterName)
    : m_widget(widget)
    , m_parameterName(parameterName)
{
    m_widget->setToolTip(GetToolTip());
}

QString GraphicsSettingsDialog::ParameterWidget::GetToolTip() const
{
    if (!m_parameterName.isEmpty())
    {
        QString tooltipText = QString(PARAMETER_TOOLTIP).arg(m_parameterName);
        return tooltipText;
    }
    return "";
}

#include <GraphicsSettingsDialog.moc>
