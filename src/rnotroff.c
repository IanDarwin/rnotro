/*
 * DEC runoff to troff converter
 */

#include <stdio.h>
#include <ctype.h>
#include <signal.h>

#define STDIN 0		/* standard input file descriptor */
#define STDOUT 1	/* standard output file descriptor */
#define STDERR 2	/* standard diagnostic output file descriptor */

#define T_NROFF 0
#define T_TROFF 1

/* Sundry defines */

#define EOL 0376	/* must fit in a char */
#define MAXLEN		1000
#define MAXCMD		256
#define CMASK		0377 /* for making chars positive */
#define HIGH_VALUE	2147483647L	/* highest value for long integers */

#define NROFF "/usr/bin/nroff"
#define TROFF "/usr/bin/troff"


/* Flag settings for command table */

#define ALIAS 01	/* cmd name is an alias abbreviation */
#define ENTRY 02	/* full entry for cmd in table */
#define SIMPLE 04	/* cmd requires simple text substitution */
#define DIRARG 010	/* cmd requires simple text substitution +
			 * arg translation; arg position is denoted
			 * by ^ in the output sequence */
#define NODELIM 020	/* arg is LETTERS & is not separated from
			 * the cmd name by any special delimiter */
#define LASTCMD 040	/* cmd must be last on input line;
			 * remainder of line is taken as arg */
#define NUARG 0100	/* cmd takes numeric args, default units to be inserted */

static int unstdin;

int appendix();
int autoparagraph();
int autotable();
int centre();
int chapter();
int doindex();
int endfooting();
int endftnote();
int endkeeptogether();
int endlist();
int endliteral();
int endnote();
int endsubtitle();
int endtitle();
int figure();
int fill();
int flagindex();
int footing();
int footnote();
int headerlevel();
int mkindex();
int keeptogether();
int leftmargin();
int list();
int listelement();
int literal();
int nofill();
int nopaging();
int note();
int numberlevel();
int numblist();
int paging();
int paragraph();
int pagesize();
int printindex();
int rightmargin();
int right();
int skip();
int spacing();
int subindex();
int subtitle();
int tabstops();
int title();
int typeset();
int varelse();
int varend();
int variable();
int varif();
int varifnot();

