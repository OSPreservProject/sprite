.\"                               *  *  *  *  *
.\"  NOTE:  The characters defined in this file produce the 
.\"         best symbols when output using the italic font 
.\"         at point size 10 (.PZ 11 on laser printers).
.\"	    Use with other fonts and/or point sizes may require
.\"	    adjustments to the definitions.
.\"                               *  *  *  *  *
.EQ
define ciplus % "\(a+" %
define citimes % "\(ax" %
define =wig % "\(eq\h'-\w'\(eq'u-\w'\s-2\(ap'u/2u'\v'-.4m'\s-2\z\(ap\(ap\s+2\v'.4m'\h'\w'\(eq'u-\w'\s-2\(ap'u/2u'" %
define -wig % "\s-1\(mi\s0\h'-\w'\s-1\(mi\s0'u-\w'\s-1\(ap'u/2u'\v'-.2m'\s-1\z\(ap\(ap\s+2\v'.2m'\h'\w'\s-1\(mi\s0'u-\w'\s-1\(ap'u/2u'" %
define =dot % "\z\(eq\v'-.6m'\h'.1m'\s+2.\s-2\v'.6m'\h'.2m'" %
define orsign  % "\(lo" %
define andsign % "\(an" %
define =del % "\v'.3m'\z\s+1=\s0\v'-.6m'\h'.07m'\s-1\(*D\s+1\v'.3m'\h'-.07m'" %
define oppA % "\(fa" %
define oppE % "\(te" %
define incl % size -1 "\v'-.2m'\fS\s-3\z\(or\fP\s+3\v'-.25m'\h'.15m'\z\(em\v'.6m'\z\(em\v'.2m'\(em\v'-.35m'" %
define angstrom % "\fR\zA\v'-.3m'\h'.2m'\(de\v'.3m'\fP" %
define star % "\(**" %
define bigstar % "\o'\(pl\(mu'" %
define || % \(or\(or\^ %
define <wig % "\z<\h'.05m'\v'.4m'\(ap\v'-.4m'" %
define >wig % "\z>\v'.4m'\(ap\v'-.4m'" %
define langle % "\(l<" %
define rangle % "\(r>" %
define hbar % "\z\fIh\fP\v'-.55m'\h'.03m'\s-2\(ru\s0\v'.55m'\h'-.03m'" %
define ppd % "\(bt" %
define <-> % "\o'\(<-\(->'" %
define <=> % "\(io" %
define |< % "\z<\v'.02m'\h'.3m'\fS\(or\fP\v'-.02m'" %
define |> % "\z>\v'.02m'\h'.2m'\fS\(or\fP\v'-.02m'" %
define ang % "\v'-.04m'\z\s-2\(sl\s+2\v'.04m'\(ru" %
define rang % "\v'-.1m'\z\s-3\fS\(or\fP\s0\v'.13m'\h'.1m'\(ru\v'-.03m'" %
define 3dot % "\v'-.8m'\z.\v'.5m'\z.\v'.5m'.\v'-.2m'" %
define thf % "\(tf" %
define quarter % roman \(14 %
define 3quarter % roman \(34 %
define degree % \(de %
define square % \(sq %
define circle % \(ci %
define blot % "\(bx" %
define bullet % \(bu %
define wig % \(ap %
define prop % \(pt %
define empty % \(es %
define member % \(mo %
define nomem % "\(!m" %
define cup % \(cu %
define cap % \(ca %
define subset % \(sb %
define supset % \(sp %
define !subset % \(ib %
define !supset % \(ip %
define ell  % "\z\(sl\v'-.1m'\h'-.04m'\s-2\fR(\fP\s0\v'.1m'\h'.04m'" %
define gauint % "\s-3\z\(ci\h'\w'\(ci'u/2u-5u'\s0\(is\s-3\h'\w'\(ci'u/2u-5u'\s0" %
define mplus % "\z+\v'-35u'\h'\w'+'u/2u'\h'-\w'\(mi'u/2u'\(mi\v'+35u'" %
tdefine dprime % "\h'-.2m'\z\(aa\h'.15m'\z\(aa\h'-.15m'\h'.2m'" %
.EN
.\"
.\"  Since the postscript character set has straight vertical greek
.\"  lower-case characters, we need to explicitly slant them.  Since
.\"  slanting effectively widens the character and causes it to crowd
.\"  its surroundings a bit, a small space has been added to the left
.\"  of each character.
.EQ
define alpha    % { back 15 "\S'-12'\^\(*a\S'0'" fwd 15 nothing } %
define beta     % { back 15 "\S'-12'\^\(*b\S'0'" fwd 15 nothing } %
define gamma    % { back 15 "\S'-12'\^\(*g\S'0'" fwd 15 nothing } %
define delta    % { back 5 "\S'-12'\^\(*d\S'0'" fwd 5 nothing } %
define epsilon  % { back 15 "\S'-12'\^\(*e\S'0'" fwd 15 nothing } %
define zeta     % { back 15 "\S'-12'\^\(*z\S'0'" fwd 15 nothing } %
define eta      % { back 15 "\S'-12'\^\(*y\S'0'" fwd 15 nothing } %
define theta    % { back 15 "\S'-12'\^\(*h\S'0'" fwd 15 nothing } %
define iota     % { back 15 "\S'-12'\^\(*i\S'0'" fwd 15 nothing } %
define kappa    % { back 15 "\S'-12'\^\(*k\S'0'" fwd 15 nothing } %
define lambda   % { back 15 "\S'-12'\^\(*l\S'0'" fwd 15 nothing } %
define mu       % { back 15 "\S'-12'\^\(*m\S'0'" fwd 15 nothing } %
define nu       % { back 15 "\S'-12'\^\(*n\S'0'" fwd 15 nothing } %
define xi       % { back 15 "\S'-12'\^\(*c\S'0'" fwd 15 nothing } %
define omicron  % { back 15 "\S'-12'\^\(*o\S'0'" fwd 15 nothing } %
define pi       % { back 15 "\S'-12'\^\(*p\S'0'" fwd 15 nothing } %
define rho      % { back 15 "\S'-12'\^\(*r\S'0'" fwd 15 nothing } %
define sigma    % { back 15 "\S'-12'\^\(*s\S'0'" fwd 15 nothing } %
define tau      % { back 15 "\S'-12'\^\(*t\S'0'" fwd 15 nothing } %
define upsilon  % { back 15 "\S'-12'\^\(*u\S'0'" fwd 15 nothing } %
define phi      % { back 15 "\S'-12'\^\(*f\S'0'" fwd 15 nothing } %
define chi      % { back 5 "\S'-12'\^\(*x\S'0'" fwd 5 nothing } %
define psi      % { back 15 "\S'-12'\^\(*q\S'0'" fwd 15 nothing } %
define omega    % { back 15 "\S'-12'\^\(*w\S'0'" fwd 15 nothing } %
.EN
