/* tcllux_signal_signals.c: start */

#define LUXSIGHNBW \
	flags = fcntl(sigfds[N], F_GETFL); \
	if (flags & O_NONBLOCK) { \
		write(sigfds[N], buf, sizeof buf); \
	} else { \
		fcntl(sigfds[N], F_SETFL, flags | O_NONBLOCK); \
		write(sigfds[N], buf, sizeof buf); \
		fcntl(sigfds[N], F_SETFL, flags); \
	}
#define LUXSIGHNONE { NULL, -1, NULL, NULL }
#define LUXSIGHS(N) Tcllux_signal_SigHandler_Signal_##N
#define LUXSIGHT(N) Tcllux_signal_SigHandler_Timestamp_##N
#define LUXSIGHSD(N) static void Tcllux_signal_SigHandler_Signal_##N (int sig)
#define LUXSIGHSC(N) static void Tcllux_signal_SigHandler_Signal_##N (int sig) { \
	int save_errno = errno; \
	char buf[1] = {(char) sig}; \
	write(sigfds[N], buf, sizeof buf); \
	errno = save_errno; \
}
#define LUXSIGHSNBC(N) static void Tcllux_signal_SigHandler_Signal_NB_##N (int sig) { \
	int save_errno = errno; \
	int flags; \
	char buf[1] = {(char) sig}; \
	LUXSIGHNBW \
	errno = save_errno; \
}
#define LUXSIGHTD(N) static void Tcllux_signal_SigHandler_Timestamp_##N (int sig)
#define LUXSIGHTC(N) static void Tcllux_signal_SigHandler_Timestamp_##N (int sig) { \
	int save_errno = errno; \
	uint64_t buf[2] = {0}; \
	struct timespec ts; \
	if (clock_gettime(CLOCK_REALTIME, &ts) == 0) { \
		buf[0] = ts.tv_sec; \
		buf[1] = ts.tv_nsec; \
	} \
	write(sigfds[N], buf, sizeof buf); \
	errno = save_errno; \
}
#define LUXSIGHTNBC(N) static void Tcllux_signal_SigHandler_Timestamp_NB_##N (int sig) { \
	int save_errno = errno; \
	int flags; \
	uint64_t buf[2] = {0}; \
	struct timespec ts; \
	if (clock_gettime(CLOCK_REALTIME, &ts) == 0) { \
		buf[0] = ts.tv_sec; \
		buf[1] = ts.tv_nsec; \
	} \
	LUXSIGHNBW \
	errno = save_errno; \
}

#ifdef SIGHUP
LUXSIGHSD(0);
LUXSIGHTD(0);
#endif
#ifdef SIGINT
LUXSIGHSD(1);
LUXSIGHTD(1);
#endif
#ifdef SIGQUIT
LUXSIGHSD(2);
LUXSIGHTD(2);
#endif
#ifdef SIGILL
LUXSIGHSD(3);
LUXSIGHTD(3);
#endif
#ifdef SIGTRAP
LUXSIGHSD(4);
LUXSIGHTD(4);
#endif
#ifdef SIGABRT
LUXSIGHSD(5);
LUXSIGHTD(5);
#endif
#ifdef SIGEMT
LUXSIGHSD(6);
LUXSIGHTD(6);
#endif
#ifdef SIGFPE
LUXSIGHSD(7);
LUXSIGHTD(7);
#endif
#ifdef SIGKILL
LUXSIGHSD(8);
LUXSIGHTD(8);
#endif
#ifdef SIGBUS
LUXSIGHSD(9);
LUXSIGHTD(9);
#endif
#ifdef SIGSEGV
LUXSIGHSD(10);
LUXSIGHTD(10);
#endif
#ifdef SIGSYS
LUXSIGHSD(11);
LUXSIGHTD(11);
#endif
#ifdef SIGPIPE
LUXSIGHSD(12);
LUXSIGHTD(12);
#endif
#ifdef SIGALRM
LUXSIGHSD(13);
LUXSIGHTD(13);
#endif
#ifdef SIGTERM
LUXSIGHSD(14);
LUXSIGHTD(14);
#endif
#ifdef SIGURG
LUXSIGHSD(15);
LUXSIGHTD(15);
#endif
#ifdef SIGSTOP
LUXSIGHSD(16);
LUXSIGHTD(16);
#endif
#ifdef SIGTSTP
LUXSIGHSD(17);
LUXSIGHTD(17);
#endif
#ifdef SIGCONT
LUXSIGHSD(18);
LUXSIGHTD(18);
#endif
#ifdef SIGCHLD
LUXSIGHSD(19);
LUXSIGHTD(19);
#endif
#ifdef SIGTTIN
LUXSIGHSD(20);
LUXSIGHTD(20);
#endif
#ifdef SIGTTOU
LUXSIGHSD(21);
LUXSIGHTD(21);
#endif
#ifdef SIGIO
LUXSIGHSD(22);
LUXSIGHTD(22);
#endif
#ifdef SIGXCPU
LUXSIGHSD(23);
LUXSIGHTD(23);
#endif
#ifdef SIGXFSZ
LUXSIGHSD(24);
LUXSIGHTD(24);
#endif
#ifdef SIGVTALRM
LUXSIGHSD(25);
LUXSIGHTD(25);
#endif
#ifdef SIGPROF
LUXSIGHSD(26);
LUXSIGHTD(26);
#endif
#ifdef SIGWINCH
LUXSIGHSD(27);
LUXSIGHTD(27);
#endif
#ifdef SIGINFO
LUXSIGHSD(28);
LUXSIGHTD(28);
#endif
#ifdef SIGUSR1
LUXSIGHSD(29);
LUXSIGHTD(29);
#endif
#ifdef SIGUSR2
LUXSIGHSD(30);
LUXSIGHTD(30);
#endif

