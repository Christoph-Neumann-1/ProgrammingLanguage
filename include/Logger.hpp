#pragma once
#include <File.hpp>
#include <sstream>
#include <iostream>
#include <fstream>
#include <memory>

//TODO: File Logger, direct certain Messages to other Logger, only print above LogLevel;
//TODO: consider rewriting with templates instead of polymorphism

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
            FlushNewLine,
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

        virtual void SetMinimumLogLevel(const LogLevel level)
        {
            minimumLogLevel = level;
        }

    protected:
        LogLevel minimumLogLevel = LogLevel::Debug;
    };

    class BasicLogger : public ILogger
    {
    protected:
        std::stringstream m_stream;
        LogLevel m_level = LogLevel::Info;

        template <typename T>
        void write(const T &value)
        {
            if (m_level >= minimumLogLevel)
                m_stream << value;
        }

        virtual void Flush(bool newline = false) = 0;

    public:
        virtual ~BasicLogger() = default;

        BasicLogger &operator<<(const std::string &message) override
        {
            write(message);
            return *this;
        }
        BasicLogger &operator<<(const char *message) override
        {
            write(message);
            return *this;
        }
        BasicLogger &operator<<(const int val) override
        {
            write(val);
            return *this;
        }
        BasicLogger &operator<<(const unsigned int val) override
        {
            write(val);
            return *this;
        }
        BasicLogger &operator<<(const long val) override
        {
            write(val);
            return *this;
        }
        BasicLogger &operator<<(const unsigned long val) override
        {
            write(val);
            return *this;
        }
        BasicLogger &operator<<(const float val) override
        {
            write(val);
            return *this;
        }
        BasicLogger &operator<<(const double val) override
        {
            write(val);
            return *this;
        }
        BasicLogger &operator<<(const bool val) override
        {
            write(val);
            return *this;
        }
        BasicLogger &operator<<(const LogLevel newLevel) override
        {
            if (m_level == newLevel)
                return *this;

            if (newLevel == LogLevel::Flush || newLevel == LogLevel::FlushNewLine)
            {
                Flush(LogLevel::FlushNewLine == newLevel);
                return *this;
            }

            Flush();
            m_level = newLevel;
            return *this;
        }
        BasicLogger &operator<<(const Line &line) override
        {
            AtPos(line);
            write(line.content);
            return *this;
        }
        BasicLogger &AtPos(const Line &line) override
        {
            if (m_level >= minimumLogLevel)
                m_stream << '[' << *line.fileName << ':' << line.lineNumber << "] ";
            return *this;
        }
    };

    class ConsoleLogger : public BasicLogger
    {
        void Flush(bool newline = false) override
        {
            if (newline)
            {
                m_stream << '\n';
            }
            std::string output = m_stream.str();
            if (output.empty())
                return;
            auto &out = m_level == LogLevel::Error ? std::cerr : std::cout;
            out << output;
            m_stream.str("");
            out.flush();
        }

    public:
        ~ConsoleLogger() { Flush(true); }
    };

    class FileLogger : public BasicLogger
    {
        std::ofstream m_file;
        void Flush(bool newline = false) override
        {
            if (newline)
            {
                m_stream << '\n';
            }
            std::string output = m_stream.str();
            if (output.empty())
                return;
            if (!m_file.is_open())
            {
                m_file.open("log.txt");
                m_file << "ERROR: LogFile not open! Creating default file." << std::endl;
            }
            m_file << output;
            m_stream.str("");
        }

    public:
        FileLogger(std::string_view fileName = "log.txt")
        {
            m_file.open(fileName.data());
        }
        ~FileLogger() { Flush(true); }
    };

    class MultiOutputLogger : public BasicLogger
    {
        //Because more than one of them might point to the same Logger, I used a shared pointer. This also allows for multiple
        //MultiOutputLoggers to share members without having to worry about their lifetime.
        std::shared_ptr<ILogger> DebugOut, InfoOut, WarningOut, ErrorOut;

        void Flush(bool newline = false) override
        {
            auto output = m_stream.str();

            auto flush = newline ? LogLevel::FlushNewLine : LogLevel::Flush;

            if (m_level == Debug)
            {
                if (!DebugOut)
                    throw std::runtime_error("DebugOut not set!");
                *DebugOut << output << flush;
            }
            else if (m_level == Info)
            {
                if (!InfoOut)
                    throw std::runtime_error("InfoOut not set!");
                *InfoOut << output << flush;
            }
            else if (m_level == Warning)
            {
                if (!WarningOut)
                    throw std::runtime_error("WarningOut not set!");
                *WarningOut << output << flush;
            }
            else if (m_level == Error)
            {
                if (!ErrorOut)
                    throw std::runtime_error("ErrorOut not set!");
                *ErrorOut << output << flush;
            }
        }

    public:
        MultiOutputLogger(const std::shared_ptr<ILogger> &debugOut, const std::shared_ptr<ILogger> &infoOut, const std::shared_ptr<ILogger> &warningOut, const std::shared_ptr<ILogger> &errorOut)
            : DebugOut(debugOut), InfoOut(infoOut), WarningOut(warningOut), ErrorOut(errorOut)
        {
        }
        MultiOutputLogger() = default;
        ~MultiOutputLogger() { Flush(true); }
        void SetDebugOut(const std::shared_ptr<ILogger> &debugOut)
        {
            DebugOut = debugOut;
        }
        void SetInfoOut(const std::shared_ptr<ILogger> &infoOut)
        {
            InfoOut = infoOut;
        }
        void SetWarningOut(const std::shared_ptr<ILogger> &warningOut)
        {
            WarningOut = warningOut;
        }
        void SetErrorOut(const std::shared_ptr<ILogger> &errorOut)
        {
            ErrorOut = errorOut;
        }
    };

    class VoidLogger : public ILogger
    {
        VoidLogger &operator<<(const std::string &) override { return *this; }
        VoidLogger &operator<<(const char *) override { return *this; }
        VoidLogger &operator<<(const int) override { return *this; }
        VoidLogger &operator<<(const unsigned int) override { return *this; }
        VoidLogger &operator<<(const long) override { return *this; }
        VoidLogger &operator<<(const unsigned long) override { return *this; }
        VoidLogger &operator<<(const float) override { return *this; }
        VoidLogger &operator<<(const double) override { return *this; }
        VoidLogger &operator<<(const bool) override { return *this; }
        VoidLogger &operator<<(const LogLevel) override { return *this; }
        VoidLogger &operator<<(const Line &) override { return *this; }

        VoidLogger &AtPos(const Line &line) override { return *this; }
    };
}