#include "shell.h"
#include "kernel.h"
#include "filesystem.h"
#include "std_lib.h"

void main() {
  fsInit();
  clearScreen();
  shell();
}

void printString(char* str) {
  while (*str != '\0') {
    if (*str == '\n') {
      interrupt10(0x0E0D, 0, 0, 0); // Print '\r'
    }
    interrupt10(0x0E00 | *str, 0, 0, 0);
    str++;
  }
}

void readString(char* buf) {
  int i = 0;
  int ax;
  char c = 0;

  while (c != '\r') {
    ax = interrupt16(0x0000, 0, 0, 0);
    c = (char) (ax & 0xFF);
    if (c == '\b') {
      if (i > 0) {
        printString("\b \b");
        i--;
      }
    } else {
      buf[i++] = c;
      interrupt10(0x0E00 | c, 0, 0, 0);
    }
  }
  buf[i - 1] = '\0';
  printString("\n");
}

void clearScreen() {
  int i, j;
  for (i = 0; i < 25; i++) {
    for (j = 0; j < 80; j++) {
      putInMemory(0xB800, 2 * (i * 80 + j), ' ');
      putInMemory(0xB800, 2 * (i * 80 + j) + 1, 0x07);
    }
  }
  interrupt10(0x0200, 0, 0, 0); // Set cursor to 0,0
}

void readSector(byte* buf, int sector) {
  int al = 1;                         // 1 sector
  int ch = div(sector, 36);           // cylinder
  int cl = mod(sector, 18) + 1;       // sector
  int dh = mod(div(sector, 18), 2);   // head
  int dl = 0;                         // drive 0
  int ax = 0x0200 | al;               // AH=0x02 (read)

  interrupt13(ax, buf, (ch << 8) | cl, (dh << 8) | dl);
}

void writeSector(byte* buf, int sector) {
  int al = 1;
  int ch = div(sector, 36);
  int cl = mod(sector, 18) + 1;
  int dh = mod(div(sector, 18), 2);
  int dl = 0;
  int ax = 0x0300 | al; // AH=0x03 (write)

  interrupt13(ax, buf, (ch << 8) | cl, (dh << 8) | dl);
}