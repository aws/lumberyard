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

#include <AzToolsFramework/UI/Logging/LogLine.h>

#include <AzCore/XML/rapidxml.h>
#include <AzCore/std/containers/list.h>
#include <AzFramework/Logging/LogFile.h>
#include <AzToolsFramework/UI/Logging/LogPanel_Panel.h>
#include <AzToolsFramework/UI/Logging/LoggingCommon.h>
#include <AzToolsFramework/UI/Logging/LogControl.h>

#include <QColor>
#include <QDateTime>

namespace AzToolsFramework
{
    namespace Logging
    {
        LogLine::LogLine(const char* inMessage, const char* inWindow, LogType inType, AZ::u64 inTime)
            : m_message(inMessage)
            , m_window(inWindow)
            , m_type(inType)
            , m_messageTime(inTime)
            , m_processed(false)
        {
            Process();
        }

        LogLine::LogLine(const LogLine& other)
        {
            *this = other;
        }

        LogLine::LogLine(LogLine&& other)
        {
            *this = AZStd::move(other);
        }

        LogLine& LogLine::operator=(LogLine&& other)
        {
            if (this != &other)
            {
                m_message = AZStd::move(other.m_message);
                m_window = AZStd::move(other.m_window);
                m_type = other.m_type;
                m_messageTime = other.m_messageTime;
                m_isRichText = other.m_isRichText;
                m_processed = other.m_processed;
            }
            return *this;
        }

        LogLine& LogLine::operator=(const LogLine& other)
        {
            if (this != &other)
            {
                m_message = other.m_message;
                m_window = other.m_window;
                m_type = other.m_type;
                m_messageTime = other.m_messageTime;
                m_isRichText = other.m_isRichText;
                m_processed = other.m_processed;
            }
            return *this;
        }

        void LogLine::Process()
        {
            if (m_processed)
            {
                return;
            }

            m_processed = true;
            // strip trailing newlines and whitespace:

            while ((!m_message.empty()) && (strchr("\n\r\t ", m_message[m_message.length() - 1])))
            {
                m_message.resize(m_message.length() - 1);
            }

            // detect richtext currently by presence of span at the beginning, or <a
            if (
                (azstrnicmp(m_message.c_str(), "<span ", 6) == 0) ||
                (azstrnicmp(m_message.c_str(), "<a ", 3) == 0)
                )
            {
                m_isRichText = true;
            }

            // the message also provides a hint, until we have a form of context harvesting:
            if ((strncmp(m_message.c_str(), "W:", 2) == 0) || (azstrnicmp(m_message.c_str(), "warn:", 5) == 0) || (azstrnicmp(m_message.c_str(), "warning:", 8) == 0))
            {
                m_type = TYPE_WARNING;
            }
            else if ((strncmp(m_message.c_str(), "E:", 2) == 0) || (azstrnicmp(m_message.c_str(), "err:", 4) == 0) || (azstrnicmp(m_message.c_str(), "error:", 6) == 0))
            {
                m_type = TYPE_ERROR;
            }
            else if ((strncmp(m_message.c_str(), "C:", 2) == 0) || (azstrnicmp(m_message.c_str(), "ctx:", 4) == 0) || (azstrnicmp(m_message.c_str(), "context:", 8) == 0))
            {
                m_type = TYPE_CONTEXT;
            }

            // note that we don't set rich text to false here.  The user is welcome to set that to true before calling add.
            // if we have not yet escalated and its rich text, then see if the rich text escalates it:
            // we don't bother checking if the type is ALREADY not just normal or debug
            if ((m_isRichText) && ((m_type == TYPE_MESSAGE) || (m_type == TYPE_DEBUG)))
            {
                // the rich text allows us to add attributes the surrounding span
                // one of those attributes allows us to change the apparent severity level.
                using namespace AZ::rapidxml;
                xml_document<> doc;
                doc.parse<parse_non_destructive>(const_cast<char*>(m_message.c_str()));
                xml_node<>* cur_node = doc.first_node("span", 4, false);
                if (cur_node)
                {
                    xml_attribute<>* attrib = cur_node->first_attribute("severity", 8, false);
                    if (attrib)
                    {
                        AZStd::string attribValue(attrib->value(), attrib->value() + attrib->value_size());
                        if (azstrnicmp(attribValue.c_str(), "err", 3) == 0)
                        {
                            m_type = TYPE_ERROR;
                        }
                        else if (azstrnicmp(attribValue.c_str(), "warn", 4) == 0)
                        {
                            m_type = TYPE_WARNING;
                        }
                    }
                }
            }
        }

