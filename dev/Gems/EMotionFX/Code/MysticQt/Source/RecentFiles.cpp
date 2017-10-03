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

#include "RecentFiles.h"
#include "MysticQtManager.h"
#include <AzFramework/API/ApplicationAPI.h>
#include <QtCore/QFileInfo>
#include <QtCore/QSettings>
#include <QtGui/QHelpEvent>
#include <QtWidgets/QToolTip>


namespace MysticQt
{
    class ToolTipMenu
        : public QMenu
    {
    public:
        ToolTipMenu(const QString title, QWidget* parent)
            : QMenu(title, parent)
        {
            mParent = parent;
        }

        virtual bool event(QEvent* e) override
        {
            bool result = QMenu::event(e);
            const QHelpEvent* helpEvent = static_cast<QHelpEvent*>(e);

            if (helpEvent->type() == QEvent::ToolTip)
            {
                QAction* action = activeAction();
                if (action)
                {
                    QToolTip::showText(helpEvent->globalPos(), action->toolTip(), mParent);
                }
            }
            else
            {
                QToolTip::hideText();
            }

            return result;
        }

    private:
        QWidget* mParent;
    };


    RecentFiles::RecentFiles()
        : QObject()
    {
        m_maxNumRecentFiles = 10;
    }


    void RecentFiles::Init(QMenu* parentMenu, size_t numRecentFiles, const char* subMenuName, const char* configStringName)
    {
        m_configStringName   = configStringName;
        m_maxNumRecentFiles  = numRecentFiles;

        m_resetRecentFilesAction = nullptr;
        m_recentFilesMenu = new ToolTipMenu(subMenuName, parentMenu);
        parentMenu->addMenu(m_recentFilesMenu);

        SetMaxRecentFiles(numRecentFiles);

        m_recentFilesMenu->addSeparator();
        m_resetRecentFilesAction = m_recentFilesMenu->addAction("Reset Recent Files", this, &RecentFiles::OnClearRecentFiles);

        // Remove legacy duplicates or duplicated filenames with inconsistent casing.
        RemoveDuplicates();
    }


    void RecentFiles::OnClearRecentFiles()
    {
        QSettings settings(this);
        settings.beginGroup("EMotionFX");
        settings.setValue(m_configStringName, QStringList());

        UpdateRecentFileActions();
        settings.endGroup();
    }


    void RecentFiles::UpdateRecentFileActions(bool checkFilesExist)
    {
        QSettings settings(this);
        settings.beginGroup("EMotionFX");
        QStringList files = settings.value(m_configStringName).toStringList();

        // Remove deleted or non-existing files from the recent files.
        if (checkFilesExist)
        {
            QString filename;
            for (int i = 0; i < files.size(); )
            {
                if (!QFile::exists(files[i]))
                {
                    files.removeAt(i);
                }
                else
                {
                    i++;
                }
            }
        }

        QString strippedName, menuItemText;
        size_t numRecentFiles = m_recentFileActions.size();
        for (size_t i = 0; i < numRecentFiles; ++i)
        {
            QAction* action = m_recentFileActions[i];

            if (i < static_cast<size_t>(files.size()))
            {
                const int intIndex  = static_cast<int>(i);
                strippedName        = QFileInfo(files[intIndex]).fileName();
                menuItemText        = tr("&%1 %2").arg(i + 1).arg(strippedName);

                action->setText(menuItemText);
                action->setData(files[intIndex]);
                action->setToolTip(files[intIndex]);
                action->setVisible(true);
            }
            else
            {
                action->setVisible(false);
            }
        }

        settings.setValue(m_configStringName, files);

        if (m_resetRecentFilesAction)
        {
            if (files.size() > 0)
            {
                m_resetRecentFilesAction->setVisible(true);
            }
            else
            {
                m_resetRecentFilesAction->setVisible(false);
            }
        }
        settings.endGroup();
    }


    void RecentFiles::SetMaxRecentFiles(size_t numRecentFiles)
    {
        m_maxNumRecentFiles = numRecentFiles;
        const size_t numOldRecentFiles = m_recentFileActions.size();

        // In case the new max recent files is bigger than the old one.
        if (numOldRecentFiles < numRecentFiles)
        {
            m_recentFileActions.resize(numRecentFiles);

            for (size_t i = numOldRecentFiles; i < numRecentFiles; ++i)
            {
                QAction* action = new QAction(m_recentFilesMenu);
                action->setVisible(false);
                m_recentFilesMenu->addAction(action);

                connect(action, SIGNAL(triggered()), this, SLOT(OnRecentFileSlot()));
                m_recentFileActions[i] = action;
            }
        }
        // In case we want to reduce the maximum number of recent files.
        else
        {
            // remove the ones that are too much
            while (m_recentFileActions.size() > numRecentFiles)
            {
                if (!m_recentFileActions.empty())
                {
                    const size_t index = m_recentFileActions.size() - 1;
                    delete m_recentFileActions[index];
                    m_recentFileActions.erase(m_recentFileActions.begin() + index);
                }
            }
        }

        UpdateRecentFileActions(true);
    }


    AZStd::string RecentFiles::GetLastRecentFileName() const
    {
        if (m_recentFileActions.empty())
        {
            return "";
        }

        QAction* action = m_recentFileActions[0];
        if (action == nullptr)
        {
            return "";
        }

        return action->data().toString().toUtf8().data();
    }


    void RecentFiles::AddRecentFile(AZStd::string filename)
    {
        EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePathKeepCase, filename);

        QSettings settings(this);
        settings.beginGroup("EMotionFX");
        QStringList files = settings.value(m_configStringName).toStringList();

        // Remove filename duplicates and add the new filename to the top of the recent files.
        if (!filename.empty())
        {
            files.removeAll(filename.c_str());
            files.prepend(filename.c_str());
        }

        // Remove the oldest filenames.
        const int maxRecentFiles = static_cast<int>(m_maxNumRecentFiles);
        while (files.size() > maxRecentFiles)
        {
            files.removeLast();
        }

        settings.setValue(m_configStringName, files);
        UpdateRecentFileActions();
        settings.endGroup();
    }


    void RecentFiles::OnRecentFileSlot()
    {
        QAction* action = qobject_cast<QAction*>(sender());
        if (action == nullptr)
        {
            return;
        }

        emit OnRecentFile(action);
    }


    void RecentFiles::RemoveDuplicates()
    {
        QSettings settings(this);
        settings.beginGroup("EMotionFX");
        QStringList files = settings.value(m_configStringName).toStringList();

        for (int i = 0; i<files.size(); i++)
        {
            for (int j = i + 1; j<files.size(); )
            {
                if (files[i].compare(files[j], Qt::CaseSensitivity::CaseInsensitive) == 0)
                {
                    files.removeAt(j);
                }
                else
                {
                    j++;
                }
            }
        }

        settings.setValue(m_configStringName, files);
        settings.endGroup();
    }
} // namespace MysticQt

#include <MysticQt/Source/RecentFiles.moc>
