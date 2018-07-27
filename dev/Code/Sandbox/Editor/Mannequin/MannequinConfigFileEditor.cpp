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
#include "MannequinConfigFileEditor.h"
#include "Util/PathUtil.h"
#include "Util/AutoDirectoryRestoreFileDialog.h"


#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QInputDialog>
#include <QPushButton>
#include <QDesktopServices>
#include <QUrl>
#include <QIODevice>
#include <QDir>

AZ::SerializeContext* CMannequinConfigFileEditor::s_serializeContext = nullptr;

namespace MannequinConfigFileHelper
{
    static const char* g_adbPath = "Animations/Mannequin/ADB/";
    static const char* g_previewPath = "Animations/Mannequin/Preview/";
    static const char* g_firstScopeName = "FullBody";
    static const char* g_firstContextName = "Main";

    //XML Element and Attribute names
    static const char* g_tagDefinitionElement = "TagDefinition";
    static const char* g_tagsElement = "Tags";
    static const char* g_animDBElement =  "AnimDB";
    static const char* g_fragmentListElement = "FragmentList";
    static const char* g_previewElement = "MannequinPreview";
    static const char* g_controllerDefElement = "controllerDef";
    static const char* g_contextsElement = "contexts";
    static const char* g_contextDataElement = "contextData";
    static const char* g_historyElement = "History";
    static const char* g_controllerDefRootElement = "ControllerDef";
    static const char* g_fragmentElement = "Fragments";
    static const char* g_fragmentDefs = "FragmentDefs";
    static const char* g_scopeContextDefsElement = "ScopeContextDefs";
    static const char* g_scopeDefsElement = "ScopeDefs";

    static const char* g_versionAttribute = "version";
    static const char* g_fragDefAttribute = "FragDef";
    static const char* g_tagDefAttribute = "TagDef";
    static const char* g_fileNameAttribute = "filename";
    static const char* g_nameAttribute = "name";
    static const char* g_enabledAttribute = "enabled";
    static const char* g_databaseAttribute = "database";
    static const char* g_contextAttribute = "context";
    static const char* g_modelAttribute = "model";
    static const char* g_startTimeAttribute = "StartTime";
    static const char* g_endTimeAttribute = "EndTime";
    static const char* g_layerAttribute = "layer";
    static const char* g_numLayersAttribute = "numLayers";

    static const char* g_controllerDefsFileNameSuffix = "ControllerDefs.xml";
    static const char* g_tagsFileNameSuffix = "Tags.xml";
    static const char* g_actionFileNameSuffix = "Actions.xml";
    static const char* g_previewFileNameSuffix = "Preview.xml";
    static const char* g_adbFileNameSuffix = "Anims.adb";
    static const char* g_fileValidationRegExp = "[\\\\:*?\" <>|]";

    static const int g_maxLayers = 16;
    static const int g_maxAssetFileCompileTries = 25;
    static const int g_delayBetweenAssetFileCompiles = 200;

    bool CreateFile(const AZStd::string &fileName)
    {
        const QString normalizedName = QDir::fromNativeSeparators(fileName.c_str());
        bool success = QDir().mkpath(normalizedName.left(normalizedName.lastIndexOf('/')));
        if (success)
        {
            QFile f(normalizedName);
            success = f.open(QIODevice::WriteOnly);
        }
        if (!success)
        {
            QMessageBox::critical(AzToolsFramework::GetActiveWindow(), QObject::tr("Error Creating File"), QObject::tr("Could not create file\n%1").arg(normalizedName));
        }
        return success;
    }

    bool ValidateFileName(const QString &fileName, const QString &fileLabel, QString &errorMessage)
    {
        if (fileName.isEmpty())
        {
            errorMessage = QObject::tr("%1 file name cannot be empty").arg(fileLabel);
            return false;
        }
        if (fileName.contains(QRegExp(MannequinConfigFileHelper::g_fileValidationRegExp)))
        {
            errorMessage = QObject::tr("%1 file name cannot contain the following characters: \\/:*?\"<>|").arg(fileLabel);
            return false;
        }
        return true;
    }
}

