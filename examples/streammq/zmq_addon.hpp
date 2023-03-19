/*
    Copyright (c) 2016-2017 ZeroMQ community
    Copyright (c) 2016 VOCA AS / Harald Nøkland

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to
    deal in the Software without restriction, including without limitation the
    rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
    sell copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
    IN THE SOFTWARE.
*/

#ifndef __ZMQ_ADDON_HPP_INCLUDED__
#define __ZMQ_ADDON_HPP_INCLUDED__

#include "zmq.hpp"

#include <deque>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#ifdef ZMQ_CPP11
#include <limits>
#include <functional>
#include <unordered_map>
#endif

namespace zmq
{
#ifdef ZMQ_CPP11

namespace detail
{
template<bool CheckN, class OutputIt>
recv_result_t
recv_multipart_n(socket_ref s, OutputIt out, size_t n, recv_flags flags)
{
    size_t msg_count = 0;
    message_t msg;
    while (true) {
        if ZMQ_CONSTEXPR_IF (CheckN) {
            if (msg_count >= n)
                throw std::runtime_error(
                  "Too many message parts in recv_multipart_n");
        }
        if (!s.recv(msg, flags)) {
            // zmq ensures atomic delivery of messages
            assert(msg_count == 0);
            return {};
        }
        ++msg_count;
        const bool more = msg.more();
        *out++ = std::move(msg);
        if (!more)
            break;
    }
    return msg_count;
}

inline bool is_little_endian()
{
    const uint16_t i = 0x01;
    return *reinterpret_cast<const uint8_t *>(&i) == 0x01;
}

inline void write_network_order(unsigned char *buf, const uint32_t value)
{
    if (is_little_endian()) {
        ZMQ_CONSTEXPR_VAR uint32_t mask = (std::numeric_limits<std::uint8_t>::max)();
        *buf++ = static_cast<unsigned char>((value >> 24) & mask);
        *buf++ = static_cast<unsigned char>((value >> 16) & mask);
        *buf++ = static_cast<unsigned char>((value >> 8) & mask);
        *buf++ = static_cast<unsigned char>(value & mask);
    } else {
        std::memcpy(buf, &value, sizeof(value));
    }
}

inline uint32_t read_u32_network_order(const unsigned char *buf)
{
    if (is_little_endian()) {
        return (static_cast<uint32_t>(buf[0]) << 24)
               + (static_cast<uint32_t>(buf[1]) << 16)
               + (static_cast<uint32_t>(buf[2]) << 8)
               + static_cast<uint32_t>(buf[3]);
    } else {
        uint32_t value;
        std::memcpy(&value, buf, sizeof(value));
        return value;
    }
}
} // namespace detail

/*  Receive a multipart message.
    
    Writes the zmq::message_t objects to OutputIterator out.
    The out iterator must handle an unspecified number of writes,
    e.g. by using std::back_inserter.
    
    Returns: the number of messages received or nullopt (on EAGAIN).
    Throws: if recv throws. Any exceptions thrown
    by the out iterator will be propagated and the message
    may have been only partially received with pending
    message parts. It is adviced to close this socket in that event.
*/
template<class OutputIt>
ZMQ_NODISCARD recv_result_t recv_multipart(socket_ref s,
                                           OutputIt out,
                                           recv_flags flags = recv_flags::none)
{
    return detail::recv_multipart_n<false>(s, std::move(out), 0, flags);
}

/*  Receive a multipart message.
    
    Writes at most n zmq::message_t objects to OutputIterator out.
    If the number of message parts of the incoming message exceeds n
    then an exception will be thrown.
    
    Returns: the number of messages received or nullopt (on EAGAIN).
    Throws: if recv throws. Throws std::runtime_error if the number
    of message parts exceeds n (exactly n messages will have been written
    to out). Any exceptions thrown
    by the out iterator will be propagated and the message
    may have been only partially received with pending
    message parts. It is adviced to close this socket in that event.
*/
template<class OutputIt>
ZMQ_NODISCARD recv_result_t recv_multipart_n(socket_ref s,
                                             OutputIt out,
                                             size_t n,
                                             recv_flags flags = recv_flags::none)
{
    return detail::recv_multipart_n<true>(s, std::move(out), n, flags);
}

/*  Send a multipart message.
    
    The range must be a ForwardRange of zmq::message_t,
    zmq::const_buffer or zmq::mutable_buffer.
    The flags may be zmq::send_flags::sndmore if there are 
    more message parts to be sent after the call to this function.
    
    Returns: the number of messages sent (exactly msgs.size()) or nullopt (on EAGAIN).
    Throws: if send throws. Any exceptions thrown
    by the msgs range will be propagated and the message
    may have been only partially sent. It is adviced to close this socket in that event.
*/
template<class Range
#ifndef ZMQ_CPP11_PARTIAL
         ,
         typename = typename std::enable_if<
           detail::is_range<Range>::value
           && (std::is_same<detail::range_value_t<Range>, message_t>::value
               || detail::is_buffer<detail::range_value_t<Range>>::value)>::type
#endif
         >
send_result_t
send_multipart(socket_ref s, Range &&msgs, send_flags flags = send_flags::none)
{
    using std::begin;
    using std::end;
    auto it = begin(msgs);
    const auto end_it = end(msgs);
    size_t msg_count = 0;
    while (it != end_it) {
        const auto next = std::next(it);
        const auto msg_flags =
          flags | (next == end_it ? send_flags::none : send_flags::sndmore);
        if (!s.send(*it, msg_flags)) {
            // zmq ensures atomic delivery of messages
            assert(it == begin(msgs));
            return {};
        }
        ++msg_count;
        it = next;
    }
    return msg_count;
}

/* Encode a multipart message.

   The range must be a ForwardRange of zmq::message_t.  A
   zmq::multipart_t or STL container may be passed for encoding.

   Returns: a zmq::message_t holding the encoded multipart data.

   Throws: std::range_error is thrown if the size of any single part
   can not fit in an unsigned 32 bit integer.

   The encoding is compatible with that used by the CZMQ function
   zmsg_encode(), see https://rfc.zeromq.org/spec/50/.
   Each part consists of a size followed by the data.
   These are placed contiguously into the output message.  A part of
   size less than 255 bytes will have a single byte size value.
   Larger parts will have a five byte size value with the first byte
   set to 0xFF and the remaining four bytes holding the size of the
   part's data.
*/
template<class Range
#ifndef ZMQ_CPP11_PARTIAL
         ,
         typename = typename std::enable_if<
           detail::is_range<Range>::value
           && (std::is_same<detail::range_value_t<Range>, message_t>::value
               || detail::is_buffer<detail::range_value_t<Range>>::value)>::type
#endif
         >
message_t encode(const Range &parts)
{
    size_t mmsg_size = 0;

    // First pass check sizes
    for (const auto &part : parts) {
        const size_t part_size = part.size();
        if (part_size > (std::numeric_limits<std::uint32_t>::max)()) {
            // Size value must fit into uint32_t.
            throw std::range_error("Invalid size, message part too large");
        }
        const size_t count_size =
          part_size < (std::numeric_limits<std::uint8_t>::max)() ? 1 : 5;
        mmsg_size += part_size + count_size;
    }

    message_t encoded(mmsg_size);
    unsigned char *buf = encoded.data<unsigned char>();
    for (const auto &part : parts) {
        const uint32_t part_size = static_cast<uint32_t>(part.size());
        const unsigned char *part_data =
          static_cast<const unsigned char *>(part.data());

        if (part_size < (std::numeric_limits<std::uint8_t>::max)()) {
            // small part
            *buf++ = (unsigned char) part_size;
        } else {
            // big part
            *buf++ = (std::numeric_limits<uint8_t>::max)();
            detail::write_network_order(buf, part_size);
            buf += sizeof(part_size);
        }
        std::memcpy(buf, part_data, part_size);
        buf += part_size;
    }

    assert(static_cast<size_t>(buf - encoded.data<unsigned char>()) == mmsg_size);
    return encoded;
}

/*  Decode an encoded message to multiple parts.

    The given output iterator must be a ForwardIterator to a container
    holding zmq::message_t such as a zmq::multipart_t or various STL
    containers.

    Returns the ForwardIterator advanced once past the last decoded
    part.

    Throws: a std::out_of_range is thrown if the encoded part sizes
    lead to exceeding the message data bounds.

    The decoding assumes the message is encoded in the manner
    performed by zmq::encode(), see https://rfc.zeromq.org/spec/50/.
 */
template<class OutputIt> OutputIt decode(const message_t &encoded, OutputIt out)
{
    const unsigned char *source = encoded.data<unsigned char>();
    const unsigned char *const limit = source + encoded.size();

    while (source < limit) {
        size_t part_size = *source++;
        if (part_size == (std::numeric_limits<std::uint8_t>::max)()) {
            if (static_cast<size_t>(limit - source) < sizeof(uint32_t)) {
                throw std::out_of_range(
                  "Malformed encoding, overflow in reading size");
            }
            part_size = detail::read_u32_network_order(source);
            // the part size is allowed to be less than 0xFF
            source += sizeof(uint32_t);
        }

        if (static_cast<size_t>(limit - source) < part_size) {
            throw std::out_of_range("Malformed encoding, overflow in reading part");
        }
        *out = message_t(source, part_size);
        ++out;
        source += part_size;
    }

    assert(source == limit);
    return out;
}

#endif


#ifdef ZMQ_HAS_RVALUE_REFS

/*
    This class handles multipart messaging. It is the C++ equivalent of zmsg.h,
    which is part of CZMQ (the high-level C binding). Furthermore, it is a major
    improvement compared to zmsg.hpp, which is part of the examples in the ØMQ
    Guide. Unnecessary copying is avoided by using move semantics to efficiently
    add/remove parts.
*/
class multipart_t
{
  private:
    std::deque<message_t> m_parts;

