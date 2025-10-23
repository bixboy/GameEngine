#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

namespace
{
constexpr std::string_view kBClassMacro = "BCLASS()";
constexpr std::string_view kBPropertyMacro = "BPROPERTY()";

struct Property
{
    std::string Type;
    std::string Name;
};

struct ParsedClass
{
    std::string Name;
    std::vector<std::string> Namespaces;
    std::string BaseType;
    std::vector<Property> Properties;
    bool HasGeneratedBody = false;
};

struct HeaderParseResult
{
    bool ContainsReflection = false;
    std::vector<ParsedClass> Classes;
};

enum class ScopeType
{
    Namespace,
    Other
};

struct ScopeEntry
{
    ScopeType Type = ScopeType::Other;
    std::size_t NamespaceCount = 0;
};

bool IsIdentifierStart(char c);
bool IsIdentifierChar(char c);
std::string Trim(std::string_view text);
std::size_t SkipWhitespaceAndComments(const std::string& text, std::size_t pos);
std::size_t SkipStringLiteral(const std::string& text, std::size_t pos);
std::size_t SkipCharLiteral(const std::string& text, std::size_t pos);
std::string ReadToken(const std::string& text, std::size_t& pos);
std::string PeekToken(const std::string& text, std::size_t pos);
std::size_t FindMatchingBrace(const std::string& text, std::size_t openPos);
std::size_t FindStatementEnd(const std::string& text, std::size_t pos, std::size_t limit);
std::size_t FindMacroWithin(const std::string& text, std::size_t start, std::size_t end, std::string_view macro);
std::string MakeQualifiedName(const std::vector<std::string>& namespaces, const std::string& name);
std::string MakeScopedName(const std::vector<std::string>& namespaces, const std::string& name);
std::string SanitizeBaseType(const std::string& base);

HeaderParseResult ParseHeader(const fs::path& filePath);
std::string GenerateHeader(const fs::path& headerPath, const HeaderParseResult& result);
bool WriteFileIfDifferent(const fs::path& path, const std::string& content);
std::vector<fs::path> CollectHeaderFiles(const std::vector<fs::path>& roots);

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

std::string MakeQualifiedName(const std::vector<std::string>& namespaces, const std::string& name)
{
    std::ostringstream oss;
    for (std::size_t i = 0; i < namespaces.size(); ++i)
    {
        if (namespaces[i].empty())
        {
            continue;
        }
        if (oss.tellp() > 0)
        {
            oss << "::";
        }
        oss << namespaces[i];
    }

    if (!name.empty())
    {
        if (oss.tellp() > 0)
        {
            oss << "::";
        }
        oss << name;
    }

    return oss.str();
}

std::string MakeScopedName(const std::vector<std::string>& namespaces, const std::string& name)
{
    std::string qualified = MakeQualifiedName(namespaces, name);
    if (qualified.empty())
    {
        return name;
    }
    if (qualified.rfind("::", 0) == 0)
    {
        return qualified;
    }
    return "::" + qualified;
}

std::string SanitizeBaseType(const std::string& base)
{
    std::istringstream stream(base);
    std::string token;
    std::ostringstream result;
    bool first = true;
    while (stream >> token)
    {
        if (token == "public" || token == "protected" || token == "private" || token == "virtual" || token == "final")
        {
            continue;
        }
        if (!token.empty() && token.back() == ',')
        {
            token.pop_back();
        }
        if (!token.empty())
        {
            if (!first)
            {
                result << ' ';
            }
            result << token;
            first = false;
        }
    }
    return Trim(result.str());
}
HeaderParseResult ParseHeader(const fs::path& filePath)
{
    HeaderParseResult result;

    std::ifstream input(filePath);
    if (!input)
    {
        std::cerr << "[BixHeaderTool] Failed to open " << filePath << " for reading.\n";
        return result;
    }

    std::stringstream buffer;
    buffer << input.rdbuf();
    std::string content = buffer.str();

    const std::size_t size = content.size();
    std::vector<std::string> namespaceStack;
    std::vector<ScopeEntry> scopeStack;

    std::size_t i = 0;
    while (i < size)
    {
        if (content.compare(i, 2, "//") == 0)
        {
            i += 2;
            while (i < size && content[i] != '\n')
            {
                ++i;
            }
            continue;
        }

        if (content.compare(i, 2, "/*") == 0)
        {
            i += 2;
            while (i + 1 < size && !(content[i] == '*' && content[i + 1] == '/'))
            {
                ++i;
            }
            if (i + 1 < size)
            {
                i += 2;
            }
            continue;
        }

        char c = content[i];
        if (c == '\"')
        {
            i = SkipStringLiteral(content, i);
            continue;
        }
        if (c == '\'')
        {
            i = SkipCharLiteral(content, i);
            continue;
        }

        if (content.compare(i, 9, "namespace") == 0 && (i == 0 || !IsIdentifierChar(content[i - 1])))
        {
            std::size_t pos = i + 9;
            pos = SkipWhitespaceAndComments(content, pos);
            std::vector<std::string> names;
            bool alias = false;

            if (pos < size && content[pos] != '{')
            {
                while (pos < size)
                {
                    pos = SkipWhitespaceAndComments(content, pos);
                    if (pos >= size)
                    {
                        break;
                    }
                    if (content[pos] == '{')
                    {
                        break;
                    }
                    if (content[pos] == '=')
                    {
                        alias = true;
                        break;
                    }
                    if (!IsIdentifierStart(content[pos]))
                    {
                        break;
                    }
                    const std::size_t start = pos;
                    ++pos;
                    while (pos < size && IsIdentifierChar(content[pos]))
                    {
                        ++pos;
                    }
                    names.emplace_back(content.substr(start, pos - start));
                    pos = SkipWhitespaceAndComments(content, pos);
                    if (pos + 1 < size && content[pos] == ':' && content[pos + 1] == ':')
                    {
                        pos += 2;
                        continue;
                    }
                    break;
                }
            }

            pos = SkipWhitespaceAndComments(content, pos);
            if (!alias && pos < size && content[pos] == '{')
            {
                ++pos;
                for (const auto& name : names)
                {
                    if (!name.empty())
                    {
                        namespaceStack.push_back(name);
                    }
                }
                scopeStack.push_back({ScopeType::Namespace, names.size()});
                i = pos;
                continue;
            }

            i = pos;
            continue;
        }

        if (content.compare(i, kBClassMacro.size(), kBClassMacro.data()) == 0)
        {
            std::size_t lineStart = content.rfind('\n', i);
            std::size_t checkPos = (lineStart == std::string::npos) ? 0 : lineStart + 1;
            while (checkPos < i && std::isspace(static_cast<unsigned char>(content[checkPos])) != 0)
            {
                ++checkPos;
            }
            std::size_t hashPos = content.find('#', checkPos);
            if (hashPos != std::string::npos && hashPos < i)
            {
                std::size_t wordStart = hashPos + 1;
                while (wordStart < i && std::isspace(static_cast<unsigned char>(content[wordStart])) != 0)
                {
                    ++wordStart;
                }
                if (wordStart < i && content.compare(wordStart, 6, "define") == 0)
                {
                    i += kBClassMacro.size();
                    continue;
                }
            }

            result.ContainsReflection = true;

            std::size_t pos = i + kBClassMacro.size();
            pos = SkipWhitespaceAndComments(content, pos);
            std::string keyword = ReadToken(content, pos);
            if (keyword != "class" && keyword != "struct")
            {
                i = pos;
                continue;
            }

            std::string className;
            while (true)
            {
                std::string token = ReadToken(content, pos);
                if (token.empty())
                {
                    break;
                }

                std::size_t afterToken = SkipWhitespaceAndComments(content, pos);
                std::string nextToken = PeekToken(content, afterToken);
                char nextChar = afterToken < size ? content[afterToken] : '\0';

                bool isCandidate = false;
                if (nextChar == ':' || nextChar == '{' || nextChar == ';')
                {
                    isCandidate = true;
                }
                else if (nextToken == "final" || nextToken == "sealed")
                {
                    isCandidate = true;
                }

                if (isCandidate)
                {
                    className = token;
                    pos = afterToken;
                    break;
                }
            }

            if (className.empty())
            {
                i = pos;
                continue;
            }

            std::string nextToken = PeekToken(content, pos);
            if (nextToken == "final" || nextToken == "sealed")
            {
                (void)ReadToken(content, pos);
                pos = SkipWhitespaceAndComments(content, pos);
            }

            std::string baseSpec;
            pos = SkipWhitespaceAndComments(content, pos);
            if (pos < size && content[pos] == ':')
            {
                ++pos;
                std::size_t baseStart = pos;
                int depth = 0;
                while (pos < size)
                {
                    if (content.compare(pos, 2, "//") == 0)
                    {
                        pos += 2;
                        while (pos < size && content[pos] != '\n')
                        {
                            ++pos;
                        }
                        continue;
                    }
                    if (content.compare(pos, 2, "/*") == 0)
                    {
                        pos += 2;
                        while (pos + 1 < size && !(content[pos] == '*' && content[pos + 1] == '/'))
                        {
                            ++pos;
                        }
                        if (pos + 1 < size)
                        {
                            pos += 2;
                        }
                        continue;
                    }

                    char ch = content[pos];
                    if (ch == '<' || ch == '(' || ch == '[')
                    {
                        ++depth;
                        ++pos;
                        continue;
                    }
                    if ((ch == '>' || ch == ')' || ch == ']') && depth > 0)
                    {
                        --depth;
                        ++pos;
                        continue;
                    }
                    if ((ch == '{' || ch == ',') && depth == 0)
                    {
                        break;
                    }

                    ++pos;
                }
                baseSpec = content.substr(baseStart, pos - baseStart);
                pos = SkipWhitespaceAndComments(content, pos);
            }

            if (pos >= size || content[pos] != '{')
            {
                i = pos;
                continue;
            }

            std::size_t bodyStart = pos;
            std::size_t bodyEnd = FindMatchingBrace(content, bodyStart);
            if (bodyEnd == std::string::npos)
            {
                std::cerr << "[BixHeaderTool] Failed to match braces for class " << className << " in " << filePath << "\n";
                break;
            }

            ParsedClass parsed;
            parsed.Name = className;
            parsed.Namespaces = namespaceStack;
            parsed.BaseType = SanitizeBaseType(baseSpec);

            auto generatedPos = content.find("GENERATED_BODY", bodyStart);
            if (generatedPos != std::string::npos && generatedPos < bodyEnd)
            {
                parsed.HasGeneratedBody = true;
            }

            std::size_t searchPos = bodyStart;
            while (true)
            {
                searchPos = FindMacroWithin(content, searchPos, bodyEnd, kBPropertyMacro);
                if (searchPos == std::string::npos || searchPos >= bodyEnd)
                {
                    break;
                }

                std::size_t propertyStart = searchPos + kBPropertyMacro.size();
                propertyStart = SkipWhitespaceAndComments(content, propertyStart);
                if (propertyStart >= bodyEnd)
                {
                    break;
                }

                std::size_t statementEnd = FindStatementEnd(content, propertyStart, bodyEnd);
                if (statementEnd == std::string::npos)
                {
                    break;
                }

                std::string declaration = content.substr(propertyStart, statementEnd - propertyStart);
                std::string trimmedDecl = Trim(declaration);
                if (trimmedDecl.empty())
                {
                    searchPos = statementEnd + 1;
                    continue;
                }

                std::size_t equalPos = trimmedDecl.find('=');
                if (equalPos != std::string::npos)
                {
                    trimmedDecl = Trim(trimmedDecl.substr(0, equalPos));
                }
                std::size_t braceInitPos = trimmedDecl.find('{');
                if (braceInitPos != std::string::npos)
                {
                    trimmedDecl = Trim(trimmedDecl.substr(0, braceInitPos));
                }

                if (trimmedDecl.empty())
                {
                    searchPos = statementEnd + 1;
                    continue;
                }

                std::size_t nameEnd = trimmedDecl.size();
                while (nameEnd > 0 && std::isspace(static_cast<unsigned char>(trimmedDecl[nameEnd - 1])) != 0)
                {
                    --nameEnd;
                }
                std::size_t nameStart = nameEnd;
                while (nameStart > 0)
                {
                    char ch = trimmedDecl[nameStart - 1];
                    if (std::isalnum(static_cast<unsigned char>(ch)) == 0 && ch != '_')
                    {
                        break;
                    }
                    --nameStart;
                }

                std::string propertyName = trimmedDecl.substr(nameStart, nameEnd - nameStart);
                std::string propertyType = Trim(trimmedDecl.substr(0, nameStart));

                if (propertyName.empty() || propertyType.empty())
                {
                    searchPos = statementEnd + 1;
                    continue;
                }

                parsed.Properties.push_back(Property{propertyType, propertyName});
                searchPos = statementEnd + 1;
            }

            if (parsed.HasGeneratedBody)
            {
                result.Classes.push_back(std::move(parsed));
            }
            else
            {
                std::cerr << "[BixHeaderTool] Warning: class '" << className << "' in " << filePath << " is missing GENERATED_BODY().\n";
            }

            i = bodyEnd + 1;
            while (i < size && std::isspace(static_cast<unsigned char>(content[i])) != 0)
            {
                ++i;
            }
            if (i < size && content[i] == ';')
            {
                ++i;
            }
            continue;
        }

        if (c == '{')
        {
            scopeStack.push_back({ScopeType::Other, 0});
            ++i;
            continue;
        }

        if (c == '}')
        {
            if (!scopeStack.empty())
            {
                ScopeEntry entry = scopeStack.back();
                scopeStack.pop_back();
                if (entry.Type == ScopeType::Namespace)
                {
                    for (std::size_t count = 0; count < entry.NamespaceCount && !namespaceStack.empty(); ++count)
                    {
                        namespaceStack.pop_back();
                    }
                }
            }
            ++i;
            continue;
        }

        ++i;
    }

    return result;
}
std::string GenerateHeader(const fs::path& headerPath, const HeaderParseResult& result)
{
    if (!result.ContainsReflection)
    {
        return {};
    }

    std::ostringstream oss;
    oss << "#pragma once\n";
    oss << "// Auto-generated by BixHeaderTool. Do not modify.\n\n";
    oss << "#include \"Bix/Reflection/BixReflection.h\"\n\n";

    if (result.Classes.empty())
    {
        oss << "#if defined(GENERATED_BODY)\n";
        oss << "#undef GENERATED_BODY\n";
        oss << "#endif\n";
        oss << "#define GENERATED_BODY(...) static_assert(false, \"BixHeaderTool: missing GENERATED_BODY() for reflected class.\")\n";
        std::string output = oss.str();
        if (!output.empty() && output.back() != '\n')
        {
            output.push_back('\n');
        }
        return output;
    }

    oss << "namespace Bix::Reflection::detail { inline constexpr int BIX_INTERNAL_GENERATED_BODY_BASELINE = __COUNTER__; }\n";
    oss << "#if defined(GENERATED_BODY)\n";
    oss << "#undef GENERATED_BODY\n";
    oss << "#endif\n";
    oss << "#define BIX_INTERNAL_GENERATED_BODY_SELECT_IMPL(index) BIX_INTERNAL_GENERATED_BODY_CASE_##index\n";
    oss << "#define BIX_INTERNAL_GENERATED_BODY_SELECT(index) BIX_INTERNAL_GENERATED_BODY_SELECT_IMPL(index)\n";
    oss << "#define GENERATED_BODY() BIX_INTERNAL_GENERATED_BODY_SELECT(__COUNTER__ - ::Bix::Reflection::detail::BIX_INTERNAL_GENERATED_BODY_BASELINE - 1)()\n\n";

    for (std::size_t index = 0; index < result.Classes.size(); ++index)
    {
        const ParsedClass& cls = result.Classes[index];
        const std::string qualified = MakeQualifiedName(cls.Namespaces, cls.Name);
        const std::string scoped = MakeScopedName(cls.Namespaces, cls.Name);

        std::string baseScoped;
        if (!cls.BaseType.empty())
        {
            std::string trimmedBase = Trim(cls.BaseType);
            if (!trimmedBase.empty())
            {
                if (trimmedBase.rfind("::", 0) == 0)
                {
                    baseScoped = trimmedBase;
                }
                else if (trimmedBase.find("::") != std::string::npos || trimmedBase.find('<') != std::string::npos)
                {
                    baseScoped = "::" + trimmedBase;
                }
                else if (cls.Namespaces.empty())
                {
                    baseScoped = "::" + trimmedBase;
                }
                else
                {
                    baseScoped = MakeScopedName(cls.Namespaces, trimmedBase);
                }
            }
        }

        oss << "#define BIX_INTERNAL_GENERATED_BODY_CASE_" << index << "() \\\n";
        oss << "public: \\\n";
        oss << "    static ::Bix::Reflection::ClassInfo& StaticClass() \\\n";
        oss << "    { \\\n";
        oss << "        using ThisClass = " << scoped << "; \\\n";
        oss << "        const ::Bix::Reflection::ClassInfo* superClass = nullptr; \\\n";
        if (!baseScoped.empty())
        {
            oss << "        superClass = &" << baseScoped << "::StaticClass(); \\\n";
        }
        oss << "        auto& classInfo = ::Bix::Reflection::detail::RegisterClass<ThisClass>( \\\n";
        oss << "            \"" << cls.Name << "\", \\\n";
        oss << "            \"" << qualified << "\", \\\n";
        oss << "            superClass, \\\n";
        oss << "            [](::Bix::Reflection::ClassInfo& classInfo) \\\n";
        oss << "            { \\\n";
        if (cls.Properties.empty())
        {
            oss << "                (void)classInfo; \\\n";
        }
        else
        {
            for (const Property& property : cls.Properties)
            {
                oss << "                ::Bix::Reflection::detail::RegisterProperty<ThisClass, " << property.Type << ">(classInfo, \"" << property.Name << "\", &ThisClass::" << property.Name << ", \"" << property.Type << "\"); \\\n";
            }
        }
        oss << "            }); \\\n";
        oss << "        return classInfo; \\\n";
        oss << "    } \\\n";
        oss << "    virtual ::Bix::Reflection::ClassInfo& GetClass() const \\\n";
        oss << "    { \\\n";
        oss << "        return StaticClass(); \\\n";
        oss << "    } \\\n";
        oss << "private: \\\n";
        oss << "    static inline ::Bix::Reflection::detail::ClassRegistrationInvoker<" << scoped << "> __BixReflection_AutoRegister{};\n\n";
    }

    std::string output = oss.str();
    if (!output.empty() && output.back() != '\n')
    {
        output.push_back('\n');
    }
    return output;
}
bool WriteFileIfDifferent(const fs::path& path, const std::string& content)
{
    std::string data = content;
    if (!data.empty() && data.back() != '\n')
    {
        data.push_back('\n');
    }

    std::string current;
    std::ifstream existing(path);
    if (existing)
    {
        std::stringstream buffer;
        buffer << existing.rdbuf();
        current = buffer.str();
        if (!current.empty() && current.back() != '\n')
        {
            current.push_back('\n');
        }
        if (current == data)
        {
            return false;
        }
    }

    std::error_code ec;
    fs::create_directories(path.parent_path(), ec);

    std::ofstream output(path, std::ios::trunc);
    if (!output)
    {
        std::cerr << "[BixHeaderTool] Failed to write " << path << "\n";
        return false;
    }

    output << data;
    return true;
}

std::vector<fs::path> CollectHeaderFiles(const std::vector<fs::path>& roots)
{
    std::vector<fs::path> files;
    auto isGenerated = [](const fs::path& p) {
        const std::string name = p.filename().string();
        return name.size() >= 12 && name.substr(name.size() - 12) == ".generated.h";
    };

    auto isHeader = [](const fs::path& p) {
        std::string ext = p.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
        return ext == ".h" || ext == ".hpp" || ext == ".hh" || ext == ".hxx";
    };

    for (const auto& root : roots)
    {
        if (!fs::exists(root))
        {
            continue;
        }

        if (fs::is_regular_file(root))
        {
            if (!isGenerated(root) && isHeader(root))
            {
                files.push_back(fs::absolute(root));
            }
            continue;
        }

        if (fs::is_directory(root))
        {
            for (const auto& entry : fs::recursive_directory_iterator(root))
            {
                if (!entry.is_regular_file())
                {
                    continue;
                }
                const fs::path& path = entry.path();
                if (isGenerated(path))
                {
                    continue;
                }
                if (isHeader(path))
                {
                    files.push_back(fs::absolute(path));
                }
            }
        }
    }

    std::sort(files.begin(), files.end());
    files.erase(std::unique(files.begin(), files.end()), files.end());
    return files;
}

} // namespace

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: BixHeaderTool <paths...>\n";
        return 1;
    }

    std::vector<fs::path> roots;
    roots.reserve(static_cast<std::size_t>(argc - 1));
    for (int i = 1; i < argc; ++i)
    {
        roots.emplace_back(argv[i]);
    }

    std::vector<fs::path> headers = CollectHeaderFiles(roots);
    if (headers.empty())
    {
        std::cout << "[BixHeaderTool] No headers to process.\n";
        return 0;
    }

    bool anyUpdated = false;

    for (const auto& header : headers)
    {
        HeaderParseResult parseResult = ParseHeader(header);
        if (!parseResult.ContainsReflection)
        {
            continue;
        }

        std::string generated = GenerateHeader(header, parseResult);
        if (generated.empty())
        {
            continue;
        }

        fs::path generatedPath = header.parent_path() / (header.stem().string() + ".generated.h");

        if (WriteFileIfDifferent(generatedPath, generated))
        {
            std::error_code ec;
            fs::path relative = fs::relative(generatedPath, header.parent_path(), ec);
            if (ec)
            {
                relative = generatedPath.filename();
            }
            std::cout << "[BixHeaderTool] Updated " << relative.string() << " for " << header.filename().string() << "\n";
            anyUpdated = true;
        }
    }

    if (!anyUpdated)
    {
        std::cout << "[BixHeaderTool] No changes.\n";
    }

    return 0;
}
