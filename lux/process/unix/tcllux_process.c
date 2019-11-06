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


#include <unistd.h>		/* getpid,getpgrp */
#include <sys/resource.h>	/* get/setpriority */
#include <errno.h>

#include <tcl.h>

#define EZT_NS		"::tcllux"
#define EZT_EN		"lux"
#define EZT_CMD		"process"
#define EZT_ENCMD	"process"
#define EZT_INIT	Tcllux_process_Init
#define EZT_SAFE_INIT	Tcllux_process_SafeInit

#include "ezt.h"

TCMD(Tcllux_process_ids_Cmd);
TCMD(Tcllux_process_gid_Cmd);
TCMD(Tcllux_process_fork_Cmd);
TCMD(Tcllux_process_exec_Cmd);
TCMD(Tcllux_process_priority_Cmd);

static Ezt_Cmd Ezt_Cmds[] = {
	{ "ids"      , Tcllux_process_ids_Cmd      },
	{ "gid"      , Tcllux_process_gid_Cmd      },
	{ "fork"     , Tcllux_process_fork_Cmd     },
	{ "exec"     , Tcllux_process_exec_Cmd     },
	{ "priority" , Tcllux_process_priority_Cmd },
	{ NULL, NULL }
};

enum tcllux_processWitches {
	LUXPROCESS_WITCH_USER,
	LUXPROCESS_WITCH_GROUP,
	LUXPROCESS_WITCH_PROCESS,
	LUXPROCESS_WITCH_INVALID = -1
};

/* Sorted by enum */
static struct Tcllux_processWitch {
	const char *name;
	enum tcllux_processWitches witch;
} Tcllux_processWitches[] = {
	{ "user"    , LUXPROCESS_WITCH_USER    },
	{ "group"   , LUXPROCESS_WITCH_GROUP   },
	{ "process" , LUXPROCESS_WITCH_PROCESS },
	{ NULL      , LUXPROCESS_WITCH_INVALID }
};

/***/

#define LPUTKV(K,V) { Tcl_ListObjAppendElement(NULL,l,Tcl_NewStringObj((K),-1)); \
		      Tcl_ListObjAppendElement(NULL,l,Tcl_NewLongObj((V)));      }

TCMD(Tcllux_process_ids_Cmd) {
	Tcl_Obj *l;
	if (objc != 1) {
		return Ezt_WrongNumArgs(interp, 1, objv, NULL);
	}
	l = Tcl_NewListObj(0, NULL);
	Tcl_IncrRefCount(l);
	LPUTKV("pid",  getpid());
	LPUTKV("ppid", getppid());
	Tcl_SetObjResult(interp, l);
	Tcl_DecrRefCount(l);
	return TCL_OK;
}

#undef LPUTKV

TCMD(Tcllux_process_gid_Cmd) {
	if (objc != 1) {
		return Ezt_WrongNumArgs(interp, 1, objv, NULL);
	}
	Tcl_SetObjResult(interp, Tcl_NewLongObj(getpgrp()));
	return TCL_OK;
}

TCMD(Tcllux_process_fork_Cmd) {
	if (objc != 1) {
		return Ezt_WrongNumArgs(interp, 1, objv, NULL);
	}
	Tcl_SetObjResult(interp, Tcl_NewLongObj(fork()));
	return TCL_OK;
}

TCMD(Tcllux_process_exec_Cmd) {
	Tcl_DString ds;
	int res;

	if (objc != 2) {
		return Ezt_WrongNumArgs(interp, 1, objv, "path");
	}

	Tcl_DStringInit(&ds);

	Tcl_UtfToExternalDString(NULL, Tcl_GetString(objv[1]), -1, &ds);

	res = execl(Tcl_DStringValue(&ds), Tcl_DStringValue(&ds), NULL);

	Tcl_DStringFree(&ds);

	if (res == -1) {
		return rperr("Couldn't exec: ");
	}

	return TCL_OK;
}

TCMD(Tcllux_process_priority_Cmd) {
	int save_errno;
	int witch;
	int iwho;
	int priority;

	if (objc != 3 && objc != 4) {
		return Ezt_WrongNumArgs(interp, 1, objv, "which who ?priority?");
	}

	if (Tcl_GetIndexFromObjStruct(interp, objv[1], Tcllux_processWitches, sizeof(Tcllux_processWitches[0]), "which", TCL_EXACT, &witch) != TCL_OK) { return TCL_ERROR; }
	if (Tcl_GetIntFromObj(interp, objv[2], &iwho) != TCL_OK) { return TCL_ERROR; }

	if (objc == 4) {
		if (Tcl_GetIntFromObj(interp, objv[3], &priority) != TCL_OK) { return TCL_ERROR; }
		if (setpriority(witch, (id_t) iwho, priority) == -1) {
			return rperr("Couln't set priority: ");
		}
	}

	save_errno = errno;
	priority = getpriority(witch, (id_t) iwho);
	if (priority == -1 && errno != 0) {
		return rperr("Couln't get priority: ");
	}
	errno = save_errno;

	Tcl_SetObjResult(interp, Tcl_NewIntObj(priority));

	return TCL_OK;
}

/***/

#include "ezt_funcs.c"

#include "ezt_init.c"


#ifdef __cplusplus
}
#endif


/* EOF */
