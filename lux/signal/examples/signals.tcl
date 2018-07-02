
#! /bin/sh
# \
exec tclsh${TCL} "$0" "$@"

#
# TclLuX Signal
#
# Signals Example
#
#
# Stuart Cassoff
# Spring 2018
#

[namespace eval ::TclLuX_Signal {

package require tcllux::signal
namespace import ::tcllux::lux

proc print_signals {} {
	variable signals
	variable fmt
	puts Signals:
	puts [format $fmt Num Signal Descr]
	set i -1; dict for {sig descr} $signals {
		puts [format $fmt [incr i] $sig $descr]
	}
}

proc sigmund {chan send} {
	if {$send eq "timestamp"} {
		binary scan [read $chan] ii s ns
		puts [format {%s.%s} $s $ns]
	} elseif {$send eq "signal"} {
		variable fmt
		variable signals
		binary scan [read $chan] c s
		set sn [lindex [dict keys $signals] $s]
		puts [format $fmt $s $sn [dict get $signals $sn]]
	} else {
		explode
	}
}

proc go {} {
	set sigs [dict create hup 1100 alrm 2200 usr1 4000 prof 5000]
	variable done 0
	variable fmt {%3s %-6s %s}
	variable signals [lux signal signals]
	print_signals
	lassign [chan pipe] r w
	chan configure $r -blocking 0
	chan configure $w -blocking 0
	foreach send [list timestamp signal] {
		fileevent $r readable [list [namespace current]::sigmund $r $send]
		set ai [after 8000 [list set [namespace current]::done 1]] ;#cheap
		puts Trying\ with\ $send:\ [dict keys $sigs]
		dict for {sig time} $sigs {
			lux signal set $sig $w $send
			after $time [list ::tcllux::lux signal send $sig [pid]]
		}
		vwait [namespace current]::done
		after cancel $ai
	}
	close $w
	close $r
}

namespace current}]::go

# EOF
