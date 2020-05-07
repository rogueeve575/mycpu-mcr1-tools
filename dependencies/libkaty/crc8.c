00001 /*  
00002  * crc8.c
00003  * 
00004  * Computes a 8-bit CRC 
00005  * 
00006  */
00007 
00008 #include <stdio.h>
00009 
00010 
00011 #define GP  0x107   /* x^8 + x^2 + x + 1 */
00012 #define DI  0x07
00013 
00014 
00015 static unsigned char crc8_table[256];     /* 8-bit table */
00016 static int made_table=0;
00017 
00018 static void init_crc8()
00019      /*
00020       * Should be called before any other crc function.  
00021       */
00022 {
00023   int i,j;
00024   unsigned char crc;
00025   
00026   if (!made_table) {
00027     for (i=0; i<256; i++) {
00028       crc = i;
00029       for (j=0; j<8; j++)
00030         crc = (crc << 1) ^ ((crc & 0x80) ? DI : 0);
00031       crc8_table[i] = crc & 0xFF;
00032       /* printf("table[%d] = %d (0x%X)\n", i, crc, crc); */
00033     }
00034     made_table=1;
00035   }
00036 }
00037 
00038 
00039 void crc8(unsigned char *crc, unsigned char m)
00040      /*
00041       * For a byte array whose accumulated crc value is stored in *crc, computes
00042       * resultant crc obtained by appending m to the byte array
00043       */
00044 {
00045   if (!made_table)
00046     init_crc8();
00047 
00048   *crc = crc8_table[(*crc) ^ m];
00049   *crc &= 0xFF;
00050 }
