00001 /*  
00002  * crc16.c
00003  * 
00004  * Computes a 16-bit CRC 
00005  * 
00006  */
00007 
00008 #include <stdio.h>
00009 
00010 
00011 #define GP 0x11021    /* x^16 + x^12 + x^5 + 1 */
00012 #define DI 0x1021    
00013 
00014 
00015 static short crc16_table[256];     /* 8-bit table */
00016 static int made_table=0;
00017 
00018 static void init_crc16()
00019      /*
00020       * Should be called before any other crc function.  
00021       */
00022 {
00023   int i,j;
00024   short crc;
00025     
00026   if (!made_table) {
00027     for (i=0; i<256; i++) {
00028       crc = (i << 8);
00029       for (j=0; j<8; j++)
00030         crc = (crc << 1) ^ ((crc & 0x8000) ? DI : 0);
00031       crc16_table[i] = crc & 0xFFFF;
00032     }
00033     made_table=1;
00034   }
00035 }
00036 
00037 
00038 void crc16(short *crc, unsigned char m)
00039      /*
00040       * For a byte array whose accumulated crc value is stored in *crc, computes
00041       * resultant crc obtained by appending m to the byte array
00042       */
00043 {
00044   if (!made_table)
00045     init_crc16();
00046   *crc = crc16_table[(((*crc) >> 8) ^ m) & 0xFF] ^ ((*crc) << 8);
00047   *crc &= 0xFFFF;
00048 }
00049 
00050 
00051 
00052 
