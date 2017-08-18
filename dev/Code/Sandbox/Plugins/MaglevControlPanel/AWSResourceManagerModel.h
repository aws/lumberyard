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

#include <QSharedPointer>
#include <QVariantList>
#include <QVariantMap>
#include <QVariant>
#include <QMap>
#include <QSet>
#include <QDebug>
#include <QModelIndex>
#include <QStandardItem>

#include <AzCore/std/functional.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/containers/vector.h>

#include "AWSResourceManager.h"

///////////////////////////////////////////////////////////////////////////////

// ColumnEnumToNameMap defines static map from column enum to property names.
// Classes that extend AWSResourceManagerModel will need to provide a
// definition for the declared s_columnEnumToNameMap field.

template<class ColumnEnumType>
class ColumnEnumToNameMap
{
public:

    typedef ColumnEnumType EnumType;

    static int GetColumnCount()
    {
        auto keys = s_columnEnumToNameMap.keys();

        if (keys.isEmpty())
        {
            return 0;
        }

        int max = 0;
        for (auto it = keys.constBegin(); it != keys.constEnd(); ++it)
        {
            int value = static_cast<int>(*it);
            if (value > max)
            {
                max = value;
            }
        }

        return max + 1;
    }

    static QString GetColumnName(ColumnEnumType column)
    {
        return s_columnEnumToNameMap[column];
    }

    static QString GetColumnName(int column)
    {
        return GetColumnName(static_cast<ColumnEnumType>(column));
    }

    static bool Contains(int column)
    {
        return Contains(static_cast<ColumnEnumType>(column));
    }

    static bool Contains(ColumnEnumType column)
    {
        return s_columnEnumToNameMap.contains(column);
    }

private:

    static const QMap<ColumnEnumType, QString> s_columnEnumToNameMap;
};


///////////////////////////////////////////////////////////////////////////////

// AWSResourceManagerModel implements a QAbstractItemModel over 
// QVariantList, QVariantMap, and QVariant data.
//
// To use, define a class that extends this class and override the
// ProcessPythonOutput method. ModelInterfaceType should extend
// IAWSResourceManagerModel.

