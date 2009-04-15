#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define OUTPUT_GIF  1
#define OUTPUT_PBM  2
#define OUTPUT_PS   3
#define OUTPUT_RAW  4
#define OUTPUT_PS_BITS  5
#define OUTPUT_EPS  6


// #define SHORT_PS    need to define this if you want old style postscript
//     ending



/*
This program is free software; you can redistribute it and/or modify
it under the terms of the Lesser GNU General Public License as published by
the Free Software Foundation; */

/*
The pdf417_encode program is distributed in the hope that it will be useful,
 but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser  Public
License for more details.  */

/* usage:      pdf417_encode infile outfile [rows] [cols] [ec_level] */

/* the infile and outfile args are required and specify the input and */
/* output files respectively.  The optional rows and cols arguments */
/* allow the user to specify number of rows ( < 90) and number of columns */
/* ( < 30) in the output barcode.  The default if these args are not */
/* supplied is 24 rows and 8 columns. Also if rows are supplied, then */
/* columns must be supplied also.   The last paramater is the ec_level*/
/* This specifies the level of error correction to use.  It is optionally */
/* supplied only if rows and columns are also supplied  */

/* This work is based somewhat on the pdf417_enc code written by */
/*              Ian Goldberg at UC-Berkeley */
/*                                        */
/* Authors: John Lien (jtlien@charter.net) */
/*          Laszlo Agoston   lagaston@bmw.co.at */
/*          Tom Tollenaere  t2@pophost.eunet.be (gif output routines )  */
/*     Useful examples provided by "Tom Tollenaere" t2@pophost.eunet.be  */

/* Revisions:  1.0    4/18/00     Initial release */
/* Revisions:  1.1    4/31/00     Added code to support differenct error */
/*                                correction levels                      */
/* Revisions:  1.2    6/08/00     Fixed the PBM output file              */
/* Revisions:  1.3    11/28/00    Fixed overrun problem found by Laszlo  */
/*                                Combined get_cvalue and get_ctype       */
/*                                 Routines (per Laszlo )                */
/*                                Fixed problems with byte_compress      */
/*                                   remainder > 900 should be           */
/*                                   remainder > 899                     */
/*                                 Also in numeric compress              */
/*                                   fix b900_result[mm] > 900            */
/*                                 Should be b900_result[mm] > 899       */
/* Revisions:  1.4    12/6/00    Add code to do \NL \CR \LF in text compress*/
/* Revisions:  1.5    12/7/00    Fix problem with 901 code at beginning */
/*                                  and default to text compress mode   */
/*                               Fix problem in do_row_begin and        */
/*                                  do_row_end.  Number of columns      */
/*                                  was wrong for right indicator       */
/*                                  and left indicator :                */
/*                                Rewrote the text_compress to work     */
/*                                  better.                             */
/*                                                                      */
/* Revisions:  1.6    12/8/00    Fixed more problems with text_compress  */
/*                                Folded in Tom Tollenaere gif output code */
/*                                Update Makefile                         */
/* Revisions:  1.7    3/13/01    Fixed problem with PDF417_STOP  */
/*                                Submitted by "nobody" at sourceforge    */
/*                                                                        */
/* Revisions:  1.8    5/17/01    Fixed problem switching between          */
/*                                text compress and byte compact mode     */
/*                                Found by Chris Cowan and Ray Sweeney    */
/* Revisions:  1.9    6/17/01    Added 900 pads for case when number      */
/*                                codewords used for message plus         */
/*                                Error Codewords < row * cols -2         */
/*                               also error if number of codewords is too */
/*                            large. Thanks to Frederic Ramanaga for this */
/*                                Added test for ecc syndromes to make    */
/*                                sure ecc is calculated correctly        */
/*                                Also, added Reed Solomon Decoder        */
/*                                Code from Phil Karns rs.c               */
/*                                (Not debugged yet...)                   */
/* Revisions:  2.0    10/12/01    Reed Solomon Decoder success            */
/*                                if test_ecc is turned on, ecc can       */
/*                                now be decoded!!!!                      */
/*                                Checked up to four errors corrected     */
/*                                at a time                               */
/* Revisions:  2.1    10/21/01    Fix problems with rows = 999            */
/*                                and postscript output                   */
/*                                Thanks to Jeff Chua                     */
/* Revisions:  2.2    10/24/01    Fix problems with rows = 999            */
/*                                and postscript output, added top and    */
/*                                bottom clear space to postscript out    */
/* Revisions:  2.3    11/04/01    Fix problems with rows = 999            */
/*                                Added better command line handling      */
/*                                submitted by Andrew Baker               */
/* Revisions:  2.4    11/21/01    Fixes for easier Virtual C++ compile    */
/*                                Thanks to Chris Wille                   */
/*                                New pdf417_en function that does most   */
/*                                of the work                             */
/* Revisions:  2.5    11/30/01    Fixes for problems with text compress   */
/*                                found by Tomas Gruszewski   */
/*                                                                        */
/* Revisions:  2.6    12/11/01    Made stdout default output for ps, pbm  */
/*                                Added -t psbits for ps bitmap only      */
/*                                Made the in_data lines wider            */
/*                                All to make this easier to use as       */
/*                                 as a printer filter.                   */
/*                                Changes due to Stephan Leemburg         */
/*                                Also,    Raod Plank had some fixes to   */
/*                                allow input lines > 256, also as a fix  */
/*                                to the nc routine                       */
/* Revisions:  2.7    12/19/01    Removed some dead code from generateEC */
/*                                routine.                                */
/*                                Commented out some code in eras_dec_rs  */
/*                                that caused some problems when using    */
/*                                erasures.                               */
/* Revision    2.8    10/25/02    No changes, just added the scan         */
/*                                    binary to decode the generated      */
/*                                    barcode                             */
/* Revision    3.1    4/13/03     Fixed a bug in PDF417_encodeGIF         */
/*                                  Found by Dash Wendrzyk                */
/* Revision    3.2    6/06/03     Added a makefile for freeBSD and        */
/*                                 code in main.c for bsd compilation     */
/*                                  Thanks to Ken Marx                    */
/* Revision    3.3    7/04/03     Added code to demonstrate usage as      */
/*                                   a callable shared library            */
/*                                 lib_test.c   is an example of a        */
/*                                  program calling the library functions */
/*                                 pdf417.h is the include used by caller */
/*                                 make lib     will make libary          */
/*                                                                        */
/* Revision    3.4    9/01/03     Fixed some errors in the pdf417_dec    */
/*                                 program.     */
/*                                                                        */
/* Revision    3.5   10/10/03     Fixed bug in get_line that changes HT   */
/*                                ( Horizontal Tab) to Space              */
/*                                Added code to allow double quotes       */
/*                                 in TC ( text compress) using \DQ       */
/*                                Added the code written by Vladislav     */
/*                                 Naumov with help from Paulo Soares    */
/*                                  to allow support for Macro PDF        */
/*                                you can now have a line such as         */
/*                                 MC "FILE ID  XXXXX Segment 2/2"        */
/*                                That specified a macro pdf file id     */
/*                                and macro pdf segment index.  This     */
/*                                must be the last line of input file    */
/*                                Also fixed bug in text_compress that   */
/*                               failed for puctuation followed by space */
/*                                                                       */
/* Revision    3.6   11/30/03     Added Borland subdirectory of code     */
/*                                  for Borland C++ due to Robert Grobau */
/*                                of U.S. Navy.  Added support for       */
/*                                stdin file and stdout file ( just      */
/*                                replace normal file name with _ )      */
/*                                                                       */
/*                                                                       */
/*                                                                       */
/*                                                                       */
/* NOTE ON COMPILATION for non-linux systems!!!!!                        */
/*  use the Makefile.nogif                                               */
/*     and remove the #define DO_GIFS definition from the source         */
/*  otherwise, the compilation expects the gif_lib.h to be in            */
/*  /usr/include/gif_lib.h                                               */
/* you can get the gif lib at                                            */
/* ftp://prtr-13.ucsc.edu/pub/libungif/libungif-4.1.0b1.tar.gz           */
/*                                                                       */


#include <ctype.h>
/* #include "globs.h" */
#include "bits.h"
#include "pdf417.h"

#ifdef DO_GIFS

#include "gif_lib.h"
#define min(a,b)        ((a) < (b) ? (a) : (b))

static GifColorType EGAPalette[] = {
  {0, 0, 0},                    /* 0. Black */
  {0, 0, 170},                  /* 1. Blue */
  {0, 170, 0},                  /* 2. Green */
  {0, 170, 170},                /* 3. Cyan */
  {170, 0, 0},                  /* 4. Red */
  {170, 0, 170},                /* 5. Magenta */
  {170, 170, 0},                /* 6. Brown */
  {170, 170, 170},              /* 7. LightGray */
  {85, 85, 85},                 /* 8. DarkGray */
  {85, 85, 255},                /* 9. LightBlue */
  {85, 255, 85},                /* 10. LightGreen */
  {85, 255, 255},               /* 11. LightCyan */
  {255, 85, 85},                /* 12. LightRed */
  {255, 85, 255},               /* 13. LightMagenta */
  {255, 255, 85},               /* 14. Yellow */
  {255, 255, 255},              /* 15. White */
};

#define EGA_PALETTE_SIZE (sizeof(EGAPalette) / sizeof(GifColorType))


static int
HandleGifError (GifFileType * GifFile)
{
  int i = GifLastError ();

  if (EGifCloseFile (GifFile) == GIF_ERROR)
    {
      GifLastError ();
    }
  return i;
}                               /* HandleGifError */
#endif

/* 
 * that would be nice to pre-declare
 * all functions before use
 */
void text_compress (char *);
void do_mode_latch (int incode);


void
put_back (int inval)
{


  int debug;
  debug = 0;

  if (debug)
    {
      printf ("Code = %d codeindex = %d \n", inval, codeindex);
    }
  if (debug)
    {
      printf (" left = %d right = %d \n", inval / 30, inval % 30);
    }
  if (codeindex < 1900)
    {
      codes[codeindex] = inval;
      data[codeindex] = inval;
      codeindex += 1;
    }

}                               /* put back */

void
generateEC (UInt32 * data, UInt32 len, UInt32 EClen)
{
  static int mods128[128] =  { 539, 108, 4,  801, 86, 550, 315, 834,
  517, 37, 865, 827, 297, 539, 330, 211, 34, 804, 587, 
  814, 130, 354, 454, 845, 898, 375, 269, 600, 726, 242, 
  374, 157, 616, 670, 432, 87, 614, 89, 336, 754, 602, 
  380, 262, 382, 378, 627, 663, 784, 898, 808, 821, 228,
  48, 507, 90, 447, 270, 95, 732, 156, 763, 297, 491, 171, 
  776, 646, 463, 712, 379, 40, 173, 278, 775, 192, 287, 236,
  186, 129, 219, 193, 569, 860, 606, 686, 432, 684, 96, 272,
  292, 674, 723, 594, 907, 457, 258, 516, 704, 490, 908,
  292, 631, 447, 148, 246, 620, 583, 244, 928, 208, 217, 
  93, 822, 415, 749, 925, 400, 444, 897, 779, 53, 379, 
  296, 580, 858, 547, 864, 310, 521 };   

static int mods256[256] = { 10, 497, 55, 648, 558, 924, 159, 307, 543,
  642, 555, 497, 95, 688, 251, 404, 173, 481, 579, 505,
  656, 41, 402, 83, 449, 872, 29, 167, 20, 189, 829, 609,
  532, 910, 191, 922, 299, 834, 54, 134, 297, 54, 213, 760,
  749, 228, 383, 605, 754, 893, 791, 180, 441, 609, 60, 620, 
  792, 331, 681, 32, 126, 342, 914, 73, 550, 647, 9, 319, 566,
  280, 394, 836, 238, 500, 144, 32, 795, 49, 118, 503, 528, 
  726, 421, 434, 773, 231, 60, 756, 755, 697, 309, 655, 852,
  261, 544, 895, 621, 732, 60, 48, 641, 805, 707, 689, 193,
  455, 640, 607, 752, 177, 384, 722, 234, 687, 673, 142, 605,
  192, 601, 374, 318, 263, 71, 739, 130, 479, 320, 628, 134,
  10, 462, 188, 588, 355, 580, 802, 49, 329, 903, 655, 199, 
  743, 290, 2, 90, 906, 538, 136, 585, 848, 433, 730, 830, 
  463, 269, 917, 255, 66, 731, 637, 470, 289, 225, 517, 8,
  51, 511, 103, 908, 399, 358, 19, 712, 803, 291, 307, 521,
  849, 376, 334, 461, 684, 812, 749, 162, 877, 274, 490, 793,
  329, 70, 315, 884, 209, 549, 284, 257, 216, 240, 325, 694,
  370, 859, 353, 668, 592, 418, 439, 137, 799, 700, 801, 913,
  540, 605, 796, 204, 11, 569, 68, 89, 919, 814, 710, 757, 828,
  280, 201, 280, 194, 733, 438, 850, 375, 190, 275, 913, 311,
  194, 858, 720, 138, 786, 905, 250, 708, 586, 82, 204, 74,
  857, 882, 766, 75, 894, 524 }; 

 int mods512[512] = { 263, 303, 121, 31, 244, 389, 410, 675, 
  738, 282, 304, 366, 251, 863, 310, 63, 647, 849, 223,
  768, 752, 357, 655, 498, 325, 600, 726, 165, 899, 332,
  164, 407, 780, 45, 919, 141, 669, 877, 411, 646, 138, 
  736, 284, 596, 20, 111, 45, 717, 433, 381, 14, 134, 175,
  533, 752, 762, 321, 297, 531, 190, 464, 616, 171, 447, 
  656, 46, 610, 721, 486, 316, 540, 576, 672, 640, 488, 667,
  105, 534, 299, 342, 316, 669, 54, 321, 749, 242, 424, 311,
  923, 39, 5, 330, 742, 720, 357, 37, 842, 687, 379, 691,
  366, 62, 353, 751, 667, 116, 923, 183, 296, 610, 273, 375,
  437, 286, 336, 535, 425, 16, 62, 187, 817, 907, 422, 155,
  308, 498, 162, 660, 791, 469, 73, 281, 922, 843, 303, 905,
  815, 210, 782, 673, 535, 699, 881, 321, 249, 548, 216, 425,
  731, 590, 806, 459, 915, 647, 31, 378, 795, 473, 309, 851,
  808, 397, 665, 756, 310, 560, 87, 61, 631, 307, 224, 720, 
  837, 838, 797, 242, 341, 252, 56, 513, 662, 381, 537, 141, 827, 
  892, 300, 207, 441, 305, 420, 789, 905, 644, 519, 437, 922,
  830, 613, 228, 492, 54, 369, 407, 66, 18, 680, 787, 45, 184,
  247, 331, 751, 695, 632, 757, 269, 346, 597, 77, 833, 424,
  586, 618, 777, 35, 536, 289, 860, 632, 841, 617, 870, 579,
  410, 708, 353, 248, 310, 677, 772, 181, 365, 563, 608, 109, 
  693, 343, 564, 563, 209, 158, 417, 193, 59, 624, 729, 672, 159,
  713, 663, 873, 145, 62, 559, 714, 644, 560, 521, 721, 383,
  842, 408, 43, 688, 699, 777, 589, 327, 511, 394, 594, 288, 
  245, 420, 472, 711, 283, 911, 578, 364, 764, 850, 475, 777,
  662, 119, 427, 43, 552, 332, 298, 124, 37, 578, 452, 591,
  699, 51, 651, 463, 35, 173, 244, 342, 167, 452, 902, 45, 
  669, 820, 533, 466, 684, 51, 289, 797, 856, 323, 578, 361,
  248, 95, 771, 729, 152, 543, 851, 499, 2, 521, 114, 494, 8,
  94, 707, 736, 827, 836, 850, 357, 288, 785, 358, 92, 221, 477, 
  164, 512, 841, 861, 65, 229, 864, 162, 575, 708, 40, 402, 304,
  168, 89, 710, 676, 207, 531, 429, 151, 237, 156, 641, 415, 381,
  570, 303, 786, 596, 328, 826, 190, 168, 384, 610, 51, 848, 768,
  395, 794, 518, 240, 258, 516, 192, 513, 390, 744, 567, 729, 
  899, 653, 76, 561, 536, 287, 684, 808, 142, 913, 593, 451, 297,
  20, 180, 482, 62, 99, 85, 855, 407, 681, 204, 290, 741, 658,
  860, 784, 107, 761, 472, 752, 98, 485, 800, 383, 272, 122, 289,
  542, 41, 552, 916, 858, 37, 545, 632, 290, 499, 476, 102, 644,
  390, 408, 781, 539, 534, 794, 268, 640, 621, 781, 346, 249,
  666, 203, 543, 435, 520, 327, 75, 846, 781, 352, 193, 87, 88,
  306, 871, 294, 643, 229, 299, 914, 155, 920, 265, 197, 492,
  350, 380, 285, 498, 118, 574, 409, 207, 428, 599, 35, 504, 373,
  77, 352 };  
  
  UInt32 base_reg[800];
  UInt32 coeff_reg[800];
  UInt32 i, j;
  int tint;
  UInt32 temp;
  UInt32 wrap;
  int debug;

  debug = 0;

  for (i = 0; i < len; i += 1)
    {
      if (debug)
        {
          printf ("Data in = %d %d \n", i, data[i]);
        }
    }
  if (debug)
    {
      printf (" In generateEC - data length = %d \n", len);
      printf (" In generateEC - ECC length = %d \n", EClen);
    }

  /* get the coefficients */

  if (EClen < 128)
    {
      for (i = 0; i < EClen; i += 1)
        {
          coeff_reg[i] = mods[EClen][i];
          if (debug)
            {
              printf ("Set coeff_reg %d to %d \n", i, coeff_reg[i]);
            }

        }
    }
  else
    {
      switch (EClen)
        {
        case 128:
          for (i = 0; i < EClen; i += 1)
            coeff_reg[i] = mods128[i];
          break;
        case 256:
          for (i = 0; i < EClen; i += 1)
            coeff_reg[i] = mods256[i];
          break;
        case 512:
          for (i = 0; i < EClen; i += 1)
            coeff_reg[i] = mods512[i];
          break;
        default:
          fprintf (stderr, "bad EClen: %d\n", EClen);
          exit (1);
          break;
        };
    }

  /* initialize b regs */

  for (i = 0; i < EClen; i += 1)
    {
      base_reg[i] = 0;
    }

  /* Initialize data */
  for (i = len; i < len + EClen; i += 1)
    data[i] = 0;


  /* Load up with data */
  for (i = 0; i < len; ++i)
    {
      wrap = (base_reg[EClen - 1] + data[i]) % GPRIME;

      /* if (wrap) wrap =  GPRIME-wrap; */
      for (j = EClen - 1; j > 0; j = j - 1)
        {
          temp = (coeff_reg[EClen - 1 - j] * wrap) % GPRIME;
          temp = (GPRIME - temp) % GPRIME;
          base_reg[j] = (base_reg[j - 1] + temp) % GPRIME;
        }
      temp = (coeff_reg[EClen - 1] * wrap) % GPRIME;
      temp = (GPRIME - temp) % GPRIME;
      base_reg[0] = temp;
    }

  /* Read off the info */
  for (j = 0; j < EClen; j += 1)
    {
      if (debug)
        {
          printf ("Before compl base_reg %d = %d \n", j, base_reg[j]);
        }
      base_reg[j] = (GPRIME - base_reg[j]) % GPRIME;
    }

  for (j = 0; j < EClen; j += 1)
    {
      if (debug)
        {
          printf ("Adding ECC byte %d = %d \n", j, base_reg[EClen - 1 - j]);
        }

      /*      data[len+j] = base_reg[j]; */
      tint = base_reg[EClen - 1 - j];
      put_back (tint);

    }

}

