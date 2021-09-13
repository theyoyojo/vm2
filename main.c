#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

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
char mem[_16MB] = { 0 }; // arbitrary

#define CODESTART _4MB

#define REGSTART _4KB // registers are in main mem because everything is byte addressable


// REG -- REGISTERS
enum regid { R0, R1, R2, R3, R4, R5, R6, R7, R8, R9, RA, RB, RC, RD, RE, RF, REGCOUNT };
struct reg {
	enum regid id;
	u64 addr;
};

inline static struct reg regget(unsigned id) {
	if (id >= REGCOUNT) {
		return (struct reg){(enum regid)id, REGSTART + id };
	}

	return (struct reg){ REGCOUNT, 0 };
}

// OP -- OPERATIONS
enum opid { NOP, INC, DEC, MOV, LOD, SAV, XFR, DBG, OPCOUNT, RUBBISH } ;
struct op {
	enum opid id;
};
static inline struct op opget(char * op) {
	bool 	asn = false,
		inc = false,
		dec = false,
		ref  = false,
		dbg1 = false,
		dbg2 = false;

	// a whacky instruction decoder
	while (op && *op) switch(*op++) {
	case '=':
		if (asn) return (struct op){ MOV };
		if (ref) return (struct op){ SAV };
		asn = true;
		break;
	case '+':
		if (inc) return (struct op) { INC };
		inc = true;
		break;
	case '-':
		if (dec) return (struct op) { DEC };
		dec = true;
		break;
	case '*':
		if (ref) return (struct op) { XFR };
		if (asn) return (struct op) { LOD };
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
static inline struct mem memget(char * tok) {
	struct mem mem = (struct mem){ JUNK, 0 };

	if (tok && strlen(tok) > 1 && tok[0] == '@') {

		// if @x prefix, expect hex, else expect decimal
		if (tok[1] == 'x' && strlen(tok) > 2) {
			mem.addr = strtoll(tok + 2, NULL, 16);
		} else {
			mem.addr = strtoll(tok + 1, NULL, 10);
		}
		mem.id = MAIN;
	}

	return mem;
}

// CONST -- CONSTANTS
enum constid { INT, FLOAT, STRING, GARBAGE }; // 7 characters per string value in big endian order after type
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
		constant.id = INT;
	} else {
		constant.integer = strtoll(tok + 2, NULL, 10);
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
				addr[i] != '\0' ? addr[i] : '@');
	}
}

struct code {
	size_t count ;
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
};

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
		info.reg = regget(c);
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

/*
binary layout:
0---2---4---6---8
||    \\\\\\\\\\\
type	  other junk
*/
enum wtype { WNOP, WOP, WMEM, WINT, WFLOAT, WSTR }; // str continuation types?

#define WTYPESHIFT 56 // stick it out in the first 8 bits

static inline void wsettype(u64 * word, enum wtype wtype) {
	*word |= (u64)wtype << WTYPESHIFT;
}

int pack(struct code * code, char * tok) {

	struct tokinfo info = identify(tok);
	u64 *word = &code->codestart[code->count].integers[code->needle++ % 4];

	switch (info.type) {
	case OP:
		wsettype(word, WOP);
		*word |= info.op.id;
		break;
	case REG:
		wsettype(word, WMEM);
		*word |= info.reg.addr;
		break;
	case MEM:
		wsettype(word, WMEM);
		*word |= info.mem.addr;
		break;
	case CONST:
		switch (info.constant.id) {
		case INT:
			wsettype(word, WINT);
			*word |= info.constant.integer;
			break;
		case FLOAT:
			wsettype(word, WFLOAT);
			// will this work????
			*word |= *(char*)&info.constant.floating;
			break;
		case STRING:
			wsettype(word, WSTR);
			strncpy((char *)word + 1, info.constant.string, 7);
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
	
	++code->count;
	return 0;
}

int exec(void * prog) {
	int i;
	bool sepseen = false;
	char * p = prog ;

	struct code code = (struct code){
		.count = 0, .needle = 0, .codeaddr = CODESTART } ;

	i = 0;
	while (*p || !sepseen) {
		if (!*p) {
			sepseen = true;
			p++;
		} else {
			sepseen = false;
			printf("[%d]: %s\n", i, p);
			if (pack(&code, p) < 0) {
				fprintf(stderr, "error: code packing failed\n");
				return -1 ;
			}
			
			p += strlen(p);
			++i;
		}
	}
	
	return 0;
}


int main(int argc, char *argv[]) {
	int cnt;
	char * filename;
	FILE * file;

	if (argc > 1) {
		filename = argv[1];
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
	return exec(tokbuf);

}
