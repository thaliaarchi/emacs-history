;; Convert buffer of Mocklisp code to real lisp. 
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
(defun convert-mocklisp-buffer () 
  "Convert buffer of mocklisp code into real lisp." 
  (interactive "") 
  (goto-char (dot-min)) 
  (insert "(or (fboundp 'ml-alias) (load \"gns/mlsupport.el\"))\n") 
  (fix-mlisp-syntax) 
  (goto-char (dot-min)) 
  (fix-mlisp-symbols) 
  (goto-char (dot-min)) 
  (while (re-search-forward "(setq[ \t\n]*(buffer-modified-p)" nil t) 
    (replace-match "(set-buffer-modified-p "))) 
 
(defun fix-mlisp-syntax () 
  (while (re-search-forward "['\"]" nil t) 
    (if (= (preceding-char) ?\") 
	(progn 
	 (forward-char -1) 
	 (forward-sexp 1)) 
      (delete-char -1) 
      (insert "?") 
      (or (= (following-char) ?\\) 
	  (insert ?\\)) 
      (forward-char 1) 
      (delete-char 1)))) 
 
(defun fix-mlisp-symbols () 
  (while (progn (forward-word 1) (not (eobp))) 
    (backward-sexp 1) 
    (let ((start (dot))) 
      (forward-sexp 1) 
      (if (setq prop (get (intern-soft (buffer-substring start (dot))) 'mocklisp)) 
	  (progn 
	    (delete-region start (dot)) 
	    (insert prop)))))) 
 
(defun ml-expansion (ml-name lisp-string) 
  (put ml-name 'mocklisp lisp-string)) 
 
(ml-expansion 'defun "ml-defun") 
(ml-expansion 'if "ml-if") 
(ml-expansion 'while "ml-while") 
(ml-expansion 'arg "ml-arg") 
(ml-expansion 'nargs "ml-nargs") 
(ml-expansion 'interactive "ml-interactive") 
(ml-expansion 'message "ml-message") 
(ml-expansion 'print "ml-print") 
(ml-expansion 'set "ml-set") 
(ml-expansion 'set-default "ml-set-default") 
(ml-expansion 'provide-prefix-argument "ml-provide-prefix-argument") 
(ml-expansion 'prefix-argument-loop "ml-prefix-argument-loop") 
(ml-expansion 'prefix-argument "ml-prefix-arg") 
(ml-expansion 'use-local-map "ml-use-local-map") 
(ml-expansion 'use-global-map "ml-use-global-map") 
(ml-expansion 'modify-syntax-entry "ml-undefined-modify-syntax-entry") 
(ml-expansion 'error-message "error") 
 
(ml-expansion 'dot "dot-marker") 
(ml-expansion 'mark "mark-marker") 
(ml-expansion 'beginning-of-file "beginning-of-buffer") 
(ml-expansion 'end-of-file "end-of-buffer") 
(ml-expansion 'set-mark "set-mark-command") 
(ml-expansion 'argument-prefix "universal-arg") 
 
(ml-expansion 'previous-page "ml-previous-page") 
(ml-expansion 'next-page "ml-next-page") 
(ml-expansion 'next-window "ml-next-window") 
(ml-expansion 'previous-window "ml-previous-window") 
 
(ml-expansion 'newline "ml-newline") 
(ml-expansion 'next-line "ml-next-line") 
(ml-expansion 'previous-line "ml-previous-line") 
(ml-expansion 'self-insert "self-insert-command") 
(ml-expansion 'meta-digit "digit-argument") 
(ml-expansion 'meta-minus "negative-argument") 
 
(ml-expansion 'newline-and-indent "ml-newline-and-indent") 
(ml-expansion 'yank-from-killbuffer "yank") 
(ml-expansion 'yank-buffer "insert-buffer") 
(ml-expansion 'delete-white-space "delete-horizontal-space") 
(ml-expansion 'widen-region "widen") 
 
(ml-expansion 'forward-paren "forward-list") 
(ml-expansion 'backward-paren "backward-list") 
(ml-expansion 'search-reverse "ml-search-backward") 
(ml-expansion 're-search-reverse "ml-re-search-backward") 
(ml-expansion 'search-forward "ml-search-forward") 
(ml-expansion 're-search-forward "ml-re-search-forward") 
(ml-expansion 'quote "regexp-quote") 
(ml-expansion 're-query-replace "query-replace-regexp") 
(ml-expansion 're-replace-string "replace-regexp") 
 
; forward-paren-bl, backward-paren-bl 
 
(ml-expansion 'get-tty-character "read-char") 
(ml-expandion 'get-tty-input "read-input") 
(ml-expandion 'get-tty-string "read-string") 
(ml-expandion 'get-tty-buffer "read-buffer") 
(ml-expandion 'get-tty-command "read-command") 
(ml-expandion 'get-tty-variable "read-variable") 
(ml-expandion 'get-tty-no-blanks-input "read-no-blanks-input") 
(ml-expandion 'get-tty-key "read-key") 
 
(ml-expansion 'c= "char-equal") 
(ml-expansion 'goto-character "goto-char") 
(ml-expansion 'substr "ml-substr") 
(ml-expansion 'variable-apropos "apropos") 
(ml-expansion 'execute-mlisp-buffer "eval-current-buffer") 
(ml-expansion 'execute-mlisp-file "load") 
(ml-expansion 'execute-mlisp-line "eval-expression") 
(ml-expansion 'visit-file "find-file") 
(ml-expansion 'read-file "find-file") 
(ml-expansion 'write-modified-files "save-some-buffers") 
(ml-expansion 'write-file-exit "save-buffers-kill-emacs") 
(ml-expansion 'write-named-file "write-file") 
(ml-expansion 'change-file-name "set-visited-file-name") 
(ml-expansion 'change-buffer-name "rename-buffer") 
(ml-expansion 'delete-buffer "kill-buffer") 
(ml-expansion 'unlink-file "delete-file") 
(ml-expansion 'file-exists "file-exists-p") 
(ml-expansion 'write-current-file "save-buffer") 
(ml-expansion 'change-directory "cd") 
(ml-expansion 'temp-use-buffer "set-buffer") 
 
(ml-expansion 'pending-input "input-pending-p") 
(ml-expansion 'execute-keyboard-macro "call-last-kbd-macro") 
(ml-expansion 'start-remembering "start-kbd-macro") 
(ml-expansion 'end-remembering "end-kbd-macro") 
(ml-expansion 'define-keyboard-macro "name-last-kbd-macro") 
(ml-expansion 'define-string-macro "ml-define-string-macro") 
 
(ml-expansion 'dot-is-visible "dot-visible-p") 
(ml-expansion 'current-column "ml-current-column") 
(ml-expansion 'current-indent "ml-current-indent") 
(ml-expansion 'insert-character "insert") 
 
(ml-expansion 'users-login-name "user-login-name") 
(ml-expansion 'users-full-name "user-full-name") 
(ml-expansion 'current-time "current-time-string") 
(ml-expansion 'current-buffer-name "buffer-name") 
(ml-expansion 'current-file-name "buffer-file-name") 
 
(ml-expansion 'local-binding-of "local-key-binding") 
(ml-expansion 'global-binding-of "global-key-binding") 
 
;defproc (ProcedureType, "procedure-type"); 
 
(ml-expansion 'remove-key-binding "global-unset-key") 
(ml-expansion 'remove-binding "global-unset-key") 
(ml-expansion 'remove-local-binding "local-unset-key") 
(ml-expansion 'remove-all-local-bindings "use-local-map nil") 
(ml-expansion 'autoload "ml-autoload") 
 
(ml-expansion 'checkpoint-frequency "auto-save-interval") 
 
(ml-expansion 'mode-string "mode-name") 
(ml-expansion 'right-margin "fill-column") 
(ml-expansion 'tab-size "tab-width") 
(ml-expansion 'default-right-margin "default-fill-column") 
(ml-expansion 'default-tab-size "default-tab-width") 
(ml-expansion 'buffer-is-modified "(buffer-modified-p)") 
 
(ml-expansion 'file-modified-time "you-lose-on-file-modified-time") 
(ml-expansion 'needs-checkpointing "you-lose-on-needs-checkpointing") 
 
(ml-expansion 'lines-on-screen "set-screen-height") 
(ml-expansion 'columns-on-screen "set-screen-width") 
 
(ml-expansion 'dumped-emacs "t") 
 
(ml-expansion 'buffer-size "ml-buffer-size") 
(ml-expansion 'dot-is-visible "pos-visible-in-window-p") 
 
(ml-expansion 'track-eol-on-^N-^P "track-eol") 
(ml-expansion 'ctlchar-with-^ "ctl-arrow") 
(ml-expansion 'help-on-command-completion-error "completion-auto-help") 
(ml-expansion 'dump-stack-trace "backtrace") 
(ml-expansion 'pause-emacs "suspend-emacs") 
(ml-expansion 'compile-it "compile") 
 
;Variable pause-writes-files 