// initialize table of 3**i for syndrome calculation
//

void
powers_init ()
{
  int ii;
  int power_of_3;
  int debug;

  debug = 0;
  power_of_3 = 1;
  Index_of[1] = GPRIME - 1;

  for (ii = 0; ii < GPRIME - 1; ii += 1)
    {
      powers_of_3[ii] = power_of_3;
      Alpha_to[ii] = power_of_3;
      if (power_of_3 < GPRIME)
        {
          log_of_3[power_of_3] = ii;
          if (ii != GPRIME - 1)
            {
              Index_of[power_of_3] = ii;
            }
        }
      else
        {
          printf ("Internal error:  powers of 3 calculation \n");
        }

      if (debug)
        {
          printf ("pow = %d  ii = %d \n", power_of_3, ii);
        }
      if (debug)
        {
          printf ("log = %d \n", ii);
        }
      power_of_3 = (power_of_3 * 3) % GPRIME;
    }
  Index_of[0] = GPRIME - 1;
  Alpha_to[GPRIME - 1] = 1;
  Index_of[GPRIME] = A0;
}

void
coeff_init_32 ()
{
  int ii;
  int jj;

  for (ii = 0; ii < 32; ii += 1)
    {
      for (jj = 0; jj < 32; jj += 1)
        {
          mods[ii][jj] = 0;
        }
    }

  mods[2][0] = 917;
  mods[2][1] = 27;
  mods[3][0] = 890;
  mods[3][1] = 351;
  mods[3][2] = 200;
  mods[4][0] = 809;
  mods[4][1] = 723;
  mods[4][2] = 568;
  mods[4][3] = 522;
  mods[5][0] = 566;
  mods[5][1] = 155;
  mods[5][2] = 460;
  mods[5][3] = 919;
  mods[5][4] = 427;
  mods[6][0] = 766;
  mods[6][1] = 17;
  mods[6][2] = 803;
  mods[6][3] = 19;
  mods[6][4] = 285;
  mods[6][5] = 861;
  mods[7][0] = 437;
  mods[7][1] = 691;
  mods[7][2] = 784;
  mods[7][3] = 597;
  mods[7][4] = 537;
  mods[7][5] = 925;
  mods[7][6] = 76;
  mods[8][0] = 379;
  mods[8][1] = 428;
  mods[8][2] = 653;
  mods[8][3] = 646;
  mods[8][4] = 284;
  mods[8][5] = 436;
  mods[8][6] = 308;
  mods[8][7] = 237;
  mods[9][0] = 205;
  mods[9][1] = 441;
  mods[9][2] = 501;
  mods[9][3] = 362;
  mods[9][4] = 289;
  mods[9][5] = 257;
  mods[9][6] = 622;
  mods[9][7] = 527;
  mods[9][8] = 567;
  mods[10][0] = 612;
  mods[10][1] = 266;
  mods[10][2] = 691;
  mods[10][3] = 818;
  mods[10][4] = 841;
  mods[10][5] = 826;
  mods[10][6] = 244;
  mods[10][7] = 64;
  mods[10][8] = 457;
  mods[10][9] = 377;
  mods[11][0] = 904;
  mods[11][1] = 602;
  mods[11][2] = 327;
  mods[11][3] = 68;
  mods[11][4] = 15;
  mods[11][5] = 213;
  mods[11][6] = 825;
  mods[11][7] = 708;
  mods[11][8] = 565;
  mods[11][9] = 45;
  mods[11][10] = 462;
  mods[12][0] = 851;
  mods[12][1] = 69;
  mods[12][2] = 7;
  mods[12][3] = 388;
  mods[12][4] = 127;
  mods[12][5] = 347;
  mods[12][6] = 684;
  mods[12][7] = 646;
  mods[12][8] = 201;
  mods[12][9] = 757;
  mods[12][10] = 864;
  mods[12][11] = 597;
  mods[13][0] = 692;
  mods[13][1] = 394;
  mods[13][2] = 184;
  mods[13][3] = 204;
  mods[13][4] = 678;
  mods[13][5] = 592;
  mods[13][6] = 322;
  mods[13][7] = 583;
  mods[13][8] = 606;
  mods[13][9] = 384;
  mods[13][10] = 342;
  mods[13][11] = 713;
  mods[13][12] = 764;
  mods[14][0] = 215;
  mods[14][1] = 105;
  mods[14][2] = 833;
  mods[14][3] = 691;
  mods[14][4] = 915;
  mods[14][5] = 478;
  mods[14][6] = 354;
  mods[14][7] = 274;
  mods[14][8] = 286;
  mods[14][9] = 241;
  mods[14][10] = 187;
  mods[14][11] = 154;
  mods[14][12] = 677;
  mods[14][13] = 669;
  mods[15][0] = 642;
  mods[15][1] = 868;
  mods[15][2] = 147;
  mods[15][3] = 575;
  mods[15][4] = 550;
  mods[15][5] = 74;
  mods[15][6] = 80;
  mods[15][7] = 5;
  mods[15][8] = 230;
  mods[15][9] = 664;
  mods[15][10] = 904;
  mods[15][11] = 109;
  mods[15][12] = 476;
  mods[15][13] = 829;
  mods[15][14] = 460;
  mods[16][0] = 65;
  mods[16][1] = 176;
  mods[16][2] = 42;
  mods[16][3] = 295;
  mods[16][4] = 428;
  mods[16][5] = 442;
  mods[16][6] = 116;
  mods[16][7] = 295;
  mods[16][8] = 132;
  mods[16][9] = 801;
  mods[16][10] = 524;
  mods[16][11] = 599;
  mods[16][12] = 755;
  mods[16][13] = 232;
  mods[16][14] = 562;
  mods[16][15] = 274;
  mods[17][0] = 192;
  mods[17][1] = 70;
  mods[17][2] = 98;
  mods[17][3] = 55;
  mods[17][4] = 733;
  mods[17][5] = 916;
  mods[17][6] = 510;
  mods[17][7] = 163;
  mods[17][8] = 437;
  mods[17][9] = 843;
  mods[17][10] = 61;
  mods[17][11] = 259;
  mods[17][12] = 650;
  mods[17][13] = 430;
  mods[17][14] = 298;
  mods[17][15] = 115;
  mods[17][16] = 425;
  mods[18][0] = 573;
  mods[18][1] = 760;
  mods[18][2] = 756;
  mods[18][3] = 233;
  mods[18][4] = 321;
  mods[18][5] = 560;
  mods[18][6] = 202;
  mods[18][7] = 312;
  mods[18][8] = 297;
  mods[18][9] = 120;
  mods[18][10] = 739;
  mods[18][11] = 275;
  mods[18][12] = 855;
  mods[18][13] = 37;
  mods[18][14] = 624;
  mods[18][15] = 315;
  mods[18][16] = 577;
  mods[18][17] = 279;
  mods[19][0] = 787;
  mods[19][1] = 754;
  mods[19][2] = 821;
  mods[19][3] = 371;
  mods[19][4] = 17;
  mods[19][5] = 508;
  mods[19][6] = 201;
  mods[19][7] = 806;
  mods[19][8] = 177;
  mods[19][9] = 506;
  mods[19][10] = 407;
  mods[19][11] = 491;
  mods[19][12] = 249;
  mods[19][13] = 923;
  mods[19][14] = 181;
  mods[19][15] = 75;
  mods[19][16] = 170;
  mods[19][17] = 200;
  mods[19][18] = 250;
  mods[20][0] = 500;
  mods[20][1] = 632;
  mods[20][2] = 880;
  mods[20][3] = 710;
  mods[20][4] = 375;
  mods[20][5] = 274;
  mods[20][6] = 258;
  mods[20][7] = 717;
  mods[20][8] = 176;
  mods[20][9] = 802;
  mods[20][10] = 109;
  mods[20][11] = 736;
  mods[20][12] = 540;
  mods[20][13] = 64;
  mods[20][14] = 45;
  mods[20][15] = 152;
  mods[20][16] = 12;
  mods[20][17] = 647;
  mods[20][18] = 448;
  mods[20][19] = 712;
  mods[21][0] = 568;
  mods[21][1] = 259;
  mods[21][2] = 193;
  mods[21][3] = 165;
  mods[21][4] = 347;
  mods[21][5] = 691;
  mods[21][6] = 310;
  mods[21][7] = 610;
  mods[21][8] = 624;
  mods[21][9] = 693;
  mods[21][10] = 763;
  mods[21][11] = 716;
  mods[21][12] = 422;
  mods[21][13] = 553;
  mods[21][14] = 681;
  mods[21][15] = 425;
  mods[21][16] = 129;
  mods[21][17] = 534;
  mods[21][18] = 781;
  mods[21][19] = 519;
  mods[21][20] = 108;
  mods[22][0] = 772;
  mods[22][1] = 6;
  mods[22][2] = 76;
  mods[22][3] = 519;
  mods[22][4] = 563;
  mods[22][5] = 875;
  mods[22][6] = 66;
  mods[22][7] = 678;
  mods[22][8] = 578;
  mods[22][9] = 716;
  mods[22][10] = 927;
  mods[22][11] = 296;
  mods[22][12] = 633;
  mods[22][13] = 244;
  mods[22][14] = 155;
  mods[22][15] = 928;
  mods[22][16] = 432;
  mods[22][17] = 838;
  mods[22][18] = 95;
  mods[22][19] = 55;
  mods[22][20] = 78;
  mods[22][21] = 665;
  mods[23][0] = 455;
  mods[23][1] = 538;
  mods[23][2] = 32;
  mods[23][3] = 581;
  mods[23][4] = 473;
  mods[23][5] = 772;
  mods[23][6] = 462;
  mods[23][7] = 194;
  mods[23][8] = 251;
  mods[23][9] = 503;
  mods[23][10] = 631;
  mods[23][11] = 1;
  mods[23][12] = 630;
  mods[23][13] = 247;
  mods[23][14] = 843;
  mods[23][15] = 101;
  mods[23][16] = 749;
  mods[23][17] = 457;
  mods[23][18] = 143;
  mods[23][19] = 597;
  mods[23][20] = 294;
  mods[23][21] = 93;
  mods[23][22] = 78;
  mods[24][0] = 433;
  mods[24][1] = 747;
  mods[24][2] = 273;
  mods[24][3] = 806;
  mods[24][4] = 697;
  mods[24][5] = 585;
  mods[24][6] = 200;
  mods[24][7] = 249;
  mods[24][8] = 628;
  mods[24][9] = 555;
  mods[24][10] = 713;
  mods[24][11] = 54;
  mods[24][12] = 608;
  mods[24][13] = 322;
  mods[24][14] = 54;
  mods[24][15] = 135;
  mods[24][16] = 385;
  mods[24][17] = 701;
  mods[24][18] = 308;
  mods[24][19] = 238;
  mods[24][20] = 166;
  mods[24][21] = 128;
  mods[24][22] = 819;
  mods[24][23] = 142;
  mods[25][0] = 367;
  mods[25][1] = 39;
  mods[25][2] = 208;
  mods[25][3] = 439;
  mods[25][4] = 454;
  mods[25][5] = 104;
  mods[25][6] = 608;
  mods[25][7] = 55;
  mods[25][8] = 916;
  mods[25][9] = 912;
  mods[25][10] = 314;
  mods[25][11] = 375;
  mods[25][12] = 760;
  mods[25][13] = 141;
  mods[25][14] = 169;
  mods[25][15] = 287;
  mods[25][16] = 765;
  mods[25][17] = 374;
  mods[25][18] = 492;
  mods[25][19] = 348;
  mods[25][20] = 251;
  mods[25][21] = 320;
  mods[25][22] = 732;
  mods[25][23] = 899;
  mods[25][24] = 847;
  mods[26][0] = 169;
  mods[26][1] = 764;
  mods[26][2] = 847;
  mods[26][3] = 131;
  mods[26][4] = 858;
  mods[26][5] = 325;
  mods[26][6] = 454;
  mods[26][7] = 441;
  mods[26][8] = 245;
  mods[26][9] = 699;
  mods[26][10] = 893;
  mods[26][11] = 446;
  mods[26][12] = 830;
  mods[26][13] = 159;
  mods[26][14] = 121;
  mods[26][15] = 269;
  mods[26][16] = 608;
  mods[26][17] = 331;
  mods[26][18] = 760;
  mods[26][19] = 477;
  mods[26][20] = 93;
  mods[26][21] = 788;
  mods[26][22] = 544;
  mods[26][23] = 887;
  mods[26][24] = 284;
  mods[26][25] = 443;
  mods[27][0] = 504;
  mods[27][1] = 710;
  mods[27][2] = 383;
  mods[27][3] = 531;
  mods[27][4] = 151;
  mods[27][5] = 694;
  mods[27][6] = 636;
  mods[27][7] = 175;
  mods[27][8] = 269;
  mods[27][9] = 93;
  mods[27][10] = 21;
  mods[27][11] = 463;
  mods[27][12] = 671;
  mods[27][13] = 438;
  mods[27][14] = 433;
  mods[27][15] = 857;
  mods[27][16] = 610;
  mods[27][17] = 560;
  mods[27][18] = 165;
  mods[27][19] = 531;
  mods[27][20] = 100;
  mods[27][21] = 357;
  mods[27][22] = 688;
  mods[27][23] = 114;
  mods[27][24] = 149;
  mods[27][25] = 825;
  mods[27][26] = 694;
  mods[28][0] = 580;
  mods[28][1] = 925;
  mods[28][2] = 461;
  mods[28][3] = 840;
  mods[28][4] = 560;
  mods[28][5] = 93;
  mods[28][6] = 427;
  mods[28][7] = 203;
  mods[28][8] = 563;
  mods[28][9] = 99;
  mods[28][10] = 586;
  mods[28][11] = 201;
  mods[28][12] = 557;
  mods[28][13] = 339;
  mods[28][14] = 277;
  mods[28][15] = 321;
  mods[28][16] = 712;
  mods[28][17] = 470;
  mods[28][18] = 920;
  mods[28][19] = 65;
  mods[28][20] = 509;
  mods[28][21] = 525;
  mods[28][22] = 879;
  mods[28][23] = 378;
  mods[28][24] = 452;
  mods[28][25] = 72;
  mods[28][26] = 222;
  mods[28][27] = 720;
  mods[29][0] = 808;
  mods[29][1] = 318;
  mods[29][2] = 478;
  mods[29][3] = 42;
  mods[29][4] = 706;
  mods[29][5] = 500;
  mods[29][6] = 264;
  mods[29][7] = 14;
  mods[29][8] = 397;
  mods[29][9] = 261;
  mods[29][10] = 862;
  mods[29][11] = 33;
  mods[29][12] = 864;
  mods[29][13] = 62;
  mods[29][14] = 462;
  mods[29][15] = 305;
  mods[29][16] = 509;
  mods[29][17] = 231;
  mods[29][18] = 316;
  mods[29][19] = 800;
  mods[29][20] = 465;
  mods[29][21] = 452;
  mods[29][22] = 738;
  mods[29][23] = 126;
  mods[29][24] = 239;
  mods[29][25] = 9;
  mods[29][26] = 845;
  mods[29][27] = 241;
  mods[29][28] = 656;
  mods[30][0] = 563;
  mods[30][1] = 235;
  mods[30][2] = 604;
  mods[30][3] = 915;
  mods[30][4] = 635;
  mods[30][5] = 324;
  mods[30][6] = 392;
  mods[30][7] = 364;
  mods[30][8] = 683;
  mods[30][9] = 541;
  mods[30][10] = 89;
  mods[30][11] = 655;
  mods[30][12] = 211;
  mods[30][13] = 194;
  mods[30][14] = 136;
  mods[30][15] = 453;
  mods[30][16] = 104;
  mods[30][17] = 12;
  mods[30][18] = 390;
  mods[30][19] = 487;
  mods[30][20] = 484;
  mods[30][21] = 794;
  mods[30][22] = 549;
  mods[30][23] = 471;
  mods[30][24] = 26;
  mods[30][25] = 910;
  mods[30][26] = 498;
  mods[30][27] = 383;
  mods[30][28] = 138;
  mods[30][29] = 926;
  mods[31][0] = 757;
  mods[31][1] = 764;
  mods[31][2] = 673;
  mods[31][3] = 108;
  mods[31][4] = 706;
  mods[31][5] = 886;
  mods[31][6] = 76;
  mods[31][7] = 234;
  mods[31][8] = 695;
  mods[31][9] = 196;
  mods[31][10] = 66;
  mods[31][11] = 270;
  mods[31][12] = 8;
  mods[31][13] = 252;
  mods[31][14] = 612;
  mods[31][15] = 825;
  mods[31][16] = 660;
  mods[31][17] = 679;
  mods[31][18] = 860;
  mods[31][19] = 898;
  mods[31][20] = 204;
  mods[31][21] = 861;
  mods[31][22] = 371;
  mods[31][23] = 142;
  mods[31][24] = 358;
  mods[31][25] = 380;
  mods[31][26] = 528;
  mods[31][27] = 379;
  mods[31][28] = 120;
  mods[31][29] = 757;
  mods[31][30] = 347;
  mods[32][0] = 410;
  mods[32][1] = 63;
  mods[32][2] = 330;
  mods[32][3] = 685;
  mods[32][4] = 390;
  mods[32][5] = 231;
  mods[32][6] = 133;
  mods[32][7] = 803;
  mods[32][8] = 320;
  mods[32][9] = 571;
  mods[32][10] = 800;
  mods[32][11] = 593;
  mods[32][12] = 147;
  mods[32][13] = 263;
  mods[32][14] = 494;
  mods[32][15] = 273;
  mods[32][16] = 517;
  mods[32][17] = 193;
  mods[32][18] = 284;
  mods[32][19] = 687;
  mods[32][20] = 742;
  mods[32][21] = 677;
  mods[32][22] = 742;
  mods[32][23] = 536;
  mods[32][24] = 321;
  mods[32][25] = 640;
  mods[32][26] = 586;
  mods[32][27] = 176;
  mods[32][28] = 525;
  mods[32][29] = 922;
  mods[32][30] = 575;
  mods[32][31] = 361;
  mods[36][0] = 575;
  mods[36][1] = 871;
  mods[36][2] = 311;
  mods[36][3] = 454;
  mods[36][4] = 504;
  mods[36][5] = 870;
  mods[36][6] = 199;
  mods[36][7] = 768;
  mods[36][8] = 634;
  mods[36][9] = 362;
  mods[36][10] = 548;
  mods[36][11] = 855;
  mods[36][12] = 529;
  mods[36][13] = 384;
  mods[36][14] = 830;
  mods[36][15] = 923;
  mods[36][16] = 222;
  mods[36][17] = 85;
  mods[36][18] = 841;
  mods[36][19] = 59;
  mods[36][20] = 518;
  mods[36][21] = 590;
  mods[36][22] = 358;
  mods[36][23] = 110;
  mods[36][24] = 695;
  mods[36][25] = 864;
  mods[36][26] = 699;
  mods[36][27] = 581;
  mods[36][28] = 642;
  mods[36][29] = 175;
  mods[36][30] = 836;
  mods[36][31] = 855;
  mods[36][32] = 709;
  mods[36][33] = 274;
  mods[36][34] = 686;
  mods[36][35] = 244;
  mods[40][0] = 5;
  mods[40][1] = 10;
  mods[40][2] = 156;
  mods[40][3] = 729;
  mods[40][4] = 684;
  mods[40][5] = 324;
  mods[40][6] = 60;
  mods[40][7] = 264;
  mods[40][8] = 99;
  mods[40][9] = 261;
  mods[40][10] = 89;
  mods[40][11] = 460;
  mods[40][12] = 742;
  mods[40][13] = 208;
  mods[40][14] = 699;
  mods[40][15] = 670;
  mods[40][16] = 512;
  mods[40][17] = 404;
  mods[40][18] = 726;
  mods[40][19] = 389;
  mods[40][20] = 492;
  mods[40][21] = 287;
  mods[40][22] = 894;
  mods[40][23] = 571;
  mods[40][24] = 41;
  mods[40][25] = 203;
  mods[40][26] = 353;
  mods[40][27] = 256;
  mods[40][28] = 243;
  mods[40][29] = 784;
  mods[40][30] = 385;
  mods[40][31] = 555;
  mods[40][32] = 595;
  mods[40][33] = 734;
  mods[40][34] = 714;
  mods[40][35] = 565;
  mods[40][36] = 205;
  mods[40][37] = 706;
  mods[40][38] = 316;
  mods[40][39] = 115;
  mods[44][0] = 285;
  mods[44][1] = 82;
  mods[44][2] = 730;
  mods[44][3] = 339;
  mods[44][4] = 436;
  mods[44][5] = 572;
  mods[44][6] = 271;
  mods[44][7] = 103;
  mods[44][8] = 758;
  mods[44][9] = 231;
  mods[44][10] = 560;
  mods[44][11] = 31;
  mods[44][12] = 213;
  mods[44][13] = 272;
  mods[44][14] = 267;
  mods[44][15] = 569;
  mods[44][16] = 773;
  mods[44][17] = 3;
  mods[44][18] = 21;
  mods[44][19] = 446;
  mods[44][20] = 706;
  mods[44][21] = 413;
  mods[44][22] = 97;
  mods[44][23] = 376;
  mods[44][24] = 60;
  mods[44][25] = 714;
  mods[44][26] = 436;
  mods[44][27] = 417;
  mods[44][28] = 405;
  mods[44][29] = 632;
  mods[44][30] = 25;
  mods[44][31] = 109;
  mods[44][32] = 876;
  mods[44][33] = 470;
  mods[44][34] = 915;
  mods[44][35] = 157;
  mods[44][36] = 840;
  mods[44][37] = 764;
  mods[44][38] = 64;
  mods[44][39] = 678;
  mods[44][40] = 848;
  mods[44][41] = 659;
  mods[44][42] = 36;
  mods[44][43] = 476;
  mods[48][0] = 669;
  mods[48][1] = 912;
  mods[48][2] = 896;
  mods[48][3] = 252;
  mods[48][4] = 338;
  mods[48][5] = 162;
  mods[48][6] = 414;
  mods[48][7] = 632;
  mods[48][8] = 626;
  mods[48][9] = 252;
  mods[48][10] = 869;
  mods[48][11] = 185;
  mods[48][12] = 444;
  mods[48][13] = 82;
  mods[48][14] = 920;
  mods[48][15] = 783;
  mods[48][16] = 565;
  mods[48][17] = 875;
  mods[48][18] = 126;
  mods[48][19] = 877;
  mods[48][20] = 524;
  mods[48][21] = 603;
  mods[48][22] = 189;
  mods[48][23] = 136;
  mods[48][24] = 373;
  mods[48][25] = 540;
  mods[48][26] = 649;
  mods[48][27] = 271;
  mods[48][28] = 836;
  mods[48][29] = 540;
  mods[48][30] = 199;
  mods[48][31] = 323;
  mods[48][32] = 888;
  mods[48][33] = 486;
  mods[48][34] = 92;
  mods[48][35] = 849;
  mods[48][36] = 162;
  mods[48][37] = 701;
  mods[48][38] = 178;
  mods[48][39] = 926;
  mods[48][40] = 498;
  mods[48][41] = 575;
  mods[48][42] = 765;
  mods[48][43] = 422;
  mods[48][44] = 450;
  mods[48][45] = 302;
  mods[48][46] = 354;
  mods[48][47] = 710;
  mods[52][0] = 187;
  mods[52][1] = 57;
  mods[52][2] = 15;
  mods[52][3] = 317;
  mods[52][4] = 835;
  mods[52][5] = 593;
  mods[52][6] = 8;
  mods[52][7] = 158;
  mods[52][8] = 95;
  mods[52][9] = 145;
  mods[52][10] = 37;
  mods[52][11] = 659;
  mods[52][12] = 576;
  mods[52][13] = 386;
  mods[52][14] = 884;
  mods[52][15] = 913;
  mods[52][16] = 495;
  mods[52][17] = 869;
  mods[52][18] = 908;
  mods[52][19] = 296;
  mods[52][20] = 437;
  mods[52][21] = 215;
  mods[52][22] = 33;
  mods[52][23] = 883;
  mods[52][24] = 877;
  mods[52][25] = 477;
  mods[52][26] = 712;
  mods[52][27] = 578;
  mods[52][28] = 349;
  mods[52][29] = 13;
  mods[52][30] = 174;
  mods[52][31] = 839;
  mods[52][32] = 914;
  mods[52][33] = 107;
  mods[52][34] = 260;
  mods[52][35] = 40;
  mods[52][36] = 532;
  mods[52][37] = 210;
  mods[52][38] = 395;
  mods[52][39] = 905;
  mods[52][40] = 163;
  mods[52][41] = 785;
  mods[52][42] = 693;
  mods[52][43] = 627;
  mods[52][44] = 393;
  mods[52][45] = 687;
  mods[52][46] = 112;
  mods[52][47] = 481;
  mods[52][48] = 717;
  mods[52][49] = 297;
  mods[52][50] = 37;
  mods[52][51] = 483;
  mods[56][0] = 163;
  mods[56][1] = 726;
  mods[56][2] = 626;
  mods[56][3] = 653;
  mods[56][4] = 414;
  mods[56][5] = 537;
  mods[56][6] = 467;
  mods[56][7] = 579;
  mods[56][8] = 729;
  mods[56][9] = 396;
  mods[56][10] = 142;
  mods[56][11] = 598;
  mods[56][12] = 860;
  mods[56][13] = 774;
  mods[56][14] = 518;
  mods[56][15] = 461;
  mods[56][16] = 136;
  mods[56][17] = 687;
  mods[56][18] = 827;
  mods[56][19] = 614;
  mods[56][20] = 841;
  mods[56][21] = 468;
  mods[56][22] = 207;
  mods[56][23] = 481;
  mods[56][24] = 649;
  mods[56][25] = 910;
  mods[56][26] = 497;
  mods[56][27] = 686;
  mods[56][28] = 186;
  mods[56][29] = 235;
  mods[56][30] = 845;
  mods[56][31] = 863;
  mods[56][32] = 821;
  mods[56][33] = 711;
  mods[56][34] = 663;
  mods[56][35] = 534;
  mods[56][36] = 393;
  mods[56][37] = 756;
  mods[56][38] = 467;
  mods[56][39] = 224;
  mods[56][40] = 442;
  mods[56][41] = 520;
  mods[56][42] = 210;
  mods[56][43] = 732;
  mods[56][44] = 864;
  mods[56][45] = 729;
  mods[56][46] = 433;
  mods[56][47] = 735;
  mods[56][48] = 70;
  mods[56][49] = 184;
  mods[56][50] = 278;
  mods[56][51] = 97;
  mods[56][52] = 492;
  mods[56][53] = 17;
  mods[56][54] = 2;
  mods[56][55] = 338;
  mods[60][0] = 77;
  mods[60][1] = 611;
  mods[60][2] = 467;
  mods[60][3] = 704;
  mods[60][4] = 555;
  mods[60][5] = 579;
  mods[60][6] = 802;
  mods[60][7] = 773;
  mods[60][8] = 303;
  mods[60][9] = 518;
  mods[60][10] = 560;
  mods[60][11] = 196;
  mods[60][12] = 314;
  mods[60][13] = 102;
  mods[60][14] = 5;
  mods[60][15] = 845;
  mods[60][16] = 248;
  mods[60][17] = 125;
  mods[60][18] = 836;
  mods[60][19] = 923;
  mods[60][20] = 88;
  mods[60][21] = 630;
  mods[60][22] = 886;
  mods[60][23] = 619;
  mods[60][24] = 37;
  mods[60][25] = 141;
  mods[60][26] = 409;
  mods[60][27] = 229;
  mods[60][28] = 77;
  mods[60][29] = 658;
  mods[60][30] = 450;
  mods[60][31] = 449;
  mods[60][32] = 93;
  mods[60][33] = 651;
  mods[60][34] = 276;
  mods[60][35] = 501;
  mods[60][36] = 166;
  mods[60][37] = 75;
  mods[60][38] = 630;
  mods[60][39] = 701;
  mods[60][40] = 388;
  mods[60][41] = 72;
  mods[60][42] = 830;
  mods[60][43] = 166;
  mods[60][44] = 187;
  mods[60][45] = 131;
  mods[60][46] = 711;
  mods[60][47] = 577;
  mods[60][48] = 834;
  mods[60][49] = 147;
  mods[60][50] = 361;
  mods[60][51] = 517;
  mods[60][52] = 76;
  mods[60][53] = 581;
  mods[60][54] = 45;
  mods[60][55] = 495;
  mods[60][56] = 366;
  mods[60][57] = 278;
  mods[60][58] = 781;
  mods[60][59] = 61;
  mods[64][0] = 543;
  mods[64][1] = 264;
  mods[64][2] = 623;
  mods[64][3] = 843;
  mods[64][4] = 381;
  mods[64][5] = 4;
  mods[64][6] = 629;
  mods[64][7] = 840;
  mods[64][8] = 771;
  mods[64][9] = 280;
  mods[64][10] = 97;
  mods[64][11] = 404;
  mods[64][12] = 83;
  mods[64][13] = 717;
  mods[64][14] = 733;
  mods[64][15] = 648;
  mods[64][16] = 502;
  mods[64][17] = 488;
  mods[64][18] = 201;
  mods[64][19] = 651;
  mods[64][20] = 158;
  mods[64][21] = 605;
  mods[64][22] = 352;
  mods[64][23] = 517;
  mods[64][24] = 535;
  mods[64][25] = 225;
  mods[64][26] = 594;
  mods[64][27] = 460;
  mods[64][28] = 31;
  mods[64][29] = 519;
  mods[64][30] = 35;
  mods[64][31] = 440;
  mods[64][32] = 184;
  mods[64][33] = 283;
  mods[64][34] = 762;
  mods[64][35] = 672;
  mods[64][36] = 400;
  mods[64][37] = 511;
  mods[64][38] = 376;
  mods[64][39] = 543;
  mods[64][40] = 822;
  mods[64][41] = 858;
  mods[64][42] = 609;
  mods[64][43] = 430;
  mods[64][44] = 172;
  mods[64][45] = 462;
  mods[64][46] = 476;
  mods[64][47] = 723;
  mods[64][48] = 612;
  mods[64][49] = 381;
  mods[64][50] = 877;
  mods[64][51] = 733;
  mods[64][52] = 505;
  mods[64][53] = 107;
  mods[64][54] = 287;
  mods[64][55] = 610;
  mods[64][56] = 106;
  mods[64][57] = 453;
  mods[64][58] = 771;
  mods[64][59] = 862;
  mods[64][60] = 93;
  mods[64][61] = 6;
  mods[64][62] = 422;
  mods[64][63] = 539;
}                               /* end */

