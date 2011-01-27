#include        "u.h"
#include        "../port/lib.h"
#include        "mem.h"
#include        "dat.h"
#include        "fns.h"
#include        "../port/error.h"

#define		MAXSIZE 1024

/* bf related */
void		bf_do_cmd(int size, char* cmd);

char		tape[MAXSIZE];
int		tape_pos = 0;
int		tape_inited = 0;

char		buffer[MAXSIZE];
int		buffer_pos = 0;
int		buffer_inited = 0;

void		init_tape(void);
int		get_tape_pos(void);
int		inc_tape_pos(void);
int		dec_tape_pos(void);

void		init_buffer(void);
void		push_buffer(char c);


/* dev */
static 	ulong	bftime;

enum{
	Qdir,
	Qcmd,
	Qdata,
};

static Dirtab bfdir[] = {
	".", 	{Qdir, 0, QTDIR},	0,	DMDIR|055,
	"bfcmd",{Qcmd},			0,	0777,
	"bfdata",{Qdata},		0,	0777,
};

static void 
bfreset(void)
{
	init_tape();
	init_buffer();
}

static void 
bfinit(void)
{
	init_tape();
	init_buffer();
}

static Chan*
bfattach(char *spec)
{
	return devattach('b', spec);
}

static Walkqid*
bfwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, bfdir, nelem(bfdir), devgen);
}

static int
bfstat(Chan *c, uchar *dp, int n)
{
	return devstat(c, dp, n, bfdir, nelem(bfdir), devgen);
}

static Chan*
bfopen(Chan *c, int omode)
{
	if( !iseve() ){
		error( Eperm );
	}
	return devopen( c, omode, bfdir, nelem(bfdir), devgen );
}

static void
bfclose(Chan *c)
{
	if( c->aux ){
		free( c->aux );
		c->aux = nil;
	}

	return;
}

static long
bfread(Chan *c, void *va, long n, vlong off)
{
	switch( (ulong)c->qid.path ){
		case Qdir:
			return devdirread( c, va, n, bfdir, nelem(bfdir), devgen );
			break;
		case Qcmd:
			n = readstr(off, va, n, tape);
			break;
		case Qdata:
			n = readstr(off, va, n, buffer);
			init_buffer();
			init_tape();
			break;
		default:
			//syslog( 1, "bf", "default");
			break;
	}

	return n;
}

static long
bfwrite(Chan* c, void* va, long n, vlong)
{
	switch( (ulong)c->qid.path ){
		case Qdir:
			error( Eisdir );
			break;
		case Qcmd:
			bf_do_cmd( n, va );
			return n;
			break;
		case Qdata:
			// not allowed to write
			return 0;
			break;
	}

	return n;
}

Dev brainfuckdevtab = {
	'b',
	"brainfuck",

	bfreset,
	devinit,
	devshutdown,
	bfattach,
	bfwalk,
	bfstat,
	bfopen,
	devcreate,
	bfclose,
	bfread,
	devbread,
	bfwrite,
	devbwrite,
	devremove,
	devwstat,
};

void 	bf_do_cmd(int size, char* cmd)
{
	int	stack = 0;
	int	pc = 0;

	if( 0 == tape_inited )
		init_tape();

	if( 0 == cmd )
		return;

	while( 1 ){
		if( pc >= size )
			break;

		switch( (char)cmd[pc] ){
			case '+':
				tape[get_tape_pos()]++;
				break;
			case '-':
				tape[get_tape_pos()]--;
				break;
			case '.':
				// save in buffer
				push_buffer( tape[get_tape_pos()] );
				break;
			case ',':
				tape[get_tape_pos()] = cmd[pc];
				break;
			case '>':
				inc_tape_pos();
				break;
			case '<':
				dec_tape_pos();
				break;
			case '[':
				if( 0 == tape[get_tape_pos()] ){
					pc++;
					while( 1 ){
						if( pc >= size ) break;
						if( stack <= 0 && cmd[pc] == ']' ) break;
						if( cmd[pc] == '[' ) stack++;
						if( cmd[pc] == ']' ) stack--;
						pc++;
					}
				}
				break;
			case ']':
				pc--;
				while( 1 ){
					if( pc < 0 ) break;
					if( stack <= 0 && cmd[pc] == '[' ) break;
					if( cmd[pc] == ']' ) stack++;
					if( cmd[pc] == '[' ) stack--;
					pc--;
				}
				pc--;
				break;
			case '*':
				// invoke syscall
				break;
			default:
				break;
		}
		pc++;
	}

	return;
}

void	init_tape(void)
{
	int i;

	for( i = 0; i < MAXSIZE; i++){
		tape[i] = 0;
	}

	tape_pos = 0;
}

int	get_tape_pos(void)
{
	return tape_pos;
}

int	inc_tape_pos(void)
{
	tape_pos++;

	if( tape_pos >= MAXSIZE )
		tape_pos = 0;

	return tape_pos;
}

int	dec_tape_pos(void)
{
	tape_pos--;

	if( tape_pos < 0 )
		tape_pos = MAXSIZE-1;

	return tape_pos;
}

void	init_buffer(void)
{
	int i;

	for( i = 0; i < MAXSIZE; i++ )
		buffer[i] = 0;
	buffer_pos = 0;

	return;
}

void	push_buffer(char c)
{
	if( 0 == buffer_inited ){
		init_buffer();
		buffer_inited = 1;
	}

	if( buffer_pos >= MAXSIZE )
		return;

	buffer[buffer_pos++] = c;

	return;
}

