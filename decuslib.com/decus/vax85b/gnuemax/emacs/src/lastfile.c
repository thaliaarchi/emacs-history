/* Mark end of data space to dump as pure, for GNU Emacs. 
   Copyright (C) 1985 Richard M. Stallman. 
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
/* How this works: 
 
 Fdump_emacs dumps everything up to my_edata as text space (pure). 
 
 The files of Emacs are written so as to have no initialized 
 data that can ever need to be altered except at the first startup. 
 This is so that those words can be dumped as sharable text. 
 
 It is not possible to exercise such control over library files. 
 So it is necessary to refrain from making their data areas shared. 
 Therefore, this file is loaded following all the files of Emacs 
 but before library files. 
 As a result, the symbol my_edata indicates the point 
 in data space between data coming from Emacs and data 
 coming from libraries. 
*/ 
 
int my_edata = 0; 