int
modbase (int x)
{
  return (x % (GPRIME - 1));

}

void
init_rs ()
{
  /* what you need to initialize to do the reed solomon/decode */
}

/*
 * Performs ERRORS+ERASURES decoding of RS codes. If decoding is successful,
 * writes the codeword into data[] itself. Otherwise data[] is unaltered.
 *
 * Return number of symbols corrected, or -1 if codeword is illegal
 * or uncorrectable. If eras_pos is non-null, the detected error locations
 * are written back. NOTE! This array must be at least NN-KK elements long.
 * 
 * First "no_eras" erasures are declared by the calling program. Then, the
 * maximum # of errors correctable is t_after_eras = floor((NN-KK-no_eras)/2).
 * If the number of channel errors is not greater than "t_after_eras" the
 * transmitted codeword will be recovered. Details of algorithm can be found
 * in R. Blahut's "Theory ... of Error-Correcting Codes".

 * Warning: the eras_pos[] array must not contain duplicate entries; decoder failure
 * will result. The decoder *could* check for this condition, but it would involve
 * extra time on every decoding operation.
 */

int
eras_dec_rs (int data[NN], int eras_pos[NN - KK], int no_eras, int data_len,
             int synd_len)
{
  int deg_lambda, el, deg_omega;
  int i, j, r, k;
  int u, q, tmp, num1, num2, den, discr_r;
  int lambda[2048 + 1], s[2048 + 1]; /* Err+Eras Locator poly
                                        * and syndrome poly */
  int b[2048 + 1], t[2048 + 1], omega[2048 + 1];
  int root[2048], reg[2048 + 1], loc[2048];
  int syn_error, count;
  int ci;
  int mismatch;
  int error_val;
  int fix_loc;
  int debug;

  debug = 0;

  init_rs ();
#define DEBUG 2

#if DEBUG >= 1 && MM != 8

  /* Check for illegal input values */
  for (i = 0; i < data_len; i++)
    if (data[i] > GPRIME)
      return -1;
#endif


  //  if ( global == 1)
  // {
  //    debug = 1;
  //  }

  /* form the syndromes; i.e., evaluate data(x) at roots of g(x)
   * namely @**(1+i)*PRIM, i = 0, ... ,(NN-KK-1)
   */
  for (i = 1; i <= synd_len; i++)
    {
      s[i] = data[data_len];
    }

  for (j = 1; j <= data_len; j++)
    {
      if (data[data_len - j] == 0)
        continue;
      tmp = Index_of[data[data_len - j]];

      for (i = 1; i <= synd_len; i++)
        {
          s[i] = (s[i] + Alpha_to[modbase (tmp + (i) * j)]) % GPRIME;

        }
    }

  mismatch = FALSE;
  for (j = 0; j < synd_len; j += 1)
    {
      if (s[j + 1] != synd_array[j])
        {
          printf ("Syndrome mismatch j = %d s[] = %d synd[] = %d \n",
                  j, s[j + 1], synd_array[j]);
          mismatch = TRUE;

        }
    }

  if (mismatch == FALSE)
    {
      if (debug)
        {
          printf ("Correct syndromes \n");
        }
    }

  /* Convert syndromes to index form, checking for nonzero condition */
  syn_error = 0;
  for (i = 1; i <= synd_len; i++)
    {
      syn_error |= s[i];
      if (debug)
        {
          printf ("Raw syndrome = %d i = %d \n", s[i], i);
        }
      s[i] = Index_of[s[i]];

    }

  if (!syn_error)
    {
      /* if syndrome is zero, data[] is a codeword and there are no
       * errors to correct. So return data[] unmodified
       */
      count = 0;
      printf ("No errors \n");
      goto finish;
    }

  for (ci = synd_len - 1; ci >= 0; ci--)
    lambda[ci + 1] = 0;

  lambda[0] = 1;

  if (no_eras > 0)
    {
      /* Init lambda to be the erasure locator polynomial */
      lambda[1] = Alpha_to[modbase (PRIM * eras_pos[0])];
      for (i = 1; i < no_eras; i++)
        {
          u = modbase (PRIM * eras_pos[i]);
          for (j = i + 1; j > 0; j--)
            {
              tmp = Index_of[lambda[j - 1]];
              if (tmp != A0)
                lambda[j] =
                  (lambda[j] + Alpha_to[modbase (u + tmp)]) % GPRIME;
            }
        }
#if DEBUG >= 1
      /* Test code that verifies the erasure locator polynomial just constructed
         Needed only for decoder debugging. */

      /* find roots of the erasure location polynomial */
      for (i = 1; i <= no_eras; i++)
        reg[i] = Index_of[lambda[i]];
      count = 0;                // usg NN = GPRIME-1?

      for (i = 1, k = data_len - Ldec; i <= data_len + synd_len;
           i++, k = modbase (data_len + k - Ldec))
        {
          q = 1;
          for (j = 1; j <= no_eras; j++)
            if (reg[j] != A0)
              {
                reg[j] = modbase (reg[j] + j);
                q = (q + Alpha_to[reg[j]]) % GPRIME;
              }
          if (q != 0)
            continue;
          /* store root and error location number indices */
          root[count] = i;
          loc[count] = k;
          count++;
        }
      if (count != no_eras)
        {
          // printf("\n lambda(x) is WRONG\n");
          // count = -1;
          //  goto finish;
        }
#if DEBUG >= 2
      printf
        ("\n Erasure positions as determined by roots of Eras Loc Poly:\n");
      for (i = 0; i < count; i++)
        printf ("loc  = %d ", loc[i]);
      printf ("\n");
#endif
#endif
    }
  for (i = 0; i < synd_len + 1; i++)
    b[i] = Index_of[lambda[i]];

  /*
   * Begin Berlekamp-Massey algorithm to determine error+erasure
   * locator polynomial
   */
  r = no_eras;
  el = no_eras;
  while (++r <= synd_len)
    {                           /* r is the step number */
      /* Compute discrepancy at the r-th step in poly-form */
      discr_r = 0;
      for (i = 0; i < r; i++)
        {
          if ((lambda[i] != 0) && (s[r - i] != A0))
            {
              if (debug)
                {
                  printf ("do add Index_of[lambda[]] = %d \n",
                          Index_of[lambda[i]]);
                }
              if (i % 2 == 1)
                {
                  discr_r = (discr_r +
                             Alpha_to[modbase
                                      ((Index_of[lambda[i]] +
                                        s[r - i]))]) % GPRIME;
                }
              else
                {
                  discr_r = (discr_r + GPRIME -
                             Alpha_to[modbase
                                      ((Index_of[lambda[i]] +
                                        s[r - i]))]) % GPRIME;
                }
              if (debug)
                {
                  printf
                    ("In loop - discr = %d i = %d r = %d lambda[i] = %d s[r-i] = %d \n",
                     discr_r, i, r, lambda[i], s[r - i]);
                }
            }
        }
      if (debug)
        {
          printf ("r = %d Discrepency = %d \n", r, discr_r);
        }

      discr_r = Index_of[discr_r]; /* Index form */

      if (discr_r == A0)
        {
          /* 2 lines below: B(x) <-- x*B(x) */
          //  COPYDOWN(&b[1],b,synd_len);
          //
          if (debug)
            {
              printf ("Discrepency = A0 \n");
            }
          for (ci = synd_len - 1; ci >= 0; ci--)
            {
              b[ci + 1] = b[ci];
            }
          b[0] = A0;
        }
      else
        {
          /* 7 lines below: T(x) <-- lambda(x) - discr_r*x*b(x) */
          /*  the T(x) will become the next lambda */

          t[0] = lambda[0];
          for (i = 0; i < synd_len; i++)
            {
              if (debug)
                {
                  printf ("i = %d b[i] = %d \n", i, b[i]);
                }
              if (b[i] != A0)
                {

                  t[i + 1] = (lambda[i + 1] +
                              Alpha_to[modbase (discr_r + b[i])]) % GPRIME;

                  if (debug)
                    {
                      printf
                        ("New t[i+1] = %d lambda[i+1] = %d b[i] = %d i = %d discr_r = %d\n",
                         t[i + 1], lambda[i + 1], b[i], i, discr_r);
                    }
                }
              else
                {
                  t[i + 1] = lambda[i + 1];
                }
              if (debug)
                {
                  printf ("i = %d t[i+1] = %d lambda[i+1] = %d \n", i,
                          t[i + 1], lambda[i + 1]);
                }
            }
          el = 0;
          if (2 * el <= r + no_eras - 1)
            {
              if (debug)
                {
                  printf ("Reached the el stuff, inv  el = %d r = %d \n", el,
                          r);
                }
              el = r + no_eras - el;
              /*
               * 2 lines below: B(x) <-- inv(discr_r) *
               * lambda(x)
               */
              for (i = 0; i <= synd_len; i++)
                {

                  if (lambda[i] == 0)
                    {
                      b[i] = A0;
                    }
                  else
                    {

                      b[i] =
                        modbase (Index_of[lambda[i]] - discr_r + GPRIME - 1);
                      if (debug)
                        {
                          printf ("Inverting le  b[i] = %d i = %d \n", b[i],
                                  i);
                        }
                    }

                }

            }
          else
            {
              if (debug)
                {
                  printf ("Reached the el stuff, x mul,   el = %d r = %d \n",
                          el, r);
                }
              /* 2 lines below: B(x) <-- x*B(x) */
              //      COPYDOWN(&b[1],b,synd_len);
              for (ci = synd_len - 1; ci >= 0; ci--)
                {
                  b[ci + 1] = b[ci];
                }
              b[0] = A0;
            }
          //      COPY(lambda,t,synd_len+1);

          for (ci = synd_len + 1 - 1; ci >= 0; ci--)
            {
              lambda[ci] = t[ci];
              if (debug)
                {
                  printf ("ci = %d Lambda = %d \n", ci, t[ci]);
                }
            }
        }
    }

  /* Convert lambda to index form and compute deg(lambda(x)) */
  deg_lambda = 0;
  for (i = 0; i < synd_len + 1; i++)
    {


      lambda[i] = Index_of[lambda[i]];

      if (lambda[i] != A0)
        {
          deg_lambda = i;
        }

      if (debug)
        {
          printf ("Lambda in index form = %d \n", lambda[i]);
        }

    }

  if (debug)
    {
      printf ("Determination of deg_lambda = %d \n", deg_lambda);
    }

  /*
   * Find roots of the error+erasure locator polynomial by Chien
   * Search
   */

  for (ci = synd_len - 1; ci >= 0; ci--)
    reg[ci + 1] = lambda[ci + 1];

  count = 0;                    /* Number of roots of lambda(x) */
  for (i = 1, k = data_len - 1; i <= GPRIME; i++)

    {
      q = 1;
      if (debug)
        {
          printf (" Reg[j] = %d q = %d i = %d \n", reg[j], q, i);
        }
      for (j = deg_lambda; j > 0; j--)
        {

          if (reg[j] != A0)
            {
              if (debug)
                {
                  printf ("loop Reg[j] pre = %d \n", reg[j]);
                }
              reg[j] = modbase (reg[j] + j);
              //      q = modbase( q +  Alpha_to[reg[j]]);
              if (deg_lambda != 1)
                {
                  if (j % 2 == 0)
                    {
                      q = (q + Alpha_to[reg[j]]) % GPRIME;
                    }
                  else
                    {
                      q = (q + GPRIME - Alpha_to[reg[j]]) % GPRIME;
                    }
                }
              else
                {
                  q = Alpha_to[reg[j]] % GPRIME;
                  if (q == 1)
                    {
                      q = q - 1;
                    }
                }
              if (debug)
                {
                  printf ("loop Reg[j] = %d q = %d i = %d j = %d %d = k\n",
                          reg[j], q, i, j, k);
                }
            }
        }

      if (q == 0)
        {
          /* store root (index-form) and error location number */
          root[count] = i;

          loc[count] = GPRIME - 1 - i;
          if (count < synd_len)
            {
              count += 1;
            }
          else
            {
              printf ("Error : Error count too big = %d \n", count);
            }

          if (debug)
            {
              printf ("root  = %d loc = %d \n", i, k);
            }

        }
      if (k == 0)
        {
          k = data_len - 1;
        }
      else
        {
          k -= 1;
        }

      /* If we've already found max possible roots,
       * abort the search to save time
       */

      if (count == deg_lambda)
        {
          break;
        }
    }
  if (deg_lambda != count)
    {
      /*
       * deg(lambda) unequal to number of roots => uncorrectable
       * error detected
       */

      printf ("Uncorrectable error: root count = %d deg lambda = %d \n",
              count, deg_lambda);
      count = -1;
      goto finish;
    }
  /*
   * Compute err+eras evaluator poly omega(x) = s(x)*lambda(x) (modulo
   * x**(synd_len)). in index form. Also find deg(omega).
   */
  deg_omega = 0;
  for (i = 0; i < synd_len; i++)
    {
      tmp = 0;
      j = (deg_lambda < i) ? deg_lambda : i;
      if (debug)
        {
          printf ("j = %d deg_lambda = %d lambda[j] = %d \n",
                  j, deg_lambda, lambda[j]);
        }
      for (; j >= 0; j--)
        {
          if ((s[i + 1 - j] != A0) && (lambda[j] != A0))
            {
              if (j % 2 == 1)
                {

                  tmp = (tmp + GPRIME -
                         Alpha_to[modbase (s[i + 1 - j] + lambda[j])]) %
                    GPRIME;
                }
              else
                {

                  tmp =
                    (tmp +
                     Alpha_to[modbase (s[i + 1 - j] + lambda[j])]) % GPRIME;
                }
              if (debug)
                {
                  printf
                    ("In tmp loop  tmp = %d i = %d j = %d s[i+1-j] = %d lambda[j] = %d \n",
                     tmp, i, j, s[i + 1 - j], lambda[j]);
                }
            }
        }
      if (tmp != 0)
        deg_omega = i;
      omega[i] = Index_of[tmp];
      if (debug)
        {
          printf ("Omega [i] = %d i = %d \n", omega[i], i);
        }

    }
  omega[synd_len] = A0;
  if (debug)
    {
      printf ("Degree of omega = %d \n", deg_omega);
    }

  /*
   * Compute error values in poly-form. num1 = omega(inv(X(l))), num2 =
   * inv(X(l))**(B0-1) and den = lambda_pr(inv(X(l))) all in poly-form
   */
  for (j = count - 1; j >= 0; j--)
    {
      num1 = 0;
      for (i = deg_omega; i >= 0; i--)
        {
          if (omega[i] != A0)
            {
              num1 =
                (num1 +
                 Alpha_to[modbase (omega[i] + ((i + 1) * root[j]))]) % GPRIME;
              if (debug)
                {
                  printf ("Num1 = %d i = %d omega[i] = %d root[j] = %d \n",
                          num1, i, omega[i], root[j]);
                }
            }
        }

      num2 = 1;
      den = 0;

      // denominator if product of all (1 - Bj Bk) for k != j
      // if count = 1, then den = 1

      den = 1;
      for (k = 0; k < count; k += 1)
        {
          if (k != j)
            {
              tmp =
                (1 + GPRIME -
                 Alpha_to[modbase (GPRIME - 1 - root[k] + root[j])]) % GPRIME;

              den = Alpha_to[modbase (Index_of[den] + Index_of[tmp])];
            }
        }

      if (debug)
        {
          printf ("den = %d \n", den);
        }

      if (den == 0)
        {
#if DEBUG >= 1
          printf ("\n ERROR: denominator = 0\n");
#endif
          /* Convert to dual- basis */
          count = -1;
          goto finish;
        }

      if (debug)
        {
          printf ("Index num1 = %d Index num2 = %d Index of den = %d \n",
                  Index_of[num1], Index_of[num2], Index_of[den]);
        }

      error_val = Alpha_to[modbase (Index_of[num1] + Index_of[num2] +
                                    GPRIME - 1 - Index_of[den])] % GPRIME;

      if (debug)
        {
          printf ("error_val = %d \n", error_val);
        }

      /* Apply error to data */
      if (num1 != 0)
        {
          if (loc[j] < data_len + 1)
            {
              fix_loc = data_len - loc[j];
              if (debug)
                {
                  printf ("Fix loc = %d \n", fix_loc);
                }
              if (fix_loc < data_len + 1)
                {
                  data[fix_loc] =
                    (data[fix_loc] + GPRIME - error_val) % GPRIME;
                }
            }
        }
    }
finish:
  if (debug)
    {
      printf ("At FINISH \n");
    }
  if (eras_pos != NULL)
    {
      for (i = 0; i < count; i++)
        {
          if (eras_pos != NULL)
            eras_pos[i] = loc[i];
        }
    }
  return count;
}

