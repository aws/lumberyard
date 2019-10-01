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

#include <AzQtComponents/Components/StyledBusyLabel.h>

#include <QLabel>
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'QImageIOHandler::d_ptr': class 'QScopedPointer<QImageIOHandlerPrivate,QScopedPointerDeleter<T>>' needs to have dll-interface to be used by clients of class 'QImageIOHandler'
#include <QMovie>
AZ_POP_DISABLE_WARNING
#include <QHBoxLayout>

namespace AzQtComponents
{
    StyledBusyLabel::StyledBusyLabel(QWidget* parent)
        : QWidget(parent)
        , m_busyIcon(new QLabel(this))
        , m_text(new QLabel(this))
    {
        setLayout(new QHBoxLayout);
        layout()->setSpacing(6);
        layout()->addWidget(m_busyIcon);
        layout()->addWidget(m_text);
        m_busyIcon->setMovie(new QMovie(this));
        m_busyIcon->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        m_text->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);

        SetBusyIcon(":/stylesheet/img/in_progress.gif");
    }

    bool StyledBusyLabel::GetIsBusy() const
    {
        return m_busyIcon->movie()->state() == QMovie::Running;
    }

    void StyledBusyLabel::SetIsBusy(bool busy)
    {
        if (m_isBusy != busy)
        {
            m_isBusy = busy;
            updateMovie();
        }
    }

    QString StyledBusyLabel::GetText() const
    {
        return m_text->text();
    }

    void StyledBusyLabel::SetText(const QString& text)
    {
        m_text->setText(text);
    }

    QString StyledBusyLabel::GetBusyIcon() const
    {
        return m_busyIcon->movie()->fileName();
    }

    void StyledBusyLabel::SetBusyIcon(const QString& iconSource)
    {
        m_busyIcon->movie()->setFileName(iconSource);
        m_busyIcon->setFixedWidth(32);
        m_busyIcon->movie()->setScaledSize(QSize(height(), height()));
        updateMovie();
    }

    int StyledBusyLabel::GetBusyIconSize() const
    {
        return m_busyIconSize;
    }

    void StyledBusyLabel::SetBusyIconSize(int iconSize)
    {
        if (m_busyIconSize != iconSize)
        {
            m_busyIconSize = iconSize;
            updateMovie();
        }
    }

    QSize StyledBusyLabel::sizeHint() const
    {
        return QSize(m_busyIconSize, m_busyIconSize);
    }

    void StyledBusyLabel::updateMovie()
    {
        if (m_isBusy)
        {
            m_busyIcon->movie()->start();
            m_busyIcon->show();
        }
        else
        {
            m_busyIcon->movie()->stop();
            m_busyIcon->hide();
        }
        m_busyIcon->setFixedWidth(m_busyIconSize);
        m_busyIcon->movie()->setScaledSize(QSize(m_busyIconSize, m_busyIconSize));
    }

#include <Components/StyledBusyLabel.moc>
} // namespace AzQtComponents
