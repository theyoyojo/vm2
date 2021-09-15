#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

typedef unsigned long long u64 ;

// RAM
#define _1KB 	(1UL << 10)
#define _2KB 	(1UL << 11)
#define _4KB 	(1UL << 12)
#define _8KB 	(1UL << 13)
#define _16KB 	(1UL << 14)
#define _32KB 	(1UL << 15)
#define _64KB 	(1UL << 16)
#define _128KB 	(1UL << 17)
#define _256KB 	(1UL << 18)
#define _512KB 	(1UL << 19)
#define _1MB 	(1UL << 20)
#define _2MB 	(1UL << 21)
#define _4MB 	(1UL << 22)
#define _8MB 	(1UL << 23)
#define _16MB 	(1UL << 24)
#define _32MB 	(1UL << 25)
#define SYSTEMSIZE _16MB
char mem[SYSTEMSIZE] = { 0 }; // arbitrary
struct syment {
	char * name;
	u64 data;
};
static struct code {
	size_t count, odometer, symcount;
	bool running;
	int needle;
	union {
		struct instruct {
			union {
				struct {
					u64 op;
					u64 arg1;
					u64 arg2;
					u64 arg3;
				};
				char bytes[32];
				u64 integers[4];
			};
		} *codestart;
		u64 codeaddr;
	};
} * code;


#define CODESTART _4MB

#define STACKSTART (_8MB | _4MB)

#define REGSTART _4KB // registers are in main mem because everything is byte addressable

int _log(char * fmt, ...) {
	int ret ;
	char pfx[] = "[DEBUG] ";
	va_list list;
	static char buf[128];

	strncpy(buf, pfx, strlen(pfx));
	va_start(list, fmt);

	strncpy(buf + strlen(pfx), fmt, 128 - strlen(pfx));
	ret = vfprintf(stdout, buf, list);

	va_end(list);
	return ret;
}

// REG -- REGISTERS
enum regid {
	R0,
	R1,
	R2,
	R3,
	R4,
	R5,
	R6,
	R7,
	R8,
	R9,
	RA,
	RB,
	RC,
	RD,
	RE,
	RF,
	REGCOUNT,
	GARBREG
};
// R0 will probably be used as accumulator
// RF is the instruction pointer
// RE is the stack pointer. As the stack grows, it becomes REEEEEEEEEE

struct reg {
	enum regid id;
	u64 addr;
};

static struct reg regget(unsigned id) {
	if (id < REGCOUNT) {
		return (struct reg){(enum regid)id, REGSTART + id * sizeof(u64)};
	}

	return (struct reg){ GARBREG, 0 };
}

/* OP -- OPERATIONS
 * 	NOP		No-op: do nothing
 *	INC	++	Increment: increment arg1
 * 	DEC	--	Decrement arg1
 *	MOV	==	Move data at arg2 into location in arg1
 * 	LOD	=*	Move data from location in arg2 into location in arg1
 *	SAV	*=	Move data from arg2 into location in location in arg1
 *	XFR	**	Move data from location in arg2 into location in location in arg1
 *	AND	&=	Store the result of a binary and of the data at the location in arg1
 *				and the data in the location of arg2 in the location in arg1
 *	JMP	!!	Set the next instruction pointer to the data on top of the stack
 *	JDR	!*	Set the instruction pointer to the data in arg1
 *	JZR	!0	If R0 is zero then set the instruction pointer to the data on top of the stack
 *	JPS	!+	If R0 is positive then set the instruction pointer to the data on top of the stack
 *	JNG	!-	If R0 is negative then set the instruction pointer to the data on top of the stack
 *	PSH	^^	Push the data at the location in arg1 onto the stack
 *	POP 	VV	Pop the data on top of the stack into the location in arg1
 *	DBG	xxx	Halt and run debug stuff
*/
enum opid {
	NOP,	
	INC,	
	DEC,	
	MOV,	
	LOD,
	SAV,
	XFR,
	AND,
	JMP,
	JDR,
	JZR,
	JPS,
	JNG,
	PSH,
	POP,
	DBG,
	OPCOUNT,
	RUBBISH
};
struct op {
	enum opid id;
};
static inline struct op opget(char * op) {
	bool 	asn = false,
		inc = false,
		dec = false,
		ref  = false,
		jmp = false,
		and = false,
		psh = false,
		pop = false,
		dbg1 = false,
		dbg2 = false;

	// a whacky instruction decoder
	while (op && *op) switch(*op++) {
	case '&':
		if (jmp) return (struct op) { JDR };
		and = true;
		break;
	case '0':
		if (jmp) return (struct op) { JZR };
		break;
	case '!':
		if (jmp) return (struct op) { JMP };
		jmp = true;
		break;
	case '=':
		if (asn) return (struct op) { MOV };
		if (ref) return (struct op) { SAV };
		if (and) return (struct op) { AND };
		asn = true;
		break;
	case '+':
		if (inc) return (struct op) { INC };
		if (jmp) return (struct op) { JPS };
		inc = true;
		break;
	case '-':
		if (dec) return (struct op) { DEC };
		if (jmp) return (struct op) { JNG };
		dec = true;
		break;
	case '*':
		if (ref) return (struct op) { XFR };
		if (asn) return (struct op) { LOD };
		if (jmp) return (struct op) { JDR };
		ref = true;
		break;
	case '^':
		if (psh) return (struct op) { PSH };
		psh = true;
		break;
	case 'V':
		if (pop) return (struct op) { POP };
		pop = true;
		break;
	case 'x':
		if (dbg1) if (dbg2) return (struct op) { DBG } ;
		else dbg2 = true ; else dbg1 = true ;
	default:
		break;
	}
	
	return (struct op) { RUBBISH };

}