struct command {
	char *cmdname;
	int flags;
	int (*docmd)();	/* used iff ~(SIMPLE|DIRARG|ALIAS) */
	char *outseq;	/* used iff SIMPLE|DIRARG|ALIAS */
} cmdtab[] = {
	"AP",	ALIAS, NULL, "AUTOPARAGRAPH",
	"APPENDIX",	NODELIM | ENTRY | LASTCMD, appendix, NULL,
	"AT",	ALIAS, NULL, "AUTOTABLE",
	"AUTOPARAGRAPH",	ENTRY, autoparagraph, NULL,
	"AUTOTABLE",	ENTRY, autotable, NULL,
	"AX",	ALIAS | NODELIM, NULL, "APPENDIX",
	"B",	ALIAS, NULL, "BLANK",
	"BB",	ALIAS, NULL, "BEGIN BAR",
	"BEGIN BAR",	ENTRY | SIMPLE, NULL, ".if \\n(BA .mc \\(br 1m",
	"BLANK",	ENTRY | DIRARG | NUARG, NULL, ".sp ^",
	"BR",	ALIAS, NULL, "BREAK",
	"BREAK",	ENTRY | SIMPLE, NULL, ".br",
	"C",	ALIAS, NULL, "CENTRE",
	"CENTER",	ALIAS, NULL, "CENTRE",
	"CENTRE",	ENTRY | NUARG, centre, NULL,
	"CH",	ALIAS | NODELIM, NULL, "CHAPTER",
	"CHAPTER",	NODELIM | ENTRY | LASTCMD, chapter, NULL,
	"DBB",	ALIAS, NULL, "DISABLE BAR",
	"DISABLE BAR",	ENTRY | SIMPLE, NULL, ".nr BA 0",
	"DO INDEX",	ENTRY, doindex, NULL,
	"DX",	ALIAS, NULL, "DO INDEX",
	"EB",	ALIAS, NULL, "END BAR",
	"EBB",	ALIAS, NULL, "ENABLE BAR",
	"EF",	ALIAS, NULL, "END FOOTING",
	"EI",	ALIAS | NODELIM, NULL, "ENDIF",
	"EKT",	ALIAS, NULL, "END KEEP TOGETHER",
	"EL",	ALIAS, NULL, "END LITERAL",
	"ELS",	ALIAS, NULL, "END LIST",
	"ELSE",	ENTRY | NODELIM, varelse, NULL,
	"EN",	ALIAS, NULL, "END NOTE",
	"ENABLE BAR",	ENTRY | SIMPLE, NULL, ".nr BA 1",
	"END BAR",	ENTRY | SIMPLE, NULL, ".if \\n(BA .mc",
	"END FOOTING",	ENTRY, endfooting, NULL,
	"END FOOTNOTE",	ENTRY, endftnote, NULL,
	"END KEEP TOGETHER",	ENTRY, endkeeptogether, NULL,
	"END LIST",	ENTRY, endlist, NULL,
	"END LITERAL",	ENTRY, endliteral, NULL,
	"END NOTE",	ENTRY, endnote, NULL,
	"END SUBTITLE",	ENTRY, endsubtitle, NULL,
	"END TITLE",	ENTRY, endtitle, NULL,
	"ENDIF",	ENTRY | NODELIM, varend, NULL,
	"EST",	ALIAS, NULL, "END SUBTITLE",
	"ET",	ALIAS, NULL, "END TITLE",
	"F",	ALIAS, NULL, "FILL",
	"FG",	ALIAS | NODELIM, NULL, "FIGURE",
	"FIGURE",	ENTRY | NODELIM | NUARG, figure, NULL,
	"FILL",	ENTRY, fill, NULL,
	"FIRST TITLE",	ENTRY | SIMPLE, NULL, ".nr FT 1",
	"FLAG INDEX",	ENTRY, flagindex, NULL,
	"FN",	ALIAS, NULL, "FOOTNOTE",
	"FOOTING",	ENTRY | NODELIM | LASTCMD, footing, NULL,
	"FOOTNOTE",	ENTRY, footnote, NULL,
	"FT",	ALIAS, NULL, "FIRST TITLE",
	"FTG",	ALIAS | NODELIM, NULL, "FOOTING",
	"HD",	ALIAS | NODELIM, NULL, "HEADER",
	"HEADER",	ENTRY | SIMPLE, NULL, ".nr HD 1\n.ds PA Page\n",
	"HEADER LEVEL",	ENTRY | LASTCMD, headerlevel, NULL,
	"HEADER LOWER",	ENTRY | SIMPLE, NULL, ".nr HD 1\n.ds PA page\n",
	"HEADER MIXED",	ENTRY | SIMPLE, NULL, ".nr HD 1\n.ds PA Page \n",
	"HEADER UPPER",	ENTRY | SIMPLE, NULL, ".nr HD 1\n.ds PA PAGE\n",
	"HL",	ALIAS | LASTCMD, NULL, "HEADER LEVEL",
	"I",	ALIAS, NULL, "INDENT",
	"IF",	ENTRY | NODELIM, varif, NULL,
	"IFNOT",	ENTRY | NODELIM, varifnot, NULL,
	"IN",	ALIAS | NODELIM, NULL, "IFNOT",
	"INDENT",	ENTRY | DIRARG | NUARG, NULL, ".ti ^",
	"INDEX",	ENTRY | LASTCMD | NODELIM, mkindex, NULL,
	"J",	ALIAS, NULL, "JUSTIFY",
	"JUSTIFY",	ENTRY | SIMPLE, NULL, ".ad",
	"KEEP TOGETHER",	ENTRY, keeptogether, NULL,
	"KT",	ALIAS, NULL, "KEEP TOGETHER",
	"L",	ALIAS, NULL, "LEFT",
	"LE",	ALIAS, NULL, "LIST ELEMENT",
	"LEFT",	ENTRY | DIRARG | NUARG, NULL, ".ti ^",
	"LEFT MARGIN",	ENTRY | NUARG, leftmargin, NULL,
	"LIST",	ENTRY | NUARG, list, NULL,
	"LIST ELEMENT",	ENTRY, listelement, NULL,
	"LITERAL",	ENTRY, literal, NULL,
	"LM",	ALIAS, NULL, "LEFT MARGIN",
	"LS",	ALIAS, NULL, "LIST",
	"LT",	ALIAS, NULL, "LITERAL",
	"NAP",	ALIAS, NULL, "NO AUTOPARAGRAPH",
	"NAT",	ALIAS, NULL, "NO AUTOTABLE",
	"NF",	ALIAS, NULL, "NOFILL",
	"NHD",	ALIAS, NULL, "NO HEADER",
	"NJ",	ALIAS, NULL, "NOJUSTIFY",
	"NM",	ALIAS, NULL, "NUMBER PAGE",
	"NNM",	ALIAS, NULL, "NO NUMBER",
	"NO AUTOPARAGRAPH",	ENTRY, autoparagraph, NULL,
	"NO AUTOTABLE",	ENTRY, autotable, NULL,
	"NO FILL",	ALIAS, NULL, "NOFILL",
	"NO FOOTINGS",	ENTRY | SIMPLE, NULL, ".NF 0 0",
	"NO FOOTINGS EVEN",	ENTRY | SIMPLE, NULL, ".NF 1 0",
	"NO FOOTINGS ODD",	ENTRY | SIMPLE, NULL, ".NF 0 1",
	"NO HEADER",	ENTRY | SIMPLE, NULL, ".nr HD 0",
	"NO JUSTIFY",	ALIAS, NULL, "NOJUSTIFY",
	"NO NUMBER",	ENTRY | SIMPLE, NULL, ".PN 0",
	"NO PAGING",	ENTRY, nopaging, NULL,
	"NO SUBTITLES",	ENTRY | SIMPLE, NULL, ".NS 0 0",
	"NO SUBTITLES EVEN",	ENTRY | SIMPLE, NULL, ".NS 1 0",
	"NO SUBTITLES ODD",	ENTRY | SIMPLE, NULL, ".NS 0 1",
	"NO TITLES",	ENTRY | SIMPLE, NULL, ".NT 0 0",
	"NO TITLES EVEN",	ENTRY | SIMPLE, NULL, ".NT 1 0",
	"NO TITLES ODD",	ENTRY | SIMPLE, NULL, ".NT 0 1",
	"NOFILL",	ENTRY, nofill, NULL,
	"NOJUSTIFY",	ENTRY | SIMPLE, NULL, ".na",
	"NONUMBER",	ALIAS, NULL, "NO NUMBER",
	"NOTE",	ENTRY | NODELIM | LASTCMD, note, NULL,
	"NPA",	ALIAS, NULL, "NO PAGING",
	"NST",	ALIAS, NULL, "NO SUBTITLES",
	"NSTE",	ALIAS, NULL, "NO SUBTITLES EVEN",
	"NSTO",	ALIAS, NULL, "NO SUBTITLES ODD",
	"NT",	ALIAS | NODELIM, NULL, "NOTE",
	"NTL",	ALIAS, NULL, "NO TITLES",
	"NTLE",	ALIAS, NULL, "NO TITLES EVEN",
	"NTLO",	ALIAS, NULL, "NO TITLES ODD",
	"NUMBER",	ALIAS, NULL, "NUMBER PAGE",
	"NUMBER APPENDIX",	ENTRY | DIRARG, NULL, ".nr ap ^",
	"NUMBER CHAPTER",	ENTRY | DIRARG, NULL, ".nr cn ^",
	"NUMBER LEVEL",	ENTRY, numberlevel, NULL,
	"NUMBER LIST",	ENTRY, numblist, NULL,
	"NUMBER PAGE",	ENTRY | DIRARG, NULL, ".PN ^",
	"P",	ALIAS, NULL, "PARAGRAPH",
	"PA",	ALIAS, NULL, "PAGING",
	"PAGE",	ENTRY | SIMPLE, NULL, ".sp \\n(.tu",
	"PAGE SIZE",	ENTRY | NUARG, pagesize, NULL,
	"PAGING",	ENTRY, paging, NULL,
	"PAPER SIZE",	ALIAS, NULL, "PAGE SIZE",
	"PARAGRAPH",	ENTRY | NUARG, paragraph, NULL,
	"PG",	ALIAS, NULL, "PAGE",
	"PRINT INDEX",	ENTRY, printindex, NULL,
	"PS",	ALIAS, NULL, "PAGE SIZE",
	"PX",	ALIAS, NULL, "PRINT INDEX",
	"R",	ALIAS, NULL, "RIGHT",
	"RIGHT",	ENTRY | NUARG, right, NULL,
	"RIGHT MARGIN",	ENTRY | NUARG, rightmargin, NULL,
	"RM",	ALIAS, NULL, "RIGHT MARGIN",
	"S",	ALIAS, NULL, "SKIP",
	"SK",	ALIAS, NULL, "SKIP",
	"SKIP",	ENTRY, skip, NULL,
	"SP",	ALIAS, NULL, "SPACING",
	"SPACING",	ENTRY, spacing, NULL,
	"ST",	ALIAS | NODELIM, NULL, "SUBTITLE",
	"SUBINDEX",	ENTRY | NODELIM | LASTCMD, subindex, NULL,
	"SUBTITLE",	ENTRY | NODELIM | LASTCMD, subtitle, NULL,
	"T",	ALIAS | NODELIM, NULL, "TITLE",
	"TAB STOPS",	ENTRY | NUARG, tabstops, NULL,
	"TEST PAGE",	ENTRY | DIRARG | NUARG, NULL, ".br\n.ne ^",
	"TITLE",	ENTRY | NODELIM | LASTCMD, title, NULL,
	"TP",	ALIAS, NULL, "TEST PAGE",
	"TS",	ALIAS, NULL, "TAB STOPS",
	"TY",	ALIAS | NODELIM, NULL, "TYPESET",
	"TYPESET",	ENTRY | NODELIM, typeset, NULL,
	"VARIABLE",	ENTRY | NODELIM, variable, NULL,
	"VR",	ALIAS | NODELIM, NULL, "VARIABLE",
	"X",	ALIAS | NODELIM, NULL, "SUBINDEX",
};

#define CMDCNT (sizeof cmdtab/sizeof cmdtab[0])

char *progname;
int tproc = T_NROFF;	/* text processor */
int ipmp[2];	/* file descriptors for pipes */
int nvar = 0;	/* number of variants encountered */
int debug = 0;	/* TRACE flag */
int killer = 0;	/* flag to indicate no output on error  */
int tprocid = 0;	/* process is'd used ... */
int iprocid = 0;	/* ... for killing tasks */
int subtproc();
char *varlist[30];	/* array of pointers to variant names */

