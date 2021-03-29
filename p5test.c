// ============================================================================
// p5test.c : use regular C library calls to check what the answers should be
// when run against the BFS filesystem
// ============================================================================

#include "p5test.h"

// ============================================================================
// Check that 'size' bytes, starting at buf[start] hold the value 'val'.
// 'testnum' is the test number - used for reporting
// ============================================================================
void check(int testnum, i8 *buf, int start, int size, int val)
{
  for (int i = start; i < start + size; ++i)
  {
    if (buf[i] != val)
    {
      printf("TEST %d : BAD  : buf[%d] = %d but should be %d \n",
             testnum, i, buf[i], val);
      return;
    }
  }
  printf("TEST %d : GOOD \n", testnum);
}

void checkStr(int testnum, i8 *buf, int start, int size, char val[])
{
  for (int i = start; i < start + size; ++i)
  {
    if (buf[i] != val[i])
    {
      printf("TEST %d : BAD  : buf[%d] = %d but should be %c \n",
             testnum, i, buf[i], val[i]);
      return;
    }
  }
  printf("TEST %d : GOOD \n", testnum);
}

// ============================================================================
// Check that 'actual' == 'expected' for test 'testnum'
// ============================================================================
void checkCursor(int testnum, int expected, int actual)
{
  if (actual == expected)
  {
    printf("TEST %d : GOOD \n", testnum);
  }
  else
  {
    printf("TEST %d : BAD  : cursor = %d but should be %d \n",
           testnum, actual, expected);
  }
}

// ============================================================================
// Create file "P5", holding 50 blocks, inside of BFSDISK, and populate
// ============================================================================
void createP5()
{

  i32 fd = fsCreate("P5");

  i8 buf[BYTESPERBLOCK];

  // Write 100 blocks.  Every byte in block 'b' the value 'b'

  for (int b = 0; b < 50; ++b)
  {
    memset(buf, b, BYTESPERBLOCK);
    fsWrite(fd, BYTESPERBLOCK, buf);
  }

  fsClose(fd);
}

// ============================================================================
// TEST 1 : Small read (100 bytes) from cursor = 0
// ============================================================================
void test1(i32 fd)
{
  i8 buf[BUFSIZE]; // buffer for reads and writes

  fsSeek(fd, 0, SEEK_SET);

  i32 curs = fsTell(fd);
  checkCursor(1, 0, curs);

  memset(buf, 0, BUFSIZE);
  i32 ret = fsRead(fd, 100, buf); // read 100 bytes from cursor = 0
  assert(ret == 100);

  curs = fsTell(fd);
  checkCursor(1, 100, curs);

  check(1, buf, 0, 100, 0);
}

// ============================================================================
// TEST 2 : Small read (200 bytes) from 30 bytes into block 1
// ============================================================================
void test2(i32 fd)
{
  i8 buf[BUFSIZE]; // buffer for reads and writes

  fsSeek(fd, 512 + 30, SEEK_SET);

  i32 curs = fsTell(fd);
  checkCursor(2, 512 + 30, curs);

  memset(buf, 0, BUFSIZE);
  i32 ret = fsRead(fd, 200, buf); // read 200 bytes from current cursor
  assert(ret == 200);

  curs = fsTell(fd);
  checkCursor(2, 512 + 30 + 200, curs);

  check(2, buf, 0, 200, 1);
}

// ============================================================================
// TEST 3 : Large, spanning read (1,000 bytes) from start of block 20
//          512*20, 488*21
// ============================================================================
void test3(i32 fd)
{
  i8 buf[BUFSIZE]; // buffer for reads and writes

  fsSeek(fd, 20 * BYTESPERBLOCK, SEEK_SET);

  i32 curs = fsTell(fd);
  checkCursor(3, 20 * 512, curs);

  memset(buf, 0, BUFSIZE);
  i32 ret = fsRead(fd, 1000, buf); // read 1,000 bytes from current cursor
  assert(ret == 1000);

  curs = fsTell(fd);
  checkCursor(3, 20 * 512 + 1000, curs);

  check(3, buf, 0, 512, 20);
  check(3, buf, 512, 488, 21);
}

// ============================================================================
// TEST 4 : Small write (77 bytes) starting at 10 bytes into block 7
//          10*7, 77*77, 425*7
// ============================================================================
void test4(i32 fd)
{
  i8 buf[BUFSIZE]; // buffer for reads and writes

  fsSeek(fd, 7 * BYTESPERBLOCK + 10, SEEK_SET);

  i32 curs = fsTell(fd);
  checkCursor(4, 7 * 512 + 10, curs);

  memset(buf, 0, BUFSIZE);
  memset(buf, 77, 77);

  fsWrite(fd, 77, buf);

  curs = fsTell(fd);
  checkCursor(4, 7 * 512 + 10 + 77, curs);

  fsSeek(fd, 7 * BYTESPERBLOCK, SEEK_SET);

  i32 ret = fsRead(fd, BYTESPERBLOCK, buf);
  assert(ret == BYTESPERBLOCK);

  check(4, buf, 0, 10, 7);
  check(4, buf, 10, 77, 77);
  check(4, buf, 87, 425, 7);
}

