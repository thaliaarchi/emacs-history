;; Fill commands for Emacs.
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
(defun set-fill-prefix () 
  "Set the fill-prefix to the current line up to dot. 
Filling expects lines to start with the fill prefix 
and reinserts the fill prefix in each resulting line." 
  (interactive) 
  (setq fill-prefix (buffer-substring 
		     (save-excursion (beginning-of-line) (dot)) 
		     (dot))) 
  (if (equal fill-prefix "") 
      (setq fill-prefix nil))) 
 
(defun fill-region-as-paragraph (from to &optional justify-flag) 
  "Fill region as one paragraph: break lines to fit fill-column. 
Prefix arg means justify too. 
From program, pass args FROM, TO and JUSTIFY-FLAG." 
  (interactive "r\nP") 
  (save-restriction 
    (narrow-to-region from to) 
    (goto-char (dot-min)) 
    (skip-chars-forward "\n") 
    (narrow-to-region (dot) (dot-max)) 
    (goto-char (dot-min)) 
    (while (re-search-forward "   *" nil t) 
      (delete-region 
       (+ (match-beginning 0) 
	  (if (save-excursion 
	       (skip-chars-backward " ])\"'") 
	       (memq (preceding-char) '(?. ?? ?!))) 
	      2 1)) 
       (match-end 0))) 
    (goto-char (dot-min)) 
    (while (re-search-forward "[.?!][])""']*\n" nil t) 
      (forward-char -1) 
      (insert ? )) 
    (and fill-prefix (not (equal fill-prefix "")) 
	 (let ((fpre (regexp-quote fill-prefix))) 
	   (goto-char (dot-min)) 
	   (forward-line 1) 
	   (while (not (eobp)) 
	     (if (looking-at fpre) 
		 (delete-region (dot) (match-end 0))) 
	     (forward-line 1)))) 
    (subst-char-in-region (dot-min) (dot-max) ?\n ? ) 
    (goto-char (dot-max)) 
    (delete-horizontal-space) 
    (goto-char (dot-min)) 
    (while (not (eobp)) 
      (move-to-column fill-column) 
      (if (eobp) 
	  nil 
	(skip-chars-backward "^ \n") 
	(if (bolp) 
	    (skip-chars-forward "^ \n") 
	  (forward-char -1)) 
	(delete-horizontal-space)) 
      (insert ?\n) 
      (and (not (eobp)) fill-prefix (not (equal fill-prefix "")) 
	   (insert fill-prefix)) 
      (and justify-flag (not (eobp)) 
	   (progn 
	     (forward-line -1) 
	     (justify-current-line) 
	     (forward-line 1)))))) 
 
(defun fill-paragraph (arg) 
  "Fill paragraph at or after dot. 
Prefix arg means justify as well." 
  (interactive "P") 
  (save-excursion 
    (forward-paragraph) 
    (or (bolp) (newline 1)) 
    (let ((end (dot))) 
      (backward-paragraph) 
      (fill-region-as-paragraph (dot) end arg)))) 
 
(defun fill-region (from to &optional justify-flag) 
  "Fill each of the paragraphs in the region. 
Prefix arg (non-nil third arg, if called from program) 
means justify as well." 
  (interactive "r") 
  (save-restriction 
   (narrow-to-region from to) 
   (goto-char (dot-min)) 
   (while (not (eobp)) 
     (let ((initial (dot)) 
	   (end (progn 
		 (forward-paragraph 1) (dot)))) 
       (forward-paragraph -1) 
       (if (>= (dot) initial) 
	   (fill-region-as-paragraph (dot) end justify-flag) 
	 (goto-char end)))))) 
 
(defun justify-current-line () 
  "Add spaces to line dot is in, so it ends at fill-column." 
  (interactive) 
  (save-excursion 
   (save-restriction 
    (let (ncols beg) 
      (beginning-of-line) 
      (skip-chars-forward " \t") 
      (setq beg (dot)) 
      (end-of-line) 
      (narrow-to-region beg (dot)) 
      (goto-char beg) 
      (while (re-search-forward "   *" nil t) 
	(delete-region 
	 (+ (match-beginning 0) 
	    (if (save-excursion 
		 (skip-chars-backward " ])\"'") 
		 (memq (preceding-char) '(?. ?? ?!))) 
		2 1)) 
	 (match-end 0))) 
      (goto-char beg) 
      (while (re-search-forward "[.?!][])""']*\n" nil t) 
	(forward-char -1) 
	(insert ? )) 
      (goto-char (dot-max)) 
      (setq ncols (- fill-column (current-column))) 
      (if (scan-buffer (dot-min) 1 ? ) 
	  (while (> ncols 0) 
	    (let ((nmove (+ 3 (% (random) 3)))) 
	      (while (> nmove 0) 
		(or (search-backward " " nil t) 
		    (progn 
		     (goto-char (dot-max)) 
		     (search-backward " "))) 
		(skip-chars-backward " ") 
		(setq nmove (1- nmove)))) 
	    (insert " ") 
	    (skip-chars-backward " ") 
	    (setq ncols (1- ncols)))))))) 
