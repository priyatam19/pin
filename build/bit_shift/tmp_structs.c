# 0 "temp_no_pp.c"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "temp_no_pp.c"
# 10 "temp_no_pp.c"
int rand (void);





void bit_shift_001 ()
{
 int a = 1;
 int ret;
 ret = a << 32;
        sink = ret;
}





void bit_shift_002 ()
{
 long a = 1;
 long ret;
 ret = a << 32;
        sink = ret;
}





void bit_shift_003 ()
{
 unsigned int a = 1;
 unsigned int ret;
 ret = a << 32;
        sink = ret;
}





void bit_shift_004 ()
{
 unsigned long a = 1;
 unsigned long ret;
 ret = a << 32;
        sink = ret;
}





void bit_shift_005 ()
{
 int a = 1;
 int ret;
 ret = a << -1;
        sink = ret;
}





void bit_shift_006 ()
{
 int a = 1;
 int ret;
 ret = a >> 32;
        sink = ret;
}





void bit_shift_007 ()
{
 int a = 1;
 int ret;
 ret = a >> -1;
        sink = ret;
}





void bit_shift_008 ()
{
 int a = 1;
 int shift = 32;
 int ret;
 ret = a << shift;
        sink = ret;
}





void bit_shift_009 ()
{
 int a = 1;
 int shift;
 int ret;
 shift = rand();
 ret = a << shift;
        sink = ret;
}





void bit_shift_010 ()
{
 int a = 1;
 int shift = 6;
 int ret;
 ret = a << ((5 * shift) + 2);
        sink = ret;
}





void bit_shift_011 ()
{
 int a = 1;
 int shift = 5;
 int ret;
 ret = a << ((shift * shift) + 7);
        sink = ret;
}





int bit_shift_012_func_001 ()
{
 return 32;
}

void bit_shift_012 ()
{
 int a = 1;
 int ret;
 ret = a << bit_shift_012_func_001();
        sink = ret;
}





void bit_shift_013_func_001 (int shift)
{
 int a = 1;
 int ret;
 ret = a << shift;
        sink = ret;
}

void bit_shift_013 ()
{
 bit_shift_013_func_001(32);
}





void bit_shift_014 ()
{
 int a = 1;
 int shifts[5] = {8, 40, 16, 32, 24};
 int ret;
 ret = a << shifts[3];
        sink = ret;
}





void bit_shift_015 ()
{
 int a = 1;
 int shift = 32;
 int shift1;
 int ret;
 shift1 = shift;
 ret = a << shift1;
        sink = ret;
}





void bit_shift_016 ()
{
 int a = 1;
 int shift = 32;
 int shift1;
 int shift2;
 int ret;
 shift1 = shift;
 shift2 = shift1;
 ret = a << shift2;
        sink = ret;
}





void bit_shift_017 ()
{
 int ret;
 ret = 1 << 32;
        sink = ret;
}





extern volatile int vflag;
void bit_shift_main ()
{
 if (vflag == 1 || vflag ==888)
 {
  bit_shift_001();
 }

 if (vflag == 2 || vflag ==888)
 {
  bit_shift_002();
 }

 if (vflag == 3 || vflag ==888)
 {
  bit_shift_003();
 }

 if (vflag == 4 || vflag ==888)
 {
  bit_shift_004();
 }

 if (vflag == 5 || vflag ==888)
 {
  bit_shift_005();
 }

 if (vflag == 6 || vflag ==888)
 {
  bit_shift_006();
 }

 if (vflag == 7 || vflag ==888)
 {
  bit_shift_007();
 }

 if (vflag == 8 || vflag ==888)
 {
  bit_shift_008();
 }

 if (vflag == 9 || vflag ==888)
 {
  bit_shift_009();
 }

 if (vflag == 10 || vflag ==888)
 {
  bit_shift_010();
 }

 if (vflag == 11 || vflag ==888)
 {
  bit_shift_011();
 }

 if (vflag == 12 || vflag ==888)
 {
  bit_shift_012();
 }

 if (vflag == 13 || vflag ==888)
 {
  bit_shift_013();
 }

 if (vflag == 14 || vflag ==888)
 {
  bit_shift_014();
 }

 if (vflag == 15 || vflag ==888)
 {
  bit_shift_015();
 }

 if (vflag == 16 || vflag ==888)
 {
  bit_shift_016();
 }

 if (vflag == 17 || vflag ==888)
 {
  bit_shift_017();
 }
}
