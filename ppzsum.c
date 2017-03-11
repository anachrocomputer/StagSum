/* ppzsum --- convert hex and binary file formats           1999-08-18 */
/* Copyright (c) 1999 John Honniball, Froods Software Development      */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#ifdef MSDOS
#include <io.h>
#endif

#define ERRFMT    (-1)
#define BINFMT    (0)
#define MOSFMT    (1)
#define SRECFMT   (2)
#define INTELFMT  (3)
#define HEXFMT    (4)   /* Auto-detect for input only */

#define K         (1024)

int getfmt(const char s[]);
int getblk(FILE *fp, unsigned char *buf, int fmt, const int blksiz, long int *blkaddr);
void putblk(FILE *fp, const unsigned char *buf, const int fmt, const int nbytes, long int blkaddr);
static int mostech(const char *lin, unsigned char *buf, long int *blkaddr);
static int intel86(const char *lin, unsigned char *buf, long int *blkaddr);

int main(int argc, const char *argv[])
{
   int i;
   char suff;
   unsigned char buf[256];
   FILE *infp = stdin;
   FILE *outfp = stdout;
   int infmt = BINFMT;
   int outfmt = MOSFMT;
   long int in_offset = 0x0000L;
   long int out_offset = 0x0000L;
   long int blkaddr = 0L;
   int nbytes;
   int blksiz = 16;
   unsigned int ppzsum = 0;
   unsigned int ppzcnt = 0;
   unsigned int ppzlen = 0;
   
   /* Parse command line */
   for (i = 1; i < argc; i++) {
      if (argv[i][0] == '-') {
         switch (argv[i][1]) {
         case 'i':
         case 'I':
            infmt = getfmt(argv[i]);
            break;
         case 'o':
         case 'O':
            outfmt = getfmt(argv[i]);
            break;
         case 'p':
         case 'P':
            sscanf(&argv[i][2], "%d", &nbytes);
            
            suff = argv[i][strlen(argv[i]) - 1];
            
            if ((suff == 'k') || (suff == 'K'))
               nbytes *= K;

            if ((nbytes < 1) || (nbytes > (64 * K))) {
               fprintf(stderr, "Invalid padding length (%d)\n", nbytes);
               exit(1);
            }
            
            ppzlen = nbytes;
         }
      }
   }

#ifdef MSDOS
   if (infmt == BINFMT) {
      if (setmode(fileno(infp), O_BINARY) < 0) {
         perror("Cannot set binary mode for input file");
         exit(1);
      }
   }
   
   if (outfmt == BINFMT) {
      if (setmode(fileno(outfp), O_BINARY) < 0) {
         perror("Cannot set binary mode for output file");
         exit(1);
      }
   }
#endif
   
   /* Seek over binary file header, if any */
   if ((infmt == BINFMT) && (in_offset != 0L)) {
      fseek(infp, in_offset, SEEK_SET);
   }
   
   /* Loop, copying data blocks through */
   while ((nbytes = getblk(infp, buf, infmt, blksiz, &blkaddr)) > 0) {
      putblk(outfp, buf, outfmt, nbytes, blkaddr + out_offset);
      for (i = 0; i < nbytes; i++) {
         ppzsum += buf[i];
         ppzcnt++;
      }
   }
      
   /* Check for abnormal termination */
   if (nbytes < 0) {
      if (infmt == BINFMT) {
         perror("input file read error");
      }
      else {
         fputs("input file read error\n", stderr);
      }
   }
   
   /* Write EOF record */
   switch (outfmt) {
   case MOSFMT:                    
      fprintf(outfp, ";0000010001\n");
      break;
   case SRECFMT:
      fprintf(outfp, "S9030000FC\n");
      break;
   case INTELFMT:
      fprintf(outfp, ":00000001FF\n");
      break;
   case BINFMT:
      break;
   default:
      fprintf(stderr, "in main: shouldn't happen\n");
      break;
   }

   fprintf(stderr, "PPZ checksum = %04X (%d bytes)\n", ppzsum & 0xffff, ppzcnt);
   for ( ; ppzcnt < ppzlen; ppzcnt++)
      ppzsum += 0xff;
      
   fprintf(stderr, "PPZ checksum padded = %04X\n", ppzsum & 0xffff);
   
   return (0);
}


/* getfmt --- parse format letter */

int getfmt(const char s[])
{
   int fmt = ERRFMT;

   switch (s[2]) {
   case 'b':
   case 'B':
      fmt = BINFMT;
      break;
   case 's':
   case 'S':
      fmt = SRECFMT;
      break;
   case 'i':
   case 'I':
      fmt = INTELFMT;
      break;
   case 'm':
   case 'M':
      fmt = MOSFMT;
      break;
   case 'h':
   case 'H':
      fmt = HEXFMT;
      break;
   }
   
   return (fmt);
}


/* getblk --- read a block of code from an input file */

