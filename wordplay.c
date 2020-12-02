/*
parts released to public domain by Evans Criswell
copyright 2016 Logan Rosen, David William Richmond Jones
copyright 2017 Innocent De Marchi
copyright 2019 Moshe Piekarski, Clayton Smith

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "unicode/ustdio.h"

#define max(A, B) ((A) > (B) ? (A) : (B))
#define min(A, B) ((A) < (B) ? (A) : (B))

#define DEFAULT_WORD_FILE "words721.txt"
#define WORDBLOCKSIZE 16384
#define MAX_WORD_LENGTH 128
#define SAFETY_ZONE MAX_WORD_LENGTH + 1
#define MAX_ANAGRAM_WORDS 32
#define MAX_PATH_LENGTH 256

UChar  *uppercase (UChar *s);
UChar  *alphabetic (UChar *s);
int     numvowels (UChar *s);
void    anagramr7 (UChar *s, UChar **accum, int *minkey, int *level);
UChar  *extract (UChar *s1, UChar *s2);
int     intmask (UChar *s);

UChar **words2;  /* Candidate word index (pointers to the words) */
UChar  *words2mem;  /* Memory block for candidate words  */
UChar **words2ptrs; /* For copying the word indexes */
UChar **wordss;    /* Keys */
UChar  *keymem;     /* Memory block for keys */
int    *wordsn;    /* Lengths of each word in words2 */
int    *wordmasks; /* Mask of which letters are contained in each word */
int     ncount;    /* Number of candidate words */
int     longestlength; /*  Length of longest word in words2 array */
UChar   largestlet;
int     rec_anag_count;  /*  For recursive algorithm, keeps track of number
			 of anagrams fond */
int     adjacentdups;
int     specfirstword;
int     maxdepthspec;
int     silent;
int     input;
int     max_depth;
int     vowelcheck;

int    *lindx1;
int    *lindx2;
int     findx1[30];
int     findx2[30];
int	findx12 = 30;

UChar    pristineinitword[MAX_WORD_LENGTH];

