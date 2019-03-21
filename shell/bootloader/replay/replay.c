#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <fcntl.h>

#include "demand.h"
#include "replay.h"
#include "trace.h"

// use this for timeouts.
static unsigned timeout_secs = 2;

// given unsigned <v> corrupt it so that <v_new> != <v>
static unsigned corrupt32(unsigned v) {
	unsigned c;
	while((c = random()) == v)
		;
	return c;
}

/*
 * run the unix side of bootloader (my-install) as a subprocess where
 * you can talk to it over a socket.
 *
 *	1. create a socket pair.
 *	2. fork, store child pid in <pid>
 *	3. in child:
 *		a. make sock[1] into file descriptor <TRACE_FD_REPLAY>
 *		b. close the unused socket side.
 *		c. execvp using argv[]
 *	4. in parent:
 *		a. close child side socket.
 *		b. make an endpoint.
 *		c. return endpoint.
 */
endpoint_t mk_endpoint_proc(const char *name, Q_t q, char *argv[]) {
	int pid;
	int sock[2];

	if(socketpair(PF_LOCAL, SOCK_STREAM, 0, sock) < 0){
		sys_die(socketpair, failed);
	}

	if((pid = fork()) < 0){
		sys_die(fork, "bad fork");
	}

	if(pid == 0){
		if(dup2(sock[1], TRACE_FD_REPLAY) < 0){
			sys_die(dup2, "bad dup");
		}
		if(close(sock[1]) < 0){
			sys_die(close, "bad close");
		}

		execvp(argv[0], argv);
		sys_die(execvp, "bad exec");
	}

	if(close(sock[1]) < 0){
		sys_die(close, "bad close");
	}
	// this should sort-of follow your handoff code except you're using
	// sockets.
  return mk_endpoint(name, q, sock[0], pid);
}

/*
 * synchronously wait for the child <this> to complete.
 * 	1. if exited cleanly, return it's error code.
 *	2. otherwise return -1.
 */
static int proc_exit_code(endpoint_t *this) {
	// use your hand-off code (more or less)
	int status;
	if(waitpid(this->pid, &status, 0) < 0){
		sys_die(waitpid, "wait pid failed");
	}
	if(!WIFEXITED(status)){
		return -1;
	}
	return WEXITSTATUS(status);
}

static void write_exact(endpoint_t *this, void *buf, int nbytes, int can_fail_p) {
        int n;
        if((n = write(this->fd, buf, nbytes)) < 0 && !can_fail_p)
                panic("i/o error writing to <%s> = <%s>\n",
                        this->name, strerror(errno));
				if(!can_fail_p){
	        demand(n == nbytes, something is wrong);
				}
}

/*
 * Use select to check if the this->fd has data.
 *  	1. return 0 if there is a timeout.
 *	2. return 1 if not.
 */
int has_data(endpoint_t *this, unsigned timeout_secs) {
	fd_set rfds;
	FD_ZERO(&rfds);
	struct timeval tv;
	FD_SET(this->fd, &rfds);
	tv.tv_sec = timeout_secs;
	tv.tv_usec = 0;
	int retval;

	if((retval = select(this->fd + 1, &rfds, NULL, NULL, &tv)) < 0){
		sys_die(select, "select failed");
	}
	else if(!retval){
		return 0;
	}
	return 1;
}

/*
 * Check if <end> has an eof.
 *
 * Note, for later: when a process dies, the 0 bytes EOF is "written"
 * to the socket/pipe. A read() of EOF = 0 bytes.
 *
 * 	1. see if there is data (EOF is data)
 * 	2. read() it if so and verify is EOF.
 *	3. give an error if not EOF.
 *	4. otherwise return 1.
 *
 * Make sure you handle the case that the read() blocks!!
 */
static int is_eof(endpoint_t *end) {
	if(!has_data(end, timeout_secs))
		err("process <%s> should have exited after 1 sec\n", end->name);
	char c;
	int bytes = read(end->fd, &c, 1);
	if(bytes < 0){
		sys_die(read, "bad read");
	}
	else if(bytes == 0){
		return 1;
	}
	return 0;
}

/*
 * Read until you get <n> bytes or 0.
 *	- give an err() if read fails (read() < 0).  EXCEPT: if <can_fail_p>
 *	is != 0.  in which case just return 0.
 *
 * NOTE: the unix side may be sending 1 byte at a time.  Thus, if we read
 * > 1 bytes, we might get a short read.  Thus, you have to keep iterating
 * until you get <n> bytes or there's a fatal error.
 */
// endpoints write 1 byte at a time.  so can get a split.
static int read_exact(endpoint_t *e, void *buf, int n, int can_fail_p) {
	int numRead = 0;
	while(numRead < n){
		if(!has_data(e, timeout_secs)){
			return -1;
		}

		int bytes = read(e->fd, buf+numRead, n-numRead);
		if(bytes < 0){
			sys_die(read, "bad read");
		}
		else if(bytes == 0){
			return numRead;
		}
		numRead += bytes;
	}
	return numRead;
}

// run the replay log.  if the replay'd code is determinist, will behave
// the same.
//
// an interesting thing is that the overall system (your laptop) is very
// non-deterministic and, even if it was determ, its state changes all the
// time.  however, b/c of interfaces (and their contracts) we can have
// deterministic code in the midst of this chaos.
void replay(endpoint_t *end, int corrupt_op) {
	int can_fail_p = 0;
	int status = 0;

	E_t *e = Q_start(&end->replay_log);
	for(int n = 0; e; e = Q_next(e), n++) {
		// note("about to do op= <%s:%d:%x>\n", op_to_s(e->op), e->cnt, e->val);

		unsigned v;

		// polarity of read/write will depend on if the unix
		// or pi side is sending.   right now we just test
		// unix sending.  so when log says WRITE32, we have
		// to read and when READ32, we have to write.
		switch(e->op) {
		case OP_READ8:
		{
			unsigned char c = e->val;
			write_exact(end, &c, 1, can_fail_p);
			break;
		}

		// replay'd process is GET32'ing, so we write to socket.
		case OP_READ32:
		{
			v = e->val;
			if(n == corrupt_op){
				v = corrupt32(v);
				can_fail_p = 1;
			}
			write_exact(end, &v, 4, can_fail_p);
			break;
		}
		// replay'd process is PUT32'ing, so we read from socket.
		case OP_WRITE32:
			assert(n != corrupt_op);
			if(read_exact(end, &v, 4, can_fail_p) !=4){
				if(!can_fail_p){
					panic("read should have been 4\n");
				}
				else{
					goto error;
				}
			}
			break;
		default: panic("invalid op <%d>\n", e->op);
		}


		// note("success: matched %s:%d:%x\n", op_to_s(e->op), e->cnt, e->val);
	}

	// successfully consumed the log.
	if(can_fail_p) {
		// I believe this can only happen on the very last corruption.
		note("process completed its input and did not fail?\n");
		goto error;
	}

	if(!is_eof(end))
		panic("expected eof: got nothing\n");
	else
		note("successful EOF\n");

	// We didn't corrupt anything: should exit with 0.
	// NOTE: if the process is in an infinite loop we will get stuck.
	if((status = proc_exit_code(end)) != 0)
		err("process exited with %d, expected 0\n", status);
	else
		note("SUCCESS: process exited with %d\n", status);
	return;

error:
	// hit an error case.
	demand(can_fail_p, something wrong);
	if((status = proc_exit_code(end)) == 0)
		err("process exited successfully, expected an error\n");
	else
		note("SUCCESS: after corrupt, process exited with %d\n", status);

}