  public:
    typedef std::deque<message_t>::value_type value_type;

    typedef std::deque<message_t>::iterator iterator;
    typedef std::deque<message_t>::const_iterator const_iterator;

    typedef std::deque<message_t>::reverse_iterator reverse_iterator;
    typedef std::deque<message_t>::const_reverse_iterator const_reverse_iterator;

    // Default constructor
    multipart_t() {}

    // Construct from socket receive
    multipart_t(socket_ref socket) { recv(socket); }

    // Construct from memory block
    multipart_t(const void *src, size_t size) { addmem(src, size); }

    // Construct from string
    multipart_t(const std::string &string) { addstr(string); }

    // Construct from message part
    multipart_t(message_t &&message) { add(std::move(message)); }

    // Move constructor
    multipart_t(multipart_t &&other) ZMQ_NOTHROW { m_parts = std::move(other.m_parts); }

    // Move assignment operator
    multipart_t &operator=(multipart_t &&other) ZMQ_NOTHROW
    {
        m_parts = std::move(other.m_parts);
        return *this;
    }

    // Destructor
    virtual ~multipart_t() { clear(); }

    message_t &operator[](size_t n) { return m_parts[n]; }

    const message_t &operator[](size_t n) const { return m_parts[n]; }

    message_t &at(size_t n) { return m_parts.at(n); }

