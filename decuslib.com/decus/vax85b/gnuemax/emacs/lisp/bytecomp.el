;; Compilation of Lisp code into byte code. 
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
(defvar byte-compile-constnum -1 
  "Transfer vector index of last constant allocated.") 
(defvar byte-compile-constants nil 
  "Alist describing contents to put in transfer vector. 
Each element is (CONTENTS . INDEX)") 
(defvar byte-compile-pc 0 
  "Index in byte string to store next opcode at.") 
(defvar byte-compile-output nil 
  "Alist describing contents to put in byte code string. 
Each element is (INDEX . VALUE)") 
(defvar byte-compile-depth 0 
  "Current depth of execution stack.") 
(defvar byte-compile-maxdepth 0 
  "Maximum depth of execution stack.") 
 
(defconst byte-varref 8 
  "Byte code opcode for variable reference.") 
(defconst byte-varset 16 
  "Byte code opcode for setting a variable.") 
(defconst byte-varbind 24 
  "Byte code opcode for binding a variable.") 
(defconst byte-call 32 
  "Byte code opcode for calling a function.") 
(defconst byte-unbind 40 
  "Byte code opcode for unbinding special bindings.") 
 
(defconst byte-constant 192 
  "Byte code opcode for reference to a constant.") 
(defconst byte-constant-limit 64 
  "Maximum index usable in  byte-constant  opcode.") 
 
(defconst byte-constant2 129 
  "Byte code opcode for reference to a constant with vector index >= 0100.") 
 
(defconst byte-goto 130 
  "Byte code opcode for unconditional jump") 
 
(defconst byte-goto-if-nil 131 
  "Byte code opcode for pop value and jump if it's nil.") 
 
(defconst byte-goto-if-not-nil 132 
  "Byte code opcode for pop value and jump if it's not nil.") 
 
(defconst byte-goto-if-nil-else-pop 133 
  "Byte code opcode for examine top-of-stack, jump and don't pop it if it's nil, 
otherwise pop it.") 
 
(defconst byte-goto-if-not-nil-else-pop 134 
  "Byte code opcode for examine top-of-stack, jump and don't pop it if it's not nil, 
otherwise pop it.") 
 
(defconst byte-return 135 
  "Byte code opcode for pop value and return it from byte code interpreter.") 
 
(defconst byte-discard 136 
  "Byte code opcode to discard one value from stack.") 
 
(defconst byte-dup 137 
  "Byte code opcode to duplicate the top of the stack.") 
 
(defconst byte-save-excursion 138 
  "Byte code opcode to make a binding to record the buffer, dot and mark.") 
 
(defconst byte-save-window-excursion 139 
  "Byte code opcode to make a binding to record entire window configuration.") 
 
(defconst byte-save-restriction 140 
  "Byte code opcode to make a binding to record the current buffer clipping restrictions.") 
 
(defconst byte-catch 141 
  "Byte code opcode for catch.  Takes, on stack, the tag and an expression for the body.") 
 
(defconst byte-unwind-protect 142 
  "Byte code opcode for unwind-protect.  Takes, on stack, an expression for the body 
and an expression for the unwind-action.") 
 
(defconst byte-condition-case 143 
  "Byte code opcode for condition-case.  Takes, on stack, the variable to bind, 
an expression for the body, and a list of clauses.") 
 
(defconst byte-temp-output-buffer-setup 144 
  "Byte code opcode for entry to with-output-to-temp-buffer. 
Takes, on stack, the buffer name. 
Binds standard-output and does some other things. 
Returns with temp buffer on the stack in place of buffer name.") 
 
(defconst byte-temp-output-buffer-show 145 
  "Byte code opcode for exit from with-output-to-temp-buffer. 
Expects the temp buffer on the stack underneath value to return. 
Pops them both, then pushes the value back on. 
Unbinds standard-output and makes the temp buffer visible.") 
 
(defconst byte-nth 56) 
(defconst byte-symbolp 57) 
(defconst byte-consp 58) 
(defconst byte-stringp 59) 
(defconst byte-listp 60) 
(defconst byte-eq 61) 
(defconst byte-memq 62) 
(defconst byte-not 63) 
(defconst byte-car 64) 
(defconst byte-cdr 65) 
(defconst byte-cons 66) 
(defconst byte-list1 67) 
(defconst byte-list2 68) 
(defconst byte-list3 69) 
(defconst byte-list4 70) 
(defconst byte-length 71) 
(defconst byte-aref 72) 
(defconst byte-aset 73) 
(defconst byte-symbol-value 74) 
(defconst byte-symbol-function 75) 
(defconst byte-set 76) 
(defconst byte-fset 77) 
(defconst byte-get 78) 
(defconst byte-substring 79) 
(defconst byte-concat2 80) 
(defconst byte-concat3 81) 
(defconst byte-concat4 82) 
(defconst byte-sub1 83) 
(defconst byte-add1 84) 
(defconst byte-eqlsign 85) 
(defconst byte-gtr 86) 
(defconst byte-lss 87) 
(defconst byte-leq 88) 
(defconst byte-geq 89) 
(defconst byte-diff 90) 
(defconst byte-negate 91) 
(defconst byte-plus 92) 
(defconst byte-max 93) 
(defconst byte-min 94) 
 
(defconst byte-dot 96) 
(defconst byte-mark 97) 
(defconst byte-goto-char 98) 
(defconst byte-insert 99) 
(defconst byte-dot-max 100) 
(defconst byte-dot-min 101) 
(defconst byte-char-after 102) 
(defconst byte-following-char 103) 
(defconst byte-preceding-char 104) 
(defconst byte-current-column 105) 
(defconst byte-indent-to 106) 
(defconst byte-scan-buffer 107) 
(defconst byte-eolp 108) 
(defconst byte-eobp 109) 
(defconst byte-bolp 110) 
(defconst byte-bobp 111) 
(defconst byte-current-buffer 112) 
(defconst byte-set-buffer 113) 
(defconst byte-read-char 114) 
(defconst byte-set-mark 115) 
 
(defun byte-recompile-directory (directory) 
  "Recompile every .el file in DIRECTORY that needs recompilation. 
This is if a .elc file exists but is older than the .el file."  
  (interactive "DByte recompile directory: ") 
  (save-some-buffers) 
  (setq directory (expand-file-name directory)) 
  (let ((files (directory-files directory)) 
	source) 
    (while files 
      (if (and (string-match ".elc$" (car files)) 
	       (not (string-match "^#" (car files))) 
	       (file-newer-than-file-p 
		(setq source (expand-file-name 
			      (substring (car files) 0 -1) 
			      directory)) 
		(expand-file-name (car files) directory))) 
	  (byte-compile-file source)) 
      (setq files (cdr files))))) 
 
(defun file-newer-than-file-p (file1 file2) 
  "Return t if FILE1 modified more recently than FILE2." 
  (let ((mtime1 (car (nthcdr 5 (file-attributes file1)))) 
	(mtime2 (car (nthcdr 5 (file-attributes file2))))) 
    (or (> (car mtime1) (car mtime2)) 
	(and (= (car mtime1) (car mtime2)) 
	     (> (car (cdr mtime1)) (car (cdr mtime2))))))) 
 
(defun byte-compile-file (filename) 
  "Compile a file of Lisp code named FILENAME into a file of byte code. 
The output file's name is made by appending \"c\" to the end of FILENAME." 
  (interactive "fByte compile file: ") 
  ;; Expand now so we get the current buffer's defaults 
  (setq filename (expand-file-name filename)) 
  (message "Compiling %s..." filename) 
  (let ((inbuffer (get-buffer-create " *Compiler Input*")) 
	(outbuffer (get-buffer-create " *Compiler Output*")) 
	sexp) 
    (save-excursion 
      (set-buffer inbuffer) 
      (erase-buffer) 
      (insert-file-contents filename) 
      (goto-char 1) 
      (set-buffer outbuffer) 
      (erase-buffer) 
      (while (save-excursion 
	      (set-buffer inbuffer) 
	      (while (progn (skip-chars-forward " \t\n") 
			    (looking-at ";")) 
		(forward-line 1)) 
	      (not (eobp))) 
	(setq sexp (read inbuffer)) 
	(print (byte-compile-file-form sexp) outbuffer)) 
      (set-buffer outbuffer) 
      (goto-char 1) 
      (while (search-forward "\n(" nil t) 
	(cond ((looking-at "defun \\|autoload ") 
	       (forward-sexp 3) 
	       (skip-chars-forward " ") 
	       (if (looking-at "\"") 
		   (progn (forward-char 1) 
			  (insert "\\\n")))))) 
      (write-region 1 (dot-max) (concat filename "c")) 
      (erase-buffer) 
      (set-buffer inbuffer) 
      (erase-buffer)))) 
 
(defun byte-compile-file-form (form) 
  (if (and (listp form) (memq (car form) '(defun defmacro))) 
      (cons (car form) (byte-compile-lambda (cdr form))) 
    form)) 
 
(defun byte-compile (funname) 
  "Byte-compile the definition of function FUNNAME (a symbol)." 
  (if (and (fboundp funname) 
	   (listp (symbol-function funname)) 
	   (eq (car (symbol-function funname)) 'lambda)) 
      (fset funname (byte-compile-lambda (symbol-function funname))))) 
 
(defun byte-compile-lambda (fun) 
  (let* ((bodyptr (cdr fun)) 
	 (int (assq 'interactive (cdr bodyptr))) 
	 newbody) 
    (if (stringp (car (cdr bodyptr))) 
	(setq bodyptr (cdr bodyptr))) 
    (setq newbody (list (byte-compile-top-level (cons 'progn (cdr bodyptr))))) 
    (if int 
	(setq newbody (cons int newbody))) 
    (if (not (eq bodyptr (cdr fun))) 
	(setq newbody (cons (nth 2 fun) newbody))) 
    (cons (car fun) (cons (car (cdr fun)) newbody)))) 
 
(defun byte-compile-top-level (form) 
  (let ((byte-compile-constants nil) 
	(byte-compile-constnum nil) 
	(byte-compile-pc 0) 
	(byte-compile-depth 0) 
	(byte-compile-maxdepth 0) 
	(byte-compile-output nil) 
	(byte-compile-string nil) 
	(byte-compile-vector nil)) 
    (let ((vars (nreverse (byte-compile-find-vars form))) 
	  (i -1)) 
      (while vars 
	(setq i (1+ i)) 
	(setq byte-compile-constants (cons (cons (car vars) i) byte-compile-constants)) 
	(setq vars (cdr vars))) 
      (setq byte-compile-constnum i)) 
    (byte-compile-form form) 
    (byte-compile-out 'byte-return 0) 
    (setq byte-compile-vector (make-vector (1+ byte-compile-constnum) 
					   nil)) 
    (while byte-compile-constants 
      (aset byte-compile-vector (cdr (car byte-compile-constants)) 
	    (car (car byte-compile-constants))) 
      (setq byte-compile-constants (cdr byte-compile-constants))) 
    (setq byte-compile-string (make-string byte-compile-pc 0)) 
    (while byte-compile-output 
      (aset byte-compile-string (car (car byte-compile-output)) 
	    (cdr (car byte-compile-output))) 
      (setq byte-compile-output (cdr byte-compile-output))) 
    (list 'byte-code byte-compile-string byte-compile-vector byte-compile-maxdepth))) 
 
(defun byte-compile-find-vars (form) 
  (let ((all-vars nil)) 
    (byte-compile-find-vars-1 form) 
    all-vars)) 
 
(defun byte-compile-find-vars-1 (form) 
  (if (symbolp form) 
      (if (not (memq form all-vars)) 
	  (setq all-vars (cons form all-vars))) 
    (if (or (not (consp form)) (eq (car form) 'quote)) 
	nil 
      (if (memq (car form) '(let let*)) 
	  (let ((binds (car (cdr form))) 
		(body (cdr (cdr form)))) 
	    (while binds 
	      (if (symbolp (car binds)) 
		  (if (not (memq (car binds) all-vars)) 
		      (setq all-vars (cons (car binds) all-vars))) 
		(if (consp (car binds)) 
		    (progn 
		      (if (not (memq (car (car binds)) all-vars)) 
			  (setq all-vars (cons (car (car binds)) all-vars))) 
		      (byte-compile-find-vars-1 (car (cdr (car binds))))))) 
	      (setq binds (cdr binds))) 
	    (while body 
	      (byte-compile-find-vars-1 (car body)) 
	      (setq body (cdr body)))) 
	(if (eq (car form) 'cond) 
	    (let ((clauses (cdr form))) 
	      (while clauses 
		(let ((body (car clauses))) 
		  (while body 
		    (byte-compile-find-vars-1 (car body)) 
		    (setq body (cdr body)))) 
		(setq clauses (cdr clauses)))) 
	  (let ((body (cdr form))) 
	    (while body 
	      (byte-compile-find-vars-1 (car body)) 
	      (setq body (cdr body))))))))) 
 
;; This is the recursive entry point for compiling each subform of an expression. 
 
(defun byte-compile-form (form) 
  (if (fboundp 'macroexpand) 
      (setq form (macroexpand form))) 
  (if (symbolp form) 
      (byte-compile-variable-ref 'byte-varref form) 
    (if (not (consp form)) 
	(byte-compile-constant form) 
      (let ((handler (get (car form) 'byte-compile))) 
	(if handler 
	    (funcall handler form) 
	  (byte-compile-push-constant (car form)) 
	  (let ((copy (cdr form))) 
	    (while copy (byte-compile-form (car copy)) (setq copy (cdr copy)))) 
	  (byte-compile-out 'byte-call (length (cdr form))) 
	  (setq byte-compile-depth (- byte-compile-depth (length (cdr form)))))))) 
  (setq byte-compile-maxdepth (max byte-compile-maxdepth 
				   (setq byte-compile-depth (1+ byte-compile-depth))))) 
 
(defun byte-compile-variable-ref (base-op var) 
  (let ((data (assq var byte-compile-constants))) 
    (if data 
	(byte-compile-out base-op (cdr data)) 
      (error (format "Variable %s seen on pass 2 of byte compiler but not pass 1" var))))) 
 
(defun byte-compile-constant (const) 
  (let ((data (if (stringp const) 
		  (assoc const byte-compile-constants) 
		(assq const byte-compile-constants)))) 
    (if data 
	(byte-compile-out-const (cdr data)) 
      (setq byte-compile-constants 
	    (cons (cons const (setq byte-compile-constnum (1+ byte-compile-constnum))) 
		  byte-compile-constants)) 
      (byte-compile-out-const byte-compile-constnum)))) 
 
(defun byte-compile-push-constant (const) 
  (byte-compile-constant const) 
  (setq byte-compile-maxdepth (max byte-compile-maxdepth 
				   (setq byte-compile-depth (1+ byte-compile-depth))))) 
 
;; Compile those primitive ordinary functions 
;; which have special byte codes just for speed. 
 
(put 'dot 'byte-compile 'byte-compile-no-args) 
(put 'dot 'byte-opcode 'byte-dot) 
 
(put 'mark 'byte-compile 'byte-compile-no-args) 
(put 'mark 'byte-opcode 'byte-mark) 
 
(put 'dot-max 'byte-compile 'byte-compile-no-args) 
(put 'dot-max 'byte-opcode 'byte-dot-max) 
 
(put 'dot-min 'byte-compile 'byte-compile-no-args) 
(put 'dot-min 'byte-opcode 'byte-dot-min) 
 
(put 'following-char 'byte-compile 'byte-compile-no-args) 
(put 'following-char 'byte-opcode 'byte-following-char) 
 
(put 'preceding-char 'byte-compile 'byte-compile-no-args) 
(put 'preceding-char 'byte-opcode 'byte-preceding-char) 
 
(put 'current-column 'byte-compile 'byte-compile-no-args) 
(put 'current-column 'byte-opcode 'byte-current-column) 
 
(put 'eolp 'byte-compile 'byte-compile-no-args) 
(put 'eolp 'byte-opcode 'byte-eolp) 
 
(put 'eobp 'byte-compile 'byte-compile-no-args) 
(put 'eobp 'byte-opcode 'byte-eobp) 
 
(put 'bolp 'byte-compile 'byte-compile-no-args) 
(put 'bolp 'byte-opcode 'byte-bolp) 
 
(put 'bobp 'byte-compile 'byte-compile-no-args) 
(put 'bobp 'byte-opcode 'byte-bobp) 
 
(put 'current-buffer 'byte-compile 'byte-compile-no-args) 
(put 'current-buffer 'byte-opcode 'byte-current-buffer) 
 
(put 'read-char 'byte-compile 'byte-compile-no-args) 
(put 'read-char 'byte-opcode 'byte-read-char) 
 
 
(put 'symbolp 'byte-compile 'byte-compile-one-arg) 
(put 'symbolp 'byte-opcode 'byte-symbolp) 
 
(put 'consp 'byte-compile 'byte-compile-one-arg) 
(put 'consp 'byte-opcode 'byte-consp) 
 
(put 'stringp 'byte-compile 'byte-compile-one-arg) 
(put 'stringp 'byte-opcode 'byte-stringp) 
 
(put 'listp 'byte-compile 'byte-compile-one-arg) 
(put 'listp 'byte-opcode 'byte-listp) 
 
(put 'not 'byte-compile 'byte-compile-one-arg) 
(put 'not 'byte-opcode 'byte-not) 
 
(put 'null 'byte-compile 'byte-compile-one-arg) 
(put 'null 'byte-opcode 'byte-not) 
 
(put 'car 'byte-compile 'byte-compile-one-arg) 
(put 'car 'byte-opcode 'byte-car) 
 
(put 'cdr 'byte-compile 'byte-compile-one-arg) 
(put 'cdr 'byte-opcode 'byte-cdr) 
 
(put 'length 'byte-compile 'byte-compile-one-arg) 
(put 'length 'byte-opcode 'byte-length) 
 
(put 'symbol-value 'byte-compile 'byte-compile-one-arg) 
(put 'symbol-value 'byte-opcode 'byte-symbol-value) 
 
(put 'symbol-function 'byte-compile 'byte-compile-one-arg) 
(put 'symbol-function 'byte-opcode 'byte-symbol-function) 
 
(put '1+ 'byte-compile 'byte-compile-one-arg) 
(put '1+ 'byte-opcode 'byte-add1) 
 
(put '1- 'byte-compile 'byte-compile-one-arg) 
(put '1- 'byte-opcode 'byte-sub1) 
 
(put 'goto-char 'byte-compile 'byte-compile-one-arg) 
(put 'goto-char 'byte-opcode 'byte-goto-char) 
 
(put 'char-after 'byte-compile 'byte-compile-one-arg) 
(put 'char-after 'byte-opcode 'byte-char-after) 
 
(put 'set-buffer 'byte-compile 'byte-compile-one-arg) 
(put 'set-buffer 'byte-opcode 'byte-set-buffer) 
 
(put 'set-mark 'byte-compile 'byte-compile-one-arg) 
(put 'set-mark 'byte-opcode 'byte-set-mark) 
 
 
(put 'eq 'byte-compile 'byte-compile-two-args) 
(put 'eq 'byte-opcode 'byte-eq) 
 
(put 'memq 'byte-compile 'byte-compile-two-args) 
(put 'memq 'byte-opcode 'byte-memq) 
 
(put 'cons 'byte-compile 'byte-compile-two-args) 
(put 'cons 'byte-opcode 'byte-cons) 
 
(put 'aref 'byte-compile 'byte-compile-two-args) 
(put 'aref 'byte-opcode 'byte-aref) 
 
(put 'set 'byte-compile 'byte-compile-two-args) 
(put 'set 'byte-opcode 'byte-set) 
 
(put 'fset 'byte-compile 'byte-compile-two-args) 
(put 'fset 'byte-opcode 'byte-fset) 
 
(put '= 'byte-compile 'byte-compile-two-args) 
(put '= 'byte-opcode 'byte-eqlsign) 
 
(put '< 'byte-compile 'byte-compile-two-args) 
(put '< 'byte-opcode 'byte-lss) 
 
(put '> 'byte-compile 'byte-compile-two-args) 
(put '> 'byte-opcode 'byte-gtr) 
 
(put '<= 'byte-compile 'byte-compile-two-args) 
(put '<= 'byte-opcode 'byte-leq) 
 
(put '>= 'byte-compile 'byte-compile-two-args) 
(put '>= 'byte-opcode 'byte-geq) 
 
(put 'get 'byte-compile 'byte-compile-two-args) 
(put 'get 'byte-opcode 'byte-get) 
 
(put 'nth 'byte-compile 'byte-compile-two-args) 
(put 'nth 'byte-opcode 'byte-nth) 
 
(put 'aset 'byte-compile 'byte-compile-three-args) 
(put 'aset 'byte-opcode 'byte-aset) 
 
(put 'substring 'byte-compile 'byte-compile-three-args) 
(put 'substring 'byte-opcode 'byte-substring) 
 
(put 'scan-buffer 'byte-compile 'byte-compile-three-args) 
(put 'scan-buffer 'byte-opcode 'byte-scan-buffer) 
 
(defun byte-compile-no-args (form) 
  (byte-compile-out (symbol-value (get (car form) 'byte-opcode)) 0)) 
 
(defun byte-compile-one-arg (form) 
  (byte-compile-form (or (car (cdr form)) ''nil))  ;; Push the argument 
  (setq byte-compile-depth (1- byte-compile-depth)) 
  (byte-compile-out (symbol-value (get (car form) 'byte-opcode)) 0)) 
 
(defun byte-compile-two-args (form) 
  (byte-compile-form (or (car (cdr form)) ''nil))  ;; Push the arguments 
  (byte-compile-form (or (nth 2 form) ''nil)) 
  (setq byte-compile-depth (- byte-compile-depth 2)) 
  (byte-compile-out (symbol-value (get (car form) 'byte-opcode)) 0)) 
 
(defun byte-compile-three-args (form) 
  (byte-compile-form (or (car (cdr form)) ''nil))  ;; Push the arguments 
  (byte-compile-form (or (nth 2 form) ''nil)) 
  (byte-compile-form (or (nth 3 form) ''nil)) 
  (setq byte-compile-depth (- byte-compile-depth 3)) 
  (byte-compile-out (symbol-value (get (car form) 'byte-opcode)) 0)) 
 
(put 'list 'byte-compile 'byte-compile-list) 
(defun byte-compile-list (form) 
  (let ((len (length form))) 
    (if (= len 1) 
	(byte-compile-constant nil) 
      (if (< len 6) 
	  (let ((args (cdr form))) 
	   (while args 
	     (byte-compile-form (car args)) 
	     (setq args (cdr args))) 
	   (setq byte-compile-depth (- byte-compile-depth (1- len))) 
	   (byte-compile-out (symbol-value 
			      (nth (- len 2) 
				   '(byte-list1 byte-list2 byte-list3 byte-list4))) 
			     0)) 
	(byte-compile-normal-call form))))) 
 
(put 'concat 'byte-compile 'byte-compile-concat) 
(defun byte-compile-concat (form) 
  (let ((len (length form))) 
    (if (= len 1) 
	(byte-compile-form "") 
      (if (= len 2) 
	  (byte-compile-form (nth 1 form)) 
	(if (< len 6) 
	    (let ((args (cdr form))) 
	     (while args 
	       (byte-compile-form (car args)) 
	       (setq args (cdr args))) 
	     (setq byte-compile-depth (- byte-compile-depth (1- len))) 
	     (byte-compile-out (symbol-value 
				(nth (- len 3) 
				     '(byte-concat2 byte-concat3 byte-concat4))) 
			       0)) 
	  (byte-compile-normal-call form)))))) 
 
(put '- 'byte-compile 'byte-compile-minus) 
(defun byte-compile-minus (form) 
  (let ((len (length form))) 
    (if (= len 2) 
	(progn 
	  (byte-compile-form (car (cdr form))) 
	  (setq byte-compile-depth (- byte-compile-depth 1)) 
	  (byte-compile-out byte-negate 0)) 
      (if (= len 3) 
	  (progn 
	    (byte-compile-form (car (cdr form))) 
	    (byte-compile-form (nth 2 form)) 
	    (setq byte-compile-depth (- byte-compile-depth 2)) 
	    (byte-compile-out byte-diff 0)) 
	(byte-compile-normal-call form))))) 
 
(put '+ 'byte-compile 'byte-compile-maybe-two-args) 
(put '+ 'byte-opcode 'byte-plus) 
 
(put 'max 'byte-compile 'byte-compile-maybe-two-args) 
(put 'max 'byte-opcode 'byte-max) 
 
(put 'min 'byte-compile 'byte-compile-maybe-two-args) 
(put 'min 'byte-opcode 'byte-min) 
 
(defun byte-compile-maybe-two-args (form) 
  (let ((len (length form))) 
    (if (= len 3) 
	(progn 
	  (byte-compile-form (car (cdr form))) 
	  (byte-compile-form (nth 2 form)) 
	  (setq byte-compile-depth (- byte-compile-depth 2)) 
	  (byte-compile-out (symbol-value (get (car form) 'byte-opcode)) 0)) 
      (byte-compile-normal-call form)))) 
	 
(defun byte-compile-normal-call (form) 
  (byte-compile-push-constant (car form)) 
  (let ((copy (cdr form))) 
    (while copy (byte-compile-form (car copy)) (setq copy (cdr copy)))) 
  (byte-compile-out 'byte-call (length (cdr form))) 
  (setq byte-compile-depth (- byte-compile-depth (length (cdr form))))) 
 
(put 'function 'byte-compile 'byte-compile-function-form) 
(defun byte-compile-function-form (form) 
  (byte-compile-constant (byte-compile-lambda (car (cdr form))))) 
 
(put 'indent-to 'byte-compile 'byte-compile-indent-to) 
(defun byte-compile-indent-to (form) 
  (let ((len (length form))) 
    (if (= len 2) 
	(progn 
	  (byte-compile-form (car (cdr form))) 
	  (setq byte-compile-depth (- byte-compile-depth 1)) 
	  (byte-compile-out byte-indent-to 0)) 
      (byte-compile-normal-call form)))) 
 
(put 'insert 'byte-compile 'byte-compile-insert) 
(defun byte-compile-insert (form) 
  (let ((len (length form))) 
    (if (< len 3) 
	(let ((args (cdr form))) 
	  (while args 
	    (byte-compile-form (car args)) 
	    (setq byte-compile-depth (- byte-compile-depth 1)) 
	    (byte-compile-out byte-insert 0) 
	    (setq args (cdr args)))) 
      (byte-compile-normal-call form)))) 
 
(put 'quote 'byte-compile 'byte-compile-quote) 
(defun byte-compile-quote (form) 
  (byte-compile-constant (car (cdr form)))) 
 
(put 'setq 'byte-compile 'byte-compile-setq) 
(defun byte-compile-setq (form) 
  (let ((args (cdr form))) 
    (while args 
      (byte-compile-form (car (cdr args))) 
      (if (null (cdr (cdr args))) 
	  (progn 
	    (byte-compile-out 'byte-dup 0) 
	    (setq byte-compile-maxdepth (max byte-compile-maxdepth (1+ byte-compile-depth))))) 
      (byte-compile-variable-ref 'byte-varset (car args)) 
      (setq args (cdr (cdr args)))))) 
 
(put 'let 'byte-compile 'byte-compile-let) 
(defun byte-compile-let (form) 
  (let ((varlist (car (cdr form)))) 
    (while varlist 
      (if (symbolp (car varlist)) 
	  (byte-compile-constant nil) 
	(byte-compile-form (car (cdr (car varlist))))) 
      (setq varlist (cdr varlist))) 
    (setq byte-compile-depth (- byte-compile-depth (length varlist)))) 
  (let ((varlist (reverse (car (cdr form))))) 
    (while varlist 
      (if (symbolp (car varlist)) 
	  (byte-compile-variable-ref 'byte-varbind (car varlist)) 
	(byte-compile-variable-ref 'byte-varbind (car (car varlist)))) 
      (setq varlist (cdr varlist)))) 
  (byte-compile-body (cdr (cdr form))) 
  (byte-compile-out 'byte-unbind (length (car (cdr form))))) 
 
(put 'let* 'byte-compile 'byte-compile-let*) 
(defun byte-compile-let* (form) 
  (let ((varlist (car (cdr form)))) 
    (while varlist 
      (if (symbolp (car varlist)) 
	  (byte-compile-constant nil) 
	(byte-compile-form (car (cdr (car varlist))))) 
      (if (symbolp (car varlist)) 
	  (byte-compile-variable-ref 'byte-varbind (car varlist)) 
	(byte-compile-variable-ref 'byte-varbind (car (car varlist)))) 
      (setq varlist (cdr varlist)))) 
  (byte-compile-body (cdr (cdr form))) 
  (byte-compile-out 'byte-unbind (length (car (cdr form))))) 
 
(put 'save-excursion 'byte-compile 'byte-compile-save-excursion) 
(defun byte-compile-save-excursion (form) 
  (byte-compile-out 'byte-save-excursion 0) 
  (byte-compile-body (cdr form)) 
  (byte-compile-out 'byte-unbind 1)) 
 
(put 'save-restriction 'byte-compile 'byte-compile-save-restriction) 
(defun byte-compile-save-restriction (form) 
  (byte-compile-out 'byte-save-restriction 0) 
  (byte-compile-body (cdr form)) 
  (byte-compile-out 'byte-unbind 1)) 
 
(put 'with-output-to-temp-buffer 'byte-compile 'byte-compile-with-output-to-temp-buffer) 
(defun byte-compile-with-output-to-temp-buffer (form) 
  (byte-compile-form (car (cdr form))) 
  (byte-compile-out 'byte-temp-output-buffer-setup 0) 
  (byte-compile-body (cdr (cdr form))) 
  (byte-compile-out 'byte-temp-output-buffer-show 0) 
  (setq byte-compile-depth (1- byte-compile-depth))) 
 
(put 'progn 'byte-compile 'byte-compile-progn) 
(defun byte-compile-progn (form) 
  (byte-compile-body (cdr form))) 
 
(put 'interactive 'byte-compile 'byte-compile-noop) 
(defun byte-compile-noop (form) 
  (byte-compile-constant nil)) 
 
(defun byte-compile-body (body) 
  (if (null body) 
      (byte-compile-constant nil) 
    (while body 
      (byte-compile-form (car body)) 
      (if (cdr body) 
	  (byte-compile-discard)) 
      (setq body (cdr body))))) 
 
(put 'prog1 'byte-compile 'byte-compile-prog1) 
(defun byte-compile-prog1 (form) 
  (byte-compile-form (car (cdr form))) 
  (if (cdr (cdr form)) 
      (progn 
	(byte-compile-body (cdr (cdr form))) 
	(byte-compile-discard)))) 
 
(put 'prog2 'byte-compile 'byte-compile-prog2) 
(defun byte-compile-prog2 (form) 
  (byte-compile-form (car (cdr form))) 
  (byte-compile-discard) 
  (byte-compile-form (nth 2 form)) 
  (if (cdr (cdr (cdr form))) 
      (progn 
	(byte-compile-body (cdr (cdr (cdr form)))) 
	(byte-compile-discard)))) 
 
(defun byte-compile-discard () 
  (byte-compile-out 'byte-discard 0) 
  (setq byte-compile-depth (1- byte-compile-depth))) 
 
(put 'if 'byte-compile 'byte-compile-if) 
(defun byte-compile-if (form) 
  (if (null (cdr (cdr form))) 
      ;; No else-forms 
      (let ((donetag (byte-compile-make-tag))) 
	(byte-compile-form (car (cdr form))) 
	(byte-compile-goto 'byte-goto-if-nil-else-pop donetag) 
	(setq byte-compile-depth (1- byte-compile-depth)) 
	(byte-compile-form (nth 2 form)) 
	(byte-compile-out-tag donetag)) 
    (let ((donetag (byte-compile-make-tag)) (elsetag (byte-compile-make-tag))) 
      (byte-compile-form (car (cdr form))) 
      (byte-compile-goto 'byte-goto-if-nil elsetag) 
      (setq byte-compile-depth (1- byte-compile-depth)) 
      (byte-compile-form (nth 2 form)) 
      (byte-compile-goto 'byte-goto donetag) 
      (byte-compile-out-tag elsetag) 
      (byte-compile-body (cdr (cdr (cdr form)))) 
      (byte-compile-out-tag donetag)))) 
 
(put 'cond 'byte-compile 'byte-compile-cond) 
(defun byte-compile-cond (form) 
  (if (cdr form) 
      (byte-compile-cond-1 (cdr form)))) 
(defun byte-compile-cond-1 (clauses) 
  (if (null (cdr clauses)) 
      ;; Only one clause 
      (let ((donetag (byte-compile-make-tag))) 
	(byte-compile-form (car (car clauses))) 
	(byte-compile-goto 'byte-goto-if-nil-else-pop donetag) 
	(setq byte-compile-depth (1- byte-compile-depth)) 
	(byte-compile-body (cdr (car clauses))) 
	(byte-compile-out-tag donetag)) 
    (let ((donetag (byte-compile-make-tag)) 
	  (elsetag (byte-compile-make-tag))) 
      (byte-compile-form (car (car clauses))) 
      (byte-compile-goto 'byte-goto-if-nil elsetag) 
      (setq byte-compile-depth (1- byte-compile-depth)) 
      (byte-compile-body (cdr (car clauses))) 
      (byte-compile-goto 'byte-goto donetag) 
      (byte-compile-out-tag elsetag) 
      (byte-compile-cond-1 (cdr clauses)) 
      (byte-compile-out-tag donetag)))) 
 
(put 'and 'byte-compile 'byte-compile-and) 
(defun byte-compile-and (form) 
  (let ((failtag (byte-compile-make-tag)) 
	(args (cdr form))) 
    (if (null args) 
	(byte-compile-form t) 
      (while args 
	(byte-compile-form (car args)) 
	(if (null (cdr args)) 
	    (byte-compile-out-tag failtag) 
	  (byte-compile-goto 'byte-goto-if-nil-else-pop failtag) 
	  (setq byte-compile-depth (1- byte-compile-depth))) 
	(setq args (cdr args)))))) 
 
(put 'or 'byte-compile 'byte-compile-or) 
(defun byte-compile-or (form) 
  (let ((wintag (byte-compile-make-tag)) 
	(args (cdr form))) 
    (if (null args) 
	(byte-compile-constant nil) 
      (while args 
	(byte-compile-form (car args)) 
	(if (null (cdr args)) 
	    (byte-compile-out-tag wintag) 
	  (byte-compile-goto 'byte-goto-if-not-nil-else-pop wintag) 
	  (setq byte-compile-depth (1- byte-compile-depth))) 
	(setq args (cdr args)))))) 
 
(put 'while 'byte-compile 'byte-compile-while) 
(defun byte-compile-while (form) 
  (let ((endtag (byte-compile-make-tag)) 
	(looptag (byte-compile-make-tag)) 
	(args (cdr (cdr form)))) 
    (byte-compile-out-tag looptag) 
    (byte-compile-form (car (cdr form))) 
    (byte-compile-goto 'byte-goto-if-nil-else-pop endtag) 
    (setq byte-compile-depth (1- byte-compile-depth)) 
    (byte-compile-body (cdr (cdr form))) 
    (byte-compile-discard) 
    (byte-compile-goto 'byte-goto looptag) 
    (byte-compile-out-tag endtag))) 
 
(put 'catch 'byte-compile 'byte-compile-catch) 
(defun byte-compile-catch (form) 
  (byte-compile-form (car (cdr form))) 
  (byte-compile-push-constant (byte-compile-top-level (cons 'progn (cdr (cdr form))))) 
  (byte-compile-out 'byte-catch 0)) 
 
(put 'save-window-excursion 'byte-compile 'byte-compile-save-window-excursion) 
(defun byte-compile-save-window-excursion (form) 
  (byte-compile-push-constant (list (byte-compile-top-level (cons 'progn (cdr form))))) 
  (byte-compile-out 'byte-save-window-excursion 0)) 
 
(put 'unwind-protect 'byte-compile 'byte-compile-unwind-protect) 
(defun byte-compile-unwind-protect (form) 
  (byte-compile-push-constant (list (byte-compile-top-level (cons 'progn (cdr (cdr form)))))) 
  (byte-compile-out 'byte-unwind-protect 0) 
  (byte-compile-form (car (cdr form))) 
  (byte-compile-out 'byte-unbind 1)) 
 
(put 'condition-case 'byte-compile 'byte-compile-condition-case) 
(defun byte-compile-condition-case (form) 
  (byte-compile-push-constant (car (cdr form))) 
  (byte-compile-push-constant (byte-compile-top-level (nth 2 form))) 
  (let ((clauses (cdr (cdr (cdr form)))) 
	compiled-clauses) 
    (while clauses 
      (let ((clause (car clauses))) 
	(setq compiled-clauses 
	      (cons (list (car clause) (byte-compile-top-level (cons 'progn (cdr clause)))) 
		    compiled-clauses))) 
      (setq clauses (cdr clauses))) 
    (byte-compile-push-constant (nreverse compiled-clauses))) 
  (byte-compile-out 'byte-condition-case 0)) 
 
(defun byte-compile-make-tag () 
  (cons nil nil)) 
 
(defun byte-compile-out-tag (tag) 
  (let ((uses (car tag))) 
    (setcar tag byte-compile-pc) 
    (while uses 
      (byte-compile-store-goto (car uses) byte-compile-pc) 
      (setq uses (cdr uses))))) 
 
(defun byte-compile-goto (opcode tag) 
  (byte-compile-out opcode 0) 
  (if (integerp (car tag)) 
      (byte-compile-store-goto byte-compile-pc (car tag)) 
    (setcar tag (cons byte-compile-pc (car tag)))) 
  (setq byte-compile-pc (+ byte-compile-pc 2))) 
 
(defun byte-compile-store-goto (at-pc to-pc) 
  (setq byte-compile-output 
	(cons (cons at-pc (logand to-pc 255)) 
	      byte-compile-output)) 
  (setq byte-compile-output 
	(cons (cons (1+ at-pc) (lsh to-pc -8)) 
	      byte-compile-output))) 
 
(defun byte-compile-out (opcode offset) 
  (setq opcode (eval opcode)) 
  (if (< offset 6) 
      (byte-compile-out-1 (+ opcode offset)) 
    (if (< offset 256) 
	(progn 
	  (byte-compile-out-1 (+ opcode 6)) 
	  (byte-compile-out-1 offset)) 
      (byte-compile-out-1 (+ opcode 7)) 
      (byte-compile-out-1 (logand offset 255)) 
      (byte-compile-out-1 (lsh offset -8))))) 
 
(defun byte-compile-out-const (offset) 
  (if (< offset byte-constant-limit) 
      (byte-compile-out-1 (+ byte-constant offset)) 
    (byte-compile-out-1 byte-constant2) 
    (byte-compile-out-1 (logand offset 255)) 
    (byte-compile-out-1 (lsh offset -8)))) 
 
(defun byte-compile-out-1 (code) 
  (setq byte-compile-output 
	(cons (cons byte-compile-pc code) 
	      byte-compile-output)) 
  (setq byte-compile-pc (1+ byte-compile-pc))) 
