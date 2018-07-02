#! /bin/sh
# \
exec tclsh${TCL} "$0" "$@"

#
# TclLuX
#
# Example using itimer and signal.
#
#
# Stuart Cassoff
# Summer 2018
#

[namespace eval uxl {

proc sigmund {chan} {
	read $chan
	incr ::count
}

proc go {} {
	package require tcllux::itimer
	package require tcllux::signal
	namespace import ::tcllux::lux

	lassign [chan pipe] r w
	chan configure $r -blocking 0
	chan configure $w -blocking 0
	fileevent $r readable [list [namespace current]::sigmund $r]

	lux signal set alrm $w
	puts signal:\ [lux signal set alrm]

	set ::count 0

	set start [clock seconds]

	after 10001 [list set ::time 1]

	lux itimer set real 0.01 0.01
	puts itimer:\ [lux itimer get real]

	vwait ::time

	set end [clock seconds]

	puts $::count\ signals\ in\ [expr {$end - $start}]\ seconds
}

namespace current}]::go

# EOF
