#pragma once
#include <string>
#include <memory>
#include <iterator>

//TODO operator + and -
//TODO make end work properly(temporary iterator for now)

namespace VWA
{
    struct Line
    {
        std::string content;
        Line *prev, *next;
        int lineNumber;

        Line(Line *_next, Line *_prev, int line_number = 0, std::string_view content = "") : content(content), prev(_prev), next(_next), lineNumber(line_number) {}
        ~Line()
        {
            delete next;
        }
        ///@brief Removes the links to other lines, allowing for this line to be deleted
        void detach()
        {
            prev = nullptr;
            next = nullptr;
        }
    };

    class File
    {
        Line *m_first, *m_last;
        struct iterator
        {
            using iterator_category = std::bidirectional_iterator_tag;
            using value_type = Line;
            using difference_type = std::ptrdiff_t;
            using pointer = Line *;
            using reference = Line &;

            Line &operator*() const
            {
                if (!current)
                    throw std::out_of_range("iterator invalid");
                return *current;
            }
            Line *operator->() const
            {
                if (!current)
                    throw std::out_of_range("iterator invalid");
                return current;
            }
            iterator &operator++()
            {
                if (current)
                    current = current->next;
                else
                    throw std::out_of_range("Tried incrementing File iterator end");
                return *this;
            }
            iterator &operator--()
            {
                current = current->prev;
                if (current == nullptr)
                    throw std::out_of_range("File iterator out of range: Tried accessing line before beginning of file");
                return *this;
            }
            iterator operator++(int)
            {
                iterator tmp = *this;
                ++*this;
                return tmp;
            }
            iterator operator--(int)
            {
                iterator tmp = *this;
                --*this;
                return tmp;
            }
            bool operator==(const iterator &other) const
            {
                return current == other.current;
            }
            bool operator!=(const iterator &other) const
            {
                return current != other.current;
            }
            iterator(Line *current) : current(current) {}
            iterator(const iterator &old) : current(old.current) {}
            iterator &operator=(const iterator &old)
            {
                current = old.current;
                return *this;
            }

        private:
            Line *current;
            friend class File;
        };

    public:
        iterator begin()
        {
            return iterator(m_first);
        }
        iterator last()
        {
            return iterator(m_last);
        }
        iterator end()
        {
            return iterator(nullptr);
        }

        File() : m_first(nullptr), m_last(nullptr) {}

        ~File()
        {
            delete m_first;
        }

        /**
         * @brief Adds a new line to the end of the file
         * 
         * If the line number is -1(default), the line number of the previous line +1 is used
         * content should not contain a newline character. There will not be an error because of this, but it does not make sense.
         */
        void append(std::string_view content, int lineNumber = -1)
        {
            if (m_last)
            {
                m_last->next = new Line(nullptr, m_last, lineNumber == -1 ? m_last->lineNumber + 1 : lineNumber, content);
                m_last = m_last->next;
            }
            else
            {
                m_first = new Line(nullptr, nullptr, lineNumber == -1 ? 0 : lineNumber, content);
                m_last = m_first;
            }
        }

        /**
         * @brief Adds a new line before the iterator. The line number of the iterator is used.
         */
        void insertBefore(const iterator &it, std::string_view content)
        {
            if (it == end())
            {
                append(content);
                return;
            }
            auto prev = it->prev;
            auto newLine = new Line(it.current, it->prev, it->lineNumber, content);
            it->prev = newLine;
            if (prev)
                prev->next = newLine;
            else
                m_first = newLine;
        }

        /**
         * @brief Adds a new line after the iterator. The line number of the iterator is used.
         */
        void insertAfter(const iterator &it, std::string_view content)
        {
            if (it == end())
                throw std::out_of_range("Tried inserting at end, not a valid iterator. Use last instead");
            if (it == last())
            {
                append(content);
                return;
            }
            auto next = it->next;
            auto newLine = new Line(next, it.current, it->lineNumber, content);
            it->next = newLine;
            next->prev = newLine;
        }
        /**
         * @brief Erases the line pointed to by the iterator.
         * 
         * A new iterator is returned, pointing to the line after the erased line.
         * If it is the last line, the iterator is set to end().
         * 
         * @param it 
         */
        iterator removeLine(const iterator &it)
        {
            if (it == end())
                throw std::out_of_range("Tried removing end, not a valid iterator. Use last instead");
            if (it == last())
            {
                it->prev->next = nullptr;
                m_last = it->prev;
                it->detach();
                delete it.current;
                return end();
            }
            if (it == begin())
            {
                it->next->prev = nullptr;
                m_first = it->next;
                it->detach();
                delete it.current;
                return begin();
            }
            it->prev->next = it->next;
            it->next->prev = it->prev;
            auto next = it->next;
            it->detach();
            delete it.current;
            return next;
        }

        bool empty() const
        {
            return m_first == nullptr;
        }
        std::string toString()
        {
            std::string ret;
            for (auto it = begin(); it != end(); ++it)
            {
                ret += it->content;
                if (it != last())
                    ret += '\n';
            }
            return ret;
        }
    };

}