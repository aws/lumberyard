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
#include "MobileSettingsDialog.h"

//QT
#include <QVector>
#include <QMenuBar>
#include <QAbstractListModel>
#include <QMessageBox>
#include <QComboBox>
#include <QFile>
#include <QtXml/QDomDocument>
#include <QtXml/qdom.h>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSpinBox>
#include <qdebug.h>
#include <QFontMetrics>

//Editor
#include <Util/AutoDirectoryRestoreFileDialog.h>
#include "IEditor.h"
#include "ILogFile.h"
#include "XConsole.h"
#include "IAssetTagging.h"
#include "ISourceControl.h"
#include <Util/FileUtil.h>

#include "ui_MobileSettingsDialog.h"

namespace MobileSettingsConst
{
    const int MOBILESETTING_FILE_VERSION = 0;
    //Files
    const char* IPHONE7_PRESETFILE = "Editor/Presets/MobileSettings/iphone7.lms";
    const char* PRESETFILE_DIR = "Editor/Presets/MobileSettings/";
    const char* GPU_DRIVER_LISTS_FILE = "Editor/Presets/MobileSettings/GPUAndDriverLists.json";
    const char* FEATURE_BLACK_LIST = "Editor/Presets/MobileSettings/MobileFeatureBlackList.json";
    const char* ANDROID_CFG_FILE = "Config/android.cfg";
    const char* IOS_CFG_FILE = "Config/ios.cfg";
    const char* ANDROID_DEFAULT_CFG_FILE = "Engine/Config/android.cfg";
    const char* IOS_DEFAULT_CFG_FILE = "Engine/Config/ios.cfg";
    // File extension
    const char* SZFILTER = "Lumberyard Mobile Setting (*.lms);;All files (*)";
    const char* JSON_FILEFILTER = "Json File(*.json);;All files(*)";
    const char* DEFAULT_FILENAME = "LumberyardMobileSettings.lms";
    const char* FILE_EXTENSION = "lms";

    //Identities
    const char* CUSTOMIZE_DEVICE_NAME = "Customize";
    const char* QSETTINGS_GROUP_MOBILESETTINGS = "MobileSettings";
    const char* DEVICE_WIDGET_OBJECTNAME = "MobileDevice";

    //XML/JSON NODE STRINGS
    const char* SETTING_XML_NODE = "MobileSetting";
    const char* SETTING_VERSION_NODE = "SettingVersion";
    const char* SETTING_PROPERTY_NODE = "Property";
    const char* JSON_GPU = "GPU";
    const char* JSON_DRIVER = "DRIVER";

    //Setting file version
    const int SETTING_VERSION = 0;
    const char* SETTING_FILE_EXTENSION = "lms";

    //UI HELP PARAMETERS
    const int EDITABLE_PAGE = 0;
    const int UNEDITABLE_PAGE = 1;

    //Operating System String
    const char* OS_ANDROID = "Android";
    const char* OS_IOS = "iOS";

    // Tool tips
    const char* PARAMETER_TOOLTIP = "The variable will update render parameter \"%1\".";
}

using namespace MobileSettingsConst;

MobileSettingsDialog::MobileSettingsDialog(QWidget* parent /* = nullptr */)
    : QDialog(parent)
    , m_ui(new Ui::MobileSettingsDialog())
    , m_customDeviceId(0)
    , m_currentpage(EDITABLE_PAGE)
    , m_driverComboBox(nullptr)
    , m_deviceComboBox(nullptr)
    , m_OSComboBox(nullptr)
    , m_OSLabel(nullptr)
    , m_blackListFile(nullptr)
    , m_blackListFile_uneditable(nullptr)
{
    //Update qlabel color when disabled
    setStyleSheet("QLabel::disabled{color: gray;}");

    // Start initialization the dialog
    m_isLoading = true;
    m_ui->setupUi(this);
    setWindowTitle("Mobile Settings");

    /////////////////////////////////////////////
    //initialize members
    m_currentpage = EDITABLE_PAGE;
    m_customDeviceId = 0;

    /////////////////////////////////////////////
    BuildUI();
    // Load settings must be called after UI built.
    LoadSystemSettings(); 

    // Finish initialization the dialog
    m_isLoading = false;
}