// calculate syndromes for the encoded message + correction
// this is to check to see that the error correction codes are correct
//

void
syndromes (int message_len, int synd_count)
{

  int i, j;
  int power_val;
  int synd_val;
  int debug;
  debug = 0;

  for (i = 0; i < synd_count; i += 1)
    {

      synd_array[i] = 0;
    }


  for (i = 0; i < synd_count; i += 1)
    {

      power_val = powers_of_3[i + 1];

      if (debug)
        {
          printf ("Power val = %d \n", power_val);
        }
      synd_val = 0;
      for (j = 0; j < message_len; j += 1)
        {
          synd_val = (data[j] + synd_val) % GPRIME;
          synd_val = (power_val * synd_val) % GPRIME;
          if (debug)
            {
              printf ("Sval = %d \n", synd_val);
            }
        }

      synd_array[i] = synd_val;

      if (debug)
        {
          printf ("i = %d syndrome = %d \n", i, synd_array[i]);
        }

    }


}

void
put_back_len (int inval)
{

  int debug;
  debug = 0;


  if (debug)
    {
      printf ("Put back len, Code = %d \n", inval);
    }


  codes[0] = inval;
  data[0] = inval;


}                               /* put back len */

void
numeric_compact (char *instring)
{



  int b900_result[16];

  int save_val[16];

  int ii, jj, kk;
  int carryin;
  int slen;
  int leading;
  int mm;
  int this_digit;
  int debug;


  debug = 0;

  if (debug)
    {
     printf ("In numeric_compact - instring = %s \n", instring);
    }

  /* calculate the base 900 values */

  if (digit_table_valid == FALSE)
    {
      digit_vals[0][0] = 0;
      digit_vals[0][1] = 0;
      digit_vals[0][2] = 0;
      digit_vals[0][3] = 0;
      digit_vals[0][4] = 0;

      digit_vals[0][5] = 0;
      digit_vals[0][6] = 0;
      digit_vals[0][7] = 0;
      digit_vals[0][8] = 0;
      digit_vals[0][9] = 0;

      digit_vals[0][10] = 0;
      digit_vals[0][11] = 0;
      digit_vals[0][12] = 0;
      digit_vals[0][13] = 0;
      digit_vals[0][14] = 0;
      digit_vals[0][15] = 1;

      for (jj = 1; jj < 46; jj += 1)
        {

          for (ii = 15; ii > -1; ii -= 1) /* multiply base 900 gits by 10 */
            {
              digit_vals[jj][ii] = (digit_vals[jj - 1][ii]) * 10;
            }

          for (ii = 15; ii > -1; ii -= 1)
            {
              save_val[ii] = digit_vals[jj][ii];
            }

          for (ii = 14; ii > -1; ii -= 1)
            {
              digit_vals[jj][ii] =
                digit_vals[jj][ii] + ((save_val[ii + 1]) / 900);
              /* add in carry */
            }

          for (ii = 15; ii > -1; ii -= 1)
            {
              digit_vals[jj][ii] = ((digit_vals[jj][ii]) % 900); /* get result */

            }

        }

      digit_table_valid = TRUE;

      if (debug)
        {
          for (jj = 0; jj < 46; jj += 1)
            {
              printf
                (" jj = %d : %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d \n",
                 jj, digit_vals[jj][15], digit_vals[jj][14],
                 digit_vals[jj][13], digit_vals[jj][12], digit_vals[jj][11],
                 digit_vals[jj][10], digit_vals[jj][9], digit_vals[jj][8],
                 digit_vals[jj][7], digit_vals[jj][6], digit_vals[jj][5],
                 digit_vals[jj][4], digit_vals[jj][3], digit_vals[jj][2],
                 digit_vals[jj][1], digit_vals[jj][0]);
            }
        }
    }

  b900_result[0] = 0;
  b900_result[1] = 0;
  b900_result[2] = 0;
  b900_result[3] = 0;
  b900_result[4] = 0;

  b900_result[5] = 0;
  b900_result[6] = 0;
  b900_result[7] = 0;
  b900_result[8] = 0;
  b900_result[9] = 0;

  b900_result[10] = 0;
  b900_result[11] = 0;
  b900_result[12] = 0;
  b900_result[13] = 0;
  b900_result[14] = 0;
  b900_result[15] = 0;

  slen = strlen (instring);
  if (debug)
    {
     printf ("In numeric compact, slen = %d string = %s \n", slen, instring);
    }

  for (ii = strlen (instring) - 1; ii > -1; ii -= 1)
    {

      this_digit = instring[ii] - '0';

      if (debug)
        {
          printf (" this_digit = %d \n", this_digit);
        }

      for (kk = 0; kk < this_digit; kk += 1)
        {
          carryin = 0;
          for (mm = 15; mm > -1; mm -= 1) /* add 10^ (slen-ii) * this_digit */
            {
              b900_result[mm] += digit_vals[slen - ii - 1][mm] + carryin;
              if (b900_result[mm] > 899)
                {
                  carryin = 1;
                  b900_result[mm] = b900_result[mm] - 900;
                }
              else
                {
                  carryin = 0;
                }
            }
        }

    }

  leading = FALSE;
  for (kk = 0; kk < 16; kk += 1)
    {
      if ((b900_result[kk] != 0) || (leading == TRUE))
        {
          leading = TRUE;
          put_back (b900_result[kk]);
        }
      else                      /* b900_result[kk] == 0 and leading = FALSE */
        {
          mm = 0;
        }
    }

  if (leading == FALSE)         /* not possible if leading 1 added */
    {
      put_back (0);
    }


}

void
nc (char *instring)
{

  int ll;
  int jj;

  int slen;

  char tempstr[MAX_LINE + 100];
  char short_str[MAX_LINE];
  int debug;

  debug = 0;

  slen = strlen (instring);

  if (debug)
    {
     printf ("In nc - strlen = %d \n", slen);
     printf ("In string = %s \n", instring);
    }

  if (strlen (instring) < 45)
    {
      tempstr[0] = '1';
      tempstr[1] = NULLCHAR;
      strcat (tempstr, instring);
      numeric_compact (tempstr);
    }
  else                          /* more than one group */
    {

      ll = 0;
      while (ll < (strlen (instring) - 44)) /* do the groups */
        {
          short_str[0] = '1';
          for (jj = 0; jj < 44; jj += 1)
            {

              short_str[jj + 1] = instring[jj + ll];
            }
          short_str[jj + 1] = NULLCHAR;
          numeric_compact (short_str);
          ll += 44;
        }
      /* do whats left */

      short_str[0] = '1';

      if (strlen (instring) - ll > 0)
        {
          for (jj = ll; jj < strlen (instring); jj += 1)
            {

              short_str[jj - ll + 1] = instring[jj];

            }
          short_str[jj - ll + 1] = NULLCHAR;
          numeric_compact (short_str);
        }
    }

}                               /* nc */

  /* byte compact a string */

void
byte_compact (char *instring)
{

#define byte_comp_6 924
#define byte_comp_05  901
  int ii;
  int groups;
  int rem;
  long t_code_a[10];
  long t_code_b[10];

  int carry;
  char tstr[MAX_LINE + 256];

  int jj, kk;
  long sum[10];

  int str_len;
  long remainder;

  long valuea;                  /* need to be 32 bit */
  long valueb;                  /* need to be 32 bit */
  long value;
  char this_char;
  int debug;

  debug = 0;
  str_len = strlen (instring);

  if (debug)
    printf ("Byte compact - string = %s \n", instring);

  if ((str_len % 2) != 0)
    {

      tstr[0] = '0';            /* prepend a zero if not even number */
      tstr[1] = NULLCHAR;
      strcat (tstr, instring);
      strcpy (instring, tstr);
      str_len += 1;
    }

  if (strlen (instring) % 2 != 0)
    {
      printf ("Internal error in byte compact \n");
    }

  for (ii = 0; ii < str_len; ii += 1)
    {
      if (!isxdigit (instring[ii]))
        {
          printf (" Bad character in byte compact string = %c \n",
                  instring[ii]);
          printf (" At character number = %i \n", ii);
        }
    }

  groups = (str_len / 12);
  rem = str_len % 12;

  if (debug)
    {
      printf ("str_len = %d \n", str_len);
      printf ("groups = %d \n", groups);
      printf ("rem = %d \n", rem);
    }

  if (rem == 0)
    {
      put_back (byte_comp_6);
    }
  else
    {
      put_back (byte_comp_05);
    }

  jj = 0;
  while (jj < groups)
    {

      valuea = 0;

      for (ii = 0; ii < 6; ii += 1) /* first 6  hex digits */
        {
          this_char = instring[(12 * jj) + ii];

          if (isdigit (this_char))
            {
              valuea = valuea * 16 + (this_char - '0');
            }
          else
            {
              if (isupper (this_char))
                {
                  valuea = valuea * 16 + (10 + this_char - 'A');
                }
              else
                {
                  valuea = valuea * 16 + (10 + this_char - 'a');
                }
            }
        }

      t_code_a[0] = 0;
      t_code_a[1] = 0;
      t_code_a[2] = 0;
      t_code_a[3] = 0;

      kk = 0;
      remainder = valuea;

      if (debug)
        {
          printf ("remainder = %ld \n", remainder);
        }

      while (remainder > 899)
        {
          t_code_a[kk] = remainder % 900;
          remainder = remainder / 900;
          kk += 1;
        }

      t_code_a[kk] = remainder;

      valueb = 0;
      for (ii = 6; ii < 12; ii += 1) /* last 6  hex digits */
        {

          this_char = instring[(12 * jj) + ii];
          if (isdigit (this_char))
            {
              valueb = valueb * 16 + (this_char - '0');
            }
          else
            {
              if (isupper (this_char))
                {
                  valueb = valueb * 16 + (10 + this_char - 'A');
                }
              else
                {
                  valueb = valueb * 16 + (10 + this_char - 'a');
                }
            }
        }

      remainder = valueb;
      kk = 0;
      t_code_b[0] = 0;
      t_code_b[1] = 0;
      t_code_b[2] = 0;
      t_code_b[3] = 0;

      while (remainder > 899)
        {
          t_code_b[kk] = remainder % 900;
          remainder = remainder / 900;
          kk += 1;
        }

      t_code_b[kk] = remainder;


      for (kk = 0; kk < 4; kk += 1)
        {
          if (debug)
            {
              printf ("t_code_a, tcode_b kk = %ld %ld %d \n", t_code_a[kk],
                      t_code_b[kk], kk);
            }
        }
      sum[0] = ((316 * t_code_a[0]) + t_code_b[0]) % 900;
      carry = ((316 * t_code_a[0]) + t_code_b[0]) / 900;

      sum[1] = (t_code_b[1] + (641 * t_code_a[0]) + (316 * t_code_a[1])
                + carry) % 900;
      carry = (t_code_b[1] + (641 * t_code_a[0]) + (316 * t_code_a[1])
               + carry) / 900;


      sum[2] = (t_code_b[2] + (20 * t_code_a[0]) + (641 * t_code_a[1])
                + (316 * t_code_a[2]) + carry) % 900;

      carry = (t_code_b[2] + (20 * t_code_a[0]) + (641 * t_code_a[1])
               + (316 * t_code_a[2]) + carry) / 900;

      sum[3] = (t_code_b[3] + (20 * t_code_a[1]) + (641 * t_code_a[2]) +
                carry) % 900;

      carry = (t_code_b[3] + (20 * t_code_a[1]) + (641 * t_code_a[2])
               + carry) / 900;

      sum[4] = ((20 * t_code_a[2]) + carry) % 900;
      sum[5] = ((20 * t_code_a[2]) + carry) / 900;


      if (debug)
        {
          printf ("Putting back tcodes \n");
        }

      put_back (sum[4]);
      put_back (sum[3]);
      put_back (sum[2]);
      put_back (sum[1]);
      put_back (sum[0]);


      jj += 1;

    }


  if (rem > 0)

    {

      for (ii = 0; ii < rem; ii += 2) /* rem hex digits */
        {
          value = 0;

          this_char = instring[(12 * jj) + ii];

          if (isdigit (this_char))
            {
              value = value * 16 + (this_char - '0');
            }
          else
            {
              if (isupper (this_char))
                {
                  value = value * 16 + (10 + this_char - 'A');
                }
              else
                {
                  value = value * 16 + (10 + this_char - 'a');
                }
            }

          this_char = instring[(12 * jj) + ii + 1];

          if (isdigit (this_char))
            {
              value = value * 16 + (this_char - '0');
            }
          else
            {
              if (isupper (this_char))
                {
                  value = value * 16 + (10 + this_char - 'A');
                }
              else
                {
                  value = value * 16 + (10 + this_char - 'a');
                }
            }
          if (debug)
            {
              printf ("Putting back value = %ld \n", value);
            }
          put_back (value);
        }
    }                           /* if remainder not 0, put out values directly */
}                               /* end byte compact */


