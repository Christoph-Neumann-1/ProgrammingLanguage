#pragma once
#include <string_view>
#include <File.hpp>
#include <iostream>
#include <fstream>

namespace VWA
{
    enum class LogLevel
    {
        Debug,
        Info,
        Warning,
        Error,
        Fatal
    };
    class Logger
    {
    protected:
        LogLevel currentLogLevel = LogLevel::Info;

    public:
        virtual ~Logger() {}
        virtual void log(std::string_view message, LogLevel level = LogLevel::Info) = 0;
        virtual void log(const Line &line, std::string_view message, LogLevel level = LogLevel::Info) = 0;
        void setLogLevel(LogLevel level)
        {
            currentLogLevel = level;
        }
        LogLevel getLogLevel()
        {
            return currentLogLevel;
        }
    };

    class ConsoleLogger : public Logger
    {
    public:
        void log(std::string_view message, LogLevel level = LogLevel::Info) override
        {
            if (level >= currentLogLevel)
            {
                std::cout << message << '\n';
            }
        }
        void log(const Line &line, std::string_view message, LogLevel level = LogLevel::Info) override
        {
            if (level >= currentLogLevel)
            {
                std::cout << "[" << *line.fileName << ":" << line.lineNumber << "] " << message << '\n';
            }
        }
    };

    class FileLogger : public Logger
    {
        std::ofstream file;

    public:
        FileLogger(const std::string &fileName)
        {
            file.open(fileName);
        }
        ~FileLogger()
        {
            file.close();
        }
        void log(std::string_view message, LogLevel level = LogLevel::Info) override
        {
            if (level >= currentLogLevel)
            {
                file << message << '\n';
            }
        }
        void log(const Line &line, std::string_view message, LogLevel level = LogLevel::Info) override
        {
            if (level >= currentLogLevel)
            {
                file << "[" << *line.fileName << ":" << line.lineNumber << "] " << message << '\n';
            }
        }
    };
}