int main (int argc, char *argv[])
{
  UFILE   *word_file_ptr;
  UChar    buffer[MAX_WORD_LENGTH];
  UChar    ubuffer[MAX_WORD_LENGTH];
  UChar    alphbuffer[MAX_WORD_LENGTH];
  UChar    initword[MAX_WORD_LENGTH];
  UChar    remaininitword[MAX_WORD_LENGTH];
  char     word_file_name[MAX_PATH_LENGTH];
  UChar    first_word[MAX_WORD_LENGTH];
  UChar    u_first_word[MAX_WORD_LENGTH];
  UChar    tempword[MAX_WORD_LENGTH];
  int      ilength;                           /* Length of initword */
  int      size;
  int      gap;
  int      switches;
  int      iholdn;
  UChar    chold;
  UChar   *wholdptr;
  int      curlen;
  int      curpos;
  UChar    curlet;
  int      icurlet;
  int      recursiveanag;
  int      listcandwords;
  int      wordfilespec;
  int      firstwordspec;
  int      maxcwordlength;
  int      mincwordlength;
  int      iarg;
  int      keyi;
  int      keyj;
  UChar  **accum;
  int      level;
  int      minkey;
  UChar    leftover[MAX_WORD_LENGTH];
  int      w2size;
  UChar   *w2memptr;
  int      w2offset;
  UChar   *keymemptr;
  int      keyoffset;
  char     no[3] = "no";
  char     yes[4] = "yes";
  int      fileinput;
  int      hasnumber;
  int      i;
  int      j;
  int      k;

/*
  printf ("Command line parameters:\n");
  for (i = 0; i < argc; i++) printf ("\"%s\" ", argv[i]);
  printf ("\n");
*/

  if (argc < 2)
  {
    fprintf (stderr,
	    "Wordplay Version 8  05-05-19 originally by Evans A Criswell\n");
    fprintf (stderr, "Usage:  ");
    fprintf (stderr, "wordplay string_to_anagram [-slxavnXmXdX] [-w word] "
		     "[-f word_file]\n\n");
    fprintf (stderr, "Capital X represents an integer.\n\n");
    fprintf (stderr, "s  = silent operation (no header or line numbers)\n");
    fprintf (stderr, "i  = include input phrase in anagram list\n");
    fprintf (stderr, "l  = print candidate word list\n");
    fprintf (stderr, "x  = do not generate anagrams (useful with l option)\n");
    fprintf (stderr, "a  = multiple occurrences of a word in an anagram OK\n");
    fprintf (stderr, "v  = allow words with no vowels to be considered\n");
    fprintf (stderr, "nX = candidate words must have n characters minimum\n");
    fprintf (stderr, "mX = candidate words must have m characters maximum\n");
    fprintf (stderr, "dX = limit anagrams to d words\n\n");
    fprintf (stderr, "w word = word to start anagrams\n");
    fprintf (stderr, "f file = word file to use (\"-f -\" for stdin)\n\n");
    fprintf (stderr, "Suggestion:  Run \"wordplay trymenow\" "
		     " to get started.\n");
    exit(-1);
  }

  strcpy (word_file_name, DEFAULT_WORD_FILE);

  recursiveanag = 1;
  listcandwords = 0;
  wordfilespec = 0;
  firstwordspec = 0;
  specfirstword = 0;     /*  this is the permanent one */
  silent = 0;
  input = 0;
  vowelcheck = 1;
  maxdepthspec = 0;

  maxcwordlength = MAX_WORD_LENGTH;
  mincwordlength = 0;

  max_depth = MAX_ANAGRAM_WORDS;

  iarg = 1;
  while (iarg < argc)
  {
    if (wordfilespec == 1)
    {
      strcpy (word_file_name, argv[iarg]);
      iarg++;
      wordfilespec = 0;
      continue;
    }
    if (firstwordspec == 1)
    {
      u_uastrcpy (first_word, argv[iarg]);
      iarg++;
      firstwordspec = 0;
      continue;
    }
    if (argv[iarg][0] == '-')
    {
      if ((int) strlen(argv[iarg]) > 1)
      {
	i = 1;
	while (i < (int) strlen(argv[iarg]))
	{
	  switch (argv[iarg][i])
	  {
            case 'a' : adjacentdups = 1;
		       break;
	    case 'l' : listcandwords = 1;
		       break;
	    case 'f' : wordfilespec = 1;
		       break;
            case 'x' : recursiveanag = 0;
		       break;
            case 's' : silent = 1;
		       break;
            case 'i' : input = 1;
                       break;
            case 'v' : vowelcheck = 0;
		       break;
            case 'w' : firstwordspec = 1;
		       specfirstword = 1;
		       break;
            case 'm' : maxcwordlength = 0;
		       i++;
		       while ((argv[iarg][i] >= '0') && (argv[iarg][i] <= '9'))
			 maxcwordlength = maxcwordlength * 10 +
					  ((int) argv[iarg][i++] - (int) '0');
                       i--;
		       break;
            case 'n' : i++;
		       while ((argv[iarg][i] >= '0') && (argv[iarg][i] <= '9'))
			 mincwordlength = mincwordlength * 10 +
					  ((int) argv[iarg][i++] - (int) '0');
                       i--;
		       break;
            case 'd' : maxdepthspec = 1;
                       max_depth = 0;
		       i++;
		       while ((argv[iarg][i] >= '0') && (argv[iarg][i] <= '9'))
			 max_depth = max_depth * 10 +
			             ((int) argv[iarg][i++] - (int) '0');
                       i--;
		       break;
            default  : fprintf (stderr, "Invalid option: \"%c\" - Ignored\n",
				argv[iarg][i]);
                       break;
	  }
	  i++;
	}
      }
      iarg++;
    }
    else
    {
      u_uastrcpy (tempword, argv[iarg]);
      u_strcpy (initword, uppercase(tempword));
      iarg++;
    }
  }

  if (silent == 0)
  {
    printf ("Wordplay Version 7.22  03-20-96, 1991   by Evans A Criswell\n");
    printf ("University of Alabama in Huntsville     criswell@cs.uah.edu\n\n");
  }

  if (silent == 0)
  {
    printf ("\n");
    printf ("Candidate word list :  %s\n", (listcandwords == 0) ? no : yes);
    printf ("Anagram Generation  :  %s\n", (recursiveanag == 0) ? no : yes);
    printf ("Adjacent duplicates :  %s\n", (adjacentdups == 0) ? no : yes);
    printf ("Vowel-free words OK :  %s\n\n", (vowelcheck == 0) ? yes : no);

    printf ("Max anagram depth   :  %d\n", max_depth);
    printf ("Maximum word length :  %d\n", maxcwordlength);
    printf ("Minimum word length :  %d\n\n", mincwordlength);

    if (specfirstword)
      printf ("First word          :  \"%s\"\n", first_word);

    printf ("Word list file      :  \"%s\"\n", word_file_name);
    u_printf ("String to anagram   :  \"%S\"\n", initword);
    printf ("\n");
  }

/* Remove non-alphabetic characters from initword */
  u_strcpy (pristineinitword, initword);
  u_strcpy (tempword, alphabetic (initword));
  if(u_strlen(initword) != u_strlen(alphabetic (initword)))
   {
   if (silent == 0) printf("Warning: Non-alphabetic characters have been removed.\n");
   }
  u_strcpy (initword, tempword);

  ilength = (int) u_strlen (initword);

/*  Sort characters of initword in increasing order  */

  size = ilength;
  gap = size;
  do
  {
    gap = max (((gap * 10) / 13), 1);
    switches = 0;
    for (i = 0; i < (size - gap); i++)
    {
      j = i + gap;
      if (initword[i] > initword[j])
      {
	chold = initword[i];
	initword[i] = initword[j];
	initword[j] = chold;
	switches++;
      }
    }
  }
  while ((switches != 0) | (gap != 1));

/*  Extract first_word (if specified) from initword and store in
    remaininitword  */

  if (specfirstword)
  {
    u_strcpy (u_first_word, uppercase(first_word));
    u_strcpy (remaininitword, extract (initword, u_first_word));
    if (remaininitword[0] == '0')
    {
      fprintf (stderr, "Specified first word \"%s\" cannot be extracted "
		       "from initial string \"%s\"\n", u_first_word, initword);
      exit (1);
    }
    if (u_strlen (remaininitword) == 0)
    {
      if (silent == 0)
      {
	printf ("Anagrams found:\n");
	printf ("     0.  %s\n", u_first_word);
      }
      else
	printf ("%s\n", u_first_word);
      exit (0);
    }

  }

/*  Allocate memory for the words themselves  */

  w2size = WORDBLOCKSIZE;

  if ((words2mem = (UChar *) malloc (w2size * sizeof (UChar))) == (UChar *) NULL)
  {
    fprintf (stderr, "Insufficient memory; malloc returned NULL.\n");
    exit (-1);
  }

/* Open the word file and read the words. */

  if (silent == 0)
  {
    printf ("\nInitializing.  Please wait while words are being loaded\n");
    printf ("and unnecessary words are being filtered out ...\n");
  }

  if (strcmp(word_file_name, "-") == 0)
  {
    fileinput = 0;
    word_file_ptr = u_finit(stdin, NULL, NULL);
  }
  else
  {
    fileinput = 1;
    if ((word_file_ptr = u_fopen (word_file_name, "r",NULL,"UTF-8")) == NULL)
    {
      fprintf (stderr, "Error opening word file.\n");
      return (-1);
    }
  }

  i = 0;
  w2memptr = words2mem;
  w2offset = 0;
  longestlength = 0;

  while (u_fgets (buffer, MAX_WORD_LENGTH, word_file_ptr) !=
	 (UChar *) NULL)
  {
    j = (int) u_strlen (buffer) - 1;

/*  Replace the newline with a null  */

    buffer[j--] = '\0';

    u_strcpy (alphbuffer, alphabetic (buffer));

    if (((int) u_strlen (alphbuffer) < mincwordlength) ||
	((int) u_strlen (alphbuffer) > maxcwordlength))
      continue;

    hasnumber = 0;
    for (j = 0; j < (int) u_strlen (buffer); j++)
      if ((buffer[j] >= '0') && (buffer[j] <= '9')) hasnumber = 1;

    if (hasnumber == 1) continue;

    u_strcpy (ubuffer, uppercase (alphbuffer));
    u_strcpy (leftover, extract (initword, ubuffer));
    if (leftover[0] == '0') continue;

    u_strcpy (w2memptr, uppercase(buffer));
    w2memptr += u_strlen (buffer) + 1;
    w2offset += u_strlen (buffer) + 1;

    if ((int) u_strlen (alphbuffer) > longestlength)
      longestlength = u_strlen (alphbuffer);

    if ((w2size - w2offset) < SAFETY_ZONE)
    {
       w2size += WORDBLOCKSIZE;
       if ((words2mem = (UChar *) realloc (words2mem, w2size)) == (UChar *) NULL)
       {
         fprintf (stderr, "Out of memory.  realloc() returned NULL.\n");
         exit (-1);
       }
       w2memptr = words2mem + w2offset;
    }

    i++;
    ncount = i;
  }

  if (fileinput == 1) u_fclose (word_file_ptr);

/* Malloc pointers for the word indexes */

  if ((words2 = (UChar **) malloc (ncount * sizeof (UChar *))) == (UChar **) NULL)
  {
    fprintf (stderr, "Insufficient memory.  malloc() returned NULL.\n");
    exit (-1);
  }

/*  Go through the loaded words and index the beginning of each word */

  words2[0] = words2mem;
  j = 1;
  for (i = 0; i < w2size; i++)
    if (j < ncount)
      if (words2mem[i] == '\0')
        words2[j++] = words2mem + i + 1;


  if (silent == 0) printf ("\n%d words loaded (%d byte block).  "
                           "Longest kept:  %d letters.\n",
			    ncount, w2size, longestlength);

  if (ncount == 0)
  {
    if (silent == 0)
      printf ("\nNo candidate words were found, so there are no anagrams.\n");
    exit(0);
  }

/* Store lengths of words from words2 array in wordsn array */

  if ((wordsn = (int *) malloc (ncount * sizeof (int))) == NULL)
  {
    fprintf (stderr, "Insufficient memory; malloc returned NULL.\n");
    exit (-1);
  }

  for (i = 0; i < ncount; i++)
  {
    u_strcpy (alphbuffer, alphabetic (words2[i]));
    wordsn[i] = (int) u_strlen (alphbuffer);
  }

/* Make a copy of the pointers from the words2 array (called words2ptrs) */

  if ((words2ptrs = (UChar **) malloc (ncount * sizeof (UChar *))) ==
      (UChar **) NULL)
  {
    fprintf (stderr, "Insufficient memory; malloc returned NULL.\n");
    exit (-1);
  }

  for (i = 0; i < ncount; i++) words2ptrs[i] = words2[i];

/* Make a copy of the word list, then sort each word in the new list
   putting letters of the words in alphabetical order */

/*  Malloc the pointers for the list of keys */

  if ((wordss = (UChar **) malloc (ncount * sizeof (UChar *))) == (UChar **) NULL)
  {
    fprintf (stderr, "Insufficient memory; malloc returned NULL.\n");
    exit (-1);
  }

/*  Make a copy of the block of memory containing the candidate word list */

  if ((keymem = (UChar *) malloc (w2size * sizeof (UChar))) == (UChar *) NULL)
  {
    fprintf (stderr, "Insufficient memory; malloc() returned NULL.\n");
    exit (-1);
  }

/*  Copy the words from the candidate word block, one by one, eliminating
    non-alphabetic characters. */

  keymemptr = keymem;
  keyoffset = 0;

  for (i = 0; i < ncount; i++)
  {
    u_strcpy (alphbuffer, alphabetic (words2[i]));
    u_strcpy (ubuffer, uppercase (alphbuffer));
    u_strcpy (keymemptr, ubuffer);
    keymemptr += wordsn[i] + 1;
    keyoffset += wordsn[i] + 1;

  }

/*  Setup the pointers to the beginnings of the words, as we did earlier
    for the candidate word indexes */

  wordss[0] = keymem;
  j = 1;
  for (i = 0; i < w2size; i++)
    if (j < ncount)
      if (keymem[i] == '\0') wordss[j++] = keymem + i + 1;

/*  Create the keys by sorting the characters of the words in the keymem space
    in place, using the wordss index pointers.  */

  for (k = 0; k < ncount; k++)
  {
    size = (int) u_strlen (wordss[k]);
    gap = size;
    do
    {
      gap = max (((gap * 10) / 13), 1);
      switches = 0;
      for (i = 0; i < (size - gap); i++)
      {
	j = i + gap;
	if (wordss[k][i] > wordss[k][j])
	{
	  chold = wordss[k][i];
	  wordss[k][i] = wordss[k][j];
	  wordss[k][j] = chold;
	  switches++;
        }
      }
    }
    while ((switches != 0) | (gap != 1));
  }

/* Sort the second "sorted" list of candidate words by first letter,
   keeping references to the original word list, sorted by length (words2)
   intact (the words2ptrs array).   */

  size = ncount;
  gap = size;
  do
  {
    gap = max (((gap * 10) / 13), 1);
    switches = 0;
    for (i = 0; i < (size - gap); i++)
    {
      j = i + gap;
      if (u_strcmp (wordss[i], wordss[j]) > 0)
      {
	wholdptr = wordss[i];
	wordss[i] = wordss[j];
	wordss[j] = wholdptr;
	wholdptr = words2ptrs[i];
	words2ptrs[i] = words2ptrs[j];
	words2ptrs[j] = wholdptr;
	switches++;
      }
    }
  }
  while ((switches != 0) | (gap != 1));
  largestlet = wordss[ncount - 1][0];

/* Sort the list of candidate words (words2 array) by length */

  size = ncount;
  gap = size;
  do
  {
    gap = max (((gap * 10) / 13), 1);
    switches = 0;
    for (i = 0; i < (size - gap); i++)
    {
      j = i + gap;
      keyi = wordsn[i];
      keyj = wordsn[j];
      if (keyi > keyj)
      {
        iholdn = wordsn[i];
        wordsn[i] = wordsn[j];
        wordsn[j] = iholdn;
        wholdptr = words2[i];
        words2[i] = words2[j];
        words2[j] = wholdptr;
        switches++;
      }
    }
  }
  while ((switches != 0) | (gap != 1));

/* Print candidate word list */

  if (listcandwords)
  {
    if (silent == 0) printf ("\nList of candidate words:\n");
    for (i = 0; i < ncount; i++)
    {
      if (silent == 0)
	printf ("%6d.  %s\n", i, words2[i]);
      else
	printf ("%s\n", words2[i]);
    }
  }


/* Create indexes into words2 array by word length.  Words of length i
   will be in elements lindx1[i] through lindx2[i] of array words2.
   Of course, the algorithm below works because words2 has already
   been sorted by word length earlier.  */

  if ((lindx1 = (int *) malloc ((longestlength + 1) * sizeof (int)))
	                == (int *) NULL)
  {
    fprintf (stderr, "Insufficient memory.  malloc() returned NULL.\n");
    exit (-1);
  }

  if ((lindx2 = (int *) malloc ((longestlength + 1) * sizeof (int)))
	                == (int *) NULL)
  {
    fprintf (stderr, "Insufficient memory.  malloc() returned NULL.\n");
    exit (-1);
  }

  for (i = 0; i <= longestlength; i++)
  {
    lindx1[i] = -1;
    lindx2[i] = -2;
  }

  if (ncount > 0)
  {
    curpos = 0;
    curlen = wordsn[curpos];
    lindx1[curlen] = curpos;
    do
    {
      while (curpos < ncount)
      {
	if (wordsn[curpos] == curlen)
	  curpos++;
        else
	  break;
      }

      if (curpos >= ncount)
      {
        lindx2[curlen] = ncount - 1;
        break;
      }
      lindx2[curlen] = curpos - 1;
      curlen = wordsn[curpos];
      lindx1[curlen] = curpos;
    }
    while (curpos < ncount);
  }

/* Create indexes into wordss array by first letter.  Words with first
   letter "A" will be will be in elements findx1[i] through findx2[i]
   of array wordss.  Of course, the algorithm below works because
   wordss has already been sorted by first letter earlier.  */

/*
  printf ("Beginning creation of first letter indexes.\n");
*/

  for (i = 0; i < findx12; i++)
  {
    findx1[i] = -1;
    findx2[i] = -2;
  }

  if (ncount > 0)
  {
    curpos = 0;
    curlet = wordss[curpos][0];
    icurlet = (int) curlet - (int) 'A';
    findx1[icurlet] = curpos;
    do
    {
      while (curpos < ncount)
      {
	if (wordss[curpos][0] == curlet)
	  curpos++;
        else
	  break;
      }
      if (curpos >= ncount)
      {
        findx2[icurlet] = ncount - 1;
        break;
      }
      findx2[icurlet] = curpos - 1;
      curlet = wordss[curpos][0];
      icurlet = (int) curlet - (int) 'A';
      findx1[icurlet] = curpos;
    }
    while (curpos < ncount);
  }

/* Create masks (integers describing which letters are in each word */

  if ((wordmasks = (int *) malloc (ncount * sizeof (int))) == NULL)
  {
    fprintf (stderr, "Insufficient memory; malloc returned NULL.\n");
    exit (-1);
  }

  for (i = 0; i < ncount; i++) wordmasks[i] = intmask (wordss[i]);

/* Do recursive method of finding anagrams */

  if ((specfirstword == 0) && (recursiveanag))
  {
    if (silent == 0) printf ("\nAnagrams found:\n");

    if ((accum = (UChar **) malloc (MAX_ANAGRAM_WORDS * sizeof (UChar *))) ==
	(UChar **) NULL)
    {
      fprintf (stderr, "Insufficient memory; malloc returned NULL.\n");
      exit(-1);
    }

    for (i = 0; i < MAX_ANAGRAM_WORDS; i++)
      if ((accum[i] = (UChar *) malloc ((longestlength + 1) * sizeof (UChar))) ==
	  (UChar *) NULL)
      {
	fprintf (stderr, "Insufficient memory; malloc returned NULL.\n");
	exit(-1);
      }

    accum[0][0] = '\0';
    level = 0;
    rec_anag_count = 0;

    minkey = findx1[(int) initword[0] - (int) 'A'];

    anagramr7 (initword, accum, &minkey, &level);
    if (rec_anag_count == 0)
      if (silent == 0)
	printf ("\nNo anagrams found by recursive algorithm.\n");
  }

  if ((specfirstword == 1) && (recursiveanag))
  {
    if (silent == 0) printf ("\nRecursive anagrams found:\n");

    if ((accum = (UChar **) malloc (MAX_ANAGRAM_WORDS * sizeof (UChar *))) ==
	(UChar **) NULL)
    {
      fprintf (stderr, "Insufficient memory; malloc returned NULL.\n");
      exit(-1);
    }

    for (i = 0; i < MAX_ANAGRAM_WORDS; i++)
      if ((accum[i] = (UChar *) malloc ((MAX_WORD_LENGTH + 1) * sizeof (UChar))) ==
	  (UChar *) NULL)
      {
	fprintf (stderr, "Insufficient memory; malloc returned NULL.\n");
	exit(-1);
      }

    u_strcpy (accum[0], u_first_word);
    level = 1;
    rec_anag_count = 0;

    minkey = findx1[(int) remaininitword[0] - (int) 'A'];

    anagramr7 (remaininitword, accum, &minkey, &level);
    if (rec_anag_count == 0)
      printf ("\nNo anagrams found by recursive algorithm.\n");

  }
  return(0);
}

