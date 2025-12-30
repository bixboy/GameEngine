#pragma once
#include <algorithm>
#include <cctype>
#include <format>
#include <iterator>
#include <ostream>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace BixEngine
{
    class String
    {
    public:
        using value_type = char;
        using size_type = std::string::size_type;
        using iterator = std::string::iterator;
        using const_iterator = std::string::const_iterator;
        static constexpr size_type npos = std::string::npos;

        String() = default;
        String(const String&) = default;
        String(String&&) noexcept = default;
        ~String() = default;

        String(const char* value) : data_(value ? value : "") {}
        String(std::string value) noexcept : data_(std::move(value)) {}
        String(std::string_view value) : data_(value) {}
        String(size_type count, char ch) : data_(count, ch) {}

        // --- Assignation ---
        
        String& operator=(const String&) = default;
        String& operator=(String&&) noexcept = default;

        String& operator=(const char* value)
        {
            data_ = value ? value : "";
            return *this;
        }

        String& operator=(std::string value) noexcept
        {
            data_ = std::move(value);
            return *this;
        }

        String& operator=(std::string_view value)
        {
            data_.assign(value);
            return *this;
        }

        // --- Accesseurs ---
        
        [[nodiscard]] size_type size() const noexcept { return data_.size(); }
        [[nodiscard]] size_type length() const noexcept { return data_.length(); }
        [[nodiscard]] bool empty() const noexcept { return data_.empty(); }
        
        void clear() noexcept { data_.clear(); }
        void reserve(size_type newCapacity) { data_.reserve(newCapacity); }
        [[nodiscard]] size_type capacity() const noexcept { return data_.capacity(); }
        
        void resize(size_type count) { data_.resize(count); }
        void resize(size_type count, char ch) { data_.resize(count, ch); }

        [[nodiscard]] char* data() noexcept { return data_.data(); }
        [[nodiscard]] const char* data() const noexcept { return data_.data(); }
        
        [[nodiscard]] const char* c_str() const noexcept { return data_.c_str(); }
        
        [[nodiscard]] std::string_view View() const noexcept { return data_; }
        [[nodiscard]] const std::string& Std() const noexcept { return data_; }

        // --- Iterateurs ---
        
        [[nodiscard]] iterator begin() noexcept { return data_.begin(); }
        [[nodiscard]] iterator end() noexcept { return data_.end(); }
        [[nodiscard]] const_iterator begin() const noexcept { return data_.begin(); }
        [[nodiscard]] const_iterator end() const noexcept { return data_.end(); }
        
        // --- Accès élément ---
        
        [[nodiscard]] char& operator[](size_type index) noexcept { return data_[index]; }
        [[nodiscard]] const char& operator[](size_type index) const noexcept { return data_[index]; }

        // --- Modification ---
        
        void push_back(char ch)
        {
            data_.push_back(ch);
        }
        
        void pop_back()
        {
            if (!data_.empty())
                data_.pop_back();
        }

        String& Append(std::string_view value)
        {
            data_.append(value);
            return *this;
        }
        String& Append(size_type count, char ch)
        {
            data_.append(count, ch);
            return *this;
        }

        operator std::string_view() const { return std::string_view(c_str(), length()); }
        
        String& operator += (const String& other) { return Append(other.View()); }
        String& operator += (std::string_view other) { return Append(other); }
        String& operator += (char ch) { data_.push_back(ch); return *this; }
        
        // --- Opérateurs + (Concaténation) ---
        
        [[nodiscard]] friend String operator + (String lhs, std::string_view rhs) { lhs += rhs; return lhs; }
        [[nodiscard]] friend String operator + (std::string_view lhs, const String& rhs) { String s(lhs); s += rhs; return s; }
        [[nodiscard]] friend String operator + (String lhs, char rhs) { lhs += rhs; return lhs; }
        
        // --- Comparaisons ---
        
        bool operator == (const String& other) const noexcept { return data_ == other.data_; }
        bool operator == (std::string_view other) const noexcept { return data_ == other; }
        bool operator == (const char* other) const noexcept { return data_ == (other ? other : ""); }
        
        bool operator != (const String& other) const noexcept { return !(*this == other); }
        bool operator != (std::string_view other) const noexcept { return !(*this == other); }

        // --- Utilitaires de Recherche ---

        [[nodiscard]] bool StartsWith(std::string_view prefix, bool caseSensitive = true) const noexcept
        {
            if (prefix.size() > data_.size())
                return false;
            
            if (caseSensitive)
                return data_.starts_with(prefix);
            
            return std::equal(prefix.begin(), prefix.end(), data_.begin(), 
            [](char a, char b)
            {
                return EqualsIgnoreCaseChar(a, b);
            });
        }

        [[nodiscard]] bool EndsWith(std::string_view suffix, bool caseSensitive = true) const noexcept
        {
            if (suffix.size() > data_.size())
                return false;
            if (caseSensitive)
                return data_.ends_with(suffix);

            return std::equal(suffix.rbegin(), suffix.rend(), data_.rbegin(), 
            [](char a, char b)
            {
                return EqualsIgnoreCaseChar(a, b);
            });
        }

        [[nodiscard]] bool Contains(std::string_view value, bool caseSensitive = true) const
        {
            if (caseSensitive)
                return data_.find(value) != npos;
            
            return FindInsensitive(value) != npos;
        }

        [[nodiscard]] bool EqualsIgnoreCase(std::string_view other) const noexcept
        {
            return data_.size() == other.size() && std::equal(data_.begin(), data_.end(), other.begin(),
            [](char a, char b)
            {
                return EqualsIgnoreCaseChar(a, b);
            });
        }

        // --- Transformations ---

        [[nodiscard]] String ToUpper() const { String res(*this); res.ToUpperInline(); return res; }
        [[nodiscard]] String ToLower() const { String res(*this); res.ToLowerInline(); return res; }

        String& ToUpperInline()
        {
            std::transform(data_.begin(), data_.end(), data_.begin(),
            [](unsigned char c)
            {
                return static_cast<char>(std::toupper(c));
            });
            
            return *this;
        }

        String& ToLowerInline()
        {
            std::transform(data_.begin(), data_.end(), data_.begin(),
            [](unsigned char c)
            {
                return static_cast<char>(std::tolower(c));
            });
            
            return *this;
        }

        // --- Trim ---
        [[nodiscard]] String Trim() const
        {
            String res(*this); res.TrimStartInline(); return res;
        }

        String& TrimStartInline()
        {
            auto notSpace = [](unsigned char c) { return !std::isspace(c); };

            auto it = std::ranges::find_if(data_, notSpace);
    
            data_.erase(data_.begin(), it);
    
            return *this;
        }

        String& TrimEndInline()
        {
            auto notSpace = [](unsigned char c) { return !std::isspace(c); };
    
            auto revView = std::views::reverse(data_);
            auto revIt = std::ranges::find_if(revView, notSpace);
            
            data_.erase(revIt.base(), data_.end());
    
            return *this;
        }

        // --- Replace ---
        [[nodiscard]] String Replace(std::string_view from, std::string_view to) const
        {
            if (from.empty())
                return *this;
            
            String result;
            result.reserve(data_.size());
            
            size_type lastPos = 0;
            size_type findPos;
            
            while ((findPos = data_.find(from, lastPos)) != npos)
            {
                result.Append(std::string_view(data_).substr(lastPos, findPos - lastPos));
                result.Append(to);
                lastPos = findPos + from.size();
            }
            
            result.Append(std::string_view(data_).substr(lastPos));
            return result;
        }

        // --- Split / Join ---

        [[nodiscard]] std::vector<String> Split(char delimiter, bool skipEmpty = false) const
        {
            std::vector<String> result;
            std::string_view sv = data_;
            size_type pos;

            while ((pos = sv.find(delimiter)) != std::string_view::npos)
            {
                if (!skipEmpty || pos > 0)
                    result.emplace_back(sv.substr(0, pos));
                
                sv = sv.substr(pos + 1);
            }
            
            if (!skipEmpty || !sv.empty())
                result.emplace_back(sv);

            return result;
        }

        // --- Formatting ---
        
        template <typename... Args>
        static String Format(std::format_string<Args...> fmt, Args&&... args)
        {
            return String(std::vformat(fmt.get(), std::make_format_args(std::forward<Args>(args)...)));
        }

        // --- Conversions Numériques ---
        
        [[nodiscard]] static String FromInt(int v) { return std::to_string(v); }
        [[nodiscard]] static String FromFloat(float v) { return std::to_string(v); }

    private:
        std::string data_;

        static bool EqualsIgnoreCaseChar(char lhs, char rhs)
        {
            return std::tolower(static_cast<unsigned char>(lhs)) == std::tolower(static_cast<unsigned char>(rhs));
        }

        [[nodiscard]] size_type FindInsensitive(std::string_view value, size_type start = 0) const
        {
            if (start >= data_.size())
                return npos;
    
            auto it = std::search(
                data_.begin() + static_cast<std::string::difference_type>(start), 
                data_.end(),
                value.begin(), value.end(),
                [](char a, char b)
                {
                    return EqualsIgnoreCaseChar(a, b);
                }
            );
    
            return (it == data_.end()) ? npos : static_cast<size_type>(std::distance(data_.begin(), it));
        }
    };
}


// Support pour std::cout << String
inline std::ostream& operator<<(std::ostream& stream, const BixEngine::String& value)
{
    return stream << value.View();
}

// Support pour std::hash
namespace std
{
    template <>
    struct hash<BixEngine::String>
    {
        std::size_t operator()(const BixEngine::String& value) const noexcept
        {
            return std::hash<std::string_view>{}(value.View());
        }
    };
    
    // Support pour std::format
    template <>
    struct formatter<BixEngine::String> : formatter<std::string_view>
    {
        auto format(const BixEngine::String& s, format_context& ctx) const
        {
            return formatter<std::string_view>::format(s.View(), ctx);
        }
    };
}