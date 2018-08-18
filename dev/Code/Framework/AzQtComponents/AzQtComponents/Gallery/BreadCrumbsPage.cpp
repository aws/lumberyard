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

#include "BreadCrumbsPage.h"
#include <Gallery/ui_BreadCrumbsPage.h>

#include <QDebug>
#include <QStyle>
#include <QToolButton>
#include <QPushButton>
#include <QFileDialog>
#include <AzQtComponents/Components/Widgets/BreadCrumbs.h>

static QWidget* createSeparator(QBoxLayout* layout, QWidget* parent)
{
    QFrame* line = new QFrame(parent);
    line->setMinimumSize(QSize(0, 0));
    line->setMaximumSize(QSize(2, 20));
    line->setStyleSheet(QStringLiteral("color: black; background-color: black"));
    line->setFrameShape(QFrame::VLine);
    line->setFrameShadow(QFrame::Sunken);

    layout->addWidget(line);

    return line;
}

BreadCrumbsPage::BreadCrumbsPage(QWidget* parent)
: QWidget(parent)
, ui(new Ui::BreadCrumbsPage)
{
    ui->setupUi(this);

    AzQtComponents::BreadCrumbs* breadCrumbs = new AzQtComponents::BreadCrumbs(this);

    // Seed the bread crumbs with an arbitrary path, to make the example obvious
    breadCrumbs->pushPath("C:/Documents/SubDirectory1/SubDirectory2/SubDirectory3");

    // Have the bread crumb widget create the right buttons for us and we just lay them out
    ui->horizontalLayout->addWidget(breadCrumbs->createButton(AzQtComponents::NavigationButton::Up));
    ui->horizontalLayout->addWidget(breadCrumbs->createButton(AzQtComponents::NavigationButton::Back));
    ui->horizontalLayout->addWidget(breadCrumbs->createButton(AzQtComponents::NavigationButton::Forward));

    createSeparator(ui->horizontalLayout, this);

    ui->horizontalLayout->addWidget(breadCrumbs);

    createSeparator(ui->horizontalLayout, this);

    auto browseButton = new QPushButton("Browse...", this);

    ui->horizontalLayout->addWidget(browseButton);

    connect(browseButton, &QPushButton::pressed, breadCrumbs, [breadCrumbs] {
        QString newPath = QFileDialog::getExistingDirectory(breadCrumbs, "Please select a new path to push into the bread crumbs");
        if (!newPath.isEmpty())
        {
            breadCrumbs->pushPath(newPath);
        }
    });

    connect(breadCrumbs, &AzQtComponents::BreadCrumbs::pathChanged, this, [](const QString& newPath) {
        qDebug() << QString("New path: %1").arg(newPath);
    });

    QString exampleText = R"(

The BreadCrumbs widget is a Lumberyard specific container widget. Use it to show paths, file and otherwise, and provide functionality to click on pieces of the path and jump to them.<br/><br/>

Some sample code:<br/><br/>

<pre>
#include &lt;AzQtComponents/Components/Widgets/BreadCrumbs.h&gt;
#include &lt;QDebug&gt;

// Create a new BreadCrumbs widget:
AzQtComponents::BreadCrumbs* breadCrumbs = new AzQtComponents::BreadCrumbs(this);

// To set the initial path or jump to another
QString somePath = "C:\\Documents";
breadCrumbs->pushPath(somePath);

// To navigate:
breadCrumbs->back();
breadCrumbs->forward();
breadCrumbs->up();

// Listen for path changes
connect(breadCrumbs, &AzQtComponents::BreadCrumbs::pathChanged, this, [](const QString& newPath){
    qDebug() &lt;&lt; QString("New path: %1").arg(newPath);
});

// To get the current path:
QString currentPath = breadCrumbs->currentPath();

// Create auto-connected navigation buttons and layout everything in a group widget:
QWidget* group = new QWidget(this);
QHBoxLayout* groupLayout = new QHBoxLayout(group);

AzQtComponents::BreadCrumbs* breadCrumbs = new AzQtComponents::BreadCrumbs(group);

layout->addWidget(breadCrumbs->createButton(AzQtComponents::NavigationButton::Up));
layout->addWidget(breadCrumbs->createButton(AzQtComponents::NavigationButton::Back));
layout->addWidget(breadCrumbs->createButton(AzQtComponents::NavigationButton::Forward));

layout->addWidget(breadCrumbs);

</pre>

)";

    ui->exampleText->setHtml(exampleText);
}

BreadCrumbsPage::~BreadCrumbsPage()
{
}

#include <Gallery/BreadCrumbsPage.moc>