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
#include "PathTreeModel.h"

#include <QFileInfo>
#include <QFont>
#include <QPixmap>
#include <QMimeData>

#include <numeric>

class PathTreeModel::Node
{
public:
    enum class StateType
    {
        Inserted,
        Normal,
        Removed
    };

    Node()
        : m_state(StateType::Normal)
        , m_parent(nullptr)
    {
    }
    Node(const Node&) = delete;
    Node(Node&& other) = delete;

    virtual ~Node()
    {
        for (Node* child : m_children)
        {
            delete child;
        }
    }

    Node& operator=(const Node&) = delete;
    Node& operator=(Node&& other) = delete;

    bool operator<(const Node& right)
    {
        return m_label < right.m_label;
    }

    bool operator==(const Node& right) { return m_label == right.m_label; }


    const QString& Label() const { return m_label; }

    const QString& Data() const { return m_data; }

    StateType State() const { return m_state; }

    void SetState(StateType state) { m_state = state; }

    Node* Parent() const { return m_parent; }

    std::vector<Node*>& Children() { return m_children; }

    bool HasChildren() const { return !m_children.empty(); }

    std::vector<Node>::size_type Size() const { return m_children.size(); }

    Node* At(int index) { return m_children.at(index); }

    int Index() const;

    void AddChild(QStringList& path, const QString& label, const QString& data);

    Node* FindMutableChild(const QString& label) { return const_cast<Node*>(FindChild(label)); }
    const Node* FindChild(const QString& label) const;

    int LeafCount() const;

    void MarkRemoved();
    void UpdateFrom(Node* source);

    void Tidy();

    void Clear();

private:
    Node(Node* parent, const QString& label)
        : m_state(StateType::Inserted)
        , m_parent(parent)
        , m_label(label)
    {
    }

    Node(Node* parent, const QString& label, const QString& data)
        : m_state(StateType::Inserted)
        , m_parent(parent)
        , m_label(label)
        , m_data(data)
    {
    }

    StateType m_state;
    Node* m_parent;
    std::vector<Node*> m_children;

    QString m_label;
    QString m_data;
};

int PathTreeModel::Node::Index() const
{
    if (m_parent)
    {
        auto position = std::find(m_parent->m_children.begin(), m_parent->m_children.end(), this);
        if (position != m_parent->m_children.end())
        {
            return std::distance(m_parent->m_children.begin(), position);
        }
    }

    return -1;
}

void PathTreeModel::Node::AddChild(QStringList& path, const QString& label, const QString& data)
{
    if (path.isEmpty())
    {
        Node* existing = FindMutableChild(label);
        if (!existing)
        {
            Node* node = new Node(this, label, data);
            m_children.insert(std::upper_bound(m_children.begin(), m_children.end(), node, [](Node* a, Node* b) { return *a < *b; }), node);
        }
        else
        {
            existing->m_state = StateType::Normal;
            existing->m_data = data;

            m_state = StateType::Normal;
        }
    }
    else
    {
        const QString first = path.first();
        path.pop_front();

        Node* existing = FindMutableChild(first);
        if (existing)
        {
            existing->AddChild(path, label, data);
            if (m_state == StateType::Removed)
            {
                m_state = StateType::Normal;
            }
        }
        else
        {
            Node* node = new Node(this, first);
            m_children.insert(std::upper_bound(m_children.begin(), m_children.end(), node, [](Node* a, Node* b) { return *a < *b; }), node);
            node->AddChild(path, label, data);
        }
    }
}

const PathTreeModel::Node* PathTreeModel::Node::FindChild(const QString& label) const
{
    for (Node* child : m_children)
    {
        if (label.compare(child->Label(), Qt::CaseInsensitive)==0)
        {
            return child;
        }
    }

    return nullptr;
}

void PathTreeModel::Node::MarkRemoved()
{
    for (Node* child : m_children)
    {
        child->MarkRemoved();
    }
    m_state = StateType::Removed;
}

