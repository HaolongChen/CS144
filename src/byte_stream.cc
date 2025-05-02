#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity )
  : capacity_( capacity ), error_( false ), stream_( "" ), close_( false ), push_len_( 0 ), pop_len_( 0 )
{}

void Writer::push( string new_data )
{
  (void)new_data;
  if ( error_ || close_ )
    return;
  string data = new_data;
  if ( data.size() > Writer::available_capacity() )
    data.erase( Writer::available_capacity(), data.size() - Writer::available_capacity() );
  stream_.append( data );
  push_len_ += data.size();
}

void Writer::close()
{
  // Your code here.
  close_ = true;
}

bool Writer::is_closed() const
{
  return close_; // Your code here.
}

uint64_t Writer::available_capacity() const
{
  return capacity_ - push_len_ + pop_len_; // Your code here.
}

uint64_t Writer::bytes_pushed() const
{
  return push_len_; // Your code here.
}

string_view Reader::peek() const
{
  return string_view( stream_ ); // Your code here.
}

void Reader::pop( uint64_t len )
{
  (void)len; // Your code here.
  if ( bytes_buffered() == 0 )
    return;
  if ( len > push_len_ - pop_len_ ) {
    stream_.erase( 0, push_len_ - pop_len_ );
    pop_len_ = push_len_;
  } else {
    stream_.erase( 0, len );
    pop_len_ += len;
  }
}

bool Reader::is_finished() const
{
  return ( push_len_ == pop_len_ && close_ ); // Your code here.
}

uint64_t Reader::bytes_buffered() const
{
  return push_len_ - pop_len_; // Your code here.
}

uint64_t Reader::bytes_popped() const
{
  return pop_len_; // Your code here.
}
