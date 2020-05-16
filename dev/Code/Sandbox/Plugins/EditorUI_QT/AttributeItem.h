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

#ifndef CRYINCLUDE_EDITORUI_QT_ATTRIBUTEITEM_H
#define CRYINCLUDE_EDITORUI_QT_ATTRIBUTEITEM_H
#pragma once

#include "api.h"

#include <QtWidgets>
#include <QVBoxLayout>
#include <QVector>
#include <QMap>
#include <functional>
#include "AttributeViewConfig.h"
#include "AttributeItemLogicCallbacks.h"

class CParticleItem;
class CVarBlock;
struct IVariable;
class QCollapseWidget;
class CAttributeView;
class CAttributeGroup;

namespace Prop
{
    struct Description;
}

#define RECORD_UNDO(varPtr) CUndo __scopedUndo(QString().sprintf("Variable change: %s", varPtr->GetName()).toUtf8().data())

class EDITOR_QT_UI_API CAttributeItem
    : public QWidget
{
    Q_OBJECT
public:
    typedef QVector<CAttributeItem*> AttributeList;
    typedef std::function<void()> Callback;
    typedef QVector<Callback> CallbackList;
    // The AttributeItem for nodePath == 1
    CAttributeItem(QWidget* parent, CAttributeView* attributeView, IVariable* var, const QString& attributePath, const CAttributeViewConfig::config::group* configGroup);

    CAttributeItem(QWidget* parent, CAttributeView* attributeView,
        IVariable* var, const QString& attributePath, unsigned int nodeDepth, const CAttributeViewConfig::config::group* configGroup, const CAttributeViewConfig::config::item* configItem);
    //Create Defautl AttributeItem. The default attributeItem will have a label shows message
    CAttributeItem(QWidget* parent, CAttributeView* attributeView, QString message);
    virtual ~CAttributeItem();

    IVariable* getVar();
    const IVariable* getVar() const;
    QString getVarName() const;
    CAttributeView* getAttributeView();
    void setOverrideVarName(QString varName);

    void TriggerUpdateCallback(){m_uiLogicUpdateCallback(this); }

    void ResolveVisibility(const QVector<QString>& visibilityVars);
    QString GetVisibilityValues();

    bool isDefaultValue();
    //! reset var to default value.  
    void ResetValueToDefault();
    bool showAdvanced(bool showAdvanced);
    bool isAdvanced(){return m_advanced; }
    bool hasAdvancedChildren(){return m_hasAdvancedChildren || isAdvanced(); }

    bool isChildOf(CAttributeItem* item) const;
    bool hasXMLConfigItem() const{return m_configItem != nullptr; }
    void ForEachRelation(std::function<bool(QString slot, QString dst)> func);
    // Get relations for src == current item
    QMap<QString, QString> GetRelations();
    // Get All relations
    const QMultiMap<QString, QPair<QString, QString> > GetVariableRelations();

    void addCallback(Callback cb);
    void updateCallbacks(const bool& recursive);

    QString getAttributePath(const bool& withLibraryName) const;

    const AttributeList& getChildren() const;
    void ForEachChild(std::function<void(CAttributeItem* child)> func);

    CAttributeItem* findItemByPath(const QString& path);
    CAttributeItem* findItemByVar(IVariable* var);

    void UILogicUpdateCallback(){m_uiLogicUpdateCallback(this); }//This will be called EVERY tick the controll is changed,
                                                                 // not just on /release/ events. (Implementation WIP)
    void disable();
    void enable();

    // Delete all child items and reset to default view
    void resetItem();

    bool isGroup();
    QString getUpdateCallbacks();
    bool hasCallback();
    // Types
    QCollapseWidget* CreateDragableGroups(const CAttributeViewConfig::config::group* grp);
    // Insert dragable parameter to custom panel
    bool InsertDragableGroup(const CAttributeViewConfig::config::item* item, const CAttributeViewConfig::config::group* grp, QWidget* insertWidget = nullptr, int index = 0);
    void HideDefault()
    {
        if (m_defaultLabel)
        {
            m_defaultLabel->hide();
        }
    }
    void ShowDefault()
    {
        if (m_defaultLabel)
        {
            m_defaultLabel->show();
        }
    }

    void RemoveDragableGroup(CAttributeGroup* group);
    QWidget* GetWidget(){ return m_widget; }

    // Undo
    void ZoomToAttribute(QPoint pos);
    void GetEmitterPath(QString& fullname);

signals:
    void SignalResetToDefault();
    void SignalEnumChanged();
    void SignalUndoPoint();

private:
    // Utility functions
    QString getVarName(IVariable* var, bool addDots);
    void addToLayout(const Prop::Description* desc, IVariable* var, QWidget* widget);


    // Generate child attributes from variable or group.
    void CreateChildAttributes(QWidget* parent, QVBoxLayout* layout, IVariable* var, const CAttributeViewConfig::config::group* grp);

    void CreateChildAttributes_ConfigGroup(const CAttributeViewConfig::config::group* grp, QWidget* parent, QVBoxLayout* layout);

    // Iterates over child variables and creates recursively new CAttributeItems
    void CreateChildAttributes_Variable(IVariable* pVar, QWidget* parent, QVBoxLayout* layout);

    // Main function which constructs a data type
    // Internally, its using other simpler types
    void CreateDataType(IVariable* var);

    // Types
    QCollapseWidget* CreateCategory(const Prop::Description* desc, IVariable* var, const CAttributeViewConfig::config::group* grp);
    void CreateString(const Prop::Description* desc, IVariable* var);
    void CreateInt(const Prop::Description* desc, IVariable* var);
    void CreateBool(const Prop::Description* desc, IVariable* var);
    void CreateFloat(const Prop::Description* desc, IVariable* var);
    void CreateVector2(const Prop::Description* desc, IVariable* var);
    void CreateVector3(const Prop::Description* desc, IVariable* var);
    void CreateVector4(const Prop::Description* desc, IVariable* var);
    void CreateEnum(const Prop::Description* desc, IVariable* var);

    void CreateCurveFloat(const Prop::Description* desc, IVariable* var);
    void CreateCurveColor(const Prop::Description* desc, IVariable* var);
    void CreateColor(const Prop::Description* desc, IVariable* var);
    void CreateSelectResource(const Prop::Description* desc, IVariable* var);

    virtual bool eventFilter(QObject* o, QEvent* e) override;
    virtual void paintEvent(QPaintEvent*) override;

    //Helper function for resolveVisiblity
    void CompareVisibility(bool& isVisible, const QString compareKey, QStringList listWithSupportedItems, QString ignoredKey = "all");
    void CheckVisibility(QStringList visibilityList, bool& isVisible);

private:
    QVBoxLayout* m_layout;
    IVariable* m_var;
    unsigned int m_nodeDepth;
    QLabel* m_defaultLabel;
    QWidget* m_widget;
    CAttributeView* m_attributeView;
    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    AttributeList m_children;
    CallbackList m_callbacks;
    QString m_attributePath;
    QString m_updatecallbacks;

    const CAttributeViewConfig::config::group* m_configGroup;
    QString m_overrideVarName;
    const CAttributeViewConfig::config::item* m_configItem;
    bool m_advanced;
    bool m_hasAdvancedChildren;
    bool m_isGroup;

    AttributeItemLogicCallback m_uiLogicUpdateCallback;

    QMultiMap<QString, QPair<QString, QString> > m_VariableRelations;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
};

#endif // CRYINCLUDE_EDITORUI_QT_ATTRIBUTEITEM_H