        const AZStd::string& LogLine::GetLogMessage() const
        {
            return m_message;
        }

        const AZStd::string& LogLine::GetLogWindow() const
        {
            return m_window;
        }

        LogLine::LogType LogLine::GetLogType() const
        {
            return m_type;
        }

        AZ::u64 LogLine::GetLogTime() const
        {
            return m_messageTime;
        }

        AZStd::string LogLine::ToString()
        {
            if (!m_processed)
            {
                Process();
            }

            using namespace AZ::Debug;

            const AZStd::string separator = " | ";
            const AZStd::string From = " from ";
            const AZStd::string EOL = "\n";

            AZStd::string dateTime = QDateTime::fromMSecsSinceEpoch(m_messageTime).toString(Qt::SystemLocaleShortDate).toUtf8().data();
            AZStd::string severity;
            switch (m_type)
            {
            case LogLine::TYPE_DEBUG:
                severity = "DEBG";
                break;
            case LogLine::TYPE_ERROR:
                severity = "EROR";
                break;
            case LogLine::TYPE_WARNING:
                severity = "Warn";
                break;
            case LogLine::TYPE_MESSAGE:
                severity = "Info";
                break;
            case LogLine::TYPE_CONTEXT:
                severity = "Context";
                break;
            }
            return dateTime + separator + severity + separator + m_message.c_str() + From + m_window.c_str() + EOL;
        }

        QVariant LogLine::data(int column, int role) const
        {
            using namespace AZ::Debug;
            if (role == LogPanel::ExtraRoles::LogLineRole)
            {
                return QVariant::fromValue<const LogLine*>(this);
            }
            if (role == LogPanel::ExtraRoles::RichTextRole) // the renderer is asking whether this cell is rich text or not.  Return a true or false.
            {
                return m_isRichText;
            }
            else if (role == Qt::DecorationRole) // the renderer is asking whether or not we want to display an icon in this cell.  Return an icon or null.
            {
                if (column == 0)
                {
                    switch (m_type)
                    {
                    case LogLine::TYPE_ERROR:
                        return LogPanel::BaseLogView::GetErrorIcon();
                    case LogLine::TYPE_WARNING:
                        return LogPanel::BaseLogView::GetWarningIcon();
                    case LogLine::TYPE_MESSAGE:
                        return LogPanel::BaseLogView::GetInformationIcon();
                    case LogLine::TYPE_DEBUG:
                        return LogPanel::BaseLogView::GetDebugIcon();
                    case LogLine::TYPE_CONTEXT:
                        return LogPanel::BaseLogView::GetInformationIcon();
                    }
                }
            }
            else if (role == Qt::DisplayRole) // the renderer wants to know what text to show in this cell.  Return a string or null
            {
                if (column == 0) // icon has no text
                {
                    return QVariant(QString());
                }
                if (column == 1)
                {
                    return QVariant(QDateTime::fromMSecsSinceEpoch(m_messageTime).toString(Qt::SystemLocaleShortDate));
                }
                else if (column == 2) // window
                {
                    return QVariant(m_window.c_str());
                }
                else if (column == 3) // message
                {
                    return QVariant(m_message.c_str());
                }
            }
            else if (role == Qt::BackgroundRole) // the renderer wants to know what the background color of this cell should be.  REturn a color or null (to use default)
            {
                switch (m_type)
                {
                case LogLine::TYPE_ERROR:
                    return QVariant(QColor::fromRgb(64, 0, 0));
                    break;
                case LogLine::TYPE_WARNING:
                    return QVariant(QColor::fromRgb(64, 64, 0));
                    break;
                case LogLine::TYPE_CONTEXT:
                    return QVariant(QColor::fromRgb(32, 0, 32));
                    break;
                }
            }
            else if (role == Qt::TextColorRole) // the renderer wants to know what the text color of this cell should be.  Return a color or empty QVariant()
            {
                switch (m_type)
                {
                case LogLine::TYPE_MESSAGE:
                    return QVariant(QColor::fromRgb(255, 255, 255));
                case LogLine::TYPE_ERROR:
                    return QVariant(QColor::fromRgb(255, 192, 192));
                case LogLine::TYPE_WARNING:
                    return QVariant(QColor::fromRgb(255, 255,192));
                case LogLine::TYPE_DEBUG:
                    return QVariant(QColor::fromRgb(128, 128, 128));
                case LogLine::TYPE_CONTEXT:
                    return QVariant(QColor::fromRgb(255, 192, 255));
                }
            }
            return QVariant();
        }
        
