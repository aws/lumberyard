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
//#include "api.h"

//Editor
#include <Util/Variable.h>

//EditorUI_QT
#include <../EditorUI_QT/FloatableDockPanel.h>


class DockWidgetTitleBar;
class QMenu;
class QGridLayout;
class QDoubleSpinBox;
class QPushButton;
class QGridLayout;
class CAttributeView;
class CParticleUIDefinition;
class FluidTabBar;
class CParticleItem;
class ContextMenu;
class DefaultViewWidget;
class CLibraryTreeViewItem;
struct IVariableContainer;
class QHBoxLayout;
class QScrollArea;
class DockableLibraryPanel;
class QCheckBox;
struct SLodInfo;
class LODLevelWidget;

class LodWidget
    : public QWidget
{
    Q_OBJECT
public:
    LodWidget(QWidget* parent);
    ~LodWidget();

    void Init();

    void AddLODLevelWidget(SLodInfo* lod);
    void RemoveLODLevelWidget(SLodInfo* lod);

    void ClearGUI();
    void RefreshGUI(CParticleItem* item, int selectedLevel = -1);
    void RefreshCurrentGUI();

    void AddLodLevel();
    void RemoveSelectedLevel();
    void RemoveAllLevels();

    void MoveSelectedLevelUp();
    void MoveSelectedLevelDown();
    void MoveSelectedToTop();
    void MoveSelectedToBottom();

    void SelectFirstLevel();
    void SelectLastLevel();
    void updateTreeItemHighlight(bool ishighlight, SLodInfo* lod, QString fullname);

    void RefreshLodPanels();

    void ClearLODSelection();

    void OnContentSizeChanged();



    /*!
    *  Select certain particle in certain lod level
    *  index: the order of the selected lod. -1 means no lod selected
    *  itemName: the selected particle's full name
    */
    void SelectLevel(int lodIdx, const QString& itemName);

    //QT
signals:
    void SignalAddLod(CParticleItem* item);
    void SignalLodItemSelected(IParticleEffect* baseParticleEffect, SLodInfo* lod);
    void SignalLodIconRemoved(QString particleFullName);
    void SignalUpdateLODIcon();
    void SignalAttributeViewItemDeleted(const QString& name);
    void SignalLodUndoPoint();

private slots:
    void BlendInValueChanged(double value);
    void BlendOutValueChanged(double value);
    void OverlapValueChanged(double value);
    void AddLodLevelSlot();
    void RemoveLodLevel(LODLevelWidget* widget, SLodInfo* lod);
    void OnLevelDistanceChanged();
    void UpdateLODIcon(CLibraryTreeViewItem* item);

    void onLodParticleItemSelected(LODLevelWidget* lodLevel, IParticleEffect* baseParticle, SLodInfo* lod);


private:    //Variables
    CParticleItem* m_Item;

    QVBoxLayout* m_Layout;

    // Top widget
    QWidget* m_TopWidget;
    QGridLayout* m_TopLayout;

    QLabel* m_BlendInLabel;
    QLabel* m_BlendOutLabel;
    QLabel* m_OverlapLabel;

    QDoubleSpinBox* m_BlendInBox;
    QDoubleSpinBox* m_BlendOutBox;
    QDoubleSpinBox* m_OverlapBox;

    //Add LoD button
    QPushButton* m_AddLevelOfDetailButton;

    //LoD level panels
    QWidget* m_LodLevelPanel;
    QVBoxLayout* m_LodLevelLayout;
    QScrollArea* m_LodLevelScroll;
    QVector<LODLevelWidget*> m_LodLevels;
    int m_SelectedLodLevel;
};



class /*EDITOR_QT_UI_API*/ DockableLODPanel
    : public FloatableDockPanel
{
    Q_OBJECT
public:
    DockableLODPanel(QWidget* parent);
    ~DockableLODPanel();

    void Init(const QString& panelName);

    void LoadSessionState();
    void SaveSessionState();

    enum class ParamShortcuts
    {
        InsertComment = 0,
        EnableDisableToggle,
    };

    enum class TitleMenuActions
    {
        AddLevel = 0,
        MoveUp,
        MoveDown,
        MoveToTop,
        MoveToBottom,
        JumpToFirst,
        JumpToLast,
        Remove,
        RemoveAll,
    };

    void PerformTitleMenuAction(TitleMenuActions action);

    void SelectLod(const SLodInfo* lod);
    
    void UpdateColors(const QMap<QString, QColor>& colorMap);

    void HideDefaultView();
    void ShowDefaultView();
    void RefreshGUI();

signals:
    void SignalPopulateTitleBarMenu(QMenu* toPopulate);
    void SignalChangeLODIcon(QString libraryName, QString itemName);
    void SignalLodItemSelectionChanged(CBaseLibraryItem* baseParticle, SLodInfo* lod);
    void SignalAttributeViewItemDeleted(const QString& name);

    ///////////////////////////////////////////////
public slots:
    void OnAddLod(CLibraryTreeViewItem* item);
    void OnAddLod(CParticleItem* item);
    void ItemSelectionChanged(CBaseLibraryItem* item);
    void onLodItemSelectionChanged(IParticleEffect* baseParticleEffect, SLodInfo* lod);
    void updateTreeItemHighLight(bool ishighlight, SLodInfo* lod, QString name);
    void UpdateLODIcon();
    void OnAttributeViewItemDeleted(const QString& name);
    void LibraryItemReselected(CBaseLibraryItem* item);
    void OnLodUndoPoint();

private: //Functions
    QMenu* GetTitleBarMenu();

    void DecorateDefaultView();
    void BuildGUI();
    void RefreshGUI(CParticleItem* item);
    void RemoveLodIcon(QString fullname);

    void SetParticle(CParticleItem* item);
    void ClearParticle();

    void EmitIconChangeRecursive(CLibraryTreeViewItem* item);
    void EmitIconChangeRecursive(QString library, IParticleEffect* item);
    void EmitIconChangeRecursive(CParticleItem* item);
    
    void CollectItemRecursive(CBaseLibraryItem* rootItem, AZStd::list<AZStd::string>& itemIds);
    void AddLodUndoPoint(CParticleItem* item);


private: //Variables
    CBaseLibraryItem* m_currentSelectedItem;
    CParticleItem* m_selectedParticle;
    QColor m_enabledTextColor;
    QColor m_disabledTextColor;

    DockWidgetTitleBar* m_titleBar;
    QMenu* m_titleBarMenu;

    DefaultViewWidget* m_defaultView;

    QVBoxLayout* m_layout;
    QWidget* m_centralWidget;
private: //UI

    LodWidget* m_LodWidget;

    bool m_IgnoreAttributeRefresh;
};