// ============================================================================
// TEST 5 : Large, spanning write (900 bytes) starting at 50 bytes into
//          block 10
//          50*10, 462*88, 438*88, 74*11
// ============================================================================
void test5(i32 fd)
{
  i8 buf[BUFSIZE]; // buffer for reads and writes

  fsSeek(fd, 10 * BYTESPERBLOCK + 50, SEEK_SET);

  i32 curs = fsTell(fd);
  checkCursor(5, 10 * 512 + 50, curs);

  memset(buf, 0, BUFSIZE);
  memset(buf, 88, 900);

  fsWrite(fd, 900, buf);

  curs = fsTell(fd);
  checkCursor(5, 10 * 512 + 50 + 900, curs);

  fsSeek(fd, 10 * BYTESPERBLOCK, SEEK_SET);

  curs = fsTell(fd);
  checkCursor(5, 10 * 512, curs);

  i32 ret = fsRead(fd, 2 * BYTESPERBLOCK, buf);
  assert(ret == 2 * BYTESPERBLOCK);

  curs = fsTell(fd);
  checkCursor(5, 12 * 512, curs);

  check(5, buf, 0, 50, 10);
  check(5, buf, 50, 462, 88);
  check(5, buf, 512, 438, 88);
  check(5, buf, 950, 74, 11);
}

// ============================================================================
// TEST 6 : Large, extending write (700 bytes) starting at block 49
//          512*99, 188*99, 324*0
// ============================================================================
void test6(i32 fd)
{
  i8 buf[BUFSIZE]; // buffer for reads and writes

  fsSeek(fd, 49 * BYTESPERBLOCK, SEEK_SET);

  i32 curs = fsTell(fd);
  checkCursor(6, 49 * 512, curs);

  memset(buf, 0, BUFSIZE);
  memset(buf, 99, 700);

  i32 written = fsWrite(fd, 700, buf);

  curs = fsTell(fd);
  checkCursor(6, 49 * 512 + 700, curs);

  fsSeek(fd, 49 * BYTESPERBLOCK, SEEK_SET);

  curs = fsTell(fd);
  checkCursor(6, 49 * 512, curs);

  i32 ret = fsRead(fd, 2 * BYTESPERBLOCK, buf);
  assert(ret == 700);

  curs = fsTell(fd);
  checkCursor(6, 49 * 512 + 700, curs);

  check(6, buf, 0, 512, 99);
  check(6, buf, 512, 188, 99);
  check(6, buf, 700, 324, 0); // technically beyond EOF
}

void test7()
{

  i32 fd = fsOpen("Test7");
  if (fd = EFNF)
  {
    fd = fsCreate("Test7");
  }

  for (int b = 0; b < 10; ++b)
  {
    char str[32] = "All Your BASE Are Belong To US!";
    fsWrite(fd, sizeof(str), str);
  }

  i8 buf[32] = {0}; // buffer for reads and writes
  fsSeek(fd, 0, SEEK_SET);

  i32 curs = fsTell(fd);
  checkCursor(7, 0, curs);

  printf("Testing off by zero correctness... Should be All Your BASE...!, no spaces before or after.");
  for (int i = 1; i <= 10; i++)
  {
    i32 ret = fsRead(fd, 32, buf);
    assert(ret == 32);
    curs = fsTell(fd);
    checkCursor(7, 32 * i, curs);
    printf("%s\n", buf);
  }
  fsClose(fd);
}

void test8()
{

  i32 fd = fsOpen("Test8");
  if (fd == EFNF)
  {
    fd = fsCreate("Test8");
  }
  i32 size = fsSize(fd);

  printf("Single Block Write:\n");
  i8 buf[512] = {0};

  buf[511] = 'Z';
  buf[0] = 'A';
  fsWrite(fd, 512, buf);

  fsClose(fd);

  fsSeek(fd, 0, SEEK_SET);
  i32 curs = fsTell(fd);
  checkCursor(8, 0, curs);

  i8 rBuf[1] = {0};
  i32 ret = fsRead(fd, 1, rBuf);
  assert(ret == 1);
  printf("Single Read Start Of File:\n");
  curs = fsTell(fd);
  checkCursor(8, 1, curs);
  check(8, rBuf, 0, 1, 'A');

  printf("Single Read End Of File:\n");
  fsSeek(fd, 511, SEEK_SET);
  curs = fsTell(fd);
  checkCursor(8, 511, curs);

  rBuf[0] = 0;
  ret = fsRead(fd, 1, rBuf);
  assert(ret == 1);

  curs = fsTell(fd);
  checkCursor(8, 512, curs);
  check(8, rBuf, 0, 1, 'Z');
}