bool MannequinConfigFileHelper::CreateNewConfig(AZStd::string& generatedPreviewFileName, QWidget *parent)
{
    QString prefixStr;
    bool isValidFileName = false;
    while (!isValidFileName)
    {
        QInputDialog dialog(parent);
        dialog.setInputMode(QInputDialog::TextInput);
        dialog.setWindowTitle(QObject::tr("New Mannequin Setup"));
        dialog.setLabelText(QObject::tr("Setup Name"));
        dialog.setTextValue(prefixStr);
        dialog.resize(dialog.sizeHint() + QSize(100, 0));
        if (dialog.exec() != QDialog::Accepted)
        {
            return false;
        }
        prefixStr = dialog.textValue();

        QString errorMessage;
        isValidFileName = ValidateFileName(prefixStr, "Mannequin config", errorMessage);
        if (!isValidFileName)
        {
            QMessageBox::warning(parent, QObject::tr("Invalid Input"), errorMessage);
        }

        //Do not allow creating a preview with an existing name
        const AZStd::string previewsFileName = MannequinConfig(prefixStr.toUtf8().data()).PreviewFilePath();
        if (isValidFileName && CFileUtil::FileExists(previewsFileName.c_str(), nullptr))
        {
            isValidFileName = false;
            QMessageBox::warning(parent, QObject::tr("File Exists"), QObject::tr("A preview file already exists with that name."));
        }
    }

    AZStd::vector<AZStd::string> generatedAssets;
    const AZStd::string prefix = prefixStr.toUtf8().data();
    CMannequinConfigFileEditor editor(prefix, parent);
    
    editor.setMinimumSize(425, 600);
    if (editor.exec() == QDialog::Accepted)
    {
        const AZStd::string pathToGameFolder = Path::GetEditingGameDataFolder() + '/';

        const MannequinConfig config = editor.GetConfig();

        //Tags File
        const AZStd::string tagsFileName = config.TagsFilePath();
        if (!CFileUtil::FileExists(tagsFileName.c_str(), nullptr))
        {
            if (!CreateFile(tagsFileName))
            {
                return false;
            }
            XmlNodeRef tagsXML = XmlHelpers::CreateXmlNode(g_tagDefinitionElement);
            tagsXML->setAttr(g_versionAttribute, 2);
            tagsXML->saveToFile(tagsFileName.c_str());
            generatedAssets.push_back(config.tagsFile);
        }

        //Write Actions File
        const AZStd::string actionsFileName = config.ActionsFilePath();
        if (!CFileUtil::FileExists(actionsFileName.c_str(), nullptr))
        {
            if (!CreateFile(actionsFileName))
            {
                return false;
            }
            XmlNodeRef actionsXML = XmlHelpers::CreateXmlNode(g_tagDefinitionElement);
            actionsXML->setAttr(g_versionAttribute, 2);
            actionsXML->saveToFile(actionsFileName.c_str());
            generatedAssets.push_back(config.actionsFile);
        }

        //Write ADB File
        const AZStd::string adbFileName = config.ADBFilePath();
        if (!CFileUtil::FileExists(adbFileName.c_str(), nullptr))
        {
            if (!CreateFile(adbFileName))
            {
                return false;
            }
            XmlNodeRef adbXML = XmlHelpers::CreateXmlNode(g_animDBElement);
            adbXML->setAttr(g_fragDefAttribute, config.actionsFile.c_str());
            adbXML->setAttr(g_tagDefAttribute, config.tagsFile.c_str());
            XmlNodeRef tagDefNode = adbXML->newChild(g_fragmentListElement);
            adbXML->saveToFile(adbFileName.c_str());
            generatedAssets.push_back(config.previewADB);
        }

        //Write Preview File
        const AZStd::string previewsFileName = config.PreviewFilePath();
        if (!CFileUtil::FileExists(previewsFileName.c_str(), nullptr))
        {
            if (!CreateFile(previewsFileName))
            {
                return false;
            }
            XmlNodeRef previewXML = XmlHelpers::CreateXmlNode(g_previewElement);
            XmlNodeRef controllDef = previewXML->newChild(g_controllerDefElement);
            controllDef->setAttr(g_fileNameAttribute, config.ControllerDefsFileAssetID().c_str());
            XmlNodeRef contexts = previewXML->newChild(g_contextsElement);
            XmlNodeRef contextData = contexts->newChild(g_contextDataElement);
            contextData->setAttr(g_nameAttribute, config.characterName.c_str());
            contextData->setAttr(g_enabledAttribute, 1);
            contextData->setAttr(g_databaseAttribute, config.previewADB.c_str());
            contextData->setAttr(g_contextAttribute, config.scopeContext.c_str());
            contextData->setAttr(g_modelAttribute, config.previewModel.c_str());
            XmlNodeRef history = previewXML->newChild(g_historyElement);
            history->setAttr(g_startTimeAttribute, 0);
            history->setAttr(g_endTimeAttribute, 0);
            previewXML->saveToFile(previewsFileName.c_str());
            generatedPreviewFileName = previewsFileName;
            generatedAssets.push_back(config.PreviewFileAssetID());
        }

        //Write Controller Defs File
        const AZStd::string controllDefsFileName = config.ControllerDefsFilePath();
        if (!CFileUtil::FileExists(controllDefsFileName.c_str(), nullptr))
        {
            if (!CreateFile(controllDefsFileName))
            {
                return false;
            }
            XmlNodeRef defsXML = XmlHelpers::CreateXmlNode(g_controllerDefRootElement);
            XmlNodeRef tags = defsXML->newChild(g_tagsElement);
            tags->setAttr(g_fileNameAttribute, config.tagsFile.c_str());
            XmlNodeRef fragments = defsXML->newChild(g_fragmentElement);
            fragments->setAttr(g_fileNameAttribute, config.actionsFile.c_str());
            defsXML->newChild(g_fragmentDefs);

            //iterate through contexts. Add to ScopeContextDefs element and aggregate all scopes underneath
            XmlNodeRef scopeContextDefs = defsXML->newChild(g_scopeContextDefsElement);
            AZStd::vector<AZStd::pair<AZStd::string, MannequinConfig::ScopeDefinition>> allScopes;
            for (const auto context : config.contextList)
            {
                scopeContextDefs->newChild(context.name.c_str());
                for (const auto scope : context.scopeList)
                {
                    allScopes.push_back(AZStd::pair<AZStd::string, MannequinConfig::ScopeDefinition>(context.name, scope));
                }
            }

            //iterate through aggregated scopes and write ScopeDefs
            XmlNodeRef scopeDefs = defsXML->newChild(g_scopeDefsElement);
            for (int i = 0; i < allScopes.size(); ++i)
            {
                const MannequinConfig::ScopeDefinition scopeDef = allScopes.at(i).second;
                if (scopeDef.name.empty())
                {
                    continue;
                }
                XmlNodeRef scope = scopeDefs->newChild(scopeDef.name.c_str());
                scope->setAttr(g_layerAttribute, scopeDef.startLayer);
                scope->setAttr(g_numLayersAttribute, scopeDef.layerCount);
                scope->setAttr(g_contextAttribute, allScopes.at(i).first.c_str());
            }
            defsXML->saveToFile(controllDefsFileName.c_str());
            generatedAssets.push_back(config.ControllerDefsFileAssetID());
        }

        // before we load the preview, we must make sure all the assets have been compiled:
        // this SLEEP is temporary, to allow Asset Processor to know about the file(s) until there is a file system fence in place to take care of this on the AP side.
        // when we save a file then immediately attempt to load it, the operating system's kernel may not have yet informed the Asset Processor that the file was modified
        // or even exists

        for (auto assetPath : generatedAssets)
        {
            bool success = false;
            int count = 0;
            while (count < g_maxAssetFileCompileTries)
            {
                AzFramework::AssetSystem::AssetStatus status = AzFramework::AssetSystem::AssetStatus_Unknown;
                EBUS_EVENT_RESULT(status, AzFramework::AssetSystemRequestBus, GetAssetStatus, assetPath);
                if (status == AzFramework::AssetSystem::AssetStatus_Compiled)
                {
                    success = true;
                    break;
                }
                CrySleep(g_delayBetweenAssetFileCompiles);
                count++;
            }
            if (!success)
            {
                QString fileName = assetPath.c_str();
                QMessageBox::warning(parent, QObject::tr("Failed Compile"), QObject::tr("%1 was unable to be generated or compiled.").arg(fileName));
                return false;
            }
        }
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
CMannequinConfigFileEditor::CMannequinConfigFileEditor(const AZStd::string &prefix, QWidget *parent)
    : QDialog(parent)
    , m_config(prefix)
{
    if (!s_serializeContext)
    {
        EBUS_EVENT_RESULT(s_serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
        AZ_Assert(s_serializeContext, "Serialization context not available when reflecting CMannequinConfigFileEditor");
        CMannequinConfigFileEditor::Reflect(s_serializeContext);
    }

    setWindowTitle(QObject::tr("New Mannequin Setup"));

    m_propEditor = new AzToolsFramework::ReflectedPropertyEditor(parent);
    m_propEditor->Setup(s_serializeContext, nullptr, true, 200);

    m_config.CreateInitialContext();
    m_propEditor->AddInstance(&m_config);
    m_propEditor->InvalidateAll();
    m_propEditor->ExpandAll();

    QPushButton *helpButton = new QPushButton(tr("?"));
    QPushButton *createButton = new QPushButton(tr("Create"));
    QPushButton *cancelButton = new QPushButton(tr("Cancel"));

    //TODO: style the pushbutton
    connect(helpButton, &QPushButton::clicked, this, []() { QDesktopServices::openUrl(QUrl(QStringLiteral("http://docs.aws.amazon.com/lumberyard/latest/userguide/mannequin-intro.html"))); });
    connect(createButton, &QPushButton::clicked, this, &CMannequinConfigFileEditor::OnCreateClicked);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(helpButton);
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(createButton);
    buttonLayout->addWidget(cancelButton);
    QVBoxLayout *mainlayout = new QVBoxLayout(this);
    mainlayout->addWidget(m_propEditor);
    mainlayout->addLayout(buttonLayout);
}


void CMannequinConfigFileEditor::Reflect(AZ::ReflectContext* context)
{
    MannequinConfig::Reflect(context);

    //TODO: remove this when we have a proper File selector
    FileSelectorHandler::RegisterFileSelectorHandler();
}


void CMannequinConfigFileEditor::OnCreateClicked()
{
    QString errorMessage;
    bool result = Validate(errorMessage);
    if (result)
    {
        const QString cdf = QtUtil::ToQString(m_config.ControllerDefsFileAssetID().c_str());
        const QString preview = QtUtil::ToQString(m_config.PreviewFileAssetID().c_str());
        QString message = tr("These files will be generated:\n\nController Definitions File - %1\nPreview File - %2").arg(cdf, preview);
        if (!CFileUtil::FileExists(QString(m_config.ADBFilePath().c_str())))
        {
            message += tr("\nPreview ADB File - %1").arg(m_config.previewADB.c_str());
        }
        if (!CFileUtil::FileExists(QString(m_config.TagsFilePath().c_str())))
        {
            message += tr("\nTags File - %1").arg(m_config.tagsFile.c_str());
        }
        if (!CFileUtil::FileExists(QString(m_config.ActionsFilePath().c_str())))
        {
            message += tr("\nActions File - %1").arg(m_config.actionsFile.c_str());
        }
        auto response = QMessageBox::question(this, tr("Output Files"), message, QMessageBox::Yes, QMessageBox::Cancel);
        if (response == QMessageBox::Yes)
        {
            accept();
        }
        else
        {
            //return to dialog.
            return;
        }
    }
    else
    {
        QMessageBox::warning(this, tr("Warning"), tr("Error: %1").arg(errorMessage));
        return;
    }
}

bool CMannequinConfigFileEditor::ValidateFileName(const QString &filePath, const QString &fileLabel, const QString &requiredSuffix, QString &errorMessage)
{
    bool valid = MannequinConfigFileHelper::ValidateFileName(filePath.right(filePath.lastIndexOf("/")), fileLabel, errorMessage);
    if (valid && !requiredSuffix.isEmpty() && !filePath.endsWith(requiredSuffix))
    {
        valid = false;
        errorMessage = QObject::tr("%1 file name must end with %2").arg(fileLabel, requiredSuffix);
    }

    return valid;
}


bool CMannequinConfigFileEditor::Validate(QString &errorMessage)
{
    if (!ValidateFileName(m_config.controllerDefsFile.c_str(), tr("Controller Definitions"), MannequinConfigFileHelper::g_controllerDefsFileNameSuffix, errorMessage))
    {
        return false;
    }

    if (!ValidateFileName(m_config.tagsFile.c_str(), tr("Tags"), MannequinConfigFileHelper::g_tagsFileNameSuffix, errorMessage))
    {
        return false;
    }

    if (!ValidateFileName(m_config.actionsFile.c_str(), tr("Actions"), MannequinConfigFileHelper::g_actionFileNameSuffix, errorMessage))
    {
        return false;
    }

    if (!ValidateFileName(m_config.previewFile.c_str(), tr("Preview"), MannequinConfigFileHelper::g_previewFileNameSuffix, errorMessage))
    {
        return false;
    }

    if (!ValidateFileName(m_config.previewADB.c_str(), tr("Preview ADB"), MannequinConfigFileHelper::g_adbFileNameSuffix, errorMessage))
    {
        return false;
    }

    if (m_config.characterName.empty())
    {
        errorMessage = tr("Character name cannot be empty");
        return false;
    }

    //model file must exist
    const QString modelFile = m_config.ModelFilePath().c_str();
    if (!CFileUtil::FileExists(modelFile))
    {
        errorMessage = tr("Invalid Preview Model file");
        return false;
    }

    //preview file must not exist
    const QString previewFile = m_config.PreviewFilePath().c_str();
    if (CFileUtil::FileExists(previewFile))
    {
        errorMessage = tr("A Preview File with the selected name already exists");
        return false;
    }
    //controller def file must not exist
    const QString controllerDefsFile = m_config.ControllerDefsFilePath().c_str();
    if (CFileUtil::FileExists(controllerDefsFile))
    {
        errorMessage = tr("A Controller Definitions File with the selected name already exists");
        return false;
    }

    //context names cannot be empty
    auto contextList = m_config.GetAvailableContexts();
    auto emptyContext = std::find_if(std::cbegin(contextList), std::cend(contextList), [](const AZStd::string &context) { return context.empty(); });
    if (emptyContext != std::cend(contextList))
    {
        errorMessage = tr("Context names cannot be empty");
        return false;
    }

    //context names must be unique
    auto lastContext = std::unique(contextList.begin(), contextList.end());
    if (lastContext != contextList.end())
    {
        errorMessage = tr("Context names must be unique");
        return false;
    }

    //scope names cannot be empty
    auto scopeList = m_config.GetAllScopes();
    auto emptyScope = std::find_if(std::cbegin(scopeList), std::cend(scopeList), [](const AZStd::string &scope) { return scope.empty(); });
    if (emptyScope != std::cend(scopeList))
    {
        errorMessage = tr("Scope names cannot be empty");
        return false;
    }

    //scope names must be unique
    auto lastScope = std::unique(scopeList.begin(), scopeList.end());
    if (lastScope != scopeList.end())
    {
        errorMessage = tr("Scope names must be unique");
        return false;
    }

    return true;
}



MannequinConfig::MannequinConfig(const AZStd::string &prefix)
{
    controllerDefsFile = prefix + MannequinConfigFileHelper::g_controllerDefsFileNameSuffix;
    tagsFile = MannequinConfigFileHelper::g_adbPath + prefix + MannequinConfigFileHelper::g_tagsFileNameSuffix;
    actionsFile = MannequinConfigFileHelper::g_adbPath + prefix + MannequinConfigFileHelper::g_actionFileNameSuffix;
    previewADB = MannequinConfigFileHelper::g_adbPath + prefix + MannequinConfigFileHelper::g_adbFileNameSuffix;
    previewFile = prefix + MannequinConfigFileHelper::g_previewFileNameSuffix;
    characterName = prefix + MannequinConfigFileHelper::g_firstContextName;
    previewModel = "Animations/Mannequin/";
}

void MannequinConfig::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
    if (serializeContext)
    {
        serializeContext->Class<ScopeDefinition>()
            ->Version(1)
            ->Field("Scope Name", &ScopeDefinition::name)
            ->Field("Start Layer", &ScopeDefinition::startLayer)
            ->Field("Layer Count", &ScopeDefinition::layerCount)
            ;

        serializeContext->Class<ContextDefinition>()
            ->Version(1)
            ->Field("Context Name", &ContextDefinition::name)
            ->Field("Scope List", &ContextDefinition::scopeList)
            ;

        serializeContext->Class<MannequinConfig>()
            ->Version(1)
            ->Field("Context Definitions", &MannequinConfig::contextList)
            ->Field("Controller Definitions File", &MannequinConfig::controllerDefsFile)
            ->Field("Tags File", &MannequinConfig::tagsFile)
            ->Field("Actions File", &MannequinConfig::actionsFile)
            ->Field("Character Name", &MannequinConfig::characterName)
            ->Field("Scope Context", &MannequinConfig::scopeContext)
            ->Field("Preview Model", &MannequinConfig::previewModel)
            ->Field("Preview ADB", &MannequinConfig::previewADB)
            ->Field("Preview File", &MannequinConfig::previewFile)
            ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (editContext)
        {
            editContext->Class<MannequinConfig>(
                "Mannequin Configuration File Data", "Data used to create and editor Mannequin config files")
                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly", 0xef428f20))
                //CDF Config
                ->ClassElement(AZ::Edit::ClassElements::Group, "Controller Definitions")
                ->DataElement(AZ::Edit::UIHandlers::Default, &MannequinConfig::contextList, "Context Definitions", "")
                    ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, true)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::RemoveNotify, &MannequinConfig::CreateInitialContext)
                    ->Attribute(AZ::Edit::Attributes::AddNotify, &MannequinConfig::OnContextAdded)
                ->DataElement(AZ::Edit::UIHandlers::Default, &MannequinConfig::controllerDefsFile, "Controller Definitions File", "ControllerDefs file located in the Mannequin/ADB folder storing Mannequin data")
                ->DataElement(AZ_CRC("FileSelector", 0xb9bcba1b), &MannequinConfig::tagsFile, "Tags File", "Tags file located in the Mannequin/ADB folder storing tags. This file is reusable across Mannequin Controller setups.")
                    ->Attribute(AZ_CRC("DisplayMode", 0x0ef4f50b), (int)FileSelectorPropertyWidget::DisplayMode::ShowFileName)
                    ->Attribute(AZ_CRC("FileFilter", 0x5f7f03b6), "XML Files (*.xml)")
                ->DataElement(AZ_CRC("FileSelector", 0xb9bcba1b), &MannequinConfig::actionsFile, "Actions File", "Actions file located in the Mannequin/ADB folder storing actions. This file is reusable across Mannequin Controller setups.")
                    ->Attribute(AZ_CRC("DisplayMode", 0x0ef4f50b), (int)FileSelectorPropertyWidget::DisplayMode::ShowFileName)
                    ->Attribute(AZ_CRC("FileFilter", 0x5f7f03b6), "XML Files (*.xml)")

                //Preview Config
                ->ClassElement(AZ::Edit::ClassElements::Group, "Main Preview Character")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->DataElement(AZ::Edit::UIHandlers::Default, &MannequinConfig::characterName, "Character Name", "The name of the Preview Context")
                ->DataElement(AZ::Edit::UIHandlers::ComboBox, &MannequinConfig::scopeContext, "Scope Context", "The Context defined in Controller Definitions that the preview will be based on")
                    ->Attribute(AZ::Edit::Attributes::StringList, &MannequinConfig::GetAvailableContexts)
                ->DataElement(AZ_CRC("FileSelector", 0xb9bcba1b), &MannequinConfig::previewModel, "Preview Model", "Character definition files are combinations of skeletons, skins and attachments. They are authored in Geppetto.")
                    ->Attribute(AZ_CRC("DisplayMode", 0x0ef4f50b), (int)FileSelectorPropertyWidget::DisplayMode::ShowFileName)
                    ->Attribute(AZ_CRC("FileFilter", 0x5f7f03b6), "CDF Files (*.cdf)")
                ->DataElement(AZ_CRC("FileSelector", 0xb9bcba1b), &MannequinConfig::previewADB, "Preview ADB", "ADB file located in Mannequin/ADB folder storing animation data")
                    ->Attribute(AZ_CRC("FileFilter", 0x5f7f03b6), "ADB Files (*.adb)")
                    ->Attribute(AZ_CRC("DisplayMode", 0x0ef4f50b), (int)FileSelectorPropertyWidget::DisplayMode::ShowFileName)
                ->DataElement(AZ::Edit::UIHandlers::Default, &MannequinConfig::previewFile, "Preview File", "Preview file located in the Mannequin/Previews folder that sets up the Mannequin tool preview.")
                ;

            editContext->Class<ContextDefinition>("Context Definitions", "")
                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &ContextDefinition::GetExpanderName)
                ->DataElement(AZ::Edit::UIHandlers::Default, &ContextDefinition::name, "Context Name", "Category designation of the movement set. Examples: Player, Enemy, Robot, Ninja, Pirate, Zombie")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshAttributesAndValues", 0xcbc2147c))
                ->DataElement(AZ::Edit::UIHandlers::Default, &ContextDefinition::scopeList, "Scope Definitions", "")
                    ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, true)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::RemoveNotify, &ContextDefinition::OnScopeRemoved)
                    ->Attribute(AZ_CRC("AddNotify", 0x16f00b95), &ContextDefinition::OnScopeAdded)
                ;

            editContext->Class<ScopeDefinition>("Scope Definitions", "")
                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &ScopeDefinition::GetExpanderName)
                ->DataElement(AZ::Edit::UIHandlers::Default, &ScopeDefinition::name, "Scope Name", "Category of parts of a character. Examples: FullBody, Torso, Legs, Arms")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshAttributesAndValues", 0xcbc2147c))
                ->DataElement(AZ::Edit::UIHandlers::Default, &ScopeDefinition::startLayer, "Start Layer", "The first animation layer the fragment will occupy")
                    ->Attribute(AZ::Edit::Attributes::Min, 0)
                    ->Attribute(AZ::Edit::Attributes::Max, MannequinConfigFileHelper::g_maxLayers - 1)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshAttributesAndValues", 0xcbc2147c))
                ->DataElement(AZ::Edit::UIHandlers::Default, &ScopeDefinition::layerCount, "Layer Count", "The number of layers the fragment will use")
                    ->Attribute(AZ::Edit::Attributes::Min, 1)
                    ->Attribute(AZ::Edit::Attributes::Max, &ScopeDefinition::maxLayerCount)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshAttributesAndValues", 0xcbc2147c))
                ;
        }
    }
}

