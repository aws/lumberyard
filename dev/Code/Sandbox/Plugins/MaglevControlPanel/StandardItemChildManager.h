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

#include <QDebug>

template<typename ParentType, typename ChildType, typename SourceType = QVariant>
class StandardItemChildManager
{
public:

    StandardItemChildManager(ParentType* parentItem)
        : m_parentItem(parentItem)
    {
    }

    void UpdateChildren(const QList<SourceType>& sourceList)
    {
        // qDebug() << "UpdateChildren";

        int sourceRow = 0;
        int targetRow = 0;

        while (targetRow < m_parentItem->rowCount() || sourceRow < sourceList.size())
        {
            // qDebug() << "sourceRow" << sourceRow << "of" << sourceList.size();
            // qDebug() << "targetRow" << targetRow << "of" << m_parentItem->rowCount();

            if (targetRow >= m_parentItem->rowCount())
            {
                // append child
                // qDebug() << "append" << sourceList[sourceRow] << " at " << targetRow;
                auto newChild = Create(sourceList[sourceRow]);
                m_parentItem->insertRow(targetRow, newChild);
                ChildAdded(newChild);
                ++sourceRow;
                ++targetRow;
            }
            else if (sourceRow >= sourceList.size())
            {
                // remove child
                // qDebug() << "remove" << m_parentItem->child(targetRow)->text() << " at " << targetRow;
                m_parentItem->removeRow(targetRow);
            }
            else
            {
                int comparision = Compare(sourceList[sourceRow], static_cast<ChildType*>(m_parentItem->child(targetRow)));

                if (comparision < 0)
                {
                    // insert child
                    // qDebug() << "insert" << sourceList[sourceRow] << " at " << targetRow;
                    auto newChild = Create(sourceList[sourceRow]);
                    m_parentItem->insertRow(targetRow, newChild);
                    ChildAdded(newChild);
                    ++targetRow;
                    ++sourceRow;
                }
                else if (comparision > 0)
                {
                    // remove child
                    // qDebug() << "remove" << m_parentItem->child(targetRow)->text() << " at " << targetRow;
                    m_parentItem->removeRow(targetRow);
                }
                else
                {
                    // update child
                    // qDebug() << "update" << m_parentItem->child(targetRow)->text() << "with" << sourceList[sourceRow]  << " at " << targetRow;
                    Update(sourceList[sourceRow], static_cast<ChildType*>(m_parentItem->child(targetRow)));

                    ++sourceRow;
                    ++targetRow;
                }
            }
        }
    }

    void InsertChild(const SourceType& source)
    {
        int targetRow = 0;
        while (targetRow <= m_parentItem->rowCount())
        {
            if (targetRow >= m_parentItem->rowCount())
            {
                // append child
                auto newChild = Create(source);
                m_parentItem->insertRow(targetRow, newChild);
                ChildAdded(newChild);
                break;
            }
            else
            {
                int comparision = Compare(source, static_cast<ChildType*>(m_parentItem->child(targetRow)));

                if (comparision < 0)
                {
                    // insert child
                    auto newChild = Create(source);
                    m_parentItem->insertRow(targetRow, newChild);
                    ChildAdded(newChild);
                    break;
                }
                else if (comparision > 0)
                {
                    // skip child
                    ++targetRow;
                }
                else
                {
                    // update child
                    Update(source, static_cast<ChildType*>(m_parentItem->child(targetRow)));
                    break;
                }
            }
        }
    }

    void RemoveChild(const SourceType& source)
    {
        int targetRow = 0;
        while (targetRow < m_parentItem->rowCount())
        {
            int comparision = Compare(source, static_cast<ChildType*>(m_parentItem->child(targetRow)));

            if (comparision < 0)
            {
                break; // not found
            }

            if (comparision == 0)
            {
                m_parentItem->removeRow(targetRow);
                break;
            }

            ++targetRow;
        }
    }

    ChildType* FindChild(const SourceType& source)
    {
        int targetRow = 0;
        while (targetRow < m_parentItem->rowCount())
        {
            int comparision = Compare(source, static_cast<ChildType*>(m_parentItem->child(targetRow)));

            if (comparision < 0)
            {
                break; // not found
            }

            if (comparision == 0)
            {
                return static_cast<ChildType*>(m_parentItem->child(targetRow));
            }

            ++targetRow;
        }
        return nullptr;
    }


protected:

    virtual ChildType* Create(const SourceType& source) = 0;
    virtual void Update(const SourceType& source, ChildType* target) = 0;
    virtual int Compare(const SourceType& source, const ChildType* target) const = 0;
    virtual void ChildAdded(ChildType* newChild) {}

    ParentType* Parent() const { return m_parentItem; }

    static int CompareVariants(const QVariant& sourceKey, const QVariant& targetKey)
    {
        if (sourceKey < targetKey)
        {
            return -1;
        }
        else if (sourceKey > targetKey)
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }

    static int CompareStrings(const QString& sourceKey, const QString& targetKey)
    {
        if (sourceKey < targetKey)
        {
            return -1;
        }
        else if (sourceKey > targetKey)
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }

private:

    ParentType* m_parentItem;
};


