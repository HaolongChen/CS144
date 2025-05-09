#include <iostream>

#include "arp_message.hh"
#include "debug.hh"
#include "ethernet_frame.hh"
#include "exception.hh"
#include "helpers.hh"
#include "network_interface.hh"
#include <iostream>

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( string_view name,
                                    shared_ptr<OutputPort> port,
                                    const EthernetAddress& ethernet_address,
                                    const Address& ip_address )
  : name_( name )
  , port_( notnull( "OutputPort", move( port ) ) )
  , ethernet_address_( ethernet_address )
  , ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address_ ) << " and IP address "
       << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  EthernetFrame data;
  if ( ip2mac_.contains( next_hop.ipv4_numeric() ) ) {
    bool boardcast = false;
    if ( ticks_.contains( next_hop.ipv4_numeric() ) ) {
      if ( ticks_[next_hop.ipv4_numeric()] > 30000 ) {
        boardcast = true;
      }
    } else {
      boardcast = true;
    }
    if ( !boardcast ) {
      data.header.type = data.header.TYPE_IPv4;
      Serializer serializer;
      dgram.serialize( serializer );
      data.payload = serializer.finish();
      data.header.src = ethernet_address_;
      data.header.dst = ip2mac_[next_hop.ipv4_numeric()];
      transmit( data );
      return;
    }
  }
  waiting_datagram_[next_hop.ipv4_numeric()].push_back( dgram );
  if ( response_.contains( next_hop.ipv4_numeric() ) && response_[next_hop.ipv4_numeric()] < 5000 )
    return;
  ARPMessage msg;
  msg.opcode = msg.OPCODE_REQUEST;
  msg.sender_ethernet_address = ethernet_address_;
  msg.sender_ip_address = ip_address_.ipv4_numeric();
  msg.target_ip_address = next_hop.ipv4_numeric();

  data.header.type = data.header.TYPE_ARP;
  Serializer serializer;
  msg.serialize( serializer );
  data.payload = serializer.finish();
  data.header.dst = ETHERNET_BROADCAST;
  data.header.src = ethernet_address_;
  response_[next_hop.ipv4_numeric()] = 0;
  transmit( data );
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( EthernetFrame frame )
{
  if ( frame.header.dst != ETHERNET_BROADCAST && frame.header.dst != ethernet_address_ )
    return;
  if ( frame.header.type == frame.header.TYPE_IPv4 ) {
    InternetDatagram dgram;
    Parser parser( frame.payload );
    dgram.parse( parser );
    datagrams_received_.push( dgram );
  } else {
    ARPMessage msg;
    Parser parser( frame.payload );
    msg.parse( parser );
    ip2mac_[msg.sender_ip_address] = msg.sender_ethernet_address;
    ticks_[msg.sender_ip_address] = 0;
    if ( msg.opcode == msg.OPCODE_REQUEST ) {
      if ( ip_address_.ipv4_numeric() != msg.target_ip_address )
        return;
      ARPMessage reply;
      reply.opcode = reply.OPCODE_REPLY;
      reply.sender_ethernet_address = ethernet_address_;
      reply.sender_ip_address = ip_address_.ipv4_numeric();
      reply.target_ethernet_address = frame.header.src;
      reply.target_ip_address = msg.sender_ip_address;

      EthernetFrame eth;
      eth.header.type = eth.header.TYPE_ARP;
      Serializer serializer;
      reply.serialize( serializer );
      eth.payload = serializer.finish();
      eth.header.dst = frame.header.src;
      eth.header.src = ethernet_address_;
      transmit( eth );
    }
    for ( InternetDatagram data : waiting_datagram_[msg.sender_ip_address] ) {
      EthernetFrame eth;
      eth.header.type = eth.header.TYPE_IPv4;
      Serializer serializer;
      data.serialize( serializer );
      eth.payload = serializer.finish();
      eth.header.src = ethernet_address_;
      eth.header.dst = frame.header.src;
      transmit( eth );
    }
    waiting_datagram_.erase( msg.sender_ip_address );
  }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  for ( auto it = ticks_.begin(); it != ticks_.end(); ) {
    it->second += ms_since_last_tick;
    if ( it->second >= 30000 ) {
      ip2mac_.erase( it->first );
      it = ticks_.erase( it );
    } else {
      it = next( it );
    }
  }
  for ( auto it = response_.begin(); it != response_.end(); ) {
    it->second += ms_since_last_tick;
    if ( it->second >= 5000 ) {
      waiting_datagram_.erase( it->first );
      it = response_.erase( it );
    } else
      it = next( it );
  }
}
