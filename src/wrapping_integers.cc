#include "wrapping_integers.hh"
#include "debug.hh"
#include <iostream>

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // Your code here.
  uint64_t mod = 1UL << 32;
  return Wrap32( zero_point.raw_value_ + n % mod );
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  // Your code here.
  uint64_t mod = 1UL << 32;
  uint64_t first_index = Wrap32::raw_value_ - zero_point.raw_value_;
  if ( checkpoint <= first_index )
    return first_index;
  if ( first_index < 0 )
    first_index += mod;
  uint64_t remainder = ( checkpoint - first_index ) % mod;
  if ( remainder < ( 1UL << 31 ) ) {
    return checkpoint - remainder;
  }
  cout << checkpoint << " " << remainder << endl;
  return checkpoint - remainder + mod;
}
