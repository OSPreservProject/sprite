; This is a version of spell, originally written by Gosling but
;hacked by Harrison.
;The changes are that if there be some misspelled word or abbreviation
;which the spell routine returns as an error, say ''cs''. In some
;occurrences, this may be an error, in others an abbreviation, and in
;others a legitimate subword like in ``topics''.
;When prompted, hit e to exit.
; 	  	   r to go into a query-replace loop
; 		   n takes you to the next occurrence of the word; if there
;		     is none, you pick up the next error. 


(defun
    (correct-spelling-mistakes word action continue further
	(setq continue 1)
	(progn
	    (while continue
		(save-excursion
		    (get)
		)
		(beginning-of-file)
		(setq further 1)	; go further to find location of error
		(while further			; go looking for occurrence
		    (setq further 0); reset to no-further
		    (if (error-occured(search-forward word))
			(progn
			    (message (concat "No further occurrences of " word " found!" ))
			    (sit-for 10)
			)
			(progn
			    (message  (concat word " ? "))
			    (setq action (get-tty-character))
			    (if
				(= action 'e') (setq continue 0)
				(= action 'n') (setq further 1)	; search for more
				(= action 'r') (progn
						   (beginning-of-line)
						   (error-occured
						       (query-replace-string word
							   (get-tty-string
							       (concat word " => "))))
					       )
			    )
			)
		    )
		)
	    )
	)
	(novalue)
    )
    (get
	(temp-use-buffer "Error log")
	(beginning-of-file)
	(set-mark)
	(end-of-line)
	(setq word (region-to-string))
	(forward-character)
	(delete-to-killbuffer)
    )
)
(defun 
    (spell
	(message (concat "Looking for errors in " (current-file-name)
		     ", please wait..."))
	(sit-for 0)
	(save-excursion
		(compile-it (concat "cat " (current-file-name) " | detex | spell ")))
	(error-occured (correct-spelling-mistakes))
	(message "Done!")
	(novalue)
    )
)
