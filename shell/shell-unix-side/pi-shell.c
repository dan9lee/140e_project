// engler: trivial shell for our pi system.  it's a good strand of yarn
// to pull to motivate the subsequent pieces we do.
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include "pi-shell.h"
#include "demand.h"
#include "support.h"

#include "tty.h"
// have pi send this back when it reboots (otherwise my-install exits).
static const char pi_done[] = "PI REBOOT!!!\n";
// pi sends this after a program executes to indicate it finished.
static const char cmd_done[] = "CMD-DONE\n";
enum {
	ARMBASE=0x8000, // where program gets linked.  we could send this.
        SOH = 0x12345678,   // Start Of Header

        BAD_CKSUM = 0x1,
        BAD_START,
        BAD_END,
	TOO_BIG,
	ACK,   // client ACK
        NAK,   // Some kind of error, restart
        EOT,   // end of transmission
};
unsigned get_uint(int fd);
void put_uint(int fd, unsigned u);
/************************************************************************
 * provided support code.
 */
static void write_exact(int fd, const void *buf, int nbytes) {
        int n;
        if((n = write(fd, buf, nbytes)) < 0) {
		panic("i/o error writing to pi = <%s>.  Is pi connected?\n",
					strerror(errno));
	}
        demand(n == nbytes, something is wrong);
}

// write characters to the pi.
static void pi_put(int fd, const char *buf) {
	int n = strlen(buf);
	demand(n, sending 0 byte string);
	write_exact(fd, buf, n);
}

// read characters from the pi until we see a newline.
int pi_readline(int fd, char *buf, unsigned sz) {
	for(int i = 0; i < sz; i++) {
		int n;
                if((n = read(fd, &buf[i], 1)) != 1) {
                	note("got %s res=%d, expected 1 byte\n", strerror(n),n);
                	note("assuming: pi connection closed.  cleaning up\n");
                        exit(0);
                }
		if(buf[i] == '\n') {
			buf[i] = 0;
			return 1;
		}
	}
	panic("too big!\n");
}


#define expect_val(fd, v) (expect_val)(fd, v, #v)
static void (expect_val)(int fd, unsigned v, const char *s) {
	unsigned got = get_uint(fd);
	if(v != got)
                panic("expected %s (%x), got: %x\n", s,v,got);
}

// print out argv contents.
static void print_args(const char *msg, char *argv[], int nargs) {
	note("%s: prog=<%s> ", msg, argv[0]);
	for(int i = 1; i < nargs; i++)
		printf("<%s> ", argv[i]);
	printf("\n");
}

// anything with a ".bin" suffix is a pi program.
static int is_pi_prog(char *prog) {
	int n = strlen(prog);

	// must be .bin + at least one character.
	if(n < 5)
		return 0;
	return strcmp(prog+n-4, ".bin") == 0;
}


/***********************************************************************
 * implement the rest.
 */

// catch control c: set done=1 when happens.
static sig_atomic_t done = 0;
void sigint_handler(int sig_no){
	done=1;

	note("\nCTRL-C pressed\n");
}

static void catch_control_c(void) {
	struct sigaction act;
	act.sa_handler = &sigint_handler;
	sigaction(SIGINT, &act, NULL);
}


// fork/exec/wait: use code from homework.
static int do_unix_cmd(char *argv[], int nargs) {
  pid_t pid = fork();
	if(pid == 0) {
		execvp(argv[0], argv);
		note("Program not found\n");
		exit(errno);
	}
	int status;
	waitpid(pid, &status, 0);
	if(!WIFEXITED(status))
		note("child exited with = %d\n", WEXITSTATUS(status));
	return 0;
}


static void send_prog(int fd, const char *name) {
	int nbytes;

	// from homework.
  unsigned *code = (void*)read_file(&nbytes, name);
	assert(nbytes%4==0);

	put_uint(fd, 0x12345678);

	put_uint(fd, nbytes);
	// expect_val(fd, ACK);
	// unsigned cksum = crc32(code,nbytes);
	// put_uint(fd, cksum);

	// printf("expecting SOH\n");

	// echo_until(fd, "\n");

	expect_val(fd, 0x12345678);
	// printf("expecting cksum\n");
	expect_val(fd, crc32(&nbytes, sizeof(nbytes)));
	// printf("expecting cksum2\n");
	// expect_val(fd, cksum);

	put_uint(fd, ACK);
	for(unsigned int i = 0; i < nbytes/sizeof(unsigned); i++) {
		put_uint(fd, code[i]);
	}
	// send EOT
	put_uint(fd, EOT);
	expect_val(fd, ACK);
}

