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


#include <assert.h>	/* assert */
#include <fcntl.h>	/* fcntl  */
#include <unistd.h>	/* dup,sync,close */

#include <tcl.h>

#include "ezt_int2ptr_ptr2int.h"

#define EZT_NS		"::tcllux"
#define EZT_EN		"lux"
#define EZT_CMD		"chan"
#define EZT_ENCMD	"chan"
#define EZT_INIT	Tcllux_chan_Init
#define EZT_SAFE_INIT	Tcllux_chan_SafeInit

#define EZT_CMD_CLIENTDATA 1

#include "ezt.h"

#define SIYU(U) if ((U) != TCL_OK) { return TCL_ERROR; }
#define SIYUB(O,V) SIYU(Tcl_GetBooleanFromObj(interp, (O), (V)))
#define SIYUI(O,V) SIYU(Tcl_GetIntFromObj(interp, (O), (V)))

TCMD(Tcllux_chan_configure_Cmd);
TCMD(Tcllux_chan_sync_Cmd);
TCMD(Tcllux_chan_fsync_Cmd);
TCMD(Tcllux_chan_dup_Cmd);
TCMD(Tcllux_chan_write_Cmd);
TCMD(Tcllux_chan_lock_Cmd);

static Ezt_Cmd Ezt_Cmds[] = {
	{ "configure" , Tcllux_chan_configure_Cmd , NULL           },
	{ "sync"      , Tcllux_chan_sync_Cmd      , NULL           },
	{ "fsync"     , Tcllux_chan_fsync_Cmd     , (ClientData) 0 },
	{ "fdatasync" , Tcllux_chan_fsync_Cmd     , (ClientData) 1 },
	{ "dup"       , Tcllux_chan_dup_Cmd       , NULL           },
	{ "write"     , Tcllux_chan_write_Cmd     , NULL           },
	{ "lock"      , Tcllux_chan_lock_Cmd      , NULL           },
	{ NULL, NULL, NULL }
};

/***/

/* Quasi-sorted */
enum tcllux_chan_chanOptions {
	LUXCHAN_OPT_HANDLE,
	LUXCHAN_OPT_CLOSEONEXEC,
	LUXCHAN_OPT_INVALID = -1
};

enum tcllux_chan_chanOptionFlags {
	LUXCHAN_OPTFLAG_RD = (1 << 0),
	LUXCHAN_OPTFLAG_WR = (1 << 1),
	LUXCHAN_OPTFLAG_RW  = (LUXCHAN_OPTFLAG_WR | LUXCHAN_OPTFLAG_RD),
	LUXCHAN_OPTFLAG_IV = 0
};

typedef struct Tcllux_chan_chanOption {
	const char* name;
	enum tcllux_chan_chanOptions id;
	enum tcllux_chan_chanOptionFlags flags;
} Tcllux_chan_chanOption;

/* Quasi-sorted */
static Tcllux_chan_chanOption Tcllux_chan_chanOptions[] = {
	{ "-handle"      , LUXCHAN_OPT_HANDLE      , LUXCHAN_OPTFLAG_RD },
	{ "-closeonexec" , LUXCHAN_OPT_CLOSEONEXEC , LUXCHAN_OPTFLAG_RW },
	{ NULL           , LUXCHAN_OPT_INVALID     , LUXCHAN_OPTFLAG_IV }
};

/* Quasi-sorted */
typedef struct Tcllux_chan_chanOptVals {
	int closeonexec;
} Tcllux_chan_chanOptVals;

typedef struct Tcllux_chan_chanOptionsToProcess {
	int optIdx; /* Index into Tcllux_chan_chanOptions */
	int objIdx; /* Index into objv */
} Tcllux_chan_chanOptionsToProcess;

static int Tcllux_chan_ChanGetOption   (Tcl_Interp *interp, Tcl_Channel ch, enum tcllux_chan_chanOptions id, Tcl_Obj **po);
static int Tcllux_chan_ChanSetOption   (Tcl_Interp *interp, Tcl_Channel ch, enum tcllux_chan_chanOptions id, Tcllux_chan_chanOptVals *ov);
static int Tcllux_chan_ChanCheckOption (Tcl_Interp *interp, enum tcllux_chan_chanOptions id, Tcllux_chan_chanOptVals *ov, Tcl_Obj *qo);

/***/

/*
 * On success, obj will have refcount 1.
 * On failure, no obj will be created.
 */
static int
Tcllux_chan_ChanGetOption (Tcl_Interp *interp, Tcl_Channel ch, enum tcllux_chan_chanOptions id, Tcl_Obj **po) {
	int hd;
	int val;
	Tcl_Obj *o = NULL;

	if (Tcl_GetChannelHandle(ch, (TCL_READABLE | TCL_WRITABLE), (ClientData) &hd) != TCL_OK) { return TCL_ERROR; }

	val = 0;

	switch (id) {
	case LUXCHAN_OPT_HANDLE:
		o = Tcl_NewIntObj(hd);
		break;
	case LUXCHAN_OPT_CLOSEONEXEC:
		if ((val = fcntl(hd, F_GETFD)) == -1) {
			return rperr("can't get closeonexec: ");
		}
		val = !((val & FD_CLOEXEC) == 0);
		o = Tcl_NewBooleanObj(val);
		break;
	default:
		Tcl_Panic("%s", "Do you know the way to San Jose?");
		break;
	}

	assert(o);

	Tcl_IncrRefCount(o);
	*po = o;

	return TCL_OK;
}

