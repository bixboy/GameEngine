#include "HeaderParser.h"
#include "TokenUtils.h"
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

    std::string GetAccessSpecifier(const std::string& content, std::size_t bodyStart, std::size_t propertyStart, const std::string& classType)
    {
        // Scan backwards from propertyStart to bodyStart
        // Default access
        std::string currentAccess = (classType == "class") ? "private" : "public";

        if (propertyStart <= bodyStart) return currentAccess;

        std::size_t pos = propertyStart;
        int depth = 0;

        while (pos > bodyStart)
        {
            pos--;
            char c = content[pos];

            // Handle braces/parens to ignore nested scopes
            if (c == '}' || c == ')' || c == ']') depth++;
            else if (c == '{' || c == '(' || c == '[') depth--;

            if (depth == 0)
            {
                // Check for access specifiers
                // Need to match "public:", "private:", "protected:"
                // We check if we are at the ':'
                if (c == ':')
                {
                    // Look back for keywords
                    std::size_t check = pos;
                    while (check > bodyStart && std::isspace(static_cast<unsigned char>(content[check - 1]))) check--;
                    
                    if (check >= 6 && content.compare(check - 6, 6, "public") == 0) return "public";
                    if (check >= 7 && content.compare(check - 7, 7, "private") == 0) return "private";
                    if (check >= 9 && content.compare(check - 9, 9, "protected") == 0) return "protected";
                }
            }
        }
        return currentAccess;
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

                // BPROPERTY(EditAnywhere, Category="Physics")
                // We want to capture the content inside ().
                std::string metadata;
                std::size_t openParen = content.find('(', i);
                std::size_t closeParen = std::string::npos;
                
                if (openParen != std::string::npos)
                {
                     // Ensure openParen is before the end of the macro usage line/block
                     // But simpler: just find matching paren from openParen
                     closeParen = FindMatchingBrace(content, openParen); // Reusing brace finder for parens works if implementation allows or use specific Paret finder
                     // Actually FindMatchingBrace might assume '{'. Let's check TokenUtils or do simple scan.
                     // The parser seems simple. Let's write a simple extraction loop.
                     
                     std::size_t depth = 1;
                     std::size_t cursor = openParen + 1;
                     while (cursor < size && depth > 0)
                     {
                         if (content[cursor] == '(') depth++;
                         else if (content[cursor] == ')') depth--;
                         cursor++;
                     }
                     
                     if (depth == 0)
                     {
                         closeParen = cursor - 1;
                         metadata = content.substr(openParen + 1, closeParen - openParen - 1);
                     }
                }

                std::size_t pos = (closeParen != std::string::npos) ? closeParen + 1 : i + kBPropertyMacro.size();
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
                    searchPos = FindMacroWithin(content, searchPos, bodyEnd, "BPROPERTY");
                    if (searchPos == std::string::npos || searchPos >= bodyEnd)
                    {
                        break;
                    }

                    std::size_t propertyStart = searchPos + sizeof("BPROPERTY") - 1; // "BPROPERTY" length is 9

                    // Capture Metadata
                    std::string propertyMetadata;
                    std::size_t propOpenParen = content.find('(', searchPos);
                    if (propOpenParen != std::string::npos && propOpenParen < bodyEnd)
                    {
                         // Check if it's right after BPROPERTY
                         // Allow whitespace
                         std::size_t checkPos = searchPos + 9;
                         while(checkPos < propOpenParen && std::isspace(content[checkPos])) checkPos++;
                         
                         if (checkPos == propOpenParen)
                         {
                             std::size_t depth = 1;
                             std::size_t cursor = propOpenParen + 1;
                             while (cursor < bodyEnd && depth > 0)
                             {
                                 if (content[cursor] == '(') depth++;
                                 else if (content[cursor] == ')') depth--;
                                 cursor++;
                             }
                             
                             if (depth == 0)
                             {
                                 propertyMetadata = content.substr(propOpenParen + 1, cursor - 1 - propOpenParen - 1);
                                 propertyStart = cursor;
                             }
                         }
                    }

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

                    std::string access = GetAccessSpecifier(content, bodyStart, propertyStart, keyword);
                    parsed.Properties.push_back(Property{propertyType, propertyName, propertyMetadata, access});
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