        bool LogLine::ParseLog(const char* entireLog, AZ::u64 logLength, AZStd::function<void(LogLine&)> logLineAdder)
        {
            // default log file parse assumes newline separated.

            if (logLength < 1)
            {
                return false;
            }

            AZStd::string parseBuffer;

            const char* start = entireLog;
            AZ::u64 pos = 0;
            AZ::u64 numChars = 0;
            while (pos < logLength)
            {
                char currentChar = entireLog[pos];
                if ((currentChar == '\n') || (currentChar == '\r') || (pos == (logLength - 1)))
                {
                    if (numChars > 0)
                    {
                        // we have found a newline or EOF.
                        parseBuffer.assign(start, numChars);

                        LogLine output;
                        if (ParseLine(output, parseBuffer))
                        {
                            logLineAdder(output);
                        }
                    }
                    numChars = 0;
                    ++pos;
                    start = entireLog + pos;
                }
                else
                {
                    ++pos;
                    ++numChars;
                }
            }

            return true;
        }

        bool LogLine::ParseLog(AZStd::list<LogLine>& logList,  const char* entireLog, AZ::u64 logLength)
        {
            return ParseLog(entireLog, logLength, 
                [&](LogLine& logLine)
                {
                    logList.push_back(logLine);
                }
            );
        }