MannequinConfig::ContextDefinition::ContextDefinition(const AZStd::string &name, const AZStd::vector<ScopeDefinition> &scopes)
    : name(name)
    , scopeList(scopes)
{
}


MannequinConfig::ContextDefinition::ContextDefinition()
{
}

void MannequinConfig::ContextDefinition::OnScopeRemoved()
{
    if (scopeList.empty())
    {
        scopeList.push_back(ScopeDefinition());
    }
}

void MannequinConfig::ContextDefinition::OnScopeAdded()
{
    static int scopeNo = 1;
    if (scopeList[scopeList.size() - 1].name.empty())
    {
        scopeList[scopeList.size() - 1].name = QString("ScopeDef%1").arg(scopeNo++).toLocal8Bit().data();
    }
}

AZStd::string MannequinConfig::ContextDefinition::GetExpanderName() const
{
    return name;
}

MannequinConfig::ScopeDefinition::ScopeDefinition()
    : startLayer(0)
    , layerCount(1)
{
}


MannequinConfig::ScopeDefinition::ScopeDefinition(AZStd::string name, int start, int count)
    : name(name)
    , startLayer(start)
    , layerCount(count)
{
}


int MannequinConfig::ScopeDefinition::maxLayerCount() const
{
    return MannequinConfigFileHelper::g_maxLayers - startLayer;
}

