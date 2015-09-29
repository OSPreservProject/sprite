% BibTeX `named' style-file version 1
%	for BibTeX version 0.98i or later, LaTeX version 2.09
%	by Peter F. Patel-Schneider
% This style is NOT guaranteed to work.  It is provided in the hope
% that it will make the preparation of papers for AAAI-87 easier. 
%
% There are undoubtably bugs in this style.  If you make bug fixes,
% improvements, etc.  please let me know.  My e-mail address is:
%	pfps@spar.slb.com
%
% Modified from:
% BibTeX standard bibliography style `alpha'
	% version 0.98c for BibTeX versions 0.98i or later, LaTeX version 2.08
	% Copyright (C) 1985, all rights reserved
	% Copying of this file is authorized only if either
	% (1) you make absolutely no changes to your copy, including name, or
	% (2) if you do make changes, you name it something other than
	% btxbst.doc, plain.bst, unsrt.bst, alpha.bst, and abbrv.bst
	% This restriction helps ensure that all standard styles are identical
%
%   Entry formatting: Similar to that recommended by Mary-Claire van Leunen
%	in "A Handbook for Scholars".  Book-like titles are italicized,
%	non-book titles are converted to sentence capitilization
%	(and not enclosed in quotes).  Either the "open" or the "closed"
%	format can be used, with the appropriate definition of
%	\newblock (the name \newblock is analogous to the names \newline
%	and \newpage).  The "open" format has a line break before each
%	major block of an entry, and lines after the first are indented
%	within blocks.  The "closed" format just runs the blocks
%	together.  This file outputs a \newblock between blocks.
%	It uses the closed format.
%
%   Citation format: [author-last-name, year]
%		     [author-last-name and author-last-name, year]
%		     [author-last-name {\em et al.}, year]
%
%   Reference list ordering: alphabetical by author or whatever passes
%	for author in the absence of one.
%
% This BibTeX style has support for short (year only) citations.  This
% is done by having the citations actually look like
%         \citename{name-info, }year
% The LaTeX style has to have the following
%     \let\@internalcite\cite
%     \def\cite{\def\citename##1{##1}\@internalcite}
%     \def\shortcite{\def\citename##1{}\@internalcite}
%     \def\@biblabel#1{\def\citename##1{##1}[#1]\hfill}
% which makes \shortcite the macro for short citations.

%   History
%   12/16/84	(HWT)	Original version, by Howard Trickey.
%   12/23/84	(LL)	Some comments made by Leslie Lamport.
%    2/12/85	(OP)	Changes based primarily on LL's comments
%    1/07/85	(pfps)	Change to named style plus some enhancements.
%    9/15/85	(pfps)  Added support for short (year only) citations.

% The ENTRY declaration
%   Like Scribe's (according to pages 231-2 of the April '84 edition),
%   but no fullauthor or editors fields because BibTeX does name handling.
%   The annote field is commented out here because this isn't an annotated
%   bibliography style

ENTRY {			% Fields:
    address		% Publisher's address
%    annote		% Long annotation used for annotated bibliographies
			% (begins sentence)
    author		% Name(s) of author(s), in BibTeX name format
    booktitle		% Title of a book when the thing being referenced
			% isn't the whole book.  For book entries, the title
			% field should be used instead.
    chapter		% Chapter number
    edition		% Edition of a book (e.g., "second")
    editor		% Name(s) of editor(s), in BibTeX name format
			% If there is also an author field, then the editor
			% field should be for the book or collection that the
			% work appears in
    howpublished	% How something strange has been published
			% (begins sentence)
    institution		% Sponsoring institution
    journal		% Journal name (macros are provided for many)
    key			% Alphabetizing (and perhaps labeling) key
			% (needed when no author or editor)
    location		% Location for a conference
    month		% Month (macros are provided)
    note		% Help in finding a reference (begins sentence)
    number		% Number of a journal or technical report
    organization	% Organization (sponsoring a conference)
    pages		% Page number or numbers (use `--' to separate a range)
    publisher		% Publisher name
    school		% School name (for theses)
    series		% The name of a series or set of books; an individual
			% book will will also have it's own title
    title		% Title
    type		% Type of a Techreport (e.g., "Research Note")
			% to be used instead of default "Technical Report"
    volume		% Volume of a journal or multivolume work
    year		% Year
  }
  {}			% No integer entry variables
  { label extralabel }	% String entry variables used to form citation label

% Each entry function starts by calling output.bibitem, to write the
% \bibitem and its arguments to the .bbl file.  Then the various fields
% are formatted and printed by output or output.check.  Those functions
% handle the writing of separators (commas, periods, \newblock's),
% taking care not to do so when they are passed a null string.
% Finally, fin.entry is called to add the final period and finish the
% entry.
%
% A bibliographic reference is formatted into a number of `blocks':
% in the open format, a block begins on a new line and subsequent
% lines of the block are indented.  A block may contain more than
% one sentence (well, not a grammatical sentence, but something to
% be ended with a sentence ending period).  The entry functions should
% call new.block whenever a block other than the first is about to be
% started.  They should call new.sentence whenever a new sentence is
% to be started.  The output functions will ensure that if two
% new.sentence's occur without any non-null string being output between
% them then there won't be two periods output.  Similarly for two
% successive new.block's.

% The output routines don't write their argument immediately.
% Instead, by convention, that argument is saved on the stack to be
% output next time (when we'll know what separator needs to come
% after it).  Meanwhile, the output routine has to pop the pending
% output off the stack, append any needed separator, and write it.
%
% To tell which separator is needed, we maintain an output.state.
% It will be one of these values:
%	before.all		just after the \bibitem
%	mid.sentence		in the middle of a sentence: comma needed
%					if more sentence is output
%	after.sentence		just after a sentence: period needed
%	after.block		just after a block (and sentence):
%					period and \newblock needed.
% Note: This style (plain) doesn't use after.sentence
%
% VAR: output.state : INTEGER		-- state variable for output
%
% The output.nonnull function saves its argument (assumed to be nonnull)
% on the stack, and writes the old saved value followed by any needed
% separator.  The ordering of the tests is decreasing frequency of
% occurrence.
%
% output.nonnull(s) ==
%  BEGIN
%	s := argument on stack
%	if output.state = mid.sentence then
%	    write$(pop() * ", ")
%		  -- "pop" isn't a function: just use stack top
%	else
%	    if output.state = after.block then
%		write$(add.period$(pop()))
%		newline$
%		write$("\newblock ")
%	    else
%		if output.state = before.all then
%		    write$(pop())
%		else	    -- output.state should be after.sentence
%		    write$(add.period$(pop()) * " ")
%		fi
%	    fi
%	fi
%	push s on stack
%	output.state := mid.sentence
%  END
%
% The output function calls output.nonnull if its argument is non-null
%
% output(s) ==
%  BEGIN
%	if s <> "" then output.nonnull(s)
%	fi
%  END
%
% The output.check function calls output.nonnull if s is non-null
% and warns the user that the t field shouldn't be empty (this is
% because it won't be a good reference without the field;  the entry
% functions try to make the formatting look reasonable even when such
% fields are empty).
%
% output.check(t,s) ==
%  BEGIN
%	if s = "" then
%	    top$("Warning: the " * t * " shouldn't be empty in " * cite$)
%	else output.nonnull(s)
%	fi
%  END
%
% The output.bibitem function writes the \bibitem for the current entry
% (the label should already have been set up), and sets up the separator
% state for the output functions.  And, it leaves a string on the stack
% as per the output convention.
%
% output.bibitem ==
%  BEGIN
%	newline$
%	write$("\bibitem[")	% for nonnumeric labels,
%	write$(label)		% these three lines should be used
%	write$("]{")		% instead of the following line
%	write$("\bibitem{")
%	write$(cite$)
%	write$("}")
%	push "" on stack
%	output.state := before.all
%  END
%
% The fin.entry function finishes off an entry by adding a period to the
% string remaining on the stack.  If the state is still before.all
% then nothing was produced for this entry, so the result will look bad,
% but the user deserves it. (We don't omit the whole entry because the
% entry was cited, and a bibitem is needed to define the citation label.)
%
% fin.entry ==
%  BEGIN
%	write$(add.period$(pop()))
%	newline$
%  END
%
% The new.block function prepares for a new block to be output, and
% new.sentence prepares for a new sentence.
%
% new.block ==
%  BEGIN
%	if output.state <> before.all then
%	    output.state := after.block
%	fi
%  END
%
% new.sentence ==
%  BEGIN
%	if output.state <> after.block then
%	    if output.state <> before.all then
%		output.state :=  after.sentence
%	    fi
%	fi
%  END
%

INTEGERS { output.state before.all mid.sentence after.sentence after.block }

FUNCTION{init.state.consts}
{ 'before.all #0 :=
  'mid.sentence #1 :=
  'after.sentence #2 :=
  'after.block #3 :=
}

STRINGS { s t }		    % temporary string holders

FUNCTION{output.nonnull}
{ 's swap$ :=
  output.state mid.sentence =
    { ", " * write$
    }
    { output.state after.block =
	{ add.period$ write$
	  newline$
	  "\newblock " write$
	}
	{ output.state before.all =
	    { write$ }
	    { add.period$ " " * write$ }
	  if$
	}
      if$
    }
  if$
  'output.state mid.sentence :=
  s
}

FUNCTION{output}
{ 's swap$ :=
  s "" =
    'skip$
    { s output.nonnull
    }
  if$
}

FUNCTION{output.check}
{ 's swap$ :=
  't swap$ :=
  s "" =
    { "Warning: the " t * " shouldn't be empty in " * cite$ * top$
    }
    { s output.nonnull
    }
  if$
}

FUNCTION{output.bibitem}
{ newline$
  "\bibitem[" write$	% for nonnumeric labels,
  label write$		% these three lines should be used
  "]{" write$		% instead of the following line
%  "\bibitem{" write$				- pfps
  cite$ write$
  "}" write$
  newline$
  ""
  'output.state before.all :=
}

% This function finishes all entries.  Note: For an otherwise empty entry
% (which is probably due to a user error) this function prints a period

FUNCTION{fin.entry}
{ add.period$
  write$
  newline$
}

FUNCTION{new.block}
{ output.state before.all =
    'skip$
    { 'output.state after.block := }
  if$
}

% This function isn't used in this style (plain)

FUNCTION{new.sentence}
{ output.state after.block =
    'skip$
    { output.state before.all =
	'skip$
	{ 'output.state after.sentence := }
      if$
    }
  if$
}

% These three functions pop one or two (integer) arguments from the stack
% and push a single one, either 0 or 1.

FUNCTION{not}
{   { #0 }
    { #1 }
  if$
}

FUNCTION{and}
{   'skip$		% the corresponding if$ would be idempotent
    { pop$ #0 }
  if$
}

FUNCTION{or}
{   { pop$ #1 }
    'skip$		% the corresponding if$ would be idempotent
  if$
}

% Here are some functions for formatting chunks of an entry.
% By convention they either produce a string that can be followed by
% a comma or period (using add.period$, so it is OK to end in a period),
% or they produce the null string.
%
% A useful utility is the field.or.null function, which checks if the
% argument is the result of pushing a `missing' field (one for which no
% assignment was made when the current entry was read in from the database),
% and returns the null string if so, otherwise it returns the field string.
%
% field.or.null(s) ==
%  BEGIN
%	if missing$(s) then return ""
%	else return s
%  END
%
% Another helper function is italicize, which returns the string that
% italicizes the argument string, if that is non-null, otherwise it
% returns the null string.  Italic corrections aren't used, so this
% function should be used when punctation will follow the result.
% Also, it needn't be \em (instead of \it) because the user shouldn't be
% emphasizing the whole bibliography.
%
% italicize(s) ==
%  BEGIN
%	if s = "" then return ""
%	else return "{\it " * s * "}"
%
% The format.names function formats the argument (which should be in
% BibTeX name format) into "First Von Last, Junior", separated by commas
% and with an "and" before the last.
%
% VAR: nameptr, namesleft, numnames: INTEGER
%      nameresult: STRING
%
% format.names(s) ==
%  BEGIN
%	nameptr := 1
%	nameresult := ""
%	numnames := num.names$(s)
%	namesleft := numnames
%	while namesleft > 0
%	  do
%	    if nameptr > 1 then
%		if namesleft > 0 then nameresult := nameresult * ", "
%		else if numnames > 2
%		       then nameresult := nameresult * ", and "
%		       else nameresult := nameresult * " and "
%		     fi
%		fi
%	    fi
%	    nameresult := nameresult *
%		format.names$(s, nameptr, "{ff }{vv }{ll}{, jj}")
%	    nameptr := nameptr + 1
%	  od
%	return nameresult
%  END
%
% The format.authors function returns the result of format.names(author)
% if the author is present, or else it returns the null string
%
% format.authors ==
%  BEGIN
%	if missing$(author) then return ""
%	else return format.names(author)
%	fi
%  END
%
% Format.editors is like format.authors, but it uses the editor field,
% and appends ", editor" or ", editors"
%
% format.editors ==
%  BEGIN
%	if missing$(editor) then return ""
%	else
%	    if num.names$(editor) > 1 then
%		return format.names(editor) * ", editors"
%	    else
%		return format.names(editor) * ", editor"
%	    fi
%	fi
%  END
%
% Other formatting functions are similar, so no "comment version" will be
% given for them.

FUNCTION{field.or.null}
{ duplicate$
  missing$
    { pop$	% a way to get the duplicate `missing' value off
      ""
    }
    'skip$	% the duplicate is the return value
  if$
}

FUNCTION{italicize}
{ 's swap$ :=
  s "" =
    { "" }
    { "{\it " s * "}" * }
  if$
}

INTEGERS{ nameptr namesleft numnames }

STRINGS{ nameresult }

FUNCTION{format.names}
{ 's swap$ :=
  'nameptr #1 :=
  'nameresult "" :=
  'numnames s num.names$ :=
  'namesleft numnames :=
    { namesleft #0 > }
    { 'namesleft namesleft #1 - :=
      nameptr #1 >
	{ namesleft #0 >
	    { 'nameresult nameresult ", " * :=}
	    { numnames #2 >
		{ 'nameresult nameresult ", and " * :=}
		{ 'nameresult nameresult " and " * :=}
	      if$
	    }
	  if$
	}
	'skip$
      if$
      'nameresult
	  nameresult
	  s nameptr "{ff }{vv }{ll}{, jj}" format.name$
	  *
	:=
      'nameptr nameptr #1 +  :=
    }
  while$
  nameresult	% return the result
}

FUNCTION{format.authors}
{ author missing$
    {""}
    {author format.names}
  if$
}

FUNCTION{format.editors}
{ editor missing$
    { ""
    }
    {
      editor num.names$ #1 >
	{ editor format.names ", editors" * }
	{ editor format.names ", editor" * }
      if$
    }
  if$
}

% The format.title function is used for non-book-like titles.  We convert to
% lowercase (except for the very first letter), and hope the user has
% brace-surrounded words that need to stay capitilized.

FUNCTION{format.title}
{ title missing$
    { "" }
    { title "ul" change.case$ }
  if$
}

% The entry.string.max function is set to BibTeX's ent_str_size constant,
% the maximum length of an entry string variable.
%
% The global.string.max function is set to BibTeX's glob_str_size constant,
% the maximum length of a global string variable.

FUNCTION{entry.string.max} { #100 }

FUNCTION{global.string.max} { #300 }

% The n.dashify function makes each single `-' in a string a double `--'
% if it's not already
%
% VAR: pageresult: STRING
%
% n.dashify(s) ==
%  BEGIN
%	t := s
%	pageresult := ""
%	while (not (t = ""))
%	  do
%	    if (first character of t = "-")
%	      then
%		if (next character isn't)
%		  then
%		    pageresult := pageresult * "--"
%		    t := t with the "-" removed
%		  else
%		    while (first character of t = "-")
%		      do
%			pageresult := pageresult * "-"
%			t := t with the "-" removed
%		      od
%		fi
%	      else
%		pageresult := pageresult * the first character
%		t := t with the first character removed
%	    fi
%	  od
%	return pageresult
%  END

STRINGS{ pageresult }

FUNCTION{n.dashify}
{ 't swap$ :=
  'pageresult "" :=
    { t "" = not }
    { t #1 #1 substring$ "-" =
	{ t #1 #2 substring$ "--" = not
	    { 'pageresult pageresult "--" * :=
	      't t #2 global.string.max substring$ :=
	    }
	    {   { t #1 #1 substring$ "-" =}
		{ 'pageresult pageresult "-" * :=
		  't t #2 global.string.max substring$ :=
		}
	      while$
	    }
	  if$
	}
	{ 'pageresult pageresult t #1 #1 substring$ * :=
	  't t #2 global.string.max substring$ :=
	}
      if$
    }
  while$
  pageresult
}

% The format.vol.num.pages function is for the volume, number, and page range
% of a journal article.  We use the format:  vol(number):pages

FUNCTION{format.vol.num.pages}
{ volume field.or.null
  number missing$
    { booktitle missing$
        { "" }
	{ "(" booktitle * ")" * }
      if$
    }
    { "(" number *
      booktitle missing$
        { "" }
	{ ", " booktitle * }
      if$
      * ")" *
    }
  if$
  *
  pages missing$
    { "" }
    { ":" pages n.dashify * }
  if$
  *
}

% The format.date function is for the month and year, but we give a warning if
% there's a missing year but the month is there, and we return the empty string
% if they're both missing

FUNCTION{format.date}
{ year missing$
    { month missing$
	{ "" }
	{ "Warning: there's a month but no year in " cite$ * top$
	  month
	}
      if$
    }
    { month missing$
	{ year }
	{ month " " * year * }
      if$
    }
  if$
}

% The format.btitle is for formatting the title field when it is a book-like
% entry---the style used here keeps it in uppers-and-lowers and italicizes it.

FUNCTION{format.btitle}
{ title field.or.null
  italicize
}

% The format.bvolume function is for formatting the volume number and/or
% series name of a multivolume book.  If the volume field is missing, we
% output either the series field italicized if it exists or the null string
% otherwise.  If both the volume and series fields are there, we assume the
% series field is the title of the whole multivolume work (the title field
% should be the title of the one referred to), and add an "of <series>".
% A tie (~) is put between the "Volume" and the volume number.
% We capitilize Volume because this function is used at the beginning of a
% block.

FUNCTION{format.bvolume}
{ volume missing$
    { series missing$
	{ "" }
	{ series italicize }
      if$
    }
    { "Volume~" volume *
      series missing$
	'skip$
	{ " of " * series italicize * }
      if$
    }
  if$
}

% The format.edition function appends " edition" to the edition, if present.
% We lowercase the edition (it should be something like "Third"), because
% this doesn't start a sentence.

FUNCTION{format.edition}
{ edition missing$
    { "" }
    { edition "ll" change.case$ " edition" * }
  if$
}

% The format.pages function is used for formatting a page range in a book.
% The multi.page.check function examines the page field for a "-" or a ","
% so that format.pages can use "page" instead of "pages" if neither exists.
% Note: global.string.max, set above, here means "take the rest of the string"
%
% VAR: multiresult: INTEGER	(actually, a boolean)
%
% multi.page.check(s) ==
%  BEGIN
%	t := s
%	multiresult := false
%	while ((not multiresult) and (not (t = "")))
%	  do
%	    if (first character of t = "-" or ",")
%	      then multiresult := true
%	      else t := t with the first character removed
%	    fi
%	  od
%	return multiresult
%  END

INTEGERS{ multiresult }

FUNCTION{multi.page.check}
{ 't swap$ :=
  'multiresult #0 :=
    { multiresult not
      t "" = not
      and
    }
    { t #1 #1 substring$ "-" =
      t #1 #1 substring$ "," =
      or
	{ 'multiresult #1 := }
	{ 't t #2 global.string.max substring$ := }
      if$
    }
  while$
  multiresult
}

FUNCTION{format.pages}
{ pages missing$
    { "" }
    { pages multi.page.check
	{ "pages~" pages n.dashify * }
	{ "page~" pages n.dashify * }
      if$
    }
  if$
}

% The format.chapter.pages puts "chapter~" in front of a chapter number,
% if present, and then appends the pages, if present.
% This doesn't begin a sentence.

FUNCTION{format.chapter.pages}
{ chapter missing$
    { format.pages }
    { "chapter~" chapter *
      pages missing$
	'skip$
	{ ", " * format.pages * }
      if$
    }
  if$
}

% The format.in.ed.booktitle function is used for starting out a sentence
% that begins "In <booktitle>", putting an editor before the title if one
% exists.

FUNCTION{format.in.ed.booktitle}
{ booktitle missing$
    { ""
    }
    { 's format.editors :=
      s "" =
	{ "In " booktitle italicize * }
	{ "In " s * ", " * booktitle italicize * }
      if$
    }
  if$
}

% The format.in.techreport function is used for starting out a sentence
% that begins "In <techreport>"

FUNCTION{format.in.techreport}
{ booktitle missing$
    { "" }
    { "In " booktitle * }
  if$
}

% The format.tr.number makes a string starting with "Technical Report"
% (or type, if that field is defined), followed by the number if there
% is one (but return the first part even if there is no number)

FUNCTION{format.tr.number}
{ type missing$
    { "Technical Report" }
    { type }
  if$
  number missing$
    'skip$
    { "~" * number * }
  if$
}

% Now we define the type functions for all entry types that may appear
% in the .BIB file -- e.g., functions like `book' and `article'.  These
% are the routines that actually generate the .BBL file output for
% the entry.  In addition, the style designer should have a function
% `default.type' for unknown types.  Note: The fields (within each list)
% are listed in order of appearance.
%
% The article function is for an article in a journal.
%	Required fields: author, title, journal, year
%	Optional fields: volume, number, pages, month, note
%
% article ==
%  BEGIN
%	output.bibitem
%	output.check("author",format.authors)
%	new.block
%	output.check("title",format.title)
%	new.block
%	output.check("journal",italicize(field.or.null(journal)))
%	output(format.vol.num.pages)
%	output.check("year",format.date)
%	new.block
%	output(field.or.null(note))
%	fin.entry
%  END
%
% The book function is for a whole book.
%	Required fields: author or editor, title, publisher, year
%	Optional fields: volume, series, address, edition, month, note
%
% book ==
%  BEGIN
%	if missing$(author) then output.check("author and editor",
%								format.editors)
%	else output.check("author",format.authors)
%	fi
%	new.block
%	output.check("title",format.btitle)
%	new.block
%	output(format.bvolume)
%	output.check("publisher",field.or.null(publisher))
%	output(field.or.null(address))
%	output(format.edition)
%	output.check("year",format.date))
%	new.block
%	output(field.or.null(note))
%	fin.entry
%  END
%
% The other entry functions are all quite similar, so no "comment version"
% will be given for them.

FUNCTION{article}
{ output.bibitem
  "author" format.authors output.check
  new.block
  "title" format.title output.check
  new.block
  "journal" journal field.or.null italicize output.check
  format.vol.num.pages output
  "year" format.date output.check
  new.block
  note field.or.null output
  fin.entry
}

FUNCTION{book}
{ output.bibitem
  author missing$
    { "author and editor" format.editors output.check
    }
    { "author" format.authors output.check
    }
  if$
  new.block
  "title" format.btitle output.check
  new.block
  format.bvolume output
  "publisher" publisher field.or.null output.check
  address field.or.null output
  format.edition output
  "year" format.date output.check
  new.block
  note field.or.null output
  fin.entry
}

% The booklet function is for bound things without publishers or
% sponsoring institutions
%	Required: title
%	Optional: author, howpublished, address, month, year, note

FUNCTION{booklet}
{ output.bibitem
  format.authors output
  new.block
  "title" format.btitle output.check
  new.block
  howpublished field.or.null output
  address field.or.null output
  format.date output
  new.block
  note field.or.null output
  fin.entry
}

% For the conference entry type, see inproceedings

% An inbook is a piece of a book: either a chapter or a page range.
%	Required: author or editor, title, chapter or pages, publisher, year
%	Optional: volume, series, address, edition, month, note

FUNCTION{inbook}
{ output.bibitem
  author missing$
    { "author and editor" format.editors output.check
    }
    { "author" format.authors output.check
    }
  if$
  new.block
  "title" format.btitle output.check
  "chapter and pages" format.chapter.pages output.check
  new.block
  format.bvolume output
  "publisher" publisher field.or.null output.check
  address field.or.null output
  format.edition output
  "year" format.date output.check
  new.block
  note field.or.null output
  fin.entry
}

% An incollection is like inbook, but where there is a separate title
% for the referenced thing (and perhaps an editor for the whole)
%	Required: author, title, booktitle, publisher, year
%	Optional: editor, chapter, pages, address, month, note

FUNCTION{incollection}
{ output.bibitem
  "authors" format.authors output.check
  new.block
  "title" format.title output.check
  new.block
  "booktitle" format.in.ed.booktitle output.check
  format.chapter.pages output
  "publisher" publisher field.or.null output.check
  address field.or.null output
  "year" format.date output.check
  new.block
  note field.or.null output
  fin.entry
}

% An intechreport is almost the same as an incollection
FUNCTION{intechreport}
{ output.bibitem
  "authors" format.authors output.check
  new.block
  "title" format.title output.check
  new.block
  "booktitle" format.in.techreport output.check
  format.chapter.pages output
  format.tr.number output
  "insitution" institution field.or.null output.check
  address field.or.null output
  "year" format.date output.check
  new.block
  note field.or.null output
  fin.entry
}

% An inproceedings is an article in a conference proceedings
%	Required: author, title, booktitle, year
%	Optional: editor, pages, organization, publisher, address,
%		  month, note, location

FUNCTION{inproceedings}
{ output.bibitem
  "author" format.authors output.check
  new.block
  "title" format.title output.check
  new.block
  "booktitle" format.in.ed.booktitle output.check
  format.pages output
  location field.or.null output
  organization field.or.null output
  publisher field.or.null output
  address field.or.null output
  "year" format.date output.check
  new.block
  note field.or.null output
  fin.entry
}

% The conference function is included for Scribe compatibility

FUNCTION{conference} {inproceedings}

% A manual is technical documentation
%	Required: title
%	Optional: author, organization, address, edition, month, year, note

FUNCTION{manual}
{ output.bibitem
  format.authors output
  new.block
  "title" format.btitle output.check
  new.block
  organization field.or.null output
  address field.or.null output
  format.edition output
  format.date output
  new.block
  note field.or.null output
  fin.entry
}

% A mastersthesis is a Master's thesis
%	Required: author, title, school, year
%	Optional: address, month, note

FUNCTION{mastersthesis}
{ output.bibitem
  "author" format.authors output.check
  new.block
  "title" format.btitle output.check
  new.block
  "Master's thesis" output
  "school" school field.or.null output.check
  address field.or.null output
  "year" format.date output.check
  new.block
  note field.or.null output
  fin.entry
}

% a misc is something that doesn't fit elsewhere
%	Required: none
%	Optional: author, title, howpublished, month, year, note

FUNCTION{misc}
{ output.bibitem
  format.authors output
  new.block
  format.title output
  new.block
  howpublished field.or.null output
  format.date output
  new.block
  note field.or.null output
  fin.entry
}

% A phdthesis is like a mastersthesis
%	Required: author, title, school, year
%	Optional: address, month, note

FUNCTION{phdthesis}
{ output.bibitem
  "author" format.authors output.check
  new.block
  "title" format.btitle output.check
  new.block
  "PhD thesis" output
  "school" school field.or.null output.check
  address field.or.null output
  "year" format.date output.check
  new.block
  note field.or.null output
  fin.entry
}

% a proceedings is a conference proceedings
%	Required: editor or organization, title, year
%	Optional: publisher, address, month, note

FUNCTION{proceedings}
{ output.bibitem
  editor missing$
    { "editor and organization" organization field.or.null output.check
    }
    { "editor" format.editors output.check
    }
  if$
  new.block
  "title" format.btitle output.check
  publisher field.or.null output
  address field.or.null output
  "year" format.date output.check
  new.block
  note field.or.null output
  fin.entry
}

% a techreport is a technical report.
%	Required: author, title, institution, year
%	Optional: type, number, address, month, note

FUNCTION{techreport}
{ output.bibitem
  "author" format.authors output.check
  new.block
  "title" format.btitle output.check
  new.block
  format.tr.number output
  "institution" institution field.or.null output.check
  address field.or.null output
  "year" format.date output.check
  new.block
  note field.or.null output
  fin.entry
}

% an unpublished is something that hasn't been published
%	Required: author, title, note
%	Optional: month, year

FUNCTION{unpublished}
{ output.bibitem
  "author" format.authors output.check
  new.block
  "title" format.title output.check
  new.block
  format.date output
  new.block
  "note" note field.or.null output.check
  fin.entry
}

% We use entry type book for an unknown type and give a warning

FUNCTION{default.type} {book}

% Here are macros for common things that may vary from style to style.
% Users are encouraged to use these macros.

% For this style (plain), months and journals are written out in full

MACRO{jan} {"January"}

MACRO{feb} {"February"}

MACRO{mar} {"March"}

MACRO{apr} {"April"}

MACRO{may} {"May"}

MACRO{jun} {"June"}

MACRO{jul} {"July"}

MACRO{aug} {"August"}

MACRO{sep} {"September"}

MACRO{oct} {"October"}

MACRO{nov} {"November"}

MACRO{dec} {"December"}

MACRO{acmcs} {"ACM Computing Surveys"}

MACRO{acta} {"Acta Informatica"}

MACRO{cacm} {"Communications of the ACM"}

MACRO{ibmjrd} {"IBM Journal of Research and Development"}

MACRO{ibmsj} {"IBM Systems Journal"}

MACRO{ieeese} {"IEEE Transactions on Software Engineering"}

MACRO{ieeetc} {"IEEE Transactions on Computers"}

MACRO{ieeetcad}
 {"IEEE Transactions on Computer-Aided Design of Integrated Circuits"}

MACRO{ipl} {"Information Processing Letters"}

MACRO{jacm} {"Journal of the ACM"}

MACRO{jcss} {"Journal of Computer and System Sciences"}

MACRO{scp} {"Science of Computer Programming"}

MACRO{sicomp} {"SIAM Journal on Computing"}

MACRO{tocs} {"ACM Transactions on Computer Systems"}

MACRO{tods} {"ACM Transactions on Database Systems"}

MACRO{tog} {"ACM Transactions on Graphics"}

MACRO{toms} {"ACM Transactions on Mathematical Software"}

MACRO{toois} {"ACM Transactions on Office Information Systems"}

MACRO{toplas} {"ACM Transactions on Programming Languages and Systems"}

MACRO{tsc} {"Theoretical Computer Science"}

% Now we read in the .BIB entries.

READ

% I CHANGED THIS SORTKEY TO BE BETTER - pfps
% Next we compute the sortkey by executing "presort" on each entry.
% The presort key contains a number of purified strings, concatenated
% with multiple blanks between them.  Since purify$ makes sure there will
% not be consecutive multiple blanks in its result, this makes things
% like "John  Jones" come before "John Green  Brown".
%
% The fields used here are: the author names (or editor names, if those
% are missing, or the key field if both are), WITH THE NUMBER OF NAMES
% HERE AFTER THE FIRST NAME, FOLLOWED BY THE YEAR, followed by the first bit
% of the title (chopping off a leading "The", "A", or "An").
% Names are formatted: Von Last First Junior, with the First names
% abbreviated.  Two blanks will separate the name parts (except the von
% and last), three will separate the names, and four will separate the
% names from the YEAR AND THE YEAR FROM THE title.

% The sort.format.names function takes an argument that should be in
% BibTeX name format, and returns a string containing "   "-separated
% names in the format described above.  The function is almost the same
% as format.names.

FUNCTION{sort.format.names}
{ 's swap$ :=
  'nameptr #1 :=
  'nameresult "" :=
  'namesleft s num.names$ :=
    { namesleft #0 > }
    { 'namesleft namesleft #1 - :=
      nameptr #1 >
	{ 'nameresult nameresult "   " * := }
	'skip$
      if$
      'nameresult
	  nameresult
	  s nameptr "{vv }{ll}{  f}{  jj}" format.name$
	    "ll" change.case$
	  purify$
	  *
	:=
       nameptr #1 =
         { 'nameresult
	    nameresult "   " * namesleft #1 + int.to.str$ *
	   := }
	 'skip$
       if$
      'nameptr nameptr #1 +  :=
    }
  while$
  nameresult	% return the result
}

% The chop.word(w,len,s) function returns either s or, if the first len
% letters of s equals w, it returns that part of s after w.

INTEGERS { len }

FUNCTION{chop.word}
{ 's swap$ :=
  'len swap$ :=
  s #1 len substring$ =		% comparing against w, on the stack
    { s len #1 + global.string.max substring$ }
    { s }
  if$
}

% The sort.format.title function returns the the argument,
% but first any leading "A "'s, "An "'s, or "The "'s are removed.
% The chop.word function uses s, so we need another string variable, t

FUNCTION{sort.format.title}
{ 't swap$ :=
  "A " #2
    "An " #3
      "The " #4 t chop.word
    chop.word
  chop.word
  #1 global.string.max substring$
  purify$
}

% There is a limit on the length of an entry string variable, which
% is what its sort.key$ is.  The limit is currently entry.string.max, so we
% take at most that many characters of the constructed key, and hope
% there aren't many references that match to that many characters!

FUNCTION{presort}
{ author missing$
    { editor missing$
	{ key field.or.null }
	{ editor sort.format.names }
      if$
    }
    { author sort.format.names }
  if$
  "    "
  *
  year field.or.null
  *
  "    "
  *
  title field.or.null
  sort.format.title
  *
  #1 entry.string.max substring$
  'sort.key$ swap$ :=
}

ITERATE{presort}

% And now we can sort
%
% To get the unsorted style (plainu) from this one (plain), it suffices
% to simply comment out the ITERATE{presort} and SORT commands

SORT

%%I CHANGED THIS PART - pfps 
%% Now comes the computation of the label.  We use numbers, in the sort order.
%% We have to keep track of the longest (in width$ terms) label, for use
%% by the "thebibliography" environment.
%%
%% For nonnumeric labels, this computation is more complicated.
%
%STRINGS{longest.label}
%
%INTEGERS{number.label longest.label.width}
%
%FUNCTION{initialize.labels}
%{ 'longest.label "" :=
%  'number.label #1  :=
%  'longest.label.width #0 :=
%}
%
%FUNCTION{forward.pass}
%{ 'label number.label int.to.str$ :=
%  'number.label number.label #1 + :=
%  label width$ longest.label.width >
%    { 'longest.label label :=
%      'longest.label.width label width$ :=
%    }
%    'skip$
%  if$
%}
%
%EXECUTE{initialize.labels}
%
%ITERATE{forward.pass}
%
%% Note:  When using nonnumeric labels, the forward.pass must compute those
%% labels as well as the `b', `c', `d' etc. that will be appended to otherwise
%% identical labels.  There should then also be a reverse.pass to compute
%% the `a', do whatever appending needs done, and compute the longest label
%% (forward.pass can't compute this).
%
%% REVERSE{reverse.pass}
%
%Now comes the computation of the label.  We use the authors' names,
%followed by the year of publication, followed by a distinguisher 'a'
%etc. if necessary.  This has been modified from sample.bst. - pfps
% Now we finish the computation of each entry's label, putting in
% the 'a's and 'b's and so forth if required.  This involves two
% passes: a forward pass to put in the 'b's, 'c's and so on, and 
% a backwards pass to put in the 'a's.  The second pass also puts
% the longest label into the variable longestlabel
%
% VARS: longest.label, last.label : string
%       label.width, last.extra   : integer
%
% label.format ==
%  BEGIN
%	numnames := num.names$(author)
%       if missing$(author) then label := ""
%       else if numnames = 1 
%	     then label := format.names$(author,1,"{vv }{ll}")
%	     else if numnames = 2
%		  then label := format.names$(author,1,"{vv }{ll and }") *
%				format.names$(author,2,"{vv }{ll}")
%                 else label := format.names$(author,1,"{vv }{ll {\em et al.}}")
%		  fi
%            fi
%       fi
%	if missing$(year) then return label
%	else return "\protect\citename{" * label * ", }" * year % pfps 9/86
%  END
%
% initialize.labels ==
%  BEGIN
%     longest.label := last.label := next.extra := null
%     label.width   := last.extra := 0
%  END
%
% forward.pass ==
%  BEGIN
%   label := label.format()
%   FPIF: if last.label = label
%           then last.extra := last.extra + 1
%                extralabel := ascii$(`a + last.extra)
%           else last.extra := 0
%                extralabel := null
%         fi
%  END
%
% reverse.pass ==
%  BEGIN
%     if next.extra = "b"
%             then extralabel := "a"
%           fi
%           label := label * extralabel
%%    if width${label} > label.width
%%             then longest.label := label
%%                  label.width := width${label}
%%           fi
%           next.extra := extralabel
%  END

STRINGS{last.label next.extra}
INTEGERS{last.extra}

FUNCTION{initialize.labels}
{ 'last.label ""    :=
  'next.extra ""    :=
  'last.extra  #0    :=
%  'label.width #0    :=
%  'longest.label "" :=
}

FUNCTION{label.format.names}
{ 's swap$ :=
  'numnames s num.names$ :=
  numnames #1 =
    { 's s #1 "{vv }{ll}" format.name$ := }
    { numnames #2 =
        { 's s #1 "{vv }{ll }and " format.name$
  	   s #2 "{vv }{ll}" format.name$ * :=
        }
        { 's s #1 "{vv }{ll }\bgroup \em et al.\egroup " format.name$ := }
      if$
    }
  if$
  s
}

FUNCTION{label.format}
{ author missing$
    { editor missing$
        { 'label "" := }
	{ 'label editor label.format.names := }
      if$
    }
    { 'label author label.format.names := }
  if$
  year missing$
    { label } % pfps 9/86
    { label "" = 
      {""} 
      {"\protect\citename{" label * ", }" *}  % pfps 9/86
      if$ 
      year * }
  if$
}

FUNCTION{forward.pass}
{ 'label label.format :=
  label last.label = 
    { 'last.extra last.extra #1 +  := 
      'extralabel  #97 last.extra + int.to.chr$ :=
    }
    { 'last.extra #0 :=
      'extralabel "" :=
    }
  if$
  'last.label label :=
}

FUNCTION{reverse.pass}
{ next.extra "b" = 
    { 'extralabel "a" :=}
    'skip$
  if$
  'label label extralabel * :=
  'next.extra extralabel :=
%  label.width label width$ > 
%    'skip$
%    { 'longest.label label :=
%      'label.width  label width$    :=
%    }
%  if$
}

EXECUTE{initialize.labels}
ITERATE{forward.pass}
REVERSE{reverse.pass}



% Now we're ready to start writing the .BBL file.  First we write
% the `preamble' containing the command
%     \begin{thebibliography}{...}
% where the `...' is the longest label.
% Also, we call init.state.consts, for use by the output routines.
%
% The following \sfcode declaration causes a `.' (period) not to
% produce an end-of-sentence space.  Each entry is divided into
% `blocks', which are separated by a \newblock command.  The following
% \def command defines \newblock to add horizontal space, giving
% a `closed' format.
%	\def\newblock{\hskip .11em plus .33em minus -.07em}
%	\sfcode`\.=1000\relax
%
% For an `open' format (each block on a new line), replace these commands by:
%	\def\newblock{\\\relax}

FUNCTION{preamble}
{ init.state.consts
  "\begin{thebibliography}{"  "xx" % longest.label
			      * "}" * write$
  newline$
  "% The newblock command here creates a `closed' format" write$
  newline$
  "% and the sfcode command affects the spacing after a period;" write$
  newline$
  "% for more details please ask a style designer." write$
  newline$
  "\def\newblock{\hskip .11em plus .33em minus -.07em}" write$
  "\sfcode`\.=1000\relax" write$
  newline$
}

EXECUTE{preamble}

% Now we execute to produce the output for all the entries

ITERATE{call.type$}

% Finally, we finish up by writing the `\end{thebibliography}' command.

FUNCTION{finish.up}{ newline$ "\end{thebibliography}" write$ newline$}

EXECUTE{finish.up}