static int
Tcllux_chan_ChanCheckOption (Tcl_Interp *interp, enum tcllux_chan_chanOptions id, Tcllux_chan_chanOptVals *ov, Tcl_Obj *qo) {
	int val;

	switch (id) {
	case LUXCHAN_OPT_CLOSEONEXEC:
		SIYUB(qo, &val)
		ov->closeonexec = val;
		break;
	default:
		Tcl_Panic("%s", "It's Tuesday, but this doesn't look like Belgium.");
		break;
	}

	return TCL_OK;
}

static int
Tcllux_chan_ChanSetOption (Tcl_Interp *interp, Tcl_Channel ch, enum tcllux_chan_chanOptions id, Tcllux_chan_chanOptVals *ov) {
	int hd;
	int val;

	if (Tcl_GetChannelHandle(ch, (TCL_READABLE | TCL_WRITABLE), (ClientData) &hd) != TCL_OK) { return TCL_ERROR; }

	switch (id) {
	case LUXCHAN_OPT_CLOSEONEXEC:
		val = ov->closeonexec;
		if (fcntl(hd, F_SETFD, val ? FD_CLOEXEC : 0) == -1) {
			return rperr("can't set closeonexec: ");
		}
		break;
	default:
		Tcl_Panic("%s", "You have angered the Gazebo!");
		break;
	}

	return TCL_OK;
}

/***/

TCMD(Tcllux_chan_configure_Cmd) {
	int i;
	int idx;
	Tcl_Obj *o;
	Tcllux_chan_chanOptVals ov;
	Tcl_Channel ch;
	int mode;

	if ((objc < 2) || ((objc > 3) && ((objc % 2) == 1))) {
		return Ezt_WrongNumArgs(interp, 1, objv, "channelId ?-option value ...?");
	}

	if ((ch = Tcl_GetChannel(interp, Tcl_GetString(objv[1]), &mode)) == NULL) { return TCL_ERROR; }

	if (objc == 3) { /* Get the value of one option */
		if (Tcl_GetIndexFromObjStruct(interp, objv[2], Tcllux_chan_chanOptions, sizeof(Tcllux_chan_chanOptions[0]), "option", TCL_EXACT, &idx) != TCL_OK) { return TCL_ERROR; }
		if ((Tcllux_chan_chanOptions[idx].flags & LUXCHAN_OPTFLAG_RD) != LUXCHAN_OPTFLAG_RD) {
			return rerr("Option isn't readable.");
		}
		if (Tcllux_chan_ChanGetOption(interp, ch, Tcllux_chan_chanOptions[idx].id, &o)          != TCL_OK) { return TCL_ERROR; }
		Tcl_SetObjResult(interp, o);
		Tcl_DecrRefCount(o);

	} else if (objc > 3) { /* Set the value of one or more options */
		int z;
		int y = 0;
		Tcllux_chan_chanOptionsToProcess yoix[COMBIEN(Tcllux_chan_chanOptions) - 1];

		/* Strategy:
		 *  Process options in user-supplied order.
		 *  Try to disturb as little as possible
		 *  in fear of a variety of errors.
		 *  1) Check all the option names and make a list.
		 *  2) Try to get the values from the 'value' objs.
		 *  3) Try to set the options. Stop and return on error.
		 */

		/* 1) Check that:
		 *   all option names are valid
		 *   all options have supplied values
		 * Build list of options and indexes of corresponding values.
		 */
		for (i = 2; i < objc; i++) {
			if (Tcl_GetIndexFromObjStruct(interp, objv[i], Tcllux_chan_chanOptions, sizeof(Tcllux_chan_chanOptions[0]), "option", TCL_EXACT, &idx) != TCL_OK) { return TCL_ERROR; }
			if (++i == objc) { return NAGFO(Tcllux_chan_chanOptions[idx].name); }
			/* Check for dups */
			for (z = 0; z < y; z++) {
				/* If dup, keep the last one */
				if (yoix[z].optIdx == idx) {
					yoix[z].objIdx = i;
					break;
				}
			}
			/* If new (not dup), add */
			if (z == y) {
				yoix[y].objIdx = i;
				yoix[y].optIdx = idx;
				y++;
			}
		}

		/*
		 * 2) Try to get the caller-supplied values to set for each option.
		 */
		for (i = 0; i < y; i++) {
			if (Tcllux_chan_ChanCheckOption(interp, Tcllux_chan_chanOptions[yoix[i].optIdx].id, &ov, objv[yoix[i].objIdx]) != TCL_OK) { return TCL_ERROR; }
		}

		/*
		 * 3) Try to set the options. Stop and return on error.
		 */
		for (i = 0; i < y; i++) {
			if (Tcllux_chan_ChanSetOption(interp, ch, Tcllux_chan_chanOptions[yoix[i].optIdx].id, &ov) != TCL_OK) { return TCL_ERROR; }
		}

	} else { /* Get the values of almost all options */
		Tcl_Obj *result;
		Tcl_Obj *errors;
		int z;

		result = Tcl_NewDictObj();
		Tcl_IncrRefCount(result);

		errors = Tcl_NewDictObj();
		Tcl_IncrRefCount(errors);

		for (i = 0; i < COMBIEN(Tcllux_chan_chanOptions) - 1; i++) {
			if ((Tcllux_chan_chanOptions[i].flags & LUXCHAN_OPTFLAG_RD) != LUXCHAN_OPTFLAG_RD) {
				continue;
			}
			if ((z = Tcllux_chan_ChanGetOption(interp, ch, Tcllux_chan_chanOptions[i].id, &o)) != TCL_OK) {
				Tcl_DictObjPut(NULL, errors, Tcl_NewStringObj(Tcllux_chan_chanOptions[i].name, -1), Tcl_GetReturnOptions(interp, z));
				o = Tcl_NewStringObj("",-1);
				Tcl_IncrRefCount(o);
			}
			Tcl_DictObjPut(NULL, result, Tcl_NewStringObj(Tcllux_chan_chanOptions[i].name, -1), o);
			Tcl_DecrRefCount(o);
		}

		if (Tcl_DictObjSize(NULL, result, &z) == TCL_ERROR) {
			Tcl_Panic("%s", "Yes it's true, this man has no dict.");
		}
		if (z > 0) {
			Tcl_DictObjPut(NULL, result, Tcl_NewStringObj("-errors", -1), errors);
		}

		Tcl_SetObjResult(interp, result);
		Tcl_DecrRefCount(result);
		Tcl_DecrRefCount(errors);
	}

	return TCL_OK;
}