MobileSettingsDialog::~MobileSettingsDialog()
{
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

//Apply current system settings. Read the settings from QSettings("Amazon","Lumberyard")
void MobileSettingsDialog::ApplyMobileSettings()
{
    QString command = "";
    bool isEditable = (m_currentpage == EDITABLE_PAGE);
    for (ParameterWidget* w : m_parameterWidgets)
    {
        command += w->GetCommand(isEditable);
    }
    QString OSEditable = "";
    QString OSUneditable = "";
    
    if (m_OSComboBox)
    {
        OSEditable = m_OSComboBox->currentText();
    }
    if (m_OSLabel)
    {
        OSUneditable = m_OSLabel->text();
    }
    QString currentOS = isEditable ? OSEditable : OSUneditable;
    QString defaultFilePath = ANDROID_DEFAULT_CFG_FILE;
    QString settingFilePath = ANDROID_CFG_FILE;
    QString projectName = GetIEditor()->GetAssetTagging()->GetProjectName();
    if (currentOS == OS_IOS)
    {
        defaultFilePath = IOS_DEFAULT_CFG_FILE;
        settingFilePath = IOS_CFG_FILE;
    }

    settingFilePath = projectName + "/" + settingFilePath;
    // We use QFile static function to check if the setting file exists instead of 
    // CFileUtil, because the CFileUtil will check the files in project cache as well.
    // We don't want to add the file in project cache to source control
    bool isFileExist = QFile::exists(settingFilePath);
    if (isFileExist)
    {
        if (!CheckOutFile(settingFilePath))
        {
            QMessageBox::warning(this, "Warning", "Could not check out the file \"" + settingFilePath + "\". Fail to apply Mobile Setting.",
                QMessageBox::Ok);
            return;
        }
    }
    else
    {
        if (!AddFile(settingFilePath))
        {
            QMessageBox::warning(this, "Warning", "Could not add the file \"" + settingFilePath + "\" to source control. "
                "Fail to apply Mobile Setting.",
                QMessageBox::Ok);
            return;
        }
    }
    
    // Write mobile settings to file
    QFile defaultSettings(defaultFilePath);
    if (!defaultSettings.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(this, "Warning", "Could not open the file \"" + defaultFilePath + ". Fail to apply Mobile Setting.",
            QMessageBox::Ok);
        return;
    }

    QByteArray defaultData = defaultSettings.readAll();
    defaultSettings.close();
    defaultData.append(command.toUtf8().data());
    QFile file(settingFilePath);
    if (!file.open(QIODevice::WriteOnly) || file.write(defaultData) != defaultData.size())
    {
        QMessageBox::warning(this, "Warning", "Could not write settings to file \"" + settingFilePath + ". Fail to apply Mobile Setting.",
            QMessageBox::Ok);
        file.close();
        return;
    }

    // Apply settings from console
    gEnv->pConsole->ExecuteString(command.toUtf8().data());

    QMessageBox::information(this, "Log", "Update the mobile setting correctly. The settings are written into \"" + settingFilePath + "\".",
        QMessageBox::Ok);

    file.close();
    return;
}


/* About how to Edit the mobile setting dialog UI:
 * A. Relative files: 
 *    1.MobileSettingsDialog.ui
 *    2.MobileSettingsDialog.cpp
 * 
 * B. Steps to add a new parameter
 *    1. Edit UI file. 
 *    Edit the MobielSettingsDialog.ui file to add parameter widget. Please note
 * developer would need to add two group of widget, one is for editable widget which
 * allow user to modified the widget. The other is for uneditable widget which is used 
 * to show the value for presets. Set the objectname for the two widgets.
 *    2. Push the value into widgets
 *    In MobileSettingsDialog::BuidUI() function, create a ParameterWidget for the new 
 *  widgets and add the new widgets to m_parameterWidgets.
 *  ParameterWidget(editable Widget just created in UI file, uneditable widget, editable wdiget type, uneditable widgettype)
 *  Exmaple for adding the ParameterWidget: 
 *  m_parameterWidgets.push_back(new ParameterWidget(m_ui->ShadowsBlendCascades, 
 *  m_ui->ShadowsBlendCascades_uneditable, ParameterWidget::WidgetType::QSPINBOX, 
 *  ParameterWidget::WidgetType::QLABEL, "e_ShadowsBlendCascades"));
 *    3. Update MobileSettingsDialog::SerializeSetting function according to the changes. 
 *    Also developer would need to increaes the SETTING_VERSION
 *
 * C. Steps to add a parameter group
 *  1. Add a new GridLayout to hold the widgets for the new group. At column(0,0) for the
 * GridLayout, add a 16x16 pushbutton as the collapse and uncollapse button. At colume (0,1)
 * add a label contains the name of the group. Add new parameter for the group in the 
 * Gridlayout(please refer to B.Steps to add a new parameter) Add the layout for both editable page
 * and uneditable page. 
 *  2. In function MobileSettingsDialog::BuildUI(), add the collpase layout as following 
 *  SetCollapsedLayout(editable dropdown button, editable layout, uneditable dropdown button, 
 *                uneditable layout);
 *  Example:
 *    SetCollapsedLayout(m_ui->MiscMemory_editable_dropdown, m_ui->MiscMemoryBuffersLayout, 
 *    m_ui->MiscMemory_uneditable_dropdown, m_ui->MiscMemoryBuffersLayout_uneditable);
 *
 */

 //Build UI, link signals and set the data for device list.
void MobileSettingsDialog::BuildUI()
{
    // Build Data must be called before BuildUI. 
    BuildData();
    
    QComboBox* mobileDevice = m_ui->MobileDevice;
    QComboBox* mobileDeviceUnEditable = m_ui->MobileDevice_uneditable;
    QMap<QString, int>::iterator iter;

    mobileDevice->setInsertPolicy(QComboBox::InsertAtBottom);
    mobileDeviceUnEditable->setInsertPolicy(QComboBox::InsertAtBottom);

    //Add data for device combo box
    
    // First added the Custom device, we always want to add the custom device as the first option.
    for (iter = m_deviceStringList.begin(); iter != m_deviceStringList.end(); ++iter)
    {
        if (iter.value() == m_customDeviceId)
        {
            mobileDevice->addItem(QString(iter.key()), QVariant(iter.value()));
            mobileDeviceUnEditable->addItem(QString(iter.key()), QVariant(iter.value()));
        }
        // For Customized settings, we do not have preset settings to load
    }
    
    //Then loop and add all non-custom device settings
    for (iter = m_deviceStringList.begin(); iter != m_deviceStringList.end(); ++iter)
    {
        if (iter.value() == m_customDeviceId)
        {
            continue;
        }

        mobileDevice->addItem(QString(iter.key()), QVariant(iter.value()));
        int index = mobileDevice->findText(QString(iter.key()));
        // For Customized settings, we do not have preset settings to load. However 
        // The device could not be customized device setting
        XmlNodeRef settingsnode = *m_mobileSettings.find(iter.value());
        //Set tooltips which is the feature list
        mobileDevice->setItemData(index, GetFeatureList(settingsnode), Qt::ToolTipRole);

        mobileDeviceUnEditable->addItem(QString(iter.key()), QVariant(iter.value()));
        index = mobileDeviceUnEditable->findText(QString(iter.key()));
        mobileDeviceUnEditable->setItemData(index, GetFeatureList(settingsnode), Qt::ToolTipRole);
    }

    // Add data for GPU combo box
    m_ui->DeviceGPU->setInsertPolicy(QComboBox::InsertAtBottom);
    QStringList gpuList = m_gpuAndDriverStringMap.keys();
    for (int index = 0; index < gpuList.size(); index++)
    {
        m_ui->DeviceGPU->addItem(gpuList[index]);
    }

    //Collpase Buttons
    SetCollapsedLayout(m_ui->MiscMemory_editable_dropdown, m_ui->MiscMemoryBuffersLayout, m_ui->MiscMemory_uneditable_dropdown, m_ui->MiscMemoryBuffersLayout_uneditable);
    SetCollapsedLayout(m_ui->Shadows_editable_dropdown, m_ui->ShadowsLayout, m_ui->Shadows_uneditable_dropdown, m_ui->ShadowsLayout_uneditable);
    SetCollapsedLayout(m_ui->Animation_editable_dropdown, m_ui->AnimationLayout, m_ui->Animation_uneditable_dropdown, m_ui->AnimationLayout_uneditable);
    SetCollapsedLayout(m_ui->ObjectDetail_editable_dropdown, m_ui->ObjectDetailLayout, m_ui->ObjectDetail_uneditable_dropdown, m_ui->ObjectDetailLayout_uneditable);
    SetCollapsedLayout(m_ui->Particles_editable_dropdown, m_ui->ParticlesLayout, m_ui->Particles_uneditable_dropdown, m_ui->ParticlesLayout_uneditable);
    SetCollapsedLayout(m_ui->Physics_editable_dropdown, m_ui->PhysicsLayout, m_ui->Physics_uneditable_dropdown, m_ui->PhysicsLayout_uneditable);
    SetCollapsedLayout(m_ui->PostProcessing_editable_dropdown, m_ui->PostProcessingLayout, m_ui->Postprocessing_uneditable_dropdown, m_ui->PostProcessingLayout_uneditable);
    SetCollapsedLayout(m_ui->Shading_editable_dropdown, m_ui->ShadingLayout, m_ui->Shading_uneditable_dropdown, m_ui->ShadingLayout_uneditable);
    SetCollapsedLayout(m_ui->Texture_editable_dropdown, m_ui->TextureLayout, m_ui->Texture_uneditable_dropdown, m_ui->TextureLayout_uneditable);
    SetCollapsedLayout(m_ui->Water_editable_dropdown, m_ui->WaterLayout, m_ui->Water_uneditable_dropdown, m_ui->WaterLayout_uneditable);
    SetCollapsedLayout(m_ui->Decals_editable_dropdown, m_ui->DecalsLayout, m_ui->Decals_uneditable_dropdown, m_ui->DecalsLayout_uneditable);

    connect(m_ui->CancelButton, &QPushButton::clicked, this, &MobileSettingsDialog::reject);
    connect(m_ui->ApplyButton, &QPushButton::clicked, this, &MobileSettingsDialog::accept);

    connect(m_ui->ExportButton, &QPushButton::clicked, this, &MobileSettingsDialog::ExportSettings);
    connect(m_ui->ImportButton, &QPushButton::clicked, this, &MobileSettingsDialog::ImportSettings);

    connect(mobileDevice, &QComboBox::currentTextChanged, this, &MobileSettingsDialog::DeviceChanged);
    connect(mobileDeviceUnEditable, &QComboBox::currentTextChanged, this, &MobileSettingsDialog::DeviceChanged);

    connect(m_ui->DeviceGPU, &QComboBox::currentTextChanged, this, &MobileSettingsDialog::UpdateDriverComboBox);

    connect(m_ui->SelectBlackListFileButton, &QPushButton::clicked, this, &MobileSettingsDialog::OpenBlackListFileDialog);
    connect(m_ui->ApplyBlackListButton, &QPushButton::clicked, this, [=](){ApplyBlackList(m_ui->BlackListFile->text()); });

    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    //Grab widgets from m_ui
    m_parameterWidgets.clear();
    
    // GPU widget must insert before driver widget. Since driver widget will update based on the value of GPU widget.
    // We would like to make sure GPU value always set before we set driver value.
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->DeviceGPU, m_ui->DeviceGPU_uneditable, ParameterWidget::WidgetType::QCOMBOBOX, ParameterWidget::WidgetType::QLABEL));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->DriverVersion, m_ui->DriverVersion_uneditable, ParameterWidget::WidgetType::QCOMBOBOX, ParameterWidget::WidgetType::QLABEL));
    m_driverComboBox = m_ui->DriverVersion;
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->GMEMSupport, m_ui->GMEMSupport_uneditable, ParameterWidget::WidgetType::QCHECKBOX, ParameterWidget::WidgetType::QCHECKBOX, "r_EnableGMEMPath"));
    m_ui->MobileDevice->setObjectName(DEVICE_WIDGET_OBJECTNAME);
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->MobileDevice, m_ui->MobileDevice_uneditable, ParameterWidget::WidgetType::QCOMBOBOX));
    m_deviceComboBox = m_ui->MobileDevice;
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->OperatingSystem, m_ui->OperatingSystem_uneditable, ParameterWidget::WidgetType::QCOMBOBOX, ParameterWidget::WidgetType::QLABEL));
    m_OSComboBox = m_ui->OperatingSystem;
    m_OSLabel = m_ui->OperatingSystem_uneditable;
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->ZPassSupport, m_ui->ZPassSupport_uneditable, ParameterWidget::WidgetType::QCOMBOBOX, ParameterWidget::WidgetType::QLABEL, "r_UseZPass"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->BlackListFile, m_ui->BlackListFile_uneditable, ParameterWidget::WidgetType::QLINEEDIT, ParameterWidget::WidgetType::QLABEL));
    m_blackListFile = m_ui->BlackListFile;
    m_blackListFile_uneditable = m_ui->BlackListFile_uneditable;
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->ResolutionHeight, m_ui->ResolutionHeight_uneditable, ParameterWidget::WidgetType::QSPINBOX, ParameterWidget::WidgetType::QLABEL));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->ResolutionWidth, m_ui->ResolutionWidth_uneditable, ParameterWidget::WidgetType::QSPINBOX, ParameterWidget::WidgetType::QLABEL));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->DrawNearShadows, m_ui->DrawNearShadows_uneditable, ParameterWidget::WidgetType::QCHECKBOX, ParameterWidget::WidgetType::QCHECKBOX, "r_DrawNearShadows"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->ParticleShadows, m_ui->ParticleShadows_uneditable, ParameterWidget::WidgetType::QCOMBOBOX, ParameterWidget::WidgetType::QLABEL, "e_ParticlesShadows"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->ShadowsBlendCascades, m_ui->ShadowsBlendCascades_uneditable, ParameterWidget::WidgetType::QSPINBOX, ParameterWidget::WidgetType::QLABEL, "e_ShadowsBlendCascades"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->ShadowsCacheFormat, m_ui->ShadowsCacheFormat_uneditable, ParameterWidget::WidgetType::QCOMBOBOX, ParameterWidget::WidgetType::QLABEL, "r_ShadowsCacheFormat"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->ShadowsCloudSupport, m_ui->ShadowsCloudSupport_uneditable, ParameterWidget::WidgetType::QCHECKBOX, ParameterWidget::WidgetType::QCHECKBOX, "e_ShadowsClouds"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->ShadowsMaxTexutreResolution, m_ui->MaxTextureResolution_uneditable, ParameterWidget::WidgetType::QSPINBOX, ParameterWidget::WidgetType::QLABEL, "e_ShadowsMaxTexRes"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->ShadowsPoolSize, m_ui->ShadowsPoolSize_uneditable, ParameterWidget::WidgetType::QSPINBOX, ParameterWidget::WidgetType::QLABEL, "e_ShadowsPoolSize"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->ShadowsStaticMap, m_ui->ShadowsStaticMap_uneditable, ParameterWidget::WidgetType::QCHECKBOX, ParameterWidget::WidgetType::QCHECKBOX, "r_ShadowsStaticMap"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->ShadowsStaticMapResolution, m_ui->ShadowsStaticMapResolution_uneditable, ParameterWidget::WidgetType::QSPINBOX, ParameterWidget::WidgetType::QLABEL, "r_ShadowsStaticMapResolution"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->ShadowsSupport, m_ui->ShadowsSupport_uneditable, ParameterWidget::WidgetType::QCHECKBOX, ParameterWidget::WidgetType::NONE, "e_Shadows"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->AutoPrecacheCGF, m_ui->AutoPrecacheCGF_uneditable, ParameterWidget::WidgetType::QCOMBOBOX, ParameterWidget::WidgetType::QLABEL, "e_AutoPrecacheCgf"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->GeomCacheBuffer, m_ui->GeomCacheBuffer_uneditable, ParameterWidget::WidgetType::QSPINBOX, ParameterWidget::WidgetType::QLABEL, "e_GeomCacheBufferSize"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->MemoryPoolSoundPrimary, m_ui->MemoryPoolSoundPrimary_uneditable, ParameterWidget::WidgetType::QSPINBOX, ParameterWidget::WidgetType::QLABEL, "s_MemoryPoolSoundPrimary"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->OcclusionOutputQueueSize, m_ui->OcclusionOutputQueueSize_uneditable, ParameterWidget::WidgetType::QSPINBOX, ParameterWidget::WidgetType::QLABEL, "e_CheckOcclusionOutputQueueSize"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->OcclusionQueueSize, m_ui->OcclusionQueueSize_uneditable, ParameterWidget::WidgetType::QSPINBOX, ParameterWidget::WidgetType::QLABEL, "e_CheckOcclusionQueueSize"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->PrecacheTerrainAndProcVeget, m_ui->PrecacheTerrainAndProcVeget_uneditable, ParameterWidget::WidgetType::QCHECKBOX, ParameterWidget::WidgetType::NONE, "e_AutoPrecacheTerrainAndProcVeget"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->PrecacheTexturesAndShaders, m_ui->PrecacheTexturesAndShaders_uneditable, ParameterWidget::WidgetType::QCHECKBOX, ParameterWidget::WidgetType::NONE, "e_AutoPrecacheTexturesAndShaders"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->MemoryDefragPoolSize, m_ui->MemoryDefragPoolSize_uneditable, ParameterWidget::WidgetType::QSPINBOX, ParameterWidget::WidgetType::QLABEL, "ca_MemoryDefragPoolSize"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->StreamCHR, m_ui->StreamCHR_uneditable, ParameterWidget::WidgetType::QCHECKBOX, ParameterWidget::WidgetType::NONE, "ca_StreamCHR"));

    m_parameterWidgets.push_back(new ParameterWidget(m_ui->Dissolve, m_ui->Dissolve_uneditable, ParameterWidget::WidgetType::QCHECKBOX, ParameterWidget::WidgetType::NONE, "e_Dissolve"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->ObjectLODRatio, m_ui->ObjectLODRatio_uneditable, ParameterWidget::WidgetType::QSPINBOX, ParameterWidget::WidgetType::QLABEL, "e_LodRatio"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->ObjectQuality, m_ui->ObjectQuality_uneditable, ParameterWidget::WidgetType::QCOMBOBOX, ParameterWidget::WidgetType::QLABEL, "e_ObjQuality"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->ProcessVegatation, m_ui->ProcessVegatation_uneditable, ParameterWidget::WidgetType::QCOMBOBOX, ParameterWidget::WidgetType::QLABEL, "e_ProcVegetation"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->Tessellation, m_ui->Tessellation_uneditable, ParameterWidget::WidgetType::QCHECKBOX, ParameterWidget::WidgetType::NONE, "e_Tessellation"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->VegetationMinSize, m_ui->VegetationMinSize_uneditable, ParameterWidget::WidgetType::QDOUBLESPINBOX, ParameterWidget::WidgetType::QLABEL, "e_VegetationMinSize"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->ViewDistanceRatio, m_ui->ViewDistanceRatio_uneditable, ParameterWidget::WidgetType::QSPINBOX, ParameterWidget::WidgetType::QLABEL, "e_ViewDistRatio"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->ViewDistanceRatioDetails, m_ui->ViewDistanceRatioDetails_uneditable, ParameterWidget::WidgetType::QSPINBOX, ParameterWidget::WidgetType::QLABEL, "e_ViewDistRatioDetail"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->ViewDistanceRatioLights, m_ui->ViewDistanceRatioLights_uneditable, ParameterWidget::WidgetType::QSPINBOX, ParameterWidget::WidgetType::QLABEL, "e_ViewDistRatioLights"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->ViewDistanceRatioVegetation, m_ui->ViewDistanceRatioVegetation_uneditable, ParameterWidget::WidgetType::QSPINBOX, ParameterWidget::WidgetType::QLABEL, "e_ViewDistRatioVegetation"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->InstanceVertex, m_ui->InstanceVertex_uneditable, ParameterWidget::WidgetType::QCHECKBOX, ParameterWidget::WidgetType::NONE, "r_ParticlesInstanceVertices"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->MinParticlesDrawSize, m_ui->MinParticlesDrawSize_uneditable, ParameterWidget::WidgetType::QSPINBOX, ParameterWidget::WidgetType::QLABEL, "e_ParticlesMinDrawPixels"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->silhouettePOM, m_ui->silhouettePOM_uneditable, ParameterWidget::WidgetType::QCHECKBOX, ParameterWidget::WidgetType::QCHECKBOX, "e_ParticlesMinDrawPixels"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->Particles, m_ui->Particles_uneditable, ParameterWidget::WidgetType::QCHECKBOX, ParameterWidget::WidgetType::QCHECKBOX, "e_Particles"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->ParticlesGI, m_ui->ParticlesGI_uneditable, ParameterWidget::WidgetType::QCHECKBOX, ParameterWidget::WidgetType::QCHECKBOX, "e_ParticlesGI"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->ParticlesMaxScreenFill, m_ui->ParticlesMaxScreenFill_uneditable, ParameterWidget::WidgetType::QSPINBOX, ParameterWidget::WidgetType::QLABEL, "e_ParticlesMaxScreenFill"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->ParticlesPoolSize, m_ui->ParticlesPoolSize_uneditable, ParameterWidget::WidgetType::QSPINBOX, ParameterWidget::WidgetType::QLABEL, "e_ParticlesPoolSize"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->ParticlesPreload, m_ui->ParticlesPreload_uneditable, ParameterWidget::WidgetType::QCHECKBOX, ParameterWidget::WidgetType::NONE, "e_ParticlesPreload"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->ParticlesQuality, m_ui->ParticlesQuality_uneditable, ParameterWidget::WidgetType::QCOMBOBOX, ParameterWidget::WidgetType::QLABEL, "e_ParticlesQuality"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->ParticlesSortQuality, m_ui->ParticlesSortQuality_uneditable, ParameterWidget::WidgetType::QCOMBOBOX, ParameterWidget::WidgetType::QLABEL, "e_ParticlesSortQuality"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->BreakabgeMemoryLimit, m_ui->BreakabgeMemoryLimit_uneditable, ParameterWidget::WidgetType::QSPINBOX, ParameterWidget::WidgetType::QLABEL, "g_breakage_mem_limit"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->BreakabgeParticlesLimit, m_ui->BreakabgeParticlesLimit_uneditable, ParameterWidget::WidgetType::QSPINBOX, ParameterWidget::WidgetType::QLABEL, "g_breakage_particles_limit"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->CutReuseDistance, m_ui->CutReuseDistance_uneditable, ParameterWidget::WidgetType::QSPINBOX, ParameterWidget::WidgetType::QLABEL, "g_tree_cut_reuse_dist"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->MaxEntityCells, m_ui->MaxEntityCells_uneditable, ParameterWidget::WidgetType::QSPINBOX, ParameterWidget::WidgetType::QLABEL, "p_max_entity_cells"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->MaxMicrocontactSolverIterations, m_ui->MaxMicrocontactSolverIterations_uneditable, ParameterWidget::WidgetType::QSPINBOX, ParameterWidget::WidgetType::QLABEL, "p_max_mc_iters"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->MaxPhysicsDistance, m_ui->MaxPhysicsDistance_uneditable, ParameterWidget::WidgetType::QDOUBLESPINBOX, ParameterWidget::WidgetType::QLABEL, "es_MaxPhysDist"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->MaxPhysicsDistanceInvisible, m_ui->MaxPhysicsDistanceInvisible_uneditable, ParameterWidget::WidgetType::QDOUBLESPINBOX, ParameterWidget::WidgetType::QLABEL, "es_MaxPhysDistInvisible"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->MaxSubsteps, m_ui->MaxSubsteps_uneditable, ParameterWidget::WidgetType::QSPINBOX, ParameterWidget::WidgetType::QLABEL, "p_max_substeps"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->MinPhysicsCellSize, m_ui->MinPhysicsCellSize_uneditable, ParameterWidget::WidgetType::QSPINBOX, ParameterWidget::WidgetType::QLABEL, "e_PhysMinCellSize"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->SecondaryBreaking, m_ui->SecondaryBreaking_uneditable, ParameterWidget::WidgetType::QCHECKBOX, ParameterWidget::WidgetType::QCHECKBOX, "g_no_secondary_breaking"));

    m_parameterWidgets.push_back(new ParameterWidget(m_ui->DepthOfField, m_ui->DepthOfField_uneditable, ParameterWidget::WidgetType::QCOMBOBOX, ParameterWidget::WidgetType::QLABEL, "r_DepthOfField"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->Flares, m_ui->Flares_uneditable, ParameterWidget::WidgetType::QCHECKBOX, ParameterWidget::WidgetType::QCHECKBOX, "r_Flares"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->HDRBloomQuality, m_ui->HDRBloomQuality_uneditable, ParameterWidget::WidgetType::QCOMBOBOX, ParameterWidget::WidgetType::QLABEL, "r_HDRBloomQuality"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->MotionBlur, m_ui->MotionBlur_uneditable, ParameterWidget::WidgetType::QCOMBOBOX, ParameterWidget::WidgetType::QLABEL, "r_MotionBlur"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->PostProcessEffects, m_ui->PostProcessEffects_uneditable, ParameterWidget::WidgetType::QCOMBOBOX, ParameterWidget::WidgetType::QLABEL, "r_PostProcessEffects"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->PostProcessFilters, m_ui->PostProcessFilters_uneditable, ParameterWidget::WidgetType::QCHECKBOX, ParameterWidget::WidgetType::QCHECKBOX, "r_PostProcessFilters"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->PostProcessGameFx, m_ui->PostProcessGameFx_uneditable, ParameterWidget::WidgetType::QCHECKBOX, ParameterWidget::WidgetType::QCHECKBOX, "r_PostProcessGameFx"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->Snow, m_ui->Snow_uneditable, ParameterWidget::WidgetType::QCOMBOBOX, ParameterWidget::WidgetType::QLABEL, "r_Snow"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->Rain, m_ui->Rain_uneditable, ParameterWidget::WidgetType::QCOMBOBOX, ParameterWidget::WidgetType::QLABEL, "r_Rain"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->SunShafts, m_ui->SunShafts_uneditable, ParameterWidget::WidgetType::QCHECKBOX, ParameterWidget::WidgetType::QCHECKBOX, "r_SunShafts"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->TransparentDOFFixup, m_ui->TransparentDOFFixup_uneditable, ParameterWidget::WidgetType::QCHECKBOX, ParameterWidget::WidgetType::QCHECKBOX, "r_TranspDepthFixup"));
    
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->AntialiasingMode, m_ui->AntialiasingMode_uneditable, ParameterWidget::WidgetType::QCOMBOBOX, ParameterWidget::WidgetType::QLABEL, "r_AntialiasingMode"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->CacheNearestCubePicking, m_ui->CacheNearestCubePicking_uneditable, ParameterWidget::WidgetType::QCHECKBOX, ParameterWidget::WidgetType::QCHECKBOX, "e_CacheNearestCubePicking"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->DeferredShadingDepthBoundsTest, m_ui->DeferredShadingDepthBoundsTest_uneditable, ParameterWidget::WidgetType::QCHECKBOX, ParameterWidget::WidgetType::QCHECKBOX, "r_DeferredShadingDepthBoundsTest"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->DetailDistance, m_ui->DetailDistance_uneditable, ParameterWidget::WidgetType::QDOUBLESPINBOX, ParameterWidget::WidgetType::QLABEL, "r_DetailDistance"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->DynamicLightsMaxEntityLights, m_ui->DynamicLightsMaxEntityLights_uneditable, ParameterWidget::WidgetType::QSPINBOX, ParameterWidget::WidgetType::QLABEL, "e_DynamicLightsMaxEntityLights"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->GIIteration, m_ui->GIIteration_uneditable, ParameterWidget::WidgetType::QSPINBOX, ParameterWidget::WidgetType::QLABEL, "e_GIIterations"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->GIMaxDistance, m_ui->GIMaxDistance_uneditable, ParameterWidget::WidgetType::QSPINBOX, ParameterWidget::WidgetType::QLABEL, "e_GIMaxDistance"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->GlobalIllumination, m_ui->GlobalIllumination_uneditable, ParameterWidget::WidgetType::QCHECKBOX, ParameterWidget::WidgetType::QCHECKBOX, "e_GI"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->GlossyScreenSpaceReflections, m_ui->GlossyScreenSpaceReflections_uneditable, ParameterWidget::WidgetType::QCHECKBOX, ParameterWidget::WidgetType::QCHECKBOX, "r_SSReflections"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->LightVolume, m_ui->LightVolume_uneditable, ParameterWidget::WidgetType::QCOMBOBOX, ParameterWidget::WidgetType::QLABEL, "e_LightVolumes"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->Refraction, m_ui->Refraction_uneditable, ParameterWidget::WidgetType::QCHECKBOX, ParameterWidget::WidgetType::QCHECKBOX, "r_Refraction"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->RefractionPartialResolves, m_ui->RefractionPartialResolves_uneditable, ParameterWidget::WidgetType::QCOMBOBOX, ParameterWidget::WidgetType::QLABEL, "r_RefractionPartialResolves"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->SSDO, m_ui->SSDO_uneditable, ParameterWidget::WidgetType::QCHECKBOX, ParameterWidget::WidgetType::QCHECKBOX, "r_ssdo"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->SSDOBandwidthOcclusion, m_ui->SSDOBandwidthOcclusion_uneditable, ParameterWidget::WidgetType::QCOMBOBOX, ParameterWidget::WidgetType::QLABEL, "r_ssdoHalfRes"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->SSDOColorBleeding, m_ui->SSDOColorBleeding_uneditable, ParameterWidget::WidgetType::QCHECKBOX, ParameterWidget::WidgetType::QCHECKBOX, "r_ssdoColorBleeding"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->TerrainAmbientOcclusion, m_ui->TerrainAmbientOcclusion_uneditable, ParameterWidget::WidgetType::QCHECKBOX, ParameterWidget::WidgetType::QCHECKBOX, "e_TerrainAo"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->TerrainNormalMap, m_ui->TerrainNormalMap_uneditable, ParameterWidget::WidgetType::QCHECKBOX, ParameterWidget::WidgetType::QCHECKBOX, "e_TerrainNormalMap"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->TiledShading, m_ui->TiledShading_uneditable, ParameterWidget::WidgetType::QCOMBOBOX, ParameterWidget::WidgetType::QLABEL, "r_DeferredShadingTiled"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->TiledShadingHairQuality, m_ui->TiledShadingHairQuality_uneditable, ParameterWidget::WidgetType::QCOMBOBOX, ParameterWidget::WidgetType::QLABEL, "r_DeferredShadingTiledHairQuality"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->VegetationUseTerrainColor, m_ui->VegetationUseTerrainColor_uneditable, ParameterWidget::WidgetType::QCHECKBOX, ParameterWidget::WidgetType::QCHECKBOX, "e_VegetationUseTerrainColor"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->VisAreaClipLightsPerPixel, m_ui->VisAreaClipLightsPerPixel_uneditable, ParameterWidget::WidgetType::QCHECKBOX, ParameterWidget::WidgetType::QCHECKBOX, "r_VisAreaClipLightsPerPixel"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->DeferredShadingSSS, m_ui->DeferredShadingSSS_uneditable, ParameterWidget::WidgetType::QCHECKBOX, ParameterWidget::WidgetType::QCHECKBOX, "r_DeferredShadingSSS"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->DynTexAtlasCloudsMaxSize, m_ui->DynTexAtlasCloudsMaxSize_uneditable, ParameterWidget::WidgetType::QSPINBOX, ParameterWidget::WidgetType::QLABEL, "r_DynTexAtlasCloudsMaxSize"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->DynTexAtlasSpritesMaxSize, m_ui->DynTexAtlasSpritesMaxSize_uneditable, ParameterWidget::WidgetType::QSPINBOX, ParameterWidget::WidgetType::QLABEL, "r_DynTexAtlasSpritesMaxSize"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->DynTexMaxSize, m_ui->DynTexMaxSize_uneditable, ParameterWidget::WidgetType::QSPINBOX, ParameterWidget::WidgetType::QLABEL, "r_DynTexMaxSize"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->EnvTexResolution, m_ui->EnvTexResolution_uneditable, ParameterWidget::WidgetType::QCOMBOBOX, ParameterWidget::WidgetType::QLABEL, "r_EnvTexResolution"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->ImposterRatio, m_ui->ImposterRatio_uneditable, ParameterWidget::WidgetType::QSPINBOX, ParameterWidget::WidgetType::QLABEL, "r_ImposterRatio"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->TerrainTextureStreamingPoolItemsNum, m_ui->TerrainTextureStreamingPoolItemsNum_uneditable, ParameterWidget::WidgetType::QSPINBOX, ParameterWidget::WidgetType::QLABEL, "e_TerrainTextureStreamingPoolItemsNum"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->TexAtlasSize, m_ui->TexAtlasSize_uneditable, ParameterWidget::WidgetType::QSPINBOX, ParameterWidget::WidgetType::QLABEL, "r_TexAtlasSize"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->TexturesStreamPoolSize, m_ui->TexturesStreamPoolSize_uneditable, ParameterWidget::WidgetType::QSPINBOX, ParameterWidget::WidgetType::QLABEL, "r_TexturesStreamPoolSize"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->TexturesStreaming, m_ui->TexturesStreaming_uneditable, ParameterWidget::WidgetType::QCHECKBOX, ParameterWidget::WidgetType::QCHECKBOX, "r_TexturesStreaming"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->TexturesStreamingSkipMips, m_ui->TexturesStreamingSkipMips_uneditable, ParameterWidget::WidgetType::QSPINBOX, ParameterWidget::WidgetType::QLABEL, "r_TexturesStreamingSkipMips"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->TexutreStreamingMipBias, m_ui->TextureStreamingMipBias_uneditable, ParameterWidget::WidgetType::QSPINBOX, ParameterWidget::WidgetType::QLABEL, "r_TexturesStreamingMipBias"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->WaterCaustices, m_ui->WaterCaustices_uneditable, ParameterWidget::WidgetType::QCHECKBOX, ParameterWidget::WidgetType::QCHECKBOX, "r_WaterCaustics"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->WaterOcean, m_ui->WaterOcean_uneditable, ParameterWidget::WidgetType::QCOMBOBOX, ParameterWidget::WidgetType::QLABEL, "e_WaterOcean"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->WaterOceanBottom, m_ui->WaterOceanBottom_uneditable, ParameterWidget::WidgetType::QCHECKBOX, ParameterWidget::WidgetType::QCHECKBOX, "e_WaterOceanBottom"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->WaterReflections, m_ui->WaterReflections_uneditable, ParameterWidget::WidgetType::QCHECKBOX, ParameterWidget::WidgetType::QCHECKBOX, "r_WaterReflections"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->WaterReflectionsQuality, m_ui->WaterReflectionsQuality_uneditable, ParameterWidget::WidgetType::QCOMBOBOX, ParameterWidget::WidgetType::QLABEL, "r_WaterReflectionsQuality"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->WaterUpdateDistance, m_ui->WaterUpdateDistance_uneditable, ParameterWidget::WidgetType::QDOUBLESPINBOX, ParameterWidget::WidgetType::QLABEL, "r_WaterUpdateDistance"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->WaterUpdateFactor, m_ui->WaterUpdateFactor_uneditable, ParameterWidget::WidgetType::QDOUBLESPINBOX, ParameterWidget::WidgetType::QLABEL, "r_WaterUpdateFactor"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->WaterUpdateThread, m_ui->WaterUpdateThread_uneditable, ParameterWidget::WidgetType::QSPINBOX, ParameterWidget::WidgetType::QLABEL, "r_WaterUpdateThread"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->WaterVolumes, m_ui->WaterVolumes_uneditable, ParameterWidget::WidgetType::QCOMBOBOX, ParameterWidget::WidgetType::QLABEL, "e_WaterVolumes"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->WaterVolumesCaustics, m_ui->WaterVolumesCaustics_uneditable, ParameterWidget::WidgetType::QCHECKBOX, ParameterWidget::WidgetType::QCHECKBOX, "r_WaterVolumeCaustics"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->Decals, m_ui->Decals_uneditable, ParameterWidget::WidgetType::QCHECKBOX, ParameterWidget::WidgetType::QCHECKBOX, "r_DeferredDecals"));
    m_parameterWidgets.push_back(new ParameterWidget(m_ui->DecalsForceDeferred, m_ui->DecalsForceDeferred_uneditable, ParameterWidget::WidgetType::QCHECKBOX, ParameterWidget::WidgetType::QCHECKBOX, "e_DecalsForceDeferred"));

    BuildBlackListMap();
}