UChar *uppercase (UChar *s)
{
  static UChar upcasestr[MAX_WORD_LENGTH + 1];
  int i;

  for (i = 0; i < (int) u_strlen (s); i++) upcasestr[i] = toupper(s[i]);
  upcasestr[i] = '\0';

  return (upcasestr);
}

UChar *alphabetic (UChar *s)
{
  static UChar alphstr[MAX_WORD_LENGTH + 1];
  int i, pos;

  pos = 0;
  for (i = 0; i < (int) u_strlen (s); i++)
    if (((s[i] >= 'A') && (s[i] <= 'Z')) || ((s[i] >= 'a') && (s[i] <= 'z')))
      alphstr[pos++] = s[i];
  alphstr[pos] = '\0';

  return (alphstr);
}

int numvowels (UChar *s)
{
  int vcount;
  UChar *cptr;

  vcount = 0;

  for (cptr = s; *cptr != '\0'; cptr++)
    switch (*cptr)
    {
      case 'A':  case 'E':  case 'I':  case 'O':  case 'U':  case 'Y':
	vcount++; break;
    }
  return (vcount);
}

void anagramr7 (UChar *s, UChar **accum, int *minkey, int *level)
{
  int i, j, extsuccess, icurlet, newminkey, s_mask;
  UChar exts[MAX_WORD_LENGTH];
  UChar tempword[MAX_WORD_LENGTH+50];
  UChar space[2]={' ','\0'};

/*  Print arguments passed in for debugging purposes */

/*
  printf ("------------------------------------------------\n");
  printf ("anagramr called with: (\"%s\", (", s);

  for (i = 0; i < *level; i++) printf ("\"%s\" ", accum[i]);
  printf ("), %d, %d)\n", *minkey, *level);
*/

/*  Exceeded depth specified by user */

  if (*level >= max_depth)
  {
    (*level)--;
    return;
  }

/*  If the number of allowable additional "levels" times the length of
    the longest candidate word is less than the length of the string
    passed in, we know this is a "dead end".    */

  if (maxdepthspec == 1)
    if ((max_depth - *level) * longestlength < u_strlen(s))
    {
      (*level)--;
      return;
    }

/*  If no vowels, dead end  */

  if (vowelcheck == 1)
    if (numvowels (s) == 0)
    {
      (*level)--;
      return;
    }

  s_mask = intmask (s);

/*  Try to extract words and recursively apply algortihm  */

  extsuccess = 0;

  icurlet = (int) s[0] - (int) 'A';
  for (i = max (*minkey, findx1[icurlet]); i <= findx2[icurlet]; i++)
  {

/*
    printf ("Considering word \"%s\" (key \"%s\").  s = \"%s\" and i = %d\n",
	     words2ptrs[i], wordss[i], s, i);
*/

/*  Quick check for extraction.  If it fails, the extract check will fail.
    If this one passes, we must still do the extract a few steps below.  */

    if ((s_mask | wordmasks[i]) != s_mask) continue;

/*  Word used twice in a row in accumulation -- most likely not a meaningful
    anagram -- treat as a dead end  */

    if (adjacentdups == 0)
      if ((*level > 0) && (u_strcmp (words2ptrs[i], accum[*level - 1]) == 0))
        continue;

/*  Extract a word from the string being anagrammed.  */

    u_strcpy (exts, extract (s, wordss[i]));

/*  If the extraction was not possible, we are at a "dead end"  */

    if (*exts == '0') continue;

/*  If the extraction was perfect (left no letters), we've found
    an anagram.   */

    if (*exts == '\0')
    {
      memset(tempword, '\0', sizeof(tempword));
      u_strcpy (accum[*level], words2ptrs[i]);
      for (j = 0; j < *level; j++) {u_strcat (tempword, accum[j]); u_strcat(tempword, space);}
      u_strcat(tempword, words2ptrs[i]);
      if ((input == 0) && !u_strcmp(tempword, pristineinitword))
          continue;
      rec_anag_count++;
      if (silent == 0) printf ("%6d.  ", rec_anag_count);
      u_printf ("%S\n", tempword);
      extsuccess = 1;
      continue;
    }

/*  The extraction was successful, but we must recursively call
    the procedure on what is left.  */

    extsuccess = 1;

    u_strcpy (accum[*level], words2ptrs[i]);
    (*level)++;

    if (adjacentdups == 0)
      newminkey = i + 1;
    else
      newminkey = i;

    anagramr7 (exts, accum, &newminkey, level);
  }

/*  Check to see if no extractions were a success */

  if (extsuccess == 0)
  {
    (*level)--;
    return;
  }
  (*level)--;

  return;
}

