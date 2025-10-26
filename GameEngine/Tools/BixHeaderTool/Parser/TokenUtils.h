#pragma once
#include <string>
#include <string_view>
#include <Types.h>

namespace BixTool
{
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
}
