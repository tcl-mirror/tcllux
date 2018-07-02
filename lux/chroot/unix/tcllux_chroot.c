/*
 * Copyright (c) 2018 Stuart Cassoff <stwo@users.sourceforge.net>
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


#include <unistd.h>	/* chroot(2) */

#include <tcl.h>

#define EZT_NS		"::tcllux"
#define EZT_EN		"lux"
#define EZT_CMD		"chroot"
#define EZT_INIT	Tcllux_chroot_Init
#define EZT_SAFE_INIT	Tcllux_chroot_SafeInit

#define EZT_CMDPROC	Tcllux_chroot_chroot_Cmd

#include "ezt.h"

TCMD(Tcllux_chroot_chroot_Cmd);

/***/

TCMD(Tcllux_chroot_chroot_Cmd) {
	Tcl_DString ds;
	int res;

	if (objc != 2) {
		return Ezt_WrongNumArgs(interp, 1, objv, "path");
	}

	Tcl_DStringInit(&ds);

	Tcl_UtfToExternalDString(NULL, Tcl_GetString(objv[1]), -1, &ds);

	res = chroot(Tcl_DStringValue(&ds));

	Tcl_DStringFree(&ds);

	if (res == -1) {
		return rperr("Couldn't chroot: ");
	}

	return TCL_OK;
}

/***/

#include "ezt_funcs.c"

#include "ezt_init.c"


#ifdef __cplusplus
}
#endif


/* EOF */
