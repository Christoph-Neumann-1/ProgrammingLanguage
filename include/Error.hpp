#pragma once

#include <string>
#include <vector>
#include <optional>
#include <optional>
#include <Logger.hpp>

namespace VWA
{

//TODO: test this and use in parser
#define TRY(x)                          \
    (                                   \
        {                               \
            auto tmp = (x);             \
            if (!tmp)                   \
                return tmp.moveError(); \
            tmp.moveValue();            \
        })

    struct Error
    {
        std::string message;
        std::optional<std::string> suggestion;
        std::optional<std::string> file;
        uint line;
        std::vector<std::string> backtrace;

        void log(ILogger &logger)
        {
            logger << ILogger::Error << "Error: " << message;
            if (suggestion)
                logger << "\nSuggestion: " << *suggestion;
            if (file)
                logger << "\nAt:" << *file << ':' << std::to_string(line);
            if (backtrace.size())
            {
                logger << "\nBacktrace:";
                for (auto &i : backtrace)
                    logger << "\n"
                           << i;
            }
            logger << ILogger::FlushNewLine;
        }

        //TODO: function for adding context
    };
    template <typename T>
    struct ErrorOr
    {
        std::optional<T> value;
        std::optional<Error> error;
        bool errorState;

    public:
        bool isError() const
        {
            return errorState;
        }

        operator bool() const
        {
            return !errorState;
        }

        //Throws an exception if it contains an error
        const T &getValue() const
        {
            if (errorState)
                throw std::runtime_error("ErrorOr is in error state");
            return *value;
        }

        T &getValue()
        {
            if (errorState)
                throw std::runtime_error("ErrorOr is in error state");
            return *value;
        }

        T &&moveValue()
        {
            if (errorState)
                throw std::runtime_error("ErrorOr is in error state");
            return std::move(*value);
        }

        const Error &getError() const
        {
            if (!errorState)
                throw std::runtime_error("ErrorOr is not in error state");
            return *error;
        }

        Error &getError()
        {
            if (!errorState)
                throw std::runtime_error("ErrorOr is not in error state");
            return *error;
        }

        Error &&moveError()
        {
            if (!errorState)
                throw std::runtime_error("ErrorOr is not in error state");
            return std::move(*error);
        }

        //TODO: implicit constructors
        ErrorOr(const T &value) requires std::is_copy_constructible_v<T>
            : value(value), errorState(false) {}
        ErrorOr(T &&value) requires std::is_move_constructible_v<T>
            : value(std::move(value)), errorState(false) {}
        ErrorOr(const Error &error)
            : error(error), errorState(true) {}
        ErrorOr(Error &&error)
            : error(std::move(error)), errorState(true) {}
        //TODO: measure performance and remove copies of empty optional if necessary
        ErrorOr(const ErrorOr &other) requires std::is_copy_constructible_v<T>
            : value(other.value), error(other.error), errorState(other.errorState) {}
        ErrorOr(ErrorOr &&other) requires std::is_move_constructible_v<T>
            : value(std::move(other.value)), error(std::move(other.error)), errorState(other.errorState) {}
        ErrorOr &operator=(const ErrorOr &other) requires std::is_copy_assignable_v<T>
        {
            value = other.value;
            error = other.error;
            errorState = other.errorState;
            return *this;
        }
        ErrorOr &operator=(ErrorOr &&other) requires std::is_move_assignable_v<T>
        {
            value = std::move(other.value);
            error = std::move(other.error);
            errorState = other.errorState;
            return *this;
        }
    };
}