/* BuildData function will do following tasks
*  1. Load all mobile setting files(all files with extension ".lms") as presets. 
*  The presets should be placed in the folder "Dev/Editor/Presets/MobileSettings/" 
*  a.The function will contribute a map with <presetId, setting data xmlNodeRef>, so once user switch to 
*  specific preset, the editor will load the settings from xmlNodeRef.
*  b.The function will also contribute a map of <setting name (shown on ui), presetId>. The map will used 
*  to build ui and find the setting preset.
*  2. Add options for GPU and drivers.
*  The list of GPU and drivers are in the file "Editor/Presets/MobileSettings/GPUAndDriverLists.json". 
*  Developers can edit the file to extend the GPU and driver lists. The list will be stored in a QString
*  Vector and assigned to the comboboxes once the ui is build. (refer to MobileSettingsDialog::BuildUI())
*  
*/
void MobileSettingsDialog::BuildData()
{
    m_deviceStringList.clear();

    int presetID = 0;

    QStringList nameFilter("*.lms"); //filter for the settings file type
    QDir directory(PRESETFILE_DIR);
    QStringList filesInPresetDirectory = directory.entryList(nameFilter);
    
    // Load files from preset folder as preset
    for (QString filename: filesInPresetDirectory)
    {
        if (Path::GetExt(filename.toStdString().c_str()) != SETTING_FILE_EXTENSION)
        {
            continue;
        }
        QString fullPath = PRESETFILE_DIR + filename;
        QString CfullPath = Path::GamePathToFullPath(fullPath);
        XmlNodeRef root = XmlHelpers::LoadXmlFromFile(CfullPath.toStdString().c_str());
        if (!root)
        {
            continue;
        }

        //Map device name with presetID. The filename will be used as Identity show on the UI.
        QString deviceName = GetAttribute(DEVICE_WIDGET_OBJECTNAME, root);
        if (deviceName.isEmpty())
        {
            QString text = "\nCould not find device name from setting file \""+CfullPath+"\". Please check the file format.\n";
            GetIEditor()->GetLogFile()->WriteString(text.toStdString().c_str());
            continue;
        }
        QString identity = deviceName;
        if (!m_deviceStringList.contains(identity))
        {
            m_deviceStringList.insert(identity, presetID);
            m_mobileSettings.insert(presetID, root);
            presetID++;
        }
        else
        {
            QString text = "\nPreset \"" + identity + "\" already exists. Please check the preset folder " + PRESETFILE_DIR + ".";
            GetIEditor()->GetLogFile()->WriteString(text.toStdString().c_str());
            continue;
        }
    }

    //Device String List always have one for Customized setting
    m_deviceStringList.insert(CUSTOMIZE_DEVICE_NAME, presetID);
    m_customDeviceId = presetID;
    presetID++;    

    //Build GPU data from json file
    QFile GpuAndDriverListsfile(GPU_DRIVER_LISTS_FILE);
    if (!GpuAndDriverListsfile.open(QIODevice::ReadOnly))
    {
        QString text = "\nGPU and driver list file \"" + QString(GPU_DRIVER_LISTS_FILE) + "\" could not open. Please check the file format.";
        GetIEditor()->GetLogFile()->WriteString(text.toStdString().c_str());
        return;
    }
    QByteArray jsonFileData = GpuAndDriverListsfile.readAll();
    GpuAndDriverListsfile.close();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonFileData);
    if (jsonDoc.isNull()) // Fail read jason File
    {
        QString text = "\nGPU and driver list file \"" + QString(GPU_DRIVER_LISTS_FILE) + "\" could not open. Please check the file format.";
        GetIEditor()->GetLogFile()->WriteString(text.toStdString().c_str());
        return;
    }
    QJsonObject jsonObj = jsonDoc.object();
    QStringList gpus = jsonObj.keys();

    for (int i = 0; i < gpus.size(); ++i)
    {
        QJsonArray gpuDriverArray = jsonObj[gpus[i]].toArray();
        QVector<QString> driverArray;
        for (int j = 0; j < gpuDriverArray.size(); ++j)
        {
            driverArray.push_back(gpuDriverArray[j].toString());
        }
        m_gpuAndDriverStringMap.insert(gpus[i], driverArray);
    }
}


