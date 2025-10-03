#pragma once

#include <algorithm>
#include <cctype>
#include <initializer_list>
#include <iterator>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Engine
{
    class String
    {
    public:
        using value_type      = std::string::value_type;
        using size_type       = std::string::size_type;
        using iterator        = std::string::iterator;
        using const_iterator  = std::string::const_iterator;

        String() = default;
        String(const String&) = default;
        String(String&&) noexcept = default;
        ~String() = default;

        String(const char* value) : data_(value ? value : "") {}
        String(std::string value) noexcept : data_(std::move(value)) {}
        String(std::string_view value) : data_(value) {}
        String(size_type count, char ch) : data_(count, ch) {}

        String& operator = (const String&) = default;
        String& operator = (String&&) noexcept = default;
        String& operator = (const char* value)
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

        [[nodiscard]] size_type size() const noexcept { return data_.size(); }
        [[nodiscard]] size_type length() const noexcept { return data_.length(); }
        [[nodiscard]] bool empty() const noexcept { return data_.empty(); }
        [[nodiscard]] bool IsEmpty() const noexcept { return data_.empty(); }

        void clear() noexcept { data_.clear(); }
        void Clear() noexcept { data_.clear(); }

        void reserve(size_type newCapacity) { data_.reserve(newCapacity); }
        [[nodiscard]] size_type capacity() const noexcept { return data_.capacity(); }
        void resize(size_type count) { data_.resize(count); }
        void resize(size_type count, char ch) { data_.resize(count, ch); }

        [[nodiscard]] char* data() noexcept { return data_.data(); }
        [[nodiscard]] const char* data() const noexcept { return data_.data(); }
        [[nodiscard]] const char* c_str() const noexcept { return data_.c_str(); }
        [[nodiscard]] std::string_view View() const noexcept { return data_; }
        [[nodiscard]] const std::string& Std() const noexcept { return data_; }

        [[nodiscard]] iterator begin() noexcept { return data_.begin(); }
        [[nodiscard]] iterator end() noexcept { return data_.end(); }
        [[nodiscard]] const_iterator begin() const noexcept { return data_.begin(); }
        [[nodiscard]] const_iterator end() const noexcept { return data_.end(); }
        [[nodiscard]] const_iterator cbegin() const noexcept { return data_.cbegin(); }
        [[nodiscard]] const_iterator cend() const noexcept { return data_.cend(); }

        [[nodiscard]] char& operator[](size_type index) noexcept { return data_[index]; }
        [[nodiscard]] const char& operator[](size_type index) const noexcept { return data_[index]; }

        [[nodiscard]] char& front() noexcept { return data_.front(); }
        [[nodiscard]] const char& front() const noexcept { return data_.front(); }
        [[nodiscard]] char& back() noexcept { return data_.back(); }
        [[nodiscard]] const char& back() const noexcept { return data_.back(); }

        void push_back(char ch) { data_.push_back(ch); }
        void pop_back() { PopBack(); }
        void PopBack()
        {
            if (!empty())
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

        String& operator+=(const String& other)
        {
            return Append(other.View());
        }

        String& operator+=(const std::string& other)
        {
            return Append(other);
        }

        String& operator+=(std::string_view other)
        {
            return Append(other);
        }

        String& operator+=(char ch)
        {
            data_.push_back(ch);
            return *this;
        }
        String& operator+=(const char* other)
        {
            if (other)
                Append(other);
            return *this;
        }

        [[nodiscard]] String operator+(const String& other) const
        {
            String result(*this);
            result += other;
            return result;
        }

        [[nodiscard]] String operator+(std::string_view other) const
        {
            String result(*this);
            result += other;
            return result;
        }

        friend String operator+(std::string_view lhs, const String& rhs)
        {
            String result(lhs);
            result += rhs;
            return result;
        }

        friend String operator+(const char* lhs, const String& rhs)
        {
            return String(lhs ? lhs : "") + rhs;
        }

        // String + std::string
        [[nodiscard]] String operator+(const std::string& other) const
        {
            String result(*this);
            result += other;
            return result;
        }

        // std::string + String
        friend String operator+(const std::string& lhs, const String& rhs) {
            return String(lhs) + rhs;
        }

        // String + const char*
        [[nodiscard]] String operator+(const char* other) const {
            String result(*this);
            result += std::string_view(other ? other : "");
            return result;
        }

        [[nodiscard]] bool operator==(const String& other) const noexcept { return data_ == other.data_; }
        [[nodiscard]] bool operator!=(const String& other) const noexcept { return data_ != other.data_; }
        
        [[nodiscard]] bool operator==(const std::string_view other) const noexcept { return data_ == other; }
        [[nodiscard]] bool operator!=(const std::string_view other) const noexcept { return data_ != other; }
        
        [[nodiscard]] bool operator==(const char* other) const noexcept { return data_ == (other ? other : ""); }
        [[nodiscard]] bool operator!=(const char* other) const noexcept { return !(*this == other); }

        [[nodiscard]] bool StartsWith(std::string_view prefix, bool caseSensitive = true) const noexcept
        {
            if (prefix.size() > data_.size())
                return false;
            const auto begin = data_.begin();
            if (caseSensitive)
                return std::equal(prefix.begin(), prefix.end(), begin);
            return std::equal(prefix.begin(), prefix.end(), begin, [](char a, char b)
            {
                return EqualsIgnoreCaseChar(a, b);
            });
        }

        [[nodiscard]] bool EndsWith(std::string_view suffix, bool caseSensitive = true) const noexcept
        {
            if (suffix.size() > data_.size())
                return false;
            const auto offset = data_.size() - suffix.size();
            const auto begin = data_.begin() + static_cast<std::ptrdiff_t>(offset);
            if (caseSensitive)
                return std::equal(suffix.begin(), suffix.end(), begin);
            return std::equal(suffix.begin(), suffix.end(), begin, [](char a, char b)
            {
                return EqualsIgnoreCaseChar(a, b);
            });
        }

        [[nodiscard]] bool Contains(std::string_view value, bool caseSensitive = true) const
        {
            return caseSensitive ? data_.find(value) != std::string::npos : FindInsensitive(value) != std::string::npos;
        }

        [[nodiscard]] bool EqualsIgnoreCase(std::string_view other) const noexcept
        {
            return data_.size() == other.size() && std::equal(data_.begin(), data_.end(), other.begin(), [](char a, char b)
            {
                return EqualsIgnoreCaseChar(a, b);
            });
        }

        [[nodiscard]] String ToUpper() const
        {
            String result(*this);
            result.ToUpperInline();
            return result;
        }

        [[nodiscard]] String ToLower() const
        {
            String result(*this);
            result.ToLowerInline();
            return result;
        }

        String& ToUpperInline()
        {
            std::transform(data_.begin(), data_.end(), data_.begin(), [](unsigned char c)
            {
                return static_cast<char>(std::toupper(c));
            });
            return *this;
        }

        String& ToLowerInline()
        {
            std::transform(data_.begin(), data_.end(), data_.begin(), [](unsigned char c)
            {
                return static_cast<char>(std::tolower(c));
            });
            return *this;
        }

        [[nodiscard]] String TrimStart() const
        {
            String result(*this);
            result.TrimStartInline();
            return result;
        }

        [[nodiscard]] String TrimEnd() const
        {
            String result(*this);
            result.TrimEndInline();
            return result;
        }

        [[nodiscard]] String Trim() const
        {
            String result(*this);
            result.TrimInline();
            return result;
        }

        String& TrimStartInline()
        {
            const auto it = std::find_if_not(data_.begin(), data_.end(), [](unsigned char c)
            {
                return std::isspace(c) != 0;
            });
            data_.erase(data_.begin(), it);
            return *this;
        }

        String& TrimEndInline()
        {
            const auto it = std::find_if_not(data_.rbegin(), data_.rend(), [](unsigned char c)
            {
                return std::isspace(c) != 0;
            });
            data_.erase(it.base(), data_.end());
            return *this;
        }

        String& TrimInline()
        {
            return TrimStartInline().TrimEndInline();
        }

        [[nodiscard]] String Replace(std::string_view from, std::string_view to, bool caseSensitive = true) const
        {
            String result(*this);
            result.ReplaceInline(from, to, caseSensitive);
            return result;
        }

        String& ReplaceInline(std::string_view from, std::string_view to, bool caseSensitive = true)
        {
            if (from.empty())
                return *this;

            size_type start = 0;
            while (start <= data_.size())
            {
                const size_type pos = caseSensitive ? data_.find(from, start) : FindInsensitive(from, start);
                if (pos == std::string::npos)
                    break;

                data_.replace(pos, from.size(), to.data(), to.size());
                const size_type advance = to.empty() ? from.size() : to.size();
                start = pos + advance;
            }
            return *this;
        }

        [[nodiscard]] std::vector<String> Split(char delimiter, bool skipEmpty = false) const
        {
            std::vector<String> result;
            const auto estimatedSegments = static_cast<size_type>(std::count(data_.begin(), data_.end(), delimiter)) + 1;
            result.reserve(estimatedSegments);

            String current;
            for (char ch : data_)
            {
                if (ch == delimiter)
                {
                    if (!current.empty() || !skipEmpty)
                    {
                        result.emplace_back(std::move(current));
                        current.clear();
                    }
                }
                else
                {
                    current.push_back(ch);
                }
            }
            if (!current.empty() || !skipEmpty)
                result.emplace_back(std::move(current));
            return result;
        }

        [[nodiscard]] std::vector<String> Split(std::string_view delimiter, bool skipEmpty = false) const
        {
            if (delimiter.empty())
                return { *this };

            std::vector<String> result;
            size_type start = 0;
            while (start <= data_.size())
            {
                const size_type pos = data_.find(delimiter, start);
                if (pos == std::string::npos)
                {
                    String tail(data_.substr(start));
                    if (!tail.empty() || !skipEmpty)
                        result.emplace_back(std::move(tail));
                    break;
                }

                String segment(data_.substr(start, pos - start));
                if (!segment.empty() || !skipEmpty)
                    result.emplace_back(std::move(segment));
                start = pos + delimiter.size();
            }
            return result;
        }

        [[nodiscard]] String Left(size_type count) const
        {
            if (count >= data_.size())
                return *this;
            return String(data_.substr(0, count));
        }

        [[nodiscard]] String Right(size_type count) const
        {
            if (count >= data_.size())
                return *this;
            return String(data_.substr(data_.size() - count));
        }

        [[nodiscard]] String Mid(size_type start, size_type count = std::string::npos) const
        {
            if (start >= data_.size())
                return String();
            return String(data_.substr(start, count));
        }

        [[nodiscard]] static String Join(const std::vector<String>& values, std::string_view delimiter)
        {
            return JoinRange(values, delimiter);
        }

        [[nodiscard]] static String Join(std::initializer_list<String> values, std::string_view delimiter)
        {
            return JoinRange(values, delimiter);
        }

        [[nodiscard]] static String FromInt(int value) { return std::to_string(value); }
        [[nodiscard]] static String FromUInt(unsigned int value) { return std::to_string(value); }
        [[nodiscard]] static String FromFloat(float value) { return std::to_string(value); }
        [[nodiscard]] static String FromDouble(double value) { return std::to_string(value); }

        [[nodiscard]] bool IsNumeric() const noexcept
        {
            if (data_.empty())
                return false;
            return std::all_of(data_.begin(), data_.end(), [](unsigned char c)
            {
                return std::isdigit(c) != 0 || c == '+' || c == '-' || c == '.';
            });
        }

        [[nodiscard]] size_type find(std::string_view value, size_type pos = 0) const noexcept {
            return data_.find(value, pos);
        }

        [[nodiscard]] size_type rfind(std::string_view value, size_type pos = std::string::npos) const noexcept {
            return data_.rfind(value, pos);
        }

        operator std::string&() & noexcept { return data_; }
        operator const std::string&() const& noexcept { return data_; }
        operator std::string&&() && noexcept { return std::move(data_); }
        operator const char*() const noexcept { return data_.c_str(); }
        operator std::string_view() const noexcept { return data_; }

    private:
        [[nodiscard]] size_type FindInsensitive(std::string_view value, size_type start = 0) const
        {
            if (start > data_.size())
                return std::string::npos;

            if (value.empty())
                return start;

            const auto begin = data_.begin() + static_cast<std::ptrdiff_t>(start);
            const auto it = std::search(begin, data_.end(), value.begin(), value.end(), [](char a, char b)
            {
                return EqualsIgnoreCaseChar(a, b);
            });

            return it == data_.end() ? std::string::npos : static_cast<size_type>(std::distance(data_.begin(), it));
        }

        static bool EqualsIgnoreCaseChar(char lhs, char rhs)
        {
            return ToLowerChar(static_cast<unsigned char>(lhs)) == ToLowerChar(static_cast<unsigned char>(rhs));
        }

        static unsigned char ToLowerChar(unsigned char c)
        {
            return static_cast<unsigned char>(std::tolower(c));
        }

        template <typename Range>
        [[nodiscard]] static String JoinRange(const Range& values, std::string_view delimiter)
        {
            auto it = std::begin(values);
            auto end = std::end(values);
            if (it == end)
                return {};

            String result;
            size_type totalSize = 0;
            size_type count = 0;
            for (const auto& value : values)
            {
                totalSize += value.size();
                ++count;
            }

            if (count == 0)
                return {};

            totalSize += (count - 1) * delimiter.size();
            result.reserve(totalSize);

            size_type index = 0;
            for (auto iter = std::begin(values); iter != end; ++iter, ++index)
            {
                result += *iter;
                if (index + 1 < count)
                    result += delimiter;
            }

            return result;
        }

        std::string data_{};
    };
}

inline std::ostream& operator<<(std::ostream& stream, const Engine::String& value)
{
    stream << value.View();
    return stream;
}

namespace std
{
    template<>
    struct hash<Engine::String>
    {
        std::size_t operator()(const Engine::String& value) const noexcept
        {
            return std::hash<std::string_view>{}(value.View());
        }
    };
}

