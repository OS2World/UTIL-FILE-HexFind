/*
  hex pattern finder v2.1
  additional features:
  - file specification:
    - wildcards in file spec
    - recursive search in subdirectories
    - optional also system or hidden files
  - search pattern:
    - wildcards allowed
    - text strings, optional case insensitive

  (c) 1998 - 2000 Heinz Repp
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>

#if defined __OS2__ && defined __IBMC__
  #define INCL_DOSFILEMGR
  #define INCL_DOSERRORS
  #include <os2.h>
#elif defined __WIN32__ && defined __BORLANDC__
  #include <dos.h>
  #include <dir.h>
  #define FALSE 0
  #define TRUE 1
  #define CCHMAXPATH MAXPATH
  #define CCHMAXPATHCOMP MAXPATH
  #define FILE_HIDDEN FA_HIDDEN
  #define FILE_SYSTEM FA_SYSTEM
  #define FILE_ARCHIVED FA_ARCH
  #define FILE_READONLY FA_RDONLY
#else
  #error compiler not supported
#endif

#define HF_STDIN 0
#define BUFFERSIZE 49152  /* 48 kB */

short int     subdirs = FALSE, verbose = FALSE, no_case = FALSE,
              p_length, p_signif, jumpm,  shift[256], jump[256],
              p_posit[256], letter[256],
         ch_prob [256] = {
14904, 1533, 1038, 865, 936, 615, 614, 643, 738, 450, 761, 389, 493, 602, 391, 528,
 602, 339, 282, 251, 346, 262, 285, 217, 300, 212, 211, 218, 277, 209, 262, 227,
 2663, 227, 320, 222, 532, 226, 342, 199, 314, 263, 301, 231, 344, 310, 366, 295,
 638, 409, 357, 414, 323, 297, 297, 252, 311, 293, 257, 248, 233, 272, 215, 416,
 567, 516, 313, 442, 587, 725, 513, 300, 296, 368, 171, 187, 373, 399, 351, 387,
 673, 235, 387, 442, 414, 362, 349, 301, 233, 172, 184, 184, 256, 208, 256, 444,
 247, 778, 317, 472, 558, 1365, 630, 349, 486, 783, 319, 210, 618, 397, 804, 722,
 501, 158, 810, 692, 1156, 583, 354, 692, 286, 253, 202, 162, 230, 239, 307, 275,
 722, 326, 244, 578, 254, 306, 200, 186, 685, 486, 201, 1045, 191, 366, 205, 171,
 253, 164, 167, 166, 154, 130, 140, 127, 163, 197, 234, 144, 144, 127, 130, 142,
 203, 188, 131, 140, 144, 136, 124, 120, 146, 117, 148, 117, 139, 119, 127, 127,
 193, 133, 129, 118, 146, 124, 135, 131, 228, 141, 140, 211, 142, 126, 139, 198,
 509, 292, 217, 229, 356, 126, 181, 283, 183, 178, 142, 136, 352, 133, 139, 138,
 202, 152, 158, 135, 140, 118, 131, 153, 188, 138, 133, 144, 159, 155, 127, 141,
 234, 149, 142, 136, 171, 137, 130, 127, 477, 263, 134, 221, 256, 127, 180, 175,
 371, 188, 181, 163, 206, 154, 212, 273, 355, 184, 230, 218, 384, 216, 364, 3928
 }; /* character probability in 1/100000 */
unsigned long filecount = 0L, bytecount = 0L, matchcount = 0L,
              fileopt = FILE_ARCHIVED | FILE_READONLY;
unsigned char buffer[BUFFERSIZE + 511], poschar[256];

char nibble (char c)
{
  c = tolower (c);
  if (c < '0' || c > '9' && c < 'a' || c > 'f')
  {
    fputs ("Hexadecimal pattern is not valid.\n", stderr);
    exit (2);
  }

  return c < 'a' ? c - '0' : c - ('a' - 10);
} /* nibble */


