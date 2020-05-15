#ifndef OBJECT_SELECTOR_MODEL_H
#define OBJECT_SELECTOR_MODEL_H

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
#include <QAbstractTableModel>
#include <vector>

class CBaseObject;

#define UNIX_SLASH "/"
#define WIN_SLASH "\\"

class ObjectSelectorModel
    : public QAbstractTableModel
{
    Q_OBJECT
public:
    typedef std::map<QString, int> TGeometryCountMap;
    typedef std::map<CBaseObjectPtr, QString> ObjToStrMap;

    enum Column
    {
        NameColumn,
        SelectedColumn,
        TypeColumn,
        LayerColumn,
        DefaultMaterialColumn,
        CustomMaterialColumn,
        BreakabilityColumn,
        SmartObjectColumn,
        TrackViewColumn,
        GeometryColumn,
        InstancesInLevel,
        LODsColumn,
        SpecColumn,
        AIGroupColumn,
        NumberOfColumns
    };

    enum Role
    {
        ObjectRole = Qt::UserRole,
        IdentationRole
    };

    enum DisplayMode
    {
        DisplayModeVisible = 0,
        DisplayModeHidden,
        DisplayModeFrozen,
    };

    enum FirstColumnImage
    {
        LIST_BITMAP_ANY,
        LIST_BITMAP_ENTITY,
        LIST_BITMAP_BRUSH,
        LIST_BITMAP_TAGPOINT,
        LIST_BITMAP_PATH,
        LIST_BITMAP_VOLUME,
        LIST_BITMAP_GROUP = 7
    };

    explicit ObjectSelectorModel(QObject* parent = nullptr);
    ~ObjectSelectorModel();
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    static QVariant data(CBaseObject*, int role, int col);
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    static QStringList ColumnNames();
    void SetObjectTypeMask(int mask);
    void SetDisplayMode(ObjectSelectorModel::DisplayMode);
    ObjectSelectorModel::DisplayMode GetDisplayMode() const;
    void SetFilterText(const QString& text);
    void SetProperties(const QString& propertyText);
    void SetMatchPropertyName(bool match);
    void SetIsLinkTool(bool);
    void SetSearchInsideObjects(bool);
    bool GetSearchInsideObjects() const;
    std::vector<CBaseObject*> GetObjects() const;
    void Reload(bool rebuildMaps = false);
    void EnableTreeMode(bool);
    bool IsTreeMode() const;

    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

    static int GetAIGroupID(CBaseObject* obj);
    static QString GetObjectName(CBaseObject* pObject);
    static TGeometryCountMap m_mGeomCountMap;
    static ObjToStrMap s_trackViewMap;

    void EmitDataChanged(const QModelIndex& index);
    void EmitDataChanged(CBaseObject* obj);
    void EmitDataChanged();
    void SetTrackViewModified(bool modified);
    bool IsTrackViewModified() const;
    void BuildMaps();
    bool AddObject(CBaseObject* obj, int level);
    void RemoveObject(CBaseObject* obj);
    void Clear();
    int IndexOfObject(CBaseObject* obj) const;

Q_SIGNALS:
    void countChanged();

private:
    void UpdateGeomCount();
    bool AcceptsObject(CBaseObject* obj) const;
    void AddObjectRecursively(CBaseObject* obj, int level);
    void AddObjectToMaps(CBaseObject* pObject);
    std::vector<CBaseObject*> m_objects;
    std::vector<int> m_indentState;
    bool IsPropertyMatchVariable(IVariable* pVar) const;
    bool IsPropertyMatch(CBaseObject* pObject) const;
    int m_objectTypeMask; // bit mask of ObjectType
    ObjectSelectorModel::DisplayMode m_displayMode;
    ObjectSelectorModel* m_sourceModel = nullptr;
    QString m_filterText;
    QString m_propertyFilter;
    bool m_matchPropertyName; // Match name or value
    bool m_bIsLinkTool = false;
    bool m_bSearchInsideObjects = false;
    bool m_treeModeEnabled = false;
    bool m_bTrackViewModified = false;
};

#endif
