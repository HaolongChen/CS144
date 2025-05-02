#include "reassembler.hh"
#include "debug.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  last_index_ += output_.writer().bytes_pushed() - buf_index_;
  buf_index_ = output_.writer().bytes_pushed();
  capacity_ = output_.writer().available_capacity();
  if ( first_index >= capacity_ + index_ || !capacity_ )
    return;
  uint64_t cnt = 0;
  // cout << flag_[1] << endl;
  for ( uint64_t i = 0; i < data.size() && i + first_index < capacity_ + index_; i++ ) {
    if ( !flag_[i + first_index] ) {
      flag_[i + first_index] = true;
      buf_[i + first_index] = data[i];
      cnt++;
    }
  }
  if ( cnt ) {
    unassembled_ += cnt;
    last_send_ = 0;
  } else if ( !data.empty() )
    return;
  string tmp { "" };
  while ( flag_[index_] ) {
    tmp.push_back( buf_[index_] );
    index_++;
    last_send_++;
    unassembled_--;
  }
  if ( !tmp.empty() ) {
    output_.writer().push( tmp );
  }
  if ( is_last_substring ) {
    eof_ = true;
    last_index_ = std::min( last_index_, first_index );
    last_length_ = std::min( last_length_, first_index + data.size() - buf_index_ );
    if ( last_length_ == index_ - buf_index_ ) {
      output_.writer().close();
    }
  }
  if ( eof_ ) {
    if ( last_length_ == index_ - buf_index_ ) {
      output_.writer().close();
    }
  }
}

// How many bytes are stored in the Reassembler itself?
// This function is for testing only; don't add extra state to support it.
uint64_t Reassembler::count_bytes_pending() const
{
  return unassembled_;
}