// MEM -- MEMORY
enum memid { MAIN, JUNK }; // maybe other address spaces
struct mem {
	enum memid id;
	u64 addr;
};

// memory references are identified by @ prefix
static struct mem memget(char * tok) {
	struct mem mem = (struct mem){ JUNK, 0 };

	if (tok && strlen(tok) > 1 && tok[0] == '@') {

		// if @d prefix, expect decimal, else expect hex
		if (tok[1] == 'd' && strlen(tok) > 2) {
			mem.addr = strtoll(tok + 2, NULL, 10);
		} else {
			mem.addr = strtoll(tok + 1, NULL, 16);
		}
		mem.id = MAIN;
	}

	return mem;
}

void * memtohost(u64 emuaddr) {
	return (void *)(mem + emuaddr);
}

u64 hosttomem(void * hostptr) {
	return (u64)hostptr - (u64)mem;
}

char * memname(u64 location) {
	static char * regnames[] = {
		[R0] = "[R0]",
		[R1] = "[R1]",
		[R2] = "[R2]",
		[R3] = "[R3]",
		[R4] = "[R4]",
		[R5] = "[R5]",
		[R6] = "[R6]",
		[R7] = "[R7]",
		[R8] = "[R8]",
		[R9] = "[R9]",
		[RA] = "[RA]",
		[RB] = "[RB]",
		[RC] = "[RC]",
		[RD] = "[RD]",
		[RE] = "[RE]",
		[RF] = "[RF]",
	};
	u64 regindex = (location - REGSTART) / sizeof(u64);
	if (regindex < REGCOUNT) {
		return regnames[regindex];
	}

	// we could have other names in the future

	return NULL;
}

// these functions directly dereference their ptrs to cells
/*
binary layout:
0---2---4---6---8
||    \\\\\\\\\\\
type	  other junk
*/
enum wtype { WNOP, WOP, WMEM, WINT, WHEX, WFLOAT, WSTR }; // str continuation types?

#define WTYPESHIFT 56 // stick it out in the first 8 bits
#define WTYPEMASK 0xff << WTYPESHIFT

static inline void wsetdata(u64 * word, u64 data) {
	*word |= data;
}

static inline void wsettype(u64 * word, enum wtype wtype) {
	*word |= (u64)wtype << WTYPESHIFT;
}

static inline enum wtype wgettype(u64 * word) {
	return *word >> WTYPESHIFT;
}

static inline u64 wgetdata(u64 * word) {
	return *word & ~(0xffUL << WTYPESHIFT);
}

// for later: typedef u64 emuaddr or something
// yeah just cast to another type if you would prefer that instead
u64 memread(u64 addr) {
	if (code->running) {
		printf(">> READ @%08llx\n", addr);
	}
	return wgetdata((u64 *)(mem + addr));
}

static inline enum wtype memtype(u64 addr) {
	return wgettype((u64 *)(mem + addr));
}

