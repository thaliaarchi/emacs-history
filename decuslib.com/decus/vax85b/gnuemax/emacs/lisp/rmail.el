;; "rmail" mail reader for Emacs. 
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
(defconst rmail-dont-reply-to-names 
  (concat (getenv "USER")) 
  "List of names to prune of reply to messages") 
 
(defconst rmail-ignored-headers 
   "^via:\\|^mail-from:\\|^origin:\\|^status:\\|^remailed\\|^received:\\|^message-id:\\|^summary-line:" 
   "") 
 
(defconst rmail-file-name 
  (expand-file-name "~/RMAIL") 
  "") 
 
(defvar rmail-mode-map nil "") 
 
(defconst rmail-inbox-list  
  '("/usr/spool/mail/$USER" "~/mbox") 
 "") 
 
(defconst rmail-message-filter nil "") 
 
(defvar rmail-summary-buffer nil "") 
(defvar rmail-summary-list nil "") 
 
(defvar rmail-buffer-save nil "Name of the buffer that RMAIL was invoked from") 
 
(defvar rmail-current-message-number 0 "Displayed message's number") 
(defvar rmail-total-messages 0 "Total number of message in RMAIL file") 
 
(defun rmail-display-attributes () 
  "creates a minor mode list that has message number, total messages, 
and attribute for current message." 
  (setq minor-modes (list (cons 'foo 
				(concat rmail-current-message-number 
					"/" 
					rmail-total-messages 
					(rmail-get-attribute-string)))))) 
 
(defun rmail-get-attribute-string () 
  "Creates a string of the current message attributes. 
We only get deleted or answered attributes for now" 
  (let ((dot-min-save (dot-min)) 
	(dot-max-save (dot-max)) 
	pos start) 
    (widen) 
    (re-search-backward "^" 1 t) 
    (setq start (dot)) 
    (re-search-forward ",," dot-max-save t) 
    (setq pos (dot)) 
    (setq deleted (re-search-backward "deleted" start t)) 
    (goto-char pos) 
    (setq answered (re-search-backward "answered" start t)) 
    (narrow-to-region dot-min-save dot-max-save) 
    (goto-char dot-min-save) 
    (concat (if deleted ",Deleted" "") 
	    (if answered ",Answered" "")))) 
 
(defun rmail-set-message-counters () 
  "Scan through the RMAIL buffer to figure out how many messages 
are there. Set counters for use with minor mode display." 
  (save-restriction 
   (save-excursion 
    (let ((dot-save (dot))) 
      (widen) 
      (goto-char (dot-min)) 
      (setq rmail-total-messages 0) 
      (while (search-forward "" dot-save t) 
	(if (save-excursion (forward-char -2) (bolp)) 
	    (setq rmail-total-messages (1+ rmail-total-messages)))) 
      (setq rmail-current-message-number rmail-total-messages) 
      (while (search-forward "" nil t) 
	(if (save-excursion (forward-char -2) (bolp)) 
	    (setq rmail-total-messages (1+ rmail-total-messages)))))))) 
 
(defun rmail (&optional file-name-arg) 
  "Read and edit incoming mail. 
Moves messages into ~/RMAIL (a file in babyl format) 
 and edits that file in Rmail Mode. 
Type Help m once editing that file, for a list of Rmail commands." 
  (interactive) 
  (let* ((last-buffer (buffer-name)) 
	 (file-name (expand-file-name (or file-name-arg rmail-file-name))) 
	 (existed (get-file-buffer file-name))) 
    (find-file file-name) 
    (rmail-mode) 
    (setq rmail-buffer-save last-buffer) 
    (if existed 
	(rmail-get-new-mail) 
      (if (= 0 (buffer-size)) 
	  (rmail-insert-rmail-file-header)) 
      (goto-char (dot-max)) 
      (if file-name-arg 
	  (rmail-show-message)) 
      (rmail-get-new-mail)))) 
 
