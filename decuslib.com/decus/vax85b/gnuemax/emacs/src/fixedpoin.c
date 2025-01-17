/* Cost calculations for insert/delete operations on terminal. 
   Copyright (C) 1984 Richard M. Stallman. 
   Based mostly on public domain code written by Chris Torek. 
 
This file is part of GNU Emacs. 
 
GNU Emacs is distributed in the hope that it will be useful, 
but without any warranty.  No author or distributor 
accepts responsibility to anyone for the consequences of using it 
or for whether it serves any particular purpose or works at all, 
unless he says so in writing. 
 
Everyone is granted permission to copy, modify and redistribute 
GNU Emacs, but only under the conditions described in the 
document "GNU Emacs copying permission notice".   An exact copy 
of the document is supposed to have been given to you along with 
GNU Emacs so that you can know how you may redistribute it all. 
It should be in a file named COPYING.  Among other things, the 
copyright notice and this notice must be preserved on all copies.  */ 
 
 
#include "config.h" 
#include "display.h" 
#ifdef DisplayDebug 
#include <stdio.h> 
#endif 
 
/* Calculate the insert and delete line costs. 
 
   We keep the ID costs in a precomputed array based on the position 
   at which the I or D is performed.  Also, there are two kinds of ID 
   costs: the "once-only" and the "repeated".  This is to handle both 
   those terminals that are able to insert N lines at a time (once- 
   only) and those that must repeatedly insert one line. 
 
   The cost to insert N lines at line L is 
   	    [tt.t_ILov  + (ScreenLength + 1 - L) * tt.t_ILpf] + 
	N * [tt.t_ILnov + (ScreenLength + 1 - L) * tt.t_ILnpf] 
 
   ILov represents the basic insert line overhead.  ILpf is the padding 
   required to allow the terminal time to move a line: insertion at line 
   L changes (ScreenLength + 1 - L) lines. 
 
   The first bracketed expression above is the overhead; the second is 
   the multiply factor.  Both are dependent only on the position at 
   which the insert is performed.  We store the overhead in ILcost and 
   the multiply factor in ILncost.  Note however that any insertion 
   must include at least one multiply factor.  Rather than compute this 
   as ILcost[line]+ILncost[line], we add ILncost into ILcost.  This is 
   reasonable because of the particular algorithm used in calcM. 
 
   Deletion is essentially the same as insertion. 
 */ 
 
CalcIDCosts () 
{ 
    register fixedpoint *ip; 
    register int i; 
    register fixedpoint ic; 
     
    CalcLID (tt.t_ILov, tt.t_ILpf, tt.t_ILnov, tt.t_ILnpf, ILcost, ILncost); 
    CalcLID (tt.t_DLov, tt.t_DLpf, tt.t_DLnov, tt.t_DLnpf, DLcost, DLncost); 
    ic = ILcost[0]; 
    ip = &ILinit[0]; 
    for (i = 1; i <= ScreenLength; i++) { 
	*ip++ = ic; 
	ic += ILncost[0]; 
    } 
    IDValid++; 
} 
 
/* Calculate the line ID overhead and multiply factor values */ 
CalcLID (ov1, pf1, ovn, pfn, ov, mf) 
     int ov1, ovn; 
     double pf1, pfn; 
     register fixedpoint *ov, *mf; 
{ 
    register int i; 
 
    if (ov1 == MissingFeature) { 
	for (i = ScreenLength; --i >= 0;) 
	    *ov++ = *mf++ = itofixp (MissingFeature); 
    } 
    else { 
	register fixedpoint fpf1 = fltofixp (pf1), 
			    fpfn = fltofixp (pfn); 
	register fixedpoint fov1 = itofixp (ov1), 
			    fovn = itofixp (ovn); 
 
    /* This could be done without multiplies at all, but what the heck. */ 
	for (i = ScreenLength; i > 0; i--) 
	    *ov++ = fov1 + i * fpf1 + (*mf++ = fovn + i * fpfn); 
    } 
} 
 
/* Character ID costs are also precomputed, but are kept in a linear 
   array with the zero point in the middle.  Thus the cost to change 'o' 
   characters to 'n' characters is just 
	DC_ICcost[n - o + MScreenWidth] 
   which we can get more simply as DCICcost[n - o].  */ 
 
CalcDCIC () 
{ 
  register int i; 
  register fixedpoint *p, c, fmf; 
 
  /* Delete costs are at negative offsets */ 
  p = &DCICcost[0]; 
  if (tt.t_DCov == MissingFeature) 
    { 
      for (i = ScreenWidth; --i >= 0;) 
	*--p = itofixp (MissingFeature); 
    } 
  else 
    { 
      c = itofixp (tt.t_DCov); 
      fmf = fltofixp (tt.t_DCmf); 
      for (i = ScreenWidth; --i >= 0;) 
	*--p = (c += fmf); 
    } 
 
  /* Doing nothing is free */ 
  p = &DCICcost[0]; 
  *p++ = itofixp (0); 
 
  /* Insert costs are at positive offsets */ 
  if (tt.t_ICov == MissingFeature) 
    { 
      for (i = ScreenWidth; --i >= 0;) 
	*p++ = itofixp (MissingFeature); 
    } 
  else 
    { 
      c = itofixp (tt.t_ICov); 
      fmf = fltofixp (tt.t_ICmf); 
      for (i = ScreenWidth; --i >= 0;) 
	*p++ = (c += fmf); 
    } 
  DCICValid++; 
#ifdef DisplayDebug 
  if (RDdebug >= 2) 
    PrintDCICCosts (); 
#endif DisplayDebug 
  } 
 
#ifdef DisplayDebug 
 
/* print out the insert/delete cost matrix */ 
PrintIDCosts () { 
    register int i; 
    fprintf (stderr, "\nILcost:\n"); 
    for (i = 0; i < ScreenLength; i++) 
	fprintf (stderr, "%2.2f ", fixptofl (ILcost[i])); 
    fprintf (stderr, "\nDLcost:\n"); 
    for (i = 0; i < ScreenLength; i++) 
	fprintf (stderr, "%2.2f ", fixptofl (DLcost[i])); 
    fprintf (stderr, "\nILncost:\n"); 
    for (i = 0; i < ScreenLength; i++) 
	fprintf (stderr, "%2.2f ", fixptofl (ILncost[i])); 
    fprintf (stderr, "\nDLncost:\n"); 
    for (i = 0; i < ScreenLength; i++) 
	fprintf (stderr, "%2.2f ", fixptofl (DLncost[i])); 
    fprintf (stderr, "\nILinit:\n"); 
    for (i = 0; i < ScreenLength; i++) 
	fprintf (stderr, "%2.2f ", fixptofl (ILinit[i])); 
    putc ('\n', stderr); 
    fflush (stderr); 
} 
 
PrintDCICCosts () { 
    register int i; 
    fprintf (stderr, "\nDCcost:\n"); 
    for (i = 1; i <= ScreenWidth; i++) 
	fprintf (stderr, "%2.2f ", fixptofl (DCICcost[-i])); 
    fprintf (stderr, "\nICcost:\n"); 
    for (i = 1; i <= ScreenWidth; i++) 
	fprintf (stderr, "%2.2f ", fixptofl (DCICcost[i])); 
    putc ('\n', stderr); 
    fflush (stderr); 
} 
 
#endif DisplayDebug 