void memwrite(u64 addr, u64 data, enum wtype wtype) {
	if (code->running) {
		printf(">> WRITE  @%08llx = %lld\n", addr, data);
	}
	*(u64 *)(mem + addr) = data;
	*(u64 *)(mem + addr) |= ((long)wtype << WTYPESHIFT);
	wsettype((u64 *)(mem + addr), wtype);
	wsetdata((u64 *)(mem + addr), data);
}

void regwrite(enum regid id, u64 data, enum wtype wtype) {
	*((u64 *)(mem + REGSTART) + id) = data;
	*((u64 *)(mem + REGSTART) + id) |= ((long)wtype << WTYPESHIFT);
}

u64 regread(enum regid id) {
	return *((u64 *)(mem + REGSTART) + id) & ~(0xffUL << WTYPESHIFT);
}


// CONST -- CONSTANTS
enum constid { INT, HEX, FLOAT, STRING, GARBAGE }; // 7 characters per string value in big endian order after type
struct constant {
	enum constid id;
	union {
		u64 integer;
		float floating;
		char string[8];
	};
};
static inline struct constant constget(char * tok) {
	struct constant constant;
	char * c;	
	bool maybefloat = false, iszero = !strncmp("0.0", tok, 3);

	// only super simple strings since whitespace always tokenizes
	if (tok[0] == '"' && tok[strlen(tok)-1] == '"') {
		constant.id = STRING;
		strncpy(constant.string, tok + 1, strlen(tok) - 2);
		return constant;
	}

	// if there is a '.' in it try and make it float
	// bail if non-hex non-'.' digits appear
	for (c = tok; *c; ++c) {
		if (*c == '.') maybefloat = true;
		else if (!isxdigit(*c)) {
			constant.id = GARBAGE;
		}
	}
	if (maybefloat) {
		constant.floating = strtof(tok, NULL);
		// if strtof returns zero but string was not "0.0" conversion failed
		if (constant.floating != 0 || iszero) constant.id = FLOAT;
	}

	// try hex and int
	if (strlen(tok) > 2 && tok[0] == '0' && tok[1] == 'x') {
		constant.integer = strtoll(tok + 2, NULL, 16);
		constant.id = HEX;
	} else {
		constant.integer = strtoll(tok, NULL, 10);
		constant.id = INT;
	}

	return constant;
}

char tokbuf[_4MB] = { 0 }; // arbitrary

int tokenize(FILE * file, char * buf, size_t len) {
	char c;
	bool ignorespaces = true, comment = false;
	size_t i = 0, cnt = 0;

	while ((c = fgetc(file)) != EOF && i < len - 1) {
		// everything between ';'s are comments
		if (c == ';' || comment) {
			comment = c != ';' || !comment;
			continue ;
		}
		// tokenize based only on whitespace
		else if (isspace(c)) {
			if (ignorespaces) {
				continue;
			} else {
				ignorespaces = true;
				buf[i] = '\0';
				cnt++;
			} 
		// only allow alphanumeric and punctuation (and yes a. => "a." not "a", ".")
		} else if (isalnum(c) || ispunct(c)) {
			ignorespaces = false;
			buf[i] = c;
		} else {
			fprintf(stderr, "error: unknown character in input stream\n");
			return -1;
		}
		++i;
	}

	// can't end stream in comment
	if (comment) {
		fprintf(stderr, "error: stream ends in comment\n");
		return -1;
	}

	// end tokbuf with double zero byte
	buf[i] = '\0';
	return cnt;
}

void memdmp(char * addr, size_t cnt) {
	size_t i;
	for (i = 0; i < cnt; ++i) {
		printf("0x%lx: %c\n", (unsigned long)(addr + i),
				addr[i] != '\0' ? addr[i] : '*');
	}
}

void memdmpd(char * addr, size_t cnt) {
	size_t i;
	for (i = 0; i < cnt; ++i) {
		printf("0x%lx: %d\n", (unsigned long)(addr + i),
				addr[i] != '\0' ? addr[i] : -1);
	}
}

void memdmp_(u64  addr, size_t cnt) {
	size_t i;
	for (i = 0; i < cnt; ++i) {
		printf("0x%lx: %d\n", (unsigned long)(addr + i),
				(mem + addr)[i] != '\0' ? (mem + addr)[i] : -1);
	}
}

enum toktype { OP, REG, MEM, CONST, NONSENSE };

struct tokinfo {
	enum toktype type;
	union {
		struct op op;
		struct reg reg;
		struct mem mem;
		struct constant constant;
	};
};

