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
#ifndef CRYINCLUDE_EDITORUI_QT_ATTRIBUTEVIEW_H
#define CRYINCLUDE_EDITORUI_QT_ATTRIBUTEVIEW_H
#pragma once

#include "api.h"

#include "AttributeItem.h"

#include <QtWidgets/QtWidgets>
#include <QVBoxLayout>
#include <Util/smartptr.h>

#include "PanelWidget/PanelWidget.h"
#include "AttributeViewConfig.h"
#include <QMap>
#include "CurveEditorContent.h"
#include "VariableWidgets/QGradientColorDialog.h"
#include "VariableWidgets/QColorSwatchWidget.h"
#include "VariableWidgets/QGradientSwatchWidget.h"
#include "VariableWidgets/QCustomColorDialog.h"

#include <AzCore/std/functional.h>

class CParticleItem;
class CVarBlock;
class QCollapsePanel;
class CMainWindow;
class DefaultViewWidget;
class QDomNode;
class QDomDocument;
struct IParticleEffect;

class EDITOR_QT_UI_API CAttributeView
    : public QDockWidget
{
    Q_OBJECT
public:
    typedef AZStd::function<void()> RefreshCallback;

    CAttributeView(QWidget* parent);
    virtual ~CAttributeView();

    CAttributeViewConfig* GetConfiguration() { return &m_config;  }
    void SetConfiguration(CAttributeViewConfig& configuration, CVarBlock* block);
    void OnStartReload();
    void OnEndReload();

    void ShowDefaultView();

    void HideDefaultView();

    void SetGradientEditorCallback(std::function<void()> callback)
    {
        callback_gradient_editor_start = callback;
    }

    void FocusVar(QString path);

    void ShowFromAttributePath(QString path);

    void SetCallbackEmitterEnableToggle(std::function<void(bool)> callback);

    void EnableGradientEditor()
    {
        if ((bool)callback_gradient_editor_start)
        {
            callback_gradient_editor_start();
        }
    }

    void ResolveVisibility();

    void RestoreScrollPosition();

    void Clear();
    void InitConfiguration(const QString& itemPath);
    void FillFromConfiguration();
    void SetConfigFromFile(QString filename);
    CAttributeViewConfig* CreateConfigFromFile(QString filename);
    void CreateDefaultConfigFile(CVarBlock* block = nullptr);
    void BuildDefaultUI();
    void ExportPanel(QDockWidget* panel, const QString filePath);
    void ImportPanel(const QString filePath, bool isFilePath = true);
    void WriteToFile(const QString& itemPath);
    void WriteToBuffer(QByteArray& out);
    void WriteAttributeItem(CAttributeItem* item, QDomNode* parent, QDomDocument* doc);
    void CreateNewCustomPanel();
    void AddToPresetList(QString filepath);
    QDockWidget* ShowEmptyCustomPanel(QString panelname, QString groupvisibility);

    void UpdateLogicalChildren(IVariable* var);
    void InsertChildren(CAttributeItem* item);

    void BuildCollapseUncollapseAllMenu();

    void updateCallbacks(CAttributeItem* item = NULL);

    void setValue(const QString& key, const QString& value);
    QString getValue(const QString& key, const QString& defaultValue);

    void openCollapseMenu(const QPoint& pos);
    void addToCollapseContextMenu(QAction* action);


    bool hasPanels() const { return m_panels.isEmpty() == false; }

    void showAdvanced(bool show);
    void copyItem(const CAttributeItem* item, bool bRecursively);

    bool isVariableIgnored(QString name);
    IVariable* getVariableFromName(QString path);
    void pasteItem(CAttributeItem* attribute);
    void removeAllParams(QDockWidget* panel);
    bool checkPasteItem(CAttributeItem* attribute); //returns true if pasteItem will paste something

    //Get variable settings infos
    QString GetConfigItemName(QString as);
    QString GetConfigItemVisibility(QString as);
    QString GetConfigItemCallback(QString as);
    std::vector<CAttributeViewConfig::config::relation>  GetConfigItemRelations(QString as);
    QString GetConfigGroupVisibility(QString as);
    const CAttributeViewConfig* GetDefaultConfig();
    CAttributeViewConfig::config::group GetConfigGroup(QString name, CAttributeViewConfig::config::group& group, bool& found);

    CAttributeViewConfig::config::item GetConfigItem(QString as, CAttributeViewConfig::config::group& group, bool& found);
    CAttributeViewConfig::config::item GetConfigItem(QString as, CAttributeViewConfig::config::item& group, bool& found);

    QVector<CAttributeGroup*> GetSelectedGroups();
    void ClearAttributeSelection();
    void AddSelectedAttribute(CAttributeGroup* group);
    void AddDragAttribute(CAttributeGroup* group);

    CAttributeItem* findItemByPath(const QString& name);
    QVector<CAttributeItem*> findItemsByVar(IVariable* var);
    //Return the first attribute item found by var
    CAttributeItem* findItemByVar(IVariable* var);
    const QString& getCurrentLibraryName() const;

    void setRefreshCallback(const RefreshCallback& refreshCallback);

    void AddLayoutMenu(QMenu* menu);
    PanelWidget* getPanelWidget() { return m_scrollAreaWidget; }

    QMap<QString, IVariable*>& getConfigVariableMap() { return m_configVariableMap; }
    QString GetVariablePath(IVariable* var);
    QString GetVariablePath(const IVariable* var);
    IVariable* GetVarFromPath(QString path);

    //TODO create gradient stops
    void hideChildren();
    void showChildren();

    void ShowAllButGroup(const QString groupVar);
    void HideAllButGroup(const QString groupVar);

    bool LoadAttributeXml(const QByteArray& data);
    void ResetToDefaultLayout();

    bool LoadAttributeConfig(QByteArray data);
    void SaveAttributeConfig(QByteArray& out);

    void LoadAttributeConfig(QString filename);
    void SaveAttributeConfig(QString filename);

    bool RenamePanel(QDockWidget* panel, QString origName, QString newName);
    void ClosePanel(QDockWidget* panel);

    bool IsMultiSelection(){ return m_bMultiSelectionOn; }

    void ZoomToAttribute(QPoint pos);

    //used to assign appropriate default emitter
    void SetCurrentItem(CParticleItem* currentSelectedItem);
    CParticleItem* GetCurrentItem();

    //use to validate an enum value change. return false if validation failed
    bool ValidateUserChangeEnum(IVariable* var, const QString& newText);

    //view will handle some special enum changes. return true if it's handled
    bool HandleUserChangedEnum(IVariable* var, const QString& newText);

    //to emit a undo point signal to main window
    void OnAttributeItemUndoPoint();

signals:
    void SignalRefreshAttributeView();
    void SignalRenamePanel(PanelTitleBar* title, QString name);
    void SignalBuildCustomAttributeList(QMenu* menu);
    void SignalAddAttributePreset(QString panelName, QString data);
    void SignalResetPresetList();
    void SignalVarialbeUpdateHighlight(bool ishighlight);
    void SignalIgnoreAttributeRefresh(bool ignored);
    void SignalRefreshAttributes();
    void SignalGetCurrentTabName(QString& itemFullName);
    void SignalItemUndoPoint(const QString& itemFullName);

private:
    void DecorateDefaultView();
    virtual void paintEvent(QPaintEvent* event) override;
    virtual void keyPressEvent(QKeyEvent* event) override;
    virtual void keyReleaseEvent(QKeyEvent* event);

    QGroupBox* CreateGroup(const QString& title);
    void SetEnabledFromPath(QString path, bool enable);
    void RecurseSetProperty(QString prop, QString value, CAttributeItem::AttributeList start);
    bool PanelExist(QString name);
    std::function<void()> callback_gradient_editor_start;
    std::function<void(bool)> m_callbackEmitterEnableToggled;

    void CorrectLayout(QByteArray& data);

    //warning user emitter shape change will overwrite parameters.
    bool ValidateEmitterShapeChange();

    //check whether child and parent emitter shape matching with the inheritance
    bool ValidateInheritance(const QString& inheritance);

    //check whether we can copy varSrc to varDst
    //this function should only be used for checking copy an AttributeItem's IVariable's clone to an AttributeItem's IVariable
    bool CanCopyVariable(IVariable* varSrc, IVariable* varDst);

    //Copy an CAttributeItem's ( w/ or w/o its children's) variable to input list variableList
    void CopyItemVariables(const CAttributeItem* item, bool recursive, /*out*/ QVector<TSmartPtr<IVariable>> & variableList);

    //Save an CAttributeItem's ( w/ or w/o its children's) variable's pointer to input list variableList and item list if it's provided
    void GetItemVariables(CAttributeItem* item, bool recursive, /*out*/ QVector<IVariable*> & variableList, /*out*/ QVector<CAttributeItem*>* itemList);

    //Set maximum particle count for the input widgets
    void SetMaxParticleCount(int maxCount);
private:
    CAttributeViewConfig m_config;
    CAttributeViewConfig m_defaultConfig;
    QVector<CAttributeViewConfig*> m_importedConfigs;
    QMap<QString, IVariable*> m_configVariableMap;
    QMap<QString, bool> m_variableIgnoreMap;

    DefaultViewWidget* m_defaultView;

    QGradientSwatchWidget m_gradientSwatch_1;
    QGradientSwatchWidget m_gradientSwatch_2;
    QColorSwatchWidget m_colorSwatch_1;
    QColorSwatchWidget m_colorSwatch_2;

    QScrollArea* m_scrollArea;

    PanelWidget* m_scrollAreaWidget;

    CAttributeItem::AttributeList m_children;
    CAttributeItem::AttributeList m_defaultChildren;
    int m_fillCount;

    typedef QMap<QString, QString> ValueMap;
    ValueMap m_valueMap;
    QString m_currLibraryName;

    QMenu* m_contextMenuCollapse;
    QVector<QAction*> m_contextMenuCollapseSinglePanelActions;
    QVector<QWidget*> m_panels;
    QVector<QString> m_custompanels;
    QVector<CAttributeGroup*> m_selectedGroups;

    bool m_isReloading;
    RefreshCallback m_refreshCallback;

    QVector<TSmartPtr<IVariable>> m_variableCopyList;

    QByteArray m_originalLayout;
    CVarBlock* m_lastVarBlock;
    CVarBlock* m_uiVars;
    QString m_lastItemPath;
    CParticleItem* m_currentItem;

    int  m_attributeViewStatus; // 0 : show emitter attributes
                                // 1 : show group attributes
    bool m_bMultiSelectionOn;
    bool m_updateLogicalChildrenFence;

};


#endif // CRYINCLUDE_EDITORUI_QT_ATTRIBUTEVIEW_H
