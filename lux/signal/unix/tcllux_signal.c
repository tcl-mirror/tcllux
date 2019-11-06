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
 * Tcl extension that provides signal facilities.
 */


#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>	/* uint64_t */
#include <signal.h>	/* sigaction, kill */
#include <unistd.h>	/* write */
#include <string.h>	/* strsignal */
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

#include "tcllux_signal_signals.c"

/***/

/*
 * Resets signal and fd/chan array elements when channel closed.
 */
static void
Tcllux_signal_ChannelCloseHandler (ClientData clientData) {
	int idx = PTR2INT(clientData);
	struct sigaction nsa;
	nsa.sa_handler = SIG_DFL;
	nsa.sa_flags = 0;
	sigemptyset(&nsa.sa_mask);
	sigaction(Tcllux_signal_Signals[idx].number, &nsa, NULL);
	sigfds[idx] = -1;
	sigchans[idx] = NULL;
}

/***/

static int
Tcllux_signal_GetSigIdx (Tcl_Interp *interp, Tcl_Obj *signameornum, int *idxPtr) {
	int idx;
	int num;
	const char *nameStr;
	if (Tcl_GetIntFromObj(NULL, signameornum, &num) == TCL_OK) {
		if (num == 0) {
			*idxPtr = -1;
			return TCL_OK;
		}
		for (idx = 0; idx < COMBIEN(Tcllux_signal_Signals); idx++) {
			if (Tcllux_signal_Signals[idx].name == NULL) { continue; }
			if (num == Tcllux_signal_Signals[idx].number) {
				*idxPtr = idx;
				return TCL_OK;
			}
		}
		return Ezt_ReportError(interp, "\"", Tcl_GetString(signameornum), "\" isn't a known signal", (char*) NULL);
	}
	nameStr = Tcl_GetString(signameornum);
	if (	   (nameStr[0] == 'S' || nameStr[0] == 's')
		&& (nameStr[1] == 'I' || nameStr[1] == 'i')
		&& (nameStr[2] == 'G' || nameStr[2] == 'g')
	) {
		nameStr += 3;
	}
	for (idx = 0; idx < COMBIEN(Tcllux_signal_Signals); idx++) {
		if (Tcllux_signal_Signals[idx].name == NULL) { continue; }
		if (Tcl_StringCaseMatch(nameStr, Tcllux_signal_Signals[idx].name, TCL_MATCH_NOCASE)) {
			*idxPtr = idx;
			return TCL_OK;
		}
	}
	return Ezt_ReportError(interp, "\"", Tcl_GetString(signameornum), "\" isn't a known signal", (char*) NULL);
}

/*
 * Returned obj has refcount 1.
 */
static Tcl_Obj *
Tcllux_signal_GetSigactionInfo (int idx) {
	Tcl_Obj *l;
	Tcl_Obj *ml;
	struct sigaction osa;
	int i;

	sigaction(Tcllux_signal_Signals[idx].number, NULL, &osa);

	l = Tcl_NewListObj(0, NULL);
	Tcl_IncrRefCount(l);

	Tcl_ListObjAppendElement(NULL, l, Tcl_NewStringObj("name", -1));
	Tcl_ListObjAppendElement(NULL, l, Tcl_NewStringObj(Tcllux_signal_Signals[idx].name, -1));
	Tcl_ListObjAppendElement(NULL, l, Tcl_NewStringObj("number", -1));
	Tcl_ListObjAppendElement(NULL, l, Tcl_NewIntObj(Tcllux_signal_Signals[idx].number));

	ml = Tcl_NewListObj(0, NULL);
	Tcl_IncrRefCount(ml);
	for (i = 0; i < COMBIEN(Tcllux_signal_Signals); i++) {
		if (Tcllux_signal_Signals[i].name == NULL) { continue; }
		if (!sigismember(&osa.sa_mask, Tcllux_signal_Signals[i].number)) { continue; }
		Tcl_ListObjAppendElement(NULL, ml, Tcl_NewStringObj(Tcllux_signal_Signals[i].name, -1));
	}	
	Tcl_ListObjAppendElement(NULL, l, Tcl_NewStringObj("mask", -1));
	Tcl_ListObjAppendElement(NULL, l, ml);
	Tcl_DecrRefCount(ml);

	Tcl_ListObjAppendElement(NULL, l, Tcl_NewStringObj("flags", -1));
	Tcl_ListObjAppendElement(NULL, l, Tcl_NewLongObj(osa.sa_flags));

	Tcl_ListObjAppendElement(NULL, l, Tcl_NewStringObj("action", -1));
	if (osa.sa_handler == SIG_DFL) {
		Tcl_ListObjAppendElement(NULL, l, Tcl_NewStringObj("default", -1));
	} else if (osa.sa_handler == SIG_IGN) {
		Tcl_ListObjAppendElement(NULL, l, Tcl_NewStringObj("ignore", -1));
	} else {
		Tcl_ListObjAppendElement(NULL, l, Tcl_NewStringObj(Tcl_GetChannelName(sigchans[idx]), -1));
		Tcl_ListObjAppendElement(NULL, l, Tcl_NewStringObj("send", -1));
		if (osa.sa_handler == Tcllux_signal_Signals[idx].sighs) {
			Tcl_ListObjAppendElement(NULL, l, Tcl_NewStringObj("signal", -1));
		} else if (osa.sa_handler == Tcllux_signal_Signals[idx].sight) {
			Tcl_ListObjAppendElement(NULL, l, Tcl_NewStringObj("timestamp", -1));
		} else {
			Tcl_ListObjAppendElement(NULL, l, Tcl_NewStringObj("?", -1));
		}
	}
	return l;
}