main(argc, argv) /* runoff converter -- main program */
int argc;
char *argv[];
{
	char *w;
	int c, errflg = 0;
	extern char *optarg;
	extern int optind;

	nvar = 0;
	progname = argv[0];
	while ((c = getopt(argc, argv, "dntv:")) != EOF)
		switch (c) {
		case 'd':	/* debug flag */
			++debug;
			break;
		case 'n':	/* text processor is nroff */
			tproc = T_NROFF;
			break;
		case 't':	/* text processor is troff */
			tproc = T_TROFF;
			break;
		case 'v':	/* variant selection specification */
			if (nvar == 0) {
				w = optarg;
				if (*w != ':')
					fprintf(stderr,
					"command line: ':' expected but %c found\n",
						*w);
				varlist[nvar++] = w+1;
				while (*++w != '\0')
					if (*w == ',') {
						*w = '\0';
						varlist[nvar++] = w+1;
					}
			}
			break;
		default:
			errflg++;
			break;
		}
	if (optind < argc || errflg)
		error("usage: %s -[nt] [-v:v1,v2...]\n", progname);
	/*
	 arguments now validated.
	 */
	mainproc();	/* off we go... */
	exit(0);
}

#define	SAMEENT 1
#define	DIFFENT 0
#define	SUBIND	1
#define	IND	2
#define	MAXENT	500

static char *startsig = "*|->";
static char *sortfile;




/* Settings for <case_lock> */

#define	MIXED		0
#define	LOWER		1
#define	UPPER		2


/* Settings for <trap_type> */

/*	<UNUSED>	1  */
#define HDTITLE		2
#define KEEPTOGETHER	3
#define TITLE		4
#define FOOTNOTE	5
#define FOOTING		6
#define SUBTITLE	7
#define LITERAL		8


/* Surrogate characters in input text */

#define	UNDERSCORE	'_'
#define POUNDSIGN	'#'
#define AMPERSAND	'&'
#define BACKSLASH	'\\'
#define	QUOTE		'\''
#define	INDEXFLAG	'>'
#define CAP		'^'
#define COMMAND		'.'
#define ENDFTNOTE	'!'


/* Settings for <format_trap> */

#define NOTRAP		0
#define CENTRE		1

static char buf[MAXLEN];
static char curcmd[MAXCMD], curarg[MAXCMD];
static char vstack[50][15];	/* stores the variant stack in IF, IFNOT, ELSE cmds */
static int stackptr = 0;
static int ELstack[50];	/* flags to indicate whether an .el is needed at ENDIF */
static int bufptr;
static int chapnum = 0;	/* flag chapter number */
static int inlines = 0;	/* input line counter for diagnostic output */
int iscmd;	/* current input line is a command */
int lastcase;
int case_lock = MIXED;	/* default typefont case */
int trap_lines = -1;	/* input line counter for conversion trap, triggered when 0 */
int trap_type = 0;	/* type of trap triggered when <trap_lines> becomes 0 */
int format_trap = 0;	/* trap for text formating commands */
int auto_flag = 0;	/* default flag for AUTOPARAGRAPH and AUTOTABLE */
int last_flag = 0;	/* memory flag when auto mode is temporarily turned off */
int kauto = 0;	/* memory flag when going into keeptogether mode */
int klast = 0;	/* memory flag when going into keeptogether mode */
int kmode = 0;	/* memory flag when going into keeptogether mode */
double auto_indent = 5;	/* default indent for AUTOPARAGRAPH and AUTOTABLE */
double auto_spacing = 1;	/* default spacing for same */
double auto_testpage = 0;	/* default test page parameter for same */
char auto_tu = 'P';	/* units for auto_testpage */
char auto_su = 'P';	/* unit for auto_spacing */
char auto_iu = 'P';	/* unit for auto_indent */
int notenest = 0;	/* indicate nesting level of NOTE */
int listnest = 0;	/* indicate nesting level of LISTS */
int contdoline = 1;	/* indicate continuing of doline routine */
int level[5] = { 0, 0, 0, 0, 0 };	/* counters for header levels */
int LEcount[5] = { 1, 1, 1, 1, 1 };	/* counters for list elements */
double listspacing[5] = { 1., 1., 1., 1., 1. };	/* Spacing for lists */
char list_su[5] = { 'i', 'i', 'i', 'i', 'i' };	/* List spacing units */
int fillmode = 1;	/* indicate fill mode */
int indexflag = 0;	/* indicate if index is to be kept */
int altarg = 0;	/* indicate if alternate measurement unit is user supplied */
double convert();
double cfactor = 240;	/* conversion factor to basic units */
double Csize = 20;	/* conversion factor C to basic units */
double Psize = 10;	/* default point size */
double Vsize = 12;	/* default vertical line space */
double height = 5.8;	/* default, in inches */
double width = 6.0;	/* default, in inches */
char hu = 'i';	/* default units for height */
char wu = 'i';	/* default units for width */

/* NOTE: If you change the page size defaults,
 * remember to change macro initialisation as well! */

mainproc() /* main conversion processor */
{
#ifdef LOCAL
	kprintf(".so runoffmacros\n");
#else
	/* initialisation and conversion macros */
#ifdef DEFAULTVERSION
	kprintf(".so /usr/misc/lib/runoffmacros\n");
#else
	kprintf(".so /usr/misc/lib/nrunoffmacros\n");
#endif
#endif	/* LOCAL */
	unstdin = ipmp[0];	/* pipe input becomes unstdin */
	if (tproc != T_NROFF)
		cfactor = 432;
	while (getline(buf, MAXLEN) > 0)
		doline(buf, iscmd);
}

/*
 * Convert a single line of input text.
 * If <arg> is negative, then this is a special call from a command conversion
 * routine.  In particular, the buffer for such calls is terminated by a '\0'
 * rather than by EOL.
 */