/* Generate a Macro PDF control block */

void
macro_compact (char *instring)
{
  char FileID[0x100];
  char buf[0x100];
  int SegIndex;
  int SegCount;
  int i;

  i =
    sscanf (instring, "FileID %s Segment %d/%d", FileID, &SegIndex,
            &SegCount);
  if (i != 3)
    {
      fprintf (stderr, "Bad MCB string: \"%s\"\n" "Terminated.\n", instring);
      exit (1);
    }

  printf ("  macro_compact (\"%s\")\n", instring);
  /*
     printf ("  i          = %d\n"
     "  SegIndex   = %d\n"
     "  SegCount   = %d\n"
     "  FileID     = '%s'\n", i, SegIndex, SegCount, FileID);
   */
  /* 
   * Macro marker codeword to indicate the beginning
   * of a Macro PDF Control Block
   */
  mcb_start = codeindex;
  put_back (928);

  /* Put segment index */
  sprintf (buf, "%05d", SegIndex - 1); // 1st segment is #0
  nc (buf);                     //numeric_compact preserving leading zeroes

  printf ("  segindex:");
  for (i = mcb_start; i < codeindex; i++)
    printf (" %03d", codes[i]);
  printf ("\n");
  /* Put FileID */
  text_compress (FileID);

  printf ("  id:");
  for (i = mcb_start; i < codeindex; i++)
    printf (" %03d", codes[i]);
  printf ("\n");

  {
    /* 
     * i put optional field to mark end of file_id
     * and because i know it :-)
     */
    put_back (923);             /* optional field */
    put_back (1);               /* Segment Count */
    sprintf (buf, "%05d", SegCount); // 1st segment is #0
    nc (buf);                   //numeric_compact preserving leading zeroes
  }

  if (SegIndex == SegCount)
    put_back (922);             /* terminator codeword */

  mcb_end = codeindex;

  printf ("  complete macro sequence:");
  for (i = mcb_start; i < codeindex; i++)
    printf (" %03d", codes[i]);
  printf ("\n");
}

void
outbit (FILE * fp, int bit)
{

  fprintf (fp, bit ? "1" : "0");

}


/* turn the row_column data into bits, for the pdf */


void
PDF417_encodePBM (char *output_filename, Int32 datacols, Int32 datarows,
                  Int32 row_height, Int32 npix)
{
  int i, j, pixn, bitm, k;
  Int32 bitpattern;
  int w;
  FILE *outp;

  if (output_filename    == NULL    )
    outp = stdout;

  if ((output_filename != NULL       ) && 
                     !(outp = fopen (output_filename, "w")))
    {

      fprintf (stderr, "Unable to open output file '%s': %s\n",
               output_filename, strerror (errno));
      exit (1);
    }

  fprintf (outp, "P1\n%d %d\n", ((datacols + 2) * 17 + 1 + 4) * npix,
           datarows * row_height + 4 * npix);

  /* Top quiet zone */
  for (i = 0; i < 2 * npix; ++i)
    {                           /* 2 x npix */
      for (j = 0; j < ((datacols + 2) * 17 + 1 + 4) * npix; ++j)
        {
          outbit (outp, 0);
          fprintf (outp, "\n");
        }

    }
  for (i = 0; i < datarows; ++i)
    {
      for (k = 0; k < row_height; ++k)
        {
          /* Left quiet zone */
          for (pixn = 0; pixn < 2 * npix; pixn++)
            {                   /* 2 * npix */
              outbit (outp, 0);
            }

          for (j = 0; j < 1; j++)
            {

              bitpattern = PDF417_START;

              for (bitm = 16; bitm >= 0; --bitm)
                {
                  for (pixn = 0; pixn < npix; pixn++)
                    {
                      outbit (outp, (bitpattern & (1 << bitm)) ? 1 : 0);
                    }
                }
            }

          w = i % 3;

          for (j = 0; j < datacols; j++)
            {
              bitpattern = PDF417_BITS[w][rows_columns[i][j]];

              for (bitm = 16; bitm >= 0; --bitm)
                {
                  for (pixn = 0; pixn < npix; pixn++)
                    {
                      outbit (outp, (bitpattern & (1 << bitm)) ? 1 : 0);
                    }
                }
            }
          for (j = datacols; j < (datacols + 1); j++)
            {

              bitpattern = PDF417_STOP;

              for (bitm = 16; bitm >= 0; --bitm)
                {
                  for (pixn = 0; pixn < npix; pixn++)
                    {
                      outbit (outp, (bitpattern & (1 << bitm)) ? 1 : 0);
                    }

                }
            }
          for (pixn = 0; pixn < npix; pixn++)
            {
              outbit (outp, 1);
            }
          /* Right quiet zone */
          for (pixn = 0; pixn < 2 * npix; pixn++)
            {
              outbit (outp, 0);
            }
          fprintf (outp, "\n");
        }

    }

  /* Bottom quiet zone */
  for (i = 0; i < 2 * npix; ++i)
    {
      for (j = 0; j < ((datacols + 2) * 17 + 1 + 4) * npix; ++j)
        {
          outbit (outp, 0);
          fprintf (outp, "\n");
        }
    }

  if (output_filename)
    {
      fclose (outp);
    }

}

/* turn the row_column data into bits, for the pdf */


#ifdef DO_GIFS
int
PDF417_encodeGIF (char *gfname, Int32 datacols, Int32 datarows,
                  Int32 row_height, Int32 npix)
{
  int i, j, pixn, bitm, k;
  Int32 bitpattern;
  int w;
  int ii, jj;
  static GifPixelType *ScanLine;
  int ImageWidth, ImageHeight;
  int debug;

  GifFileType *GifFile;
  ColorMapObject *ColorMap;
  ColorMap = MakeMapObject (16, EGAPalette);

  ImageWidth = ((datacols + 2) * 17 + 1 + 4) * npix;
  ImageHeight = datarows * row_height + 4 * npix;

  debug = 0;

  if ((ScanLine =
       (GifPixelType *) malloc (sizeof (GifPixelType) * ImageWidth)) == NULL)
    {

      exit (EXIT_FAILURE);
    }


  GifFile = EGifOpenFileName (gfname, 0);

  if (EGifPutScreenDesc
      (GifFile, ImageWidth, ImageHeight, ColorMap->BitsPerPixel, 0,
       ColorMap) == GIF_ERROR)
    {
      free ((char *) ScanLine);
      return HandleGifError (GifFile);
    }



  if (EGifPutImageDesc (GifFile, 0, 0, ImageWidth, ImageHeight, FALSE,
                        NULL) == GIF_ERROR)
    {
      free ((char *) ScanLine);
      return HandleGifError (GifFile);
    }


  GifQprintf ("\npdf417_enc: Image 1 at (0, 0) [%dx%d]:     ",
              ImageWidth, ImageHeight);

  /* Top quiet zone */
  ii = 0;                       // counts rows
  for (i = 0; i < 2 * npix; ++i)
    {                           /* 2 x npix */
      for (j = 0; j < ImageWidth; ++j)
        {
          ScanLine[j] = (GifPixelType) 15;
        }
      if (EGifPutLine (GifFile, ScanLine, ImageWidth) == GIF_ERROR)
        {
          free ((char *) ScanLine);
          return HandleGifError (GifFile);
        }
      GifQprintf ("\b\b\b\b%-4d", ii);
      ii++;
    }


  for (i = 0; i < datarows; ++i)
    {
      jj = 0;                   //counts columns in ScanLine
      for (k = 0; k < row_height; ++k)
        {
          /* Left quiet zone */
          for (pixn = 0; pixn < 2 * npix; pixn++)
            {                   /* 2 * npix */
              ScanLine[jj] = (GifPixelType) 15;
              jj++;
            }

          if (debug)
            {
              printf ("Done with quiet zone \n");
            }

          for (j = 0; j < 1; j++)
            {

              bitpattern = PDF417_START;

              for (bitm = 16; bitm >= 0; --bitm)
                {
                  for (pixn = 0; pixn < npix; pixn++)
                    {
                      ScanLine[jj] = (bitpattern & (1 << bitm)) ? 0 : 15;
                      jj++;
                    }
                }

              if (debug)
                {
                  printf ("Done with start \n");
                }

            }

          w = i % 3;

          for (j = 0; j < datacols; j++)
            {
              bitpattern = PDF417_BITS[w][rows_columns[i][j]];

              for (bitm = 16; bitm >= 0; --bitm)
                {
                  for (pixn = 0; pixn < npix; pixn++)
                    {
                      ScanLine[jj] = (bitpattern & (1 << bitm)) ? 0 : 15;
                      jj++;
                    }
                }
            }

          if (debug)
            {
              printf ("Done with data cols \n");
            }

          for (j = datacols; j < (datacols + 1); j++)
            {

              bitpattern = PDF417_STOP;

              for (bitm = 16; bitm >= 0; --bitm)
                {
                  for (pixn = 0; pixn < npix; pixn++)
                    {
                      ScanLine[jj] = (bitpattern & (1 << bitm)) ? 0 : 15;
                      jj++;
                    }

                }
            }

          if (debug)
            {
              printf ("Done with stop \n");
            }

          for (pixn = 0; pixn < npix; pixn++)
            {
              ScanLine[jj] = 0;
              jj++;
            }
          /* Right quiet zone */
          for (pixn = 0; pixn < 2 * npix; pixn++)
            {
              ScanLine[jj] = 15;
              jj++;
            }
          if (EGifPutLine (GifFile, ScanLine, ImageWidth) == GIF_ERROR)
            {
              free ((char *) ScanLine);
              return HandleGifError (GifFile);
            }
          GifQprintf ("\b\b\b\b%-4d", ii);
          ii++;
          jj = 0;

          if (debug)
            {
              printf ("Doing right quiet zone \n");
            }

        }

    }

  if (debug)
    {
      printf ("Doing bottom quiet zone \n");
    }

  /* Bottom quiet zone */
  for (i = 0; i < 2 * npix; ++i)
    {

      for (j = 0; j < ImageWidth; ++j)
        {
          ScanLine[j] = (GifPixelType) 15;
        }
      if (EGifPutLine (GifFile, ScanLine, ImageWidth) == GIF_ERROR)
        {
          free ((char *) ScanLine);
          return HandleGifError (GifFile);
        }
      GifQprintf ("\b\b\b\b%-4d", ii);
      ii++;

    }

  if (debug)
    {
      printf ("Done with bottom quiet zone \n");
    }


  if (EGifCloseFile (GifFile) == GIF_ERROR)
    {
      free ((char *) ScanLine);
      return HandleGifError (GifFile);
    }

  if (debug)
    {
      printf ("About to do free \n");
    }

  if (debug)
    {
      printf ("Done with free \n");
    }

}

#endif

void
make_pdf (Int32 ** out)
{

  int ii, jj;

  int w;
  int outlen;

  int outptr;

  *out = NULL;

  /* Each row has start, left, data, right, stop */

  outlen = number_of_rows * (number_of_columns + 2);
  *out = malloc (sizeof (Int32) * outlen);
  if (!*out)
    {
      printf ("Unable to malloc out array \n");
    };

  outptr = 0;

  for (ii = 0; ii < number_of_rows; ii += 1)
    {

      /* Do each row */

      (*out)[outptr++] = PDF417_START;

      w = ii % 3;

      for (jj = 0; jj < number_of_columns; jj += 1)
        {
          (*out)[outptr++] = PDF417_BITS[w][rows_columns[ii][jj]];
        }

      (*out)[outptr++] = PDF417_STOP;
    }

}

void
make_pdf_ps_dim (char *output_filename, Int32 ** out, int justbits, int outtype,
	     int Xwid, int Ydim, int QZ)
{

  int ii, jj;
  int w;

  int bits[2000];
  int abit;
  int mm;
  int tint;
  int this_bit[5];
  int val;
  int cnt_17;
  int debug;
  int tnum;
  int QZ2;
  FILE *outfile1;
  double xwid, rowh;
  double symbol_h, symbol_w;

  if (output_filename    == NULL    )
    outfile1 = stdout;

  if ((output_filename    != NULL    ) && 
                 !(outfile1 = fopen (output_filename, "w")))
    {

      fprintf (stderr, "Unable to open output file '%s': %s\n",
               output_filename, strerror (errno));
      exit (1);
    }
  debug = 0;


  /* set module width, row height */
  if (Xwid <= 0) Xwid = 10;	// number of mils.
  xwid = 1/1000.0 * Xwid;
  if (Ydim <= 0) Ydim = 3;
  rowh = xwid * Ydim;
  if (QZ <= 0) QZ = 2;
  QZ2 = QZ * 2;

  cnt_17 = (number_of_columns + 2) * 17;

  symbol_w = (cnt_17 + 1 + QZ2) * xwid * 72.0;	// stop is 18 bits hence '1';

  symbol_h = (number_of_rows * rowh + QZ2 * xwid) * 72.0;
  if (justbits == FALSE)
    {
      if (outtype != OUTPUT_PS) {
        fprintf (outfile1, "%%!PS-Adobe-1.0 EPSF-1.0\n");
	fprintf (outfile1, "%%%%BoundingBox: 0 0 %.3f %.3f\n",
		symbol_w, symbol_h);
      }
      else
        fprintf (outfile1, "%%!PS-Adobe-1.0\n"
                         "stroke\n"
                         " 100 100 translate\n");

      // fill entire rect with white
      fprintf (outfile1, "1 setgray\n");
      fprintf (outfile1, "0 0 moveto\n");
      fprintf (outfile1, "%.2f %.2f lineto\n", 0.0, symbol_h);
      fprintf (outfile1, "%.2f %.2f lineto\n", symbol_w, symbol_h);
      fprintf (outfile1, "%.2f %.2f lineto\n", symbol_w, 0.0);
      fprintf (outfile1, "0 0 lineto fill\n");

      // move over left and bottom quiet zones
      fprintf (outfile1, "%.3f %.3f translate\n", QZ *xwid * 72.0,
		QZ *xwid * 72.0);

      /* 
       * most readers need smallest bar to be at least .01 inch wide
       * and at least .03 inch high  (.01 inch now replaced by xwid,
       * .03 inch replaced by rowh).
       */
      fprintf (outfile1, " %.3f %.3f scale\n",
               (double)((cnt_17 + 1) * xwid * 72.0),
	       (double)(number_of_rows * rowh * 72.0)) ;
    }
  fprintf (outfile1, " %d %d 1 [%d 0 0 -%d 0 %d] {<\n", cnt_17+1,
           number_of_rows, cnt_17+1, number_of_rows, number_of_rows);

  /* Each row has start, left, data, right, stop */

  // top quiet zone
  for (ii = 0; ii < number_of_rows; ii += 1)
    {

      /* Do each row */

      abit = 0x10000;
      for (mm = 0; mm < 17; mm += 1)
        {
          if ((PDF417_START & abit) > 0)
            {
              bits[mm] = FALSE;
            }
          else
            {
              bits[mm] = TRUE;
            }
          abit = abit >> 1;
        }

      w = ii % 3;

      for (jj = 0; jj < number_of_columns; jj += 1)
        {
          tint = PDF417_BITS[w][rows_columns[ii][jj]];

          if (debug)
            {
              printf ("Row column = %d tint = %x, ii = %d jj = %d \n",
                      rows_columns[ii][jj], tint, ii, jj);
            }
          abit = 0x10000;
          for (mm = 0; mm < 17; mm += 1)
            {
              if ((tint & abit) > 0)
                {
                  bits[mm + (17 * (jj + 1))] = FALSE;
                }
              else
                {
                  bits[mm + (17 * (jj + 1))] = TRUE;
                }
              abit = abit >> 1;
            }

        }

      abit = 0x10000;
      for (mm = 0; mm < 17; mm += 1)
        {
          if ((PDF417_STOP & abit) > 0)
            {
              bits[mm + (17 * (jj + 1))] = FALSE;
            }
          else
            {
              bits[mm + (17 * (jj + 1))] = TRUE;
            }
          abit = abit >> 1;
        }


      cnt_17 = (number_of_columns + 2) * 17;
      bits[cnt_17] = FALSE;		// last (18th) stop bit

tnum = (cnt_17 + 1 + 7) / 8;
tnum <<= 1;
for (mm = cnt_17 + 1; mm < tnum * 4; mm++)
  bits[mm] = TRUE;
      for (mm = 0; mm < tnum; mm += 1)
        {
          this_bit[0] = bits[(4 * mm)];
          this_bit[1] = bits[(4 * mm) + 1];
          this_bit[2] = bits[(4 * mm) + 2];
          this_bit[3] = bits[(4 * mm) + 3];

          val = 0;
          if (this_bit[0] == TRUE)
            {
              val = val + 8;
            }
          if (this_bit[1] == TRUE)
            {
              val = val + 4;
            }
          if (this_bit[2] == TRUE)
            {
              val = val + 2;
            }
          if (this_bit[3] == TRUE)
            {
              val = val + 1;
            }
          if (val < 10)
            {
              fprintf (outfile1, "%c", '0' + val);
              if (debug)
                {
                  printf ("c = %c \n", '0' + val);
                }
            }
          else
            {
              fprintf (outfile1, "%c", 'A' + val - 10);
              if (debug)
                {
                  printf ("c = %c \n", 'A' + val - 10);
                }
            }

        }
      fprintf (outfile1, "\n");

    }


  // bottom quiet zone

#ifdef SHORT_PS

  fprintf (outfile1, ">\n");

  if (output_filename)
    {
      fclose (outfile1);
    }
  return;
#endif
  fprintf (outfile1, ">} image\n");
  if (justbits == FALSE)
    {
      if (outtype == OUTPUT_PS) fprintf (outfile1, " showpage \n");
    }

  if (output_filename)
    {
      fclose (outfile1);
    }

}