static struct Tcllux_signal_Signal {
	const char *name;
	int number;
	void (*sighs) (int);
	void (*sight) (int);
} Tcllux_signal_Signals[] = {
#ifdef SIGHUP
	{ "HUP", SIGHUP, LUXSIGHS(0), LUXSIGHT(0) },
#else
	LUXSIGHNONE,
#endif
#ifdef SIGINT
	{ "INT", SIGINT, LUXSIGHS(1), LUXSIGHT(1) },
#else
	LUXSIGHNONE,
#endif
#ifdef SIGQUIT
	{ "QUIT", SIGQUIT, LUXSIGHS(2), LUXSIGHT(2) },
#else
	LUXSIGHNONE,
#endif
#ifdef SIGILL
	{ "ILL", SIGILL, LUXSIGHS(3), LUXSIGHT(3) },
#else
	LUXSIGHNONE,
#endif
#ifdef SIGTRAP
	{ "TRAP", SIGTRAP, LUXSIGHS(4), LUXSIGHT(4) },
#else
	LUXSIGHNONE,
#endif
#ifdef SIGABRT
	{ "ABRT", SIGABRT, LUXSIGHS(5), LUXSIGHT(5) },
#else
	LUXSIGHNONE,
#endif
#ifdef SIGEMT
	{ "EMT", SIGEMT, LUXSIGHS(6), LUXSIGHT(6) },
#else
	LUXSIGHNONE,
#endif
#ifdef SIGFPE
	{ "FPE", SIGFPE, LUXSIGHS(7), LUXSIGHT(7) },
#else
	LUXSIGHNONE,
#endif
#ifdef SIGKILL
	{ "KILL", SIGKILL, LUXSIGHS(8), LUXSIGHT(8) },
#else
	LUXSIGHNONE,
#endif
#ifdef SIGBUS
	{ "BUS", SIGBUS, LUXSIGHS(9), LUXSIGHT(9) },
#else
	LUXSIGHNONE,
#endif
#ifdef SIGSEGV
	{ "SEGV", SIGSEGV, LUXSIGHS(10), LUXSIGHT(10) },
#else
	LUXSIGHNONE,
#endif
#ifdef SIGSYS
	{ "SYS", SIGSYS, LUXSIGHS(11), LUXSIGHT(11) },
#else
	LUXSIGHNONE,
#endif
#ifdef SIGPIPE
	{ "PIPE", SIGPIPE, LUXSIGHS(12), LUXSIGHT(12) },
#else
	LUXSIGHNONE,
#endif
#ifdef SIGALRM
	{ "ALRM", SIGALRM, LUXSIGHS(13), LUXSIGHT(13) },
#else
	LUXSIGHNONE,
#endif
#ifdef SIGTERM
	{ "TERM", SIGTERM, LUXSIGHS(14), LUXSIGHT(14) },
#else
	LUXSIGHNONE,
#endif
#ifdef SIGURG
	{ "URG", SIGURG, LUXSIGHS(15), LUXSIGHT(15) },
#else
	LUXSIGHNONE,
#endif
#ifdef SIGSTOP
	{ "STOP", SIGSTOP, LUXSIGHS(16), LUXSIGHT(16) },
#else
	LUXSIGHNONE,
#endif
#ifdef SIGTSTP
	{ "TSTP", SIGTSTP, LUXSIGHS(17), LUXSIGHT(17) },
#else
	LUXSIGHNONE,
#endif
#ifdef SIGCONT
	{ "CONT", SIGCONT, LUXSIGHS(18), LUXSIGHT(18) },
#else
	LUXSIGHNONE,
#endif
#ifdef SIGCHLD
	{ "CHLD", SIGCHLD, LUXSIGHS(19), LUXSIGHT(19) },
#else
	LUXSIGHNONE,
#endif
#ifdef SIGTTIN
	{ "TTIN", SIGTTIN, LUXSIGHS(20), LUXSIGHT(20) },
#else
	LUXSIGHNONE,
#endif
#ifdef SIGTTOU
	{ "TTOU", SIGTTOU, LUXSIGHS(21), LUXSIGHT(21) },
#else
	LUXSIGHNONE,
#endif
#ifdef SIGIO
	{ "IO", SIGIO, LUXSIGHS(22), LUXSIGHT(22) },
#else
	LUXSIGHNONE,
#endif
#ifdef SIGXCPU
	{ "XCPU", SIGXCPU, LUXSIGHS(23), LUXSIGHT(23) },
#else
	LUXSIGHNONE,
#endif
#ifdef SIGXFSZ
	{ "XFSZ", SIGXFSZ, LUXSIGHS(24), LUXSIGHT(24) },
#else
	LUXSIGHNONE,
#endif
#ifdef SIGVTALRM
	{ "VTALRM", SIGVTALRM, LUXSIGHS(25), LUXSIGHT(25) },
#else
	LUXSIGHNONE,
#endif
#ifdef SIGPROF
	{ "PROF", SIGPROF, LUXSIGHS(26), LUXSIGHT(26) },
#else
	LUXSIGHNONE,
#endif
#ifdef SIGWINCH
	{ "WINCH", SIGWINCH, LUXSIGHS(27), LUXSIGHT(27) },
#else
	LUXSIGHNONE,
#endif
#ifdef SIGINFO
	{ "INFO", SIGINFO, LUXSIGHS(28), LUXSIGHT(28) },
#else
	LUXSIGHNONE,
#endif
#ifdef SIGUSR1
	{ "USR1", SIGUSR1, LUXSIGHS(29), LUXSIGHT(29) },
#else
	LUXSIGHNONE,
#endif
#ifdef SIGUSR2
	{ "USR2", SIGUSR2, LUXSIGHS(30), LUXSIGHT(30) },
#else
	LUXSIGHNONE,
#endif
};