///////////////////////////////////////////////////////////
//Function to update UIs
void MobileSettingsDialog::DeviceChanged(const QString& device)
{
    QString deviceName(device);
    int currentDevice = m_deviceStringList[deviceName];

    //Sync the device selection
    QSignalBlocker block(m_ui->MobileDevice_uneditable);
    m_ui->MobileDevice_uneditable->setCurrentText(device);
    QSignalBlocker block2(m_ui->MobileDevice);
    m_ui->MobileDevice->setCurrentText(device);

    if (currentDevice == m_customDeviceId)
    {
        SetPageIndex(EDITABLE_PAGE);
    }
    else
    {
        bool success = SerializeSetting(m_mobileSettings[currentDevice], true);
        if (!success)
        {
            QMessageBox::warning(this, "Warning", "Fail to load saved settings. Switch to Customized settings.",
                QMessageBox::Ok);
            SetPageIndex(EDITABLE_PAGE);
            m_deviceComboBox->setCurrentText(QString(m_deviceStringList.key(m_customDeviceId)));
            return;
        }

        // If user choose a preset device. Show Uneditbale widgets to prevent user from
        // editing the preset.
        SetPageIndex(UNEDITABLE_PAGE);
        
        // While it is still loading datas, do not show parameters changes
        // Switch to custom device will not change any other parameter 
        // except for the device combobox. No need to call ShowParameterChange 
        // in that case.
        if (!m_isLoading)
        {
            ShowParameterChange();
        }
    }
}

