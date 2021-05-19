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

#include "stdafx.h"

#include <FileContentModel.h>

#include <FileContentModel.moc>

#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QDateTime>
#include <QDebug>

// QStandardItem's implementation of setData doesn't
// forward the role when calling dataChanged and
// doesn't give the extending class any notification
// that data has changed.
class QImprovedItem
    : public QStandardItem
{
public:

    void setData(const QVariant& value, int role) override
    {
        // Preserve QStandardItem behavior.
        if (role == Qt::EditRole)
        {
            role = Qt::DisplayRole;
        }

        if (m_data[role] == value)
        {
            return;
        }

        m_data[role] = value;

        auto m = model();
        if (m)
        {
            m->dataChanged(index(), index(), QVector<int>{role});
        }

        dataChanged(value, role);
    }

    QVariant data(int role) const override
    {
        // Preserve QStandardItem behavior.
        if (role == Qt::EditRole)
        {
            role = Qt::DisplayRole;
        }

        return m_data[role];
    }

protected:

    virtual void dataChanged(const QVariant& value, int role) {}

private:

    QMap<int, QVariant> m_data;
};

class FileContentItem
    : public QImprovedItem
{
public:

    FileContentItem(const QString& path, bool doNotDelete)
    {
        QFileInfo fileInfo {
            path
        };
        setData(path, FileContentModel::PathRole);
        setData(fileInfo.lastModified(), FileContentModel::FileTimestampRole);
        setData(doNotDelete, FileContentModel::DoNotDeleteRole);
    }

private:

    void dataChanged(const QVariant& value, int role) override
    {
        if (role == Qt::DisplayRole)
        {
            setData(true, FileContentModel::ModifiedRole);
        }
    }
};

FileContentModel::FileContentModel(const QString& path, bool doNotDelete, AZStd::function<QSharedPointer<IStackStatusModel>()> stackStatusModelProivder)
    : m_path{path}
    , m_stackStatusModelProivder{stackStatusModelProivder}
{
    appendRow(new FileContentItem {path, doNotDelete});
    connect(&m_fileSystemWatcher, &QFileSystemWatcher::fileChanged, this, &FileContentModel::OnFileChanged);
    m_fileSystemWatcher.addPath(path);
    Reload();
}

void FileContentModel::OnFileChanged(const QString& path)
{
    QFileInfo fileInfo {
        m_path
    };
    auto modifiedTime = fileInfo.lastModified();

    // Filter out multiple notifications in rapid succession generated for some file operations.
    if (ContentIndex().data(FileTimestampRole).toDateTime().msecsTo(modifiedTime) >= 1)
    {
        setData(ContentIndex(), modifiedTime, FileTimestampRole);
        Reload();
    }
}

bool FileContentModel::Save()
{
    QFile f(m_path);
    if (f.open(QFile::WriteOnly | QFile::Text))
    {
        setData(ContentIndex(), false, ModifiedRole);
        QTextStream out(&f);
        out << data(ContentIndex(), Qt::DisplayRole).toString();
        return true;
    }
    return false;
}

void FileContentModel::Reload()
{
    QFile f(m_path);
    if (f.open(QFile::ReadOnly | QFile::Text))
    {
        QTextStream in(&f);
        setData(ContentIndex(), in.readAll(), Qt::DisplayRole);
        setData(ContentIndex(), false, ModifiedRole);
    }
}

QSharedPointer<IStackStatusModel> FileContentModel::GetStackStatusModel() const
{
    return m_stackStatusModelProivder();
}
