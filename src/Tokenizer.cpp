#include <Tokenizer.hpp>
namespace VWA
{

    Token ParseNumber(const std::string &str, size_t &pos)
    {
        size_t start = pos;
        //TODO: support scientific notation
        //TODO: unary minus
        //First find the end of the number
        enum NumType
        {
            INT,
            LONG,
            FLOAT,
            DOUBLE
        };
        uint nDots = 0;
        size_t end;
        //TODO: consider removing feature: if the last char is a dot, it is equal to .0
        for (end = pos; end < str.size(); ++end)
        {
            if (str[end] == '.')
            {
                if (end != str.size() - 1)
                {
                    if (str[end + 1] == '.')
                    {
                        break;
                    }
                }
                nDots++;
                if (nDots > 1)
                {
                    throw std::runtime_error("Invalid number");
                }
            }
            else if (!isdigit(str[end]))
            {
                break;
            }
        }
        char endChar = end == str.size() ? '\0' : str[end];
        NumType type;
        if (nDots)
        {
            if (endChar == 'f')
            {
                type = FLOAT;
                pos = end;
            }
            else
            {
                type = DOUBLE;
                pos = end - 1;
            }
        }
        else
        {
            if (endChar == 'l')
            {
                type = LONG;
                pos = end;
            }
            else
            {
                type = INT;
                pos = end - 1;
            }
        }
        switch (type)
        {
        case INT:
            return Token{.type = TokenType::int_, .value = std::stoi(str.substr(start, end - start))};
        case LONG:
            return Token{.type = TokenType::long_, .value = std::stol(str.substr(start, end - start))};
        case FLOAT:
            return Token{.type = TokenType::float_, .value = std::stof(str.substr(start, end - start))};
        case DOUBLE:
            return Token{.type = TokenType::double_, .value = std::stod(str.substr(start, end - start))};
        }
        throw std::runtime_error("Invalid number");
    }

    char GetEscaped(char char_)
    {
        switch (char_)
        {
        case 'n':
            return '\n';
        case 't':
            return '\t';
        case '\\':
            return '\\';
        case '\'':
            return '\'';
        case '"':
            return '"';
        }
        throw std::runtime_error("Invalid escape sequence");
    }

    Token ParseString(std::string str, size_t &pos)
    {
        //First find the next unescaped quote
        size_t start = pos;
        ++pos;
        for (size_t i = pos; i < str.size(); ++pos)
        {
            switch (str[i])
            {
            case '"':
                return Token{.type = TokenType::string_literal, .value = str.substr(start + 1, i - start - 1)};
            case '\\':
                //TODO: more escape sequences
                //TODO: error when escape is last char
                str.replace(i, 2, 1, GetEscaped(str[i + 1]));
                ++pos;
            default:
                ++i;
            }
        }
        throw std::runtime_error("Unterminated string");
    }

    Token ParseChar(std::string str, size_t &pos)
    {
        bool containsEscaped = str[pos + 1] == '\\';
        char content = containsEscaped ? GetEscaped(str[pos + 2]) : str[pos + 1];
        if (containsEscaped)
        {
            if (str.size() <= pos + 3)
            {
                throw std::runtime_error("Unterminated char");
            }
            if (str[pos + 3] != '\'')
            {
                throw std::runtime_error("Invalid char");
            }
            pos += 3;
        }
        else
        {
            if (str.size() <= pos + 2)
            {
                throw std::runtime_error("Unterminated char");
            }
            if (str[pos + 2] != '\'')
            {
                throw std::runtime_error("Invalid char");
            }
            pos += 2;
        }
        return Token{.type = TokenType::char_, .value = content};
    }

    std::string ParseIdentifier(const std::string &str, size_t &pos)
    {
        if (!isalpha(str[pos]))
        {
            throw std::runtime_error("Invalid identifier");
        }
        size_t start = pos;
        for (size_t end=pos+1; end <= str.size(); ++end)
        {
            if ((!isalnum(str[end]) && str[end] != '_') || end == str.size())
            {
                pos = end-1;
                return str.substr(start, end - start);
            }
        }
        throw std::runtime_error("Invalid identifier");
    }