void MobileSettingsDialog::SetPageIndex(int page)
{
    m_ui->stackedWidget->setCurrentIndex(page);
    m_currentpage = page;
}

void MobileSettingsDialog::UpdateDriverComboBox(const QString & gpu)
{
    // Check if the QMap contains gpu first. If we directly access the 
    // value, the QMap will inserts a default-constructed value into the map
    // with the key.
    if (!m_gpuAndDriverStringMap.contains(gpu))
    {
        // Could not find key in the list
        return;
    }
    QVector<QString> driverList = m_gpuAndDriverStringMap[gpu];
    
    // Clear 
    m_driverComboBox->clear();

    // Add data for Driver combo box
    m_driverComboBox->setInsertPolicy(QComboBox::InsertAtBottom);
    for (int index = 0; index < driverList.size(); index++)
    {
        m_driverComboBox->addItem(driverList[index]);
    }
}

///////////////////////////////////////////////////////////////////////
// settings file management
void MobileSettingsDialog::reject()
{
    QDialog::reject();
}

void MobileSettingsDialog::accept()
{
    XmlNodeRef node;
    SerializeSetting(node, false);
    int result = QMessageBox::Yes;

    //Pop out the warnning dialog for customized setting
    if (m_currentpage == EDITABLE_PAGE)
    {
        result = QMessageBox::question(this, "Warning", "A non-tested setting could potientially crash the game if the setting does not match the device. Are you sure to apply the customized setting?",
            QMessageBox::Yes, QMessageBox::No);
    }

    //Save and exit
    if (result == QMessageBox::Yes)
    {
        SaveSystemSettings();
        ApplyMobileSettings();
    }    
}

// Save the current UI options to system qsettings
void MobileSettingsDialog::SaveSystemSettings()
{
    QSettings settings("Amazon", "Lumberyard");
    XmlNodeRef node;
    SerializeSetting(node, false);
    QString savingString = node->getXML();
    settings.setValue(QSETTINGS_GROUP_MOBILESETTINGS, savingString);
    settings.sync();
}

void MobileSettingsDialog::LoadSystemSettings()
{
    QSettings settings("Amazon", "Lumberyard");
    QString currentDeviceValue = QString(m_deviceStringList.key(m_customDeviceId));
    if (!settings.value(QSETTINGS_GROUP_MOBILESETTINGS).isNull())
    {
        QString buffer = settings.value(QSETTINGS_GROUP_MOBILESETTINGS).toString();
        XmlNodeRef node = XmlHelpers::LoadXmlFromBuffer(buffer.toStdString().c_str(), buffer.length());
        bool success = SerializeSetting(node, true);
        if (!success)
        {
            QMessageBox::warning(this, "Warning", QString("Fail to load saved system settings."),
                QMessageBox::Ok);
        }
        else if (m_deviceComboBox != nullptr)
        {
            currentDeviceValue = m_deviceComboBox->currentText();
        }
    }
    if (m_deviceComboBox)
    {
        // Use DeviceChanged to focus updating the device settings. Otherwise if the device already
        // set to custom device value, it will not update the UI
        DeviceChanged(currentDeviceValue);
    }
}

