/* Extracted from GNU coreutils cksum_vmull.c */
/* Function: bswap_neon */

#include <stdio.h>

uint64x2_t
bswap_neon (uint64x2_t in){
  uint64x2_t a =
    vreinterpretq_u64_u8 (vrev64q_u8 (vreinterpretq_u8_u64 (in)));
  a = vcombine_u64 (vget_high_u64 (a), vget_low_u64 (a));
  return a;
}