AZStd::string MannequinConfig::ScopeDefinition::GetExpanderName() const
{
    return name;
}

void MannequinConfig::CreateInitialContext()
{
    if (contextList.empty())
    {
        contextList.push_back(ContextDefinition());
        contextList[0].name = MannequinConfigFileHelper::g_firstContextName;
        scopeContext = contextList[0].name;
        OnContextAdded();
    }
}

void MannequinConfig::OnContextAdded()
{
    static int contextNo = 1;
    const int contextIndex = contextList.size() - 1;
    AZ_Assert(contextIndex >= 0, "OnContextAdded called without any contexts being added");
    if (contextList[contextIndex].name.empty())
    {
        contextList[contextIndex].name = QString("ScopeContext%1").arg(contextNo++).toLocal8Bit().data();
    }

    ScopeDefinition scope;
    if (contextIndex == 0)
    {
        scope.name = MannequinConfigFileHelper::g_firstScopeName;
    }
    contextList[contextIndex].scopeList.push_back(scope);
    contextList[contextIndex].OnScopeAdded();
}

AZStd::vector<AZStd::string> MannequinConfig::GetAvailableContexts() const
{
    AZStd::vector<AZStd::string> contextNames(contextList.size());
    for (int i = 0; i < contextList.size(); ++i)
    {
        contextNames[i] = contextList[i].name;
    }
    return contextNames;
}