int getblk(FILE *fp,
            unsigned char *buf,
            int fmt,
            const int blksiz,
            long int *blkaddr)
{
   static long int filepos = 0L;
   int nbytes;
   char lin[256];
   
   if (fmt == BINFMT) {
      *blkaddr = filepos;

      nbytes = fread(buf, 1, blksiz, fp);

      if (nbytes > 0)
         filepos += (long int)nbytes;
   }
   else {
      if (fgets(lin, sizeof (lin), fp) == NULL)
         return (0);    /* Normal EOF */
         
      if (fmt == HEXFMT) {
         switch (lin[0]) {
         case ':':
            fmt = INTELFMT;
            break;
         case 'S':
            fmt = SRECFMT;
            break;
         case ';':
            fmt = MOSFMT;
            break;
         default:
            fprintf(stderr, "Unrecognised input hex format\n");
            return (-1);
            break;
         }
      }

      switch (fmt) {
      case MOSFMT:
         if (lin[0] != ';') {
            fprintf(stderr, "Invalid MOS Technology hex file\n");
            return (-1);
         }
         
         nbytes = mostech(lin, buf, blkaddr);
         break;
      case SRECFMT:
         if (lin[0] != 'S') {
            fprintf(stderr, "Invalid S-record hex file\n");
            return (-1);
         }
         break;
      case INTELFMT:
         if (lin[0] != ':') {
            fprintf(stderr, "Invalid Intel hex file\n");
            return (-1);
         }

         nbytes = intel86(lin, buf, blkaddr);
         break;
      }
   }
   
#ifdef DB
   fprintf(stderr, "getblk: nbytes = %d\n", nbytes);
#endif
   
   return (nbytes);
}


/* putblk --- puts a block of code onto the object file */

void putblk(FILE *fp,
            const unsigned char *buf,
            const int fmt,
            const int nbytes,
            long int blkaddr)
{
   int i;
   unsigned int checksum;

   if (fmt == BINFMT) {
      if(fwrite (buf, 1, nbytes, fp) != nbytes)
         perror("fwrite");
   }
   else {
      switch (fmt) {
      case MOSFMT:
         fprintf(fp, ";%02X%04lX", nbytes, blkaddr);
         checksum = nbytes;
         break;
      case SRECFMT:
         fprintf(fp, "S1%02X%04lX", nbytes + 3, blkaddr);
         checksum = nbytes + 3;
         break;
      case INTELFMT:
         fprintf(fp, ":%02X%04lX00", nbytes, blkaddr);
         checksum = nbytes;
         break;
      default:
         fprintf(stderr, "in putblk: shouldn't happen\n");
         break;
      }

      checksum += blkaddr & 0xff;
      checksum += (blkaddr >> 8) & 0xff;

      for (i = 0; i < nbytes; i++) {
         fprintf(fp, "%02X", buf[i]);
         checksum += buf[i];
      }

      switch (fmt) {
      case MOSFMT:
         fprintf(fp, "%04X\n", checksum & 0xffff);
         break;
      case SRECFMT:
         checksum = ~checksum;
         fprintf(fp, "%02X\n", checksum & 0xff);
         break;
      case INTELFMT:
         checksum = ~checksum + 1;
         fprintf(fp, "%02X\n", checksum & 0xff);
         break;
      default:
         fprintf(stderr, "in putblk: shouldn't happen\n");
         break;
      }
   }
}


/* mostech */

static int mostech(const char *lin, unsigned char *buf, long int *blkaddr)
{
   unsigned int a;
   char len[3];
   char addr[5];
   char byte[3];
   char chks[5];
   int nchars;
   int slen;
   unsigned int nbytes;
   unsigned int b;
   unsigned int sum1;   /* Checksum read from file */
   unsigned int sum2;   /* Checksum computed from data bytes */
   int i, j;
   
   slen = strlen(lin);

   len[0] = lin[1];
   len[1] = lin[2];
   len[2] = '\0';
   
   addr[0] = lin[3];
   addr[1] = lin[4];
   addr[2] = lin[5];
   addr[3] = lin[6];
   addr[4] = '\0';
   
   if (sscanf(len, "%x", &nbytes) != 1) {
/*    fprintf(stderr, "%s: invalid length byte (%s) in line %d:\n", fname, len, l); */
      fprintf(stderr, "invalid length byte (%s):\n", len);
      fputs(lin, stderr);
      return (-1);
   }

   if (sscanf(addr, "%x", &a) != 1) {
/*    fprintf(stderr, "%s: invalid address (%s) in line %d:\n", fname, addr, l); */
      fprintf(stderr, "invalid address (%s):\n", addr);   
      fputs(lin, stderr);
      return (-1);
   }
   
   nchars = (nbytes * 2) + 12;
   
#ifdef DB
   printf("addr = %04x, len = %02x, nchars = %d, strlen = %d\n",
                  a,          nbytes,        nchars,      slen);
#endif
   
   if (nchars > slen) {
/*    fprintf(stderr, "%s: line %d too short:\n", fname, l); */
      fprintf(stderr, "line too short:\n");
      fputs(lin, stderr);
      return (-1);
   }
   else if (nchars < slen) {
/*    fprintf(stderr, "%s: line %d too long:\n", fname, l); */
      fprintf(stderr, "line too long:\n");
      fputs(lin, stderr);
      return (-1);
   }
   
   chks[0] = lin[nchars - 5];
   chks[1] = lin[nchars - 4];
   chks[2] = lin[nchars - 3];
   chks[3] = lin[nchars - 2];
   chks[4] = '\0';
   
   if (sscanf(chks, "%x", &sum1) != 1) {
/*    fprintf(stderr, "%s: invalid checksum (%s) in line %d:\n", fname, chks, l); */
      fprintf(stderr, "invalid checksum (%s):\n", chks);
      fputs(lin, stderr);
      return (-1);
   }
   
   sum2 = nbytes;
   sum2 += a & 0xff;
   sum2 += (a >> 8) & 0xff;

   for (i = 0, j = 7; i < nbytes; i++) {
      byte[0] = lin[j++];
      byte[1] = lin[j++];
      byte[2] = '\0';
      
      sscanf(byte, "%x", &b);
      buf[i] = b;
      sum2 += b;
   }
   
   if (sum1 != sum2) {
/*    fprintf(stderr, "%s: checksum error in line %d: computed %04x, read %s\n", fname, l, sum2, chks); */
      fprintf(stderr, "checksum error: computed %04x, read %s\n", sum2, chks);
      fputs(lin, stderr);
      return (-1);
   }
   
   *blkaddr = (long int)a;

   return (nbytes);
}


