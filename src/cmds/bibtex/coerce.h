void printanewline();
void markwarning();
void markerror();
void markfatal();
void printoverflow();
void printconfusion();
void bufferoverflow();
boolean zinputln();
#define inputln(f) zinputln((alphafile) (f))
void zoutpoolstr();
#define outpoolstr(f, s) zoutpoolstr((alphafile) (f), (strnumber) (s))
void zprintapoolstr();
#define printapoolstr(s) zprintapoolstr((strnumber) (s))
void pooloverflow();
void filenmsizeoverflow();
void zouttoken();
#define outtoken(f) zouttoken((alphafile) (f))
void printatoken();
void printbadinputline();
void printskippingwhateve();
void samtoolongfilenamepr();
void samwrongfilenameprin();
void printauxname();
void auxerrprint();
void zauxerrillegalanother();
#define auxerrillegalanother(cmdnum) zauxerrillegalanother((integer) (cmdnum))
void auxerrnorightbracepr();
void auxerrstuffafterrigh();
void auxerrwhitespaceinar();
void printbibname();
void printbstname();
void hashciteconfusion();
void zcheckciteoverflow();
#define checkciteoverflow(lastcite) zcheckciteoverflow((citenumber) (lastcite))
void auxend1errprint();
void auxend2errprint();
void bstlnnumprint();
void bsterrprintandlookfo();
void bstwarnprint();
void eatbstprint();
void unknwnfunctionclassc();
void zprintfnclass();
#define printfnclass(fnloc) zprintfnclass((hashloc) (fnloc))
void ztraceprfnclass();
#define traceprfnclass(fnloc) ztraceprfnclass((hashloc) (fnloc))
void idscanningconfusion();
void bstidprint();
void bstleftbraceprint();
void bstrightbraceprint();
void zalreadyseenfunctionp();
#define alreadyseenfunctionp(seenfnloc) zalreadyseenfunctionp((hashloc) (seenfnloc))
void singlfnoverflow();
void biblnnumprint();
void biberrprint();
void bibwarnprint();
void zcheckfieldoverflow();
#define checkfieldoverflow(totalfields) zcheckfieldoverflow((integer) (totalfields))
void eatbibprint();
void zbiboneoftwoprint();
#define biboneoftwoprint(char1, char2) zbiboneoftwoprint((ASCIIcode) (char1), (ASCIIcode) (char2))
void bibequalssignprint();
void bibunbalancedbracesp();
void bibfieldtoolongprint();
void macrowarnprint();
void bibidprint();
void bibcmdconfusion();
void citekeydisappearedco();
void zbadcrossreferencepri();
#define badcrossreferencepri(s) zbadcrossreferencepri((strnumber) (s))
void nonexistentcrossrefe();
void zprintmissingentry();
#define printmissingentry(s) zprintmissingentry((strnumber) (s))
void bstexwarnprint();
void bstmildexwarnprint();
void bstcantmesswithentri();
void illeglliteralconfusi();
void unknwnliteralconfusi();
void zprintstklit();
#define printstklit(stklt, stktp) zprintstklit((integer) (stklt), (stktype) (stktp))
void zprintlit();
#define printlit(stklt, stktp) zprintlit((integer) (stklt), (stktype) (stktp))
void outputbblline();
void bst1printstringsizee();
void bst2printstringsizee();
void zbracesunbalancedcomp();
#define bracesunbalancedcomp(poplitvar) zbracesunbalancedcomp((strnumber) (poplitvar))
void caseconversionconfus();
void traceandstatprinting();
boolean zaopenin();
#define aopenin(f, pathspec) zaopenin((palphafile *) &(f), (integer) (pathspec))
boolean zaopenout();
#define aopenout(f) zaopenout((palphafile *) &(f))
void zstartname();
#define startname(filename) zstartname((strnumber) (filename))
void zaddextension();
#define addextension(ext) zaddextension((strnumber) (ext))
void zaddarea();
#define addarea(area) zaddarea((strnumber) (area))
strnumber makestring();
boolean zstreqbuf();
#define streqbuf(s, buf, bfptr, len) zstreqbuf((strnumber) (s),  (buf), (bufpointer) (bfptr), (bufpointer) (len))
boolean zstreqstr();
#define streqstr(s1, s2) zstreqstr((strnumber) (s1), (strnumber) (s2))
void zlowercase();
#define lowercase(buf, bfptr, len) zlowercase( (buf), (bufpointer) (bfptr), (bufpointer) (len))
void zuppercase();
#define uppercase(buf, bfptr, len) zuppercase( (buf), (bufpointer) (bfptr), (bufpointer) (len))
hashloc zstrlookup();
#define strlookup(buf, j, l, ilk, insertit) zstrlookup( (buf), (bufpointer) (j), (bufpointer) (l), (strilk) (ilk), (boolean) (insertit))
void zpredefine();
#define predefine(pds, len, ilk) zpredefine( (pds), (pdslen) (len), (strilk) (ilk))
void zinttoASCII();
#define inttoASCII(int, intbuf, intbegin, intend) zinttoASCII((integer) (int),  (intbuf), (bufpointer) (intbegin), (bufpointer *) &(intend))
void zadddatabasecite();
#define adddatabasecite(newcite) zadddatabasecite((citenumber *) &(newcite))
boolean zfindcitelocsforthisc();
#define findcitelocsforthisc(citestr) zfindcitelocsforthisc((strnumber) (citestr))
void zswap();
#define swap(swap1, swap2) zswap((citenumber) (swap1), (citenumber) (swap2))
boolean zlessthan();
#define lessthan(arg1, arg2) zlessthan((citenumber) (arg1), (citenumber) (arg2))
void zquicksort();
#define quicksort(leftend, rightend) zquicksort((citenumber) (leftend), (citenumber) (rightend))
void zbuildin();
#define buildin(pds, len, fnhashloc, bltinnum) zbuildin( (pds), (pdslen) (len), (hashloc *) &(fnhashloc), (bltinrange) (bltinnum))
void predefcertainstrings();
boolean zscan1();
#define scan1(char1) zscan1((ASCIIcode) (char1))
boolean zscan1white();
#define scan1white(char1) zscan1white((ASCIIcode) (char1))
boolean zscan2();
#define scan2(char1, char2) zscan2((ASCIIcode) (char1), (ASCIIcode) (char2))
boolean zscan2white();
#define scan2white(char1, char2) zscan2white((ASCIIcode) (char1), (ASCIIcode) (char2))
boolean zscan3();
#define scan3(char1, char2, char3) zscan3((ASCIIcode) (char1), (ASCIIcode) (char2), (ASCIIcode) (char3))
boolean scanalpha();
void zscanidentifier();
#define scanidentifier(char1, char2, char3) zscanidentifier((ASCIIcode) (char1), (ASCIIcode) (char2), (ASCIIcode) (char3))
boolean scannonneginteger();
boolean scaninteger();
boolean scanwhitespace();
boolean eatbstwhitespace();
void skiptokenprint();
void printrecursionillega();
void skptokenunknownfunct();
void skipillegalstuffafte();
void zscanfndef();
#define scanfndef(fnhashloc) zscanfndef((hashloc) (fnhashloc))
boolean eatbibwhitespace();
boolean compressbibwhite();
boolean scanbalancedbraces();
boolean scanafieldtokenandea();
boolean scanandstorethefield();
void zdecrbracelevel();
#define decrbracelevel(poplitvar) zdecrbracelevel((strnumber) (poplitvar))
void zcheckbracelevel();
#define checkbracelevel(poplitvar) zcheckbracelevel((strnumber) (poplitvar))
void znamescanforand();
#define namescanforand(poplitvar) znamescanforand((strnumber) (poplitvar))
boolean vontokenfound();
void vonnameendsandlastna();
void skipstuffatspbracele();
void bracelvlonelettersco();
boolean zenoughtextchars();
#define enoughtextchars(enoughchars) zenoughtextchars((bufpointer) (enoughchars))
void figureouttheformatte();
void zpushlitstk();
#define pushlitstk(pushlt, pushtype) zpushlitstk((integer) (pushlt), (stktype) (pushtype))
void zpoplitstk();
#define poplitstk(poplit, poptype) zpoplitstk((integer *) &(poplit), (stktype *) &(poptype))
void zprintwrongstklit();
#define printwrongstklit(stklt, stktp1, stktp2) zprintwrongstklit((integer) (stklt), (stktype) (stktp1), (stktype) (stktp2))
void poptopandprint();
void popwholestack();
void initcommandexecution();
void checkcommandexecutio();
void addpoolbufandpush();
void zaddbufpool();
#define addbufpool(pstr) zaddbufpool((strnumber) (pstr))
void zaddoutpool();
#define addoutpool(pstr) zaddoutpool((strnumber) (pstr))
void xequals();
void xgreaterthan();
void xlessthan();
void xplus();
void xminus();
void xconcatenate();
void xgets();
void xaddperiod();
void xchangecase();
void xchrtoint();
void xcite();
void xduplicate();
void xempty();
void xformatname();
void xinttochr();
void xinttostr();
void xmissing();
void xnumnames();
void xpreamble();
void xpurify();
void xquote();
void xsubstring();
void xswap();
void xtextlength();
void xtextprefix();
void xtype();
void xwarning();
void xwidth();
void xwrite();
void zexecutefn();
#define executefn(exfnloc) zexecutefn((hashloc) (exfnloc))
void getthetoplevelauxfil();
void auxbibdatacommand();
void auxbibstylecommand();
void auxcitationcommand();
void auxinputcommand();
void poptheauxstack();
void getauxcommandandproc();
void lastcheckforauxerror();
void bstentrycommand();
boolean badargumenttoken();
void bstexecutecommand();
void bstfunctioncommand();
void bstintegerscommand();
void bstiteratecommand();
void bstmacrocommand();
void getbibcommandorentry();
void bstreadcommand();
void bstreversecommand();
void bstsortcommand();
void bststringscommand();
void getbstcommandandproc();
void initialize();
