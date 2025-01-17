; This file is for use by TeX82 (see man page) to allow switching to 
;  Emacs at a line number given on the command line 
; It assumes that it has been called by: 
;	emacs -ltex-start -estartline linenumber file 
 
(defun startline () 
  (find-file (car (cdr args))) 
  (goto-char (dot-min)) 
  (forward-line (1- (car args))) 
  (setq args nil)) 
