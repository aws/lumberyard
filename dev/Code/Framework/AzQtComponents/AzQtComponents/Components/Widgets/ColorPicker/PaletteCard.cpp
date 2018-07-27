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

#include <AzQtComponents/Components/Widgets/ColorPicker/PaletteCard.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/PaletteView.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/ColorController.h>
#include <AzQtComponents/Components/Widgets/CardHeader.h>
#include <AzQtComponents/Components/Style.h>
#include <QVBoxLayout>
#include <QMenu>

namespace AzQtComponents
{

PaletteCard::PaletteCard(QSharedPointer<Palette> palette, Internal::ColorController* controller, QUndoStack* undoStack, QWidget* parent)
    : QWidget(parent)
    , m_modified(false)
    , m_header(new CardHeader(this))
    , m_palette(palette)
    , m_paletteView(new PaletteView(palette.data(), controller, undoStack, this))
{
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    m_header->setExpandable(true);
    m_header->setHasContextMenu(true);
    mainLayout->addWidget(m_header);

    connect(m_paletteView->model(), &QAbstractItemModel::rowsInserted, this, &PaletteCard::handleModelChanged);
    connect(m_paletteView->model(), &QAbstractItemModel::rowsRemoved, this, &PaletteCard::handleModelChanged);
    connect(m_paletteView->model(), &QAbstractItemModel::dataChanged, this, &PaletteCard::handleModelChanged);

    connect(m_paletteView, &PaletteView::selectedColorsChanged, this, [controller](const QVector<AZ::Color>& selectedColors) {
        if (selectedColors.size() == 1)
        {
            controller->setColor(selectedColors[0]);
        }
    });

    auto paletteLayout = new QHBoxLayout();
    paletteLayout->setContentsMargins(QMargins{ 16, 0, 0, 0 });
    paletteLayout->addWidget(m_paletteView);
    mainLayout->addLayout(paletteLayout);

    connect(m_header, &CardHeader::expanderChanged, m_paletteView, &QWidget::setVisible);
    connect(m_header, &CardHeader::contextMenuRequested, this, [this](const QPoint& point) {
        QMenu menu;
        menu.addAction(tr("Save color library..."), this, &PaletteCard::saveClicked);
        menu.addAction(tr("Close color library"), this, &PaletteCard::removeClicked);
        menu.exec(point);
    });
}

PaletteCard::~PaletteCard()
{
}

void PaletteCard::setTitle(const QString& title)
{
    if (title == m_title)
    {
        return;
    }

    m_title = title;
    updateHeader();

    emit titleChanged(title);
}

QString PaletteCard::title() const
{
    return m_title;
}

bool PaletteCard::modified() const
{
    return m_modified;
}

void PaletteCard::setModified(bool modified)
{
    if (modified == m_modified)
    {
        return;
    }

    m_modified = modified;
    updateHeader();

    emit modifiedChanged(modified);

    // force style sheet recomputation so the title color changes
    // (polish/unpolish doesn't work as the title label is a child of m_header)
    m_header->setStyleSheet(m_header->styleSheet());
}

QSharedPointer<Palette> PaletteCard::palette() const
{
    return m_palette;
}

void PaletteCard::handleModelChanged()
{
    Palette loader;
    QString fileName = QStringLiteral("%1.pal").arg(m_title);
    bool loaded = loader.load(fileName);

    setModified(!loaded || (loaded && loader.colors() != m_palette->colors()));
}

void PaletteCard::updateHeader()
{
    QString headerText = m_title;
    if (m_modified)
    {
        headerText += QStringLiteral("*");
    }

    m_header->setTitle(headerText);
}

void PaletteCard::setSwatchSize(const QSize& size)
{
    m_paletteView->setItemSize(size);
}

void PaletteCard::setGammaEnabled(bool enabled)
{
    m_paletteView->setGammaEnabled(enabled);
}

void PaletteCard::setGamma(qreal gamma)
{
    m_paletteView->setGamma(gamma);
}

} // namespace AzQtComponents
#include <Components/Widgets/ColorPicker/PaletteCard.moc>
