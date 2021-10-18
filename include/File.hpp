#pragma once
#include <string>
#include <memory>
#include <iterator>
#include <fstream>

//TODO: iterator over all characters in file.
//TODO: equivalent to string view
namespace VWA
{
    struct Line
    {
        std::string content;
        Line *prev, *next;
        int lineNumber;
        std::shared_ptr<std::string> fileName;

        Line(Line *_next, Line *_prev, const std::shared_ptr<std::string> &file_name = nullptr, int line_number = 1, std::string_view content = "") : content(content), prev(_prev), next(_next), lineNumber(line_number), fileName(file_name) {}
        ~Line()
        {
            if (next != this)
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
        std::shared_ptr<std::string> m_fileName;
        Line *m_end, *m_first;

        File(){};

    public:
        struct iterator
        {
            using iterator_category = std::bidirectional_iterator_tag;
            using value_type = Line;
            using difference_type = std::ptrdiff_t;
            using pointer = Line *;
            using reference = Line &;

            Line &operator*() const
            {
                if (!current->next)
                    throw std::out_of_range("Cannot dereference end");
                return *current;
            }
            Line *operator->() const
            {
                if (!current->next)
                    throw std::out_of_range("Cannot dereference end");
                return current;
            }
            iterator &operator++()
            {
                current = current->next;
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
            iterator operator+(int n)
            {
                iterator tmp = *this;
                for (int i = 0; i < n; i++)
                    ++tmp;
                return tmp;
            }
            iterator operator-(int n)
            {
                iterator tmp = *this;
                for (int i = 0; i < n; i++)
                    --tmp;
                return tmp;
            }
            iterator &operator+=(int n)
            {
                for (int i = 0; i < n; i++)
                    ++*this;
                return *this;
            }
            iterator &operator-=(int n)
            {
                for (int i = 0; i < n; i++)
                    --*this;
                return *this;
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

        iterator begin()
        {
            return iterator(m_first);
        }
        iterator end()
        {
            return iterator(m_end);
        }

        explicit File(const std::shared_ptr<std::string> &fileName) : m_fileName(fileName), m_end(new Line(nullptr, nullptr, m_fileName, -1, "")), m_first(m_end)
        {
            //To allow for easier iteration
            //Example: removeLine in loop
            m_end->next = m_end;
        }

        explicit File(const std::string &filename) : File(std::make_shared<std::string>(filename)) {}

        explicit File(std::istream &file, const std::shared_ptr<std::string> &fileName) : File(fileName)
        {
            std::string tmp;
            while (std::getline(file, tmp))
            {
                append(tmp);
            }
        }

        explicit File(std::istream &file, const std::string &filename) : File(file, std::make_shared<std::string>(filename)) {}

        File(File &other) : File(other.m_fileName)
        {
            for (auto &line : other)
            {
                append(line.content);
            }
        }
        File(File &&other) : m_fileName(other.m_fileName), m_end(other.m_end), m_first(other.m_first)
        {
            other.m_first = nullptr;
        }

        File &operator=(File &other)
        {
            delete m_first;
            m_fileName = other.m_fileName;
            m_end = new Line(nullptr, nullptr, m_fileName, -1, "");
            m_first = m_end;
            m_end->next = m_end;
            for (auto &line : other)
            {
                append(line.content);
            }
            return *this;
        }
        File &operator=(File &&other)
        {
            delete m_first;
            m_fileName = other.m_fileName;
            m_first = other.m_first;
            m_end = other.m_end;
            other.m_first = nullptr;
            return *this;
        }

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
            //Checks if there is a line to append to
            if (m_first == m_end)
            {
                m_first = new Line(m_end, nullptr, m_fileName, lineNumber == -1 ? 1 : lineNumber, content);
                m_end->prev = m_first;
            }
            else
            {
                auto last = m_end->prev;
                last->next = new Line(m_end, last, m_fileName, lineNumber == -1 ? last->lineNumber + 1 : lineNumber, content);
                m_end->prev = last->next;
            }
        }

        /**
         * @brief Adds a new line before the iterator. The line number of the iterator is used.
         * 
         * The iterator stays valid.
         */
        void insertBefore(const iterator &it, std::string_view content)
        {
            if (it == end())
            {
                append(content);
                return;
            }
            auto prev = it->prev;
            auto newLine = new Line(it.current, it->prev, it.current->fileName, it->lineNumber, content);
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
                throw std::out_of_range("Tried inserting at end, not a valid iterator.");
            if (it == --end())
            {
                append(content);
                return;
            }
            auto next = it->next;
            auto newLine = new Line(next, it.current, it.current->fileName, it->lineNumber, content);
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
                throw std::out_of_range("Tried removing end, not a valid iterator");
            auto last = --end();
            if (it == begin())
            {
                it->next->prev = nullptr;
                m_first = it->next;
                it->detach();
                delete it.current;
                return m_first;
            }
            it->prev->next = it->next;
            it->next->prev = it->prev;
            auto next = it->next;
            it->detach();
            delete it.current;
            return next;
        }

        /**
         * @brief Deletes all lines from start to end.
         * 
         * @param start inclusive
         * @param end not inclusive
         * @return iterator pointing to the next line after the deleted lines  (=end)
         */
        iterator removeLines(const iterator &start, const iterator &end)
        {
            if (start == this->end())
                return end;
            for (auto it = start; it != end;)
            {
                it = removeLine(it);
            }
            return end;
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
                if (it != --end())
                    ret += '\n';
            }
            return ret;
        }

        std::string toStringWithDebug()
        {
            std::string ret;
            for (auto it = begin(); it != end(); ++it)
            {
                ret += *it->fileName + ":" + std::to_string(it->lineNumber) + ":\t" + it->content;
                if (it != --end())
                    ret += '\n';
            }
            return ret;
        }

        struct FilePos
        {
            iterator line = nullptr;
            size_t firstChar = -1;
        };
        FilePos find(char c, iterator line, size_t character)
        {
            for (; line != end(); line++)
            {
                if (auto res = line->content.find(c, character); res != line->content.npos)
                {
                    return {line, res};
                }
                character = 0;
            }
            return {end(), std::string::npos};
        }
        FilePos find(char c, iterator line)
        {
            return find(c, line, 0);
        }
        FilePos find(char c)
        {
            return find(c, m_first);
        }
        FilePos find(const std::string_view str, iterator line, size_t character)
        {
            for (; line != end(); line++)
            {
                if (auto res = line->content.find(str, character); res != line->content.npos)
                {
                    return {line, res};
                }
                character = 0;
            }
            return {end(), std::string::npos};
        }
        FilePos find(const std::string_view str, iterator line)
        {
            return find(str, line, 0);
        }
        FilePos find(const std::string_view str)
        {
            return find(str, begin());
        }
        /**
         * @brief Creates a copie of all lines in range [first, last)
         * 
         * @param start included
         * @param end The end of the range is not included
         * @return File 
         */
        File subFile(iterator start, iterator end)
        {
            File ret(m_fileName);
            for (auto it = start; it != end; it++)
            {
                ret.append(it->content, it->lineNumber);
            }
            return ret;
        }

        void insertAfter(File &&other, iterator it)
        {
            if (other.m_first == other.m_end)
                return;
            if (it == end())
                throw std::out_of_range("Tried inserting at end, not a valid iterator.");
            if (it == --end())
            {
                append(std::move(other));
                return;
            }
            auto next = it->next;
            it->next = other.m_first;
            other.m_first->prev = it.current;
            next->prev = other.m_end->prev;
            other.m_end->prev->next = next;
            other.m_first = nullptr;
            delete other.m_end;
        }

        void append(File &&other)
        {
            if (other.m_first == other.m_end)
                return;
            if (m_first == m_end)
            {

                m_first = other.m_first;
                m_end->prev = other.m_end->prev;
                other.m_first = nullptr;
                delete other.m_end;
            }
            else
            {
                m_end->prev->next = other.m_first;
                other.m_first->prev = m_end->prev;
                m_end->prev = other.m_end->prev;
                other.m_end->prev->next = m_end;
                other.m_first = nullptr;
                delete other.m_end;
            }
        }

        void insertBefore(File &&other, iterator it)
        {
            if (other.m_first == other.m_end)
                return;
            if (it == end())
            {
                append(std::move(other));
                return;
            }

            it->prev->next = other.m_first;
            other.m_first->prev = it->prev;
            it->prev = other.m_end->prev;
            other.m_end->prev->next = it.current;
            other.m_first = nullptr;
            delete other.m_end;
        }

        File extractLines(iterator start, iterator end)
        {
            if (start == end)
                return File(m_fileName);
            if (start == this->end())
                throw std::out_of_range("Tried extracting end, not a valid iterator");
            File ret;
            ret.m_first = start.current;
            ret.m_end = new Line(nullptr, nullptr, nullptr, -1, "");
            ret.m_end->next = ret.m_end;
            ret.m_end->prev = (end - 1).current;
            ret.m_end->prev->next = ret.m_end;
            if (start == begin())
            {
                end->prev = nullptr;
                m_first = end.current;
            }
            else
            {
                (start - 1)->next = end.current;
                end->prev = (start - 1).current;
                start->prev = nullptr;
            }
            return ret;
        }
    };
}