void PathTreeModel::Node::UpdateFrom(Node* source)
{
    Q_ASSERT(source);
    Q_ASSERT(source->m_label == m_label);

    auto mine = m_children.begin();
    auto theirs = source->m_children.begin();

    while (true)
    {
        if (mine == m_children.end())
        {
            while (theirs != source->m_children.end())
            {
                (*theirs)->m_parent = this;
                m_children.push_back(*theirs);
                theirs = source->m_children.erase(theirs);
            }
            return;
        }

        if (theirs == source->m_children.end() || *(*mine) < *(*theirs))
        {
            (*mine)->SetState(StateType::Removed);
            ++mine;
        }
        else if (*(*theirs) < *(*mine))
        {
            (*theirs)->m_parent = this;
            mine = m_children.insert(mine, *theirs);
            ++mine;
            theirs = source->m_children.erase(theirs);
        }
        else
        {
            (*mine)->UpdateFrom(*theirs);
            ++mine;
            ++theirs;
        }
    }
}

void PathTreeModel::Node::Tidy()
{
    m_state = StateType::Normal;
    for (Node* child : m_children)
    {
        child->Tidy();
    }
}

void PathTreeModel::Node::Clear()
{
    for (Node* child : m_children)
    {
        delete child;
    }
    m_children.clear();
}

int PathTreeModel::Node::LeafCount() const
{
    if (m_children.empty() && m_data.length() > 0)
    {
        return 1;
    }

    return std::accumulate(m_children.cbegin(), m_children.cend(), 0, [&](int value, const Node* node)
        {
            return value + node->LeafCount();
        });
}

PathTreeModel::PathTreeModel(PathTreeModel::PathDetectionMethod pathDetectionMethod, QObject* parent /* = nullptr */)
    : QAbstractItemModel(parent)
    , m_root(new Node())
    , m_mimeTypes({ "text/plain" })
    , m_pathDetectionMethod(pathDetectionMethod)
{
}

PathTreeModel::~PathTreeModel()
{
}

void PathTreeModel::Clear()
{
    m_root->Clear();
}

void PathTreeModel::AddNode(const QString& path, const QString& data)
{
    QFileInfo info(path);
    if (m_pathDetectionMethod == PathSeparators)
    {
        QStringList pathList = info.path().split('/');
        m_root->AddChild(pathList, info.baseName(), data);
    }
    else
    {
        QStringList components = path.split('.');
        QString label = components.back();
        components.pop_back();
        m_root->AddChild(components, label, data);
    }
}

int PathTreeModel::LeafCount() const
{
    return m_root->LeafCount();
}

void PathTreeModel::BeginIterativeUpdate()
{
    m_root->MarkRemoved();
}

void PathTreeModel::UpdateFrom(PathTreeModel& source)
{
    m_root->UpdateFrom(source.m_root.data());
    EndUpdate();
}

void PathTreeModel::EndUpdate()
{
    Tidy(m_root.data());
}

void PathTreeModel::Tidy(Node* node)
{
    std::vector<int> removed;

    auto& children = node->Children();

    auto erasures = std::remove_if(children.begin(), children.end(), [&](const Node* node)
            {
                if (node->State() == Node::StateType::Removed)
                {
                    removed.push_back(node->Index());
                    delete node;
                    return true;
                }
                return false;
            });

    children.erase(erasures, children.end());

    QModelIndex parent = {};
    if (!(*node == *m_root))
    {
        parent = createIndex(node->Index(), 0, node);
    }

    for (int i : removed)
    {
        beginRemoveRows(parent, i, i);
        endRemoveRows();
    }

    for (Node* child : children)
    {
        if (child->State() == Node::StateType::Normal)
        {
            Tidy(child);
        }
        else
        {
            beginInsertRows(parent, child->Index(), child->Index());
            endInsertRows();
        }
    }

    node->Tidy();
}