void
make_pdf_ps (char *output_filename, Int32 ** out, int justbits)
{

  int ii, jj;

  int w;
  int outlen;

  int outptr;
  int bits[2000];
  int abit;
  int mm;
  int tint;
  int this_bit[5];
  int val;
  int cnt_17;
  int debug;
  int tnum;
  FILE *outfile1;

  if (output_filename    == NULL    )
    outfile1 = stdout;

  if ((output_filename    != NULL    ) && 
                 !(outfile1 = fopen (output_filename, "w")))
    {

      fprintf (stderr, "Unable to open output file '%s': %s\n",
               output_filename, strerror (errno));
      exit (1);
    }
  debug = 0;



  cnt_17 = (number_of_columns + 2) * 17;
  tnum = (((cnt_17 + 6) / 4) << 2) + 8;
  if (tnum % 8 != 0)
    {
      tnum += 4;
    }
  if (justbits == FALSE)
    {
      fprintf (outfile1, "%%!PS-Adobe-1.0\n"
                         "stroke\n"
                         " 100 100 translate\n");
      /* 
       * most readers need smallest bar to be at least .01 inch wide
       * and at least .03 inch high 
       */
      fprintf (outfile1, " %.0f %.0f scale\n",
               tnum * .01 * 72, (number_of_rows + 4) * .03 * 72);
    }
  fprintf (outfile1, " %d %d 1 [%d 0 0 -%d 0 %d] {<\n", tnum,
           number_of_rows + 4, tnum, number_of_rows + 4, number_of_rows + 4);


  /* Each row has start, left, data, right, stop */

  outlen = number_of_rows * (number_of_columns + 2);
  if (debug)
    {
      printf ("Outlen = %d number_of_rows = %d number_of_cols = %d \n",
              outlen, number_of_rows, number_of_columns);
    }


  outptr = 0;

  // top quiet zone

  for (ii = 0; ii < 2; ii += 1)
    {
      tnum = ((cnt_17 + 6) / 4) + 2;
      if (debug)
        {
          printf ("tnum = %d \n", tnum);
        }
      if ((tnum % 2) != 0)
        {
          tnum += 1;
        }
      for (mm = 0; mm < tnum; mm += 1)
        {
          fprintf (outfile1, "F");
        }
      fprintf (outfile1, "\n");
    }
  for (ii = 0; ii < number_of_rows; ii += 1)
    {

      /* Do each row */

      abit = 0x10000;
      for (mm = 0; mm < 17; mm += 1)
        {
          if ((PDF417_START & abit) > 0)
            {
              bits[mm] = FALSE;
            }
          else
            {
              bits[mm] = TRUE;
            }
          abit = abit >> 1;
        }

      w = ii % 3;

      for (jj = 0; jj < number_of_columns; jj += 1)
        {
          tint = PDF417_BITS[w][rows_columns[ii][jj]];

          if (debug)
            {
              printf ("Row column = %d tint = %x, ii = %d jj = %d \n",
                      rows_columns[ii][jj], tint, ii, jj);
            }
          abit = 0x10000;
          for (mm = 0; mm < 17; mm += 1)
            {
              if ((tint & abit) > 0)
                {
                  bits[mm + (17 * (jj + 1))] = FALSE;
                }
              else
                {
                  bits[mm + (17 * (jj + 1))] = TRUE;
                }
              abit = abit >> 1;
            }

        }

      abit = 0x10000;
      for (mm = 0; mm < 17; mm += 1)
        {
          if ((PDF417_STOP & abit) > 0)
            {
              bits[mm + (17 * (jj + 1))] = FALSE;
            }
          else
            {
              bits[mm + (17 * (jj + 1))] = TRUE;
            }
          abit = abit >> 1;
        }


      cnt_17 = (number_of_columns + 2) * 17;
      bits[cnt_17] = FALSE;
      for (mm = cnt_17 + 1; mm < cnt_17 + 20; mm += 1)
        {
          bits[mm] = TRUE;
        }
      for (mm = cnt_17 + 20; mm < cnt_17 + 30; mm += 1)
        {
          bits[mm] = FALSE;
        }
      tnum = ((cnt_17 + 6) / 4) + 2;
      if (debug)
        {
          printf ("tnum = %d \n", tnum);
        }
      if ((tnum % 2) != 0)
        {
          tnum += 1;
        }
      for (mm = 0; mm < tnum; mm += 1)
        {
          this_bit[0] = bits[(4 * mm)];
          this_bit[1] = bits[(4 * mm) + 1];
          this_bit[2] = bits[(4 * mm) + 2];
          this_bit[3] = bits[(4 * mm) + 3];

          val = 0;
          if (this_bit[0] == TRUE)
            {
              val = val + 8;
            }
          if (this_bit[1] == TRUE)
            {
              val = val + 4;
            }
          if (this_bit[2] == TRUE)
            {
              val = val + 2;
            }
          if (this_bit[3] == TRUE)
            {
              val = val + 1;
            }
          if (val < 10)
            {
              fprintf (outfile1, "%c", '0' + val);
              if (debug)
                {
                  printf ("c = %c \n", '0' + val);
                }
            }
          else
            {
              fprintf (outfile1, "%c", 'A' + val - 10);
              if (debug)
                {
                  printf ("c = %c \n", 'A' + val - 10);
                }
            }

        }
      fprintf (outfile1, "\n");

    }


  // bottom quiet zone

  for (ii = 0; ii < 2; ii += 1)
    {
      tnum = ((cnt_17 + 6) / 4) + 2;
      if (debug)
        {
          printf ("tnum = %d \n", tnum);
        }
      if ((tnum % 2) != 0)
        {
          tnum += 1;
        }
      for (mm = 0; mm < tnum; mm += 1)
        {
          fprintf (outfile1, "F");
        }
      fprintf (outfile1, "\n");
    }

#ifdef SHORT_PS

  fprintf (outfile1, ">\n");

  if (output_filename)
    {
      fclose (outfile1);
    }
  return;
#endif
  fprintf (outfile1, ">} image\n");
  if (justbits == FALSE)
    {
      fprintf (outfile1, " showpage \n");
    }

  if (output_filename)
    {
      fclose (outfile1);
    }

}


void
make_pdf_raw (Int32 ** out)
{

  int ii, jj;
  int w;
  int outlen;
  int outptr;
  int abit;
  int mm;
  int tint;
  int cnt_17;
  int debug;
  int tnum;

  debug = 0;

  cnt_17 = (number_of_columns + 2) * 17;
  tnum = (((cnt_17 + 6) / 4) << 2) + 8;
  if (tnum % 8 != 0)
    {
      tnum += 4;
    }


  /* Each row has start, left, data, right, stop */

  outlen = number_of_rows * (number_of_columns + 2);
  if (debug)
    {
      printf ("Outlen = %d number_of_rows = %d number_of_cols = %d \n",
              outlen, number_of_rows, number_of_columns);
    }


  outptr = 0;

  // top quiet zone

  for (ii = 0; ii < 2; ii += 1)
    {
      tnum = ((cnt_17 + 6) / 4) + 2;
      if (debug)
        {
          printf ("tnum = %d \n", tnum);
        }
      if ((tnum % 2) != 0)
        {
          tnum += 1;
        }
      for (mm = 0; mm < tnum; mm += 1)
        {
          raw_bits[ii][4 * mm] = FALSE;
          raw_bits[ii][(4 * mm) + 1] = FALSE;
          raw_bits[ii][(4 * mm) + 2] = FALSE;
          raw_bits[ii][(4 * mm) + 3] = FALSE;
        }

    }

  for (ii = 2; ii < number_of_rows + 2; ii += 1)
    {

      /* Do each row */

      abit = 0x10000;
      for (mm = 0; mm < 17; mm += 1)
        {
          if ((PDF417_START & abit) > 0)
            {
              raw_bits[ii][mm] = FALSE;
            }
          else
            {
              raw_bits[ii][mm] = TRUE;
            }
          abit = abit >> 1;
        }

      w = ii % 3;

      for (jj = 0; jj < number_of_columns; jj += 1)
        {
          tint = PDF417_BITS[w][rows_columns[ii][jj]];

          if (debug)
            {
              printf ("Row column = %d tint = %x, ii = %d jj = %d \n",
                      rows_columns[ii][jj], tint, ii, jj);
            }
          abit = 0x10000;
          for (mm = 0; mm < 17; mm += 1)
            {
              if ((tint & abit) > 0)
                {
                  raw_bits[ii][mm + (17 * (jj + 1))] = FALSE;
                }
              else
                {
                  raw_bits[ii][mm + (17 * (jj + 1))] = TRUE;
                }
              abit = abit >> 1;
            }

        }

      // do the stop

      abit = 0x10000;
      for (mm = 0; mm < 17; mm += 1)
        {
          if ((PDF417_STOP & abit) > 0)
            {
              raw_bits[ii][mm + (17 * (jj + 1))] = FALSE;
            }
          else
            {
              raw_bits[ii][mm + (17 * (jj + 1))] = TRUE;
            }
          abit = abit >> 1;
        }

      //  do right space

      cnt_17 = (number_of_columns + 2) * 17;
      raw_bits[ii + 2][cnt_17] = FALSE;
      for (mm = cnt_17 + 1; mm < cnt_17 + 20; mm += 1)
        {
          raw_bits[ii][mm] = TRUE;
        }
      for (mm = cnt_17 + 20; mm < cnt_17 + 30; mm += 1)
        {
          raw_bits[ii][mm] = FALSE;
        }
      tnum = ((cnt_17 + 6) / 4) + 2;
      if (debug)
        {
          printf ("tnum = %d \n", tnum);
        }
      if ((tnum % 2) != 0)
        {
          tnum += 1;
        }

    }


  // bottom quiet zone

  for (ii = number_of_rows + 2; ii < number_of_rows + 4; ii += 1)
    {
      tnum = ((cnt_17 + 6) / 4) + 2;
      if (debug)
        {
          printf ("tnum = %d \n", tnum);
        }
      if ((tnum % 2) != 0)
        {
          tnum += 1;
        }
      for (mm = 0; mm < tnum; mm += 1)
        {
          raw_bits[ii][4 * mm] = FALSE;
          raw_bits[ii][(4 * mm) + 1] = FALSE;
          raw_bits[ii][(4 * mm) + 2] = FALSE;
          raw_bits[ii][(4 * mm) + 3] = FALSE;
        }

    }

}



int
get_cvalue_Mode (unsigned char inchar, int *Mode_ret)
{

  int Mode;
  int value;


  if (isalpha ((unsigned char) inchar) || isdigit ((unsigned char) inchar))
    {                           /* A-Z a-z or 0-9 */

      if (isalpha (inchar))
        {                       /* A-Z or a-z */

          if (isupper (inchar))
            {                   /* A-Z */
              value = inchar - 'A';
              Mode = 8;
            }
          else
            {
              value = inchar - 'a';

              Mode = 4;
            }
        }
      else
        {
          value = inchar - '0';
          Mode = 2;
        }

    }
  else
    {

      switch (inchar)
        {
        case '&':
          value = 10;
          Mode = 2;
          break;
        case CR:
          value = 11;
          Mode = 3;
          break;
        case HT:
          value = 12;
          Mode = 3;
          break;
        case ',':
          value = 13;
          Mode = 3;
          break;
        case ':':
          value = 14;
          Mode = 3;
          break;
        case '#':
          value = 15;
          Mode = 2;
          break;
        case '-':
          value = 16;
          Mode = 3;
          break;
        case '.':
          value = 17;
          Mode = 3;
          break;
        case '$':
          value = 18;
          Mode = 3;
          break;
        case '/':
          value = 19;
          Mode = 3;
          break;
        case '+':
          value = 20;
          Mode = 2;
          break;
        case '%':
          value = 21;
          Mode = 2;
          break;
        case '*':
          value = 22;
          Mode = 3;
          break;
        case '=':
          value = 23;
          Mode = 2;
          break;
        case '^':
          value = 24;
          Mode = 2;
          break;
        case ' ':
          value = 26;
          Mode = 14;
          break;
        case ';':
          value = 0;
          Mode = 1;
          break;
        case '<':
          value = 1;
          Mode = 1;
          break;
        case '>':
          value = 2;
          Mode = 1;
          break;
        case '@':
          value = 3;
          Mode = 1;
          break;
        case '[':
          value = 4;
          Mode = 1;
          break;
        case '\\':
          value = 5;
          Mode = 1;
          break;
        case ']':
          value = 6;
          Mode = 1;
          break;
        case '_':
          value = 7;
          Mode = 1;
          break;
        case '`':
          value = 8;
          Mode = 1;
          break;
        case '~':
          value = 9;
          Mode = 1;
          break;
        case '!':
          value = 10;
          Mode = 1;
          break;
        case LF:
          value = 15;
          Mode = 1;
          break;
        case '"':
          value = 20;
          Mode = 1;
          break;
        case '|':
          value = 21;
          Mode = 1;
          break;
        case '(':
          value = 23;
          Mode = 1;
          break;
        case ')':
          value = 24;
          Mode = 1;
          break;
        case '?':
          value = 25;
          Mode = 1;
          break;
        case '{':
          value = 26;
          Mode = 1;
          break;
        case '}':
          value = 27;
          Mode = 1;
          break;
        case '\'':
          value = 28;
          Mode = 1;
          break;
        default:
          value = 26;
          Mode = 14;
          printf ("Internal error \n");
        }
    }

  *Mode_ret = Mode;
  return (value);
}


#define latch_to_alpha_num 28
#define latch_to_alpha_punct 29
#define latch_to_num  28
#define latch_to_lower 27
#define latch_to_punct 25
#define latch_fr_mixed_to_alpha 28
#define switch_to_punct 29
#define switch_to_alpha 27
#define TRUE 1
#define FALSE 0

/* expand \CR \LF \BS \NL \DQ */

void
expand_string (char *instring)
{

  char tempstr[MAX_LINE + 256];
  char newstr[MAX_LINE + 256];
  int parseok;
  int debug;
  int ii, jj;

  debug = 0;
  strcpy (tempstr, instring);

  ii = 0;
  jj = 0;

  while (ii < strlen (tempstr))
    {
      if (tempstr[ii] == BACKSLASH)
        {
          parseok = FALSE;
          if ((tempstr[ii + 1] == 'L') && (tempstr[ii + 2] == 'F'))
            {
              newstr[jj] = LF;
              ii += 3;
              jj += 1;
              parseok = TRUE;
            }
          if ((tempstr[ii + 1] == 'C') && (tempstr[ii + 2] == 'R'))
            {
              newstr[jj] = CR;
              ii += 3;
              jj += 1;
              parseok = TRUE;
            }

          if ((tempstr[ii + 1] == 'B') && (tempstr[ii + 2] == 'S'))
            {
              newstr[jj] = BACKSLASH;
              ii += 3;
              jj += 1;
              parseok = TRUE;
            }
          if ((tempstr[ii + 1] == 'N') && (tempstr[ii + 2] == 'L'))
            {
              newstr[jj] = CR;
              newstr[jj + 1] = LF;
              ii += 3;
              jj += 2;
              parseok = TRUE;
            }
          if ((tempstr[ii + 1] == 'D') && (tempstr[ii + 2] == 'Q'))
            {
              newstr[jj] = '"';
              ii += 3;
              jj += 1;
              parseok = TRUE;
            }
          if (parseok == FALSE)
            {
              newstr[jj] = tempstr[ii];
              jj += 1;
              ii += 1;
              printf ("Warning text string has single backslash \n");
              printf (" Use \\BS if you want a back slash \n");
            }

        }
      else
        {
          newstr[jj] = tempstr[ii];
          ii += 1;
          jj += 1;
        }
    }

  newstr[jj] = NULLCHAR;

  if (debug)
    {
      for (ii = 0; ii < strlen (newstr); ii += 1)
        {
          printf (" newstr [%d] = %d - %c \n", ii, newstr[ii], newstr[ii]);
        }
    }

  strcpy (instring, newstr);

}

