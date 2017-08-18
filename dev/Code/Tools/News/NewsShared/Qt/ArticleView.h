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

#include <QWidget>

namespace Ui
{
    class ArticleViewWidget;
    class PinnedArticleViewWidget;
}

class QFrame;
class QBetterLabel;

namespace News
{
    class ArticleDescriptor;
    class ResourceManifest;

    class ArticleView
        : public QWidget
    {
        Q_OBJECT
    public:
        ArticleView(QWidget* parent,
            const ArticleDescriptor& article,
            const ResourceManifest& manifest);
        ~ArticleView() = default;

        void Update();
        QSharedPointer<const ArticleDescriptor> GetArticle() const;

    Q_SIGNALS:
        void articleSelectedSignal(QString resourceId);
        void linkActivatedSignal(const QString& link);

    protected:
        void SetupViewWidget(QFrame* widgetImageFrame, QBetterLabel* widgetTitle, QBetterLabel* widgetBody);
        void mousePressEvent(QMouseEvent* event);

    private:

        QFrame* m_widgetImageFrame = nullptr;
        QBetterLabel* m_widgetTitle = nullptr;
        QBetterLabel* m_widgetBody = nullptr;
        QBetterLabel* m_icon = nullptr;

        QSharedPointer<const ArticleDescriptor> m_pArticle;
        const ResourceManifest& m_manifest;

        void RemoveIcon();

    private Q_SLOTS:
        void linkActivatedSlot(const QString& link);
        void articleSelectedSlot();
    };

    class ArticleViewDefaultWidget : public ArticleView
    {
    public:
        ArticleViewDefaultWidget(QWidget* parent,
            const ArticleDescriptor& article,
            const ResourceManifest& manifest);
        ~ArticleViewDefaultWidget() = default;

    private:
        Ui::ArticleViewWidget* m_ui = nullptr;
    };

    class ArticleViewPinnedWidget : public ArticleView
    {
    public:
        ArticleViewPinnedWidget(QWidget* parent,
            const ArticleDescriptor& article,
            const ResourceManifest& manifest);
        ~ArticleViewPinnedWidget() = default;

    private:
        Ui::PinnedArticleViewWidget* m_ui = nullptr;
    };
}
