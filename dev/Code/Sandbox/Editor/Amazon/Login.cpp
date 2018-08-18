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
#include <Amazon/Login.h>

#include <iostream>
#include <chrono>
#include <aws/core/http/HttpClient.h>
#include <aws/core/http/HttpResponse.h>
#include <aws/core/utils/json/JsonSerializer.h>
#include <qurlquery.h>
#include <aws/core/utils/StringUtils.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>

#include <Amazon/Login.moc>
#include <qaction.h>
#include <QDesktopServices>
#include <qnetworkrequest.h>
#include <qwebframe.h>
#include <qnetworkreply.h>
#include <LyMetricsProducer/LyMetricsAPI.h>

const char* TAG = "AmazonLogin";
namespace Amazon {
    const int WEBVIEW_WIDTH = 439;
    const int WEBVIEW_HEIGHT = 561;
    const char* SETTINGS_PATH = "Identity";
    const char* SETTINGS_ACCESS_TOKEN = "AccessToken2";
    const char* TOKEN_QUERY_STRING_KEY = "aToken";
    const char* DIRECTED_ID_QUERY_STRING_KEY = "openid.identity";
    const char* AUTHPORTAL_URL = "https://www.amazon.com/ap/signin?openid.ns=http://specs.openid.net/auth/2.0&openid.claimed_id=http://specs.openid.net/auth/2.0/identifier_select&openid.identity=http://specs.openid.net/auth/2.0/identifier_select&openid.mode=checkid_setup&openid.oa2.scope=device_auth_access&openid.ns.oa2=http://www.amazon.com/ap/ext/oauth/2&openid.oa2.response_type=token&language=en-US&marketPlaceId=ATVPDKIKX0DER&openid.return_to=https://www.amazon.com/ap/maplanding&openid.pape.max_auth_age=0&forceMobileLayout=true&openid.assoc_handle=amzn_lumberyard_desktop_us&pageId=amzn_lumberyard_desktop";
    const char* FORGOT_PASSWORD_URL = "https://www.amazon.com/ap/forgotpassword";
    const char* WELCOME_TITLE_TEXT = "Welcome to Lumberyard.";
    const char* WELCOME_TEXT = "You must log into the editor with your Amazon.com account.  <br/> If you prefer, you may create a new account for use with Lumberyard.";
    const char* FORGOT_PASSWORD_TEXT = " <br/> <br/><span style=\"color:red;\" > Please check your email for instructions on how to update your password<br /> and then return here to login.</span>";

    static const char* EDITOR_LOGIN_EVENT_NAME = "EditorLogin";
    static const char* EDITOR_OPENED_EVENT_NAME = "EditorOpened";
    static const char* EDITOR_DIRECTED_ID_ATTRIBUTE_NAME = "DirectedId";

    const char* TROUBLE_CONNECTING_TEXT = "<style>"
        "a { color: #0066C0; text-decoration: none; }"
        "</style>"
        "<p>You must have access to the internet the first time you launch <br/>"
        "Amazon Lumberyard. Please ensure your computer is connected to <br/>"
        "the internet and that Lumberyard is not blocked from accessing <br/>"
        "<a href=\"https://www.amazon.com\">https://www.amazon.com</a> by a firewall or proxy.<br/>"
        "Visit our <a href=\"https://gamedev.amazon.com/forums/help/login\">forums</a> to learn more";

    const char* FOOTER_TEXT = "<style>"
        "a { color: #0066C0; text-decoration: none; }"
        "</style>"
        "By using Lumberyard, you agree to the <a href=\"https://aws.amazon.com/agreement/\">AWS Customer Agreement</a>, <br/>"
        "<a href=\"https://aws.amazon.com/service-terms/#56._Amazon_Lumberyard\">Lumberyard Service Terms</a> and "
        "<a href=\"https://aws.amazon.com/privacy/\">AWS Privacy Notice</a>.";

