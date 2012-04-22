int 
div100(long long dividend)
{
  long long divisor = 0x28f5c29;
  return ((divisor * dividend)>>32) & 0xffffffff;
}