    std::vector<Token> Tokenizer(File file)
    {
        //TODO: use emplace_back
        std::vector<Token> tokens;
        for (auto &line : file)
        {
            for (size_t pos = 0; pos < line.content.length(); ++pos)
            {
                switch (line.content[pos])
                {
                case ' ':
                case '\t':
                    continue;
                case '"':
                    tokens.push_back(ParseString(line.content, pos));
                    tokens.back().line = line.lineNumber;
                    tokens.back().file = line.fileName;
                    continue;
                case '\'':
                    tokens.push_back(ParseChar(line.content, pos));
                    tokens.back().line = line.lineNumber;
                    tokens.back().file = line.fileName;
                    continue;
                case '(':
                    tokens.push_back(Token{.type = TokenType::lparen, .file = line.fileName, .line = line.lineNumber});
                    continue;
                case ')':
                    tokens.push_back(Token{.type = TokenType::rparen, .file = line.fileName, .line = line.lineNumber});
                    continue;
                case '{':
                    tokens.push_back(Token{.type = TokenType::lbrace, .file = line.fileName, .line = line.lineNumber});
                    continue;
                case '}':
                    tokens.push_back(Token{.type = TokenType::rbrace, .file = line.fileName, .line = line.lineNumber});
                    continue;
                case '[':
                    tokens.push_back(Token{.type = TokenType::lbracket, .file = line.fileName, .line = line.lineNumber});
                    continue;
                case ']':
                    tokens.push_back(Token{.type = TokenType::rbracket, .file = line.fileName, .line = line.lineNumber});
                    continue;
                case ';':
                    tokens.push_back(Token{.type = TokenType::semicolon, .file = line.fileName, .line = line.lineNumber});
                    continue;
                case ',':
                    tokens.push_back(Token{.type = TokenType::comma, .file = line.fileName, .line = line.lineNumber});
                    continue;
                case ':':
                    if (line.content.size() > pos + 1)
                    {
                        if (line.content[pos + 1] == ':')
                        {
                            tokens.push_back(Token{.type = TokenType::double_colon, .file = line.fileName, .line = line.lineNumber});
                            ++pos;
                            continue;
                        }
                    }
                    tokens.push_back(Token{.type = TokenType::colon, .file = line.fileName, .line = line.lineNumber});
                    continue;
                case '+':
                    tokens.push_back(Token{.type = TokenType::plus, .file = line.fileName, .line = line.lineNumber});
                    continue;
                case '-':
                    if (line.content.size() > pos + 1)
                    {
                        if (line.content[pos + 1] == '>')
                        {
                            tokens.push_back(Token{.type = TokenType::arrow_operator, .file = line.fileName, .line = line.lineNumber});
                            ++pos;
                            continue;
                        }
                    }
                    tokens.push_back(Token{.type = TokenType::minus, .file = line.fileName, .line = line.lineNumber});
                    continue;
                case '*':
                    if(line.content.size() > pos + 1)
                    {
                        if (line.content[pos + 1] == '*')
                        {
                            tokens.push_back(Token{.type = TokenType::power, .file = line.fileName, .line = line.lineNumber});
                            ++pos;
                            continue;
                        }
                    }
                    tokens.push_back(Token{.type = TokenType::star, .file = line.fileName, .line = line.lineNumber});
                    continue;
                case '/':
                    tokens.push_back(Token{.type = TokenType::divide, .file = line.fileName, .line = line.lineNumber});
                    continue;
                case '%':
                    tokens.push_back(Token{.type = TokenType::mod, .file = line.fileName, .line = line.lineNumber});
                    continue;
                case '&':
                    if (line.content.size() > pos + 1)
                    {
                        if (line.content[pos + 1] == '&')
                        {
                            tokens.push_back(Token{.type = TokenType::and_, .file = line.fileName, .line = line.lineNumber});
                            ++pos;
                            continue;
                        }
                    }
                    tokens.push_back(Token{.type = TokenType::ampersand, .file = line.fileName, .line = line.lineNumber});
                    continue;
                case '|':
                    if (line.content.size() > pos + 1)
                    {
                        if (line.content[pos + 1] == '|')
                        {
                            tokens.push_back(Token{.type = TokenType::or_, .file = line.fileName, .line = line.lineNumber});
                            ++pos;
                            continue;
                        }
                    }
                    throw std::runtime_error("Invalid operator");
                case '!':
                    if (line.content.size() > pos + 1)
                    {
                        if (line.content[pos + 1] == '=')
                        {
                            tokens.push_back(Token{.type = TokenType::neq, .file = line.fileName, .line = line.lineNumber});
                            ++pos;
                            continue;
                        }
                    }
                    tokens.push_back(Token{.type = TokenType::not_, .file = line.fileName, .line = line.lineNumber});
                    continue;
                case '=':
                    if (line.content.size() > pos + 1)
                    {
                        if (line.content[pos + 1] == '=')
                        {
                            tokens.push_back(Token{.type = TokenType::eq, .file = line.fileName, .line = line.lineNumber});
                            ++pos;
                            continue;
                        }
                    }
                    tokens.push_back(Token{.type = TokenType::assign, .file = line.fileName, .line = line.lineNumber});
                    continue;
                case '<':
                    if (line.content.size() > pos + 1)
                    {
                        if (line.content[pos + 1] == '=')
                        {
                            tokens.push_back(Token{.type = TokenType::leq, .file = line.fileName, .line = line.lineNumber});
                            ++pos;
                            continue;
                        }
                    }
                    tokens.push_back(Token{.type = TokenType::lt, .file = line.fileName, .line = line.lineNumber});
                    continue;
                case '>':
                    if (line.content.size() > pos + 1)
                    {
                        if (line.content[pos + 1] == '=')
                        {
                            tokens.push_back(Token{.type = TokenType::geq, .file = line.fileName, .line = line.lineNumber});
                            ++pos;
                            continue;
                        }
                    }
                    tokens.push_back(Token{.type = TokenType::gt, .file = line.fileName, .line = line.lineNumber});
                    continue;
                case '.':
                    if (!isdigit(line.content[pos + 1]))
                    {
                        if (line.content.size() > pos + 1)
                        {
                            if (line.content[pos + 1] == '.')
                            {
                                tokens.push_back(Token{.type = TokenType::range_operator, .file = line.fileName, .line = line.lineNumber});
                                ++pos;
                                continue;
                            }
                        }
                        tokens.push_back(Token{.type = TokenType::dot, .file = line.fileName, .line = line.lineNumber});
                        continue;
                    }
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    tokens.push_back(ParseNumber(line.content, pos));
                    tokens.back().line = line.lineNumber;
                    tokens.back().file = line.fileName;
                    continue;
                default:
                    auto identifier = ParseIdentifier(line.content, pos);
                    if (auto it = keywords.find(identifier); it != keywords.end())
                    {
                        if (it->first == "true")
                        {
                            tokens.push_back(Token{.type = TokenType::bool_, .value = true, .file = line.fileName, .line = line.lineNumber});
                        }
                        else if (it->first == "false")
                        {
                            tokens.push_back(Token{.type = TokenType::bool_, .value = false, .file = line.fileName, .line = line.lineNumber});
                        }
                        else
                            tokens.push_back(Token{.type = it->second, .file = line.fileName, .line = line.lineNumber});
                    }
                    else
                    {
                        tokens.push_back(Token{.type = TokenType::identifier, .value = std::move(identifier), .file = line.fileName, .line = line.lineNumber});
                    }
                }
            }
        }
        tokens.push_back(Token{.type = TokenType::eof, .file = tokens.back().file, .line =  tokens.back().line});
        return tokens;
    }

}
