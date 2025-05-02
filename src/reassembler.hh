#pragma once

#include "byte_stream.hh"
#include <vector>

class Reassembler
{
public:
  // Construct Reassembler to write into given ByteStream.
  explicit Reassembler( ByteStream&& output )
    : output_( std::move( output ) )
    , buf_( 9999999 )
    , flag_( 9999999 )
    , capacity_( output_.writer().available_capacity() )
    , unassembled_( 0 )
    , index_( 0 )
    , buf_index_( output_.writer().bytes_pushed() )
    , eof_( false )
    , last_send_( 0 )
    , last_index_( 99999999 )
    , last_length_( 99999999 )
  {}

  /*
   * Insert a new substring to be reassembled into a ByteStream.
   *   `first_index`: the index of the first byte of the substring
   *   `data`: the substring itself
   *   `is_last_substring`: this substring represents the end of the stream
   *   `output`: a mutable reference to the Writer
   *
   * The Reassembler's job is to reassemble the indexed substrings (possibly out-of-order
   * and possibly overlapping) back into the original ByteStream. As soon as the Reassembler
   * learns the next byte in the stream, it should write it to the output.
   *
   * If the Reassembler learns about bytes that fit within the stream's available capacity
   * but can't yet be written (because earlier bytes remain unknown), it should store them
   * internally until the gaps are filled in.
   *
   * The Reassembler should discard any bytes that lie beyond the stream's available capacity
   * (i.e., bytes that couldn't be written even if earlier gaps get filled in).
   *
   * The Reassembler should close the stream after writing the last byte.
   */
  void insert( uint64_t first_index, std::string data, bool is_last_substring );

  // How many bytes are stored in the Reassembler itself?
  // This function is for testing only; don't add extra state to support it.
  uint64_t count_bytes_pending() const;

  void print( void );

  // Access output stream reader
  Reader& reader() { return output_.reader(); }
  const Reader& reader() const { return output_.reader(); }

  // Access output stream writer, but const-only (can't write from outside)
  const Writer& writer() const { return output_.writer(); }

private:
  ByteStream output_;
  std::vector<char> buf_;
  std::vector<bool> flag_;
  uint64_t capacity_;
  uint64_t unassembled_;
  uint64_t index_;
  uint64_t buf_index_;
  bool eof_;
  uint64_t last_send_;
  uint64_t last_index_;
  uint64_t last_length_;
};