struct tokinfo identify(char * tok) {
	size_t len = strlen(tok);
	char c;
	struct tokinfo info;

	if (len < 2) {
		info.type = NONSENSE;
	}
	// registers
	u64 foo = REGSTART; (void)foo;
	if (tok[0] == 'R' || toupper(tok[0]) == 'R') {
		if (!isxdigit(c = tok[1])) {
			info.type = NONSENSE;
			return info ;
		}
		if (isalpha(c) && islower(c)) {
			// accept Ra == RA == ra == rA
			c = toupper(c) ;
		}
		info.type = REG;
		if ((info.reg = regget(c - '0')).id == GARBREG) { // convert ascii numbers to binary
			fprintf(stderr, "error: '%s' is a garbage register\n", tok);
			info.type = NONSENSE;
		}
	// operations
	} else if ((info.op = opget(tok)).id != RUBBISH) {
		info.type = OP;
	// memory reference
	} else if ((info.mem = memget(tok)).id != JUNK ) {
		info.type = MEM;
	// constant
	} else if  ((info.constant = constget(tok)).id != GARBAGE) {
		info.type = CONST;
	} else {
		info.type = NONSENSE;
	}

	return info;
}

char * opstr(enum opid opid) {
	static char * opstrs[] = {
		[NOP] = "No-op",	/* (void)0; 	*/
		[INC] = "Increment ",	/* ++A 		*/
		[DEC] = "Decrement",	/* --B		*/
		[MOV] = "Move",		/* A = B	*/
		[LOD] = "Load",		/* A = *B	*/
		[SAV] = "Save",		/* *A = B	*/
		[XFR] = "Transfer",	/* *A = *B	*/
		[AND] = "Binary and",	/* A &= B	*/
		[JMP] = "Jump uncond.",	/* goto *stack  */
		[JDR] = "Jump direct",	/* goto arg1 	*/
		[JZR] = "Jump zero",	/* R0 == 0 	=> goto *stack */
		[JPS] = "Jump pos.",	/* R0 > 0 	=> goto *stack */
		[JNG] = "Jump neg.",	/* R0 < 0 	=> goto *stack */
		[PSH] = "Push",		/* *--stack = arg1 */
		[POP] = "Pop",		/* arg1 = *stack++ */
		[DBG] = "Debug",	/* (debug)	*/
	};

	return opstrs[opid];
}


int printword(u64 addr, char buf[], size_t bufsz) {
	u64 data = memread(addr);
	
	char * tmp;
	size_t len = 0, i;
	switch (memtype(addr)) {
	case WOP:
		tmp = opstr((enum opid)data);
		len = strlen(tmp);
		strncpy(buf, tmp, len);
#define COLWTH 12 // yeah now it's not a magic number wonderful
#define COLWTHSTR "12"
#define COLWTHSTRMINUSONE "11"
		for (i = 0; COLWTH - len - i > 0; ++i) {
			strcpy(buf + len + i, " ");
		}
		len += i ;
		break;
	case WMEM:
		if ((tmp = memname(data))) {
			len = strlen(tmp);
			strncpy(buf, tmp, len);
			for (i = 0; COLWTH - len - i > 0; ++i) {
				strcpy(buf + len + i, " ");
			}
			len += i ;
		} else {
			len = sprintf(buf, "@%-" COLWTHSTRMINUSONE "llx", data);
		}
		break;
	case WINT:
		len = sprintf(buf, "%-" COLWTHSTR "llu", data);
		break;
	case WHEX:
		len = sprintf(buf, "@%-" COLWTHSTRMINUSONE "llx", data);
		break;
	case WFLOAT:
		// maybe????
		len = sprintf(buf, "%" COLWTHSTR "g", (float)data);
		break;
	case WSTR:
		strncpy(buf, ((char *)&data + 1), 7); // 8 per u64 minus metadata
		len = 7;
		for (i = -1; COLWTH - len - i > 0; ++i) {
			strcpy(buf + len + i, " ");
		}
		len += i ;
		break;
	case WNOP:
	default:
		tmp = "  ( junk )  ";
		len = strlen(tmp);
		strncpy(buf, tmp, len);
	}

	if (len > bufsz) {
		fprintf(stderr, "error: you wrote more bytes to the buffer than available!\n");
		return -1;
	}

	if (len != 12) { // should be tautological
		fprintf(stderr, "Aiee! Length is not 12 data@word='%llx@$%llx'!\n", data, addr);
		// kannot kontinue
		exit(1);
		
	}

	return len;
}

