/*
 * Copyright (c) 2018,2019 Stuart Cassoff <stwo@users.sourceforge.net>
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
 * Tcl extension that provides channel operations.
 */


#ifdef __cplusplus
extern "C" {
#endif


#include <assert.h>	/* assert */
#include <fcntl.h>	/* fcntl */
#include <unistd.h>	/* dup,sync,close */
#include <errno.h>	/* errno */

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

/***/

enum tcllux_chan_lockOpts {
	LUXCHAN_LOCKOPT_SHARED,
	LUXCHAN_LOCKOPT_START,
	LUXCHAN_LOCKOPT_LENGTH,
	LUXCHAN_LOCKOPT_WHENCE,
	LUXCHAN_LOCKOPT_BLOCKING,
	LUXCHAN_LOCKOPT_PIDVAR,
	LUXCHAN_LOCKOPT_INVALID = -1
};

typedef struct Tcllux_chan_lockOpt {
	const char* name;
	enum tcllux_chan_lockOpts id;
} Tcllux_chan_lockOpt;

static Tcllux_chan_lockOpt Tcllux_chan_lockOpts[] = {
	{ "-shared"   , LUXCHAN_LOCKOPT_SHARED   },
	{ "-start"    , LUXCHAN_LOCKOPT_START    },
	{ "-length"   , LUXCHAN_LOCKOPT_LENGTH   },
	{ "-origin"   , LUXCHAN_LOCKOPT_WHENCE   },
	{ "-blocking" , LUXCHAN_LOCKOPT_BLOCKING },
	{ "-pidvar"   , LUXCHAN_LOCKOPT_PIDVAR   },
	{ NULL        , LUXCHAN_LOCKOPT_INVALID  }
};

enum tcllux_chan_lockWhences {
	LUXCHAN_LOCKWHENCE_START,
	LUXCHAN_LOCKWHENCE_CURRENT,
	LUXCHAN_LOCKWHENCE_END,
	LUXCHAN_LOCKWHENCE_INVALID = -1
};

static struct Tcllux_chan_lockWhences {
	const char *name;
	enum tcllux_chan_lockWhences id;
	short l_whence;
} Tcllux_chan_lockWhences[] = {
	{ "start"   , LUXCHAN_LOCKWHENCE_START   , SEEK_SET },
	{ "current" , LUXCHAN_LOCKWHENCE_CURRENT , SEEK_CUR },
	{ "end"     , LUXCHAN_LOCKWHENCE_END     , SEEK_END },
	{ NULL      , LUXCHAN_LOCKWHENCE_INVALID , -1       }
};

enum tcllux_chan_lockCmds {
	LUXCHAN_LOCKCMD_LOCK,
	LUXCHAN_LOCKCMD_UNLOCK,
	LUXCHAN_LOCKCMD_CANLOCK,
	LUXCHAN_LOCKCMD_INVALID = -1
};

static struct Tcllux_chan_lockCmds {
	enum tcllux_chan_lockCmds id;
	const char *errmsg;
} Tcllux_chan_lockCmds[] = {
	{ LUXCHAN_LOCKCMD_LOCK,    "can't lock: "       },
	{ LUXCHAN_LOCKCMD_UNLOCK,  "can't unlock: "     },
	{ LUXCHAN_LOCKCMD_CANLOCK, "can't check lock: " },
	{ LUXCHAN_LOCKCMD_INVALID, NULL                 }
};

/***/

enum tcllux_chan_chanOpts {
	LUXCHAN_CHANOPT_HANDLE,
	LUXCHAN_CHANOPT_CLOSEONEXEC,
	LUXCHAN_CHANOPT_INVALID = -1
};

enum tcllux_chan_chanOptFlags {
	LUXCHAN_CHANOPTFLAG_RD = (1 << 0),
	LUXCHAN_CHANOPTFLAG_WR = (1 << 1),
	LUXCHAN_CHANOPTFLAG_RW  = (LUXCHAN_CHANOPTFLAG_WR | LUXCHAN_CHANOPTFLAG_RD),
	LUXCHAN_CHANOPTFLAG_IV = 0
};

typedef struct Tcllux_chan_chanOpt {
	const char* name;
	enum tcllux_chan_chanOpts id;
	enum tcllux_chan_chanOptFlags flags;
} Tcllux_chan_chanOpt;

static Tcllux_chan_chanOpt Tcllux_chan_chanOpts[] = {
	{ "-handle"      , LUXCHAN_CHANOPT_HANDLE      , LUXCHAN_CHANOPTFLAG_RD },
	{ "-closeonexec" , LUXCHAN_CHANOPT_CLOSEONEXEC , LUXCHAN_CHANOPTFLAG_RW },
	{ NULL           , LUXCHAN_CHANOPT_INVALID     , LUXCHAN_CHANOPTFLAG_IV }
};

typedef struct Tcllux_chan_chanOptVals {
	int closeonexec;
} Tcllux_chan_chanOptVals;

typedef struct Tcllux_chan_chanOptsToProcess {
	int optIdx; /* Index into Tcllux_chan_chanOpts */
	int objIdx; /* Index into objv */
} Tcllux_chan_chanOptsToProcess;

static void Tcllux_chan_ChanInitChanOptVals (Tcllux_chan_chanOptVals *ov);
static int Tcllux_chan_ChanGetOpt   (Tcl_Interp *interp, Tcl_Channel ch, enum tcllux_chan_chanOpts id, Tcl_Obj **po);
static int Tcllux_chan_ChanSetOpt   (Tcl_Interp *interp, Tcl_Channel ch, enum tcllux_chan_chanOpts id, Tcllux_chan_chanOptVals *ov);
static int Tcllux_chan_ChanCheckOpt (Tcl_Interp *interp, enum tcllux_chan_chanOpts id, Tcllux_chan_chanOptVals *ov, Tcl_Obj *qo);

TCMD(Tcllux_chan_configure_Cmd);
TCMD(Tcllux_chan_sync_Cmd);
TCMD(Tcllux_chan_fsync_Cmd);
TCMD(Tcllux_chan_dup_Cmd);
TCMD(Tcllux_chan_lock_Cmd);
TCMD(Tcllux_chan_owner_Cmd);
TCMD(Tcllux_chan_write_Cmd);

static Ezt_Cmd Ezt_Cmds[] = {
	{ "configure" , Tcllux_chan_configure_Cmd , NULL           },
	{ "sync"      , Tcllux_chan_sync_Cmd      , NULL           },
	{ "fsync"     , Tcllux_chan_fsync_Cmd     , (ClientData) 0 },
	{ "fdatasync" , Tcllux_chan_fsync_Cmd     , (ClientData) 1 },
	{ "dup"       , Tcllux_chan_dup_Cmd       , NULL           },
	{ "lock"      , Tcllux_chan_lock_Cmd      , (ClientData) 0 }, /* see Tcllux_chan_lockCmds */
	{ "unlock"    , Tcllux_chan_lock_Cmd      , (ClientData) 1 }, /* see Tcllux_chan_lockCmds */
	{ "canlock"   , Tcllux_chan_lock_Cmd      , (ClientData) 2 }, /* see Tcllux_chan_lockCmds */
	{ "owner"     , Tcllux_chan_owner_Cmd     , NULL           },
	{ "write"     , Tcllux_chan_write_Cmd     , NULL           },
	{ NULL, NULL, NULL }
};

/***/

static int
Tcllux_chan_GetChannelHandle (Tcl_Channel ch, int direction, int *handlePtr) {
	ClientData hd;
	if (Tcl_GetChannelHandle(ch, direction, &hd) != TCL_OK) { return TCL_ERROR; }
	*handlePtr = PTR2INT(hd);
	return TCL_OK;
}

/***/

static void
Tcllux_chan_ChanInitChanOptVals (Tcllux_chan_chanOptVals *ov) {
	ov->closeonexec = 0;
}

/*
 * On success, obj will have refcount 1.
 * On failure, no obj will be created.
 */
static int
Tcllux_chan_ChanGetOpt (Tcl_Interp *interp, Tcl_Channel ch, enum tcllux_chan_chanOpts id, Tcl_Obj **po) {
	int hd;
	int val;
	Tcl_Obj *o = NULL;

	if (Tcllux_chan_GetChannelHandle(ch, (TCL_READABLE | TCL_WRITABLE), &hd) != TCL_OK) {
		return TCL_ERROR;
	}

	val = 0;

	switch (id) {
	case LUXCHAN_CHANOPT_HANDLE:
		o = Tcl_NewIntObj(hd);
		break;
	case LUXCHAN_CHANOPT_CLOSEONEXEC:
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
Tcllux_chan_ChanCheckOpt (Tcl_Interp *interp, enum tcllux_chan_chanOpts id, Tcllux_chan_chanOptVals *ov, Tcl_Obj *qo) {
	int val;

	switch (id) {
	case LUXCHAN_CHANOPT_CLOSEONEXEC:
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
Tcllux_chan_ChanSetOpt (Tcl_Interp *interp, Tcl_Channel ch, enum tcllux_chan_chanOpts id, Tcllux_chan_chanOptVals *ov) {
	int hd;
	int val;

	if (Tcllux_chan_GetChannelHandle(ch, (TCL_READABLE | TCL_WRITABLE), &hd) != TCL_OK) {
		return TCL_ERROR;
	}

	switch (id) {
	case LUXCHAN_CHANOPT_CLOSEONEXEC:
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
		return Ezt_WrongNumArgs(interp, 1, objv, "channelId ?-option ?value? ...?");
	}

	if ((ch = Tcl_GetChannel(interp, Tcl_GetString(objv[1]), &mode)) == NULL) { return TCL_ERROR; }

	Tcllux_chan_ChanInitChanOptVals(&ov);

	if (objc == 3) { /* Get the value of one option */
		if (Tcl_GetIndexFromObjStruct(interp, objv[2], Tcllux_chan_chanOpts, sizeof(Tcllux_chan_chanOpts[0]), "option", TCL_EXACT, &idx) != TCL_OK) { return TCL_ERROR; }
		if ((Tcllux_chan_chanOpts[idx].flags & LUXCHAN_CHANOPTFLAG_RD) != LUXCHAN_CHANOPTFLAG_RD) {
			return rerr("Option isn't readable.");
		}
		if (Tcllux_chan_ChanGetOpt(interp, ch, Tcllux_chan_chanOpts[idx].id, &o)          != TCL_OK) { return TCL_ERROR; }
		Tcl_SetObjResult(interp, o);
		Tcl_DecrRefCount(o);

	} else if (objc > 3) { /* Set the value of one or more options */
		int z;
		int y = 0;
		Tcllux_chan_chanOptsToProcess yoix[COMBIEN(Tcllux_chan_chanOpts) - 1];

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
			if (Tcl_GetIndexFromObjStruct(interp, objv[i], Tcllux_chan_chanOpts, sizeof(Tcllux_chan_chanOpts[0]), "option", TCL_EXACT, &idx) != TCL_OK) { return TCL_ERROR; }
			if (++i == objc) { return NAGFO(Tcllux_chan_chanOpts[idx].name); }
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
			if (Tcllux_chan_ChanCheckOpt(interp, Tcllux_chan_chanOpts[yoix[i].optIdx].id, &ov, objv[yoix[i].objIdx]) != TCL_OK) { return TCL_ERROR; }
		}

		/*
		 * 3) Try to set the options. Stop and return on error.
		 */
		for (i = 0; i < y; i++) {
			if (Tcllux_chan_ChanSetOpt(interp, ch, Tcllux_chan_chanOpts[yoix[i].optIdx].id, &ov) != TCL_OK) { return TCL_ERROR; }
		}

	} else { /* Get the values of almost all options */
		Tcl_Obj *result;
		Tcl_Obj *errors = NULL;
		int errCode;

		result = Tcl_NewListObj(0, NULL);
		Tcl_IncrRefCount(result);

#define LPUTLKV(L,K,V) { Tcl_ListObjAppendElement(NULL,(L),Tcl_NewStringObj((K),-1)); \
			 Tcl_ListObjAppendElement(NULL,(L),(V));                      }

		for (i = 0; i < COMBIEN(Tcllux_chan_chanOpts) - 1; i++) {
			if ((Tcllux_chan_chanOpts[i].flags & LUXCHAN_CHANOPTFLAG_RD) != LUXCHAN_CHANOPTFLAG_RD) {
				continue;
			}
			if ((errCode = Tcllux_chan_ChanGetOpt(interp, ch, Tcllux_chan_chanOpts[i].id, &o)) != TCL_OK) {
				errors = Tcl_NewListObj(0, NULL);
				Tcl_IncrRefCount(errors);
				LPUTLKV(errors, Tcllux_chan_chanOpts[i].name, Tcl_GetReturnOptions(interp, errCode));
				continue;
			}
			LPUTLKV(result, Tcllux_chan_chanOpts[i].name, o);
			Tcl_DecrRefCount(o);
		}

		if (Tcl_ListObjLength(NULL, result, &i) == TCL_ERROR) {
			Tcl_Panic("%s", "Yes it's true, this man has no dict.");
		}
		if (errors != NULL) {
			LPUTLKV(result, "-errors", errors);
			Tcl_DecrRefCount(errors);
		}

#undef LPUTLKV

		Tcl_SetObjResult(interp, result);
		Tcl_DecrRefCount(result);
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

	if (Tcllux_chan_GetChannelHandle(ch, mode, &hd) != TCL_OK) { return TCL_ERROR; }

	if (PTR2INT(clientData)) {
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
	if (Tcllux_chan_GetChannelHandle(ch, mode, &hd) != TCL_OK) {
		 return TCL_ERROR;
	}
	if ((newhd = dup(hd)) == -1) {
		return rperr("Couldn't dup: ");
	}
	if (fcntl(newhd, F_SETFD, FD_CLOEXEC) == -1) {
		close(newhd);
		return rperr("Couldn't dup: ");
	}

	newch = Tcl_MakeFileChannel((ClientData) INT2PTR(newhd), mode);

	Tcl_RegisterChannel(interp, newch);

	rerr(Tcl_GetChannelName(newch));

	return TCL_OK;
}

TCMD(Tcllux_chan_lock_Cmd) {
	Tcl_Channel ch;
	int mode;
	int hd;
	int idx;
	int i;
	Tcl_Obj *pidvar = NULL;
	struct flock fl;
	int l_cmd = -1;
	int shared = 1;
	int whence_idx = 0;
	int blocking = 0;
	short l_type;
	int l_start = 0;
	int l_len = 0;
	int cmd_idx = PTR2INT(clientData);

	if (objc < 2) {
		return Ezt_WrongNumArgs(interp, 1, objv, "channelId ?options?");
	}
	if ((ch = Tcl_GetChannel(interp, Tcl_GetString(objv[1]), &mode)) == NULL) {
		return TCL_ERROR;
	}
	if (Tcllux_chan_GetChannelHandle(ch, mode, &hd) != TCL_OK) {
		 return TCL_ERROR;
	}
	for (i = 2; i < objc; i++) {
		if (Tcl_GetIndexFromObjStruct(interp, objv[i], Tcllux_chan_lockOpts, sizeof(Tcllux_chan_lockOpts[0]), "option", TCL_EXACT, &idx) != TCL_OK) { return TCL_ERROR; }
		if (++i == objc) { return NAGFO(Tcllux_chan_lockOpts[idx].name); }
		switch (Tcllux_chan_lockOpts[idx].id) {
		case LUXCHAN_LOCKOPT_START:    SIYUI(objv[i], &l_start);  break;
		case LUXCHAN_LOCKOPT_LENGTH:   SIYUI(objv[i], &l_len);    break;
		case LUXCHAN_LOCKOPT_WHENCE:   if (Tcl_GetIndexFromObjStruct(interp, objv[i], Tcllux_chan_lockWhences, sizeof(Tcllux_chan_lockWhences[0]), "origin", TCL_EXACT, &whence_idx) != TCL_OK) { return TCL_ERROR; }; break;
		case LUXCHAN_LOCKOPT_SHARED:   SIYUB(objv[i], &shared);   break;
		case LUXCHAN_LOCKOPT_BLOCKING: SIYUB(objv[i], &blocking); break;
		case LUXCHAN_LOCKOPT_PIDVAR:   pidvar = objv[i];          break;
		case LUXCHAN_LOCKOPT_INVALID:  Tcl_Panic("%s", "Ha!");    break;
		default: Tcl_Panic("%s", "Should we panic? Yes.");        break;
		}
	}
	l_cmd  = blocking ? F_SETLKW : F_SETLK;
	l_type = shared   ? F_RDLCK  : F_WRLCK;
	switch (Tcllux_chan_lockCmds[cmd_idx].id) {
	case LUXCHAN_LOCKCMD_LOCK:
		break;
	case LUXCHAN_LOCKCMD_UNLOCK:
		l_type = F_UNLCK;
		break;
	case LUXCHAN_LOCKCMD_CANLOCK:
		l_cmd = F_GETLK;
		break;
	default:
		Tcl_Panic("%s", "He sold him a horse but delivered a mule.");
		break;
	}
	fl.l_start = l_start;
	fl.l_len = l_len;
	fl.l_pid = -1;
	fl.l_type = l_type;
	fl.l_whence = Tcllux_chan_lockWhences[whence_idx].l_whence;
	if (fcntl(hd, l_cmd, &fl) == -1) {
		if (Tcl_GetErrno() != EAGAIN) {
			return Ezt_ReportError(interp, Tcllux_chan_lockCmds[cmd_idx].errmsg, Tcl_PosixError(interp), (char *) NULL);
		}
		Tcl_SetObjResult(interp, Tcl_NewBooleanObj(0));
	} else if (Tcllux_chan_lockCmds[cmd_idx].id == LUXCHAN_LOCKCMD_CANLOCK) {
		if (fl.l_type == F_UNLCK) {
			Tcl_SetObjResult(interp, Tcl_NewBooleanObj(1));
		} else {
			if (pidvar != NULL) {
				if (Tcl_ObjSetVar2(interp, pidvar, NULL, Tcl_NewIntObj(fl.l_pid), TCL_LEAVE_ERR_MSG) == NULL) {
					return TCL_ERROR;
				}
			}
			Tcl_SetObjResult(interp, Tcl_NewBooleanObj(0));
		}
	} else {
		Tcl_SetObjResult(interp, Tcl_NewBooleanObj(1));
	}
	return TCL_OK;
}

TCMD(Tcllux_chan_owner_Cmd) {
	Tcl_Channel ch;
	int mode;
	int hd;
	int val;

	if (objc != 2 && objc != 3) {
		return Ezt_WrongNumArgs(interp, 1, objv, "channelId ?owner?");
	}
	if ((ch = Tcl_GetChannel(interp, Tcl_GetString(objv[1]), &mode)) == NULL) {
		return TCL_ERROR;
	}
	if (Tcllux_chan_GetChannelHandle(ch, mode, &hd) != TCL_OK) {
		 return TCL_ERROR;
	}
	if (objc == 3) {
		SIYUI(objv[2], &val)
		if (fcntl(hd, F_SETOWN, val) == -1) {
			return rperr("can't set owner: ");
		}
	} else {
		if ((val = fcntl(hd, F_GETOWN)) == -1) {
			return rperr("can't get owner: ");
		}
		Tcl_SetObjResult(interp, Tcl_NewIntObj(val));
	}
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

/***/

#include "ezt_funcs.c"

#include "ezt_init.c"


#ifdef __cplusplus
}
#endif


/* EOF */
