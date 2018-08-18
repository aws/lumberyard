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

#include <QDialog>

#include <AzToolsFramework/UI/PropertyEditor/PropertyDoubleSpinCtrl.hxx>
#include <AzToolsFramework/UI/PropertyEditor/PropertyIntSpinCtrl.hxx>

class QGridLayout;
class QLabel;

namespace Ui
{
    class GraphicsSettingsDialog;
}

class GraphicsSettingsDialog
    : public QDialog
{
    Q_OBJECT

public:

    explicit GraphicsSettingsDialog(QWidget* parent = nullptr);
    virtual ~GraphicsSettingsDialog();

public slots:
    //Accept and reject
    void reject() override;
    void accept() override;

private slots:
    //Update UIs
    void PlatformChanged(const QString& platform);
    bool CVarChanged(AZStd::any val, const char* cvarName, int specLevel);
    void CVarChanged(AZ::s64 i);
    void CVarChanged(double d);
    void CVarChanged(const QString& s);

private:

    // The struct ParameterWidget is used to store the parameter widget
    // m_parameterName will be the name of the parameter the widget represent.
    struct ParameterWidget
    {
        ParameterWidget(QWidget* widget, QString parameterName = "");

        QString GetToolTip() const;

        const char* PARAMETER_TOOLTIP = "The variable will update render parameter \"%1\".";
        QWidget* m_widget;
        QString m_parameterName;
    };

    struct CollapseGroup
    {
        QPushButton* m_dropDownButton;
        QGridLayout* m_gridlayout;

        bool m_isCollapsed;

        // UI STYLE
        QIcon m_iconOpen;
        QIcon m_iconClosed;

        CollapseGroup();

        void ToggleCollpased();
        void ToggleButton(QPushButton* button, QGridLayout* layout);
    };

    void OpenCustomSpecDialog();
    void ApplyCustomSpec(const QString& customFilePath);

    enum CVarStateComparison
    {
        EDITED_OVERWRITTEN_COMPARE = 1,
        EDITED_ORIGINAL_COMPARE = 2,
        OVERWRITTEN_ORIGINAL_COMPARE = 3,

        END_CVARSTATE_COMPARE,
    };

    bool CheckCVarStatesForDiff(AZStd::pair<AZStd::string, CVarInfo>* it, int cfgFileIndex, CVarStateComparison cmp);

    // Save out settings into project-level cfg files
    void SaveSystemSettings();

    // Load in project-level cfg files for current platform
    void LoadPlatformConfigurations();
    // Build UI column for spec level of current platform
    void BuildColumn(int specLevel);
    // Initial UI building
    void BuildUI();
    // Cleaning out UI before loading new platform information
    void CleanUI();
    // Shows/hides custom spec option
    void ShowCustomSpecOption(bool show);
    // Shows/hides category labels and dropdowns
    void ShowCategories(bool show);
    // Warns about unsaved changes (returns true if accepted)
    bool SendUnsavedChangesWarning(bool cancel);

    /////////////////////////////////////////////
    // UI help functions

    // Setup collapsed buttons
    void SetCollapsedLayout(QPushButton* togglebutton, QGridLayout* layout);
    // Sets the platform entry index for the given platform
    void SetPlatformEntry(ESystemConfigPlatform platform);
    // Gets the platform enum given the platform name
    ESystemConfigPlatform GetConfigPlatformFromName(const AZStd::string& platformName);

    ////////////////////////////////////////////
    // Members

    // Qt values
    const int INPUT_MIN_WIDTH = 100;
    const int INPUT_MIN_HEIGHT = 20;
    const int INPUT_ROW_SPAN = 1;
    const int INPUT_COLUMN_SPAN = 1;
    const int CVAR_ROW_OFFSET = 2;
    const int CVAR_VALUE_COLUMN_OFFSET = 2;
    const int PLATFORM_LABEL_ROW = 1;
    const int CVAR_LABEL_COLUMN = 1;

    // Tool tips
    const QString SETTINGS_FILE_PATH = "Config/spec/";
    const char* CFG_FILEFILTER = "Cfg File(*.cfg);;All files(*)";

    bool m_isLoading;
    bool m_showCustomSpec;
    bool m_showCategories;

    QScopedPointer<Ui::GraphicsSettingsDialog> m_ui;

    QVector<CollapseGroup*> m_uiCollapseGroup;

    QVector<ParameterWidget*> m_parameterWidgets;

    // cvar name --> pair(type, CVarStatus for each file)
    AZStd::unordered_map<AZStd::string, CVarInfo> m_cVarTracker;

    AZStd::unordered_map<ESystemConfigPlatform, AZStd::vector<AZStd::string> > m_cfgFiles;

    AZStd::vector<AZStd::pair<AZStd::string, ESystemConfigPlatform> > m_platformStrings;

    struct CVarGroupInfo
    {
        QVector<QLabel*> m_platformLabels;
        QVector<QLabel*> m_cvarLabels;
        QVector<AzToolsFramework::PropertyIntSpinCtrl*> m_cvarSpinBoxes;
        QVector<AzToolsFramework::PropertyDoubleSpinCtrl*> m_cvarDoubleSpinBoxes;
        QVector<QLineEdit*> m_cvarLineEdits;
        QGridLayout* m_layout;
    };

    AZStd::unordered_map<AZStd::string, CVarGroupInfo> m_cvarGroupData;

    ESystemConfigPlatform m_currentPlatform;

    int m_dirtyCVarCount;
};