static int           sigfds[COMBIEN(Tcllux_signal_Signals)] = {-1};
static Tcl_Channel sigchans[COMBIEN(Tcllux_signal_Signals)] = {NULL};

#ifdef SIGHUP
LUXSIGHSC(0);
LUXSIGHTC(0);
#endif
#ifdef SIGINT
LUXSIGHSC(1);
LUXSIGHTC(1);
#endif
#ifdef SIGQUIT
LUXSIGHSC(2);
LUXSIGHTC(2);
#endif
#ifdef SIGILL
LUXSIGHSC(3);
LUXSIGHTC(3);
#endif
#ifdef SIGTRAP
LUXSIGHSC(4);
LUXSIGHTC(4);
#endif
#ifdef SIGABRT
LUXSIGHSC(5);
LUXSIGHTC(5);
#endif
#ifdef SIGEMT
LUXSIGHSC(6);
LUXSIGHTC(6);
#endif
#ifdef SIGFPE
LUXSIGHSC(7);
LUXSIGHTC(7);
#endif
#ifdef SIGKILL
LUXSIGHSC(8);
LUXSIGHTC(8);
#endif
#ifdef SIGBUS
LUXSIGHSC(9);
LUXSIGHTC(9);
#endif
#ifdef SIGSEGV
LUXSIGHSC(10);
LUXSIGHTC(10);
#endif
#ifdef SIGSYS
LUXSIGHSC(11);
LUXSIGHTC(11);
#endif
#ifdef SIGPIPE
LUXSIGHSC(12);
LUXSIGHTC(12);
#endif
#ifdef SIGALRM
LUXSIGHSC(13);
LUXSIGHTC(13);
#endif
#ifdef SIGTERM
LUXSIGHSC(14);
LUXSIGHTC(14);
#endif
#ifdef SIGURG
LUXSIGHSC(15);
LUXSIGHTC(15);
#endif
#ifdef SIGSTOP
LUXSIGHSC(16);
LUXSIGHTC(16);
#endif
#ifdef SIGTSTP
LUXSIGHSC(17);
LUXSIGHTC(17);
#endif
#ifdef SIGCONT
LUXSIGHSC(18);
LUXSIGHTC(18);
#endif
#ifdef SIGCHLD
LUXSIGHSC(19);
LUXSIGHTC(19);
#endif
#ifdef SIGTTIN
LUXSIGHSC(20);
LUXSIGHTC(20);
#endif
#ifdef SIGTTOU
LUXSIGHSC(21);
LUXSIGHTC(21);
#endif
#ifdef SIGIO
LUXSIGHSC(22);
LUXSIGHTC(22);
#endif
#ifdef SIGXCPU
LUXSIGHSC(23);
LUXSIGHTC(23);
#endif
#ifdef SIGXFSZ
LUXSIGHSC(24);
LUXSIGHTC(24);
#endif
#ifdef SIGVTALRM
LUXSIGHSC(25);
LUXSIGHTC(25);
#endif
#ifdef SIGPROF
LUXSIGHSC(26);
LUXSIGHTC(26);
#endif
#ifdef SIGWINCH
LUXSIGHSC(27);
LUXSIGHTC(27);
#endif
#ifdef SIGINFO
LUXSIGHSC(28);
LUXSIGHTC(28);
#endif
#ifdef SIGUSR1
LUXSIGHSC(29);
LUXSIGHTC(29);
#endif
#ifdef SIGUSR2
LUXSIGHSC(30);
LUXSIGHTC(30);
#endif

/* tcllux_signal_signals.c: end */