AZStd::vector<AZStd::string> MannequinConfig::GetAllScopes() const
{
    AZStd::vector<AZStd::string> scopeNames;
    for (const auto context : contextList)
    {
        for (const auto scope : context.scopeList)
        {
            scopeNames.push_back(scope.name);
        }
    }
    return scopeNames;
}

AZStd::string MannequinConfig::ControllerDefsFileAssetID() const
{
    return MannequinConfigFileHelper::g_adbPath + controllerDefsFile;
}

AZStd::string MannequinConfig::PreviewFileAssetID() const
{
    return MannequinConfigFileHelper::g_previewPath + previewFile;
}

AZStd::string MannequinConfig::PreviewFilePath() const
{
    return FullPathFromAssetID(PreviewFileAssetID());
}

AZStd::string MannequinConfig::ControllerDefsFilePath() const
{
    return FullPathFromAssetID(ControllerDefsFileAssetID());
}

AZStd::string MannequinConfig::TagsFilePath() const
{
    return FullPathFromAssetID(tagsFile);
}

AZStd::string MannequinConfig::ActionsFilePath() const
{
    return FullPathFromAssetID(actionsFile);
}

AZStd::string MannequinConfig::ADBFilePath() const
{
    return FullPathFromAssetID(previewADB);
}

AZStd::string MannequinConfig::ModelFilePath() const
{
    return FullPathFromAssetID(previewModel);
}

