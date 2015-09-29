;;
;; This file, "TeX-spell.el", is a subsystem of "TeX-mode.el".
;;
;; Copyright (C) 1986, 1987	Pehong Chen (phc@renoir.berkeley.edu)
;; -- Please send bugs and comments to the above address
;;
;; <DISCLAIMER>
;; This program is still under development.  Neither the author nor 
;; Berkeley VorTeX group accepts responsibility to anyone for the consequences
;; of using it or for whether it serves any particular purpose or works at all.
;;

(defconst tex-spell-dict-buff "--- TeX Dictionary ---")
(defconst tex-spell-help-buff "--- TeX Spelling Help ---")
(defconst tex-spell-error-buff "--- TeX Spelling Errors ---")
(defconst tex-spell-hsl-suffix ".hsl"
  "Default filename extension for hashed spelling list.")
(defconst tex-spell-usl-suffix ".usl"
  "Default filename extension for unhashed spelling list.")

(defun tex-spell-document (&optional prefix)
  "Check spelling of every file included in current document.
With prefix argument, check spelling using a customized hashed spelling list,
if any, without query.  A customized hashed spelling list is either current
document master filename base concatenated with \".hsl\", or the file
bound to the variable tex-hsl-default."
  (interactive "P")
  (tex-check-master-file)
  (tex-check-document-type)
  (tex-get-include-files)
  (let ((lst tex-include-files)
	(asknot nil)
	msg fn fnnd)
    (catch 'finish
      (while lst
	(save-excursion
	  (set-buffer (find-file-noselect (setq fn (car lst))))
	  (setq msg (concat "Checking " tex-current-type " spelling for entire document"))
	  (setq fnnd (file-name-nondirectory fn))
	  (setq lst (append (tex-get-input-files msg fnnd) (cdr lst)))
	  (if (or asknot
		  (tex-confirm-visit
		    (concat "Check spelling for " fnnd " [SPC=(y)es  DEL=(n)o  RET=(a)ll  ESC=(d)one]? ")))
	    (progn
	      (find-file fn)
	      (message "%s, doing %s..." msg fnnd)
	      (sit-for 3)
	      (tex-spell-buffer prefix))))))
    (message "%s...done" msg)))

(defun tex-spell-buffer (&optional prefix)
  "Check spelling of every word in the buffer.
With prefix argument, check spelling using a customized hashed spelling list,
if any, without query.  A customized hashed spelling list is either current
document master filename base concatenated with \".hsl\", or the file
bound to the variable tex-hsl-default."
  (interactive "P")
  (tex-spell-region prefix 1 (point-max) (buffer-name)))

(defun tex-spell-region (&optional prefix start end &optional description)
  "Like tex-buffer-spell but applies only to region.
From program, applies from START to END.
With prefix argument, check spelling using a customized hashed spelling list,
if any, without query.  A customized hashed spelling list is either current
document master filename base concatenated with \".hsl\", or the file
bound to the variable tex-hsl-default."
  (interactive "P\nr")
  (let ((sm (set-marker (make-marker) start))
	(em (set-marker (make-marker) end)))
    (tex-check-document-type)
    (tex-check-master-file)			; defined in TeX-misc.el
    (if (file-exists-p tex-spell-filter)
      nil
      (error "Spelling filter \"%s\" does not exist" tex-spell-filter))
    (save-window-excursion
      (let ((=err= (get-buffer-create tex-spell-error-buff))
	    (=cmd= (get-buffer-create "*Shell Command Output*"))
	    (=buff= (current-buffer))
	    (=fn= (buffer-file-name))
	    (=msgs= (concat "Checking " tex-doc-type " spelling for " (or description "region")))
	    speller)
	(catch 'all-done
	  (while t
	    (setq start (marker-position sm))
	    (setq end (marker-position em))
	    (setq tex-spell-small-window nil)
	    (save-excursion
	      (set-buffer =err=)
	      (widen)
	      (erase-buffer))
	    (setq speller (tex-spell-get-speller prefix))
	    (if (= ?\n (char-after (1- end)))
	      (progn
		(call-process-region start end shell-file-name nil =err= nil "-c" tex-spell-filter)
		(save-excursion
		  (set-buffer =err=)
		  (call-process-region 1 (point-max) shell-file-name t =err= nil "-c" speller)))
	      (let ((filter tex-spell-filter)) ; will be nil in =err= if not assgined
		(save-excursion
		  (set-buffer =err=)
		  (insert-buffer-substring =buff= start end)
		  (insert ?\n)
		  (call-process-region 1 (point-max) shell-file-name t =err= nil "-c" filter)
		  (call-process-region 1 (point-max) shell-file-name t =err= nil "-c" speller))))
	    (tex-spell-examine prefix)))))))
       