        bool LogLine::ParseLine(LogLine& outLine, const AZStd::string& line)
        {
            // the standard log outputs (if machine readable format) to 
            // ~~MS_SINCE_EPOCH~~SEVERITY~~THREADID~~WINDOW~~MESSAGE
            // we can always add additional parsers here, or parse modes, etc.
            // we're also going to assume if it doesn't have tags like that, its a plain message log line
            // for speed, we'll manually walk through the line ourselves.
            // we also want to avoid actually allocating memory here on the heap.


            size_t lineSize = line.size();
            if (lineSize == 0)
            {
                // this can't be a valid line.
                return false;
            }

            // pass over it once to see if it has all the required fields:

            // TIME     SEV      THREAD   WINDOW   MESSAGE

            enum FieldIndexes
            {
                Idx_Time,
                Idx_Severity,
                Idx_Thread,
                Idx_Window,
                Idx_Message,
                NUM_FIELD_INDEXES,
            };

            size_t fieldBegins[NUM_FIELD_INDEXES];
            size_t fieldSizes[NUM_FIELD_INDEXES];
            int fieldIndex = 0;

            memset(fieldBegins, 0, sizeof(size_t) * NUM_FIELD_INDEXES);
            memset(fieldSizes, 0, sizeof(size_t) * NUM_FIELD_INDEXES);
            
            if (strncmp(line.c_str(), "~~", 2) == 0)
            {
                // its probably in the delimited format here:
                
                size_t fieldStartPos = 2; // start after the opening ~'s
                bool inDelimiter = false;
                for (size_t pos = 2; pos < lineSize - 1; ++pos)
                {
                    if (fieldIndex >= NUM_FIELD_INDEXES)
                    {
                        break; // found all the fields.
                    }
                    char currentChar = line[pos];

                    if (currentChar == '~')
                    {
                        // this could be a delimiter.
                        if (inDelimiter)
                        {
                            // this is the second delimiter char in a row, so its the end delimiter.
                            // current field is over! 
                            fieldSizes[fieldIndex] = (pos - fieldStartPos) - 1;
                            fieldBegins[fieldIndex] = fieldStartPos;
                            ++fieldIndex;
                            fieldStartPos = pos + 1;
                            inDelimiter = false;

                            // if we just moved on to the last field, which is the message, its just the rest of the string...
                            if (fieldIndex == Idx_Message)
                            {
                                fieldBegins[fieldIndex] = fieldStartPos;
                                fieldSizes[fieldIndex] = lineSize - fieldStartPos; // the rest of the line.
                                break;
                            }
                        }
                        else
                        {
                            inDelimiter = true;
                        }
                    }
                    else
                    {
                        inDelimiter = false;
                        // some other char.
                    }
                }
            }

            const int maxFieldSize = 80;

            // if everything went okay with parsing:
            if ((fieldBegins[Idx_Message] != 0) && (fieldSizes[Idx_Time] < maxFieldSize) && (fieldSizes[Idx_Severity] < maxFieldSize) && (fieldSizes[Idx_Window] < maxFieldSize))
            {
                char tempScratch[maxFieldSize];
                azstrncpy(tempScratch, AZ_ARRAY_SIZE(tempScratch), line.c_str() + fieldBegins[Idx_Time], fieldSizes[Idx_Time]);
                tempScratch[fieldSizes[Idx_Time]] = 0;

                bool convertedOK = false;
                AZ::u64 msSinceEpoch = QString(tempScratch).toULongLong(&convertedOK, 10);
                if (convertedOK)
                {
                    outLine.m_messageTime = msSinceEpoch;
                }
                else
                {
                    outLine.m_messageTime = QDateTime::currentDateTime().toMSecsSinceEpoch();
                }
                
                azstrncpy(tempScratch, AZ_ARRAY_SIZE(tempScratch), line.c_str() + fieldBegins[Idx_Severity], fieldSizes[Idx_Severity]);
                tempScratch[fieldSizes[Idx_Severity]] = 0;
                int severityValue = QString(tempScratch).toInt(&convertedOK, 10);
                if (convertedOK)
                {
                    switch (severityValue)
                    {
                    case AzFramework::LogFile::SEV_DEBUG:
                        outLine.m_type = Logging::LogLine::TYPE_DEBUG;
                        break;
                    case AzFramework::LogFile::SEV_NORMAL:
                        outLine.m_type = Logging::LogLine::TYPE_MESSAGE;
                        break;
                    case AzFramework::LogFile::SEV_WARNING:
                        outLine.m_type = Logging::LogLine::TYPE_WARNING;
                        break;
                    default:
                        outLine.m_type = Logging::LogLine::TYPE_ERROR;
                        break;
                    }
                }

                outLine.m_window.assign(line.c_str() + fieldBegins[Idx_Window], fieldSizes[Idx_Window]);
                outLine.m_message.assign(line.c_str() + fieldBegins[Idx_Message], fieldSizes[Idx_Message]);
                outLine.Process();
                return true;
            }
            else
            {
                // assume some random unknown format log message line.
                outLine.m_messageTime = QDateTime::currentDateTime().toMSecsSinceEpoch();
                outLine.m_message = line.c_str();
                outLine.m_window = "Message";
                outLine.Process();
                return true;
            }
        }
    }
}