AZStd::string MannequinConfig::FullPathFromAssetID(const AZStd::string &id) const
{
    return Path::GetEditingGameDataFolder() + '/' + id;
}

#include <QToolButton>
FileSelectorPropertyWidget::FileSelectorPropertyWidget(QWidget *pParent /*= nullptr*/)
    :QWidget(pParent)
    , m_mode(DisplayMode::ShowFileName)
{
    m_valueEdit = new QLineEdit;

    QToolButton *mainButton = new QToolButton;
    mainButton->setText("..");
    connect(mainButton, &QToolButton::clicked, this, &FileSelectorPropertyWidget::OnEditClicked);
    connect(m_valueEdit, &QLineEdit::textEdited, this, &FileSelectorPropertyWidget::OnTextChanged);

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->addWidget(m_valueEdit, 1);
    mainLayout->addWidget(mainButton);
    mainLayout->setContentsMargins(1, 1, 1, 1);
}

void FileSelectorPropertyWidget::SetValue(const QString &value)
{
    //check whether we passed in assetID or just filename.
    int lastDirSeparator = value.lastIndexOf(QLatin1String("/"));
    if (lastDirSeparator != -1)
    {
        //store file name and folder separately
        m_fileName = value.mid(lastDirSeparator + 1);
        m_assetIDFolder = value.left(lastDirSeparator);
    }
    else
    {
        m_fileName = value;
        m_assetIDFolder.clear();
    }

    UpdateLineEdit();
}

