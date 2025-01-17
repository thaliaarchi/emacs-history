;; Display time and load in mode line of Emacs.
;; Copyright (C) 1985 Richard M. Stallman.

;; This file is part of GNU Emacs.

;; GNU Emacs is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY.  No author or distributor
;; accepts responsibility to anyone for the consequences of using it
;; or for whether it serves any particular purpose or works at all,
;; unless he says so in writing.  Refer to the GNU Emacs General Public
;; License for full details.

;; Everyone is granted permission to copy, modify and redistribute
;; GNU Emacs, but only under the conditions described in the
;; GNU Emacs General Public License.   A copy of this license is
;; supposed to have been given to you along with GNU Emacs so you
;; can know your rights and responsibilities.  It should be in a
;; file named COPYING.  Among other things, the copyright notice
;; and this notice must be preserved on all copies.


(defvar display-time-process nil)

(defun display-time ()
  "Display current time and load level in mode line of each buffer.
Updates automatically every minute."
  (interactive)
  (let ((live (and display-time-process
		   (eq (process-status display-time-process) 'run))))
    (if (not live)
	(save-excursion
	  (if display-time-process
	      (delete-process display-time-process))
	  (setq global-mode-string "time and load")
	  (setq display-time-process
		(start-process "display-time" (current-buffer)
			       (expand-file-name "loadst" exec-directory)
			       "-n" "60"))
	  ;; Give process no buffer.
	  (set-process-buffer display-time-process nil)
	  (process-kill-without-query display-time-process)
	  (set-process-sentinel display-time-process 'display-time-sentinel)
	  (set-process-filter display-time-process 'display-time-filter)))))

(defun display-time-sentinel (proc reason)
  (or (eq (process-status proc) 'run)
      (setq global-mode-string ""))
  ;; Force mode-line updates
  (save-excursion (set-buffer (other-buffer)))
  (set-buffer-modified-p (buffer-modified-p))
  (sit-for 0))

(defun display-time-filter (proc string)
  ;; Desired data can't need more than the last 30 chars,
  ;; so save time by flushing the rest.
  ;; This way, if we have many different times all collected at once,
  ;; we can discard all but the last few very fast.
  (if (> (length string) 30)
      (setq string (substring string -30)))
  ;; Now discard all but the very last one.
  (let (idx)
    (while (setq idx (string-match "\\]." string))
      (setq string (substring string (1+ idx)))))
  ;; Append the date if desired.
  (if display-time-day-and-date
      (setq string (concat (substring (current-time-string) 0 11) string)))
  ;; Install the new time for display.
  (setq global-mode-string string)
  ;; Force redisplay of all buffers' mode lines to be considered.
  (save-excursion (set-buffer (other-buffer)))
  (set-buffer-modified-p (buffer-modified-p))
  ;; Do redisplay right now, if no input pending.
  (sit-for 0))