    const message_t &at(size_t n) const { return m_parts.at(n); }

    iterator begin() { return m_parts.begin(); }

    const_iterator begin() const { return m_parts.begin(); }

    const_iterator cbegin() const { return m_parts.cbegin(); }

    reverse_iterator rbegin() { return m_parts.rbegin(); }

    const_reverse_iterator rbegin() const { return m_parts.rbegin(); }

    iterator end() { return m_parts.end(); }

    const_iterator end() const { return m_parts.end(); }

    const_iterator cend() const { return m_parts.cend(); }

    reverse_iterator rend() { return m_parts.rend(); }

    const_reverse_iterator rend() const { return m_parts.rend(); }

    // Delete all parts
    void clear() { m_parts.clear(); }

    // Get number of parts
    size_t size() const { return m_parts.size(); }

    // Check if number of parts is zero
    bool empty() const { return m_parts.empty(); }

    // Receive multipart message from socket
    bool recv(socket_ref socket, int flags = 0)
    {
        clear();
        bool more = true;
        while (more) {
            message_t message;
#ifdef ZMQ_CPP11
            if (!socket.recv(message, static_cast<recv_flags>(flags)))
                return false;
#else
            if (!socket.recv(&message, flags))
                return false;
#endif
            more = message.more();
            add(std::move(message));
        }
        return true;
    }

    // Send multipart message to socket
    bool send(socket_ref socket, int flags = 0)
    {
        flags &= ~(ZMQ_SNDMORE);
        bool more = size() > 0;
        while (more) {
            message_t message = pop();
            more = size() > 0;
#ifdef ZMQ_CPP11
            if (!socket.send(message, static_cast<send_flags>(
                                        (more ? ZMQ_SNDMORE : 0) | flags)))
                return false;
#else
            if (!socket.send(message, (more ? ZMQ_SNDMORE : 0) | flags))
                return false;
#endif
        }
        clear();
        return true;
    }

    // Concatenate other multipart to front
    void prepend(multipart_t &&other)
    {
        while (!other.empty())
            push(other.remove());
    }

