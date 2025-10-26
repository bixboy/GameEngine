#include "BixHeaderTool/Parser/HeaderParser.h"
#include "BixHeaderTool/Parser/TokenUtils.h"
#include <fstream>
#include <filesystem>
#include <sstream>
#include <iostream>


namespace fs = std::filesystem;

namespace BixTool
{

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

        std::ifstream input(filePath, std::ios::in | std::ios::binary);
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
}