(defun rmail-insert-rmail-file-header () 
  (insert "Version: 5 
Note:   This is the header of an rmail file. 
Note:   If you are seeing it in rmail, 
Note:    it means the file has no messages in it.\n\^_")) 
 
(defun rmail-output-to-rmail-file (file-name) 
  "Append the current message to an RMAIL file. If the file does not 
exist, ask if it should be created." 
  (interactive "FAppend message to RMAIL file: ") 
  (save-excursion 
   (let ((eom (dot-max)) 
	 (bom (dot-min)) 
	 (buffer (current-buffer)) 
	 tom create) 
     (if (not (file-exists-p file-name)) 
	  (if (yes-or-no-p 
	       (concat "\"" file-name "\" does not exist, create it? ")) 
	      (progn 
	       (setq file-buffer (create-file-buffer file-name)) 
	       (set-buffer file-buffer) 
	       (rmail-insert-rmail-file-header) 
	       (write-region (dot-min) (dot-max) file-name t 1) 
	       (set-buffer buffer) 
	       (kill-buffer file-buffer)))) 
     (if (file-exists-p file-name) 
	 (progn 
	  (widen) 
	  (re-search-backward "^" nil t) 
	  (setq start (dot)) 
	  (goto-char (+ (dot) 2)) 
	  (re-search-forward "^" nil t) 
	  (append-to-file (+ start 1) (dot) file-name) 
	  (narrow-to-region bom eom)))))) 
 
(defun rmail-get-new-mail () 
  "Move any new mail from /usr/spool/mail or mbox into this Rmail file." 
  (interactive) 
  (rmail-forget-summary) 
  (widen) 
  (let ((opos (dot))) 
    (goto-char (dot-max)) 
    (save-restriction 
     (narrow-to-region (dot) (dot)) 
     (let ((files rmail-inbox-list) 
	   file tofile) 
       (while files 
	 (setq file (substitute-in-file-name (car files)) 
	       tofile (concat file "~")) 
	 (if (file-exists-p tofile) 
	     (insert-file-contents tofile) 
	   (if (file-exists-p file) 
	       (progn 
		(rename-file file tofile) 
		(insert-file-contents tofile)))) 
	 (setq files (cdr files)))) 
     (rmail-convert-to-babyl-format) 
     (save-buffer) 
     (let ((files rmail-inbox-list)) 
       (while files 
	 (condition-case () 
	     (delete-file (concat (substitute-in-file-name (car files)) "~")) 
	   (error nil)) 
	 (setq files (cdr files)))) 
     (goto-char (dot-min)) 
     (forward-line 1)) 
    (if (eobp) 
	(progn 
	 (goto-char opos) 
	 (message "(No new mail has arrived)"))) 
    (rmail-set-message-counters) 
    (rmail-show-message t))) 
 
(defun convert-unix-to-arpa () 
  (save-excursion 
   (save-restriction 
    (let ((start (dot)) 
	  (end (condition-case err 
			       (progn (search-forward "\n\n") (dot)) 
		 (error (message "can't find EOH") nil))) 
	  has-from has-date) 
      (cond (end 
	     (narrow-to-region start end) 
	     (goto-char start) 
	     (setq has-from (search-forward "\nFrom:" nil t)) 
	     (goto-char start) 
	     (setq has-date (search-forward "\nDate:" nil t)) 
	     (cond ((and (not has-from) has-date) 
		    ;; kill "date:" line if "from:" line missing 
		    (goto-char start) 
		    (search-forward "\nDate:") 
		    (beginning-of-line) 
		    (delete-region (dot) 
				   (progn (forward-line 2) (dot))))) 
	     (goto-char start) 
	     (if (re-search-forward 
	      "^From \\([^ ]*\\)\\( \\|  \\)\\([^ ]*\\) \\([^ ]*\\) *\\([0-9]*\\) \\([0-9:]*\\) 19\\([0-9]*\\)\n" nil t) 
		 (cond ((not has-from) 
			(replace-match 
			 "Date: \\3, \\5 \\4 \\7 \\6 est\nFrom: \\1\n")) 
		       (t 
			(replace-match "")))))))))) 
 
(defun rmail-convert-to-babyl-format () 
  (while (not (eobp)) 
    (insert "\^L\n0,,\n") 
;    (delete-region (dot) (progn (forward-line 1) (dot))) 
    (let ((start (dot))) 
      (convert-unix-to-arpa) 
      (if (re-search-forward 
	   "\n\nFrom [^ ]*\\( \\|  \\)[^ ]* [^ ]* [ :0-9]*$" nil t) 
	  (beginning-of-line) 
	(goto-char (dot-max))) 
      (save-excursion 
       (save-restriction 
	(narrow-to-region start (dot)) 
	(goto-char (dot-min)) 
	(while (search-forward "" nil t) 
	  (forward-char -1) 
	  (if (bolp) 
	      (replace-match "^_")		;^ and _ 
	    (forward-char 1)))))) 
    (insert ?\^_))) 
 
(if rmail-mode-map 
    nil 
  (setq rmail-mode-map (make-keymap)) 
  (suppress-keymap rmail-mode-map) 
  (define-key rmail-mode-map "." 'beginning-of-buffer) 
  (define-key rmail-mode-map " " 'scroll-up) 
  (define-key rmail-mode-map "\177" 'scroll-down) 
  (define-key rmail-mode-map "n" 'rmail-next-message) 
  (define-key rmail-mode-map "p" 'rmail-previous-message) 
  (define-key rmail-mode-map "\en" 'rmail-next-all) 
  (define-key rmail-mode-map "\ep" 'rmail-previous-all) 
  (define-key rmail-mode-map "d" 'rmail-delete-forward) 
  (define-key rmail-mode-map "u" 'rmail-undelete-previous-message) 
  (define-key rmail-mode-map "\eu" 'rmail-undelete-message) 
  (define-key rmail-mode-map "e" 'rmail-expunge) 
  (define-key rmail-mode-map "s" 'save-buffer) 
  (define-key rmail-mode-map "g" 'rmail-get-new-mail) 
  (define-key rmail-mode-map "h" 'rmail-summary) 
  (define-key rmail-mode-map "m" 'mail-other-window) 
  (define-key rmail-mode-map "r" 'rmail-reply) 
  (define-key rmail-mode-map "c" 'rmail-continue) 
  (define-key rmail-mode-map "f" 'rmail-forward) 
  (define-key rmail-mode-map "F" 'rmail-find) 
  (define-key rmail-mode-map "j" 'rmail-goto-message) 
  (define-key rmail-mode-map "o" 'rmail-output-to-rmail-file) 
  (define-key rmail-mode-map "\^o" 'rmail-output) 
  (define-key rmail-mode-map "i" 'rmail-input) 
  (define-key rmail-mode-map "q" 'rmail-quit) 
  (define-key rmail-mode-map "?" 'describe-mode) 
  (define-key rmail-mode-map "\^D" 'rmail-delete-backward)) 
 
(defun rmail-mode () 
  "Rmail Mode is used by M-x rmail for editing Rmail files. 
All normal editing commands are turned off. 
Instead, these commands are available: 
 
.	Move dot to front of this message (same as Meta-<). 
Space	Scroll to next screen of this message. 
Delete  Scroll to previous screen of this message. 
n	Move to Next non-deleted message. 
p	Move to Previous non-deleted message. 
Meta-n	Move to Next message whether deleted or not. 
Meta-p	Move to Previous message whether deleted or not. 
F	Search for string and show message it is found in. 
j	Jump to message specified by numeric position in file. 
d	Delete this message, move to next nondeleted. 
C-d	Delete this message, move to previous nondeleted. 
u	Undelete previous message.  Meta-u undelete this message. 
e	Expunge deleted messages. 
s	Save the file (same as C-x C-s). 
g	Move new mail from /usr/spool/mail or mbox into this file. 
m	Mail a message.  Same as C-x 4 m. 
c	Continue composing outgoing message started before. 
r	Reply to this message.  Like m but initializes some fields. 
f	Forward this message to another user. 
o       Output this message to an RMAIL file (append it). 
C-o	Output this message to a file (append it). 
i	Input rmail file.  Run Rmail on that file. 
h	Show headers buffer, with a one line summary of each message. 
q       Quit out of RMAIL and save RMAIL file." 
  (interactive) 
  (kill-all-local-variables) 
  (make-local-variable 'rmail-summary-buffer) 
  (make-local-variable 'rmail-summary-list) 
  (make-local-variable 'rmail-current-message-number) 
  (make-local-variable 'rmail-total-messages) 
  (make-local-variable 'rmail-buffer-save) 
  (setq major-mode 'rmail-mode) 
  (setq mode-name "RMAIL") 
  (set-syntax-table text-mode-syntax-table) 
  (use-local-map rmail-mode-map) 
  (setq local-abbrev-table text-mode-abbrev-table) 
  (and (boundp 'rmail-mode-hook) 
       rmail-mode-hook 
       (funcall rmail-mode-hook))) 
 
(defun rmail-show-message (&optional filter) 
  (if (re-search-backward "^" nil t) 
      (let (start end) 
	(forward-line 1) 
	(setq start (dot)) 
	(re-search-forward "^") 
	(setq end (1- (dot))) 
	(goto-char start) 
	(narrow-to-region start end) 
	(if (search-forward "\n*** EOOH ***\n" nil t) 
	    (narrow-to-region (dot) end) 
	  (rmail-reformat-message) 
	  (goto-char (dot-min))) 
	(rmail-display-attributes)) 
    (goto-char (dot-min)) 
    (re-search-forward "^") 
    (beginning-of-line) 
    (rmail-display-attributes) 
    (narrow-to-region (dot-min) (dot)))) 
 
(defun rmail-reformat-message () 
  (goto-char (dot-min)) 
  (or (= (following-char) ?0) 
      (error "Bad format in RMAIL file.")) 
  (delete-char 1) 
  (insert "1") 
  (forward-line 1) 
  (let ((str (buffer-substring (dot) 
			       (progn 
				(re-search-forward "\n\n\n*" nil 'move) 
				(dot))))) 
    (insert "*** EOOH ***\n") 
    (narrow-to-region (dot) (dot-max)) 
    (insert str)) 
  (goto-char (dot-min)) 
  (and filter rmail-ignored-headers 
       (rmail-clear-headers)) 
  (and filter rmail-message-filter 
       (funcall rmail-message-filter))) 
 
(defun rmail-clear-headers () 
  (if (search-forward "\n\n" nil t) 
      (save-restriction 
        (narrow-to-region (dot-min) (dot)) 
	(goto-char (dot-min)) 
	(while (re-search-forward rmail-ignored-headers nil t) 
	  (beginning-of-line) 
	  (delete-region (dot) 
			 (progn (re-search-forward "\n[^ \t]") 
				(forward-char -1) 
				(dot))))))) 
 
(defun rmail-select-message (function &rest args) 
  (let ((omin (dot-min)) 
	(omax (dot-max)) 
	(odot (dot)) 
	win) 
    (unwind-protect 
     (progn 
      (widen) 
      (apply function args) 
      (setq win t)) 
     (if win 
	 (rmail-show-message t) 
       (goto-char odot) 
       (narrow-to-region omin omax))))) 
 
(defun rmail-message-attr-p (attribute) 
  (save-restriction 
   (save-excursion 
    (goto-char (dot-min)) 
    (widen) 
    (if (re-search-backward "^" nil t) 
	(let (start) 
	  (forward-line 1) 
	  (setq start (dot)) 
	  (search-forward ",,") 
	  (re-search-backward attribute start t)))))) 
 
(defun rmail-set-attr (attribute state) 
  (let ((omin (- (buffer-size) (dot-min))) 
	(omax (- (buffer-size) (dot-max)))) 
    (unwind-protect 
     (save-excursion 
      (goto-char (dot-min)) 
      (widen) 
      (if (re-search-backward "^" nil t) 
	  (let (start) 
	    (forward-line 1) 
	    (setq start (dot)) 
	    (search-forward ",,") 
	    (forward-char -2) 
	    (or (eq state 
		    (not (not (re-search-backward attribute start t)))) 
		(if state 
		    (insert "," attribute) 
		  (replace-match "") 
		  (delete-region (dot) 
				 (progn 
				  (search-backward ",") 
				  (dot))))) 
	    (forward-line 1) 
	    (rmail-make-summary-line)))) 
     (narrow-to-region (- (buffer-size) omin) 
		       (- (buffer-size) omax))))) 
 
;(defun rmail-all-headers () 
;  (interactive) 
;  (rmail-show-message nil)) 
 
(defun rmail-pass-headers () 
  (interactive) 
  (goto-char (dot-min)) 
  (re-search-forward "\n\n\n*" nil 'move) 
  (recenter-window 0)) 
 
(defun rmail-quit () 
  "Quit out of RMAIL." 
  (interactive) 
  (save-buffer) 
  (switch-to-buffer rmail-buffer-save)) 
 
(defun rmail-input (filename) 
  "Run Rmail on FILE." 
  (interactive "FRun RMAIL on RMAIL file: ") 
  (rmail filename)) 
 
(defun rmail-output (filename) 
  "Append this message to FILE." 
  (interactive "FOutput message to file: ") 
  (append-to-file (dot-min) (dot-max) filename)) 
 
(defun rmail-find (string) 
  "Show message containing next match for STRING (after dot)." 
  (interactive "sFind string: ") 
  (rmail-select-message 'search-forward string) 
  (rmail-set-message-counters) 
  (rmail-display-attributes)) 
 
(defun rmail-goto-message (n) 
  "Show message number N (prefix arg), counting from start of file." 
  (interactive "p") 
  (setq rmail-current-message-number 0) 
  (rmail-select-message 
   '(lambda (n) (progn (goto-char 1) 
		       (rmail-move-forward-message n))) 
   n)) 
 
(defun rmail-last-message () 
  "Show last message in file." 
  (interactive) 
  (setq rmail-current-message-number rmail-total-messages) 
  (rmail-select-message 
   '(lambda () 
	    (goto-char (dot-max)) 
	    (rmail-move-forward-message -1)))) 
 
(defun rmail-next-message (n) 
  "Show following non-deleted message. 
With prefix arg N, moves forward N non-deleted messages 
or backward if N is negative." 
  (interactive "p") 
  (rmail-select-message 'rmail-move-forward-undeleted-message n)) 
 
(defun rmail-previous-message (n) 
  "Show previous non-deleted message. 
With prefix arg N, moves backward N non-deleted messages 
or forward if N is negative." 
  (interactive "p") 
  (rmail-select-message 'rmail-move-forward-undeleted-message (- n))) 
 
(defun rmail-next-all (n) 
  "Show following message whether deleted or not. 
With prefix arg N, moves forward N messages 
or backward if N is negative." 
  (interactive "p") 
  (rmail-select-message 'rmail-move-forward-message n)) 
 
(defun rmail-previous-all (n) 
  "Show previous message whether deleted or not. 
With prefix arg N, moves backward N messages 
or forward if N is negative." 
  (interactive "p") 
  (rmail-select-message 'rmail-move-forward-message (- n))) 
 
(defun rmail-move-forward-message (n) 
  (while (and (> n 0) 
	      (re-search-forward "^" nil t)) 
    (setq rmail-current-message-number (1+ rmail-current-message-number)) 
    (setq n (1- n))) 
  (if (< n 0) 
      (progn 
       (re-search-backward "^" nil t) 
       (while (and (< n 0) 
		   (re-search-backward "^" nil t)) 
	 (setq rmail-current-message-number (1- rmail-current-message-number)) 
	 (setq n (1+ n))))) 
  (if (< n 0) 
      (message "No previous message")) 
  (if (> n 0) 
      (message "No following message")) 
  (forward-line 1)) 
 
(defun rmail-move-forward-undeleted-message (n) 
  (let ((lastwin (dot)) 
	(lastwinnumber rmail-current-message-number)) 
    (while (and (> n 0) 
		(re-search-forward "^" nil t)) 
      (save-excursion 
       (forward-line 1) 
       (setq rmail-current-message-number 
	     (1+ rmail-current-message-number)) 
       (if (rmail-not-deleted-p) 
	   (setq lastwin (dot) 
		 lastwinnumber rmail-current-message-number 
		 n (1- n))))) 
    (if (< n 0) 
	(progn 
	 (re-search-backward "^\^_") 
	 (while (and (< n 0) 
		     (re-search-backward "^" nil t)) 
	   (save-excursion 
	    (forward-line 1) 
	    (setq rmail-current-message-number 
		  (1- rmail-current-message-number)) 
	    (if (rmail-not-deleted-p) 
		(setq lastwin (dot) 
		      lastwinnumber rmail-current-message-number 
		      n (1+ n))))))) 
    (if (< n 0) 
	(message "No previous nondeleted message")) 
    (if (> n 0) 
	(message "No following nondeleted message")) 
    (setq rmail-current-message-number lastwinnumber) 
    (goto-char lastwin))) 
 
(defun rmail-not-deleted-p () 
  (let ((start (dot))) 
    (search-forward ",,") 
    (not (search-backward "deleted" start t)))) 
 
(defun rmail-delete-message () 
  "Delete this message and stay on it." 
  (interactive) 
  (rmail-set-attr "deleted" t) 
  (rmail-display-attributes)) 
 
(defun rmail-undelete-message () 
  "Remove deletion mark from this message." 
  (interactive) 
  (rmail-set-attr "deleted" nil) 
  (rmail-display-attributes)) 
 
(defun rmail-undelete-previous-message () 
  "Remove deletion mark from previous message, and select it." 
  (interactive) 
  (rmail-previous-all 1) 
  (rmail-undelete-message)) 
 
(defun rmail-delete-forward () 
  "Delete this message and move to next nondeleted one. 
Deleted messages stay in the file 
 until the `e' command (rmail-expunge) is given." 
  (interactive) 
  (rmail-set-attr "deleted" t) 
  (rmail-next-message 1)) 
 
(defun rmail-delete-backward () 
  "Delete this message and move to previous nondeleted one. 
Deleted messages stay in the file 
 until the `e' command (rmail-expunge) is given." 
  (interactive) 
  (rmail-set-attr "deleted" t) 
  (rmail-next-message -1)) 
 
(defun rmail-expunge () 
  "Actually erase all deleted messages in the file." 
  (interactive) 
  (message "Expunging deleted messages...") 
  (rmail-forget-summary) 
  (widen) 
  (save-excursion 
   (goto-char (dot-min)) 
   (let ((delete-counter 1)) 
     (re-search-forward "^[ \t\n]*") 
     (while (not (eobp)) 
       (let ((odot (dot)) 
	     delete) 
	 (search-forward ",,") 
	 (setq delete (re-search-backward "deleted" odot t)) 
	 (re-search-forward "^[ \t\n]*") 
	 (if delete 
	     (progn (if (> rmail-current-message-number delete-counter) 
			(setq rmail-current-message-number 
			      (1- rmail-current-message-number))) 
		    (delete-region odot (dot)) 
		    (setq rmail-total-messages 
			  (1- rmail-total-messages))) 
	   (setq delete-counter (1+ delete-counter))))))) 
  (message "Expunging deleted messages...done") 
  (rmail-show-message)) 
 
(defun rmail-continue () 
  "Continue composing outgoing message previously being composed." 
  (interactive) 
  (mail-other-window t)) 
 
(defun rmail-reply (just-sender) 
  "Reply to the current message. 
Normally include CC: to all other recipients of original message; 
prefix argument means ignore them. 
While composing the reply, use C-z y to yank the original message into it." 
  (interactive "P") 
  (rmail-set-attr "answered" t) 
  (rmail-display-attributes) 
  (mail-other-window nil 
	(or (rmail-fetch-field "from") 
	    (rmail-fetch-field "reply-to")) 
	(rmail-fetch-field "subject") 
	(let* ((string (rmail-fetch-field "from")) 
	       (stop-pos (string-match "  *at \\|  *@ \\| *(\\| *<" string))) 
	  (concat (if stop-pos (substring string 0 stop-pos) string) 
		  "'s message of " 
		  (rmail-fetch-field "date"))) 
	(and (not just-sender) 
	     (progn 
	      (setq cc-list 
		    (rmail-dont-reply-to 
		     (concat (rmail-fetch-field "to") 
			     (progn (setq cc-field 
					  (rmail-fetch-field "cc")) 
				    (if (or (string= cc-field "") 
					    (null cc-field)) 
					"" 
				      (concat ", " 
					      cc-field)))))) 
	      (if (string= cc-list "") nil 
		cc-list))) 
	(current-buffer))) 
 
(defun rmail-string-delete (string start end) 
  "Generic string delete function. This should be added to some global 
file. Delete substring of STRING starting from START going to END." 
  (concat (substring string 0 start) 
	  (if (null end) "" 
	    (substring string (1+ end) nil)))) 
		      
(defun rmail-strip-quoted-names (address) 
  "Get rid of fancy stuff in an address list." 
  (setq pos (string-match "\".*\"" address)) 
  (if pos (rmail-strip-quoted-names 
	   (rmail-string-delete address 
				pos 
				(string-match "\"" address (1+ pos)))) 
    address)) 
 
(defun rmail-strip-brackets-from-address (address) 
  (setq pos (string-match "<\\|>" address)) 
  (if pos (rmail-strip-brackets-from-address 
	   (rmail-string-delete address pos pos)) 
    address)) 
   
(defun rmail-dont-reply-to (userids) 
  "Takes a list of mailing addresses and prunes off any that are listed 
in the constant rmail-dont-reply-to-names." 
  (setq userids (rmail-strip-brackets-from-address userids)) 
  (setq userids (rmail-strip-quoted-names userids)) 
  (setq pos (string-match rmail-dont-reply-to-names (downcase userids) nil)) 
  (if pos (rmail-dont-reply-to 
	   (rmail-string-delete userids 
				pos 
				(string-match "[ ,]" userids pos))) 
    (progn 
     ; get rid of any trailing commas 
     (if (setq pos (string-match ", *$" userids)) 
	(setq userids (substring userids 0 pos))) 
     ; remove leading spaces. they bother me. 
     (rmail-string-delete userids 0 (1- (string-match "[^ ]*" userids)))))) 
 
(defun rmail-fetch-field (field-name) 
  (save-excursion 
   (let ((case-fold-search t)) 
     (goto-char (dot-min)) 
     (if (re-search-forward (concat "^" field-name ":[ \t]*") nil t) 
	 (let ((odot (dot))) 
	   (while (progn (forward-line 1) 
			 (looking-at "[ \t]"))) 
	   (buffer-substring odot (1- (dot)))))))) 
 
(defun rmail-forward () 
  "Forward the current message to another user." 
  (interactive) 
  (let ((forward-buffer (current-buffer))) 
    (if (mail-other-window) 
	(save-excursion 
	 (goto-char (dot-max)) 
	 (forward-line 1) 
	 (insert-buffer forward-buffer))))) 
 
(defun rmail-summary () 
  "Display a summary of all messages, one line per message." 
  (interactive) 
  (let ((odot (dot)) 
	(list rmail-summary-list) 
	listelt) 
    (if rmail-summary-list 
	(pop-to-buffer rmail-summary-buffer) 
      (setq list (rmail-new-summary))) 
    (while (and list (> (car (cdr (car list))) odot)) 
      (setq list (cdr list))) 
    (setq listelt (car list)) 
    (goto-char (dot-min)) 
    (if listelt 
	(forward-line (1- (car listelt)))))) 
 
(defun rmail-forget-summary () 
  (while rmail-summary-list 
    (move-marker (car (cdr (car rmail-summary-list))) nil) 
    (setq rmail-summary-list (cdr rmail-summary-list)))) 
   
(defun rmail-new-summary () 
  (message "Computing summary lines...") 
  (or rmail-summary-buffer 
      (setq rmail-summary-buffer 
	    (create-file-buffer (concat (buffer-name) "-summary")))) 
  (let ((summary-count 0) 
	summary-list 
	(rbuf (current-buffer))) 
    (save-restriction 
     (save-excursion 
      (widen) 
      (goto-char (dot-min)) 
      (while (re-search-forward "^\^_\^L" nil t) 
	(setq summary-count (1+ summary-count)) 
	(forward-line 2) 
	(or (looking-at "Summary-line: ") 
	    (rmail-make-summary-line)) 
	(search-forward ": ") 
	(setq rmail-summary-list 
	      (cons (list summary-count (dot-marker) nil) rmail-summary-list)) 
	(forward-line 1) 
	(setcar (cdr (cdr (car rmail-summary-list))) (dot))) 
      (setq list (reverse rmail-summary-list)) 
      (setq summary-list list) 
      (set-buffer rmail-summary-buffer) 
      (widen) 
      (erase-buffer) 
      (while list 
	(princ (car (car list)) (current-buffer)) 
	(insert ? ) 
	(insert-buffer-substring rbuf 
				 (car (cdr (car list))) 
				 (car (cdr (cdr (car list))))) 
	(setq list (cdr list))))) 
    (let ((sbuf rmail-summary-buffer)) 
      (pop-to-buffer sbuf) 
      (set-buffer sbuf)) 
    (rmail-summary-mode) 
    (setq rmail-buffer rbuf) 
    summary-list) 
  (message "Computing summary lines...done")) 
 
(defun rmail-make-summary-line () 
  ;; Delete existing summary line record. 
  (if (looking-at "Summary-line: ") 
      (delete-region (dot) 
		     (progn (forward-line 1) 
			    (dot)))) 
  (let ((beg (dot)) 
	deleted 
	end 
	from subj date) 
    (forward-line -1) 
    (setq deleted (re-search-forward "\\Wdeleted\\W" beg t)) 
    (search-forward "\n\^_") 
    (search-backward "\n*** EOOH ***\n" beg t) 
    (setq end (dot)) 
    (save-restriction 
     (narrow-to-region beg end) 
     (setq from (rmail-fetch-field "from")) 
     (setq subj (rmail-fetch-field "subject")) 
     (setq date (rmail-fetch-field "date"))) 
    (goto-char beg) 
    (insert "Summary-line: " 
	    (if deleted "D " "  ") 
	    (or date "") 
	    "  " (or from "anonymous") 
	    "  " (or subj "") "\n") 
    (goto-char beg))) 
 
(defvar rmail-buffer nil "") 
(defvar rmail-summary-mode-map nil "") 
 
(if rmail-summary-mode-map 
    nil 
  (setq rmail-summary-mode-map (make-keymap)) 
  (suppress-keymap rmail-summary-mode-map) 
  (define-key rmail-summary-mode-map "j" 'rmail-summary-goto-msg) 
  (define-key rmail-summary-mode-map "n" 'rmail-summary-next-msg) 
  (define-key rmail-summary-mode-map "p" 'rmail-summary-previous-msg) 
  (define-key rmail-summary-mode-map "d" 'rmail-summary-delete-forward)) 
 
(defun rmail-summary-next-msg () 
  (interactive) 
  (forward-line 1) 
  (rmail-summary-goto-msg)) 
 
(defun rmail-summary-previous-msg () 
  (interactive) 
  (forward-line -1) 
  (rmail-summary-goto-msg)) 
 
(defun rmail-summary-delete-forward () 
  (interactive) 
  (rmail-summary-goto-msg) 
  (pop-to-buffer rmail-buffer) 
  (rmail-delete-forward) 
  (pop-to-buffer rmail-summary-buffer) 
  (move-to-column 2) 
  (delete-char 1) 
  (insert "D") 
  (forward-line 1)) 
 
(defun rmail-summary-mode () 
  "RMAIL Summary Mode. 
A subset of the Rmail mode commands are supported in this mode.  
As commands are issued in the summary buffer the corresponding 
mail message is displayed in the rmail buffer. 
 
n       Move to next message. 
p       Move to previous message. 
j       Jump to the message at the cursor location. 
d       Delete the message at the cursor location and move to next message." 
  (interactive) 
  (kill-all-local-variables) 
  (make-local-variable 'rmail-buffer) 
  (setq major-mode 'rmail-summary-mode) 
  (setq mode-name "RMAIL Summary") 
  (use-local-map rmail-summary-mode-map) 
  (setq truncate-lines t) 
  (set-syntax-table text-mode-syntax-table)) 
 
(defun rmail-summary-goto-msg () 
  (interactive) 
  (let (num (buf rmail-buffer)) 
    (save-excursion 
     (beginning-of-line) 
     (setq num (read (current-buffer)))) 
    (pop-to-buffer buf) 
    (set-buffer buf) 
    (widen) 
    (goto-char (car (cdr (assq num rmail-summary-list)))) 
    (setq rmail-current-message-number num) 
    (rmail-show-message t) 
    (pop-to-buffer rmail-summary-buffer))) 
