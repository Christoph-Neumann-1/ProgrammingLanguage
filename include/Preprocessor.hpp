#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <stdexcept>

namespace VWA
{
    namespace
    {
        struct Macro
        {
            std::string body;
        };
    }
    std::string preprocess(std::string input)
    {
        std::unordered_map<std::string, Macro> macros;

        auto currentMacroDefine = input.find("MACRO");
        while (currentMacroDefine != std::string::npos)
        {
            auto endMacro = input.find("ENDMACRO", currentMacroDefine);
            if (endMacro == std::string_view::npos)
            {
                throw std::runtime_error("ENDMACRO not found");
            }
            auto macroNextLine = input.find('\n', currentMacroDefine);
            std::string macroName(input.substr(currentMacroDefine + 6, macroNextLine - currentMacroDefine - 6));
            macroName.erase(std::remove(macroName.begin(), macroName.end(), ' '), macroName.end());
            auto macroBody = input.substr(macroNextLine + 1, endMacro - macroNextLine - 2);
            macros[macroName] = Macro{.body = std::string(macroBody)};
            auto nextNewLine = input.find('\n', endMacro);
            input.replace(currentMacroDefine, nextNewLine - currentMacroDefine + 1, "");
            currentMacroDefine = input.find("MACRO");
        }
        for (auto currentMacro = input.find('#'); currentMacro != std::string::npos; currentMacro = input.find('#', currentMacro))
        {
            if (input[currentMacro - 1] == '\\')
            {
                currentMacro++;
                continue;
            }
            bool macroHasArgs = true;
            size_t macroNameEnd = input.find('(', currentMacro);
            if (macroNameEnd == std::string::npos)
            {
                macroNameEnd = input.find('\n', currentMacro);
                macroHasArgs = false;
            }
            if (macroNameEnd == std::string::npos)
            {
                macroNameEnd = input.find(' ');
            }
            if (macroNameEnd == std::string::npos)
            {
                macroNameEnd = input.size();
            }
            std::string macroName(input.substr(currentMacro + 1, macroNameEnd - currentMacro - 1));
            macroName.erase(std::remove(macroName.begin(), macroName.end(), ' '), macroName.end());
            auto macro = macros.find(macroName);
            if (macro == macros.end())
            {
                throw std::runtime_error("Macro not found " + macroName);
            }
            if (!macroHasArgs)
            {
                input.replace(currentMacro, macroNameEnd - currentMacro + 1, macro->second.body);
            }
            else
            {
                auto macroArgsLast = input.find(')', currentMacro) - 1;
                while(input[macroArgsLast-1] == '\\')
                {
                    macroArgsLast=input.find(')', macroArgsLast+1)-1;
                }
                if (macroArgsLast == std::string_view::npos)
                {
                    throw std::runtime_error("Macro arguments end not found");
                }
                std::vector<std::string> macroArgs;
                auto macroArgsStart = macroNameEnd + 1;
                auto macroArgsCurrent = macroArgsStart;
                for (size_t i = macroArgsStart; i <= macroArgsLast; i++)
                {
                    if (input[i] == ',')
                    {
                        if (!(i > macroArgsStart && input[i - 1] == '\\'))
                        {
                            macroArgs.push_back(input.substr(macroArgsCurrent, i - macroArgsCurrent));
                            macroArgsCurrent = i + 1;
                        }
                    }
                }
                macroArgs.push_back(input.substr(macroArgsCurrent, macroArgsLast - macroArgsCurrent+1));
                std::string macroBody = macro->second.body;
                for (size_t i = 0; i < macroArgs.size(); i++)
                {
                    auto macroArg = macroBody.find("$" + std::to_string(i));
                    while (macroArg != std::string_view::npos)
                    {
                        macroBody.replace(macroArg, 2, macroArgs[i]);
                        macroArg = macroBody.find("$" + std::to_string(i));
                    }
                }
                input.replace(currentMacro, macroArgsLast - currentMacro + 2, macroBody);
            }
        }
        return input;
    }
}