UChar *extract (UChar *s1, UChar *s2)
{

/*  Returns the characters remaining in s1 after extracting the characters
    one by one that appear in s2.  If the extraction is impossible (if s2
    contains a character not in s1), the string "0" (zero) is returned.   If
    no characters remain in s1 after the extraction, then the null string ""
    is returned.

    Examples:  extract ("STOP", "SO")  returns "TP"
               extract ("AAA", "A") returns "AA"
               extract ("BCA", "ABC") returns ""
               extract ("ABCDE", "ABF") returns "0"  ('zero', not 'oh')
*/

  static UChar r1[MAX_WORD_LENGTH];
  UChar t1[MAX_WORD_LENGTH];
  UChar *s1p, *s2p, *r1p, *s1end, *s2end;
  int found, s1len, s2len;

  r1p = r1;

  u_strcpy (t1, s1);
  s1p = t1;
  s1len = (int) u_strlen (s1p);
  s1end = s1p + s1len;

  s2p = s2;
  s2len = (int) u_strlen (s2);
  s2end = s2p + s2len;

  for (s2p = s2; s2p < s2end; s2p++)
  {
    found = 0;
    for (s1p = t1; s1p < s1end; s1p++)
    {
      if (*s2p == *s1p)
      {
        *s1p = '0';
        found = 1;
        break;
      }
    }
    if (found == 0)
    {
      *r1 = '0';
      *(r1 + 1) = '\0';
      return (r1);
    }
  }

  r1p = r1;
  for (s1p = t1; s1p < s1end; s1p++)
    if (*s1p != '0') *(r1p++) = *s1p;
  *r1p = '\0';

  return (r1);
}

int intmask (UChar *s)
{

/*  Assumes "s" is all uppercase */

  UChar *sptr;
  int mask;

  mask = 0;
  for (sptr = s; *sptr != '\0'; sptr++)
    if ((*sptr >= 'A') && (*sptr <= 'Z')) mask |= 1 << (int) (*sptr - 'A');

  return (mask);
}

