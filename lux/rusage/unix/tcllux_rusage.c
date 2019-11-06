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
 * Tcl extension that provides a no-operation command.
 */


#ifdef __cplusplus
extern "C" {
#endif


#include <sys/resource.h>	/* getrusage */
#include <stdio.h>		/* snprintf */

#include <tcl.h>

#define EZT_NS		"::tcllux"
#define EZT_EN		"lux"
#define EZT_CMD		"rusage"
#define EZT_INIT	Tcllux_rusage_Init
#define EZT_SAFE_INIT	Tcllux_rusage_SafeInit

#define EZT_CMDPROC	Tcllux_rusage_rusage_Cmd

#define EZT_TIMEVAL2DBL 1

#include "ezt.h"

TCMD(Tcllux_rusage_rusage_Cmd);

/***/

TCMD(Tcllux_rusage_rusage_Cmd) {
	Tcl_Obj *l;
	int who = RUSAGE_SELF;
	struct rusage r;

	if (objc > 2) {
		return Ezt_WrongNumArgs(interp, 1, objv, "?who?");
	}

	if (objc > 1) {
		const char *optionStr = Tcl_GetString(objv[1]);
		if (Tcl_StringMatch(optionStr, "self")) {
			who = RUSAGE_SELF;
		} else if (Tcl_StringMatch(optionStr, "children")) {
			who = RUSAGE_CHILDREN;
#ifdef RUSAGE_THREAD
		} else if (Tcl_StringMatch(optionStr, "thread")) {
			who = RUSAGE_THREAD;
		} else {
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("bad who \"%s\": must be self, children or thread", optionStr));
			return TCL_ERROR;
#else
		} else {
			Tcl_SetObjResult(interp, Tcl_ObjPrintf("bad who \"%s\": must be self or children", optionStr));
			return TCL_ERROR;
#endif
		}
	}

	if (getrusage(who, &r) != 0) {
		return rperr("Couldn't get rusage: ");
	}

	l = Tcl_NewListObj(0, NULL);
	Tcl_IncrRefCount(l);

	Tcl_ListObjAppendElement(NULL, l, Tcl_NewStringObj("rusage", -1));
	Tcl_ListObjAppendElement(NULL, l, objc > 1 ? objv[1] : Tcl_NewStringObj("self", -1));
	Tcl_ListObjAppendElement(NULL, l, Tcl_NewStringObj("utime",  -1));
	Tcl_ListObjAppendElement(NULL, l, Ezt_TimevalToDoubleObj(&r.ru_utime));
	Tcl_ListObjAppendElement(NULL, l, Tcl_NewStringObj("stime",  -1));
	Tcl_ListObjAppendElement(NULL, l, Ezt_TimevalToDoubleObj(&r.ru_stime));

#define LPUTKV(K,V) { Tcl_ListObjAppendElement(NULL,l,Tcl_NewStringObj((K),-1)); \
		      Tcl_ListObjAppendElement(NULL,l,Tcl_NewLongObj((V)));      }

	LPUTKV("maxrss",   r.ru_maxrss);
	LPUTKV("ixrss",    r.ru_ixrss);
	LPUTKV("idrss",    r.ru_idrss);
	LPUTKV("isrss",    r.ru_isrss);
	LPUTKV("minflt",   r.ru_minflt);
	LPUTKV("majflt",   r.ru_majflt);
	LPUTKV("nswap",    r.ru_nswap);
	LPUTKV("inblock",  r.ru_inblock);
	LPUTKV("oublock",  r.ru_oublock);
	LPUTKV("msgsnd",   r.ru_msgsnd);
	LPUTKV("msgrcv",   r.ru_msgrcv);
	LPUTKV("nsignals", r.ru_nsignals);
	LPUTKV("nivcsw",   r.ru_nivcsw);

#undef LPUTKV

	Tcl_SetObjResult(interp, l);
	Tcl_DecrRefCount(l);

	return TCL_OK;
}

/***/

#include "ezt_funcs.c"

#include "ezt_init.c"


#ifdef __cplusplus
}
#endif


/* EOF */