void test9()
{

  i32 fd = fsOpen("Test9");
  if (fd == EFNF)
  {
    fd = fsCreate("Test9");
  }

  i8 buf[513] = {0};

  buf[512] = 'Z';
  buf[0] = 'A';
  fsWrite(fd, 513, buf);

  i32 size = fsSize(fd);
  assert(size == 513);

  fsSeek(fd, 512, SEEK_SET);
  i32 curs = fsTell(fd);
  checkCursor(9, 512, curs);

  i8 rBuf[100] = {0};
  i32 ret = fsRead(fd, 1, rBuf);
  assert(ret == 1);

  curs = fsTell(fd);
  checkCursor(9, 513, curs);
  check(9, rBuf, 0, 1, 'Z');

  printf("Reading past EOF:\n");
  i8 rBuf2[100] = {0};
  ret = fsRead(fd, 100, rBuf2);
  assert(ret == 0);
  curs = fsTell(fd);
  checkCursor(9, 513, curs);

  size = fsSize(fd);
  assert(size == 513);

  fsClose(fd);
}

void test10()
{

  i32 fd = fsOpen("Test10");
  if (fd == EFNF)
  {
    fd = fsCreate("Test10");
  }

  printf("Offset Write:\n");
  fsSeek(fd, 10, SEEK_SET);
  i8 buf[10] = {0};
  buf[9] = 'Z';
  buf[0] = 'A';
  fsWrite(fd, 10, buf);

  i32 curs = fsTell(fd);
  checkCursor(10, 20, curs);

  i32 size = fsSize(fd);
  assert(size == 20);

  printf("Appending Write:\n");
  i8 buf2[512] = {0};
  buf2[511] = 'Y';
  buf2[0] = 'B';
  fsWrite(fd, 512, buf2);

  curs = fsTell(fd);
  checkCursor(10, 532, curs);

  size = fsSize(fd);
  assert(size == 532);

  fsSeek(fd, 0, SEEK_SET);

  i8 rBuf[1000] = {0};
  int ret = fsRead(fd, 1000, rBuf);
  assert(ret == 532);

  curs = fsTell(fd);
  checkCursor(10, 532, curs);
  check(10, rBuf, 10, 1, 'A');
  check(10, rBuf, 19, 1, 'Z');
  check(10, rBuf, 20, 1, 'B');
  check(10, rBuf, 531, 1, 'Y');

  fsClose(fd);
}

void test11()
{

  i32 fd = fsOpen("Test11");
  if (fd == EFNF)
  {
    fd = fsCreate("Test11");
  }

  printf("Offset Write: 1 Block 1 Byte\n");
  fsSeek(fd, 513, SEEK_SET);
  i8 buf[20] = {0};
  buf[19] = 'Z';
  buf[0] = 'A';
  fsWrite(fd, 20, buf);

  i32 curs = fsTell(fd);
  checkCursor(11, 533, curs);

  i32 size = fsSize(fd);
  assert(size == 533);

  fsSeek(fd, 0, SEEK_SET);

  i8 rBuf[1000] = {0};
  int ret = fsRead(fd, 1000, rBuf);
  assert(ret == 533);

  curs = fsTell(fd);
  checkCursor(10, 533, curs);
  check(11, rBuf, 513, 1, 'A');
  check(11, rBuf, 532, 1, 'Z');
  check(11, rBuf, 10, 1, '\0');

  fsClose(fd);
}

void test12()
{

  i32 fd = fsOpen("Test12");
  if (fd == EFNF)
  {
    fd = fsCreate("Test12");
  }

  printf("Write 500:\n");
  fsSeek(fd, 0, SEEK_SET);
  i8 buf[500] = {0};
  buf[499] = 'Z';
  buf[0] = 'A';
  fsWrite(fd, 500, buf);

  i32 curs = fsTell(fd);
  checkCursor(12, 500, curs);

  i32 size = fsSize(fd);
  assert(size == 500);

  printf("Seek / Write to indirect 3000:\n");
  fsSeek(fd, 3000, SEEK_SET);

  i8 buf2[10] = {0};
  buf2[9] = 'Y';
  buf2[0] = 'B';
  fsWrite(fd, 10, buf2);

  curs = fsTell(fd);
  checkCursor(12, 3010, curs);

  size = fsSize(fd);
  assert(size == 3010);

  fsSeek(fd, 0, SEEK_SET);

  i8 rBuf[3020] = {0};
  int ret = fsRead(fd, 3020, rBuf);
  assert(ret == 3010);

  curs = fsTell(fd);
  checkCursor(12, 3010, curs);
  check(12, rBuf, 0, 1, 'A');
  check(12, rBuf, 499, 1, 'Z');
  check(12, rBuf, 3000, 1, 'B');
  check(12, rBuf, 3009, 1, 'Y');
  check(12, rBuf, 900, 1, '\0');

  fsClose(fd);
}

void p5test()
{
  i32 fd = fsOpen("P5"); // open "P5" for testing

  test1(fd);
  test2(fd);
  test3(fd);
  test4(fd);
  test5(fd);
  test6(fd);
  fsClose(fd);

  test7();
  test8();
  test9();
  test10();
  test11();
  test12();
}
