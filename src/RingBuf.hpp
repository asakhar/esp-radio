#pragma once

#include <stddef.h>

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
template <class F, size_t Size> class RingBuf {
public:
  /**
   * RingBuf Constructor.
   */
  RingBuf() = default;

  /**
   *
   * \return the RingBuf free space in bytes. Not ISR callable.
   */
  size_t bytesFree() const {
    size_t count;
    noInterrupts();
    count = m_count;
    interrupts();
    return Size - count;
  }

  constexpr size_t capacity() const { return Size; }
  
  /**
   * \return the RingBuf free space in bytes. ISR callable.
   */
  size_t bytesFreeIsr() const { return Size - m_count; }
  /**
   * \return the RingBuf used space in bytes. Not ISR callable.
   */
  size_t bytesUsed() const {
    size_t count;
    noInterrupts();
    count = m_count;
    interrupts();
    return count;
  }
  /**
   * \return the RingBuf used space in bytes.  ISR callable.
   */
  size_t bytesUsedIsr() const { return m_count; }
  /**
   * Copy data to the RingBuf from buf.
   * The number of bytes copied may be less than count if
   * count is greater than bytesFree.
   *
   * This function may be used in an ISR with writeOut()
   * in non-interrupt code.
   *
   * \param[in] buf Location of data to be copied.
   * \param[in] count number of bytes to be copied.
   * \return Number of bytes actually copied.
   */
  size_t memcpyIn(const void *buf, size_t count) {
    const uint8_t *src = (const uint8_t *)buf;
    size_t n = Size - m_count;
    if (count > n) {
      count = n;
    }
    size_t nread = 0;
    while (nread != count) {
      n = minSize(Size - m_head, count - nread);
      memcpy(m_buf + m_head, src + nread, n);
      m_head = advance(m_head, n);
      nread += n;
    }
    m_count += nread;
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
   * \param[out] buf Location to receive the data.
   * \param[in] count number of bytes to be copied.
   * \return Number of bytes actually copied.
   */
  size_t memcpyOut(void *buf, size_t count) {
    uint8_t *dst = reinterpret_cast<uint8_t *>(buf);
    size_t nwrite = 0;
    size_t n = m_count;
    if (count > n) {
      count = n;
    }
    while (nwrite != count) {
      n = minSize(Size - m_tail, count - nwrite);
      memcpy(dst + nwrite, m_buf + m_tail, n);
      m_tail = advance(m_tail, n);
      nwrite += n;
    }
    m_count -= nwrite;
    return nwrite;
  }

private:
  uint8_t __attribute__((aligned(4))) m_buf[Size];
  volatile size_t m_count{0};
  size_t m_head{0};
  size_t m_tail{0};

  size_t advance(size_t index, size_t n) {
    index += n;
    return index < Size ? index : index - Size;
  }
  // avoid macro MIN
  size_t minSize(size_t a, size_t b) { return a < b ? a : b; }
};