    // Concatenate other multipart to back
    void append(multipart_t &&other)
    {
        while (!other.empty())
            add(other.pop());
    }

    // Push memory block to front
    void pushmem(const void *src, size_t size)
    {
        m_parts.push_front(message_t(src, size));
    }

    // Push memory block to back
    void addmem(const void *src, size_t size)
    {
        m_parts.push_back(message_t(src, size));
    }

    // Push string to front
    void pushstr(const std::string &string)
    {
        m_parts.push_front(message_t(string.data(), string.size()));
    }

    // Push string to back
    void addstr(const std::string &string)
    {
        m_parts.push_back(message_t(string.data(), string.size()));
    }

    // Push type (fixed-size) to front
    template<typename T> void pushtyp(const T &type)
    {
        static_assert(!std::is_same<T, std::string>::value,
                      "Use pushstr() instead of pushtyp<std::string>()");
        m_parts.push_front(message_t(&type, sizeof(type)));
    }

    // Push type (fixed-size) to back
    template<typename T> void addtyp(const T &type)
    {
        static_assert(!std::is_same<T, std::string>::value,
                      "Use addstr() instead of addtyp<std::string>()");
        m_parts.push_back(message_t(&type, sizeof(type)));
    }

    // Push message part to front
    void push(message_t &&message) { m_parts.push_front(std::move(message)); }

    // Push message part to back
    void add(message_t &&message) { m_parts.push_back(std::move(message)); }

    // Alias to allow std::back_inserter()
    void push_back(message_t &&message) { m_parts.push_back(std::move(message)); }

    // Pop string from front
    std::string popstr()
    {
        std::string string(m_parts.front().data<char>(), m_parts.front().size());
        m_parts.pop_front();
        return string;
    }

    // Pop type (fixed-size) from front
    template<typename T> T poptyp()
    {
        static_assert(!std::is_same<T, std::string>::value,
                      "Use popstr() instead of poptyp<std::string>()");
        if (sizeof(T) != m_parts.front().size())
            throw std::runtime_error(
              "Invalid type, size does not match the message size");
        T type = *m_parts.front().data<T>();
        m_parts.pop_front();
        return type;
    }

    // Pop message part from front
    message_t pop()
    {
        message_t message = std::move(m_parts.front());
        m_parts.pop_front();
        return message;
    }

    // Pop message part from back
    message_t remove()
    {
        message_t message = std::move(m_parts.back());
        m_parts.pop_back();
        return message;
    }

    // get message part from front
    const message_t &front() { return m_parts.front(); }

    // get message part from back
    const message_t &back() { return m_parts.back(); }

    // Get pointer to a specific message part
    const message_t *peek(size_t index) const { return &m_parts[index]; }

    // Get a string copy of a specific message part
    std::string peekstr(size_t index) const
    {
        std::string string(m_parts[index].data<char>(), m_parts[index].size());
        return string;
    }

    // Peek type (fixed-size) from front
    template<typename T> T peektyp(size_t index) const
    {
        static_assert(!std::is_same<T, std::string>::value,
                      "Use peekstr() instead of peektyp<std::string>()");
        if (sizeof(T) != m_parts[index].size())
            throw std::runtime_error(
              "Invalid type, size does not match the message size");
        T type = *m_parts[index].data<T>();
        return type;
    }

    // Create multipart from type (fixed-size)
    template<typename T> static multipart_t create(const T &type)
    {
        multipart_t multipart;
        multipart.addtyp(type);
        return multipart;
    }

    // Copy multipart
    multipart_t clone() const
    {
        multipart_t multipart;
        for (size_t i = 0; i < size(); i++)
            multipart.addmem(m_parts[i].data(), m_parts[i].size());
        return multipart;
    }

    // Dump content to string
    std::string str() const
    {
        std::stringstream ss;
        for (size_t i = 0; i < m_parts.size(); i++) {
            const unsigned char *data = m_parts[i].data<unsigned char>();
            size_t size = m_parts[i].size();

            // Dump the message as text or binary
            bool isText = true;
            for (size_t j = 0; j < size; j++) {
                if (data[j] < 32 || data[j] > 127) {
                    isText = false;
                    break;
                }
            }
            ss << "\n[" << std::dec << std::setw(3) << std::setfill('0') << size
               << "] ";
            if (size >= 1000) {
                ss << "... (too big to print)";
                continue;
            }
            for (size_t j = 0; j < size; j++) {
                if (isText)
                    ss << static_cast<char>(data[j]);
                else
                    ss << std::hex << std::setw(2) << std::setfill('0')
                       << static_cast<short>(data[j]);
            }
        }
        return ss.str();
    }

