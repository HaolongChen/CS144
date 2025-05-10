#include "router.hh"
#include "debug.hh"

#include <iostream>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  route_table_.emplace_back( route_prefix, prefix_length, next_hop, interface_num );
}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
  for ( const auto& interfaces : interfaces_ ) {
    queue<InternetDatagram>& dgram = interfaces->datagrams_received();
    while ( !dgram.empty() ) {
      IPv4Datagram now = dgram.front();
      dgram.pop();
      if ( now.header.ttl <= 1 )
        continue;
      auto perfect_match_it = route_table_.end();
      uint32_t dst_ip = now.header.dst;
      for ( auto it = route_table_.begin(); it != route_table_.end(); ++it ) {
        uint32_t mask = static_cast<uint32_t>( (uint64_t)( 0xffffffff ) << ( 32 - it->prefix_length ) );
        if ( ( dst_ip & mask ) == ( it->route_prefix & mask ) ) {
          if ( perfect_match_it == route_table_.end() || perfect_match_it->prefix_length < it->prefix_length ) {
            perfect_match_it = it;
          }
        }
      }
      if ( perfect_match_it == route_table_.end() )
        continue;
      now.header.ttl--;
      now.header.compute_checksum();
      if ( perfect_match_it->next_hop.has_value() )
        interface( perfect_match_it->interface_num )->send_datagram( now, perfect_match_it->next_hop.value() );
      else
        interface( perfect_match_it->interface_num )->send_datagram( now, Address::from_ipv4_numeric( dst_ip ) );
    }
  }
}
