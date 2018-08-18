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

#ifndef CRYINCLUDE_EDITOR_MANNEQUIN_HELPER_MANNEQUINCONFIGFILEEDITOR_H
#define CRYINCLUDE_EDITOR_MANNEQUIN_HELPER_MANNEQUINCONFIGFILEEDITOR_H
#pragma once

#include <QDialog>
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
class QLineEdit;

namespace AzToolsFramework
{
    class ReflectedPropertyEditor;
}

namespace MannequinConfigFileHelper
{
    bool CreateNewConfig(AZStd::string& generatedPreviewFileName, QWidget *parent = nullptr);
    bool ValidateFileName(const QString &fileName, const QString &fileLabel, QString &errorMessage);
}

struct MannequinConfig
{
    AZ_TYPE_INFO(MannequinConfig, "{46594EBF-06D3-4416-958C-EDC44BBD4C01}");
    MannequinConfig(const AZStd::string &prefix);
    MannequinConfig() {}

    static void Reflect(AZ::ReflectContext* context);

    struct ScopeDefinition
    {
        AZ_TYPE_INFO(ScopeDefinition, "{65BBAC48-7C44-4E05-BC2A-FBDDD4C38525}");
        ScopeDefinition();
        ScopeDefinition(AZStd::string name, int start, int count);

        int maxLayerCount() const;
        AZStd::string GetExpanderName() const;

        AZStd::string name;
        int startLayer;
        int layerCount;
    };

    struct ContextDefinition
    {
        AZ_TYPE_INFO(ContextDefinition, "{815D3525-EE6D-4FDA-B247-6128559CE5B7}");
        ContextDefinition();
        ContextDefinition(const AZStd::string &name, const AZStd::vector<ScopeDefinition> &scopes);

        AZStd::string name;
        AZStd::vector<ScopeDefinition> scopeList;
        void OnScopeRemoved();
        void OnScopeAdded();
        AZStd::string GetExpanderName() const;
    };

    void CreateInitialContext();
    AZStd::vector<AZStd::string> GetAvailableContexts() const;
    AZStd::vector<AZStd::string> GetAllScopes() const;

    AZStd::string ControllerDefsFileAssetID() const;
    AZStd::string PreviewFileAssetID() const;

    //Get full file paths
    AZStd::string PreviewFilePath() const;
    AZStd::string TagsFilePath() const;
    AZStd::string ActionsFilePath() const;
    AZStd::string ControllerDefsFilePath() const;
    AZStd::string ADBFilePath() const;
    AZStd::string ModelFilePath() const;

    AZStd::vector<ContextDefinition> contextList;
    AZStd::string controllerDefsFile;
    AZStd::string tagsFile;
    AZStd::string actionsFile;
    AZStd::string characterName;
    AZStd::string scopeContext;
    AZStd::string previewModel;
    AZStd::string previewADB;
    AZStd::string previewFile;

private:
    AZStd::string FullPathFromAssetID(const AZStd::string &id) const;
    void OnContextAdded();
};


class CMannequinConfigFileEditor
    : public QDialog
{
    Q_OBJECT
public:
    CMannequinConfigFileEditor(const AZStd::string &prefix, QWidget *parent = nullptr);

    static void Reflect(AZ::ReflectContext* context);
    MannequinConfig GetConfig() const {return m_config;}

private:
    void OnCreateClicked();
    bool Validate(QString &errorMessage);
    bool ValidateFileName(const QString &filePath, const QString &fileLabel, const QString &requiredSuffix, QString &errorMessage);

    MannequinConfig m_config;
    AzToolsFramework::ReflectedPropertyEditor* m_propEditor;
    static AZ::SerializeContext* s_serializeContext;
};

class FileSelectorPropertyWidget : public QWidget
{
    Q_OBJECT
public:
    enum class DisplayMode
    {
        ShowFileName,
        ShowPath
    };
    AZ_CLASS_ALLOCATOR(FileSelectorPropertyWidget, AZ::SystemAllocator, 0);
    FileSelectorPropertyWidget(QWidget *pParent = nullptr);

    void SetValue(const QString &value);
    QString GetValue() const;

    void SetDisplayMode(DisplayMode mode);
    void SetFileFilter(const QString &fileFilter);

    void OnEditClicked();

signals:
    void ValueChanged(const QString &value);

private:
    void OnTextChanged(const QString &newText);
    void UpdateLineEdit();

private:
    //The path from the game folder (e.g. Animations/Mannequin/Preview/)
    QString m_assetIDFolder;
    QString m_fileName;

    DisplayMode m_mode;
    QString m_fileFilter;

    QLineEdit *m_valueEdit;
};

class FileSelectorHandler : public QObject, public AzToolsFramework::PropertyHandler <AZStd::string, FileSelectorPropertyWidget>
{
public:
    AZ_CLASS_ALLOCATOR(FileSelectorHandler, AZ::SystemAllocator, 0);
    bool IsDefaultHandler() const override;
    QWidget* CreateGUI(QWidget *pParent) override;

    AZ::u32 GetHandlerName() const override;
    void ConsumeAttribute(FileSelectorPropertyWidget* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;
    void WriteGUIValuesIntoProperty(size_t index, FileSelectorPropertyWidget* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
    bool ReadValuesIntoGUI(size_t index, FileSelectorPropertyWidget* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)  override;

    static void RegisterFileSelectorHandler();
};


#endif // CRYINCLUDE_EDITOR_MANNEQUIN_HELPER_MANNEQUINFILECHANGEWRITER_H