void ParseOption (char *option)
{
  do
  {
    switch (tolower (*option))
    {
      case 'i':
        switch (*++option)
        {
          case '-':
            option++;
            no_case = FALSE;
            break;
          case '+':
            option++;
          default:
            no_case = TRUE;
        } /* endswitch r */
        break;

      case 'h':
        switch (*++option)
        {
          case '-':
            option++;
            fileopt &= ~FILE_HIDDEN;
            break;
          case '+':
            option++;
          default:
            fileopt |= FILE_HIDDEN;
        } /* endswitch h */
        break;

      case 's':
        switch (*++option)
        {
          case '-':
            option++;
            fileopt &= ~FILE_SYSTEM;
            break;
          case '+':
            option++;
          default:
            fileopt |= FILE_SYSTEM;
        } /* endswitch s */
        break;

      case 'r':
        switch (*++option)
        {
          case '-':
            option++;
            subdirs = FALSE;
            break;
          case '+':
            option++;
          default:
            subdirs = TRUE;
        } /* endswitch r */
        break;

      case 'v':
        switch (*++option)
        {
          case '-':
            option++;
            verbose = FALSE;
            break;
          case '+':
            option++;
          default:
            verbose = TRUE;
        } /* endswitch r */
        break;

      default:
        fprintf (stderr, "Unknown option \"%s\".\n", option);
        exit (3);

    } /* endswitch */
  } while (*option);

} /* ParseOption */


void ParsePattern (char *hexarg)
{
  register int i, j;
  int          state, lastw;
  short int    pattern[256], p_lttr[256];

  p_length = 0;
  p_signif = 0;
  lastw = 0;
  state = 0;

  do
  {
    switch (state)
    {
      case 0:
        switch (*hexarg)
        {
          case '"':        /* double quoted string */
          case '\'':       /* single quoted string */
            state = *hexarg++;
            break;
          case ' ':
            hexarg++;      /* ignore blanks */
            break;
          case '?':        /* wildcard */
            hexarg++;
            pattern[p_length] = -1;
            lastw = ++p_length;
            break;
          default:
            pattern[p_length] = nibble (*hexarg++) << 4;
            state = 1;
        }
        break;
      case 1:
        pattern[p_length] |= nibble (*hexarg++);
        if (no_case)
          p_lttr[p_length] = FALSE;
        p_posit[p_signif++] = p_length++;
        state = 0;
        break;
      default:
        if (*hexarg == state)
        {
          hexarg++;
          state = 0;
        }
        else
        {
          pattern[p_length] = *hexarg++;
          if (no_case)
          {
            if (pattern[p_length] >= 'A' && pattern[p_length] <= 'Z')
            {
              pattern[p_length] |= 0x20;
              p_lttr[p_length] = TRUE;
            }
            else if (pattern[p_length] >= 'a' && pattern[p_length] <= 'z')
              p_lttr[p_length] = TRUE;
            else
              p_lttr[p_length] = FALSE;
          }
          p_posit[p_signif++] = p_length++;
        }
    } /* endswitch state */

  } while (*hexarg && p_length < 256); /* enddo */

  if (state == 1)
    nibble ('?');    /* causes appropriate error exit */
  if (lastw == p_length)
  {
    fputs ("Trailing wildcard is not allowed.\n", stderr);
    exit (4);
  }

  /* insertion sort p_posit up to p_signif-2 in ch_prob order */
  if (no_case)
    for (i = 'a'; i <= 'z'; i++)
      ch_prob[i] += ch_prob[i & ~0x20];

  for (i = 1; i < p_signif - 1; i++)
  {
    state = p_posit[i];
    for (j = i;
         j > 0 && ch_prob[pattern[p_posit[j - 1]]] < ch_prob[pattern[state]];
         j--) p_posit[j] = p_posit[j - 1];
    p_posit[j] = state;
  }

  /* make shift and jump tables */
  for (i = 0; i < 256; i++)
     shift[i] = p_length - lastw;
  for (i = 0; i < p_signif; i++)
  {
    poschar[i] = pattern[p_posit[i]];
    if (no_case) letter[i] = p_lttr[p_posit[i]];

    if (p_posit[i] >= lastw)
    {
      shift[poschar[i]] = p_length - 1 - p_posit[i];
      if (no_case && letter[i])
        shift[poschar[i] & ~0x20] = shift[poschar[i]];
    }
    jump[i] = p_length;
  }
  jumpm = p_length;
  /* try decreasing jumps after (partial) match */
  for (j = p_length - 1; j > 0; j--)
  {
    /* check decreasing char positions:
      if match is possible on shifted pattern after:
      - mismatch, note shift distance as possible
      - match before, try next position           */
    for (i = p_signif - 1; i >= 0; i--)
    {
      if (p_posit[i] < j || pattern[p_posit[i] - j] < 0)
        jump[i] = j;
      else if (no_case && letter[i] != p_lttr[p_posit[i] - j])
      {
        if (letter[i])
        {
          if (poschar[i] != pattern[p_posit[i] - j] | 0x20)
          {
            jump[i] = j;
            break;
          }
        }
        else
        {
          jump[i] = j;
          if ((poschar[i] | 0x20) != pattern[p_posit[i] - j])
            break;
        }
      }
      else if (poschar[i] != pattern[p_posit[i] - j])
      {
        jump[i] = j;
        break;
      }
    }
    if (i < 0)
      jumpm = j;
  }

  if (verbose)
  {
    printf ("Searching for HEX pattern ");
    for (i = 0; i < p_length; i++)
      if (pattern[i] < 0)
        printf ("?? ");
      else
        printf ("%02X ", pattern[i]);
    printf ("...\n");
  }

} /* ParsePattern */