doline(bufp, arg)
register char bufp[MAXLEN];
int arg;
{
	register int c, ch;
	int bufpsave;
	int eol;
	int eolok = 1;
	int newline;	/* on if next char is start of a logical line */
	int newlflag;	/* next setting of <newline> */
	int index;
	char *dirarg;
	char indexbuf[MAXLEN], *xptr;

	if (debug)
		fprintf(stderr, "entering doline\n");	/* TRACE */
	bufptr = 0;	/*** reset bufptr, for command conversion subroutines ***/
	newline = trap_type != LITERAL;	/* never on if LITERAL mode */
	if (arg < 0) {
		eol = '\0';
		iscmd = 0;
		while ((c = getch(bufp, 0)) == ' ' || c == '\t')
			;	/* get rid of leading blanks */
		--bufptr;	/* save the non-blank character */
	}
	else {
		eol = EOL;
		if (bufp[bufptr] == ENDFTNOTE && trap_type == FOOTNOTE) {
			endftnote();
			getarg(0, 0, curarg);
		}
	}
	while ((c = getch(bufp, 0)) != eol && eolok && contdoline) {
		newlflag = 0;
		switch (c) {
		case UNDERSCORE:	/* treat next character as text */
			if (eolok = (ch = getch(bufp, 0)) != eol)
				if (ch == '\\')
					kprintf("\\&\\\\\\&");
				else
					kprintf("\\&%c", ch);
			break;
		case POUNDSIGN:	/* unexpandable space */
			kprintf("\\ ");
			break;
		case AMPERSAND:	/* underline next character */
			if (eolok = (ch = getch(bufp, 0)) != eol) {
				kprintf("\\fI");
				kprintf("%c", ch);
				kprintf("\\fR");
			}
			break;
		case CAP:	/*	^	next character upper case
				 ^^  upper case lock
				 ^\  mixed case lock
				 ^&  underline lock */
			if (eolok = (ch = getch(bufp, 0)) != eol)
				switch (ch) {
				case CAP:	/* upper case lock */
					case_lock = UPPER;
					break;
				case BACKSLASH:	/* mixed case lock */
					case_lock = MIXED;
					break;
				case AMPERSAND:	/* underline lock */
					kprintf("\\fI");
					break;
				default:	/* next character upper case */
					kprintf("%c", upper(ch));
					break;
				}
			break;
		case BACKSLASH:	/*	\	next character lower case
				 \\  lower case lock
				 \^  mixed case lock
				 \&  underline lock off */
			if (eolok = (ch = getch(bufp, 0)) != eol)
				switch (ch) {
				case BACKSLASH:	/* lower case lock */
					case_lock = LOWER;
					break;
				case CAP:	/* mixed case lock */
					case_lock = MIXED;
					break;
				case AMPERSAND:	/* underline lock off */
					kprintf("\\fR");
					break;
				default:	/* next character lower case */
					kprintf("%c", lower(ch));
					break;
				}
			break;
		case QUOTE:	/* single quote mark */
			/* escape it in case it is at the start of a line */
			kprintf("\\&'");
			break;
		case INDEXFLAG:	/* next char index entry */
			if (indexflag) {
				for (xptr = indexbuf; *xptr = getch(bufp, 0),
				    (!isascii(*xptr) || !isspace(*xptr)) &&
				    *xptr != '\n'; xptr++)
					kprintf("%c", *xptr);
				*xptr = '\0';
				kprintf("\n.ds IE ~%s\n.IN\n", indexbuf);
			}
			else
				/* flag not enabled; treat character as text */
				kprintf("%c", c);
			break;
		case COMMAND:	/* format command line */
			if (iscmd) {
				/* save in case command is suppressed */
				bufpsave = bufptr;
				index = getcmd(curcmd);	/* get current command */
				if (trap_type == LITERAL)
					if (
#ifdef CHKINDEX
	/* Although this makes sense, it breaks error printing. */
					    index >= 0 &&
#endif
					    strlcmp(-1, cmdtab[index].cmdname,
					    "END LITERAL") != 0) {
						iscmd = 0;	/* ignore this cmd */
						/* and restore bufptr */
						bufptr = bufpsave;
					}
				if (iscmd) {
#ifdef CHKINDEX
	/* Although this makes sense, it breaks error printing. */
					if (index >= 0)
#endif
						getarg(cmdtab[index].flags&LASTCMD,
						    cmdtab[index].flags&NUARG,
						    curarg);	/* and any arg */
					if (debug)
						fprintf(stderr,
							"command: %s^argument: %s^",
							 curcmd, curarg);	/* TRACE */
					if (index < 0) {
						diagnosline();
						fprintf(stderr,
							"command not recognized: %s %s\n"
							, curcmd, curarg);
					}
					else if (cmdtab[index].flags&SIMPLE) {
						kprintf(cmdtab[index].outseq);
						kprintf("\n");
					}
					else if (cmdtab[index].flags&DIRARG) {
						dirarg = cmdtab[index].outseq;
						while ((c = *dirarg++) != '^')
						/* this char precedes the arg. */
							kprintf("%c", c);
						kprintf("%s%s\n", curarg, dirarg);
					}
					else	/* call command function */
						(*cmdtab[index].docmd)();
					/* more input may follow command */
					newlflag = 1;
					break;
				}
			}
			/* else it's just an ordinary period; treat it as default */
			kprintf("\\&");	/* deactivate the period */
		default:
			if (newline)
				if (auto_flag == (isascii(c) && isspace(c)? 1: -1) &&
				    trap_type < 4) {
					if (auto_flag == -1)	/*Autotable */
						--bufptr;
					emitauto();	/* AUTOPARAGRAPH and AUTOTABLE */
					/* get 1st real (non-blank) char */
					break;
				}
			if (case_lock == LOWER)
				c = lower(c);
			else if (case_lock == UPPER)
				c = upper(c);
			kprintf("%c", c);
			break;
		}
		newline = newlflag;	/* isn't this just great? */
	}
	checktrap();	/* check for possible input trap after this line */
	if (eolok != 1) {
		diagnosline();
		fprintf(stderr, "unexpected end-of-line, continuing\n");
	}
	contdoline = 1;
}



/* check the variant list to see if var is in selected list */
vcheck(var)
char *var;
{
	int i = 0;

	while (i < nvar)
		if (strlcmp(-1, varlist[i++], var) == 0)
			return 1;
	return 0;
}

lower(c) /* convert <c> to lower case if it was upper case; ASCII only */
register int c;
{
	return (isascii(c) && isupper(c))? tolower(c): c;
}

upper(c) /* convert <c> to upper case if it was lower case; ASCII only */
register int c;
{
	return (isascii(c) && islower(c))? toupper(c): c;
}

strlcmp(len, i, j) /* logical comparison of string i vs. string j for len chars
	 (or all chars if len is -1) */
char *i, *j;
int len;
{
	for (; *i == *j; i++, j++)
		if (--len == 0 || *i == '\0')
			return 0;
	return *i - *j;
}

double
convert(arg, unit) /* convert argument in some unit to basic units */
double arg;
char unit;
{
	switch (unit) {
	case 'i':	/* inches */
		return arg*cfactor;
	case 'c':	/* centimetres */
		return arg*cfactor*0.39;
	case 'P':	/* Picas */
		return arg*cfactor/6;
	case 'm':	/* ems */
		if (tproc != T_NROFF)
			return arg*6*Psize;
		else
			return arg*Csize;
	case 'n':	/* ens */
		if (tproc != T_NROFF)
			return arg*3*Vsize;
		else
			return arg*Csize;
	case 'p':	/* points */
		if (tproc != T_NROFF)
			return 6*arg;
		else
			return 3.4*arg;
	case 'v':	/* Vs */
		return arg*Vsize;
	case 'u':	/* basic units */
		return arg;
	default:	/* unrecognized unit character */
		diagnosline();
		fprintf(stderr, "unrecognized units, use: i,c,P,m,n,p,u,v\n");
		return HIGH_VALUE;
	}
}

/*
 * Lookup command name in table; return index of row or -1 if no match.
 * If <len> is -1, the command name is assumed to be terminated by a '\0';
 * otherwise, the search terminates after the first <len> characters
 * and only commands with the NODELIM bit set are considered.
 */
lookup(len, cmd)
int len;
char *cmd;
{
	int low, high, mid, comp;
	int cmdlen;

	low = 0;
	high = CMDCNT-1;
	while (low <= high) {
		mid = (low+high)/2;
		if ((comp = strlcmp(len, cmd, cmdtab[mid].cmdname)) < 0)
			high = mid-1;
		else if (comp > 0)
			low = mid+1;
		else {
			if (len == -1 || cmdtab[mid].flags&NODELIM &&
			    (cmdlen = strlen(cmdtab[mid].cmdname)) == len) {
				/* found it; find full entry in command table */
				if (cmdtab[mid].flags&ALIAS)
					if ((mid = lookup(-1,
					    cmdtab[mid].outseq)) < 0)
						error(
					"fatal error in command table\n",
							(char *) 0);
				return mid;
			}
			else if (cmdlen > len)
				high = mid-1;
			else {
				/* lengths must be equal but NODELIM was off. */
				if (cmdtab[mid].flags&NODELIM)
					/* should never get here */
					fprintf(stderr,
						"internal error in command table\n");
				else
					return -1;
			}
		}
	}
	return -1;	/* command not found */
}