void
text_compress (char *instring)
{

  int this_prev_type;
  int prev_value;
  int prev_type;
  int end_of_text;

  char new_char;

  int value1;
  int value2;

  int type1;
  int jj;
  int num_type_found;
  int debug;
  int total_chars;
  int index;
  int typetemp;
  int symbol_char_array[2000];

  int single_punctuation;
  int single_capital;

  int ii;

  debug = 0;
  prev_value = 0;
  prev_type = 8;

  end_of_text = FALSE;


  total_chars = 0;

  jj = 0;

  if (debug)
    {
      printf ("In text_compress - instring = %s \n", instring);
    }

  if (debug)
    {
      for (ii = 0; ii < strlen (instring); ii += 1)
        {
          printf ("Char %d = %d \n", ii, instring[ii]);
        }
    }

  index = 0;

  while (end_of_text == FALSE)
    {
      new_char = instring[jj];
      jj += 1;

      if (debug)
        {
          printf ("New char = %c \n", new_char);
        }

      value1 = get_cvalue_Mode (new_char, &type1);


      if (debug)
        {
          printf (" value1 = %d  type1 = %d \n", value1, type1);
          printf (" prev_value = %d  prev_type = %d \n", prev_value,
                  prev_type);
        }
      if ((type1 & prev_type) > 0) // some overlap
        {
          if (debug)
            {
              printf ("Some overlap  value1 = %d prev_value = %d \n", value1,
                      prev_value);
            }

          symbol_char_array[index] = value1;
          index += 1;

          prev_value = value1;
          if ((type1 == 8) || (type1 == 4) || (type1 == 2) || (type1 == 1))
            {
              prev_type = type1;
            }

        }

      else                      // no overlap, must do a sub break
        {
          this_prev_type = prev_type;

          if (debug)
            {
              printf ("No overlap- this_prev_type = %d \n", this_prev_type);
            }

          if ((this_prev_type & 8) > 0)
            {
              num_type_found = FALSE;

              // A-Z followed by

              if ((type1 & 2) > 0)
                {
                  num_type_found = TRUE;
                  // number

                  if (debug)
                    {
                      printf ("A-Z followed by number,mixed \n");
                    }

                  symbol_char_array[index] = latch_to_num;
                  index += 1;

                  symbol_char_array[index] = value1;
                  index += 1;

                  prev_type = 2;
                  prev_value = value1;

                }

              if (((type1 & 1) > 0) && (num_type_found == FALSE))
                {


                  if (debug)
                    {
                      printf (" Capital followed by punctuation \n");
                    }
                  // punctuation

                  if (instring[jj] != NULLCHAR)
                    {
                      value2 = get_cvalue_Mode (instring[jj], &typetemp);
                    }
                  else
                    {
                      typetemp = 0;
                    }

                  if ((typetemp & 1) > 0)
                    {
                      single_punctuation = FALSE;
                    }
                  else
                    {
                      single_punctuation = TRUE;
                    }

                  if (single_punctuation)
                    {
                      symbol_char_array[index] = switch_to_punct;
                      index += 1;
                      symbol_char_array[index] = value1;
                      index += 1;
                    }
                  else
                    {
                      symbol_char_array[index] = latch_to_num;
                      index += 1;
                      symbol_char_array[index] = latch_to_punct;
                      index += 1;
                      symbol_char_array[index] = value1;
                      index += 1;

                      prev_type = 1;
                      prev_value = value1;
                    }

                }

              if ((type1 & 4) > 0)
                {

                  if (debug)
                    {
                      printf (" Capital followed by lower \n");
                    }

                  // lower case


                  if (debug)
                    {
                      printf ("Latch to lower, begin \n");
                    }
                  symbol_char_array[index] = latch_to_lower;
                  index += 1;
                  symbol_char_array[index] = value1;
                  index += 1;
                  prev_value = value1;
                  prev_type = 4;

                }
            }


          if ((this_prev_type & 4) > 0)
            {
              // lower followed by
              num_type_found = FALSE;

              if ((type1 & 8) > 0)
                {

                  // Capital
                  if (debug)
                    {
                      printf ("Lower followed by capital \n");
                    }

                  if (instring[jj] != NULLCHAR)
                    {
                      value2 = get_cvalue_Mode (instring[jj], &typetemp);
                    }
                  else
                    {
                      typetemp = 0;
                    }

                  if ((typetemp & 8) > 0)
                    {
                      single_capital = FALSE;
                    }
                  else
                    {
                      single_capital = TRUE;
                    }

                  if (single_capital)
                    {

                      symbol_char_array[index] = switch_to_alpha;
                      index += 1;
                      symbol_char_array[index] = value1;
                      index += 1;

                    }
                  else
                    {

                      symbol_char_array[index] = latch_to_num;
                      index += 1;
                      symbol_char_array[index] = latch_to_alpha_num;
                      index += 1;
                      symbol_char_array[index] = value1;
                      index += 1;
                      prev_type = 8;
                      prev_value = value1;
                    }

                }

              if ((type1 & 2) > 0)
                {
                  num_type_found = TRUE;

                  if (debug)
                    {
                      printf ("Lower followed by number \n");
                    }
                  // number

                  symbol_char_array[index] = latch_to_num;
                  index += 1;
                  symbol_char_array[index] = value1;
                  index += 1;
                  prev_type = 2;
                  prev_value = value1;

                }

              if (((type1 & 1) > 0) && (num_type_found == FALSE))
                {

                  // punctuation
                  if (debug)
                    {
                      printf ("Lower followed by punctuation \n");
                    }

                  if (instring[jj] != NULLCHAR)
                    {

                      value2 = get_cvalue_Mode (instring[jj], &typetemp);
                    }
                  else
                    {
                      typetemp = 0;
                    }
                  if ((typetemp & 1) > 0)
                    {
                      single_punctuation = FALSE;
                    }
                  else
                    {
                      single_punctuation = TRUE;
                    }

                  if (single_punctuation)
                    {
                      symbol_char_array[index] = switch_to_punct;
                      index += 1;
                      symbol_char_array[index] = value1;
                      index += 1;
                    }
                  else
                    {
                      symbol_char_array[index] = latch_to_num;
                      index += 1;
                      symbol_char_array[index] = latch_to_punct;
                      index += 1;
                      symbol_char_array[index] = value1;
                      index += 1;

                      prev_type = 1;
                      prev_value = value1;
                    }

                }

            }

          if ((this_prev_type & 2) > 0)
            {


              // num followed by

              if ((type1 & 8) > 0)
                {

                  // Capital

                  if (debug)
                    {
                      printf ("num followed by capital \n");
                    }


                  symbol_char_array[index] = latch_fr_mixed_to_alpha;
                  index += 1;
                  symbol_char_array[index] = value1;
                  index += 1;

                  prev_type = 8;
                  prev_value = value1;

                }

              if ((type1 & 4) > 0)
                {

                  // lower

                  if (debug)
                    {
                      printf ("num followed by lower \n");
                    }

                  symbol_char_array[index] = latch_to_lower;
                  index += 1;
                  symbol_char_array[index] = value1;
                  index += 1;
                  prev_value = value1;
                  prev_type = 4;


                }

              if ((type1 & 1) > 0)

                {

                  // punct

                  if (debug)
                    {
                      printf ("num followed by punctuation \n");
                    }


                  if (instring[jj] != NULLCHAR)
                    {
                      value2 = get_cvalue_Mode (instring[jj], &typetemp);
                    }
                  else
                    {
                      typetemp = 0;
                    }
                  if ((typetemp & 1) > 0)
                    {
                      single_punctuation = FALSE;
                    }
                  else
                    {
                      single_punctuation = TRUE;
                    }
                  if (single_punctuation)
                    {
                      symbol_char_array[index] = switch_to_punct;
                      index += 1;
                      symbol_char_array[index] = value1;
                      index += 1;
                    }
                  else
                    {
                      symbol_char_array[index] = latch_to_punct;
                      index += 1;
                      symbol_char_array[index] = value1;
                      index += 1;
                      prev_type = 1;
                      prev_value = value1;
                    }
                }

            }


          if (((this_prev_type & 1) > 0) && ((this_prev_type & 2) == 0))
            {

              // punct, not any also found in mixed,  followed by

              if ((type1 & 8) > 0)
                {

                  if (debug)
                    {
                      printf ("punct followed by capital \n");
                    }
                  // Capital

                  symbol_char_array[index] = latch_to_alpha_punct;
                  index += 1;
                  symbol_char_array[index] = value1;
                  index += 1;
                  prev_type = 8;
                  prev_value = value1;

                }

              else if ((type1 & 4) > 0)
                {

                  // lower
                  if (debug)
                    {
                      printf ("punct followed by lower \n");
                    }

                  symbol_char_array[index] = latch_to_alpha_punct;
                  index += 1;
                  symbol_char_array[index] = latch_to_lower;
                  index += 1;
                  symbol_char_array[index] = value1;
                  index += 1;
                  prev_value = value1;
                  prev_type = 4;

                }

              else if ((type1 & 2) > 0)

                {
                  if (debug)
                    {
                      printf ("punct followed by number \n");
                    }
                  // num

                  symbol_char_array[index] = latch_to_alpha_punct;
                  index += 1;
                  symbol_char_array[index] = latch_to_num;
                  index += 1;
                  symbol_char_array[index] = value1;
                  index += 1;
                  prev_value = value1;
                  prev_type = 2;

                }               /* number */

            }                   /* punctuation */

        }                       /* overlap */

      if (instring[jj] == NULLCHAR)
        {
          end_of_text = TRUE;
        }

    }                           /* while end of text == false */

  if (index % 2 == 1)
    {
      symbol_char_array[index] = 29;
      index += 1;
    }

  for (ii = 0; ii < (index / 2); ii += 1)
    {
      value2 = (30 * symbol_char_array[2 * ii])
        + symbol_char_array[(2 * ii) + 1];
      if (debug)
        {
          printf ("Makeing symbol - %d , %d , %d \n", ii,
                  symbol_char_array[2 * ii], symbol_char_array[(2 * ii) + 1]);
        }

      put_back (value2);

    }

}

void
do_row_begin (int row, int eclen)
{

  int left;
  int debug;

  debug = 0;
  if (row < 90)
    {
      if (row % 3 == 0)
        {
          left = ((row / 3) * 30) + ((number_of_rows - 1) / 3);
        }
      if (row % 3 == 1)
        {
          left = ((row / 3) * 30) + ((number_of_rows - 1) % 3) +
            (3 * ec_level);
        }
      if (row % 3 == 2)
        {
          left = ((row / 3) * 30) + (number_of_columns - 3);

        }

      rows_columns[row][0] = left;

      if (debug)
        {
          printf ("code = %d \n", left);
          printf ("rc = B0x%x\n", PDF417_BITS[row % 3][left]);
        }
    }
  else
    {
      printf ("Error: More than 90 rows \n");
    }

}

void
do_row_end (int row, int eclen)
{
  int debug;
  int right;

  debug = 0;
  if (row < 90)
    {
      if (row % 3 == 1)
        {
          right = ((row / 3) * 30) + ((number_of_rows - 1) / 3);
        }
      if (row % 3 == 2)
        {
          right = ((row / 3) * 30) + ((number_of_rows - 1) % 3) +
            (3 * ec_level);
        }
      if (row % 3 == 0)
        {
          right = ((row / 3) * 30) + (number_of_columns - 3);

        }
      rows_columns[row][number_of_columns - 1] = right;

      if (debug)
        {
          printf ("code = %d\n", right);
          printf ("rc = 0x%x\n", PDF417_BITS[row % 3][right]);
        }
    }
  else
    {
      printf ("Error: More than 90 rows \n");
    }




}

void
do_data (int row, int c_index, int index)
{
  int this_data;
  int debug;
  debug = 0;

  this_data = data[index];
  // if (debug)
  //  {
  //   printf("Doing do_data - index = %d data = %d \n", index, data[index]);
  //    printf("row = %d col = %d \n",row,c_index + 1);
  //   }
  rows_columns[row][c_index + 1] = this_data;

  if (debug)
    {

      printf ("code = %d,  l,r = %d %d \n", this_data, this_data / 30,
              this_data % 30);
      printf ("rc = 0x%x\n", PDF417_BITS[row % 3][this_data]);
    }
}

void
do_prelude ()
{

}

void
do_post ()
{

}

/* make the rows and columns, except for the begin and stop stuff */

void
do_rows_columns (int eclen)
{
  int i, j;
  int index;
  int debug;

  debug = 0;

  if (debug)
    {
      printf ("Do rows and columns \n");
    }

  index = 0;
  for (i = 0; i < number_of_rows; i += 1)
    {

      do_row_begin (i, eclen);

      for (j = 0; j < number_of_columns - 2; j += 1)
        {
          index = i * (number_of_columns - 2) + j;

          do_data (i, j, index);

        }

      do_row_end (i, eclen);

    }

  if (debug)
    {
      printf (" Return from do rows and columns \n");
    }

}



void
get_line (char *linestr, FILE * infile)
{
  int i;
  char *fgets ();
  char *fstat;
  char *lineptr;
  int debug;

  debug = 0;

  if (debug)
    {
      printf ("In get line \n");
    }

  fstat = fgets (linestr, MAX_LINE, infile);


  if (fstat == NULL)
    {
      end_of_file = TRUE;
    }

  lineptr = &linestr[0];
  linenum += 1;

  i = 0;
  while ((*lineptr != '\n') && (i < MAX_LINE))
    {
      if ((*lineptr < ' ') && (*lineptr != HT))
        {
          *lineptr = ' ';
        }
      lineptr++;
      i += 1;
    }
}                               /* end of getline */


void
skipblanks ()
{

  while ((inline_str[linechar] == ' ') && (linechar < MAX_LINE)
         && (inline_str[linechar] != '\n'))
    {
      linechar += 1;
    }

}                               /* skipblanks */


/* get the next operation from the input file */


int
get_next_operation ()
{
  int jj;
  int op;



  int debug;

  debug = 0;
  linechar = 0;

  if (debug)
    {
      printf ("In get next operation - inline_str = %s \n", inline_str);
    }

  skipblanks ();

  op = -1;

  if (strncmp (&inline_str[linechar], "NC", 2) == 0)
    op = NC;
  if (strncmp (&inline_str[linechar], "TC", 2) == 0)
    op = TC;
  if (strncmp (&inline_str[linechar], "BC", 2) == 0)
    op = BC;
  if (strncmp (&inline_str[linechar], "MC", 2) == 0)
    op = MC;                    /* MACRO CONTROL BLOCK */

  if (op != -1)
    {
      linechar += 2;
    }
  else
    {
      printf (" Error: Input line must begin with NC, TC, BC or MC\n");
    }

  while ((inline_str[linechar] != quotchar)
         && (inline_str[linechar] != NULLCHAR))
    {
      linechar += 1;
    }

  jj = 0;
  opstring[0] = NULLCHAR;

  if (inline_str[linechar] == quotchar)
    {
      linechar += 1;
      while ((inline_str[linechar] != quotchar)
             && (inline_str[linechar] != NULLCHAR) && (jj < MAX_LINE))
        {
          opstring[jj] = inline_str[linechar];
          jj += 1;
          linechar += 1;
        }
      opstring[jj] = NULLCHAR;

    }

  if (debug)
    {
      printf ("In get next op - opstring = %s \n", opstring);
    }
  return (op);
}

void
do_mode_latch (int incode)
{

  put_back (incode);


}                               /* do_mode_latch */


// read the input data from a file
//
void
get_data ()
{
  int op;
  int prev_op;
  int done;
  int debug;

  debug = 0;


  /* infile1 = fopen("temp", "r"  ); */


  if (infile1 == NULL)
    {
      printf (" Could not open file temp \n");
      exit (0);

    }

  end_of_file = FALSE;

  prev_op = -1;

  get_line (inline_str, infile1);

  if (debug)
    {
      printf ("First line = %s \n", inline_str);
    }

  while (end_of_file == FALSE)
    {

      if (debug)
        {
          printf ("About to call get_next_op \n");
        }

      op = get_next_operation ();

      done = FALSE;

      if ((op == BC) && (prev_op == TC))
        {

          // do_mode_latch( 924);
          done = TRUE;
          prev_op = BC;
        }

      if ((op == BC) && (prev_op == -1) && (done == FALSE))
        {
          done = TRUE;
          prev_op = BC;
        }

      if ((op == BC) && (prev_op == NC) && (done == FALSE))
        {

          //      do_mode_latch( 901);
          done = TRUE;
          prev_op = BC;
        }

      if ((op == BC) && (prev_op == BC) && (done == FALSE))
        {

          done = TRUE;
          prev_op = BC;
        }

      if ((op == TC) && (prev_op == BC) && (done == FALSE))
        {

          do_mode_latch (900);
          done = TRUE;
          prev_op = TC;
        }

      if ((op == TC) && (prev_op == NC) && (done == FALSE))
        {

          do_mode_latch (900);
          done = TRUE;
          prev_op = TC;
        }

      if ((op == TC) && (prev_op == -1) && (done == FALSE))
        {

          /*  do_mode_latch( 900);    */
          /* no need to put out 900 because Text Compress is default */

          done = TRUE;
          prev_op = TC;
        }

      if ((op == TC) && (prev_op == TC) && (done == FALSE))
        {

          do_mode_latch (900);
          done = TRUE;
          prev_op = TC;
        }

      if ((op == NC) && (prev_op == TC) && (done == FALSE))
        {

          do_mode_latch (902);
          done = TRUE;
          prev_op = NC;
        }

      if ((op == NC) && (prev_op == -1) && (done == FALSE))
        {

          do_mode_latch (902);
          done = TRUE;
          prev_op = NC;
        }

      if ((op == NC) && (prev_op == BC) && (done == FALSE))
        {

          do_mode_latch (902);
          done = TRUE;
          prev_op = NC;
        }

      if ((op == NC) && (prev_op == NC) && (done == FALSE))
        {


          done = TRUE;
          prev_op = NC;
        }

      if (opstring[0] != NULLCHAR)
        {
          switch (op)
            {

            case NC:
              nc (opstring);
              break;


            case BC:
              byte_compact (opstring);
              break;


            case TC:
              expand_string (opstring);
              text_compress (opstring);
              break;

            case MC:
              macro_compact (opstring);
              break;

            default:
              printf ("Unknown compaction mode");
              exit (1);
            }
        }
      get_line (inline_str, infile1);

      if (debug)
        {
          printf ("Next line = %s \n", inline_str);
        }

    }

  if (debug)
    {
      printf ("Leaving get_data \n");
    }

}


#define PDF417_RECORD_LEN (1024)

//
//  Take from a buffer and put in raw_input array for byte compress
//
void output_bc_raw(const char* buf, int len, int linecnt)
{
    int i;
    char tstr[20];

    if (!buf || !len || ( linecnt > 63))
    {
        return;
    }
    
    for (i = 0; i < len; i++)
    {
        if (i % PDF417_RECORD_LEN == 0)
        {
            sprintf(raw_input[linecnt],"BC \"");
        }
        
        sprintf(tstr, "%02X", (unsigned char)buf[i]);
        strncat(raw_input[linecnt],tstr,3);
        
        if ((i + 1) % PDF417_RECORD_LEN == 0 || i == len - 1)
        {
            strncat(raw_input[linecnt],"\"\n",4);
        }        
    }
}

//
//  Take from a buffer and put in raw_input array for numeric compress
//
void output_nc_raw(const char* buf, int len, int linecnt)
{
    int i;
    char tstr[20];

    if (!buf || !len || (linecnt > 63))
    {
        return;
    }
    for (i = 0; i < len; i++)
    {
        if (i % PDF417_RECORD_LEN == 0)
        {
            sprintf(raw_input[linecnt],"NC \"");
        }
        
        tstr[0] =buf[i];
        tstr[1] = NULLCHAR;
        strncat( raw_input[linecnt],tstr,5);

        if ((i + 1) % PDF417_RECORD_LEN == 0 || i == len - 1)
        {
            strncat(raw_input[linecnt],"\"\n",4);
        }
    }
}

//
//  Take from a buffer and put in raw_input array for text compress
//
void output_tc_raw(const char* buf, int len, int linecnt)
{
    int i ;
    char tstr[10];

    if (!buf || !len || (linecnt > 63))
    {
        return;
    }
    
    for (i = 0; i < len; i++)
    {
        if (i % PDF417_RECORD_LEN == 0)
        {
            sprintf(raw_input[linecnt],"TC \"");
        }
        
        switch(buf[i])
        {
        case '\"':
            strncat(raw_input[linecnt],"\\DQ", 6);
            break;
        case '\n':
            strncat(raw_input[linecnt],"\\LF", 6);
            break;
        case '\r':
            strncat(raw_input[linecnt],"\\CR", 6);
            break;
        case '\\':
            strncat(raw_input[linecnt],"\\BS", 6);
            break;        
        default:
	  tstr[0] = buf[i];
          tstr[1] = NULLCHAR;
	  strncat(raw_input[linecnt],tstr,4);
            break;
        }
        
        if ((i + 1) % PDF417_RECORD_LEN == 0 || i == len - 1)
        {
            strncat(raw_input[linecnt],"\"\n", 4);
        }
    }
}

int is_tc(char c)
{
    if (c >= 32 && c <= 126)
    {
        return 1;
    }
    else if (c && strchr("\t\n\r", c))
    {
        return 1;
    }
    
    return 0;
}

int is_nc(char c)
{
    return (c >= '0' && c <= '9');
}


/* Very literal implementation of Annex P algorithm */
// take a buffer full of text and
// using annex P algorithm, convert to the raw format

int pdf417_prep_to_raw(const char* buffer, int len)
{
    const char* p; //ptr to current sequence of bytes
    int n, t, b;  //counters for numeric, text, and byte compaction candidates

    int linecnt;
         
    p = buffer;
    linecnt = 0;

    while(p < buffer + len)
    {
        //STEP3: test for sequence for numeric compaction
    
        n = 0;
    
        while(p + n < buffer + len)
        {
            if (!is_nc(p[n]))
            {
                break;
            }
            n++;
        }
        
        if (n >= 13)
        {
            /* Use numeric compaction */
            output_nc_raw(p, n, linecnt);
            linecnt += 1;
            p += n;
            continue;
        }
        
        //STEP10: test for sequence for text compaction
        
        t = n = 0;
    
        while(p + n + t  < buffer + len)
        {
            if (is_nc(p[n + t]))
            {
                n++;
            }
            else if (is_tc(p[n + t]))
            {
                t++;
                t += n;
                n = 0;
            }
            else
            {
                break;
            }

            if (n == 13)
            {
                break;
            }
        }

        if (n < 13)
        {
            //encountered BC byte or end of buffer reached. convert remaining n to t
            t += n;
            n = 0;
        }
        
        if (t >= 5)
        {
            /* Use text compaction */
            output_tc_raw(p, t, linecnt);
            linecnt += 1;
            p += t;
            continue;
        }
        
        //STEP17: test for sequence for byte compaction
        
        b = t = n = 0;
    
        while(p + n + t + b < buffer + len)
        {
            if (is_nc(p[n + t + b]))
            {
                n++;
            }
            else if (is_tc(p[n + t + b]))
            {
                t++;
                t += n;
                n = 0;
            }
            else
            {
                b++;
                b += n + t;
                n = 0;
                t = 0;
            }
            if (n == 13)
            {
                b += t;
                t = 0;
                break;
            }
            if (t >= 5)
            {
                b += n;
                n = 0;
                break;
            }
        }

        
        if (n < 13)
        {
            //end of buffer reached. convert remaining n to t
            t += n;
            n = 0;
        }

        if (t < 5)
        {
            //end of buffer reached. convert remaining t to b
            b += t;
            t = 0;
        }

        if (b >= 1)
        {
            // Use byte compaction

            // Annex P Step 18 recommends:
            // If b == 1 and current mode is text,
            // then we should SHIFT into Byte Compaction Mode;
            // otherwise, we should LATCH into Byte Compaction Mode.
            // pdf417_encode should make this distinction since
            // we cannot do it here.
            
            output_bc_raw(p, b, linecnt);
            linecnt += 1;
            p += b;
        }
    }

    pdf417_set_raw_count( linecnt);   // number of line in raw input

    return 1;
}