unsigned char *FastSearch (register unsigned char *where)
{
  register int i;

  /* outer loop */
  for ( ; ; )
  {
    where += p_length - 1;

    /* inner loop */
    while (i = shift[*where])
      where += i;
    /* inner loop */

    where -= p_length - 1;

    i = p_signif - 1;
    do
      if (--i < 0)
        return where;
    while (no_case && letter[i] ?
           (where[p_posit[i]] | 0x20) == poschar[i] :
           where[p_posit[i]] == poschar[i]);

    where += jump[i];
  } /* outer loop */

} /* FastSearch */


void SearchFile (char *filename, int handle)
{
  unsigned long result, f_offset;
  unsigned char *fill, *found, pdc[12];
  int i;

  if (verbose)
    printf ("Searching %s ...\n", filename);

  f_offset = 0;
  fill = buffer;
  pdc[0] = '(';

  while ((result = read (handle, fill, BUFFERSIZE)) > 0)
  {
    /* statistics */
    bytecount += result;

    /* append pattern after end */
    fill += result;
    for (i = 0; i < p_signif; i++)
      fill[p_posit[i]] = poschar[i];

    found = FastSearch (buffer);

    while (found <= fill - p_length)
    {
      /* it's a real match - print and count it */
      sprintf (&pdc[1], "%lu", f_offset + (found - buffer));
      printf ("offset 0x%08lX%11s) in %s\n",
              f_offset + (found - buffer), pdc, filename);
      matchcount++;

      found = FastSearch (found + jumpm);
    }

    if (result < BUFFERSIZE)
    {
      break;
    }
    else
    {
      result = p_length - 1;
      memcpy (buffer, fill - result, result);
      f_offset += fill - buffer - result;
      fill = buffer + result;
    }
  }

  /* statistics */
  filecount++;

} /* SearchFile */


int SearchPath (char *path, char *mask)
{
  int rc, fhandle;
#if defined __OS2__ && defined __IBMC__
  ULONG count;
  HDIR dhandle;
  FILEFINDBUF3 finfo;
#elif defined __WIN32__ && defined __BORLANDC__
  struct ffblk finfo;
  #define achName ff_name
#endif
  char FullPathName[CCHMAXPATH];

  /* 1. search files */
  strcpy (FullPathName, path);
  strcat (FullPathName, mask);

#if defined __OS2__ && defined __IBMC__
  dhandle = HDIR_CREATE;
  count = 1;
  if (DosFindFirst (FullPathName,
                    &dhandle,
                    fileopt,
                    &finfo,
                    sizeof (finfo),
                    &count,
                    FIL_STANDARD) == NO_ERROR)
#elif defined __WIN32__ && defined __BORLANDC__
  if (findfirst (FullPathName, &finfo, fileopt) == 0)
#endif
  {
    do
    {
      strcpy (FullPathName, path);
      strcat (FullPathName, finfo.achName);

      if ((fhandle = open (FullPathName, O_RDONLY | O_BINARY, 0)) == -1)
      {
        fprintf (stderr, "Error opening file %s.\n", FullPathName);
      }
      else
      {
        SearchFile (FullPathName, fhandle);
        close (fhandle);
      }
#if defined __OS2__ && defined __IBMC__
    } while (DosFindNext (dhandle,
                          &finfo,
                          sizeof (finfo),
                          &count) == NO_ERROR); /* enddo */
#elif defined __WIN32__ && defined __BORLANDC__
    } while (findnext (&finfo) == 0);
#endif

#if defined __OS2__ && defined __IBMC__
    DosFindClose (dhandle);
#elif defined __WIN32__ && defined __BORLANDC__
    findclose (&finfo);
#endif
    rc = 0;
  }
  else
  {
    rc = 1;  /* no files found */
  } /* endif DosFindFirst */

  /* 2. if recursive search subdirectories */
  if (subdirs)
  {
    strcpy (FullPathName, path);
    strcat (FullPathName, "*");

#if defined __OS2__ && defined __IBMC__
    dhandle = HDIR_CREATE;
    count = 1;
    if (DosFindFirst (FullPathName,
                      &dhandle,
                      fileopt | MUST_HAVE_DIRECTORY,
                      &finfo,
                      sizeof (FILEFINDBUF3),
                      &count,
                      FIL_STANDARD) == NO_ERROR)
#elif defined __WIN32__ && defined __BORLANDC__
    if (findfirst (FullPathName, &finfo, fileopt | FA_DIREC) == 0)
#endif
    {
      do
      {
        if (strcmp (finfo.achName, ".") && strcmp (finfo.achName, ".."))
        {
          strcpy (FullPathName, path);
          strcat (FullPathName, finfo.achName);
          strcat (FullPathName, "\\");

          rc &= SearchPath (FullPathName, mask);
        }
#if defined __OS2__ && defined __IBMC__
      } while (DosFindNext (dhandle,
                            &finfo,
                            sizeof (finfo),
                            &count) == NO_ERROR); /* enddo */
#elif defined __WIN32__ && defined __BORLANDC__
      } while (findnext (&finfo) == 0);
#endif

#if defined __OS2__ && defined __IBMC__
      DosFindClose (dhandle);
#elif defined __WIN32__ && defined __BORLANDC__
      findclose (&finfo);
#endif
    } /* endif DosFindFirst */
  } /* endif */

  return rc;
} /* SearchPath */