error(s1, s2) /* print error message and terminate */
char *s1, *s2;
{
	diagnosline();
	if (s2 == 0)
		fprintf(stderr, s1);
	else
		fprintf(stderr, s1, s2);
	exit(1);	/* signal abnormal exit */
}

/* move arg. to current cmd in <buf> to desired location for processing */
getarg(last, nuarg, w)
char *w;
int last, nuarg;
{
	char *p;
	int c;
	int numflag = 0;	/* flag numeric character presence in argument */
	int lim = MAXCMD;
	int parity = 0;	/* cumulative parity of quotes on the line */

	altarg = 0;
	/*
	 * The end of a command is generally designated by the first
	 * occurrence of ! . ; that is not within quotes, or a newline.
	 * If <last> is set, however, then this is the last command
	 * on the line, and the only possible argument terminator is newline.
	 */
	p = w;
	while (--lim > 0) {
		/* i.e. modulo 2 addition */
		parity = parity != ((c = getch(buf, 0)) == '"');
		if (nuarg) {
			if (isascii(c) && isdigit(c)) {
				/*
				 * once a digit is scanned numflag is set
				 * so that at the end of the number,
				 * a unit can be added.
				 */
				numflag = 1;
				*p++ = c;
			}
			else {
				if (numflag) {
					/* default units are inches/10 */
					(void) sprintf(p-1, ".%ci%c", *(p-1), c);
					p += 3;
					numflag = 0;
				}
				else
					*p++ = c;
			}
		}
		else if (c == UNDERSCORE)	/* next character is escaped */
			/* ignore escape character and get following one */
			*p++ = getch(buf, 0);
		else
			*p++ = c;
		if (!((parity || last || c != '!' && c != '.' && c != ';') &&
		    c != '\n'))
			break;
	}
	if (c == '.')
		--bufptr;	/* restore indication of subsequent command */
	if (c == '!') {	/* comment follows current command argument */
		/* check for alternate unit measurements */
		if ((c = getch(buf, 0)) == '<') {
			altarg = 1;
			p = w;	/* reassign curarg */
			while (--lim > 0 && (c = *p++ = getch(buf, 0)) != '>' &&
			    c != '\n' && c != ';')
				;
			if (getch(buf, 0) != '\n')
				--bufptr;	/* remove CR if it is there */
		}
		else {
			--bufptr;
			while (--lim > 0 && (c = getch(buf, 0)) != ';' && c != '\n')
				;
		}
	}
	if (lim <= 0) {	/* line was too long and was truncated */
		diagnosline();
		fprintf(stderr, "command line too long, truncated\n");
	}
	if (c == ';')
		if ((c = buf[bufptr]) != COMMAND)
			iscmd = 0;	/* text follows */
	*(p-1) = '\0';
}

/*
 * <eolflag> is 1 if we are treating text sequentially, so that the occurrence
 * of a newline character causes us to fetch the next line of text.  otherwise,
 * the flag is 0.
 */
getch(bufp, eolflag)
register char bufp[MAXLEN];
int eolflag;
{
	register int c;

	while ((c = bufp[bufptr++]&CMASK) == EOL && eolflag)
		(void) getline(bufp, MAXLEN);
	return c == (EOF&CMASK)? EOL: c;
}

getline(bufp, lim) /* get next line of input and reset cursor position */
register char *bufp;
int lim;
{
	int c, i, len;

	do {
		for (len = 0; --lim > 0 && (c = getchar()) != EOF && c != '\n';
		    len++)
			bufp[len] = c;
		if (c == '\n')
			bufp[len++] = c;	/* preserve carriage returns */
		bufp[len] = (c == EOF? EOF: EOL);
		++inlines;
	/* ignore blank lines if not LITERAL */
	} while (bufp[0] == '\n' && trap_type != LITERAL);
	iscmd = bufp[bufptr = 0] == COMMAND &&
		(trap_type != LITERAL || trap_lines < 0);
	if (iscmd == 0)
		--trap_lines;	/* decrement input line counter if not a command */
	if (debug) {
		fprintf(stderr, "getline: ");
		for (i = 0; i < len; i++)
			fprintf(stderr, "%c", bufp[i]);
		fprintf(stderr, "^ len=%d\n", len);
	}	/* TRACE */
	return len;	/* return current length of buffer as result */
}

/*
 * Move command in <buf> to argument for processing,
 * and find its index in the table.
 * The command name is terminated by the first non-blank non-letter character.
 * Multiple blanks separating multi-word command names are squeezed out.
 */
getcmd(w)
char *w;
{
	char *cmd = w;
	int c, index = -1;
	int lim = MAXCMD;

	while (--lim > 0) {
		if (0 == isalpha(c = *w++ = getch(buf, 0))) {
			if (c == ' ') {
				if ((index = lookup(w-cmd-1, cmd)) >= 0)
				/* command found; rest of line is argument */
					break;
				if (*(w-2) == ' ')
					--w;	/* ignore consecutive blanks in input*/
			}
			else {
				--bufptr;
				break;
			}
		}
		else
			*(w-1) = upper(c);
	}
	if (*(w-2) == ' ')
		--w;
	*(w-1) = '\0';
	if (index < 0)
		index = lookup(-1, cmd);

	/* If NODELIM type commands are followed by a ';', it should be
	 * ignored.
	 */

	if (index >= 0)
		if ((cmdtab[index].flags&NODELIM) && buf[bufptr] == ';')
			(void) getch(buf, 0);
	return index;
}

/* Killable printf */
/* VARARGS1 */
kprintf(fmt, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12)
char *fmt, *a0, *a1, *a2, *a3, *a4, *a5, *a6, *a7, *a8, *a9, *a10, *a11, *a12;
{
	if (killer != -1)
		printf(fmt, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9,
			a10, a11, a12);
}

transfer(fd1, fd2) /* direct transfer from file descriptor 1 to fd2 */
register int fd1, fd2;
{
	char tbuf[BUFSIZ];
	register int n;

	(void) fflush(stderr);
	while ((n = read(fd1, tbuf, BUFSIZ)) > 0)
		(void) write(fd2, tbuf, n);
}

checktrap() /* check for possible conversion trap on this input line */
{
	if (trap_lines == 0) {	/* if zero, there is something to do */
		switch (trap_type) {
		case HDTITLE:	/* complete header title format */
			kprintf("\n.sp\n");
			break;
		case LITERAL:	/* clean up after LITERAL command */
			kprintf(".RL\n");	/* invoke cleanup macro */
			break;
		default:
			error("fatal error -- no input trap found", (char *)NULL);
		}
		--trap_lines;	/* make sure it's negative */
		trap_type = 0;	/* and clear the trap type */
	}

	switch (format_trap) {
	case NOTRAP:	/* nothing to do */
		break;
	case CENTRE:	/* clean up, reset line length */
		kprintf(".ll\n");
		break;
	default:
		diagnosline();
		error("fatal error -- improper format trap found", (char *)NULL);
		break;
	}
	format_trap = NOTRAP;
	if (fillmode == 0 || trap_type >= 4)
		/* RUNOFF NOFILL mode requires break after each line */
		kprintf(".br\n");
}

cmdbad() /* print a message indicating the current command is invalid */
{
	diagnosline();
	/* couldn't be much easier */
	fprintf(stderr, "invalid command: %s %s\n", curcmd, curarg);
}

/* Command conversion subroutines. */
/* These are invoked implicitly via command table references. */

appendix() /* sets up new appendix */
{
	case_lock = MIXED;
	chapnum = 0;
	if (*curarg != '\0') {	/* title argument present */
		kprintf(".TT 0 1 1\n");
		doline(curarg, -1);
		contdoline = 0;
		kprintf("\n.RT\n");
	}
	else
		kprintf(".NT 0 0\n");
	kprintf(".NS 0 0 \n");
	kprintf(".AP\n");	/* set up appendix macro */
}



