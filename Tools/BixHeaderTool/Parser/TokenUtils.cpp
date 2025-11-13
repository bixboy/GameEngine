#include "TokenUtils.h"

namespace BixTool
{
    bool IsIdentifierStart(char c)
    {
        return std::isalpha(static_cast<unsigned char>(c)) != 0 || c == '_';
    }

    bool IsIdentifierChar(char c)
    {
        return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_';
    }

    std::string Trim(std::string_view text)
    {
        std::size_t start = 0;
        std::size_t end = text.size();
        while (start < end && std::isspace(static_cast<unsigned char>(text[start])) != 0)
        {
            ++start;
        }
        while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1])) != 0)
        {
            --end;
        }
        return std::string{text.substr(start, end - start)};
    }


    //   =============================
    //   ========== Siking  ==========
    //   =============================

    std::size_t SkipWhitespaceAndComments(const std::string& text, std::size_t pos)
    {
        const std::size_t size = text.size();
        while (pos < size)
        {
            char c = text[pos];
            if (std::isspace(static_cast<unsigned char>(c)) != 0)
            {
                ++pos;
                continue;
            }

            if (c == '/' && pos + 1 < size)
            {
                if (text[pos + 1] == '/')
                {
                    pos += 2;
                    while (pos < size && text[pos] != '\n')
                    {
                        ++pos;
                    }
                    continue;
                }

                if (text[pos + 1] == '*')
                {
                    pos += 2;
                    while (pos + 1 < size && !(text[pos] == '*' && text[pos + 1] == '/'))
                    {
                        ++pos;
                    }
                    if (pos + 1 < size)
                    {
                        pos += 2;
                    }
                    continue;
                }
            }

            break;
        }

        return pos;
    }

    std::size_t SkipStringLiteral(const std::string& text, std::size_t pos)
    {
        const char quote = text[pos];
        ++pos;
        const std::size_t size = text.size();
        while (pos < size)
        {
            char c = text[pos];
            if (c == '\\')
            {
                pos += 2;
                continue;
            }
            if (c == quote)
            {
                ++pos;
                break;
            }
            ++pos;
        }
        return pos;
    }

    std::size_t SkipCharLiteral(const std::string& text, std::size_t pos)
    {
        return SkipStringLiteral(text, pos);
    }

    //   =============================
    //   ========== Tokens  ==========
    //   ============================= 

    std::string ReadToken(const std::string& text, std::size_t& pos)
    {
        pos = SkipWhitespaceAndComments(text, pos);
        if (pos >= text.size())
        {
            return {};
        }

        char c = text[pos];
        if (IsIdentifierStart(c))
        {
            const std::size_t start = pos;
            ++pos;
            while (pos < text.size() && IsIdentifierChar(text[pos]))
            {
                ++pos;
            }
            return text.substr(start, pos - start);
        }

        if (std::isdigit(static_cast<unsigned char>(c)) != 0)
        {
            const std::size_t start = pos;
            ++pos;
            while (pos < text.size() && std::isalnum(static_cast<unsigned char>(text[pos])) != 0)
            {
                ++pos;
            }
            return text.substr(start, pos - start);
        }

        if (c == ':' && pos + 1 < text.size() && text[pos + 1] == ':')
        {
            pos += 2;
            return "::";
        }

        ++pos;
        return std::string(1, c);
    }

    std::string PeekToken(const std::string& text, std::size_t pos)
    {
        return ReadToken(text, pos);
    }

    //   =============================
    //   ========== FINDERS ==========
    //   ============================= 

    std::size_t FindMatchingBrace(const std::string& text, std::size_t openPos)
    {
        if (openPos >= text.size() || text[openPos] != '{')
        {
            return std::string::npos;
        }

        int depth = 0;
        std::size_t pos = openPos;
        const std::size_t size = text.size();
        while (pos < size)
        {
            if (text.compare(pos, 2, "//") == 0)
            {
                pos += 2;
                while (pos < size && text[pos] != '\n')
                {
                    ++pos;
                }
                continue;
            }

            if (text.compare(pos, 2, "/*") == 0)
            {
                pos += 2;
                while (pos + 1 < size && !(text[pos] == '*' && text[pos + 1] == '/'))
                {
                    ++pos;
                }
                if (pos + 1 < size)
                {
                    pos += 2;
                }
                continue;
            }

            char c = text[pos];
            if (c == '\"')
            {
                pos = SkipStringLiteral(text, pos);
                continue;
            }
            if (c == '\'')
            {
                pos = SkipCharLiteral(text, pos);
                continue;
            }

            if (c == '{')
            {
                ++depth;
            }
            else if (c == '}')
            {
                --depth;
                if (depth == 0)
                {
                    return pos;
                }
            }

            ++pos;
        }

        return std::string::npos;
    }

    std::size_t FindStatementEnd(const std::string& text, std::size_t pos, std::size_t limit)
    {
        int parenDepth = 0;
        int braceDepth = 0;
        int bracketDepth = 0;

        while (pos < limit)
        {
            if (text.compare(pos, 2, "//") == 0)
            {
                pos += 2;
                while (pos < limit && text[pos] != '\n')
                {
                    ++pos;
                }
                continue;
            }

            if (text.compare(pos, 2, "/*") == 0)
            {
                pos += 2;
                while (pos + 1 < limit && !(text[pos] == '*' && text[pos + 1] == '/'))
                {
                    ++pos;
                }
                if (pos + 1 < limit)
                {
                    pos += 2;
                }
                continue;
            }

            char c = text[pos];
            if (c == '\"')
            {
                pos = SkipStringLiteral(text, pos);
                continue;
            }
            if (c == '\'')
            {
                pos = SkipCharLiteral(text, pos);
                continue;
            }

            switch (c)
            {
            case '(':
                ++parenDepth;
                break;
            case ')':
                if (parenDepth > 0)
                {
                    --parenDepth;
                }
                break;
            case '{':
                ++braceDepth;
                break;
            case '}':
                if (braceDepth > 0)
                {
                    --braceDepth;
                }
                break;
            case '[':
                ++bracketDepth;
                break;
            case ']':
                if (bracketDepth > 0)
                {
                    --bracketDepth;
                }
                break;
            case ';':
                if (parenDepth == 0 && braceDepth == 0 && bracketDepth == 0)
                {
                    return pos;
                }
                break;
            default:
                break;
            }

            ++pos;
        }

        return std::string::npos;
    }

    std::size_t FindMacroWithin(const std::string& text, std::size_t start, std::size_t end, std::string_view macro)
    {
        std::size_t pos = start;
        while (pos < end)
        {
            if (text.compare(pos, 2, "//") == 0)
            {
                pos += 2;
                while (pos < end && text[pos] != '\n')
                {
                    ++pos;
                }
                continue;
            }

            if (text.compare(pos, 2, "/*") == 0)
            {
                pos += 2;
                while (pos + 1 < end && !(text[pos] == '*' && text[pos + 1] == '/'))
                {
                    ++pos;
                }
                if (pos + 1 < end)
                {
                    pos += 2;
                }
                continue;
            }

            if (text[pos] == '\"')
            {
                pos = SkipStringLiteral(text, pos);
                continue;
            }
            if (text[pos] == '\'')
            {
                pos = SkipCharLiteral(text, pos);
                continue;
            }

            if (pos + macro.size() <= end && text.compare(pos, macro.size(), macro.data()) == 0)
            {
                return pos;
            }

            ++pos;
        }

        return std::string::npos;
    }
}