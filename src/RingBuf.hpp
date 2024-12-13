#pragma once

#include <algorithm>
#include <atomic>
#include <cstring>

/**
 * \class RingBuf
 * \brief Ring buffer for data loggers.
 *
 * This ring buffer may be used in ISRs.  bytesFreeIsr(), bytesUsedIsr(),
 * memcopyIn(), and memcopyOut() are ISR callable.  For ISR use call
 * memcopyIn() in the ISR and use writeOut() in non-interrupt code
 * to write data to a file. readIn() and memcopyOut can be use in a
 * similar way to provide file data to an ISR.
 *
 * Print into a RingBuf in an ISR should also work but has not been verified.
 */
class RingBuf {
public:
  /**
   * RingBuf Constructor.
   */
  explicit inline RingBuf(size_t cap) : m_buf{new uint8_t[cap]}, m_cap{cap} {};

  ~RingBuf() {
    m_count = 0;
    delete[] m_buf;
  }

  RingBuf(RingBuf &) = delete;
  RingBuf(RingBuf &&) = delete;
  RingBuf &operator=(RingBuf &) = delete;
  RingBuf &operator=(RingBuf &&) = delete;

  inline size_t vacant() const { return capacity() - used(); }

  inline size_t capacity() const { return m_cap; }

  inline size_t used() const { return m_count; }

  /**
   * Copy data to the RingBuf from buf.
   * The number of bytes copied may be less than count if
   * count is greater than bytesFree.
   *
   * This function may be used in an ISR with writeOut()
   * in non-interrupt code.
   *
   * \param[in] src Location of data to be copied.
   * \param[in] count number of bytes to be copied.
   * \return Number of bytes actually copied.
   */
  inline size_t memcpyIn(const uint8_t *src, size_t count) {
    if (m_lock.exchange(true)) {
      return 0;
    }
    size_t n = vacant();
    if (count > n) {
      count = n;
    }
    size_t nread = 0;
    while (nread != count) {
      n = std::min(capacity() - m_head, count - nread);
      memcpy(m_buf + m_head, src + nread, n);
      m_head = advance(m_head, n);
      nread += n;
    }
    m_count += nread;
    m_lock.store(false);
    return nread;
  }

  /**
   * Copy date from the RingBuf to buf.
   * The number of bytes copied may be less than count if
   * bytesUsed is less than count.
   *
   * This function may be used in an ISR with readIn() in
   * non-interrupt code.
   *
   * \param[out] dst Location to receive the data.
   * \param[in] count number of bytes to be copied.
   * \return Number of bytes actually copied.
   */
  inline size_t memcpyOut(uint8_t *dst, size_t count) {
    if (m_lock.exchange(true)) {
      return 0;
    }
    size_t nwrite = 0;
    size_t n = m_count;
    if (count > n) {
      count = n;
    }
    size_t tail = (m_head + capacity() - m_count) % capacity();
    while (nwrite != count) {
      n = std::min(capacity() - tail, count - nwrite);
      memcpy(dst + nwrite, m_buf + tail, n);
      tail = advance(tail, n);
      nwrite += n;
    }
    m_count -= nwrite;
    m_lock.store(false);
    return nwrite;
  }

  inline int peek() const {
    int value;
    size_t tail;
    bool truev = true;
    if (m_lock.exchange(truev)) {
      return 0;
    }
    if (m_count < 1) {
      value = -1;
      goto end;
    }
    tail = (m_head + capacity() - m_count) % capacity();
    value = m_buf[tail];

  end:
    m_lock.store(false);
    return value;
  }

  bool clear() {
    if (m_lock.exchange(true)) {
      return false;
    }
    m_count = 0;
    m_head = 0;
    m_lock.store(false);
    return true;
  }

private:
  uint8_t *m_buf{nullptr};
  size_t m_cap{0};
  size_t m_head{0};
  size_t m_count{0};
  mutable std::atomic_bool m_lock{false};

  size_t advance(size_t index, size_t n) {
    index += n;
    return index < capacity() ? index : index - capacity();
  }
};