static inline void tracksprintf(char * buf, size_t * printed, size_t * bufsz, char * fmt, ...) {
	va_list list;
	size_t len;

	va_start(list,fmt);

	len = vsprintf(buf + *printed, fmt, list);
	*printed += len;
	*bufsz -= len;

	va_end(list);
}

static inline void printjagededge(char * buf, size_t * printed, size_t * bufsz) {
	int i;
	char c;
#define NICE_WIDTH 80
	for (i = 0; i < NICE_WIDTH; ++i) {
		if (i % 2) {
			c = '\\';
		} else {
			c = '/';
		}
		tracksprintf(buf, printed, bufsz, "%c", c);
	}
	tracksprintf(buf, printed, bufsz, "\n");
}

int printline(u64 addr, char buf[], size_t * bufsz, size_t * printed) {
	size_t * _printed, len, dummy = 0, i,
		wordsperline = sizeof(struct instruct)/sizeof(u64);
	if (printed) {
		_printed = printed;
	} else  {
		_printed = &dummy;
	}

	tracksprintf(buf, _printed, bufsz, ">>>>> @%08llx: ", addr);
	for (i = 0; i < wordsperline; ++i) {
		len = printword(addr + i * sizeof(u64), buf + *_printed, *bufsz - *_printed);
		if (len < 0) {
			return -1;
		}
		*_printed += len;
		*bufsz += len;
		if (i < wordsperline - 1) {
			tracksprintf(buf, _printed, bufsz, " | ");
		} else {
			tracksprintf(buf, _printed, bufsz, " <<<<<\n");
		}
	}

	return *_printed;
}

int printlines(u64 addr, size_t count, char buf[], size_t bufsz) {
	size_t i, printed = 0;

	tracksprintf(buf, &printed, &bufsz, "[Display Binary Tape for @%08llx + %04lu lines]\n| %-12s | %-12s | %-12s | %-12s | %-16s |\n",
			addr, count, "Emu. Address", "Operation",
			"Argument one", "Argument two", "Argument three" );

// line: 5> + 4" @: " + 8 + 9" | " + 48%x + 6" <<<<<" == 80 characters coincidentally
	printjagededge(buf, &printed, &bufsz);
	for (i = 0; i < count; ++i) {
		printline(addr + i * sizeof(struct instruct), buf, &bufsz, &printed);
	}
	printjagededge(buf, &printed, &bufsz);
	
	return printed;
}

static inline char * regstr(void) {
	static char buf[_1KB] = { 0 }; 
	printlines(REGSTART, 4, buf, _1KB);
	return buf;
}

static inline char * codestr(size_t n) {
	static char buf[_1KB] = { 0 }; 
	printlines(CODESTART, n, buf, _1KB);
	return buf;
}

static  char * stackstr(void) {
	static char buf[_4KB] = { 0 }; 
	u64 rounddown = regread(RE) - (regread(RE) % sizeof(struct instruct));
	printlines(rounddown , (STACKSTART - rounddown)/sizeof(struct instruct), buf, _1KB);
	return buf;
}

static char * unalignedstr = "!unaligned                                                           unaligned!"; 
static inline char * linestr(u64 line) {
	static char buf[128] = { 0 };
	if (line % 8) {
		return unalignedstr;
	}

	size_t sz = 128;
	printline(line, buf, &sz, NULL);
	return buf;

}

int pack(struct code * code, char * tok) {

	struct tokinfo info = identify(tok);
	// end of 4 word 'line' or new op means return to begining of next line,
	// but we must do the the return for OP now and for end-of-line after for counting
	if (code->needle > 0 && info.type == OP) {
		++code->count;
		code->needle = 0 ;
	}

	u64 word = code->codeaddr + (sizeof(struct instruct) * code->count) + (sizeof(u64) * code->needle++);

	switch (info.type) {
	case OP:
		memwrite(word, info.op.id, WOP);
		break;
	case REG:
		memwrite(word, info.reg.addr, WMEM);
		break;
	case MEM:
		memwrite(word, info.mem.addr, WMEM);
		break;
	case CONST:
		switch (info.constant.id) {
		case INT:
			memwrite(word, info.constant.integer, WINT);
			break;
		case HEX:
			memwrite(word, info.constant.integer, WHEX);
			break;
		case FLOAT:
			// will this work????
			memwrite(word, *(char*)&info.constant.floating, WFLOAT);
			break;
		case STRING:
			memwrite(word, *(u64*)info.constant.string, WSTR);
			break;
		case GARBAGE:
		default:
			break;
		}
		break;
	case NONSENSE:
	default:
		fprintf(stderr, "error: what is '%s'?\n", tok);
		return -1 ;
	}
	
	if (code->needle > 3) {
		++code->count;
		code->needle = 0 ;
	}
	
	return 0;
}