TCMD(Tcllux_signal_set_Cmd) {
	const char *action;
	int idx = 0; /*cw*/
	Tcl_Channel chan = NULL;
	int fd = -1;
	int timestamp = 0;
	struct sigaction nsa;

	if (objc < 2 || objc > 4) {
		return Ezt_WrongNumArgs(interp, 1, objv, "sig ?action? ?send?");
	}

	if (Tcllux_signal_GetSigIdx(interp, objv[1], &idx) != TCL_OK) {
		return TCL_ERROR;
	}

	if (idx == -1) {
		return TCL_OK;
	}

	if (objc == 2) {
		Tcl_Obj *o;
		o = Tcllux_signal_GetSigactionInfo(idx);
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
		nsa.sa_handler = timestamp ? Tcllux_signal_Signals[idx].sight : Tcllux_signal_Signals[idx].sighs;
	}

	nsa.sa_flags = 0;
	sigemptyset(&nsa.sa_mask);

	sigfds[idx] = fd;

	if (sigchans[idx] != NULL) {
		Tcl_DeleteCloseHandler(sigchans[idx], Tcllux_signal_ChannelCloseHandler, (ClientData) INT2PTR(idx));
		sigchans[idx] = NULL;
	}

	if (chan != NULL) {
		Tcl_CreateCloseHandler(chan, Tcllux_signal_ChannelCloseHandler, (ClientData) INT2PTR(idx));
		sigchans[idx] = chan;
	}

	if (sigaction(Tcllux_signal_Signals[idx].number, &nsa, NULL) != 0) {
		if (sigchans[idx] != NULL) {
			Tcl_DeleteCloseHandler(chan, Tcllux_signal_ChannelCloseHandler, (ClientData) INT2PTR(idx));
			sigchans[idx] = NULL;
		}
		sigfds[idx] = -1;
		return rperr("Couldn't set signal: ");
	}

	return TCL_OK;
}

TCMD(Tcllux_signal_send_Cmd) {
	pid_t pid;
	int idx;
	if (objc != 3) {
		return Ezt_WrongNumArgs(interp, 1, objv, "sig pid");
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &pid) != TCL_OK) {
		return TCL_ERROR;
	}
	if (Tcllux_signal_GetSigIdx(interp, objv[1], &idx) != TCL_OK) {
		return TCL_ERROR;
	}
	if (kill(pid, idx == -1 ? 0 : Tcllux_signal_Signals[idx].number) != 0) {
		return rperr("Couldn't send signal: ");
	}
	return TCL_OK;
}

TCMD(Tcllux_signal_signals_Cmd) {
	Tcl_Obj *r;
	Tcl_Obj *l;
	int idx;
	if (objc != 1) {
		return Ezt_WrongNumArgs(interp, 1, objv, NULL);
	}
	r = Tcl_NewListObj(0, NULL);
	Tcl_IncrRefCount(r);
	for (idx = 0; idx < COMBIEN(Tcllux_signal_Signals); idx++) {
		if (Tcllux_signal_Signals[idx].name == NULL) { continue; }
		l = Tcl_NewListObj(0, NULL);
		Tcl_IncrRefCount(l);
		Tcl_ListObjAppendElement(NULL, l, Tcl_NewStringObj(Tcllux_signal_Signals[idx].name, -1));
		Tcl_ListObjAppendElement(NULL, l, Tcl_NewIntObj(Tcllux_signal_Signals[idx].number));
		Tcl_ListObjAppendElement(NULL, l, Tcl_NewStringObj(strsignal(Tcllux_signal_Signals[idx].number), -1));
		Tcl_ListObjAppendElement(NULL, r, l);
		Tcl_DecrRefCount(l);
	}
	Tcl_SetObjResult(interp, r);
	Tcl_DecrRefCount(r);
	return TCL_OK;
}

/***/

#include "ezt_funcs.c"

#include "ezt_init.c"


#ifdef __cplusplus
}
#endif


/* EOF */