QString FileSelectorPropertyWidget::GetValue() const
{
    return m_fileName.isEmpty() ? QString() : QString("%1/%2").arg(m_assetIDFolder, m_fileName);
}


void FileSelectorPropertyWidget::OnTextChanged(const QString &newText)
{
    m_fileName = newText;
    emit ValueChanged(GetValue());
}

void FileSelectorPropertyWidget::OnEditClicked()
{
    const QString pathToGameFolder = QDir::fromNativeSeparators(Path::GetEditingGameDataFolder().c_str() + QLatin1String("/"));
    const QString initialFolder =  pathToGameFolder + m_assetIDFolder;
    CAutoDirectoryRestoreFileDialog dlg(QFileDialog::AcceptOpen, QFileDialog::AnyFile, QString{}, initialFolder, m_fileFilter, QFileDialog::DontConfirmOverwrite, tr("Select File"), this);
    dlg.selectFile(m_fileName);
    dlg.setLabelText(QFileDialog::Accept, "Select");
    if (dlg.exec() == QDialog::Accepted)
    {
        QString fileName = dlg.selectedFiles().first();
        const QDir gameDir(pathToGameFolder);
        const QDir selectedFileGameDir(fileName.left(pathToGameFolder.length()));
        if (gameDir != selectedFileGameDir)
        {
            QMessageBox::warning(this, tr("Invalid File"), tr("You must select a file location within the current game"));
            return;
        }
        fileName = fileName.mid(pathToGameFolder.length());
        SetValue(fileName);
        emit ValueChanged(GetValue());
    }
}

