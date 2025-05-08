#include "tcp_receiver.hh"
#include "debug.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  // Your code here.
  if ( message.RST ) {
    rst_ = true;
    reassembler_.set_error();
    return;
  }
  if ( message.SYN ) {
    zero_point_ = message.seqno;
    set_syn_ = true;
  }
  fin_ |= message.FIN;
  uint32_t secno = message.seqno.unwrap( zero_point_, 0 );
  bool flag = false;
  if ( set_syn_ ) {
    uint64_t tmp = reassembler_.writer().bytes_pushed();
    reassembler_.insert( secno - 1 + message.SYN, message.payload, message.FIN );
    if ( tmp != reassembler_.writer().bytes_pushed() )
      flag = true;
    if ( !message.payload.empty() )
      ackno_ = Wrap32::wrap( zero_point_.unwrap( zero_point_, 0 )
                               + ( flag && ( ( fin_ && !message.SYN ) || ( message.FIN && message.SYN ) ) )
                               + reassembler_.writer().bytes_pushed() + 1,
                             zero_point_ );
    else
      ackno_ = Wrap32::wrap( secno + ( ( fin_ && !message.SYN ) || ( message.FIN && message.SYN ) ) + message.SYN,
                             zero_point_ );
  }
  // cout << reassembler_.reader().peek() << endl;
  if ( fin_ && flag ) {
    reassembler_.close();
  }
}

TCPReceiverMessage TCPReceiver::send() const
{
  // Your code here.
  TCPReceiverMessage msg;
  if ( set_syn_ ) {
    msg.ackno = optional<Wrap32> { ackno_ };
  }
  msg.RST = rst_ | reassembler_.writer().has_error();
  uint64_t tmp = reassembler_.writer().available_capacity();
  if ( tmp > UINT16_MAX )
    msg.window_size = UINT16_MAX;
  else
    msg.window_size = tmp;
  return msg;
}