(defun tex-spell-examine (prefix)
  (let ((case-fold-search t)
	(case-replace t)
	(down t)
	(first t)
	(master tex-master-file)
	word msgs reg-word new-word selected-word
	cmd next-cmd last-pos old-pos)
    (if (save-excursion
	  (set-buffer =err=)
	  (goto-char 1)
	  (> (buffer-size) 0))
      (progn
	(while (save-excursion
		 (if tex-spell-small-window
		   (progn
		     (delete-window)
		     (other-window -1)
		     (enlarge-window tex-slow-win-lines)
		     (setq tex-spell-small-window nil)))
		 (set-buffer =err=)
		 (not (or (= (buffer-size) 0)
			  (and down 
			       (eobp)
			       (if (tex-spell-exit master "next")
				 t
				 (previous-line 1)
				 nil))
			  (and (not down)
			       first
			       (tex-spell-exit master "previous")))))
	  (save-excursion
	    (set-buffer =err=)
	    (setq word (buffer-substring (point) (save-excursion (end-of-line) (point)))))
	  (tex-spell-get-cmd))
;;	(if (and (buffer-modified-p) (y-or-n-p (concat "Save file " =fn= "? ")))
;;	  (write-file =fn=))
	(if (y-or-n-p (concat =msgs= ", try again? "))
	  t
	  (message "%s...done" =msgs=)
	  (throw 'all-done t)))
      (message "%s...done (no errors found)" =msgs=)
      (throw 'all-done t))))

(defun tex-spell-get-cmd ()
  (setq msgs (concat "Erroneous `" word "' [SPC  DEL  n  p  r  R  w  C-r  ?=help]"))
  (setq reg-word (concat "\\b" (regexp-quote word) "\\b"))
  (setq last-pos (goto-char start))
  (setq next-cmd t)
  (setq selected-word nil)
  (catch 'get-cmd
    (while t
      (if next-cmd
	(progn
	  (message msgs)
	  (setq cmd (read-char))))
	(setq next-cmd t)
      (cond
	((= cmd ? )            ; Ignore and goto next error
	 (save-excursion
	   (set-buffer =err=)
	   (next-line 1)
	   (setq first nil)
	   (setq down t))
	 (throw 'get-cmd t))
	((= cmd ?\177)         ; Ignore and goto previous
	 (save-excursion
	   (set-buffer =err=)
	   (if (bobp)
	     (setq first t)
	     (previous-line 1))
	   (setq down nil))
	 (throw 'get-cmd t))
	((= cmd ?n)                        ; Next instance
	 (setq old-pos (point))
	 (goto-char last-pos)
	 (if (tex-spell-search reg-word end)
	   (progn
	     (setq last-pos (point))
	     (goto-char (match-beginning 0)))
	   (goto-char old-pos)
	   (message "Wrapping around...")
	   (sit-for 1)
	   (goto-char start)            ; Wrap-around
	   (tex-spell-search reg-word end)      ; Must be there
	   (setq last-pos (point))
	   (goto-char (match-beginning 0))))
	((= cmd ?p)                      	; Previous instance
	 (if (tex-spell-search reg-word start t)	; Backward
	   (save-excursion
	     (goto-char (match-end 0))
	     (setq last-pos (point)))
	   (message "Wrapping around...")
	   (sit-for 1)
	   (goto-char end)            ; Wrap-around
	   (tex-spell-search reg-word start t)    ; Must be there
	   (save-excursion
	     (goto-char (match-end 0))
	     (setq last-pos (point)))))
	((= cmd ?r)                      	 ; Replace with word
	 (setq new-word (read-string
			  (concat "Replacing `" word "' by: ")
			  word))
	 (save-excursion
	   (catch 'abort
	     (tex-spell-replace 'abort end reg-word word new-word t)
	     (save-excursion
	       (set-buffer =err=)
	       (delete-region (point) (progn (next-line 1) (point)))
	       (if (bobp) (setq down t)))
	     (throw 'get-cmd t))))
	((= cmd ?R)                         ; Replace w/o word
	 (setq new-word (read-string 
			  (concat "Replacing `" word "' by: ")
			  selected-word))
	 (save-excursion
	   (catch 'abort
	     (tex-spell-replace 'abort end reg-word word new-word t)
	     (save-excursion
	       (set-buffer =err=)
	       (delete-region (point) (progn (next-line 1) (point)))
	       (if (bobp) (setq down t)))
	     (throw 'get-cmd t))))
	((= cmd ?w)		  	; Dictionary lookup
	 (catch 'abort
	   (tex-spell-word word 'abort)))
	((= cmd ?\^r)
	 (message "Entering recursive edit...(return to spelling checking by ESC C-c)")
	 (let ((win (get-buffer-window tex-spell-dict-buff)))
	   (if win
	     (let ((pos (point)) left)
	       (pop-to-buffer tex-spell-dict-buff)
	       (recursive-edit)
	       (if (eq win (get-buffer-window (buffer-name)))
		 (progn
		   (save-excursion
		     (forward-word 1)
		     (backward-word 1)
		     (setq left (point)))
		   (backward-word 1)
		   (insert ?[)
		   (setq left (point))
		   (forward-word 1)
		   (setq selected-word (buffer-substring left (point)))
		   (insert ?])
		   (pop-to-buffer =buff=)
		   (setq next-cmd nil)
		   (message msgs)
		   (setq cmd (read-char))
		   (pop-to-buffer tex-spell-dict-buff)
		   (delete-window))))
	     (recursive-edit))))
	((= cmd ??)
	 (save-window-excursion
	   (tex-spell-help)
	   (setq next-cmd nil)
	   (message msgs)
	   (setq cmd (read-char))))
	(t
	 (ding)
	 (message "%s...WHAT?" msgs)
	 (sit-for 1))))))

(defun tex-spell-help ()
  (pop-to-buffer tex-spell-help-buff)
  (if (> (buffer-size) 0)
    nil
    (insert-string
"SPC -- Ignore current erroneous word, try next error, if any.
DEL -- Ignore current erroneous word and try the previous, if any. 
  n -- Go to next instance of the word in buffer, wrap around if necessary.
  p -- Go to previous instance of the word in buffer, wrap around if necessary.
  r -- Replace all instances of the word below dot.
       A repetition of current erroneous word appears at replacement prompt.
  R -- Replace all instances of the word below dot.
       If a word is selected in --- TeX Dictionary ---, it is repeated at 
       prompt; otherwise nothing is repeated.
  w -- Dictionary lookup for words containing the specified substring.
       Result displayed in the other window called --- TeX Dictionary ---.
C-r -- Enter recursive edit.  Return to spelling checking by ESC C-c.
  ? -- This help message.
C-g -- Abort to top level."))
  (enlarge-window (- 15 (window-height)))
  (goto-char 1)
  (other-window 1))
      
(defun tex-spell-exit (master scroll)
  (let (cmd)
    (catch 'done
      (while t
	(message "No more %s entry, exit? [SPC=(y)es  DEL=(n)o  RET=(s)ave]" scroll)
	(setq cmd (read-char))
        (cond ((or (= cmd ? ) (= cmd ?y))
	       (throw 'done t))
	      ((or (= cmd ?\177) (= cmd ?n))
	       (throw 'done nil))
	      ((or (= cmd ?\r) (= cmd ?s))
	       (tex-spell-save-hsl master)
	       (throw 'done t))
	      (t
	       (setq cmd nil)
	       (ding)
	       (message "No more %s entry, exit? (SPC/y  DEL/n  RET/s)...WHAT?" scroll)
	       (sit-for 1)))))))

(defun tex-spell-check-hsl (fn fnnd msgs)
  (save-excursion
    (message "%s, verifying spelling list %s..." msgs fnnd)
    (set-buffer =cmd=)
    (erase-buffer)
    (call-process shell-file-name nil t nil "-c" (concat "echo \"am\" | spellout " fn))
    (if (= (buffer-size) 0)
      t
      (message "Spelling list is corrupted, type any key to continue" fnnd)
      (read-char)
      (message "Removing corrupted spelling list %s..." fn)
      (shell-command (concat "rm " fn))
      (message "Removing corrupted spelling list %s...done" fn)
      (sit-for 3)
      (message "%s..." msgs)
      nil)))

(defun tex-spell-get-speller (prefix)
   "Make a hashed spelling list."
   (let* ((hl (tex-spell-ext =fn= tex-spell-hsl-suffix))
	  (hlnd (file-name-nondirectory hl))
          (ul (tex-spell-ext =fn= tex-spell-usl-suffix))
	  (ulnd (file-name-nondirectory ul))
	  (default ""))
     (if (and (or (file-exists-p hl)
		  (and tex-hsl-default
		       (file-exists-p
			 (setq hl (expand-file-name tex-hsl-default)))
	               (setq hlnd (file-name-nondirectory hl))))
	      (file-newer-than-file-p hl ul)
	      (or prefix
	          (y-or-n-p (concat "Use " hlnd " as hashed spelling list? ")))
	      (tex-spell-check-hsl hl hlnd =msgs=))
       (progn
	 (message "%s, using %s as spelling list..." =msgs= (file-name-nondirectory hl))
         (concat tex-speller " -d " hl))
       (if (and (or (file-exists-p ul)
		    (and tex-usl-default
			 (file-exists-p
			   (setq ul (expand-file-name tex-usl-default)))
			 (setq ulnd (file-name-nondirectory ul))))
		(or prefix
		    (y-or-n-p (concat "Use " ulnd " as unhashed spelling list? "))))
	 (progn
	   (message "%s, creating %s from %s..." =msgs= hlnd ulnd)
	   (if (and tex-usl-default (file-exists-p tex-usl-default))
	     (setq default tex-usl-default))
	   (call-process shell-file-name nil =cmd= nil "-c"
	     (concat "cat " ul " " default "|" tex-spellout " " tex-hsl-global
		     "|" tex-spellin " " tex-hsl-global ">" hl))
	   (message "%s, using %s as spelling list..." =msgs= hlnd)
	   (concat tex-speller " -d " hl))
	 (message "%s..." =msgs=)
	 tex-speller))))

(defun tex-spell-ext (fn suffix)
  (let* ((fnnd (file-name-nondirectory fn))
	 (dir (file-name-directory fn))
	 (pos (string-match "\\." fnnd)))
    (if (and pos (> pos 0))
      (concat (or dir "") (substring fnnd 0 pos) suffix)
      (concat fn suffix))))

(defun tex-spell-check-words ()
  (if (save-excursion
	(set-buffer =err=)
	(goto-char 1)
	(> (buffer-size) 0))
    (if (y-or-n-p "Reexamine uncorrected words before saving? ")
      (let (word msgs cmd next-cmd)
	(save-excursion
	  (set-buffer =err=)
	  (goto-char 1)
	  (catch 'checked
	    (while (> (buffer-size) 0)
	      (beginning-of-line)
	      (if (eobp)
		(if (y-or-n-p "No more next word, done? ")
		  (throw 'checked t)
		  (previous-line 1)))
	      (setq word (buffer-substring
			   (point) (save-excursion (end-of-line) (point))))
	      (if (string-equal word "")
		(kill-line 1))
	      (tex-spell-cw-cmd))
	    (message "No uncorrected words left.")
	    (sit-for 3)
	    (throw 'done t)))))
    (message "No uncorrected words left.")
    (sit-for 3)
    (throw 'done t)))

(defun tex-spell-cw-cmd ()
  (setq msgs (concat "Looking at `" word "'  [SPC=(n)ext  DEL=(p)revious  RET=(r)emove  ESC=(d)one]"))
  (catch 'get-cmd
    (while t
      (message msgs)
      (setq next-cmd t)
      (setq cmd (read-char))
      (cond
	((or (= cmd ? ) (= cmd ?n))            ; Ignore and goto next error
	 (next-line 1))
	((or (= cmd ?\177) (= cmd ?p))        ; Ignore and goto previous
	 (if (bobp)
	   (if (y-or-n-p "No more previous word, done? ")
	     (throw 'checked t))
	   (previous-line 1)))
	((or (= cmd ?\r) (= cmd ?r))
	 (kill-line 1)
	 (if (eobp)
	   (previous-line 1)))
	((or (= cmd ?\e) (= cmd ?d))
	 (throw 'checked t))
	(t
	 (setq next-cmd nil)
	 (ding)
	 (message "%s...WHAT?" msgs)
	 (sit-for 1)))
      (if next-cmd
	(throw 'get-cmd t)))))
  
(defun tex-spell-save-hsl (master)
  (tex-spell-check-words)
  (let* ((hl (tex-spell-ext master tex-spell-hsl-suffix))
	 (hlnd (file-name-nondirectory hl))
	 (hl-p (file-exists-p hl))
	 (ul (tex-spell-ext master tex-spell-usl-suffix))
	 (ulnd (file-name-nondirectory ul))
	 (ul-p (file-exists-p ul))
	 (newer (file-newer-than-file-p hl ul))
	 (hl-g (and hl-p
		    (tex-spell-check-hsl hl hlnd "Saving spelling list")))
	 (default ""))
    (if ul-p
      (progn
	(message "Extending unhashed spelling list %s..." ulnd)
	(call-process-region 1 (point-max) shell-file-name nil =cmd= nil "-c"
	  (concat "sort -u " ul "> /tmp/" ulnd "; mv /tmp/" ulnd " ."))
	(message "Extending unhashed spelling list %s...done" ulnd))
      (message "Creating unhashed spelling list %s..." ulnd)
      (write-region 1 (point-max) ul nil 'no-message)
      (message "Creating unhashed spelling list %s...done" ulnd))
    (if (and newer hl-g)
     (progn
       (message "Extending hashed spelling list %s..." hlnd)
       (call-process-region 1 (point-max) shell-file-name nil =cmd= nil "-c"
	 (concat "cat |" tex-spellout " " hl "|" tex-spellin " " hl
		   ">" "/tmp/" hlnd "; mv /tmp/" hlnd " ."))
       (message "Extending hashed spelling list %s...done" hlnd)
       (sit-for 3))
     (message "Creating hashed spelling list %s from %s..." hlnd ulnd)
     (if (and tex-usl-default (file-exists-p tex-usl-default))
       (setq default tex-usl-default))
     (call-process shell-file-name nil =cmd= nil "-c"
       (concat "cat " ul " " default "|" tex-spellout " " tex-hsl-global
	       "|" tex-spellin " " tex-hsl-global ">" hl))
     (message "Creating hashed spelling list %s from %s...done" hlnd ulnd)
     (sit-for 3))))

(defun tex-spell-word (&optional pre &optional abort)
  "Check the spelling of a word.  If PRE is non-nil, use it as default.
If pre is a non-string and is set by prefix argument, then use the
preceding word as default."
  (interactive "P")
  (let ((old (current-buffer))
	(tmp (get-buffer-create tex-spell-dict-buff))
	(offset 0)
	(dict nil)
	key m i fix prefix suffix msg bow)
    (catch 'loop
      (while t
        (message "Lookup string as prefix, infix, or suffix? (RET/p i s)")
	(setq fix (read-char))
	(cond
	  ((or (= fix ?\r) (= fix ?p))
	   (setq prefix "")
	   (setq suffix "")
	   (setq fix "prefix")
	   (throw 'loop t))
	  ((= fix ?i)
	   (setq dict t)
	   (setq prefix "")
	   (setq suffix "")
	   (setq fix "infix")
	   (throw 'loop t))
	  ((= fix ?s)
	   (setq dict t)
	   (setq prefix "^.*")
	   (setq suffix "$")
	   (setq fix "suffix")
	   (throw 'loop t))
	  (t
	   (ding)
           (message "Lookup string as prefix, infix, or suffix (RET/p i s) WHAT?")
	   (sit-for 1)))))
    (if (and pre (not (stringp pre)))
      (save-excursion
	(backward-word 1)
        (setq bow (point))
	(forward-word 1)
	(setq pre (buffer-substring bow (point)))))
    (setq key (read-string (concat "String as " fix ": ") pre))
    (if (string-equal key "")
      (if abort
	(progn
	  (ding)
	  (message "Don't know how to lookup a null string.")
	  (throw abort nil))
        (error "Don't know how to lookup a null string."))
      (setq msg (concat "Looking up `" key "' as " fix " in \"" tex-dictionary "\"..."))
      (setq key (concat prefix key suffix)))
    (message msg)
    (set-buffer tmp)
    (erase-buffer)
    (if dict
      (call-process tex-egrep nil t nil key tex-dictionary)
      (call-process tex-look nil t nil key))
    (if (= (buffer-size) 0)
      (progn
	(ding)
        (message "%sfailed" msg)
	(sit-for 1))
      (goto-char 1)
      (while (not (eobp))
	(end-of-line)
	(setq offset (max offset (+ (current-column) 5)))
	(next-line 1))
      (setq m (/ (window-width) offset))
      (goto-char 1)
      (while (not (eobp))
	(setq i 1)
	(while (and (< i m) (not (eobp)))
	  (end-of-line)
	  (delete-char 1)
	  (indent-to (1+ (* i offset)))
	  (setq i (1+ i)))
	(next-line 1))
      (goto-char 1)
      (pop-to-buffer tmp)
      (message "%sdone" msg))
    (pop-to-buffer old)))

(defun tex-spell-replace (cont bound from-regexp from-string to-string query-flag)
  (let ((nocasify (not (and case-fold-search 
			    case-replace
			    (string-equal from-string (downcase from-string)))))
	(literal nil)
	(keep-going t))
    (push-mark)
    (push-mark)
    (while (and keep-going
		(not (eobp))
		(progn
		  (set-mark (point))
		  (tex-spell-search from-regexp bound)))
      (undo-boundary)
      (if (not query-flag)
	  (replace-match to-string nocasify literal)
	(let (done replaced)
	  (while (not done)
	    (message "Query replacing `%s' with `%s' (^C if to quit prematurely)..." from-string to-string)
	    (setq char (read-char))
	    (cond
	      ((= char ?\^c)
	       (ding)
	       (message "Quitting query replacing...(return to previous level)")
	       (sit-for 1)
	       (throw cont nil))
	      ((= char ?\e)
	       (setq keep-going nil)
	       (setq done t))
	      ((= char ?^)
	       (goto-char (mark))
	       (setq replaced t))
	      ((= char ?\ )
	       (or replaced (replace-match to-string nocasify literal))
	       (setq done t))
	      ((= char ?\.)
	       (or replaced (replace-match to-string nocasify literal))
	       (setq keep-going nil)
	       (setq done t))
	      ((and (not replaced) (= char ?\,))
	       (replace-match to-string nocasify literal)
	       (setq replaced t))
	      ((= char ?!)
	       (or replaced (replace-match to-string nocasify literal))
	       (setq done t query-flag nil))
	      ((= char ?\177)
	       (setq done t))
	      ((= char ?\^r)
	       (store-match-data
		 (prog1 (match-data)
			(save-excursion (recursive-edit)))))
	      ((= char ?\^w)
	       (delete-region (match-beginning 0) (match-end 0))
	       (store-match-data
		 (prog1 (match-data)
		        (save-excursion (recursive-edit))))
	       (setq done t))
	      (t
	       (setq keep-going nil)
	       (setq unread-command-char char)
	       (setq done t)))))))
    keep-going))

(defvar tex-slow-speed 2400)
(defvar tex-slow-win-lines 1)
(defvar tex-spell-small-window nil "Flag designating small window")
(defvar tex-spell-large-window-begin 0)
(defvar tex-spell-large-window-end 0)

(defun tex-spell-search (regexp bound &optional backward)
  (let ((search-fun (if backward 're-search-backward 're-search-forward))
	(slow-terminal-mode (<= (baud-rate) tex-slow-speed))
	(window-min-height (min window-min-height (1+ tex-slow-win-lines)))
	(found-dot nil))	; to restore dot from a small window
    (if (funcall search-fun regexp bound t)
      (progn
        (setq found-dot (point))
        (if (not tex-spell-small-window)
	  (progn
	    (setq tex-spell-large-window-begin 
	      (save-excursion 
		(move-to-window-line 0)
		(beginning-of-line)
		(point)))
	    (setq tex-spell-large-window-end
	      (save-excursion 
		(move-to-window-line -1)
		(end-of-line)
		(point)))))
        (if (and slow-terminal-mode
		 (not (or tex-spell-small-window (pos-visible-in-window-p))))
	  (progn
	    (setq tex-spell-small-window t)
	    (move-to-window-line 0)
	    (split-window nil (- (window-height) (1+ tex-slow-win-lines)))
	    (other-window 1)))
	(goto-char found-dot)
	(if (and tex-spell-small-window
		 (>= found-dot tex-spell-large-window-begin)
		 (<= found-dot tex-spell-large-window-end))
	  (progn
	    (setq tex-spell-small-window nil)
	    (delete-window)
	    (other-window -1)
	    (enlarge-window tex-slow-win-lines)
	    (goto-char found-dot)))
 	t))))