//
// read the input data from an array of strings
//
void
get_data_raw (int num_lines)
{
  int op;
  int prev_op;
  int done;
  int debug;
  int linenum;

  debug = 0;


  end_of_file = FALSE;

  prev_op = -1;

  strncpy (inline_str, raw_input[0], MAX_LINE);
  linechar = 0;

  if (debug)
    {
      printf ("First line = %s \n", inline_str);
    }

  linenum = 1;
  while (linenum <= raw_lines)
    {

      if (debug)
        {
          printf ("About to call get_next_op \n");
        }

      op = get_next_operation ();

      done = FALSE;

      if ((op == BC) && (prev_op == TC))
        {

          // do_mode_latch( 924);
          done = TRUE;
          prev_op = BC;
        }

      if ((op == BC) && (prev_op == -1) && (done == FALSE))
        {
          done = TRUE;
          prev_op = BC;
        }

      if ((op == BC) && (prev_op == NC) && (done == FALSE))
        {

          //      do_mode_latch( 901);
          done = TRUE;
          prev_op = BC;
        }

      if ((op == BC) && (prev_op == BC) && (done == FALSE))
        {

          done = TRUE;
          prev_op = BC;
        }

      if ((op == TC) && (prev_op == BC) && (done == FALSE))
        {

          do_mode_latch (900);
          done = TRUE;
          prev_op = TC;
        }

      if ((op == TC) && (prev_op == NC) && (done == FALSE))
        {

          do_mode_latch (900);
          done = TRUE;
          prev_op = TC;
        }

      if ((op == TC) && (prev_op == -1) && (done == FALSE))
        {

          /*  do_mode_latch( 900);    */
          /* no need to put out 900 because Text Compress is default */

          done = TRUE;
          prev_op = TC;
        }

      if ((op == TC) && (prev_op == TC) && (done == FALSE))
        {

          do_mode_latch (900);
          done = TRUE;
          prev_op = TC;
        }

      if ((op == NC) && (prev_op == TC) && (done == FALSE))
        {

          do_mode_latch (902);
          done = TRUE;
          prev_op = NC;
        }

      if ((op == NC) && (prev_op == -1) && (done == FALSE))
        {

          do_mode_latch (902);
          done = TRUE;
          prev_op = NC;
        }

      if ((op == NC) && (prev_op == BC) && (done == FALSE))
        {

          do_mode_latch (902);
          done = TRUE;
          prev_op = NC;
        }

      if ((op == NC) && (prev_op == NC) && (done == FALSE))
        {


          done = TRUE;
          prev_op = NC;
        }

      if (opstring[0] != NULLCHAR)
        {
          switch (op)
            {

            case NC:
              nc (opstring);
              break;


            case BC:
              byte_compact (opstring);
              break;


            case TC:
              expand_string (opstring);
              text_compress (opstring);
              break;
            case MC:
              macro_compact (opstring);
              break;

            default:
              printf ("Unknown compaction mode");
              exit (1);

            }
        }


      if (linenum < 64)
        {
          strncpy (inline_str, raw_input[linenum], MAX_LINE);
          linechar = 0;
          linenum += 1;
        }
      else
        {
          printf ("Too many lines of raw input ; line number = %d \n",
                  linenum);
        }

      if (debug)
        {
          printf ("Next line = %s \n", inline_str);
        }

    }

  if (debug)
    {
      printf ("Leaving get_data \n");
    }

}

#ifdef DO_GIFS
#define OUTPUT_GIF  1
#endif /* DO_GIFS */
#define OUTPUT_PBM  2
#define OUTPUT_PS   3
#define OUTPUT_RAW  4
#define OUTPUT_PS_BITS 5


void
Usage_old (void)
{
  fprintf (stderr, "Usage_old: pdf417_enc [-t type] infile outfile [nrows] "
           "[ncols] [ec_level]\n");
#ifdef DO_GIFS
  fprintf (stderr, "\tValid types are: gif, pbm, ps, raw\n\n");
#else
  fprintf (stderr, "\tValid types are: pbm, ps, raw\n\n");
#endif /* DO_GIFS */
  fprintf (stderr, "\tDefault type is  ps\n\n");
}

void
pdf417_set_raw_line (int linenum, char *instr)
{
  if (linenum < 64)
    {
      if (strlen (instr) < RAW_INPUT_WIDTH)
        {
          strcpy (raw_input[linenum], instr);
        }
    }
}
void
pdf417_set_raw_count (int countin)
{

  if (countin < 64)
    {
      raw_lines = countin;
    }
}
void
pdf417_set_input_filename (char *instr)
{
  input_filename = strdup (instr);
}

void
pdf417_set_output_filename (char *instr)
{

  output_filename = strdup (instr);
}


//
// this is the routine that does all the work...
//
//  you can use this as a routine to call, without using main()
//   init_arrays must be called first!
//    output_type = (1=GIF, 2=PBM, 3=PS, 4=RAW, 5=PSBITS 6=EPS)
//    if use_default = TRUE, the rows = 24, cols = 8
//    output_filename must be initialized to the name of output file
//    if the output_filename is NULL string, then stdout will be used
//     ( this applies to no using main )
//    input_filename must be initialized to the name of the input file
//    If raw_in_mode is TRUE, then the raw_input array of strings
//    must be set up by the caller, as well as raw_lines = number of
//    of strings in the array.
//
//          Xwid parameter sets the module width,
//          the Ydim sets the row height and QZ sets the Quiet
//           Zone thickness       
void
pdf417_en_new(int in_rval, int in_cval, int in_ec_level, int output_type,
           int use_default, int raw_in_mode, int Xwid, int Ydim, int QZ)
{
  int datafound;
  int fill_count;
  int debug;
  int justbits;

  int j;
  int datalen;

  // do not open the file if in raw_in_mode
  // if not and raw_in_mode and cannot open file, then error exit
  //
  infile1 = NULL;

  debug = 0;

  if (debug)
    {
      printf
        ("in_cval = %d in_rval = %d in_ec_level = %d , output_type = %d \n",
         in_cval, in_rval, in_ec_level, output_type);
      printf ("use_default = %d raw_in_mode = %d \n", use_default,
              raw_in_mode);
    }

  if (input_filename    == NULL    )
    {
      infile1 = stdin;
    }
  else
    {
     if ((raw_in_mode == FALSE)
      && (infile1 = fopen (input_filename, "r")) == NULL)
      {

      fprintf (stderr, "Unable to open input file '%s': %s\n", input_filename,
               strerror (errno));
      exit (1);
     }
    }

  /* now do real ones */

  datafound = TRUE;
  mcb_start = -1;
  mcb_end = -1;
  debug = 0;
  eclen = 0;
  ec_level = in_ec_level;

  if (ec_level < 9)             /* all the ecc levels up to 8 */

    {
      eclen = 2 << ec_level;
    }
  else
    {
      printf ("Error correction levels over 8 not currently supported \n");
    }

  if (debug)
    {
      printf (" ec_level = %d eclen = %d \n", ec_level, eclen);
    }

  while (datafound)
    {
      codeindex = 0;
      put_back (0);             /* data length field, fill in when data read finished */
      /*  put_back(924);   byte compaction */


      if (raw_in_mode == FALSE)
        {
          get_data ();          /* read in the data from an input file */
        }
      else
        {
          get_data_raw (raw_lines); // read in the data from the raw_input
          // array
        }

      if (debug)
        {
          printf ("Returned from get_data \n");
        }

      datafound = FALSE;

      if (codeindex + eclen > 929)
        {
          printf ("Error: size of barcode too large \n");
          printf ("Number of codewords              = %d \n", codeindex);
          printf ("Number of error correction codes = %d \n", eclen);
          printf ("Total exceeds 929 \n");
          exit (1);
        }
      if (in_rval != 999)       // rows and columns already calculated
        {
          fill_count = ((in_cval - 2) * in_rval) - eclen - codeindex;
          number_of_rows = in_rval;
          number_of_columns = in_cval;
        }
      else                      // rows not calculated, in_rval = 999
        {
          number_of_columns = in_cval;
          if (in_cval != 2)
            {
              if ((eclen + codeindex) % (in_cval - 2) != 0)
                {

                  fill_count =
                    (in_cval - 2) - ((eclen + codeindex) % (in_cval - 2));
                  number_of_rows = ((eclen + codeindex) / (in_cval - 2)) + 1;;
                }
              else
                {
                  number_of_rows = (eclen + codeindex) / (in_cval - 2);
                  fill_count = 0;
                }
              if (number_of_rows > 90)
                {
                  printf (" data will not fit in 90 rows; must reduce\n");
                  printf (" input data or use more columns.\n");
		  exit (1);
                }
            }
          else
            {
              printf ("Must have more than two columns \n");
              exit (1);
            }
        }


      if (debug)
        {
          printf
            ("Fill count = %d col = %d row = %d eclen = %d codeindex = %d\n",
             fill_count, in_rval, in_cval, eclen, codeindex);
        }

      if (fill_count > 0)
        {
          for (j = 0; j < fill_count; j += 1)
            {
              put_back (900);
            }
        }
      else
        {
          if (fill_count < 0)
            {
              printf
                ("Message length plus Error Code Length exceed rows * ( cols -2) \n");
              printf
                ("Message length = %d  Error Code Length = %d Total = %d \n",
                 codeindex, eclen, codeindex + eclen);
              printf ("Rows = %d Columns -2 = %d  Message area = %d \n",
                      in_rval, in_cval - 2, in_rval * (in_cval - 2));

              printf ("Use larger row or column size \n");
              printf ("Or set rows = 999 for unlimited row size \n");
              exit (1);
            }
        }

      put_back_len (codeindex);

      datalen = codeindex;
      /*
       * move MCB (Macro Conrol Block), if any.
       * It should _follow_ pad codewords
       * Warning - if MCB is followed by any data,
       * this data will be lost!
       */
      if ((mcb_start) != -1 && (mcb_end != -1))
        {
          int i;
          for (i = mcb_end - 1; i >= mcb_start; i--)
            {
              codes[datalen - (mcb_end - i)] = codes[i];
              codes[i] = 900;   /* padding */
              data[datalen - (mcb_end - i)] = data[i];
              data[i] = 900;    /* padding */
            }
        }


      if (debug)
        {
          printf ("Generate EC data \n");
        }

      generateEC (data, datalen, eclen); /* do the error correction, 64  */

      if (use_default == TRUE)  // no rows or columns given on command line
        {
          number_of_rows = 24;
          number_of_columns = 8;
        }

      do_rows_columns (eclen);  /* take ec len as parameter */

      /* make_pdf(&out);        make the bits from the final rectangle */

      switch (output_type)
        {
#ifdef DO_GIFS
        case OUTPUT_GIF:
          PDF417_encodeGIF (output_filename, number_of_columns,
                            number_of_rows, 7, 2);
          if (debug)
            printf ("Return from encodeGIF\n");
          break;
#endif /* DO_GIFS */
        case OUTPUT_PBM:
          PDF417_encodePBM (output_filename, number_of_columns,
                            number_of_rows, 7, 2);
          if (debug)
            printf ("Return from encodePBM\n");
          break;
        case OUTPUT_RAW:
          if (debug)
            printf ("Calling make_pdf_raw\n");
          make_pdf_raw (&out);
          if (debug)
            printf ("Return from make_pdf_raw\n");
          break;
        case OUTPUT_PS_BITS:
          if (debug)
            printf ("Calling make_pdf_ps\n");
          justbits = TRUE;
          make_pdf_ps_dim (output_filename, &out, justbits, output_type,
		       Xwid, Ydim, QZ);
          if (debug)
            printf ("Return from make_pdf_ps\n");
          break;
        case OUTPUT_PS:
        case OUTPUT_EPS:
        default:
          justbits = FALSE;
          if (debug)
            printf ("Calling make_pdf_ps_dim\n");
          make_pdf_ps_dim(output_filename, &out, justbits, output_type,
		       Xwid, Ydim, QZ);
          if (debug)
            printf ("Return from make_pdf_ps\n");
          break;

        }
    }

  if (infile1)
    {
      fclose (infile1);
    }

}

//
// this is the routine that does all the work...
//
//  you can use this as a routine to call, without using main()
//   init_arrays must be called first!
//    output_type = (1=GIF, 2=PBM, 3=PS, 4=RAW, 5=PSBITS )
//    if use_default = TRUE, the rows = 24, cols = 8
//    output_filename must be initialized to the name of output file
//    if the output_filename is NULL string, then stdout will be used
//     ( this applies to no using main )
//    input_filename must be initialized to the name of the input file
//    If raw_in_mode is TRUE, then the raw_input array of strings
//    must be set up by the caller, as well as raw_lines = number of
//    of strings in the array.

void
pdf417_en (int in_rval, int in_cval, int in_ec_level, int output_type,
           int use_default, int raw_in_mode)
{
  int datafound;
  int fill_count;
  int debug;
  int justbits;

  int j;
  int datalen;

  // do not open the file if in raw_in_mode
  // if not and raw_in_mode and cannot open file, then error exit
  //
  infile1 = NULL;

  debug = 0;

  if (debug)
    {
      printf
        ("in_cval = %d in_rval = %d in_ec_level = %d , output_type = %d \n",
         in_cval, in_rval, in_ec_level, output_type);
      printf ("use_default = %d raw_in_mode = %d \n", use_default,
              raw_in_mode);
    }

  if (input_filename    == NULL    )
    {
      infile1 = stdin;
    }
  else
    {
     if ((raw_in_mode == FALSE)
      && (infile1 = fopen (input_filename, "r")) == NULL)
      {

      fprintf (stderr, "Unable to open input file '%s': %s\n", input_filename,
               strerror (errno));
      exit (1);
     }
    }

  /* now do real ones */

  datafound = TRUE;
  mcb_start = -1;
  mcb_end = -1;
  debug = 0;
  eclen = 0;
  ec_level = in_ec_level;

  if (ec_level < 9)             /* all the ecc levels up to 8 */

    {
      eclen = 2 << ec_level;
    }
  else
    {
      printf ("Error correction levels over 8 not currently supported \n");
    }

  if (debug)
    {
      printf (" ec_level = %d eclen = %d \n", ec_level, eclen);
    }

  while (datafound)
    {
      codeindex = 0;
      put_back (0);             /* data length field, fill in when data read finished */
      /*  put_back(924);   byte compaction */


      if (raw_in_mode == FALSE)
        {
          get_data ();          /* read in the data from an input file */
        }
      else
        {
          get_data_raw (raw_lines); // read in the data from the raw_input
          // array
        }

      if (debug)
        {
          printf ("Returned from get_data \n");
        }

      datafound = FALSE;

      if (codeindex + eclen > 929)
        {
          printf ("Error: size of barcode too large \n");
          printf ("Number of codewords              = %d \n", codeindex);
          printf ("Number of error correction codes = %d \n", eclen);
          printf ("Total exceeds 929 \n");
          exit (0);
        }
      if (in_rval != 999)       // rows and columns already calculated
        {
          fill_count = ((in_cval - 2) * in_rval) - eclen - codeindex;
          number_of_rows = in_rval;
          number_of_columns = in_cval;
        }
      else                      // rows not calculated, in_rval = 999
        {
          number_of_columns = in_cval;
          if (in_cval != 2)
            {
              if ((eclen + codeindex) % (in_cval - 2) != 0)
                {

                  fill_count =
                    (in_cval - 2) - ((eclen + codeindex) % (in_cval - 2));
                  number_of_rows = ((eclen + codeindex) / (in_cval - 2)) + 1;;
                }
              else
                {
                  number_of_rows = (eclen + codeindex) / (in_cval - 2);
                  fill_count = 0;
                }
              if (number_of_rows > 90)
                {
                  printf (" data will not fit in less than 90 rows\n");
                  printf (" must reduce input data or use more columns\n");
                }
            }
          else
            {
              printf ("Must have more than two columns \n");
              exit (0);
            }
        }


      if (debug)
        {
          printf
            ("Fill count = %d col = %d row = %d eclen = %d codeindex = %d\n",
             fill_count, in_rval, in_cval, eclen, codeindex);
        }

      if (fill_count > 0)
        {
          for (j = 0; j < fill_count; j += 1)
            {
              put_back (900);
            }
        }
      else
        {
          if (fill_count < 0)
            {
              printf
                ("Message length plus Error Code Length exceed rows * ( cols -2) \n");
              printf
                ("Message length = %d  Error Code Length = %d Total = %d \n",
                 codeindex, eclen, codeindex + eclen);
              printf ("Rows = %d Columns -2 = %d  Message area = %d \n",
                      in_rval, in_cval - 2, in_rval * (in_cval - 2));

              printf ("Use larger row or column size \n");
              printf ("Or set rows = 999 for unlimited row size \n");
              exit (0);
            }
        }

      put_back_len (codeindex);

      datalen = codeindex;
      /*
       * move MCB (Macro Conrol Block), if any.
       * It should _follow_ pad codewords
       * Warning - if MCB is followed by any data,
       * this data will be lost!
       */
      if ((mcb_start) != -1 && (mcb_end != -1))
        {
          int i;
          for (i = mcb_end - 1; i >= mcb_start; i--)
            {
              codes[datalen - (mcb_end - i)] = codes[i];
              codes[i] = 900;   /* padding */
              data[datalen - (mcb_end - i)] = data[i];
              data[i] = 900;    /* padding */
            }
        }

      if (debug)
        {
          printf ("Generate EC data \n");
        }

      generateEC (data, datalen, eclen); /* do the error correction, 64  */

      if (use_default == TRUE)  // no rows or columns given on command line
        {
          number_of_rows = 24;
          number_of_columns = 8;
        }

      do_rows_columns (eclen);  /* take ec len as parameter */

      /* make_pdf(&out);        make the bits from the final rectangle */

      switch (output_type)
        {
#ifdef DO_GIFS
        case OUTPUT_GIF:
          PDF417_encodeGIF (output_filename, number_of_columns,
                            number_of_rows, 7, 2);
          if (debug)
            printf ("Return from encodeGIF\n");
          break;
#endif /* DO_GIFS */
        case OUTPUT_PBM:
          PDF417_encodePBM (output_filename, number_of_columns,
                            number_of_rows, 7, 2);
          if (debug)
            printf ("Return from encodePBM\n");
          break;
        case OUTPUT_RAW:
          if (debug)
            printf ("Calling make_pdf_raw\n");
          make_pdf_raw (&out);
          if (debug)
            printf ("Return from make_pdf_raw\n");
          break;
        case OUTPUT_PS_BITS:
          if (debug)
            printf ("Calling make_pdf_ps\n");
          justbits = TRUE;
          make_pdf_ps (output_filename, &out, justbits);
          if (debug)
            printf ("Return from make_pdf_ps\n");
          break;
        case OUTPUT_EPS:
          justbits = FALSE;
          if (debug)
            printf ("Calling make_pdf_ps_dim\n");
          make_pdf_ps_dim(output_filename, &out, justbits,OUTPUT_EPS,
                                             20, 5, 2 );
          if (debug)
            printf ("Return from make_pdf_ps\n");
        case OUTPUT_PS:
        default:
          justbits = FALSE;
          if (debug)
            printf ("Calling make_pdf_ps\n");
          make_pdf_ps (output_filename, &out, justbits);
          if (debug)
            printf ("Return from make_pdf_ps\n");
          break;

        }
    }

  if (infile1)
    {
      fclose (infile1);
    }

}

// initialize arrays
//

void
pdf417_init_arrays ()
{

  quotchar = 34;

  coeff_init_32 ();             /* set up ecc constants */

  powers_init ();               /* init 3**i table for syndrome calculation */

}