int process(struct code * code, void * prog) {
	int i;
	bool sepseen = false;
	char * p = prog ;

	i = 0;
	while (*p || !sepseen) {
		if (!*p) {
			sepseen = true;
			p++;
		} else {
			sepseen = false;
			printf("[%d]: %s\n", i, p);
			if (pack(code, p) < 0) {
				fprintf(stderr, "error: code packing failed\n");
				return -1 ;
			}
			
			p += strlen(p);
			++i;
		/* memdmp_(code.codeaddr, 32); */
		}
	}

	// account for final instruction:
	++code->count;

	// Set RD = code->count
	regwrite(RD, code->count, WINT);
	// Set RE = 0x
	regwrite(RE, STACKSTART, WHEX);
	// Set RF = IP = 0x0040000 = CODESTART
	regwrite(RF, CODESTART, WHEX);
	
	return 0;
}

int exec(struct code * code) {
	int ret = 0;
	u64 ip;
	size_t wsz = sizeof(u64);
	enum opid op;

	ip = regread(RF);
	code->odometer = 0;
	code->running = true;
	if (ip % 1 << 8) {
		fprintf(stderr, "error: alignment check failed for @%llx\n", ip);
		ret = -1;
		goto out;
	}
	// Let OP A B C = OP arg1 arg3 arg3
	code->running = true;
	while (memtype(ip) == WOP && code->odometer++ < code->count) {
		switch (op = (enum opid)memread(ip)) {
		case MOV:
			// A = B
			memwrite(memread(ip + wsz), memread(ip + 2 * wsz), memtype(ip + 2 * wsz));
			break;
		case AND:
			// A &= B
			memwrite(memread(ip + wsz), memread(memread(ip + wsz)) & memread(memread(ip + 2 * wsz)), memtype(memread(ip + wsz)));
			break;
		case LOD:
			// A = *B
			memwrite(memread(ip + wsz), memread(memread(ip + 2 * wsz)),
					memtype(memread(ip + 2 * wsz)));
		case SAV:
			// *A = B
			memwrite(memread(memread(ip + wsz)),
					memread(ip + 2 * wsz), memtype(ip + 2 * wsz));
		case XFR:
			// *A = *B
			memwrite(memread(memread(ip + wsz)), memread(memread(ip + 2 * wsz)),
					memtype(memread(ip + 2 * wsz)));
		case INC:
			// A++
			memwrite(memread(ip + wsz), memread(memread(ip + wsz)) + 1, WINT);
			break;
		case DEC:
			// A--
			memwrite(memread(ip + wsz), memread(memread(ip +  wsz)) - 1, WINT);
			break;
		case JMP:
			// RF = *RE - size(instruction)
			memwrite(regget(RF).addr, memread(regget(RE).addr) - sizeof(struct instruct), WMEM);
			break;
		case JDR:
			// RF = A - size(instruction)
			memwrite(regget(RF).addr, memread(memread(ip + wsz)) - sizeof(struct instruct), WMEM);
			break;
		case JPS:
			// If R0 > 0 then RF = *RE - size(instruction)
			if (regread(R0) > 0) {
				regwrite(RF, regread(RE) - sizeof(struct instruct), WMEM);
			}
			break;
		case JNG:
			// If R0 < 0 then RF = *RE - size(instruction)
			if (regread(R0) < 0) {
				regwrite(RF, regread(RE) - sizeof(struct instruct), WMEM);
			}
			break;
		case JZR:
			// If R0 == 0 then RF = *RE - size(instruction)
			if (regread(R0) == 0) {
				regwrite(RF, regread(RE) - sizeof(struct instruct), WMEM);
			}
			break;
		case PSH:
			// stack -= size(word) then *stack = arg1
			regwrite(RE, regread(RE) - sizeof(u64), WMEM);
			memwrite(regread(RE), memread(memread(ip + wsz)), memtype(memread(ip + wsz)));
			break;
		case POP:
			// arg1 = *stack then stack += size(word)
			memwrite(memread(ip + wsz), memread(regread(RE)), memtype(regread(RE)));
			regwrite(RE, regread(RE) + sizeof(u64), WMEM);
			break;
		case DBG:
			/* ~ debug break ~ */
			goto debug_break;
		case NOP:
		default:
			break ;
		}

		printf("[%lu] EXECUTE\n%s%s\n", code->odometer - 1, linestr(ip), regstr());
		ip += sizeof(struct instruct);
	}
out:
	code->running = false;

	if (memtype(ip) != WOP) {
		fprintf(stderr, "error: no opcode found at @%llx\n", ip);
		ret = -1;
	}

	printf("MACHINE HALT\n");
	printf("REGISTERS:\n%s\n", regstr());
	printf("CODE:\n%s\n", codestr(code->count));
	printf("STACK:\n%s\n", stackstr());

	return ret;

debug_break:
	printf("... debug stub ...\n");

	/* debug instruction stuff goes here in this cave */

	goto out;
}