    // Check if equal to other multipart
    bool equal(const multipart_t *other) const ZMQ_NOTHROW
    {
        return *this == *other;
    }

    bool operator==(const multipart_t &other) const ZMQ_NOTHROW
    {
        if (size() != other.size())
            return false;
        for (size_t i = 0; i < size(); i++)
            if (at(i) != other.at(i))
                return false;
        return true;
    }

    bool operator!=(const multipart_t &other) const ZMQ_NOTHROW
    {
        return !(*this == other);
    }

#ifdef ZMQ_CPP11

    // Return single part message_t encoded from this multipart_t.
    message_t encode() const { return zmq::encode(*this); }

    // Decode encoded message into multiple parts and append to self.
    void decode_append(const message_t &encoded)
    {
        zmq::decode(encoded, std::back_inserter(*this));
    }

    // Return a new multipart_t containing the decoded message_t.
    static multipart_t decode(const message_t &encoded)
    {
        multipart_t tmp;
        zmq::decode(encoded, std::back_inserter(tmp));
        return tmp;
    }

#endif

  private:
    // Disable implicit copying (moving is more efficient)
    multipart_t(const multipart_t &other) ZMQ_DELETED_FUNCTION;
    void operator=(const multipart_t &other) ZMQ_DELETED_FUNCTION;
}; // class multipart_t

inline std::ostream &operator<<(std::ostream &os, const multipart_t &msg)
{
    return os << msg.str();
}

#endif // ZMQ_HAS_RVALUE_REFS

#if defined(ZMQ_BUILD_DRAFT_API) && defined(ZMQ_CPP11) && defined(ZMQ_HAVE_POLLER)
class active_poller_t
{
  public:
    active_poller_t() = default;
    ~active_poller_t() = default;

    active_poller_t(const active_poller_t &) = delete;
    active_poller_t &operator=(const active_poller_t &) = delete;

    active_poller_t(active_poller_t &&src) = default;
    active_poller_t &operator=(active_poller_t &&src) = default;

    using handler_type = std::function<void(event_flags)>;

    void add(zmq::socket_ref socket, event_flags events, handler_type handler)
    {
        if (!handler)
            throw std::invalid_argument("null handler in active_poller_t::add");
        auto ret = handlers.emplace(
          socket, std::make_shared<handler_type>(std::move(handler)));
        if (!ret.second)
            throw error_t(EINVAL); // already added
        try {
            base_poller.add(socket, events, ret.first->second.get());
            need_rebuild = true;
        }
        catch (...) {
            // rollback
            handlers.erase(socket);
            throw;
        }
    }

    void remove(zmq::socket_ref socket)
    {
        base_poller.remove(socket);
        handlers.erase(socket);
        need_rebuild = true;
    }

    void modify(zmq::socket_ref socket, event_flags events)
    {
        base_poller.modify(socket, events);
    }

    size_t wait(std::chrono::milliseconds timeout)
    {
        if (need_rebuild) {
            poller_events.resize(handlers.size());
            poller_handlers.clear();
            poller_handlers.reserve(handlers.size());
            for (const auto &handler : handlers) {
                poller_handlers.push_back(handler.second);
            }
            need_rebuild = false;
        }
        const auto count = base_poller.wait_all(poller_events, timeout);
        std::for_each(poller_events.begin(),
                      poller_events.begin() + static_cast<ptrdiff_t>(count),
                      [](decltype(base_poller)::event_type &event) {
                          assert(event.user_data != nullptr);
                          (*event.user_data)(event.events);
                      });
        return count;
    }

    ZMQ_NODISCARD bool empty() const noexcept { return handlers.empty(); }

    size_t size() const noexcept { return handlers.size(); }

  private:
    bool need_rebuild{false};

    poller_t<handler_type> base_poller{};
    std::unordered_map<socket_ref, std::shared_ptr<handler_type>> handlers{};
    std::vector<decltype(base_poller)::event_type> poller_events{};
    std::vector<std::shared_ptr<handler_type>> poller_handlers{};
};     // class active_poller_t
#endif //  defined(ZMQ_BUILD_DRAFT_API) && defined(ZMQ_CPP11) && defined(ZMQ_HAVE_POLLER)


} // namespace zmq

#endif // __ZMQ_ADDON_HPP_INCLUDED__