TCMD(Tcllux_chan_sync_Cmd) {
	if (objc != 1) { return Ezt_WrongNumArgs(interp, 1, objv, NULL); }
	sync();
	return TCL_OK;
}

TCMD(Tcllux_chan_fsync_Cmd) {
	Tcl_Channel ch;
	int mode;
	int hd;

	if (objc != 2) { return Ezt_WrongNumArgs(interp, 1, objv, "channelId"); }

	if ((ch = Tcl_GetChannel(interp, Tcl_GetString(objv[1]), &mode)) == NULL) { return TCL_ERROR; }

	if (Tcl_GetChannelHandle(ch, mode, (ClientData) &hd) != TCL_OK) { return TCL_ERROR; }

	if ((int) clientData) {
		if (fdatasync(hd) == -1) { return rperr("Couldn't fdatasync: "); }
	} else {
		if (fsync(hd) == -1) { return rperr("Couldn't fsync: "); }
	}

	return TCL_OK;
}

TCMD(Tcllux_chan_dup_Cmd) {
	Tcl_Channel ch;
	int mode;
	int hd;
	int newhd;
	Tcl_Channel newch;

	if (objc != 2) {
		return Ezt_WrongNumArgs(interp, 1, objv, "channelId");
	}
	if ((ch = Tcl_GetChannel(interp, Tcl_GetString(objv[1]), &mode)) == NULL) {
		return TCL_ERROR;
	}
	if (Tcl_GetChannelHandle(ch, mode, (ClientData) &hd) != TCL_OK) {
		 return TCL_ERROR;
	}
	if ((newhd = dup(hd)) == -1) {
		return rperr("Couldn't dup: ");
	}
	if (fcntl(newhd, F_SETFD, FD_CLOEXEC) == -1) {
		close(newhd);
		return rperr("Couldn't dup: ");
	}

	newch = Tcl_MakeFileChannel(INT2PTR(newhd), mode);

	Tcl_RegisterChannel(interp, newch);

	rerr(Tcl_GetChannelName(newch));

	return TCL_OK;
}

TCMD(Tcllux_chan_write_Cmd) {
	Tcl_Channel ch; int mode; int written;
	if (objc != 3) { return Ezt_WrongNumArgs(interp, 1, objv, "channelId data"); }
	if ((ch = Tcl_GetChannel(interp, Tcl_GetString(objv[1]), &mode)) == NULL) { return TCL_ERROR; }
	if ((written = Tcl_WriteObj(ch, objv[2])) == -1) { return rperr("Couldn't write: "); }
	Tcl_SetObjResult(interp, Tcl_NewIntObj(written));
	return TCL_OK;
}

TCMD(Tcllux_chan_lock_Cmd) {
	rerr("Coming soon!");
	return TCL_OK;
}

/***/

#include "ezt_funcs.c"

#include "ezt_init.c"


#ifdef __cplusplus
}
#endif


/* EOF */
