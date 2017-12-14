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
#ifndef CRYINCLUDE_EDITOR_MOBILE_SETTINGS_DIALOG_H
#define CRYINCLUDE_EDITOR_MOBILE_SETTINGS_DIALOG_H
#pragma once

#include <QDialog>
#include <QMap>

class QAbstractButton;
class NestedQAction;
class MenuActionsModel;
class ActionShortcutsModel;
class QComboBox;
class QGridLayout;
class QLabel;

namespace Ui
{
    class MobileSettingsDialog;
}

class MobileSettingsDialog
    : public QDialog
{
    Q_OBJECT
public:    

    MobileSettingsDialog(QWidget* parent = nullptr);
    virtual ~MobileSettingsDialog();

private slots:
    //Update UIs
    void DeviceChanged(const QString& device);

public slots:
    //Accept and reject
    void reject() override;
    void accept() override;

private:
    // The struct ParameterWidget is used to store the parameter widget for
    // both editable version and uneditable version. 
    // m_parameterName will be the name of the parameter the widget represent.
    struct ParameterWidget
    {
      enum WidgetType
      {
        QSPINBOX = 0,
        QDOUBLESPINBOX,
        QCOMBOBOX,
        QCHECKBOX,
        QLABEL,
        QLINEEDIT,
        NONE                    //If uneditable widget type is None, will automatically treat the type as editable widget
      };
      QWidget* m_widget;
      QWidget* m_widget_uneditable;
      WidgetType m_widgetType;
      WidgetType m_uneditableWidgetType;
      QString m_parameterName;
      QString m_currentValue;
      QString m_previousValue;

      ParameterWidget(QWidget* widget, QWidget* uneditable, WidgetType type, WidgetType type_uneditable = WidgetType::NONE, QString parameterName = "");

      // Get value from editable widget or uneditable widget
      QString value(bool forEditable = true);
      bool SetBothValue(const QString& value);
      // Set value for editable widget or uneditable widget
      bool SetValue(const QString& value, bool forEditable = true);
      // Return the command used to set the parameter with current value
      QString GetCommand(bool forEditable);
      // If the widget is a render parameter related widget, return tooltips
      // for current parameter, otherwise return empty string
      QString GetToolTip();
    };

    struct CollapseGroup
    {
      QPushButton* m_dropDownButton_editable;
      QGridLayout* m_gridlayout_editable;
      QPushButton* m_dropDownButton_uneditable;
      QGridLayout* m_gridlayout_uneditable;

      bool m_isCollapsed;

      // UI STYLE
      QIcon m_iconOpen;
      QIcon m_iconClosed;

      CollapseGroup();

      void toggleCollpased();
      void toggleButton(QPushButton* button, QGridLayout* layout);
    };

    void SaveSystemSettings();
    // LoadSystemSettings() must be called after BuildUI().
    void LoadSystemSettings();

    // On load settings, return false, if loading file fails. Otherwise return true.
    // On export settings, SerializeSetting() always return true
    bool SerializeSetting(XmlNodeRef& node, bool isLoad);
    bool LoadSettings(const QString& filepath);
    bool SaveSettings(const QString& filepath);
    void ExportSettings();
    void ImportSettings();

    //MobileDeviceSettings GetSettingDataFromUI();
    //void SetSettingsToUI(MobileDeviceSettings data);

    void BuildUI();
    void BuildData();
    void BuildBlackListMap();

    /////////////////////////////////////////////
    // UI help functions
    // Set the page to show. Currently we have editable page for Customized dialog and uneditable for presets
    void SetPageIndex(int page);
    
    //Set drivers according to selected gpu
    void UpdateDriverComboBox(const QString& gpu);

    // Open file dialot to choose black list file from
    void OpenBlackListFileDialog();
    // Disable the feature according to blacklist
    void ApplyBlackList(const QString& blackListFile);
    // Reset all black list settings
    void ResetBlackList();
    // Disable widgets related to blacklist
    void SetWidgetDisabled(const QString& featureName, bool disabled);
    void SetWidgetDisabled(const QVector<QWidget*>& widgets, bool disabled);

    // Setup collapsed buttons
    void SetCollapsedLayout(QPushButton* togglebutton_editable, QGridLayout* layout_editable, QPushButton* togglebutton_uneditable, QGridLayout* layout_uneditable);
    // Show changes of variables 
    void ShowParameterChange();

    void ApplyMobileSettings();

    ///////////////////////////////////////////
    // Read data from xml node help functions
    QString GetAttribute(const QString& attributeName, XmlNodeRef& node);
    QString GetFeatureList(XmlNodeRef& node);    

    ////////////////////////////////////////////
    //Source Control Read&Write File
    bool CheckOutFile(const QString & filePath, const QString& desc = "MobileSettings");
    bool AddFile(const QString & filePath, const QString& desc = "MobileSettings");

    ////////////////////////////////////////////
    // Members
    int m_customDeviceId;
    int m_currentpage;
    QComboBox* m_driverComboBox;
    QComboBox* m_deviceComboBox;
    QComboBox* m_OSComboBox;
    QLabel* m_OSLabel;
    QLineEdit* m_blackListFile;
    QLabel* m_blackListFile_uneditable;
    bool m_isLoading;

    QScopedPointer<Ui::MobileSettingsDialog> m_ui;
    QMap<int, XmlNodeRef> m_mobileSettings;

    //The string lists hold the texts of the comboboxes
    QMap<QString, int> m_deviceStringList;
    QMap<QString, QVector<QString>> m_gpuAndDriverStringMap;

    //This stores all widgets related to specific settings. Not including m_mobileDevice.
    QMap<QString, QVector<QWidget*>> m_featureBlackListMap;

    QVector<CollapseGroup*> m_uiCollapseGroup;
    
    QVector<ParameterWidget*> m_parameterWidgets;

};

#endif //CRYINCLUDE_EDITOR_MOBILE_SETTINGS_DIALOG_H