bool MobileSettingsDialog::SerializeSetting(XmlNodeRef& node, bool isLoad)
{
    if (isLoad) // Import
    {
        if (!node)
        {
            return false;
        }
        int version = -1;
        if (!node->haveAttr(SETTING_VERSION_NODE))
        {
            return false;
        }
        node->getAttr(SETTING_VERSION_NODE, version);
        switch (version) //Check version number
        {
        case MOBILESETTING_FILE_VERSION: // Current version
        {
            XmlNodeRef child = node->findChild(SETTING_PROPERTY_NODE);
            if (child == NULL)
            {
                return false;
            }
            for (ParameterWidget* w : m_parameterWidgets)
            {
                QString value;
                // Always use the object name of editable widget as the key 
                if (!child->haveAttr(w->m_widget->objectName().toStdString().c_str()))
                {
                    return false;
                }
                child->getAttr(w->m_widget->objectName().toStdString().c_str(), value);
                w->SetBothValue(QString(value));
                // Apply black list if the black list value is set
                if (w->m_widget == m_blackListFile)
                {
                    ApplyBlackList(QString(value));
                }
            }
            break;
        }
        default:
            AZ_Assert(false, "Unknown version of MobileSettings: %d", version);
            return false;
        }
        return true;
    }
    else //Export
    {
        node = XmlHelpers::CreateXmlNode(SETTING_XML_NODE);
        node->setAttr(SETTING_VERSION_NODE, SETTING_VERSION);
        XmlNodeRef child = XmlHelpers::CreateXmlNode(SETTING_PROPERTY_NODE);
        bool isEditable = (m_currentpage == EDITABLE_PAGE);
        for (ParameterWidget* w : m_parameterWidgets)
        {
            QString value = w->value(isEditable).toUtf8().data();
            // Always use the object name of editable widget as the key 
            child->setAttr(w->m_widget->objectName().toStdString().c_str(), value);
        }
        node->addChild(child);
        return true;
    }
}

bool MobileSettingsDialog::LoadSettings(const QString& filepath)
{
    XmlNodeRef root = XmlHelpers::LoadXmlFromFile(filepath.toStdString().c_str());
    if (!root)
    {
        return false;
    }
    return SerializeSetting(root, true);
}

bool MobileSettingsDialog::SaveSettings(const QString& filepath)
{
    XmlNodeRef node;
    SerializeSetting(node,false);
    bool success = node->saveToFile(filepath.toStdString().c_str());
    if (!success)
    {
        return false;
    }
    return true;
}

void MobileSettingsDialog::ExportSettings()
{
    CAutoDirectoryRestoreFileDialog exportFileSelectionDialog(QFileDialog::AcceptSave, QFileDialog::AnyFile, FILE_EXTENSION, DEFAULT_FILENAME, SZFILTER, {}, {}, this);
    if (exportFileSelectionDialog.exec())
    {
        QString file = QString(exportFileSelectionDialog.selectedFiles().first());
        if (!file.isEmpty())
        {
            if (!SaveSettings(file))
            {
                QMessageBox::warning(this, "Warning", QString("Fail to save file to \"%1\".").arg(QString(file)),
                    QMessageBox::Ok);
            }
        }
    }
}

void MobileSettingsDialog::ImportSettings()
{
    CAutoDirectoryRestoreFileDialog importFileSelectionDialog(QFileDialog::AcceptOpen, QFileDialog::ExistingFile, FILE_EXTENSION, {}, SZFILTER, {}, {}, this);

    if (importFileSelectionDialog.exec())
    {
        QString file = QString(importFileSelectionDialog.selectedFiles().first());
        if (!file.isEmpty())
        {
            if(!LoadSettings(file))
            {
                QMessageBox::warning(this, "Warning", QString("Fail to load file \"%1\".").arg(QString(file)),
                    QMessageBox::Ok);
            }
            else
            {
                // While it is still loading data, do not show parameters changes
                // Switch to custom device will not change any other parameter 
                // except for the device combobox. No need to call ShowParameterChange 
                // in that case.
                if (!m_isLoading)
                {
                    // Add parameter change notification for import settings.
                    ShowParameterChange();
                }
            }
            // User can only import data to "Customized" option.
            if (m_deviceComboBox)
            {
                m_deviceComboBox->setCurrentText(QString(m_deviceStringList.key(m_customDeviceId)));
            }

        }
    }
}

void MobileSettingsDialog::OpenBlackListFileDialog()
{
    CAutoDirectoryRestoreFileDialog importBlackListFileDialog(QFileDialog::AcceptOpen, QFileDialog::ExistingFile, ".json", {}, JSON_FILEFILTER, {}, {}, this);

    if (importBlackListFileDialog.exec())
    {
        QString file = importBlackListFileDialog.selectedFiles().first();
        if (!file.isEmpty())
        {
            m_blackListFile->setText(file);
        }
    }
}

// Disable the feature according to blacklist
void MobileSettingsDialog::ApplyBlackList(const QString& blackListFile)
{
    // On black list applied, reset widget status.
    ResetBlackList();
    if (blackListFile.isEmpty())
    {
        return;
    }

    QFile GpuAndDriverListsfile(blackListFile);
    if (!GpuAndDriverListsfile.exists())
    {
        QMessageBox::warning(this, "Warning", "Could not find black list file.",
            QMessageBox::Ok);
        return;
    }

    if (!GpuAndDriverListsfile.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(this, "Warning", "Could not open black list file.",
            QMessageBox::Ok);
        return;
    }
    QByteArray jsonFileData = GpuAndDriverListsfile.readAll();
    GpuAndDriverListsfile.close();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonFileData);
    if (jsonDoc.isNull()) // Fail read jason File
    {
        QMessageBox::warning(this, "Warning", "Could not read JSON file " + blackListFile + ". Please check if the JSON file is valid.",
            QMessageBox::Ok);
        return;
    }

    QJsonObject jsonObj = jsonDoc.object();
    QString currentDevice = m_ui->MobileDevice->currentText();
    if (jsonObj.contains(currentDevice))
    {
        QJsonArray blackList = jsonObj[currentDevice].toArray();
        for (int index = 0; index < blackList.size(); ++index)
        {
            QString featureName = blackList[index].toString();
            SetWidgetDisabled(featureName, true);
        }
    }
}

// Reset all black list settings
void MobileSettingsDialog::ResetBlackList()
{
    for (const QVector<QWidget*>& widgets : m_featureBlackListMap)
    {
        SetWidgetDisabled(widgets, false);
    }
}

void MobileSettingsDialog::SetWidgetDisabled(const QVector<QWidget*>& widgets, bool disabled)
{
    for (QWidget* w : widgets)
    {
        w->setDisabled(disabled);
    }
}

void MobileSettingsDialog::SetWidgetDisabled(const QString& featureName, bool disabled)
{
    QMap<QString, QVector<QWidget*>>::iterator i = m_featureBlackListMap.find(featureName);
    // Could not find featureName from the map
    if (i == m_featureBlackListMap.end())
    {
        return;
    }
    SetWidgetDisabled(i.value(), disabled);
}

void MobileSettingsDialog::SetCollapsedLayout(QPushButton* togglebutton_editable, QGridLayout* layout_editable, QPushButton* togglebutton_uneditable, QGridLayout* layout_uneditable)
{
    CollapseGroup* cgroup = new CollapseGroup();
    cgroup->m_dropDownButton_editable = togglebutton_editable;
    cgroup->m_gridlayout_editable = layout_editable;
    cgroup->m_isCollapsed = false;
    cgroup->m_dropDownButton_uneditable = togglebutton_uneditable;
    cgroup->m_gridlayout_uneditable = layout_uneditable;

    if (cgroup->m_dropDownButton_editable)
    {
        cgroup->m_dropDownButton_editable->setStyleSheet("QPushButton{ border: 0px solid transparent;}");
        connect(cgroup->m_dropDownButton_editable, &QPushButton::clicked, this, [=](){ cgroup->toggleCollpased(); });
        cgroup->m_dropDownButton_editable->setIcon(cgroup->m_iconOpen);
    }
    if (cgroup->m_dropDownButton_uneditable)
    {
        cgroup->m_dropDownButton_uneditable->setStyleSheet("QPushButton{ border: 0px solid transparent;}");
        connect(cgroup->m_dropDownButton_uneditable, &QPushButton::clicked, this, [=](){ cgroup->toggleCollpased(); });
        cgroup->m_dropDownButton_uneditable->setIcon(cgroup->m_iconOpen);
    }
    m_uiCollapseGroup.push_back(cgroup);
}