autoparagraph() /* set flag for AUTOPARAGRAPH and NO AUTOPARAGRAPH */
{
	auto_flag = curcmd[0] == 'A';
}



autotable() /* set flag for AUTOTABLE and NO AUTOTABLE */
{
	auto_flag = -(curcmd[0] == 'A');
}

centre() /* centre text string on this or subsequent line */
{
	setlinelength();
	kprintf(".ce\n");
	format_trap = CENTRE;
	if ((buf[bufptr]&CMASK) == EOL)	/* no arg. text; text is next line */
		(void) getline(buf, MAXLEN);
	iscmd = 0;
}

chapter() /* sets up new chapter */
{
	case_lock = MIXED;
	chapnum = 1;
	if (*curarg != '\0') {	/* title argument present */
		kprintf(".TT 0 1 1\n");
		doline(curarg, -1);
		contdoline = 0;
		kprintf("\n.RT\n");
	}
	else
		kprintf(".NT 0 0\n");
	kprintf(".NS 0 0\n");
	kprintf(".PN 2\n");
	kprintf(".CH \n");
}

diagnosline() /* puts out the current buf content during diagnostic output */
{
	register int c;
	int i;

	i = 0;
	fprintf(stderr, "line %d: ", inlines);
	while ((c = buf[i++]) != '\n' && c != '\0')
		putc(c, stderr);
	fprintf(stderr, ":***-> ");
	if (killer) {	/* -k option set and we just detected the first error
			 * so we kill child processes sob, sob.....
			 * and stop output to stdout	*/
		(void) fflush(stdout);
		(void) close(STDOUT);
		if (tprocid != 0) {	/* avoid wiping us out if this is an early error */
			(void) kill(tprocid, SIGTERM);
			(void) kill(iprocid, SIGTERM);
		}
		(void) unlink(sortfile);
		killer = -1;
	}
}

doindex() /* puts out paginated index */
{
	if (killer != -1) {
		kprintf(".tm %s\n", startsig);
		kprintf(".DX");
		(void) fflush(stdout);
		transfer(unstdin, STDOUT);
		kprintf(".RX\n");	/* reset formatting in effect before calling index */
	}
}

emitauto() /* emits a command sequence for paragraphing or tabling */
{
	kprintf(".br\n.ti \\n(.iu+%.1f%c\n.sp %.1f%c\n", auto_indent, auto_iu,
		 auto_spacing, auto_su);
	if (auto_testpage)	/* optional third parameter */
		kprintf(".ne %.1f%c\n", auto_testpage, auto_tu);
	while (isspace(getch(buf, 0)))
		;	/* remove leading white space */
	--bufptr;
}



endfooting() /* end block of footing text */
{
	if (trap_type == FOOTING) {
		trap_type = 0;	/*clear it */
		kprintf(".RF\n");
	}
	else {
		diagnosline();
		fprintf(stderr, "END FOOTING not preceded by FOOTING command\n");
	}
}

endftnote() /* end block of footnote */
{
	if (trap_type != FOOTNOTE) {
		diagnosline();
		fprintf(stderr, "END FOOTNOTE not preceded by FOOTNOTE command\n");
	}
	else {
		trap_type = 0;
		case_lock = lastcase;
		kprintf(".EF\n");
	}
}



endkeeptogether() /* reset mode at end of keeptogether text */
{
	if (trap_type == KEEPTOGETHER) {
		trap_type = 0;
		kprintf(".EK\n");
		fillmode = kmode;
		auto_flag = kauto;
		last_flag = klast;
	}
	else {
		diagnosline();
		fprintf(stderr, "END KEEP TOGETHER not preceded by KEEP TOGETHER\n");
	}
}



endlist() /* reset mode at end of list */
{
	if (--listnest < 0) {
		diagnosline();
		fprintf(stderr, "END LIST command not preceded by LIST command\n");
		listnest = 0;
	}
	else {
		kprintf(".sp %.1f%c\n", listspacing[listnest], list_su[listnest]);
		if (listnest == 0)
			kprintf(".br\n.RD\n");
		else
			kprintf(".in %dP+\\nIu\n", listnest*4 + 1);
	}
}



endliteral() /* end block of literal text */
{
	if (trap_type == LITERAL) {
		trap_type = 0;	/* clear trap type */
		kprintf(".RL\n");	/* invoke cleanup macro */
	}
	else {
		diagnosline();
		fprintf(stderr, "END LITERAL not preceded by LITERAL command\n");
	}
}



endnote() /* end note section */
{
	if (--notenest < 0) {
		diagnosline();
		fprintf(stderr, "END NOTE not preceded by NOTE command\n");
		notenest = 0;
	}
	else {
		kprintf(".sp 2P\n.ll +15\n.in -15\n");
		if (notenest == 0)	/* no more notes */
			kprintf(".nr -D 0\n");

	}
}



endsubtitle() /* end block of subtitle text */
{
	if (trap_type == SUBTITLE) {
		trap_type = 0;	/* clear trap*/
		kprintf(".RS\n");	/*clean up macro */
	}
	else {
		diagnosline();
		fprintf(stderr, "ENDSUBTITLE not preceded by SUBTITLE command");
	}
}



endtitle() /* end block of title text */
{
	if (trap_type == TITLE) {
		trap_type = 0;	/* clear trap type */
		kprintf(".RT\n");	/* invoke cleanup macro */
	}
	else {
		diagnosline();
		fprintf(stderr, "END TITLE not preceded by TITLE command\n");
	}
}



figure() /* convert figure command */
{
	char c, *w;

	w = curarg;
	while ((c = *w++) != '\0')
		if (isdigit(c) || c == '.')	/* check for digit or decimal point */
			break;
	--w;
	switch (upper(*curarg)) {
	case 'T':	/* put figure at top of next page */
		kprintf(".nr FG %s\n", w);
		break;
	case 'B':	/* put figure at bottom of next page */
		kprintf(".ch TF -\\nBu-%s\n", w);
		break;
	case 'A':	/* put figure at first available top or bottom */
		kprintf(".ie \\n(.tu<%s .nr FG %s\n.el .ch TF -(%s+\\nBu)\n", w, w,
			 w);
		break;
	default:	/* include the case when arg is not specified */
		kprintf(".ie \\n(.tu<%s .nr FG %s\n.el .sp %s\n", w, w, w);
		break;
	}
}



fill() /* set mode for fill text */
{
	if (fillmode == 0 && trap_type < 4) {
		/* reset last mode before nofill mode was on */
		auto_flag = last_flag;
		fillmode = 1;
	}
	kprintf(".ad b\n.ev 1\n.ad b\n.ev\n");
}

flagindex() /* set mode for receiving index entries to be sorted in sortfile */
{
	indexflag = 1;
}



footing() /* set mode for footing text input */
{
	char *w;
	int even, odd;

	even = odd = 1;	/*default footing on all pages */
	w = curarg;
	if (0 == strlcmp(4, w, "EVEN")*strlcmp(4, w, "even")) {
		odd = 0;
		w += 4;

	}
	else if (0 == strlcmp(3, w, "ODD")*strlcmp(3, w, "odd")) {
		even = 0;
		w += 3;
	}
	kprintf(".FF %d %d \n", odd, even);
	if (*w == ' ' || *w == ';')
		++w;
	setfooting(w);
}


