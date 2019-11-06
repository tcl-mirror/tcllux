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

#define _GNU_SOURCE 1	/* Needed for getresuid on some systems */

#include <unistd.h>	/* getlogin_r,getresuid */
#include <sys/types.h>	/* getlogin_r,getresuid */
#include <limits.h>	/* LOGIN_NAME_MAX */

#include <tcl.h>

#define EZT_NS		"::tcllux"
#define EZT_EN		"lux"
#define EZT_CMD		"user"
#define EZT_ENCMD	"user"
#define EZT_INIT	Tcllux_user_Init
#define EZT_SAFE_INIT	Tcllux_user_SafeInit

#include "ezt.h"

TCMD(Tcllux_user_login_Cmd);
TCMD(Tcllux_user_ids_Cmd);

static Ezt_Cmd Ezt_Cmds[] = {
	{ "login" , Tcllux_user_login_Cmd },
	{ "ids"   , Tcllux_user_ids_Cmd   },
	{ NULL, NULL }
};

/***/

TCMD(Tcllux_user_login_Cmd) {
	char buf[LOGIN_NAME_MAX];
	if (objc != 1) {
		return Ezt_WrongNumArgs(interp, 1, objv, NULL);
	}
	if (getlogin_r(buf, sizeof buf) != 0) {
		return rperr("Couldn't get login: ");
	}
	Tcl_SetObjResult(interp, Tcl_NewStringObj(buf, -1));
	return TCL_OK;
}

#define LPUTKV(K,V) { Tcl_ListObjAppendElement(NULL,l,Tcl_NewStringObj((K),-1)); \
		      Tcl_ListObjAppendElement(NULL,l,Tcl_NewLongObj((V)));      }

TCMD(Tcllux_user_ids_Cmd) {
	Tcl_Obj *l;
	uid_t ruid, euid, suid;
	if (objc != 1) {
		return Ezt_WrongNumArgs(interp, 1, objv, NULL);
	}
	if (getresuid(&ruid, &euid, &suid) != 0) {
		return rperr("Couldn't get ids: ");
	}
	l = Tcl_NewListObj(0, NULL);
	Tcl_IncrRefCount(l);
	LPUTKV("ruid", ruid);
	LPUTKV("euid", euid);
	LPUTKV("suid", suid);
	Tcl_SetObjResult(interp, l);
	Tcl_DecrRefCount(l);
	return TCL_OK;
}

#undef LPUTKV

#include "ezt_funcs.c"

#include "ezt_init.c"


#ifdef __cplusplus
}
#endif


/* EOF */
