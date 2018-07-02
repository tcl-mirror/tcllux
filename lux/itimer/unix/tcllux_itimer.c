/*
 * Copyright (c) 2017,2018 Stuart Cassoff <stwo@users.sourceforge.net>
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


#include <sys/time.h>	/* getitimer,setitimer */

#include <tcl.h>

#define EZT_NS		"::tcllux"
#define EZT_EN		"lux"
#define EZT_CMD		"itimer"
#define EZT_ENCMD	"itimer"
#define EZT_INIT	Tcllux_itimer_Init
#define EZT_SAFE_INIT	Tcllux_itimer_SafeInit

#define EZT_TIMEVAL2DBL 1
#define EZT_DBL2TIMEVAL 1

#include "ezt.h"

TCMD(Tcllux_itimer_itimers_Cmd);
TCMD(Tcllux_itimer_get_Cmd);
TCMD(Tcllux_itimer_set_Cmd);

static Ezt_Cmd Ezt_Cmds[] = {
	{ "itimers" , Tcllux_itimer_itimers_Cmd },
	{ "get"     , Tcllux_itimer_get_Cmd     },
	{ "set"     , Tcllux_itimer_set_Cmd     },
	{ NULL, NULL }
};

/***/

static int Tcllux_itimer_GetWhich (Tcl_Interp *interp, Tcl_Obj *o, int *which);
static Tcl_Obj * Tcllux_itimer_ItimerToDict (struct itimerval *itv);

/***/

TCMD(Tcllux_itimer_itimers_Cmd) {
	if (objc != 1) {
		return Ezt_WrongNumArgs(interp, 1, objv, NULL);
	}
	Tcl_SetObjResult(interp, Tcl_NewStringObj("real virtual profiling", -1));
	return TCL_OK;
}

TCMD(Tcllux_itimer_get_Cmd) {
	Tcl_Obj *d;
	struct itimerval itv;
	int which;

	if (objc > 2) {
		return Ezt_WrongNumArgs(interp, 1, objv, "?timer?");
	}

	if (objc == 1) {
		which  = ITIMER_REAL;
	} else {
		if (Tcllux_itimer_GetWhich(interp, objv[1], &which) != TCL_OK) {
			return TCL_ERROR;
		}
	}

	if (getitimer(which, &itv) != 0) {
		return rperr("Couldn't get itimer: ");
	}

	d = Tcllux_itimer_ItimerToDict(&itv);
	Tcl_SetObjResult(interp, d);
	Tcl_DecrRefCount(d);

	return TCL_OK;
}

TCMD(Tcllux_itimer_set_Cmd) {
	Tcl_Obj *d;
	struct itimerval itv;
	struct itimerval itv_out;
	int which;

	if (objc < 3 || objc > 4) {
		return Ezt_WrongNumArgs(interp, 1, objv, "timer value ?interval?");
	}

	if (Tcllux_itimer_GetWhich(interp, objv[1], &which) != TCL_OK) {
		return TCL_ERROR;
	}

	timerclear(&itv.it_interval);
	timerclear(&itv.it_value);

	if (Ezt_DoubleObjToTimeval(interp, objv[2], &itv.it_value) != TCL_OK) {
		return TCL_ERROR;
	}

	if (objc == 4) {
		if (Ezt_DoubleObjToTimeval(interp, objv[3], &itv.it_interval) != TCL_OK) {
			return TCL_ERROR;
		}
	}

	if (setitimer(which, &itv, &itv_out) != 0) {
		return rperr("Couldn't set itimer: ");
	}

	d = Tcllux_itimer_ItimerToDict(&itv_out);
	Tcl_SetObjResult(interp, d);
	Tcl_DecrRefCount(d);

	return TCL_OK;
}

/***/

static int
Tcllux_itimer_GetWhich (Tcl_Interp *interp, Tcl_Obj *o, int *which) {
	const char *optionStr = Tcl_GetString(o);
	if (Tcl_StringMatch(optionStr, "real")) {
		*which = ITIMER_REAL;
	} else if (Tcl_StringMatch(optionStr, "virtual")) {
		*which = ITIMER_VIRTUAL;
	} else if (Tcl_StringMatch(optionStr, "profiling")) {
		*which = ITIMER_PROF;
	} else {
		return Ezt_ReportError(interp, "bad timer \"", optionStr, "\": must be real, virtual or profiling", (char *) NULL);
	}
	return TCL_OK;
}

/* Returned Obj has refcount 1. */
static Tcl_Obj *
Tcllux_itimer_ItimerToDict (struct itimerval *itv) {
	Tcl_Obj *d;
	d = Tcl_NewDictObj();
	Tcl_IncrRefCount(d);
	Tcl_DictObjPut(NULL, d, Tcl_NewStringObj("interval", -1), Ezt_TimevalToDoubleObj(&itv->it_interval));
	Tcl_DictObjPut(NULL, d, Tcl_NewStringObj("value",    -1), Ezt_TimevalToDoubleObj(&itv->it_value));
	return d;
}

/***/

#include "ezt_funcs.c"

#include "ezt_init.c"


#ifdef __cplusplus
}
#endif


/* EOF */
