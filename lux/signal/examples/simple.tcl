#! /bin/sh
# \
exec tclsh${TCL} "$0" "$@"

#
# TclLuX Signal
#
# Simple signal example
#
#
# Stuart Cassoff
# Summer 2019
#

[namespace eval ::TclLuX_Signal {

package require tcllux::signal
namespace import ::tcllux::lux

proc sigreceiver {chan send} {
	if {$send eq "timestamp"} {
		binary scan [read $chan 16] mm s ns
		puts "Got signal at $s.$ns"
	} elseif {$send eq "signal"} {
		binary scan [read $chan 1] c s
		puts "Got signal number $s"
	}
}

proc sigsetup {sig} {
	set send signal
	#set send timestamp
	lassign [chan pipe] r w
	chan configure $r -translation binary -blocking 0
	chan configure $w -translation binary -blocking 0
	chan event $r readable [list [namespace current]::sigreceiver $r $send]
	lux signal set $sig $w $send
}

proc go {} {
	sigsetup hup
	after 800  [list ::tcllux::lux signal send hup [pid]]
	after 1600 [list set [namespace current]::done 1]
	puts waiting
	vwait [namespace current]::done
	puts done
}

namespace current}]::go

# EOF