footnote() /* set mode for footnotes */
{
	if (trap_type == FOOTNOTE) {
		diagnosline();
		fprintf(stderr, "footnote nesting not allowed, command ignored\n");
	}
	else {
		trap_type = FOOTNOTE;
		lastcase = case_lock;
		case_lock = MIXED;
		kprintf(".SF\n");
	}
}



headerlevel() /* set mode for specified header level */
{
	char *w, c;
	int i, j;

	w = curarg;
	for (; ; ) {
		if (*w++ != ' ') {
			if (((c = *(w-1)) <= '5') && (c >= '1'))
				i = c-'1';
			else {
				diagnosline();
				fprintf(stderr, "header level not in range 1-5\n");
			}
			break;
		}
	}
	kprintf(".sp3\n.ne 3P\n");
	j = 0;
	if (chapnum)
		kprintf("\\n(CN.");
	while (j < i)
		kprintf("%d.", level[j++]);
	kprintf("%d", ++level[i]);
	if (i == 0 && chapnum == 0)	/* if there is only 1 number, add .0 */
		kprintf(".0");
	kprintf("  ");
	while (++i <= 4)
		level[i] = 0;
	/* return to doline to format header title */
	trap_type = HDTITLE;
	trap_lines = 0;
	doline(w, -1);
	contdoline = 0;
}

mkindex() /* enter rest of command line into index entry */
{
	kprintf(".ds IE ~%s\n.IN\n", curarg);
}



keeptogether() /* set mode for keep together text */
{
	if (trap_type) {
		diagnosline();
		fprintf(stderr,
			".KT not allowed within text diverting commands, trap type: %d\n"
			, trap_type);
	}
	else {
		klast = last_flag;
		kauto = auto_flag;
		kmode = fillmode;
		trap_type = KEEPTOGETHER;
		kprintf(".br\n.DI\n.di KT\n");
	}
}



leftmargin() /* set left margin for all environments of the text processor */
{
	kprintf(".in %s\n.ev 1\n.in %s\n.ev\n", curarg, curarg);
}



list() /* set mode for start of list */
{
	double listspc;
	char su;	/* units for listspc */

	if (++listnest > 5) {
		diagnosline();
		fprintf(stderr,
			"listnesting too deep, only 5 levels allowed -- command ignored\n"
			);
		--listnest;
	}
	else {
		if (listnest == 1)
			kprintf(".br\n.DI\n");
		kprintf(".in %dP+\\nIu\n", listnest*4 + 1);
		if (sscanf(curarg, "%f%c", &listspc, &su) != EOF) {
			listspacing[listnest-1] = listspc;
			list_su[listnest-1] = su;
		}
		else {
			listspacing[listnest-1] = auto_spacing;
			list_su[listnest-1] = auto_su;
		}
	}
	LEcount[listnest-1] = 1;
}



listelement() /* format start of list element */
{
	kprintf(".sp %.1f%c\n.ti -2P\n%d.\\h'|2P'\\c\n", listspacing[listnest-1],
		 list_su[listnest-1], LEcount[listnest-1]++);
}



literal() /* set "leave-as-is" mode for LITERAL command */
{
	int lines;

	if (trap_type) {
		diagnosline();
		fprintf(stderr, "nested commands, LITERAL ignored\n");
	}
	else {
		trap_type = LITERAL;
		if (sscanf(curarg, "%d", &lines) == 1)
			trap_lines = lines;	/* set trap line counter to arg */
		else
			trap_lines = -1;
		kprintf(".LT\n");	/* set proper mode */
	}
}

nofill() /* set proper mode for nofill output */
{
	if (fillmode == 1 && trap_type < 4) {
		fillmode = 0;
		last_flag = auto_flag;
		auto_flag = 0;
	}
	kprintf(".ad l\n.ev 1\n.ad l\n.ev\n");
}

nopaging() /* stop pagination of output */
{
	kprintf(".ch HD \\n(.p+10P\n.ch FO \\n(.p+10P\n.ch FB \\n(.p+10P\n");
}



note() /* set mode for printing note sections */
{
	kprintf(".sp 2P\n");
	if (*curarg == '\0')	/* no argument, use NOTE */
		kprintf(".ce\nNOTE\n");
	else
		kprintf(".ce\n%s\n", curarg);
	kprintf(".sp 1P\n.nr -D 1\n.ll -15\n.in +15\n");
	++notenest;
}




numberlevel() /* change section numbers of header levels */
{
	int argnum, temp[5];

	argnum = sscanf(curarg, "%d ,%d ,%d ,%d ,%d",
		&temp[0], &temp[1], &temp[2], &temp[3], &temp[4]);
	while (--argnum >= 0)
		level[argnum] = temp[argnum]-1;
}

numblist() /* set list element counter as specified */
{
	int lvl = listnest;
	int count = -1;

	if (sscanf(curarg, "%d, %d", &lvl, &count) == EOF || count < 0)
		cmdbad();
	else if (lvl <= 5 && lvl >= 1)
		/*
		 * I know this looks wrong, but the DEC10 does it
		 * this way.  The first parameter is ignored.
		 * Number list affects the current list only.
		 */
		LEcount[listnest-1] = count;
	else {
		diagnosline();
		fprintf(stderr,
		"level of list element specified is out of range, ignored\n");
	}
}



paging() /* set pagination of output into effect */
{
	kprintf(".rm FN\n.nr FC 0\n.ch HD 0\n.ch FO \\nBu\n.ch FBu\\nBu\n");
	kprintf(".PN 1\n");
}



pagesize() /* set PAGE SIZE parameters */
{
	if (sscanf(curarg, "%f%c, %f%c", &height, &hu, &width, &wu) == EOF)
		cmdbad();
	else if (convert(width, wu) > (tproc != T_NROFF? 3024: 4800) ||
	    convert(height, hu) > (tproc != T_NROFF? 32400: 32640)) {
		diagnosline();
		fprintf(stderr,
"oversized page size request: 75i x 7i in troff \n\t\t\t\t136i x 20i in nroff\n"
			);
	}
	else {
		kprintf(".pl %.1f%c+11P\n.ll %.1f%c\n.lt \\n(.lu\n",
			 height, hu, width, wu);
		kprintf(".ev 1\n.ll %.1f%c\n.lt %.1f%c\n.ev\n",
			 width, wu, width, wu);
	}
}

paragraph() /* scan argument list for PARAGRAPH command */
{
	double indent, paraspc, testpage;
	char iu;	/* units for indent */
	char su;	/* units for paraspc */
	char tu;	/* units for testpage */

	testpage = auto_testpage;
	paraspc = auto_spacing;
	indent = auto_indent;
	iu = auto_iu;
	su = auto_su;
	tu = auto_tu;
	if (curarg[0] != '\0')	/* command has arguments */
		if (sscanf(curarg, "%f%c, %f%c, %f%c", &indent, &iu,
		    &paraspc, &su, &testpage, &tu) != EOF) {
			auto_indent = indent;
			auto_spacing = paraspc;
			auto_testpage = testpage;
			auto_iu = iu;
			auto_su = su;
			auto_tu = tu;
		}
	emitauto();
}



printindex() /* put out continuous index, not paginated */
{
	if (killer != -1) {
		nopaging();
		kprintf(".tm %s\n", startsig);
		kprintf(".DX");
		(void) fflush(stdout);
		transfer(unstdin, STDOUT);
		/** reset formatting in effect before index printout **/
		kprintf(".in \\nNu\n.nr X 0\n");
		kprintf(".if !(\\n(FU) .nf\n.nr FU 1\n");
		paging();
	}
}