    LoginDialog::LoginDialog(QWidget& parent)
        : QDialog(&parent)
        , m_webView()
        , m_layout()
        , m_oauthToken()
        , m_page()
        , m_nam()
        , m_loadingLbl()
        , m_loadingMovie(":/Login/spinner_2x._V1_.gif")
        , m_loadingRetry("Retry")
        // This spacer needs to be allocated on the heap or it will be freed
        // twice causing a crash (the layout would try to free it and so would
        // the LoginDialog)
        , m_spacer(new QSpacerItem(0, 0))
    {
        this->setWindowFlags(Qt::Dialog | Qt::Popup);
        this->setWindowTitle(tr("Login to Amazon Lumberyard"));
        this->setContentsMargins(30, 29, 27, 30);
        this->setObjectName("LoginDialog");

        m_nam = new QNetworkAccessManager(this);

        m_layout.setContentsMargins(0, 0, 0, 0);

        m_welcomeTitle.setText(WELCOME_TITLE_TEXT);
        m_welcomeTitle.setContentsMargins(0, 0, 0, 6);

        m_welcomeText.setText(WELCOME_TEXT);
        m_welcomeText.setContentsMargins(0, 0, 0, 20);

        m_footerText.setText(FOOTER_TEXT);
        m_footerText.setTextInteractionFlags(Qt::TextBrowserInteraction);
        m_footerText.setOpenExternalLinks(true);
        m_footerText.setContentsMargins(0, 20, 0, 0);

        m_loadingInfoLbl.setTextInteractionFlags(Qt::TextBrowserInteraction);
        m_loadingInfoLbl.setOpenExternalLinks(true);

        m_webViewFrame.setFixedSize(443, 565);
        m_webView.move(2, 2);

        m_webView.setFixedSize(WEBVIEW_WIDTH, WEBVIEW_HEIGHT);
        m_webView.setParent(&m_webViewFrame);

        m_loadingLbl.setMovie(&m_loadingMovie);
        m_loadingLbl.setAlignment(Qt::AlignCenter);
        m_loadingInfoLbl.setAlignment(Qt::AlignCenter);
        m_loadingRetry.setMaximumWidth(100);
        m_loadingRetry.setMinimumHeight(30);
        m_loadingMovie.start();

        m_layout.addWidget(&m_welcomeTitle);
        m_layout.addWidget(&m_welcomeText);
        m_layout.addWidget(&m_webViewFrame);
        
        m_layout.addWidget(&m_loadingInfoLbl);
        m_layout.addWidget(&m_loadingLbl);
        m_layout.addWidget(&m_loadingRetry);
        m_layout.setAlignment(&m_loadingInfoLbl, Qt::AlignCenter);
        m_layout.setAlignment(&m_loadingLbl, Qt::AlignCenter);
        m_layout.setAlignment(&m_loadingRetry, Qt::AlignCenter);
        m_layout.setAlignment(Qt::AlignCenter);

        m_layout.addSpacerItem(m_spacer);
        m_layout.addWidget(&m_footerText);
        m_page.setNetworkAccessManager(m_nam);


        //This allows createWindow to be called when the webpage tries to open a new window
        m_page.settings()->setAttribute(QWebSettings::JavascriptCanOpenWindows, true);

        init();
        this->setLayout(&m_layout);

        QObject::connect(&m_webView, &QWebView::urlChanged, this, &LoginDialog::urlChanged);
        QObject::connect(m_nam, &QNetworkAccessManager::finished, this, &LoginDialog::networkFinished);
        QObject::connect(&m_loadingRetry, &QPushButton::clicked, this, &LoginDialog::init);
    }

    LoginDialog::~LoginDialog() {
        QObject::disconnect(&m_webView, &QWebView::urlChanged, this, &LoginDialog::urlChanged);
        QObject::disconnect(m_nam, &QNetworkAccessManager::finished, this, &LoginDialog::networkFinished);
        QObject::disconnect(&m_loadingRetry, &QPushButton::clicked, this, &LoginDialog::init);
    }


    void LoginDialog::init()
    {
        //Our custom page does some link redirection and delegates some links to the desktop browser
        m_webView.setPage(&m_page);
        m_webView.setUrl(QUrl(AUTHPORTAL_URL));
        m_loadingInfoLbl.setProperty("HasNoWindowDecorations", true);
        m_loadingLbl.setProperty("HasNoWindowDecorations", true);
        m_loadingLbl.show();
        m_loadingInfoLbl.setText("Connecting to Amazon...");
        m_loadingInfoLbl.show();
        m_webViewFrame.hide();
        m_spacer->changeSize(WEBVIEW_WIDTH, WEBVIEW_HEIGHT - m_loadingLbl.size().height() - m_loadingInfoLbl.size().height());
        m_loadingRetry.hide();
    }

    //We look for amazon.com/ap links(which is authportal) and open them in-frame
    //if m_page points to something, this means createWindow was called, which means we want to use the user's desktop browser
    bool AmazonLoginWebPage::acceptNavigationRequest(QWebFrame* frame, const QNetworkRequest& request, NavigationType type)
    {
        auto url = request.url();
        auto display = url.toDisplayString().toStdString();

        if (m_page != nullptr || url.toDisplayString().contains("amazon.com/ap/forgotpassword") || !url.toDisplayString().contains("amazon.com/ap"))
        {
            QDesktopServices::openUrl(url);
            return false;
        }
        else
        {
            //All authportal links should be handled in the webview
            return true;
        }
    }

    //The non-AuthPortal links, like privacy notice and help, use JS to open a new window
    //We have to override this to return a page that will do something intelligent, this is how QT works
    //We are returning our own webpage type which knows to open the link in the user's browser
    QWebPage* AmazonLoginWebPage::createWindow(WebWindowType type)
    {
        return new AmazonLoginWebPage(this);
    }

