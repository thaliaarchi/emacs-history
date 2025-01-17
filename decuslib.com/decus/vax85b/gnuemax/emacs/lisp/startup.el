;; Process Emacs shell arguments 
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
; -e function		execute function 
; -l file		load file 
; file			visit file 
; -q			load no profile 
; -u user		load user's profile 
 
(setq top-level '(normal-top-level)) 
 
(defvar inhibit-command-line nil 
  "Non-nil inhibits usual processing of command line args from shell. 
Exception: -q and -u are processed normally anyway.") 
 
(defvar command-line-processed nil "t once command line has been processed") 
 
(defun normal-top-level () 
  (if command-line-processed 
      (message "Back to top level.") 
    (setq command-line-processed t) 
    (command-line))) 
 
(defun command-line () 
  (let ((args (cdr command-line-args)) 
	(user (getenv "USER")) 
	done) 
    (while (and (not done) args) 
      (let ((argi (car args))) 
	(if (string-equal argi "-q") 
	    (setq user nil 
		  args (cdr args)) 
	  (if (string-equal argi "-u") 
	      (setq args (cdr args) 
		    user (car args) 
		    args (cdr args)) 
	    (setq done t))))) 
    (if user 
	(or (load (concat "~" user "/.emacs") t t) 
	    (load "default-profile" t t))) 
    (if (not inhibit-command-line) 
	(command-line-1 args)))) 
 
(defun command-line-1 (args) 
  (if (null args) 
      (progn 
       (insert (emacs-version)) 
       (goto-char (dot-min)) 
       (search-forward " " nil nil 3) 
       (delete-region (1- (dot)) (dot-max)) 
       (insert ", Copyright (C) 1985 by Richard M. Stallman. 
It is freeware: you may copy and redistribute it under certain conditions. 
Type C-h C-c to see those conditions. 
Type C-h C-d for information on getting copies from me.\n") 
       (message "Type Control-h for help; Control-x u to undo changes") 
       (set-buffer-modified-p nil) 
       (sit-for 100) 
       (erase-buffer) 
       (set-buffer-modified-p nil)) 
    (let ((dir default-directory)) 
      (while args 
	(let ((argi (car args))) 
	  (setq args (cdr args)) 
	  (if (string-equal argi "-e") 
	      (setq tem (funcall (intern (car args))) 
		    args (cdr args)) 
	    (if (string-equal argi "-l") 
		(setq tem (load (expand-file-name (car args) dir)) 
		      args (cdr args)) 
	      (find-file (expand-file-name argi dir))))))) 
    (message "Type Control-H for help; Control-X u to undo changes"))) 
