/*
 * Copyright (c) 2017 Stuart Cassoff <stwo@users.sourceforge.net>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */


/*
 * Tcl extension that provides something.
 */


#ifdef __cplusplus
extern "C" {
#endif


#include <signal.h>	/* sigaction, kill */
#include <unistd.h>	/* write */
#include <time.h>	/* clock_gettime */
#include <fcntl.h>	/* fcntl */
#include <errno.h>	/* errno */

#include <tcl.h>

#include "ezt_int2ptr_ptr2int.h"

#define EZT_NS		"::tcllux"
#define EZT_EN		"lux"
#define EZT_CMD		"signal"
#define EZT_ENCMD	"signal"
#define EZT_INIT	Tcllux_signal_Init
#define EZT_SAFE_INIT	Tcllux_signal_SafeInit

#include "ezt.h"

TCMD(Tcllux_signal_signals_Cmd);
TCMD(Tcllux_signal_set_Cmd);
TCMD(Tcllux_signal_send_Cmd);

static const Ezt_Cmd Ezt_Cmds[] = {
	{ "signals" , Tcllux_signal_signals_Cmd },
	{ "set"     , Tcllux_signal_set_Cmd     },
	{ "send"    , Tcllux_signal_send_Cmd    },
	{ NULL, NULL }
};

/***/

static int           sigfds[NSIG] = {-1};
static Tcl_Channel sigchans[NSIG] = {NULL};

/***/

static void
Tcllux_signal_SigHandler_Signal (int sig) {
	int save_errno = errno;
	int fd = sigfds[sig];
	int flags;
#if 0
	long buf[2] = {0};
	struct timespec ts;
	if (clock_gettime(CLOCK_REALTIME, &ts) == 0) {
		buf[0] = ts.tv_sec;
		buf[1] = ts.tv_nsec;
	}
#else
	char buf[1] = {(char) sig};
#endif
	flags = fcntl(fd, F_GETFL);
	if (flags & O_NONBLOCK) {
		write(fd, buf, sizeof buf);
	} else {
		fcntl(fd, F_SETFL, flags | O_NONBLOCK);
		write(fd, buf, sizeof buf);
		fcntl(fd, F_SETFL, flags);
	}
	errno = save_errno;
}

static void
Tcllux_signal_SigHandler_Timestamp (int sig) {
	int save_errno = errno;
	int fd = sigfds[sig];
	int flags;
#if 1
	long buf[2] = {0};
	struct timespec ts;
	if (clock_gettime(CLOCK_REALTIME, &ts) == 0) {
		buf[0] = ts.tv_sec;
		buf[1] = ts.tv_nsec;
	}
#else
	char buf[1] = {(char) sig};
#endif
	flags = fcntl(fd, F_GETFL);
	if (flags & O_NONBLOCK) {
		write(fd, buf, sizeof buf);
	} else {
		fcntl(fd, F_SETFL, flags | O_NONBLOCK);
		write(fd, buf, sizeof buf);
		fcntl(fd, F_SETFL, flags);
	}
	errno = save_errno;
}

/*
 * Resets signal and fd/chan array elements when channel closed.
 */
static void
Tcllux_signal_ChannelCloseHandler (ClientData clientData) {
	int sig = (int) clientData;
	struct sigaction nsa = {{0}, 0, 0};
	nsa.sa_handler = SIG_DFL;
	sigaction(sig, &nsa, NULL);
	sigfds[sig] = -1;
	sigchans[sig] = NULL;
}

/***/

static int
Tcllux_signal_GetSigNum (Tcl_Interp *interp, Tcl_Obj *signameornum, int *signum) {
	int num;
	if (Tcl_GetIntFromObj(NULL, signameornum, &num) != TCL_OK) {
		const char *nameStr = Tcl_GetString(signameornum);
		if (	   (nameStr[0] == 'S' || nameStr[0] == 's')
			&& (nameStr[1] == 'I' || nameStr[1] == 'i')
			&& (nameStr[2] == 'G' || nameStr[2] == 'g')
		) {
			nameStr += 3;
		}
		for (num = 1; num < NSIG; num++) {
			if (Tcl_StringCaseMatch(nameStr, sys_signame[num], TCL_MATCH_NOCASE)) {
				break;
			}
		}
		if (num == NSIG) {
			return Ezt_ReportError(interp, "\"", nameStr, "\" isn't a known signal name", (char*) NULL);
		}
	}
	*signum = num;
	return TCL_OK;
}

/*
 * Returned obj has refcount 1.
 */
static Tcl_Obj *
Tcllux_signal_GetSigactionInfo (int sig) {
	Tcl_Obj *d;
	struct sigaction osa;

	sigaction(sig, NULL, &osa);

	d = Tcl_NewDictObj();
	Tcl_IncrRefCount(d);

	Tcl_DictObjPut(NULL, d, Tcl_NewStringObj("name",   -1), Tcl_NewStringObj(sys_signame[sig], -1));
	Tcl_DictObjPut(NULL, d, Tcl_NewStringObj("number", -1), Tcl_NewIntObj(sig));
	Tcl_DictObjPut(NULL, d, Tcl_NewStringObj("mask",   -1), Tcl_NewLongObj(osa.sa_mask));
	Tcl_DictObjPut(NULL, d, Tcl_NewStringObj("flags",  -1), Tcl_NewLongObj(osa.sa_flags));

	if (osa.sa_handler == SIG_DFL) {
		Tcl_DictObjPut(NULL, d, Tcl_NewStringObj("action", -1), Tcl_NewStringObj("default", -1));
	} else if (osa.sa_handler == SIG_IGN) {
		Tcl_DictObjPut(NULL, d, Tcl_NewStringObj("action", -1), Tcl_NewStringObj("ignore", -1));
	} else {
		Tcl_Obj *o;
		Tcl_DictObjPut(NULL, d, Tcl_NewStringObj("action", -1), Tcl_NewStringObj(Tcl_GetChannelName(sigchans[sig]), -1));
		if (osa.sa_handler == Tcllux_signal_SigHandler_Signal) {
			o = Tcl_NewStringObj("signal", -1);
		} else if (osa.sa_handler == Tcllux_signal_SigHandler_Timestamp) {
			o = Tcl_NewStringObj("timestamp", -1);
		} else {
			o = Tcl_NewStringObj("?", -1);
		}
		Tcl_IncrRefCount(o);
		Tcl_DictObjPut(NULL, d, Tcl_NewStringObj("send",  -1), o);
		Tcl_DecrRefCount(o);
	}

	return d;
}

TCMD(Tcllux_signal_set_Cmd) {
	const char *action;
	int sig = 0; /*cw*/
	Tcl_Channel chan = NULL;
	int fd = -1;
	int timestamp = 0;
	struct sigaction nsa = {{0}, 0, 0};

	if (objc < 2 || objc > 4) {
		return Ezt_WrongNumArgs(interp, 1, objv, "sig ?action? ?send?");
	}

	if (Tcllux_signal_GetSigNum(interp, objv[1], &sig) != TCL_OK) {
		return TCL_ERROR;
	}

	if (objc == 2) {
		Tcl_Obj *o;
		o = Tcllux_signal_GetSigactionInfo(sig);
		Tcl_SetObjResult(interp, o);
		Tcl_DecrRefCount(o);
		return TCL_OK;
	}

	if (objc == 4) {
		action = Tcl_GetString(objv[3]);
		if (Tcl_StringMatch(action, "signal")) {
			timestamp = 0;
		} else if (Tcl_StringMatch(action, "timestamp")) {
			timestamp = 1;
		} else {
			return Ezt_ReportError(interp, "bad send \"", action, "\": should be signal or timestamp", (char*) NULL);
		}
	}

	action = Tcl_GetString(objv[2]);

	if (Tcl_StringMatch(action, "default")) {
		nsa.sa_handler = SIG_DFL;
	} else if (Tcl_StringMatch(action, "ignore")) {
		nsa.sa_handler = SIG_IGN;
	} else {
		chan = Tcl_GetChannel(interp, Tcl_GetString(objv[2]), NULL);
		if (chan == NULL) {
			return TCL_ERROR;
		}
		if (Tcl_GetChannelHandle(chan, TCL_WRITABLE, (ClientData) &fd) != TCL_OK) {
			return TCL_ERROR;
		}
		nsa.sa_handler = timestamp ? Tcllux_signal_SigHandler_Timestamp : Tcllux_signal_SigHandler_Signal;
	}

	sigfds[sig] = fd;

	if (sigchans[sig] != NULL) {
		Tcl_DeleteCloseHandler(sigchans[sig], Tcllux_signal_ChannelCloseHandler, (ClientData) INT2PTR(sig));
		sigchans[sig] = NULL;
	}

	if (chan != NULL) {
		Tcl_CreateCloseHandler(chan, Tcllux_signal_ChannelCloseHandler, (ClientData) INT2PTR(sig));
		sigchans[sig] = chan;
	}

	if (sigaction(sig, &nsa, NULL) != 0) {
		if (sigchans[sig] != NULL) {
			Tcl_DeleteCloseHandler(chan, Tcllux_signal_ChannelCloseHandler, (ClientData) INT2PTR(sig));
			sigchans[sig] = NULL;
		}
		sigfds[sig] = -1;
		return rperr("Couldn't set signal: ");
	}

	return TCL_OK;
}

TCMD(Tcllux_signal_send_Cmd) {
	pid_t pid;
	int sig;
	if (objc != 3) {
		return Ezt_WrongNumArgs(interp, 1, objv, "sig pid");
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &pid) != TCL_OK) {
		return TCL_ERROR;
	}
	if (Tcllux_signal_GetSigNum(interp, objv[1], &sig) != TCL_OK) {
		return TCL_ERROR;
	}
	if (kill(pid, sig) != 0) {
		return rperr("Couldn't send signal: ");
	}
	return TCL_OK;
}

TCMD(Tcllux_signal_signals_Cmd) {
	Tcl_Obj *o;
	int i;

	if (objc != 1) {
		return Ezt_WrongNumArgs(interp, 1, objv, NULL);
	}

	o = Tcl_NewDictObj();
	Tcl_IncrRefCount(o);

	Tcl_DictObjPut(NULL, o, Tcl_NewStringObj("", -1), Tcl_NewStringObj("", -1));

	for (i = 1; i < NSIG; i++) {
		Tcl_DictObjPut(NULL, o, Tcl_NewStringObj(sys_signame[i], -1), Tcl_NewStringObj(sys_siglist[i], -1));
	}

	Tcl_SetObjResult(interp, o);
	Tcl_DecrRefCount(o);

	return TCL_OK;
}

/***/

#include "ezt_funcs.c"

#include "ezt_init.c"


#ifdef __cplusplus
}
#endif


/* EOF */