void usage(char * argv[]) {
	printf("usage: %s <program text filename> [ -b <binary input filename> | -o <binary output filename> ]\n", argv[0]);
}

int main(int argc, char *argv[]) {
	int cnt, ret;
	char * filename, opt,
	     *bininfilename = NULL, 
	     *binoutfilename = "bin.raw";
	/* char buf[_4KB]= { 0 }; */
	FILE * file, * bininfile = NULL;

	code = malloc(sizeof(struct code));
	if (!code) {
		fprintf(stderr, "oom!\n");
		return 1;
	}

	*code = (struct code){
		.count = 0,
		.odometer = 0,
		.needle = 0,
		.running = false,
		.codeaddr = CODESTART,
	};


	while ((opt = getopt(argc, argv, "b:o:h")) != -1) switch(opt) {
		case 'b':
			bininfilename = optarg;
			if (!(bininfile = fopen(bininfilename, "rb"))) {
				fprintf(stderr, "error: unable to open '%s' for binary reading\n", optarg);
			}
			_log("Reading binary input from file '%s'\n", bininfilename);
			break;
		case 'o':
			binoutfilename = optarg;
			break;
		case '?':
			if (optopt == 'b' || optopt == 'o') {
				fprintf(stderr, "error: argument neeeded for '-%c' flag\n", optopt);
			} else if (isprint(optopt)) {
				fprintf(stderr, "error: I've never seen'-%c' in my life\n", optopt);
			} else {
				fprintf(stderr, "error: what the heck is '-%x'?\n", optopt);
			}
		case 'h':
		default:
			usage(argv);
			return 0;
	}


	if (bininfile) {
		printf("LOAD 16MB IMAGE '%s'\n", bininfilename);
		ret = fread(mem, sizeof(char), SYSTEMSIZE, bininfile);
		code->count = regread(RD);
		if (ret < 0) {
			fprintf(stderr, "error: failed to read 16MB from file\n");
			return 1;
		}
		// skip the parsing and...
		goto cool_stuff;
	}
	else if (argc >= optind) {
		filename = argv[optind];
		if (!filename) {
			usage(argv) ;
			return 1;
		}
		if (!(file = fopen(filename, "r"))) {
			fprintf(stderr, "error: failed to open file '%s'\n", filename);
			return 1;
		}
	} else {
		file = stdin;
	}


	printf("hello welkom 2 my  vm\n");

	cnt = tokenize(file, tokbuf, _4MB);

	if (cnt < 0) {
		// error
		return 1;
	}

	fclose(file);

	process(code, tokbuf);

	/* printlines(CODESTART, 3, buf, _4KB); */
	/* printf("%s", buf); */

	FILE * tmp = fopen(binoutfilename, "wb");
	if (tmp) {
		fwrite(mem, sizeof(char), SYSTEMSIZE, tmp);
		fclose(tmp);
	} else {
		fprintf(stderr, "error, unable to open tmp.raw for binary writing");
	}

cool_stuff:

	printf("EXEC SYSTEM\n\n") ;
	printf("REGISTERS:\n%s\n", regstr());
	printf("CODE:\n%s\n", codestr(code->count));

	ret = exec(code);

	free(code);
	return ret;

}
