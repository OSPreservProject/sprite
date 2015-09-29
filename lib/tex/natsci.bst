%% BibTeX `natsci' family
%
% **********************************************************
% ***** This will not work with BibTeX 0.99a or later ******
% **********************************************************
%
%% 08 jan 87 gildea natural sciences version created from this version:
	% version 0.98c for BibTeX versions 0.98i or later, LaTeX version 2.08
	% Copyright (C) 1985, all rights reserved
	% Copying of this file is authorized only if either
	% (1) you make absolutely no changes to your copy, including name, or
	% (2) if you do make changes, you name it something other than
	% btxbst.doc, plain.bst, unsrt.bst, alpha.bst, and abbrv.bst
	% This restriction helps ensure that all standard styles are identical

%% Copyright (c) 1987 by Stephen Gildea
%% Permission to copy all or part of this work is granted, provided
%% that the copies are not made or distributed for resale, and that
%% the copyright notice and this notice are retained.
%% 
%% THIS WORK IS PROVIDED ON AN "AS IS" BASIS.  THE AUTHOR PROVIDES NO
%% WARRANTY WHATSOEVER, EITHER EXPRESS OR IMPLIED, REGARDING THE WORK,
%% INCLUDING WARRANTIES WITH RESPECT TO ITS MERCHANTABILITY OR FITNESS
%% FOR ANY PARTICULAR PURPOSE.

%% Natural Sciences bibliography format:
%%  citations in the text look like (Gildea, 1987).
%%  There are no labels in the bibliography.
%%  There are no "sentences"; commas separate all parts of an entry.
%%  The first author's name is reversed: Last, First.
%%  The number is not listed, just the volume.
%%  For two entries by the same author in the same year, a letter ("a",
%%    "b", etc.) is added to the year in the reference list as well as in
%%    the citation label.
%%
%% Other things fixed by Gildea:
%%  The editors are listed after, not before, the booktitle.
%%  The comma after a field is italicized if the field was.

%% Note to users:
%%  Since the natsci format does not use labels in the bibliography,
%%  the LaTeX style needs to be modified slightly.  As a minimum,
%%  redefine \@biblabel as follows:
%%
%%  \def\@biblabel#1{\hfill}
%%
%%  To get the indenting right, the following should be added to the
%%  list initialization in the definition of the thebibliography
%%  environment:
%%
%%  \itemindent=-\leftmargin
%%
%%  Of course, the best solution is to use a LaTeX style that was designed
%%  to use this bibliography style.

%% The commented source is available from the author.
%% If you make any improvements, I'd like to hear about them.
%% Stephen Gildea
%% MIT Earth Resources Lab
%% Cambridge, Mass. 02139
%% mit-erl!gildea
%% gildea@erl.mit.edu

ENTRY
  { address
    author
    booktitle
    chapter
    edition
    editor
    howpublished
    institution
    journal
    key
    month
    note
    number
    organization
    pages
    publisher
    school
    series
    title
    type
    volume
    year
  }
  {}

  { label extra.label sort.label }

INTEGERS { output.state before.all mid.sentence after.sentence after.block }

FUNCTION {init.state.consts}
{ 'before.all #0 :=
  'mid.sentence #1 :=
  'after.sentence #2 :=
  'after.block #3 :=
}

STRINGS { s t }

FUNCTION {add.comma}
{ duplicate$
  #-1 #1 substring$ "}" =
    { #-2 #1024 substring$ ",} " * }
    { ", " * }
  if$
}

FUNCTION {output.nonnull}
{ 's swap$ :=
  output.state mid.sentence =
    { add.comma write$ }
    { output.state after.block =
	{ add.period$ write$
	  newline$
	  "\newblock " write$
	}
	{ output.state before.all =
	    'write$
	    { add.period$ " " * write$ }
	  if$
	}
      if$
    }
  if$
  'output.state mid.sentence :=
  s
}

FUNCTION {output}
{ 's swap$ :=
  s "" =
    'skip$
    { s output.nonnull }
  if$
}

FUNCTION {output.check}
{ 's swap$ :=
  't swap$ :=
  s "" =
    { "Warning: the " t * " shouldn't be empty in " * cite$ * top$ }
    { s output.nonnull }
  if$
}

FUNCTION {output.bibitem}
{ newline$

  "\bibitem[" write$
  label write$
  "]{" write$

  cite$ write$
  "}" write$
  newline$
  ""
  'output.state before.all :=
}

FUNCTION {fin.entry}
{ add.period$
  write$
  newline$
}

FUNCTION {new.block}
{ output.state before.all =
    'skip$
    { 'output.state after.block := }
  if$
}

FUNCTION {new.sentence}
{ output.state after.block =
    'skip$
    { output.state before.all =
	'skip$
	{ 'output.state after.sentence := }
      if$
    }
  if$
}

FUNCTION {not}
{   { #0 }
    { #1 }
  if$
}

FUNCTION {and}
{   'skip$
    { pop$ #0 }
  if$
}

FUNCTION {or}
{   { pop$ #1 }
    'skip$
  if$
}

FUNCTION {field.or.null}
{ duplicate$
  missing$
    { pop$ "" }
    'skip$
  if$
}

FUNCTION {italicize}
{ 's swap$ :=
  s "" =
    { "" }
    { "{\it " s * "}" * }
  if$
}

INTEGERS { nameptr namesleft numnames }

STRINGS {nameresult}

FUNCTION {format.names}
{ 's swap$ :=
  'nameptr #1 :=
  'nameresult "" :=
  'numnames s num.names$ :=
  'namesleft numnames :=
    { namesleft #0 > }
    {

      't s nameptr "{f.~}{vv~}{ll}{, jj}" format.name$ :=

      nameptr #1 >
	{ namesleft #1 >
	    { 'nameresult nameresult ", " * t * := }
	    { numnames #2 >
		{ 'nameresult nameresult "," * := }
		'skip$
	      if$
	      t "others" =
		{ 'nameresult nameresult " et al." * := }
		{ 'nameresult nameresult " and " * t * := }
	      if$
	    }
	  if$
	}
	{ 'nameresult nameresult t * := }
      if$
      'nameptr nameptr #1 + :=
      'namesleft namesleft #1 - :=
    }
  while$
  nameresult
}

FUNCTION {format.names.reverse1}
{ 's swap$ :=
  'nameptr #1 :=
  'nameresult "" :=
  'numnames s num.names$ :=
  'namesleft numnames :=
    { namesleft #0 > }
    {
      nameptr #1 >
	{ 

          't s nameptr "{f.~}{vv~}{ll}{, jj}" format.name$ :=

          namesleft #1 >
	    { 'nameresult nameresult ", " * t * := }
	    { numnames #2 >
		{ 'nameresult nameresult "," * := }
		'skip$
	      if$
	      t "others" =
		{ 'nameresult nameresult " et al." * := }
		{ 'nameresult nameresult " and " * t * := }
	      if$
	    }
	  if$
	}
	{

          't s nameptr "{vv~}{ll}{, jj}{, f.}" format.name$ :=

          'nameresult nameresult t * :=
        }
      if$
      'nameptr nameptr #1 + :=
      'namesleft namesleft #1 - :=
    }
  while$
  nameresult
}

FUNCTION {format.authors}
{ author missing$
    { "" }
    { author format.names.reverse1 }
  if$
}

FUNCTION {format.eds}
{ editor missing$
    { "" }
    { editor num.names$ #1 >
	{ editor format.names.reverse1 " (eds.)" * }
	{ editor format.names.reverse1 " (ed.)" * }
      if$
    }
  if$
}

FUNCTION {format.editors}
{ editor missing$
    { "" }
    { editor num.names$ #1 >
	{ editor format.names ", editors" * }
	{ editor format.names ", editor" * }
      if$
    }
  if$
}

FUNCTION {format.edited.by}
{ editor missing$
    'skip$
    { add.comma "edited by " * editor format.names * }
  if$
}

FUNCTION {format.title}

{ title missing$
    { "" }
    { title "ul" change.case$ }
  if$

}

FUNCTION {entry.string.max} { #100 }

FUNCTION {global.string.max} { #300 }

STRINGS {pageresult}

FUNCTION {n.dashify}
{ 't swap$ :=
  'pageresult "" :=
    { t "" = not }
    { t #1 #1 substring$ "-" =
	{ t #1 #2 substring$ "--" = not
	    { 'pageresult pageresult "--" * :=
	      't t #2 global.string.max substring$ :=
	    }
	    {   { t #1 #1 substring$ "-" = }
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

FUNCTION {format.date}
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
      extra.label *
    }
  if$
}

FUNCTION {format.btitle}
{ title field.or.null
  italicize
}

FUNCTION {format.bvolume}
{ volume missing$
    { series missing$
	{ "" }
	{ series italicize }
      if$
    }
    { "vol.~" volume *
      series missing$
	'skip$
	{ " of " * series italicize * }
      if$
    }
  if$
}

FUNCTION {format.edition}
{ edition missing$
    { "" }
    { edition "ll" change.case$ " edition" * }
  if$
}

INTEGERS {multiresult}

FUNCTION {multi.page.check}
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

FUNCTION {format.pages}
{ pages missing$
    { "" }
    { pages multi.page.check
	{ "pp.~" pages n.dashify * }
	{ "p.~" pages n.dashify * }
      if$
    }
  if$
}

FUNCTION {format.vol.num.pages}
{ volume field.or.null
  number missing$
    'skip$
    { "(" number * ")" * *
      volume missing$
	{ "Warning: there's a number but no volume in " cite$ * top$ }
	'skip$
      if$
    }
  if$
  pages missing$
    'skip$
    { duplicate$ "" =
	'skip$
	{ ":" * }
      if$
      pages n.dashify *
    }
  if$
}

FUNCTION {format.vol.pages}
{ volume missing$
    'skip$
    {volume italicize}
  if$
  pages missing$
    'skip$
    { duplicate$ "" =
	'skip$
	{ add.comma }
      if$
      pages n.dashify *
    }
  if$
}

FUNCTION {format.chapter.pages}
{ chapter missing$
    'format.pages
    { "chapter~" chapter *
      pages missing$
	'skip$
	{ add.comma format.pages * }
      if$
    }
  if$
}

FUNCTION {format.in.ed.booktitle}
{ booktitle missing$
    { "" }
    { 's format.editors :=
      s "" =
	{ "In " booktitle italicize * }
	{ "In " s * add.comma booktitle italicize * }
      if$
    }
  if$
}

FUNCTION {format.in.booktitle.ed}
{ booktitle missing$
    { "" }
    { "in " booktitle italicize *
      format.edited.by
    }
  if$
}

FUNCTION {format.tr.number}
{ type missing$
    { "Technical Report" }
    { type }
  if$
  number missing$
    'skip$
    { "~" * number * }
  if$
}

FUNCTION {article}
{ output.bibitem
  "author" format.authors output.check
  "title" format.title output.check
  "journal" journal field.or.null italicize output.check
  format.vol.pages output
  "year" format.date output.check
  note field.or.null output
  fin.entry
}

FUNCTION {book}
{ output.bibitem
  author missing$
    { "author and editor" format.eds output.check }
    { "author" format.authors output.check }
  if$
  "title" format.btitle output.check
  format.bvolume output
  "publisher" publisher field.or.null output.check
  address field.or.null output
  format.edition output
  "year" format.date output.check
  note field.or.null output
  fin.entry
}

FUNCTION {booklet}
{ output.bibitem
  format.authors output
  "title" format.btitle output.check
  howpublished field.or.null output
  address field.or.null output
  format.date output
  note field.or.null output
  fin.entry
}

FUNCTION {inbook}
{ output.bibitem
  author missing$
    { "author and editor" format.eds output.check }
    { "author" format.authors output.check }
  if$
  "title" format.btitle output.check
  "chapter and pages" format.chapter.pages output.check
  format.bvolume output
  "publisher" publisher field.or.null output.check
  address field.or.null output
  format.edition output
  "year" format.date output.check
  note field.or.null output
  fin.entry
}

FUNCTION {incollection}
{ output.bibitem
  "authors" format.authors output.check
  "title" format.title output.check
  "booktitle" format.in.booktitle.ed output.check
  format.chapter.pages output
  "publisher" publisher field.or.null output.check
  address field.or.null output
  "year" format.date output.check
  note field.or.null output
  fin.entry
}

FUNCTION {inproceedings}
{ output.bibitem
  "author" format.authors output.check
  "title" format.title output.check
  "booktitle" format.in.booktitle.ed output.check
  format.pages output
  organization field.or.null output
  publisher field.or.null output
  address field.or.null output
  "year" format.date output.check
  note field.or.null output
  fin.entry
}

FUNCTION {conference} { inproceedings }

FUNCTION {manual}
{ output.bibitem
  format.authors output
  "title" format.btitle output.check
  organization field.or.null output
  address field.or.null output
  format.edition output
  format.date output
  note field.or.null output
  fin.entry
}

FUNCTION {mastersthesis}
{ output.bibitem
  "author" format.authors output.check
  "title" format.btitle output.check
  "Master's thesis" output
  "school" school field.or.null output.check
  address field.or.null output
  "year" format.date output.check
  note field.or.null output
  fin.entry
}

FUNCTION {misc}
{ output.bibitem
  format.authors output
  format.title output
  howpublished field.or.null output
  format.date output
  note field.or.null output
  fin.entry
}

FUNCTION {phdthesis}
{ output.bibitem
  "author" format.authors output.check
  "title" format.btitle output.check
  "PhD thesis" output
  "school" school field.or.null output.check
  address field.or.null output
  "year" format.date output.check
  note field.or.null output
  fin.entry
}

FUNCTION {proceedings}
{ output.bibitem
  editor missing$
    { organization missing$
	'skip$
	{ organization field.or.null output }
      if$
    }
    { format.editors output }
  if$
  "title" format.btitle output.check
  editor missing$
    'skip$
    { organization field.or.null output }
  if$
  publisher field.or.null output
  address field.or.null output
  "year" format.date output.check
  note field.or.null output
  fin.entry
}

FUNCTION {techreport}
{ output.bibitem
  "author" format.authors output.check
  "title" format.btitle output.check
  format.tr.number output
  "institution" institution field.or.null output.check
  address field.or.null output
  "year" format.date output.check
  note field.or.null output
  fin.entry
}

FUNCTION {unpublished}
{ output.bibitem
  "author" format.authors output.check
  "title" format.title output.check
  format.date output
  "note" note field.or.null output.check
  fin.entry
}

FUNCTION {default.type} { book }

MACRO {jan} {"January"}

MACRO {feb} {"February"}

MACRO {mar} {"March"}

MACRO {apr} {"April"}

MACRO {may} {"May"}

MACRO {jun} {"June"}

MACRO {jul} {"July"}

MACRO {aug} {"August"}

MACRO {sep} {"September"}

MACRO {oct} {"October"}

MACRO {nov} {"November"}

MACRO {dec} {"December"}

MACRO {bssa} {"Bull. Seis. Soc. Am."}

MACRO {grl} {"Geophys. Res. Lett."}

MACRO {jgr} {"J. Geophys. Res."}

MACRO {jgsa} {"J. Geol. Soc. Am."}

MACRO {mit} {"Mass. Inst. of Tech."}

MACRO {usgs} {"U.S. Geol. Surv."}

READ

FUNCTION {sortify}
{ purify$
  "ll" change.case$
}

FUNCTION {format.label.names}
{ 's swap$ :=
  'numnames s num.names$ :=
  numnames #1 >
    { numnames #2 >
	{ 'namesleft #1 := }
	{ 'namesleft numnames := }
      if$
      'nameptr #1 :=
      'nameresult "" :=
	{ namesleft #0 > }
	{ nameptr numnames =
	    { s nameptr "{ff}{vv}{ll}{jj}" format.name$ "others" =
		{ 'nameresult nameresult " et al." * := }
		{ nameresult "" =
		    'skip$
		    { 'nameresult nameresult " and " * := }
		  if$
		  'nameresult nameresult s nameptr "{vv}{ll}" format.name$
		  * :=
		}
	      if$
	    }
	    { 'nameresult nameresult s nameptr "{vv}{ll}" format.name$
	      * :=
	    }
	  if$
	  'nameptr nameptr #1 + :=
	  'namesleft namesleft #1 - :=
	}
      while$
      numnames #2 >
	{ 'nameresult nameresult " et al." * := }
	'skip$
      if$
    }
    { 'nameresult s #1 "{vv}{ll}" format.name$ :=
    }
  if$
  nameresult
}

FUNCTION {calc.label}
{ 'label
  author missing$
    { editor missing$
	{ key missing$
	    { "Warning: need a key to make a label in " cite$ * top$
	      title field.or.null purify$ #1 #3 substring$
	    }
	    { key purify$ #1 #3 substring$ }
	  if$
	}
	{ editor format.label.names }
      if$
    }
    { author format.label.names }
  if$
  add.comma
  year field.or.null purify$
  *
  :=
  'sort.label label sortify :=
}

FUNCTION {sort.format.names}
{ 's swap$ :=
  'nameptr #1 :=
  'nameresult "" :=
  'numnames s num.names$ :=
  'namesleft numnames :=
    { namesleft #0 > }
    { nameptr #1 >
	{ 'nameresult nameresult "   " * := }
	'skip$
      if$

      't s nameptr "{vv{ } }{ll{ }}{  f{ }}{  jj{ }}" format.name$ :=

      nameptr numnames = t "others" = and
	{ 'nameresult nameresult "et al" * := }
	{ 'nameresult nameresult t sortify * := }
      if$
      'nameptr nameptr #1 + :=
      'namesleft namesleft #1 - :=
    }
  while$
  nameresult
}

INTEGERS {len}

FUNCTION {chop.word}
{ 's swap$ :=
  'len swap$ :=
  s #1 len substring$ =
    { s len #1 + global.string.max substring$ }
    { s }
  if$
}

FUNCTION {sort.format.title}
{ 't swap$ :=
  "A " #2
    "An " #3
      "The " #4 t chop.word
    chop.word
  chop.word
  #1 global.string.max substring$
  sortify
}

FUNCTION {presort}
{

  calc.label
  sort.label
  "    "
  *

  author missing$
    { editor missing$
	{ key field.or.null sortify }
	{ editor sort.format.names }
      if$
    }
    { author sort.format.names }
  if$

  *

  "    "
  *
  title field.or.null
  sort.format.title
  *
  #1 entry.string.max substring$
  'sort.key$ swap$ :=
}

ITERATE {presort}

SORT

STRINGS { longest.label last.sort.label next.extra }

INTEGERS { longest.label.width last.extra.num }

FUNCTION {initialize.longest.label}
{ 'longest.label "" :=
  'last.sort.label "" :=
  'next.extra "" :=
  'longest.label.width #0 :=
  'last.extra.num #0 :=
}

FUNCTION {forward.pass}
{ last.sort.label sort.label =
    { 'last.extra.num last.extra.num #1 + :=
      'extra.label last.extra.num int.to.chr$ :=
    }
    { 'last.extra.num "a" chr.to.int$ :=
      'extra.label "" :=
      'last.sort.label sort.label :=
    }
  if$
}

FUNCTION {reverse.pass}
{ next.extra "b" =
    { 'extra.label "a" := }
    'skip$
  if$
  'label label extra.label * :=
  label width$ longest.label.width >
    { 'longest.label label :=
      'longest.label.width label width$ :=
    }
    'skip$
  if$
  'next.extra extra.label :=
}

EXECUTE {initialize.longest.label}

ITERATE {forward.pass}

REVERSE {reverse.pass}

FUNCTION {preamble}
{ "\begin{thebibliography}{X}" write$
  newline$
}

EXECUTE {preamble}

EXECUTE {init.state.consts}

ITERATE {call.type$}

FUNCTION {finish.up} { newline$ "\end{thebibliography}" write$ newline$ }

EXECUTE {finish.up}