void FileSelectorPropertyWidget::UpdateLineEdit()
{
    m_valueEdit->setText(m_mode == DisplayMode::ShowFileName ? m_fileName : GetValue());
}


void FileSelectorPropertyWidget::SetDisplayMode(DisplayMode mode)
{
    if (m_mode != mode)
    {
        m_mode = mode;
        UpdateLineEdit();
    }
}

void FileSelectorPropertyWidget::SetFileFilter(const QString &fileFilter)
{
    m_fileFilter = fileFilter;
}

bool FileSelectorHandler::IsDefaultHandler() const
{
    return false;
}

AZ::u32 FileSelectorHandler::GetHandlerName() const
{
    return AZ_CRC("FileSelector", 0xb9bcba1b);
}


QWidget* FileSelectorHandler::CreateGUI(QWidget *pParent)
{
    FileSelectorPropertyWidget* newCtrl = aznew FileSelectorPropertyWidget(pParent);
    connect(newCtrl, &FileSelectorPropertyWidget::ValueChanged, newCtrl, [newCtrl]()
    {
        EBUS_EVENT(AzToolsFramework::PropertyEditorGUIMessages::Bus, RequestWrite, newCtrl);
    });

    return newCtrl;
}

void FileSelectorHandler::ConsumeAttribute(FileSelectorPropertyWidget* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
{
    if (attrib == AZ_CRC("DisplayMode", 0x0ef4f50b))
    {
        int value;
        if (attrValue->Read<int>(value))
        {
            FileSelectorPropertyWidget::DisplayMode mode = FileSelectorPropertyWidget::DisplayMode::ShowPath;
            if (value == static_cast<int>(FileSelectorPropertyWidget::DisplayMode::ShowFileName))
            {
                mode = FileSelectorPropertyWidget::DisplayMode::ShowFileName;
            }
            GUI->SetDisplayMode(mode);
        }
        else
        {
            // emit a warning!
            AZ_WarningOnce("FileSelectorHandler", false, "Failed to read 'DisplayMode' attribute from property '%s' into FileSelectorHandler", debugName);
        }
    }
    if (attrib == AZ_CRC("FileFilter", 0x5f7f03b6))
    {
        AZStd::string value;
        if (attrValue->Read<AZStd::string>(value))
        {
            GUI->SetFileFilter(value.c_str());
        }
        else
        {
            // emit a warning!
            AZ_WarningOnce("FileSelectorHandler", false, "Failed to read 'FileFilter' attribute from property '%s' into FileSelectorHandler", debugName);
        }
    }
}

void FileSelectorHandler::WriteGUIValuesIntoProperty(size_t /*index*/, FileSelectorPropertyWidget* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* /*node*/)
{
    AZStd::string val;
    val = GUI->GetValue().toUtf8().data();
    instance = static_cast<property_t>(val);
}

bool FileSelectorHandler::ReadValuesIntoGUI(size_t /*index*/, FileSelectorPropertyWidget* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* /*node*/)
{
    GUI->SetValue(instance.c_str());
    return false;
}

void FileSelectorHandler::RegisterFileSelectorHandler()
{
    EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, aznew FileSelectorHandler());
}



#include <Mannequin/MannequinConfigFileEditor.moc>
