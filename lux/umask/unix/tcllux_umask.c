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


#include <sys/types.h>
#include <sys/stat.h>

#include <tcl.h>

#define EZT_NS		"::tcllux"
#define EZT_EN		"lux"
#define EZT_CMD		"umask"
#define EZT_INIT	Tcllux_umask_Init
#define EZT_SAFE_INIT	Tcllux_umask_SafeInit

#define EZT_CMDPROC	Tcllux_umask_umask_Cmd

#define EZT_NO_WRONGNUMARGS 1
#define EZT_NO_REPORTERROR 1

#include "ezt.h"

TCMD(Tcllux_umask_umask_Cmd);

/***/

TCMD(Tcllux_umask_umask_Cmd) {
	mode_t mask_old;

	if (objc > 2) {
		Tcl_WrongNumArgs(interp, 1, objv, "?mask?");
		return TCL_ERROR;
	}

	if (objc == 2) {
		mode_t mask_new;
		int i;
		if (Tcl_GetIntFromObj(interp, objv[1], &i) != TCL_OK) {
			return TCL_ERROR;
		}
		mask_new = i;
		mask_old = umask(mask_new);
	} else {
		mask_old = umask(0);
		umask(mask_old);
	}

	Tcl_SetObjResult(interp, Tcl_NewIntObj(mask_old));

	return TCL_OK;
}

/***/

#include "ezt_init.c"


#ifdef __cplusplus
}
#endif


/* EOF */
