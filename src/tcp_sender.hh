#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"
#include <deque>

#include <functional>

class TCPSender
{
public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( ByteStream&& input, Wrap32 isn, uint64_t initial_RTO_ms )
    : input_( std::move( input ) )
    , isn_( isn )
    , initial_RTO_ms_( initial_RTO_ms )
    , sent_SYN_( false )
    , sent_FIN_( false )
    , current_RTO_ms_( initial_RTO_ms )
    , outstanding_collections_()
    , outstanding_bytes_( 0 )
    , received_msg_()
    , window_size_( 1 )
    , abs_secno_( 0 )
    , consecutive_retransmissions_nums_( 0 )
    , has_error_( false )
    , start_timer_( false )
    , receive_abs_secno_( 0 )
    , ticks_( 0 )
  {
    received_msg_.ackno = isn_;
    received_msg_.window_size = 1;
  }

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage make_empty_message() const;

  /* Receive and process a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Type of the `transmit` function that the push and tick methods can use to send messages */
  using TransmitFunction = std::function<void( const TCPSenderMessage& )>;

  /* Push bytes from the outbound stream */
  void push( const TransmitFunction& transmit );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called */
  void tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit );

  // Accessors
  uint64_t sequence_numbers_in_flight() const;  // For testing: how many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // For testing: how many consecutive retransmissions have happened?
  const Writer& writer() const { return input_.writer(); }
  const Reader& reader() const { return input_.reader(); }
  Writer& writer() { return input_.writer(); }

private:
  Reader& reader() { return input_.reader(); }

  ByteStream input_;
  Wrap32 isn_;
  uint64_t initial_RTO_ms_;
  bool sent_SYN_;
  bool sent_FIN_;
  uint64_t current_RTO_ms_;
  std::deque<TCPSenderMessage> outstanding_collections_;
  uint64_t outstanding_bytes_;
  TCPReceiverMessage received_msg_;
  uint16_t window_size_;
  uint64_t abs_secno_;
  uint64_t consecutive_retransmissions_nums_;
  bool has_error_;
  bool start_timer_;
  uint64_t receive_abs_secno_;
  uint64_t ticks_;
};