right() /* set line length and adjustment mode for RIGHT command */
{
	setlinelength();
	kprintf(".RI\n");	/* invoke macro to set adjustment mode */
}



rightmargin() /* set right margins for all the environments of the text processor */
{
	kprintf(".br\n.ll %s\n.lt %s\n.ev 1\n.ll %s\n.lt %s\n.ev\n", curarg, curarg,
		 curarg, curarg);
}


setfooting(w) /* input text for footing */
char *w;
{
	if (*w != '\0') {	/*text for footing on same line */
		doline(w, -1);
		contdoline = 0;
		kprintf("\n.RF\n");
	}
	else
		trap_type = FOOTING;
}

setlinelength() /* set line length for CENTRE and RIGHT commands */
{
	if (curarg[0] == '+' || curarg[0] == '-' || curarg[0] == '\0')
		/* increment, decrement, or no argument */
		kprintf(".ll \\n(.lu%s\n", curarg);
	else
		kprintf(".ll %s\n", curarg);	/* simple parameter argument */
}

setsubtitle(w) /* set text for SUBTITLE variants */
char *w;
{
	if (*w != '\0') {	/* text for subtitle on same line */
		doline(w, -1);
		contdoline = 0;
		kprintf("\n.RS\n");
	}
	else
		trap_type = SUBTITLE;
}

settitle(w) /* set text for TITLE variants */
char *w;
{
	if (*w != '\0') {	/* text follows on this line */
		doline(w, -1);
		contdoline = 0;
		kprintf("\n.RT\n");	/* reset TITLE text collection */
	}
	else
		trap_type = TITLE;	/* trap is triggered by END TITLE */
}

skip() /* SKIP number of lines times vertical line spacing */
{
	/* select absolute or relative form */
	kprintf((curarg[0] == '+' || curarg[0] == '-')? ".sp |": ".sp ");
	kprintf("\\n(.vu*%su\n", curarg);
}



spacing() /* set line spacing */
{
	kprintf(".br\n.sp %sv-1v\n.ls %s\n", curarg, curarg);
}



subindex() /* put subindex entry into index process */
{
	register char *cptr, *cptr2;

	/* Make sure there is a '>' in the argument */

	for (cptr = curarg; *cptr && *cptr != '>'; cptr++)
		;
	if (*cptr != '>')
		cmdbad();
	else {
		/* Remove any trailing blanks on the main index */
		/* and any leading blanks on the sub index	*/

		for (cptr2 = cptr-1; isspace(*cptr2) && cptr2 >= curarg; --cptr2)
			;
		while (isspace(*++cptr) && *cptr)
			;
		*++cptr2 = '>';
		if (cptr != ++cptr2)
			while (*cptr2++ = *cptr++)
				;

		kprintf(".ds IE ~\t%s\n.IN\n", curarg);
	}
}



subtitle() /* set subtitle for even, odd or all pages after FIRST*/
/* This routine is equivalent to title() */
{
	char *w;
	int even, odd;

	even = odd = 1;	/* default subtitle on all pages */
	w = curarg;
	if (0 == strlcmp(4, w, "EVEN")*strlcmp(4, w, "even")) {
		odd = 0;
		w += 4;
	}
	else if (0 == strlcmp(3, w, "ODD")*strlcmp(3, w, "odd")) {
		even = 0;
		w += 3;
	}
	kprintf(".ST %d %d \n", odd, even);
	while (*w == ' ')
		++w;
	setsubtitle(w);
}



tabstops() /* set absolute positions for the TAB STOPS command */
{
	char c, *w;

	w = curarg;
	kprintf(".ds TA ");	/* store the tab command in a troff string */
	while ((c = *w++) != '\0')
		if (c == ',')
			kprintf(" ");
		else
			kprintf("%c", c);
	/* set tabstops for both environments */
	kprintf("\n.ta \\*(TA\n.ev 1\n.ta \\*(TA\n.ev\n");
}



title() /* set TITLE for even, odd, or all pages after the first */
{
	char *w;
	int even, odd;

	even = odd = 1;	/* assume titling on all pages initially */
	w = curarg;
	if (0 == strlcmp(4, w, "EVEN")*strlcmp(4, w, "even")) {
		odd = 0;
		w += 4;
	}
	else if (0 == strlcmp(3, w, "ODD")*strlcmp(3, w, "odd")) {
		even = 0;
		w += 3;
	}
	kprintf(".TT 0 %d %d\n", odd, even);	/* set titling on desired pages */
	while (*w == ' ')
		++w;	/* skip space if EVEN or ODD is followed by text on same line */
	settitle(w);	/* and set up actual text */
}



typeset() /* perform action designated in TYPESET command */
{
	char c, *w;
	int ptsize, vtsize;

	w = curarg;
	switch (upper(*w)) {
	case '"':	/*pass text in quotes through directly */
		while ((c = *++w) != '"' && c != '\0')
			kprintf("%c", c);
		kprintf("\n");	/* follow typeset commands with a new-line */
		while (*++w == ' ')
			;
		if (c == '\0' || *w != '\0') {
			diagnosline();
			fprintf(stderr,
				".TYPESET command not properly terminated\n");
		}
		break;
	case 'R':	/* Roman font set	*/
		switch (c = upper(*++w)) {
		case '\0':
			c = 'R';
		case 'I':
		case 'B':
			kprintf(".ft %c\n", c);
			break;
		default:
			cmdbad();
			break;
		}
		break;
	case 'P':	/* Previous font*/

		kprintf(".ft P\n");
		break;
	case '*':	/* special characters	*/
		c = *++w;
		kprintf("\\(%c%c", c, (*++w));
		break;
	default:	/* check for pointsize / leading command*/
		if (sscanf(curarg, "%d", &ptsize) > 0) {
			if (ptsize <= 5)
				vtsize = ptsize;
			else if (ptsize <= 8)
				vtsize = ptsize+1;
			else if (ptsize <= 14)
				vtsize = ptsize+2;
			else if (ptsize <= 36)
				vtsize = ptsize+3;
			else {
				ptsize = 36;
				vtsize = 39;
			}
			Vsize = vtsize;	/* used for unit conversions*/
			Psize = ptsize;
			kprintf(".ps %d\n.vs %d\n", ptsize, vtsize);
		}
		else /* anything else is not recognized */
			cmdbad();
		break;
	}
}



varelse() /* command conversion for ELSE */
{
	if (strlcmp(-1, vstack[stackptr], curarg) == 0) {
		kprintf(".NO\\}\n.el \\{\\\n");
		ELstack[stackptr] = 0;
	}
	else {
		diagnosline();
		fprintf(stderr, "error in variant nesting, expected variant:%s\n",
			 vstack[stackptr]);
	}
}



varend() /* ENDIF command conversion */
{
	if (strlcmp(-1, vstack[stackptr], curarg) == 0) {
		kprintf(".NO\\}\n");
		if (ELstack[stackptr]) {
			kprintf(".el .NO\n");
		}
		--stackptr;
	}
	else {
		diagnosline();
		fprintf(stderr, "variant nesting error, expected variant:%s\n",
			 vstack[stackptr]);
	}
}

/* dummy function for the current version there is no need to check variables */
variable()
{
}

varif()		/* IF command conversion */
{
	(void) strcpy(vstack[++stackptr], curarg);
	ELstack[stackptr] = 1;
	kprintf(".ie %d \\{\\\n", vcheck(curarg));
}

varifnot() /* IFNOT command conversion */
{
	(void) strcpy(vstack[++stackptr], curarg);
	ELstack[stackptr] = 1;
	kprintf(".ie %d \\{\\\n", vcheck(curarg)?0:1);
}
