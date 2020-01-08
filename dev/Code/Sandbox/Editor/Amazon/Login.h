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
#ifndef CRYINCLUDE_EDITOR_LOGIN_H
#define CRYINCLUDE_EDITOR_LOGIN_H
#pragma once

#include <QDialog>
#include <QWebView>
#include <QVBoxLayout>
#include <QLabel>
#include <QMovie>
#include <QPushButton>
#include <QSpacerItem>

#include <memory>
#include <AzCore/PlatformDef.h>
AZ_PUSH_DISABLE_WARNING(4251 4355 4996, "-Wunknown-warning-option")
#include <aws/core/http/HttpClientFactory.h>
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/utils/json/JsonSerializer.h>
AZ_POP_DISABLE_WARNING

namespace Amazon {
    class QWidgetWinWrapper
        : public QWidget
    {
    public:
        QWidgetWinWrapper(HWND winToWrap)
            : QWidget()
        {
            create((WId)winToWrap, false, false);
        }
    };

    class AmazonIdentity
    {
    public:
        AmazonIdentity(QString accessToken)
            : m_accessToken(accessToken)
        {}
        const QString& GetAccessToken() const { return m_accessToken; }

    private:
        QString m_accessToken;
    };

    /* This simple QWebPage subclass handles opening links in either the editor webview OR invoking the desktop browser*/
    class AmazonLoginWebPage
        : public QWebPage
    {
    public:
        AmazonLoginWebPage()
            : m_page(nullptr){}

        AmazonLoginWebPage(QWebPage* page)
            : m_page(page)
        {}


        bool acceptNavigationRequest(QWebFrame* frame, const QNetworkRequest& request, NavigationType type) override;
        QWebPage* createWindow(WebWindowType type) override;

    private:
        QWebPage* m_page;
    };

    class LoginWelcomeTitle
        : public QLabel
    {
        Q_OBJECT
    };

    class LoginWelcomeText
        : public QLabel
    {
        Q_OBJECT
    };

    class LoginFooterText
        : public QLabel
    {
        Q_OBJECT
    };

    class LoginWebViewFrame
        : public QFrame
    {
        Q_OBJECT
    };

    class LoginWebView
        : public QWebView
    {
        Q_OBJECT;
    };

    class LoginDialog
        : public QDialog
    {
        Q_OBJECT
    public:
        LoginDialog(QWidget& parent);
        const QString& GetAccessToken() const { return m_oauthToken; };
        ~LoginDialog();
    public slots:
        void urlChanged(const QUrl& url);
        void networkFinished(QNetworkReply* reply);
        void init();

    private:
        QString m_oauthToken;
        LoginWebViewFrame m_webViewFrame;
        LoginWebView m_webView;
        LoginWelcomeTitle m_welcomeTitle;
        LoginWelcomeText m_welcomeText;
        LoginFooterText m_footerText;
        QVBoxLayout m_layout;
        AmazonLoginWebPage m_page;
        QNetworkAccessManager* m_nam;
        LoginWelcomeText m_loadingInfoLbl;
        LoginWelcomeText m_loadingLbl;
        QPushButton m_loadingRetry;
        QMovie m_loadingMovie;
        QSpacerItem* m_spacer;
        QString m_forgotPasswordText;
    };

    class LoginManager
    {
    public:
        LoginManager(QWidget& parent)
            : m_httpClient(Aws::Http::CreateHttpClient(Aws::Client::ClientConfiguration()))
            , m_parent(parent)
        {};

        std::shared_ptr<AmazonIdentity> GetLoggedInUser();
        void LogOut() { m_identity.reset(); }

    private:
        //Read the identity information from the local device
        //and retrieve a new access token
        //If either of these processes fail, m_identity will be null
        void InitIdentity();

        //Save the current identity where InitIdentity can load it later
        void PersistProfile();

        std::shared_ptr<Aws::Http::HttpClient> m_httpClient;
        std::shared_ptr<AmazonIdentity> m_identity;
        QWidget& m_parent;
    };
} // namespace Amazon
#endif // CRYINCLUDE_EDITOR_LOGIN_H


