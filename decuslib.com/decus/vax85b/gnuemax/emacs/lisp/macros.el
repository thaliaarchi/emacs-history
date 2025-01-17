;; Non-primitive commands for keyboard macros. 
;; Copyright (C) 1985 Richard M. Stallman. 
 
;; This file is part of GNU Emacs. 
 
;; GNU Emacs is distributed in the hope that it will be useful, 
;; but without any warranty.  No author or distributor 
;; accepts responsibility to anyone for the consequences of using it 
;; or for whether it serves any particular purpose or works at all, 
;; unless he says so in writing. 
 
;; Everyone is granted permission to copy, modify and redistribute 
;; GNU Emacs, but only under the conditions described in the 
;; document "GNU Emacs copying permission notice".   An exact copy 
;; of the document is supposed to have been given to you along with 
;; GNU Emacs so that you can know how you may redistribute it all. 
;; It should be in a file named COPYING.  Among other things, the 
;; copyright notice and this notice must be preserved on all copies. 
 
 
(defun append-kbd-macro (macroname filename &optional keys) 
  "Append kbd macro NAME in file FILE, as Lisp code to define the macro. 
Use  load  to load the file. 
Third argument KEYS non-nil means also record the keys it is on. 
 (This is the prefix argument, when calling interactively.)" 
  (interactive "CAppend kbd macro (name): \nFAppend kbd macro %s to file: \nP") 
  (write-kbd-macro macroname filename keys t)) 
 
(defun write-kbd-macro (macroname filename &optional keys appendflag) 
  "Save kbd macro NAME in file FILE, as Lisp code to define the macro. 
Use  load  to load the file. 
Third argument KEYS non-nil means also record the keys it is on. 
 (This is the prefix argument, when calling interactively.) 
Fourth argument APPENDFLAG non-nil meams append to FILE's existing contents." 
  (interactive "CWrite kbd macro (name): \nFWrite kbd macro %s to file: \nP") 
  (let ((buffer (get-buffer-create " write-kbd-macro-temp"))) 
    (save-excursion 
     (set-buffer buffer) 
     (insert "(fset '") 
     (prin1 macroname buffer) 
     (insert "\n   ") 
     (prin1 (symbol-function macroname) buffer) 
     (insert ")\n") 
     (let ((keys (where-is-internal macroname))) 
       (while keys 
	 (insert "(global-set-key ") 
	 (prin1 keys buffer) 
	 (insert " '") 
	 (prin1 macroname buffer) 
	 (insert ")\n") 
	 (setq keys (cdr keys)))) 
     (write-region (dot-min) (dot-max) filename appendflag)))) 
 
(defun kbd-macro-query (flag) 
  "Query user during kbd macro execution. 
With prefix argument, enters recursive edit, 
 reading keyboard commands even within a kbd macro. 
 You can give different commands each time the macro executes. 
Without prefix argument, reads a character.  Your options are: 
 Space -- execute the rest of the macro. 
 Delete -- skip the rest of the macro; start next repetition. 
 C-d -- skip rest of the macro and don't repeat it any more. 
 C-r -- enter a recursive edit, then on exit ask again for a character 
 C-l -- redisplay screen and ask again." 
  (interactive "P") 
  (if flag 
      (let (executing-macro defining-kbd-macro) 
	(recursive-edit)) 
    (let ((loop t)) 
      (while loop 
	(message "Proceed with macro? (Space, Delete, C-d, C-r or C-l) ") 
	(let ((char (let (executing-macro defining-kbd-macro) (read-char)))) 
	  (cond ((= char ? ) 
		 (setq loop nil)) 
		((= char ?\177) 
		 (setq loop nil) 
		 (setq executing-macro "")) 
		((= char ?\^d) 
		 (setq loop nil) 
		 (setq executing-macro t)) 
		((= char ?^\l) 
		 (redraw-screen)) 
		((= char ?\^r) 
		 (let (executing-macro defining-kbd-macro) 
		   (recursive-edit))))))))) 
