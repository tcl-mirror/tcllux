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


#include <sys/utsname.h>	/* uname */

#include <tcl.h>

#define EZT_NS		"::tcllux"
#define EZT_EN		"lux"
#define EZT_CMD		"uname"
#define EZT_INIT	Tcllux_uname_Init
#define EZT_SAFE_INIT	Tcllux_uname_SafeInit

#define EZT_CMDPROC	Tcllux_uname_uname_Cmd

#include "ezt.h"

TCMD(Tcllux_uname_uname_Cmd);

/***/

TCMD(Tcllux_uname_uname_Cmd) {
	Tcl_Obj *l;
	struct utsname u;

	if (objc != 1) {
		return Ezt_WrongNumArgs(interp, 1, objv, NULL);
	}

	if (uname(&u) == -1) {
		return rperr("Couldn't get uname: ");
	}

	l = Tcl_NewListObj(0, NULL);
	Tcl_IncrRefCount(l);

#define LPUTKV(K,V) { Tcl_ListObjAppendElement(NULL,l,Tcl_NewStringObj((K),-1)); \
		      Tcl_ListObjAppendElement(NULL,l,Tcl_NewStringObj((V),-1)); }

	LPUTKV("sysname",  u.sysname);
	LPUTKV("nodename", u.nodename);
	LPUTKV("release",  u.release);
	LPUTKV("version",  u.version);
	LPUTKV("machine",  u.machine);

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