QModelIndex PathTreeModel::IndexOf(const QString& entry) const
{
    QStringList path = entry.split('/');
    return IndexOf(path, m_root.data());
}

QModelIndex PathTreeModel::IndexOf(QStringList& path, const Node* node) const
{
    QString firstElement = path.first();
    const Node* child = node->FindChild(firstElement);
    // if its the last node and it has an extension, then try again without it if we don't find it:
    if ((!child) && (firstElement.contains('.')))
    {
        QString withoutExtension;
        withoutExtension = firstElement.left(firstElement.indexOf('.'));
        child = node->FindChild(withoutExtension);
    }

    if (child)
    {
        path.pop_front();
        if (!path.isEmpty())
        {
            return IndexOf(path, child);
        }
        else
        {
            return createIndex(child->Index(), 0, static_cast<void*>(const_cast<Node*>(child)));
        }
    }

    return {};
}

QModelIndex PathTreeModel::index(int row, int column, const QModelIndex& parent) const
{
    if (parent == QModelIndex())
    {
        if (row < 0 || row >= m_root->Size())
        {
            return {};
        }

        return createIndex(row, 0, m_root->At(row));
    }
    Node* node = static_cast<Node*>(parent.internalPointer());

    if (!node || row < 0 || row >= node->Size())
    {
        return {};
    }

    return createIndex(row, column, node->At(row));
}

Qt::ItemFlags PathTreeModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);

    if (index.isValid())
    {
        Node* node = static_cast<Node*>(index.internalPointer());
        if (!node->HasChildren())
        {
            return Qt::ItemIsDragEnabled | defaultFlags;
        }
    }

    return defaultFlags;
}

int PathTreeModel::rowCount(const QModelIndex& parent) const
{
    if (parent == QModelIndex())
    {
        return m_root->Size();
    }

    if (!parent.isValid())
    {
        return 0;
    }

    Node* node = static_cast<Node*>(parent.internalPointer());
    return node->Size();
}

int PathTreeModel::columnCount(const QModelIndex& parent) const
{
    return 1;
}

QModelIndex PathTreeModel::parent(const QModelIndex& index) const
{
    if (!index.isValid())
    {
        return {};
    }

    Node* node = static_cast<Node*>(index.internalPointer());
    Node* parent = node->Parent();
    if (parent)
    {
        int index = parent->Index();
        if (index >= 0)
        {
            return createIndex(index, 0, parent);
        }
    }

    return {};
}

QVariant PathTreeModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
    {
        return {};
    }

    Node* node = static_cast<Node*>(index.internalPointer());

    switch (role)
    {
    case Qt::FontRole:
        if (node->HasChildren())
        {
            return QFont().bold();
        }
        return {};
    case Qt::DecorationRole:
        if (node->HasChildren() && m_directoryIcon)
        {
            return *m_directoryIcon.data();
        }
        return {};
    case Qt::DisplayRole:
    case Role::LabelRole:
        return node->Label();
    case Role::DataRole:
        return node->Data();
    case Role::NonLeafNodeRole:
        return node->HasChildren();
    case Role::ChildCountRole:
    {
        QVariant v;
        v.setValue(node->Children().size());
        return v;
    }
    default:
        return {};
    }
}

QStringList PathTreeModel::mimeTypes() const
{
    return m_mimeTypes;
}

QMimeData* PathTreeModel::mimeData(const QModelIndexList& indexes) const
{
    QMimeData* mimeData = new QMimeData();
    QByteArray encodedData;

    QDataStream stream(&encodedData, QIODevice::WriteOnly);

    foreach(QModelIndex index, indexes)
    {
        if (index.isValid())
        {
            QString text = data(index, Role::DataRole).toString();
            stream << text;
        }
    }

    mimeData->setData(m_mimeTypes.first(), encodedData);
    return mimeData;
}

#include <Controls/PathTreeModel.moc>