// ship pi program to the pi.
static int run_pi_prog(int pi_fd, char *argv[], int nargs) {
  if(is_pi_prog(argv[0])) {
		printf("command %s\n", argv[0]);
		char* msg = "bin\n";
		pi_put(pi_fd, msg);
		send_prog(pi_fd, argv[0]);
		return 1;
	}
	return 0;
}

/*
 * suggested steps:
 * 	1. just do echo.
 *	2. add reboot()
 *	3. add catching control-C, with reboot.
 *	4. run simple program: anything that ends in ".bin"
 *
 * NOTE: any command you send to pi must end in `\n` given that it reads
 * until newlines!
 */
static int shell(int pi_fd, int unix_fd) {
	const unsigned maxargs = 32;
	char *argv[maxargs];
	char buf[8192];

	catch_control_c();

	// wait for the welcome message from the pi?  note: we
	// will hang if the pi does not send an entire line.  not
	// sure about this: should we keep reading til newline?
	note("> ");

	// put_uint(pi_fd, 0x12345678);
	// unsigned c = get_uint(pi_fd);
	// while(1) {
	// 	if(c == 0x12345678) {
	// 		printf("pi is ready!\n");
	// 		break;
	// 	} else {
	// 		printf("bad cookie from pi: %u\n", c);
	// 	}
	// 	c = get_uint(pi_fd);
	// }
	// for(unsigned i = 0; i < 20; i++) get_uint(pi_fd);
	// fgets(buf, sizeof buf, stdin);
	pi_put(pi_fd,"echo hello world\n");
	echo_until(pi_fd, "\n");
	note("> ");
	while(!done && fgets(buf, sizeof buf, stdin)) {
		int n = strlen(buf)-1;
		buf[n] = 0;

		if(strncmp(buf, "echo", 4) == 0) {
			printf("Going to echo: %s\n",buf);
			pi_put(pi_fd,buf);
			pi_put(pi_fd, "\n");
			echo_until(pi_fd, "\n");
		} else if(strncmp(buf, "get", 3) ==0) {
			unsigned core = buf[4]-'0';
			if(core < 1 || core > 3) { 
				printf("bad core number\n");
				continue;
			}
			printf("Getting output from core %u\n",core);

			pi_put(pi_fd,buf);
			pi_put(pi_fd, "\n");

			echo_until(pi_fd, "\n");
		} else if(strncmp(buf, "send", 4) == 0) {
			unsigned core = buf[5]-'0';
			unsigned start = buf[12]-'0';
			if(core < 1 || core > 3) { 
				printf("bad core number\n");
				continue;
			}
			printf("sending to core %u\n", core);
			pi_put(pi_fd,buf);
			pi_put(pi_fd, "\n");

			unsigned long addr = 0x1000000*start;
			unsigned lower = (unsigned) addr;
			unsigned upper = (unsigned) (addr >> 32);
			put_uint(pi_fd, lower);
			put_uint(pi_fd, upper);
			printf("%u %u\n", upper, lower);
			echo_until(pi_fd, "\n");
			printf("sending program\n");
			send_prog(pi_fd, buf+7);
			echo_until(pi_fd, "\n");

			
		} else if(strncmp(buf, "stop", 4) == 0) {
			printf("stopping!!!\n");
			pi_put(pi_fd,buf);
			pi_put(pi_fd, "\n");
			echo_until(pi_fd, "\n");
		}

		// // is it a builtin?  do it.
		// else if(do_builtin_cmd(pi_fd, argv, nargs))
		// 	;
		// // if not a pi program (end in .bin) fork-exec
		// else //{if(!run_pi_prog(pi_fd, argv, nargs))}
		// 	do_unix_cmd(argv, nargs);
		note("> ");
	}

	if(done) {
		printf("\ngot control-c: going to shutdown pi.\n");
    pi_put(pi_fd, "reboot\n");
    pi_readline(pi_fd, buf, 1024);
    printf("pi rebooted. shell done.\n");
    //exit(0);
	}
	return 0;
}

int main(void) {
  const char *portname = 0;
  int fd = open_tty(&portname);

  // set it to be 8n1  and 115200 baud
  fd = set_tty_to_8n1(fd, B115200, 1);

  // giving the pi side a chance to get going.
  sleep(1);
  // echo_until(fd, cmd_done);
	return shell(fd, 0);
}