    void sendUserEvent(const char* eventName, const char* directedId)
    {
        std::shared_ptr<Aws::Http::HttpClient> httpClient =  Aws::Http::CreateHttpClient(Aws::Client::ClientConfiguration());

        Aws::String eventsUrl = "https://sfzuat1qh2.execute-api.us-east-1.amazonaws.com/prod/events";
        auto httpRequest(Aws::Http::CreateHttpRequest(eventsUrl, Aws::Http::HttpMethod::HTTP_POST, Aws::Utils::Stream::DefaultResponseStreamFactoryMethod));

        Aws::Utils::Json::JsonValue clientJson, eventJson;
        clientJson.WithString("client_id", Aws::Utils::StringUtils::URLDecode(directedId).c_str());

        const auto now = std::chrono::system_clock::now();
        const auto epoch = now.time_since_epoch();
        const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(epoch);


        eventJson.WithString("event_type", eventName)
            .WithObject("client", clientJson)
            .WithInt64("event_timestamp", seconds.count() * 1000);

        std::shared_ptr<Aws::StringStream> sstream = Aws::MakeShared<Aws::StringStream>(eventName);
        auto eventString = eventJson.WriteCompact();
        *sstream << eventString;
        httpRequest->AddContentBody(sstream);
        httpRequest->SetContentLength(std::to_string(eventString.length()).c_str());
        httpRequest->SetHeaderValue("Content-Type", "application/json");

        auto httpResponse(httpClient->MakeRequest(*httpRequest, nullptr, nullptr));
        return;
    }

    void LoginDialog::urlChanged(const QUrl& newUrl)
    {
        QUrlQuery queryString(newUrl);
        if (queryString.hasQueryItem(TOKEN_QUERY_STRING_KEY))
        {
            m_oauthToken = queryString.queryItemValue(DIRECTED_ID_QUERY_STRING_KEY);

			sendUserEvent(EDITOR_LOGIN_EVENT_NAME, m_oauthToken.toStdString().c_str());

            accept();
        }
    }

    void LoginDialog::networkFinished(QNetworkReply* reply)
    {
        m_loadingLbl.hide();
        if (reply->error() != QNetworkReply::NetworkError::NoError && reply->error() != QNetworkReply::NetworkError::OperationCanceledError)
        {
            m_loadingInfoLbl.setText(TROUBLE_CONNECTING_TEXT);
            m_loadingRetry.show();
            m_spacer->changeSize(WEBVIEW_WIDTH, WEBVIEW_HEIGHT - m_loadingLbl.size().height() - m_loadingInfoLbl.size().height() - m_loadingRetry.height());
            return;
        }

        //If we just loaded the page after submitting forgot your password, lets navigate back to the login screen
        if (m_webView.url() == QUrl(FORGOT_PASSWORD_URL))
        {
            m_welcomeText.setText(m_forgotPasswordText);
            m_welcomeText.setContentsMargins(0, 0, 0, 20);
            m_webView.setUrl(QUrl(AUTHPORTAL_URL));
        }

        //We are successfully making network calls. Lets show our webView and remove all our loading stuff
        m_layout.removeItem(m_spacer);
        m_loadingInfoLbl.hide();
        m_loadingLbl.hide();
        m_layout.removeWidget(&m_loadingInfoLbl);
        m_layout.removeWidget(&m_loadingLbl);
        m_layout.removeWidget(&m_loadingRetry);

        //Without these calls, there is a weird issue where after this function there is a ton of empty space at the top of our layout
        //Upon moving, the layout. These updates prevent this empty space from appearing
        m_webViewFrame.show();
        m_welcomeTitle.update();
        m_welcomeText.update();
        m_webViewFrame.update();
        m_footerText.update();
        m_layout.update();
    }

    std::shared_ptr<AmazonIdentity> LoginManager::GetLoggedInUser()
    {
        if (!m_identity)
        {
            InitIdentity();
        }
        if (!m_identity)
        {
            LoginDialog loginDialog(m_parent);

            int returnVal = loginDialog.exec();
            if (returnVal == QDialog::Accepted)
            {
                m_identity = Aws::MakeShared<AmazonIdentity>(TAG, loginDialog.GetAccessToken());
                PersistProfile();
            }
        }

        static bool s_EditorOpenedEventSent = false;
        if(!s_EditorOpenedEventSent && m_identity)
        {   
            std::string aToken = m_identity->GetAccessToken().toStdString();
            sendUserEvent(EDITOR_OPENED_EVENT_NAME, aToken.c_str());
            s_EditorOpenedEventSent = true;
        }
		
        return m_identity;
    }

    void LoginManager::InitIdentity()
    {
        QSettings settings;
        settings.beginGroup(SETTINGS_PATH);
        QString aToken = settings.value(SETTINGS_ACCESS_TOKEN, "").toString();
        if (!aToken.isEmpty())
        {
            //For now, we are simply validating that they've ever logged in
            m_identity = Aws::MakeShared<AmazonIdentity>(TAG, aToken);
        }
    }
    void LoginManager::PersistProfile()
    {
        auto aToken = m_identity->GetAccessToken();
        QSettings settings;
        settings.beginGroup(SETTINGS_PATH);
        settings.setValue(SETTINGS_ACCESS_TOKEN, aToken);
    }
} // namespace Amazon


