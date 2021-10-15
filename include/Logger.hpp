#pragma once
#include <File.hpp>
#include <sstream>
#include <iostream>

namespace VWA
{
    class ILogger
    {
    public:
        enum LogLevel
        {
            Debug,
            Info,
            Warning,
            Error,
            Flush,
        };

        virtual ~ILogger() = default;

        ILogger() {}
        ILogger(const ILogger &) = delete;
        ILogger &operator=(const ILogger &) = delete;

        virtual ILogger &operator<<(const std::string &) = 0;
        virtual ILogger &operator<<(const char *) = 0;
        virtual ILogger &operator<<(const int) = 0;
        virtual ILogger &operator<<(const unsigned int) = 0;
        virtual ILogger &operator<<(const long) = 0;
        virtual ILogger &operator<<(const unsigned long) = 0;
        virtual ILogger &operator<<(const float) = 0;
        virtual ILogger &operator<<(const double) = 0;
        virtual ILogger &operator<<(const bool) = 0;
        virtual ILogger &operator<<(const LogLevel) = 0;
        virtual ILogger &operator<<(const Line &) = 0;

        virtual ILogger &AtPos(const Line &line) = 0;
    };

    class ConsoleLogger : public ILogger
    {
        std::stringstream m_stream;
        LogLevel m_level = LogLevel::Info;

        template <typename T>
        void write(const T &value)
        {
            m_stream << value;
        }

        void Flush()
        {
            std::string output = m_stream.str();
            if (output.empty())
                return;
            std::cout << '[' << static_cast<int>(m_level) << "]: " << output;
            m_stream.str("");
            std::cout.flush();
        }

    public:
        ~ConsoleLogger()
        {
            Flush();
        }

        ConsoleLogger &operator<<(const std::string &message) override
        {
            write(message);
            return *this;
        }
        ConsoleLogger &operator<<(const char *message) override
        {
            write(message);
            return *this;
        }
        ConsoleLogger &operator<<(const int val) override
        {
            write(val);
            return *this;
        }
        ConsoleLogger &operator<<(const unsigned int val) override
        {
            write(val);
            return *this;
        }
        ConsoleLogger &operator<<(const long val) override
        {
            write(val);
            return *this;
        }
        ConsoleLogger &operator<<(const unsigned long val) override
        {
            write(val);
            return *this;
        }
        ConsoleLogger &operator<<(const float val) override
        {
            write(val);
            return *this;
        }
        ConsoleLogger &operator<<(const double val) override
        {
            write(val);
            return *this;
        }
        ConsoleLogger &operator<<(const bool val) override
        {
            write(val);
            return *this;
        }
        ConsoleLogger &operator<<(const LogLevel newLevel) override
        {
            if (m_level == newLevel)
                return *this;

            if (newLevel == LogLevel::Flush)
            {
                Flush();
                return *this;
            }

            Flush();
            m_level = newLevel;
            return *this;
        }
        ConsoleLogger &operator<<(const Line &line) override
        {
            AtPos(line);
            write(line.content);
            return *this;
        }
        ConsoleLogger &AtPos(const Line &line) override
        {
            m_stream << '[' << *line.fileName << ':' << line.lineNumber << "] ";
            return *this;
        }
    };
}