template<class ModelInterfaceType>
class AWSResourceManagerModel
    : public ModelInterfaceType
{

private:

    AWSResourceManager* m_resourceManager;

    typedef AWSResourceManagerModel<ModelInterfaceType> Self;
    typedef ColumnEnumToNameMap<typename ModelInterfaceType::ColumnEnum> ColumnMap;

protected:

    // Constructors

    AWSResourceManagerModel(AWSResourceManager* resourceManager)
        : m_resourceManager{resourceManager}
    {
    }

    // Command Execution

    void ExecuteAsync(AWSResourceManager::RequestId requestId, const char* command, const QVariantMap& args = QVariantMap{}) const
    {
        ResourceManager()->ExecuteAsync(requestId, command, args);
    }

    void ExecuteAsync(AWSResourceManager::ExecuteAsyncCallback callback, const char* command, const QVariantMap& args = QVariantMap{}) const
    {
        ResourceManager()->ExecuteAsync(callback, command, args);
    }

    // Content

    static int ColumnCount()
    {
        static int s_columnCount = ColumnMap::GetColumnCount();
        return s_columnCount;
    }

    virtual int CompareRowItems(QStandardItem* a, int aRow, QStandardItem* b, int bRow)
    {
        auto aItem = a->child(aRow, 0);
        auto bItem = b->child(bRow, 0);
        return aItem->data(Qt::DisplayRole).compare(bItem->data(Qt::DisplayRole));
    }

    virtual void UpdateChildItem(typename ModelInterfaceType::ColumnEnum column, QStandardItem* sourceChildItem, QStandardItem* targetChildItem)
    {
        targetChildItem->setData(sourceChildItem->data(Qt::DisplayRole), Qt::DisplayRole);
    }

    void UpdateItems(const QVariantList& sourceList)
    {
        auto sourceItem = MakeItemFromList(sourceList, [this](const QVariant& value){ return MakeRowItemsFromMap(value); });
        UpdateItemChildren(sourceItem.data(), invisibleRootItem());
    }

    void UpdateItemChildren(QStandardItem* sourceItem, QStandardItem* targetItem)
    {
        //qDebug() << "UpdatingItemChildren";

        int sourceRow = 0;
        int targetRow = 0;

        while (targetRow < targetItem->rowCount() || sourceRow < sourceItem->rowCount())
        {
            //qDebug() << "sourceRow" << sourceRow << "of" << sourceItem->rowCount();
            //qDebug() << "targetRow" << targetRow << "of" << targetItem->rowCount();

            if (targetRow >= targetItem->rowCount())
            {
                // append to targetItem
                //qDebug() << "append" << sourceItem->child(sourceRow, 0)->data(Qt::DisplayRole) << " at " << targetRow;
                beginInsertRows(targetItem->index(), targetRow, targetRow);
                targetItem->insertRow(targetRow, sourceItem->takeRow(sourceRow));
                endInsertRows();
                ++targetRow;
            }
            else if (sourceRow >= sourceItem->rowCount())
            {
                // remove from targetItem
                //qDebug() << "remove" << targetItem->child(targetRow, 0)->data(Qt::DisplayRole) << " at " << targetRow;
                beginRemoveRows(targetItem->index(), targetRow, targetRow);
                targetItem->removeRow(targetRow);
                endRemoveRows();
            }
            else
            {
                int comparision = CompareRowItems(sourceItem, sourceRow, targetItem, targetRow);
                if (comparision < 0)
                {
                    // insert in targetItem
                    //qDebug() << "insert" << sourceItem->child(sourceRow, 0)->data(Qt::DisplayRole) << " at " << targetRow;
                    beginInsertRows(targetItem->index(), targetRow, targetRow);
                    targetItem->insertRow(targetRow, sourceItem->takeRow(sourceRow));
                    endInsertRows();
                    ++targetRow;
                }
                else if (comparision > 0)
                {
                    // remove targetItem
                    //qDebug() << "remove" << targetItem->child(targetRow, 0)->data(Qt::DisplayRole) << " at " << targetRow;
                    beginRemoveRows(targetItem->index(), targetRow, targetRow);
                    targetItem->removeRow(targetRow);
                    endRemoveRows();
                }
                else
                {
                    // update targetItem
                    //qDebug() << "update" << sourceItem->child(sourceRow, 0)->data(Qt::DisplayRole) << " at " << targetRow;
                    for (int column = 0; column < ColumnCount(); ++column)
                    {
                        auto targetChildItem = targetItem->child(targetRow, column);
                        auto sourceChildItem = sourceItem->child(sourceRow, column);

                        //qDebug() << "column" << column << "target data" << targetChildItem->data(Qt::DisplayRole) << "source data" << sourceChildItem->data(Qt::DisplayRole);

                        if (targetChildItem != sourceChildItem)
                        {
                            //qDebug() << "update";
                            UpdateChildItem(intToColumnEnum(column), sourceChildItem, targetChildItem);
                        }

                        if (targetChildItem->hasChildren() || sourceChildItem->hasChildren())
                        {
                            //qDebug() << "has children";
                            UpdateItemChildren(sourceChildItem, targetChildItem);
                        }
                    }

                    ++sourceRow;
                    ++targetRow;
                }
            }
        }
    }

    typedef AZStd::function<QList<QStandardItem*>(const QVariant& value)> MakeRowItemsFunction;

    QSharedPointer<QStandardItem> MakeItemFromList(const QVariant& value, MakeRowItemsFunction makeRowItems)
    {
        return MakeItemFromList(value.toList(), makeRowItems);
    }

    QSharedPointer<QStandardItem> MakeItemFromList(const QVariantList& list, MakeRowItemsFunction makeRowItems)
    {
        QSharedPointer<QStandardItem> item {
            new QStandardItem {}
        };
        for (auto it = list.constBegin(); it != list.constEnd(); ++it)
        {
            item->appendRow(makeRowItems(*it));
        }
        return item;
    }

    QList<QStandardItem*> MakeEmptyRowItems()
    {
        QList<QStandardItem*> items;
        for (int column = 0; column < ColumnCount(); ++column)
        {
            items.append(new QStandardItem());
        }
        return items;
    }

    QList<QStandardItem*> MakeRowItemsFromMap(const QVariant& value)
    {
        QVariantMap map = value.toMap();

        QList<QStandardItem*> items = MakeEmptyRowItems();

        //qDebug() << "MakeRowItemsFromMap" << toDebugString(map);

        for (int column = 0; column < ColumnCount(); ++column)
        {
            if (ColumnMap::Contains(column))
            {
                items[column]->setData(GetProperty(map, column), Qt::DisplayRole);
                //qDebug() << "column" << column << "data" << items[column]->data(Qt::DisplayRole);
            }
        }

        return items;
    }

    int FindRow(typename ModelInterfaceType::ColumnEnum column, const QVariant& value, int role = Qt::DisplayRole) const
    {
        return FindRow(invisibleRootItem(), column, value, role);
    }

    int FindRow(QStandardItem* parent, typename ModelInterfaceType::ColumnEnum column, const QVariant& value, int role = Qt::DisplayRole) const
    {
        for (int row = 0; row < parent->rowCount(); ++row)
        {
            if (parent->child(row, columnEnumToInt(column))->data(role) == value)
            {
                return row;
            }
        }
        return -1;
    }

    QStandardItem* GetChild(QStandardItem* parent, int row, typename ModelInterfaceType::ColumnEnum column) const
    {
        return parent->child(row, columnEnumToInt(column));
    }

    // Get Property

    static QVariant GetProperty(const QVariant& variant, int column)
    {
        return GetProperty(variant.toMap(), ColumnMap::GetColumnName(column));
    }

    static QVariant GetProperty(const QVariant& variant, const QString& name)
    {
        return GetProperty(variant.toMap(), name);
    }

    static QVariant GetProperty(const QVariantMap& map, int column)
    {
        return GetProperty(map, ColumnMap::GetColumnName(column));
    }

    static QVariant GetProperty(const QVariant& map, typename ModelInterfaceType::ColumnEnum column)
    {
        return GetProperty(map.toMap(), ColumnMap::GetColumnName(column));
    }

    static QVariant GetProperty(const QVariantMap& map, typename ModelInterfaceType::ColumnEnum column)
    {
        return GetProperty(map, ColumnMap::GetColumnName(column));
    }

    static QVariant GetProperty(const QVariantMap& map, const QString& name)
    {
        return map[name];
    }

    // Debug

    void Dump(QStandardItem* item, const QString& indent)
    {
        for (int row = 0; row < item->rowCount(); ++row)
        {
            //qDebug() << indent << "row" << row << ":";
            for (int column = 0; column < item->columnCount(); ++column)
            {
                auto child = item->child(row, column);
                //qDebug() << indent << "  column" << column << ":" << child->data(Qt::DisplayRole);
                if (child->hasChildren())
                {
                    Dump(child, indent + "  ");
                }
            }
        }
    }

    void Dump(const char* title)
    {
        //qDebug() << title << "with" << rowCount() << "rows and" << columnCount() << "columns:";
        Dump(invisibleRootItem(), QString {"  "});
    }

public:

    // Sorting

    static void Sort(QVariantList& list, typename ModelInterfaceType::ColumnEnum column)
    {
        Sort(list, ColumnMap::GetColumnName(column));
    }

    static void Sort(QVariantList& list, const QString& column)
    {
        auto compare = [column](const QVariant& v1, const QVariant& v2)
            {
                return GetProperty(v1, column) < GetProperty(v2, column);
            };

        qSort(list.begin(), list.end(), compare);
    }

    // State

    AWSResourceManager* ResourceManager() const { return m_resourceManager; }

};

