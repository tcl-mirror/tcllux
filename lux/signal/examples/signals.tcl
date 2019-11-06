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
# Summer 2019
#

[namespace eval ::TclLuX_Signal {

package require tcllux::signal
namespace import ::tcllux::lux

proc print_signal {sig} {
	puts [format "%-6s %2s %s" {*}$sig]
}

proc dump_signal {sig} {
	set ss [lux signal set [lindex $sig 0]]
	if {[dict size $ss] == 6} {
		puts [format "%s %-6s %s %2s %s %s %s %2x %s %-7s %s %s  %s" {*}$ss [lindex $sig 2]]
	} else {
		puts [format "%s %-6s %s %2s %s %s %s %2x %s %-7s  %s" {*}$ss [lindex $sig 2]]
	}
}

proc sigmund {chan send} {
	if {$send eq "timestamp"} {
		binary scan [read $chan 16] mm s ns
		puts $s.$ns
	} elseif {$send eq "signal"} {
		variable signals
		binary scan [read $chan 1] c s
		if {[set i [lsearch -index 1 $signals $s]] != -1} {
			print_signal [lindex $signals $i]
		}
	} else {
		explode
	}
}

proc go {} {
	set sigs [dict create hup 1100 alrm 2200 usr1 4000 prof 5000]
	variable done 0
	variable signals [lux signal signals]
	foreach sig $signals {
		dump_signal $sig
	}
	lassign [chan pipe] r w
	chan configure $r -translation binary -blocking 0
	chan configure $w -translation binary -blocking 0
	foreach send [list signal timestamp] {
		chan event $r readable [list [namespace current]::sigmund $r $send]
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
