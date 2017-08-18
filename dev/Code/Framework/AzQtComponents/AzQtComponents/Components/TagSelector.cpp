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

#include <AzQtComponents/Components/TagSelector.h>

#include <QVBoxLayout>
#include <QComboBox>
#include <QGridLayout>
#include <QPainter>
#include <QList>
#include <QPixmap>
#include <QDebug>
#include <QLineEdit>
#include <QMouseEvent>

namespace AzQtComponents
{
    enum
    {
        TagBorderWidth = 4,
        TagDeleteButtonRightMargin = 1,
        TagTextLeftMargin = 1,
        TagTextButtonSpacing = 10,
        TagHeight = 20,
        TagMaxTextWidth = 100
    };

    class TagContainer
        : public QWidget
    {
    public:
        explicit TagContainer(QWidget* parent = nullptr)
            : QWidget(parent)
            , m_layout(new QHBoxLayout(this))
        {
            m_layout->setMargin(0);
            m_layout->setSpacing(2);
            m_layout->addStretch();
        }

        void setNumColumns(int n)
        {
            if (n != m_numColumns)
            {
                m_numColumns = n;
            }
        }

        void appendTag(const QString& tag)
        {
            appendTag(new TagWidget(tag, this));
        }

        void appendTag(TagWidget* tag)
        {
            if (contains(tag->text()))
            {
                return;
            }

            m_tags.append(tag);
            m_layout->insertWidget(m_layout->count() - 1, tag);

            connect(tag, &TagWidget::deleteClicked, [this, tag] {
                    removeTag(tag);
                });
        }

        void removeTag(TagWidget* tag)
        {
            if (m_tags.contains(tag))
            {
                m_tags.removeAll(tag);
                tag->deleteLater();
            }
        }

        int count() const
        {
            return m_tags.count();
        }

        bool contains(const QString& tag)
        {
            return std::find_if(m_tags.cbegin(), m_tags.cend(), [tag](TagWidget* wid)
                {
                    return wid->text() == tag;
                }) != m_tags.cend();
        }

        QHBoxLayout* m_layout;
        int m_numColumns = 4;
        QList<TagWidget*> m_tags;
    };

    TagWidget::TagWidget(const QString& text, QWidget* parent)
        : QWidget(parent)
        , m_deleteButton(":/stylesheet/img/tag_delete.png")
    {
        setText(text);
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    }

    void TagWidget::paintEvent(QPaintEvent*)
    {
        // Draw border-image through stylesheets:
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

        p.setPen(QColor(204, 204, 204));
        p.drawPixmap(deleteButtonRect(), m_deleteButton, m_deleteButton.rect());

        QFontMetrics fm(font());
        QRect rect = textRect();
        QString elidedText = fm.elidedText(m_text, Qt::ElideRight, rect.width());
        p.drawText(rect, elidedText);
    }

    void TagWidget::mousePressEvent(QMouseEvent* ev)
    {
        m_deleteButtonPressed = deleteButtonRect().contains(ev->pos());
        update();
    }

    void TagWidget::mouseReleaseEvent(QMouseEvent* ev)
    {
        if (m_deleteButtonPressed && deleteButtonRect().contains(ev->pos()))
        {
            emit deleteClicked();
        }

        m_deleteButtonPressed = false;
        update();
    }

    QRect TagWidget::textRect() const
    {
        const int x = TagBorderWidth + TagTextLeftMargin;
        const int y = TagBorderWidth - 1;

        // Only subtract one border, it's normal for some characters touch the border, like 'g':
        const int h = height() - TagBorderWidth;

        QFontMetrics fm(font());
        const int w = qMin((int)TagMaxTextWidth, fm.width(m_text));
        return QRect(x, y, w, h);
    }

    QRect TagWidget::deleteButtonRect() const
    {
        const QSize buttonSize = m_deleteButton.size();
        const int x = width() - delegeButtonRightOffset();
        const int y = rect().center().y() - (buttonSize.height() / 2) + 1;

        return QRect({ x, y }, buttonSize);
    }

    int TagWidget::delegeButtonRightOffset() const
    {
        return TagBorderWidth + m_deleteButton.width() + TagDeleteButtonRightMargin;
        ;
    }

    void TagWidget::setText(const QString& text)
    {
        if (m_text != text)
        {
            m_text = text;
            setFixedSize(sizeHint());
            update();
        }
    }

    QString TagWidget::text() const
    {
        return m_text;
    }

    QSize TagWidget::sizeHint() const
    {
        const int w = textRect().right() + TagTextButtonSpacing + delegeButtonRightOffset();
        return QSize(w, TagHeight);
    }

    TagSelector::TagSelector(QWidget* parent)
        : QWidget(parent)
        , m_combo(new QComboBox())
        , m_tagContainer(new TagContainer())
    {
        auto layout = new QVBoxLayout(this);
        m_combo->setEditable(true);
        m_combo->lineEdit()->setPlaceholderText(tr("Filter by"));
        m_combo->lineEdit()->setClearButtonEnabled(true);
        layout->setSpacing(5);
        layout->setMargin(0);
        layout->addWidget(m_combo);
        layout->addWidget(m_tagContainer);

        connect(m_combo, SIGNAL(activated(int)), this, SLOT(selectTag(int)));
    }

    QComboBox* TagSelector::combo() const
    {
        return m_combo;
    }

    void TagSelector::selectTag(int comboIndex)
    {
        if (comboIndex < 0 || comboIndex >= m_combo->count())
        {
            return;
        }

        QString itemText = m_combo->itemText(comboIndex);
        if (itemText.isEmpty())
        {
            return;
        }


        m_tagContainer->appendTag(itemText);
    }

#include <Components/TagSelector.moc>
} // namespace AzQtComponents