// Build the list of Black list widget.
// Once disable the feature in black list file, the widgets in the widget list will be disabled.
// Current feature lists:
/*
**        AmbientOcclusion
**        Decals
**        Screen Space Reflection
**        Water Reflection
**        MotionBlur
**        DepthOfField
**        VisArea
**        Rain
**        Snow
**        Cloud
**        ZPASS
**        GMEM
**        HDRRender
**        Water Ocean
**        Water Volumes
**
**
** Blacklist file example:
** Example.json:
**   {
**    "Customize" :[
**        Cloud,
**        GMEM
**    ]
**    "A Device Name" : [
**       Feature A You Would Like To Disable,
**       Feature B You Would Like To Disable,
**       ...
**    ]
**   }
** This file will disable feature for Cloud shader and GMEM
** 
**
** How to extend the features:
** m_featureBlackListMap is a map of <featureName, Vector of related parameter widgets>
** To add a new feature, create a pair and insert it to the map. Rember to add the featureName
** to the feature list above to track the features.
**
*/
void MobileSettingsDialog::BuildBlackListMap()
{
    m_featureBlackListMap.clear();

    // Ambient Occlusion
    QVector<QWidget*> AOWidgets;
    AOWidgets.push_back(m_ui->TerrainAmbientOcclusion);
    AOWidgets.push_back(m_ui->TerrainAmbientOcclusion_uneditable);
    AOWidgets.push_back(m_ui->TerrainAmbientOcclusionLabel);
    AOWidgets.push_back(m_ui->TerrainAmbientOcclusionLabel_uneditable);
    m_featureBlackListMap.insert("AmbientOcclusion", AOWidgets);

    // Decals
    QVector<QWidget*> DecalsWidgets;
    DecalsWidgets.push_back(m_ui->Decals);
    DecalsWidgets.push_back(m_ui->Decals_uneditable);
    DecalsWidgets.push_back(m_ui->DecalsLabel);
    DecalsWidgets.push_back(m_ui->DecalsLabel_uneditable);
    m_featureBlackListMap.insert("Decals", DecalsWidgets);

    // Screen space reflection
    QVector<QWidget*> SSRelectionWidgets;
    SSRelectionWidgets.push_back(m_ui->GlossyScreenSpaceReflections);
    SSRelectionWidgets.push_back(m_ui->GlossyScreenSpaceReflections_uneditable);
    SSRelectionWidgets.push_back(m_ui->GlossyScreenSpaceReflectionsLabel);
    SSRelectionWidgets.push_back(m_ui->GlossyScreenSpaceReflectionsLabel_uneditable);
    m_featureBlackListMap.insert("Screen Space Reflection", SSRelectionWidgets);
    

    // Water Reflection
    QVector<QWidget*> WaterReflectionWidgets;
    WaterReflectionWidgets.push_back(m_ui->WaterReflectionsLabel);
    WaterReflectionWidgets.push_back(m_ui->WaterReflectionsLabel_uneditable);
    m_featureBlackListMap.insert("Water Reflection", WaterReflectionWidgets);
    

    // Motion Blur
    QVector<QWidget*> BlurWidgets;
    BlurWidgets.push_back(m_ui->MotionBlur);
    BlurWidgets.push_back(m_ui->MotionBlur_uneditable);
    BlurWidgets.push_back(m_ui->MotionBlurLabel);
    BlurWidgets.push_back(m_ui->MotionBlurLabel_uneditable);
    m_featureBlackListMap.insert("MotionBlur", BlurWidgets);

    // Depth of Field
    QVector<QWidget*> DOFWidgets;
    DOFWidgets.push_back(m_ui->DepthOfField);
    DOFWidgets.push_back(m_ui->DepthOfField_uneditable);
    DOFWidgets.push_back(m_ui->DepthOfFieldLabel);
    DOFWidgets.push_back(m_ui->DepthOfFieldLabel_uneditable);
    m_featureBlackListMap.insert("DepthOfField", DOFWidgets);

    // VisArea
    QVector<QWidget*> VisAreaWidgets;
    VisAreaWidgets.push_back(m_ui->VisAreaClipLightsPerPixel);
    VisAreaWidgets.push_back(m_ui->VisAreaClipLightsPerPixel_uneditable);
    VisAreaWidgets.push_back(m_ui->VisAreaClipLightsPerPixelLabel);
    VisAreaWidgets.push_back(m_ui->VisAreaClipLightsPerPixelLabel_uneditable);
    m_featureBlackListMap.insert("VisArea", VisAreaWidgets);


    // Rain
    QVector<QWidget*> RainWidgets;
    RainWidgets.push_back(m_ui->Rain);
    RainWidgets.push_back(m_ui->Rain_uneditable);
    RainWidgets.push_back(m_ui->RainLabel);
    RainWidgets.push_back(m_ui->RainLabel_uneditable);
    m_featureBlackListMap.insert("Rain", RainWidgets);

    // Snow
    QVector<QWidget*> SnowWidgets;
    SnowWidgets.push_back(m_ui->Snow);
    SnowWidgets.push_back(m_ui->Snow_uneditable);
    SnowWidgets.push_back(m_ui->SnowLabel);
    SnowWidgets.push_back(m_ui->SnowLabel_uneditable);
    m_featureBlackListMap.insert("Snow", SnowWidgets);

    // Cloud
    QVector<QWidget*> CloudWidgets;
    CloudWidgets.push_back(m_ui->ShadowsCloudSupport);
    CloudWidgets.push_back(m_ui->ShadowsCloudSupport_uneditable);
    CloudWidgets.push_back(m_ui->ShadowsCloudSupportLabel);
    CloudWidgets.push_back(m_ui->ShadowsCloudSupportLabel_uneditable);
    m_featureBlackListMap.insert("Cloud", CloudWidgets);

    //ZPASS
    QVector<QWidget*> ZPASSWidgets;
    ZPASSWidgets.push_back(m_ui->ZPassSupport);
    ZPASSWidgets.push_back(m_ui->ZPassSupport_uneditable);
    ZPASSWidgets.push_back(m_ui->ZPassSupportLabel);
    ZPASSWidgets.push_back(m_ui->ZPassSupportLabel_uneditable);
    m_featureBlackListMap.insert("ZPASS", ZPASSWidgets);

    //GMEM
    QVector<QWidget*> GMEMWidgets;
    GMEMWidgets.push_back(m_ui->GMEMSupport);
    GMEMWidgets.push_back(m_ui->GMEMSupport_uneditable);
    GMEMWidgets.push_back(m_ui->GMEMSupportLabel);
    GMEMWidgets.push_back(m_ui->GMEMSupportLabel_uneditable);
    m_featureBlackListMap.insert("GMEM", GMEMWidgets);


    // HDRRender
    QVector<QWidget*> HDRWidgets;
    HDRWidgets.push_back(m_ui->HDRSupport);
    HDRWidgets.push_back(m_ui->HDRSupport_uneditable);
    HDRWidgets.push_back(m_ui->HDRSupportLabel);
    HDRWidgets.push_back(m_ui->HDRSupportLabel_uneditable);
    m_featureBlackListMap.insert("HDRRender", HDRWidgets);


    // Water Ocean
    QVector<QWidget*> OceanWidgets;
    OceanWidgets.push_back(m_ui->WaterOcean);
    OceanWidgets.push_back(m_ui->WaterOcean_uneditable);
    OceanWidgets.push_back(m_ui->WaterOceanLabel);
    OceanWidgets.push_back(m_ui->WaterOceanLabel_uneditable);
    m_featureBlackListMap.insert("Water Ocean", OceanWidgets);

    // Water Volumes
    QVector<QWidget*> WaterVolumesWidgets;
    WaterVolumesWidgets.push_back(m_ui->WaterVolumes);
    WaterVolumesWidgets.push_back(m_ui->WaterVolumes_uneditable);
    WaterVolumesWidgets.push_back(m_ui->WaterVolumesLabel);
    WaterVolumesWidgets.push_back(m_ui->WaterVolumesLabel_uneditable);
    m_featureBlackListMap.insert("Water Volumes", WaterVolumesWidgets);
}

QString MobileSettingsDialog::GetAttribute(const QString& attributeName, XmlNodeRef& node)
{
    if (!node)
    {
        return "";
    }
    XmlNodeRef child = node->findChild(SETTING_PROPERTY_NODE);
    if (!child)
    {
        return "";
    }
    QString value = "";
    int version = -1;
    node->getAttr(SETTING_VERSION_NODE, version);
    switch (version) //Check version number
    {
    case MOBILESETTING_FILE_VERSION: // Current version
    {
        child->getAttr(attributeName.toUtf8().data(), value);
        break;
    }
    default:
        AZ_Assert(false, "Invalid version for MobileSettings: %d", version);
        break;
    }

    return QString(value);
}

/////////////////////////////////////////////////////////////////
//Source control Read&Write File
bool MobileSettingsDialog::CheckOutFile(const QString & filePath, const QString& description)
{
    if (GetIEditor()->IsSourceControlConnected())
    {
        bool success = false;
        ISourceControl* pSControl = GetIEditor()->GetSourceControl();
        if (pSControl)
        {
            int eFileAttribs = pSControl->GetFileAttributes(filePath.toStdString().c_str());
            // If the file is already checked out, do nothing and return true.
            if (eFileAttribs &SCC_FILE_ATTRIBUTE_CHECKEDOUT) 
            {
                return true;
            }
            // If the file is not in source control yet, add it 
            else if (!(eFileAttribs & SCC_FILE_ATTRIBUTE_MANAGED) && (eFileAttribs & SCC_FILE_ATTRIBUTE_NORMAL))
            {
                success = pSControl->Add(filePath.toStdString().c_str(), description.toStdString().c_str(), ADD_WITHOUT_SUBMIT | ADD_CHANGELIST);
                if (success)
                {
                    QString message = "File \"" + filePath + "\" is added to source control.";
                    QMessageBox::information(this, "Log", message, QMessageBox::Ok);
                }
            }
            // If the file is in source controll but not checked out, check out.
            else if (!(eFileAttribs & SCC_FILE_ATTRIBUTE_BYANOTHER) && !(eFileAttribs & SCC_FILE_ATTRIBUTE_CHECKEDOUT) && (eFileAttribs & SCC_FILE_ATTRIBUTE_MANAGED))
            {
                if (pSControl->GetLatestVersion(filePath.toStdString().c_str()))
                {
                    success = pSControl->CheckOut(filePath.toStdString().c_str(), ADD_CHANGELIST);
                    if (success)
                    {
                        QString message = "File \"" + filePath + "\" is checked out to source control.";
                        QMessageBox::information(this, "Log", message, QMessageBox::Ok);
                    }
                }
            }
            return success;
        }
    }
    else
    {
        // return true if user would like to overwrite the file
        // return false if user refuse to overwrite the file
        return CFileUtil::OverwriteFile(filePath);
    }
    return false;
}