int main (int argc, char *argv[])
{
  int i;
  char rootpath[CCHMAXPATH], filemask[CCHMAXPATHCOMP], *filename = NULL;

  printf ("HexFind 2.1 (c) 1998-2000 by Heinz Repp\n");

  /* parse command line: */

  /* 1. leading options */
  i = 1;
  while (i < argc && (*argv[i] == '/' || *argv[i] == '-'))
    ParseOption (++argv[i++]);

  /* 2. pattern */
  if (i < argc)
  {
    ParsePattern (argv[i]);
  }
  else
  {
    fputs ("\n"
"Usage:   HexFind [-options] [\"]<pattern>[\"] files ...\n"
"\n"
"   options:  <i>gnore case, <r>ecurse into subdirectories,\n"
"             include <h>idden or <s>ystem files, be <v>erbose\n"
" <pattern>:  one or more of [ HEX-pair | 'string' | ? ]\n"
"             (? = wildcard, pattern must be contiguous or quoted!)\n",
           stderr);
    exit (1);
  }

  /* 3. trailing options and file specs */
  while (++i < argc)
  {
    if (*argv[i] == '/' || *argv[i] == '-')
      ParseOption (++argv[i]);
    else
    {
      strcpy (rootpath, argv[i]);
      /* replace slashes by backslashes */
      for (filename = rootpath;
           filename = strchr (filename, '/');
           *filename++ = '\\');
      /* split into path and filename */
      if (filename = strrchr (rootpath, '\\'))
        filename++;   /* skip path delimiter */
      else
        filename = rootpath;      /* no path */

      if (*filename == '\0')      /* only path given */
      {
        strcpy (filemask, "*");
      }
      else
      {
        struct stat stbuff;

        if (strpbrk (filename, "?*") == NULL &&
                 stat (rootpath, &stbuff) == 0 &&
                 stbuff.st_mode & S_IFDIR)
        /* no wildcards, name exists and is a directory */
        {
          strcat (rootpath, "\\");
          strcpy (filemask, "*");
        }
        else
        {
          strcpy (filemask, filename);
          *filename = '\0';
        }
      }

      if (SearchPath (rootpath, filemask))
        fprintf (stderr, "Found no files matching '%s%s'.\n", rootpath, filemask);
    } /* endif */
  } /* endwhile all arguments */

  /* if no file spec until now */
  if (filename == NULL)
  {
    setmode (HF_STDIN, O_BINARY);
    SearchFile ("StdIn", HF_STDIN);
  }

  if (verbose)
    printf ("Found %lu occurence(s) in %lu file(s) (%lu bytes searched).\n",
             matchcount, filecount, bytecount);

  exit (0);
} /* main */
