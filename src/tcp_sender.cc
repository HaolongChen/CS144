#include "tcp_sender.hh"
#include "debug.hh"
#include "tcp_config.hh"
#include <iostream>

using namespace std;

// This function is for testing only; don't add extra state to support it.
uint64_t TCPSender::sequence_numbers_in_flight() const
{
  return outstanding_bytes_;
}

// This function is for testing only; don't add extra state to support it.
uint64_t TCPSender::consecutive_retransmissions() const
{
  return consecutive_retransmissions_nums_;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  if ( writer().has_error() ) {
    has_error_ = true;
    TCPSenderMessage msg = make_empty_message();
    transmit( msg );
    return;
  }
  uint64_t effective_window_size_
    = ( received_msg_.window_size == 0 && outstanding_bytes_ == 0 ) ? 1 : received_msg_.window_size;
  while ( outstanding_bytes_ < effective_window_size_ ) {
    TCPSenderMessage msg {};
    if ( !sent_SYN_ ) {
      msg.seqno = isn_;
      sent_SYN_ = true;
      msg.SYN = true;
    } else {
      msg.seqno = Wrap32::wrap( abs_secno_, isn_ );
    }
    uint64_t capacity = min( effective_window_size_ - outstanding_bytes_ - msg.SYN,
                             min( TCPConfig::MAX_PAYLOAD_SIZE, writer().reader().bytes_buffered() ) );
    if ( effective_window_size_ < outstanding_bytes_ + sent_SYN_ )
      capacity = 0;
    read( writer().reader(), capacity, msg.payload );
    cout << capacity << endl;
    if ( writer().writer().is_closed() && writer().reader().bytes_buffered() == 0 && sent_SYN_ && !sent_FIN_
         && outstanding_bytes_ + msg.sequence_length() < effective_window_size_ ) {
      msg.FIN = true;
      sent_FIN_ = true;
    }
    if ( !msg.sequence_length() )
      break;
    outstanding_collections_.push_back( msg );
    outstanding_bytes_ += msg.sequence_length();
    transmit( msg );
    abs_secno_ += msg.sequence_length();
    if ( !start_timer_ && outstanding_bytes_ ) {
      start_timer_ = true;
      current_RTO_ms_ = initial_RTO_ms_;
    }
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  TCPSenderMessage msg {};
  msg.seqno = Wrap32::wrap( abs_secno_, isn_ );
  bool err = has_error_ | writer().has_error();
  if ( err ) {
    msg.RST = true;
  }
  return msg;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  if ( msg.RST || has_error_ ) {
    has_error_ = true;
    return;
  }
  window_size_ = msg.window_size;
  received_msg_ = msg;
  if ( !outstanding_bytes_ )
    return;
  if ( msg.ackno.has_value() ) {
    if ( msg.ackno.value().unwrap( isn_, abs_secno_ ) > abs_secno_ )
      return;
    // cout << outstanding_collections_.front().sequence_length() << " " <<
    // outstanding_collections_.front().seqno.unwrap(isn_, abs_secno_) << " " << msg.ackno.value().unwrap(isn_,
    // abs_secno_) << endl;
    while ( outstanding_collections_.front().sequence_length()
                + outstanding_collections_.front().seqno.unwrap( isn_, abs_secno_ )
              <= msg.ackno.value().unwrap( isn_, abs_secno_ )
            && outstanding_bytes_ ) {
      receive_abs_secno_ += outstanding_collections_.front().sequence_length();
      outstanding_bytes_ -= outstanding_collections_.front().sequence_length();
      outstanding_collections_.pop_front();
      consecutive_retransmissions_nums_ = 0;
      current_RTO_ms_ = initial_RTO_ms_;
      if ( outstanding_bytes_ == 0 )
        start_timer_ = false;
      else
        start_timer_ = true;
      if ( outstanding_collections_.empty() )
        break;
    }
  }
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  if ( has_error_ ) {
    transmit( make_empty_message() );
    return;
  }
  if ( ms_since_last_tick >= current_RTO_ms_ && start_timer_ && outstanding_bytes_ ) {
    transmit( outstanding_collections_.front() );
    if ( window_size_ ) {
      current_RTO_ms_ = ( 1UL << consecutive_retransmissions_nums_ ) * initial_RTO_ms_;
    } else {
      current_RTO_ms_ = initial_RTO_ms_;
    }
  } else {
    current_RTO_ms_ -= ms_since_last_tick;
  }
}