/* intel86 */

static int intel86(const char *lin, unsigned char *buf, long int *blkaddr)
{
   unsigned int a;
   unsigned int itype;
   char len[3];
   char addr[5];
   char type[3];
   char byte[3];
   char chks[3];
   int nchars;
   int slen;
   unsigned int nbytes;
   unsigned int b;
   unsigned int sum1;   /* Checksum read from file */
   unsigned int sum2;   /* Checksum computed from data bytes */
   int i, j;
   
   slen = strlen(lin);

   len[0] = lin[1];
   len[1] = lin[2];
   len[2] = '\0';
   
   addr[0] = lin[3];
   addr[1] = lin[4];
   addr[2] = lin[5];
   addr[3] = lin[6];
   addr[4] = '\0';
   
   type[0] = lin[7];
   type[1] = lin[8];
   type[2] = '\0';
   
   if (sscanf(len, "%x", &nbytes) != 1) {
/*    fprintf(stderr, "%s: invalid length byte (%s) in line %d:\n", fname, len, l); */
      fprintf(stderr, "invalid length byte (%s):\n", len);
      fputs(lin, stderr);
      return (-1);
   }

   if (sscanf(addr, "%x", &a) != 1) {
/*    fprintf(stderr, "%s: invalid address (%s) in line %d:\n", fname, addr, l); */
      fprintf(stderr, "invalid address (%s):\n", addr);   
      fputs(lin, stderr);
      return (-1);
   }
   
   if (sscanf(type, "%x", &itype) != 1) {
/*    fprintf(stderr, "%s: invalid length byte (%s) in line %d:\n", fname, len, l); */
      fprintf(stderr, "invalid record type (%s):\n", len);
      fputs(lin, stderr);
      return (-1);
   }

   nchars = (nbytes * 2) + 12;
   
#ifdef DB
   printf("addr = %04x, len = %02x, nchars = %d, strlen = %d\n",
                  a,          nbytes,        nchars,      slen);
#endif
   
   if (nchars > slen) {
/*    fprintf(stderr, "%s: line %d too short:\n", fname, l); */
      fprintf(stderr, "line too short:\n");
      fputs(lin, stderr);
      return (-1);
   }
   else if (nchars < slen) {
/*    fprintf(stderr, "%s: line %d too long:\n", fname, l); */
      fprintf(stderr, "line too long:\n");
      fputs(lin, stderr);
      return (-1);
   }
   
   chks[0] = lin[nchars - 3];
   chks[1] = lin[nchars - 2];
   chks[2] = '\0';
   
   if (sscanf(chks, "%x", &sum1) != 1) {
/*    fprintf(stderr, "%s: invalid checksum (%s) in line %d:\n", fname, chks, l); */
      fprintf(stderr, "invalid checksum (%s):\n", chks);
      fputs(lin, stderr);
      return (-1);
   }
   
   if (itype != 0)   /* EOF record */
      return (0);
      
   sum2 = nbytes;
   sum2 += a & 0xff;
   sum2 += (a >> 8) & 0xff;

   for (i = 0, j = 9; i < nbytes; i++) {
      byte[0] = lin[j++];
      byte[1] = lin[j++];
      byte[2] = '\0';
      
      sscanf(byte, "%x", &b);
      buf[i] = b;
      sum2 += b;
   }
   
   sum2 = (-sum2) & 0xff;
   
   if (sum1 != sum2) {
/*    fprintf (stderr, "%s: checksum error in line %d: computed %04x, read %s\n", fname, l, sum2, chks); */
      fprintf (stderr, "checksum error: computed %04x, read %s\n", sum2, chks);
      fputs (lin, stderr);
      return (-1);
   }
   
   *blkaddr = (long int)a;

   return (nbytes);
}