// This function should only be called for those file does not exist. The 
// function will create an empty file for given path. If user enable the 
// source control and the file is no under source control yet, then the 
// function will mark the file as added.If the file is already under source
// control, check out the file will overwrite the empty file.
bool MobileSettingsDialog::AddFile(const QString& filePath, const QString& desc)
{
    // Create an empty file first. If the file does not exist the ISourceControl will
    // try looking for the file in other directories, which is not what we want.
    // Also please do not use CryFile or CFileUtil here for the same reason.

    // The file should not exists.
    AZ_Assert(!QFile::exists(filePath), "MobileSettingsDialog: Failed to add file to P4. File already exist %s", filePath.toStdString().c_str());
    QFile file(filePath);

    if (!file.open(QIODevice::WriteOnly))
    {
        QMessageBox::warning(this, "Warning", "Failed to create file " + filePath + ".",
            QMessageBox::Ok);
        return false;
    }
    file.close();

    if (GetIEditor()->IsSourceControlConnected())
    {
        bool success = false;
        ISourceControl* pSControl = GetIEditor()->GetSourceControl();
        if (pSControl)
        {
            int eFileAttribs = pSControl->GetFileAttributes(filePath.toStdString().c_str());
            if (eFileAttribs &SCC_FILE_ATTRIBUTE_CHECKEDOUT)
            {
                return true;
            }
            else if (!(eFileAttribs & SCC_FILE_ATTRIBUTE_BYANOTHER) && !(eFileAttribs & SCC_FILE_ATTRIBUTE_CHECKEDOUT) && (eFileAttribs & SCC_FILE_ATTRIBUTE_MANAGED))
            {
                if (pSControl->GetLatestVersion(filePath.toStdString().c_str()))
                {
                    success = pSControl->CheckOut(filePath.toStdString().c_str(), ADD_CHANGELIST);
                    if (success)
                    {
                        QString message = "File \"" + filePath + "\" is checked out to source control.";
                        QMessageBox::information(this, "Log", message,
                            QMessageBox::Ok);
                    }
                }
            }
            else // In other case create empty file and add it to source control
            {
                
                success = pSControl->Add(filePath.toStdString().c_str(), desc.toStdString().c_str(), ADD_WITHOUT_SUBMIT | ADD_CHANGELIST);
                if (success)
                {
                    QString message = "File \"" + filePath + "\" is added to source control.";
                    QMessageBox::information(this, "Log", message,
                        QMessageBox::Ok);
                }
            }
            
        }

        return success;
    }
    return true;
}

////////////////////////////////////////////////////////////////////////
// 
void MobileSettingsDialog::ShowParameterChange()
{
    QString warningmessage = "";
    for (ParameterWidget* widget : m_parameterWidgets)
    {
        if (widget->m_currentValue != widget->m_previousValue)
        {
            warningmessage += widget->m_widget->objectName() + ": " + widget->m_previousValue + " -> " + widget->m_currentValue + "\n";
        }
    }
    if (warningmessage.isEmpty()) return;
    warningmessage = "The following parameters are changed : \n" + warningmessage;

    QMessageBox::information(this, "Warning", warningmessage, QMessageBox::Ok);
}

QString MobileSettingsDialog::GetFeatureList(XmlNodeRef& node)
{
    QString FeatureList = "";
    bool isEditable = (m_currentpage == EDITABLE_PAGE);
    int version = -1;
    node->getAttr(SETTING_VERSION_NODE, version);
    switch (version) //Check version number
    {
    case MOBILESETTING_FILE_VERSION: // Current version
    {
        QDomDocument xmldoc;
        qDebug() << QString(node->getXML());
        xmldoc.setContent(QString(node->getXML()));
        QDomNode rootnode = xmldoc.firstChild();
        QDomElement propertyNode = rootnode.firstChildElement();
        QDomNamedNodeMap attributes = propertyNode.attributes();
        for (int i = 0; i < attributes.length(); i++)
        {
            QDomNode node = attributes.item(i);
            QDomAttr att = node.toAttr();
            FeatureList += att.name() + "=" + att.value() + "\n";
        }

        break;
    }
    default:
        AZ_Assert(false, "Unknown version of MobileSettings: %d", version);
        break;
    }
    return FeatureList;
}

/////////////////////////////////////////////////////////////////////////
// Collapse Group
MobileSettingsDialog::CollapseGroup::CollapseGroup()
    : m_dropDownButton_editable(nullptr)
    , m_gridlayout_editable(nullptr)
    , m_dropDownButton_uneditable(nullptr)
    , m_gridlayout_uneditable(nullptr)
    , m_isCollapsed(false)
{
    m_iconOpen = QIcon(":/PropertyEditor/Resources/group_open.png");
    m_iconClosed = QIcon(":/PropertyEditor/Resources/group_closed.png");
}

void MobileSettingsDialog::CollapseGroup::toggleButton(QPushButton* button, QGridLayout* layout)
{
    if (button && m_gridlayout_editable)
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

void MobileSettingsDialog::CollapseGroup::toggleCollpased()
{
    m_isCollapsed = !m_isCollapsed;
    toggleButton(m_dropDownButton_editable, m_gridlayout_editable);
    toggleButton(m_dropDownButton_uneditable, m_gridlayout_uneditable);
}

///////////////////////////////////////////////////////////////////
// ParameterWidget
MobileSettingsDialog::ParameterWidget::ParameterWidget(QWidget* widget, QWidget* uneditable, WidgetType type, WidgetType type_uneditable, QString parameterName)
    : m_widget(widget)
    , m_widget_uneditable(uneditable)
    , m_widgetType(type)
    , m_parameterName(parameterName)
    , m_uneditableWidgetType(type_uneditable)
    , m_currentValue("")
    , m_previousValue("")
{
    m_widget->setToolTip(GetToolTip());
    m_widget_uneditable->setToolTip(GetToolTip());
}

QString MobileSettingsDialog::ParameterWidget::value(bool forEditable)
{
    WidgetType type = m_widgetType;
    QWidget* Cwidget = m_widget;
    if (!forEditable)
    {
        type = m_uneditableWidgetType == WidgetType::NONE ? type : m_uneditableWidgetType;
        Cwidget = m_widget_uneditable;
    }

    switch (type)
    {
    case QSPINBOX:
    {
        QSpinBox* widget = static_cast<QSpinBox*>(Cwidget);
        return QString::number(widget->value());
    }
    case QDOUBLESPINBOX:
    {
        QDoubleSpinBox* widget = static_cast<QDoubleSpinBox*>(Cwidget);
        return QString::number(widget->value());
    }
    case QCOMBOBOX:
    {
        QComboBox* widget = static_cast<QComboBox*>(Cwidget);
        return widget->currentText();
    }
    case QCHECKBOX:
    {
        QCheckBox* widget = static_cast<QCheckBox*>(Cwidget);
        return QString::number(widget->isChecked() ? 1 : 0);
    }
    case QLABEL:
    {
        QLabel* widget = static_cast<QLabel*>(Cwidget);
        return widget->text();
    }
    case QLINEEDIT:
    {
        QLineEdit* widget = static_cast<QLineEdit*>(Cwidget);
        return widget->text();
    }
    default:
    {
        return "";
    }
    }
}

bool MobileSettingsDialog::ParameterWidget::SetBothValue(const QString& value)
{
    bool result = SetValue(value, false);
    // We always want to set the editable value last to overwrite the 
    // m_previousValue and m_currentValue. When user edits the editable 
    // widget, the update will no be reflected to the uneditable widget. 
    // For this reason, the editable widget always have the latest value 
    // settings, while uneditable widget would not.
    result = SetValue(value) && result;
    return result;
}

bool MobileSettingsDialog::ParameterWidget::SetValue(const QString& value, bool forEditable)
{
    WidgetType type = m_widgetType;
    QWidget* Cwidget = m_widget;
    if (!forEditable)
    {
        type = m_uneditableWidgetType == WidgetType::NONE ? type : m_uneditableWidgetType;
        Cwidget = m_widget_uneditable;
    }

    switch (type)
    {
    case QSPINBOX:
    {
        QSpinBox* widget = static_cast<QSpinBox*>(Cwidget);
        m_previousValue = QString::number(widget->value());
        m_currentValue = value;
        widget->setValue(value.toInt());
        return true;
    }
    case QDOUBLESPINBOX:
    {
        QDoubleSpinBox* widget = static_cast<QDoubleSpinBox*>(Cwidget);
        m_previousValue = QString::number(widget->value());
        m_currentValue = value;
        widget->setValue(value.toDouble());
        return true;
    }
    case QCOMBOBOX:
    {
        QComboBox* widget = static_cast<QComboBox*>(Cwidget);
        m_previousValue = widget->currentText();
        m_currentValue = value;
        widget->setCurrentText(value);
        return true;
    }
    case QCHECKBOX:
    {
        QCheckBox* widget = static_cast<QCheckBox*>(Cwidget);
        m_previousValue = widget->isChecked() ? "True" : "False";
        widget->setChecked(value != "0"
                           && value != "false" 
                           && value != "False");
        m_currentValue = widget->isChecked() ? "True" : "False";
        return true;
    }
    case QLABEL:
    {
        QLabel* widget = static_cast<QLabel*>(Cwidget);
        m_previousValue = widget->text();
        m_currentValue = value;
        widget->setText(value);
        return true;
    }
    case QLINEEDIT:
    {
        QLineEdit* widget = static_cast<QLineEdit*>(Cwidget);
        widget->setText(value);
        return true;
    }
    default:
    {
        return false;
    }
    }
}

QString MobileSettingsDialog::ParameterWidget::GetCommand(bool forEditable)
{
    if (!m_parameterName.isEmpty())
    {
        QString parameterValue = value(forEditable);
        
        // When widget is QComboBox widget, the actaully value for the 
        // Parameter will the index of the text 
        if (m_widgetType == WidgetType::QCOMBOBOX)
        {
            int index = (static_cast<QComboBox*>(m_widget))->findText(parameterValue);
            parameterValue = QString::number(index);
        }
        
        return m_parameterName + "=" + parameterValue + "\n";
    }
    return "";
}

QString MobileSettingsDialog::ParameterWidget::GetToolTip()
{
    if (!m_parameterName.isEmpty())
    {
        QString tooltipText = QString(PARAMETER_TOOLTIP).arg(m_parameterName);
        return tooltipText;
    }
    return "";
}


#include <MobileSettingsDialog.moc>
