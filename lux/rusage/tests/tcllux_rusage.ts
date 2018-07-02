apply {ts {
    $ts interp		tclsh8.6
    $ts dir		/home/stu/tcl/lux/packages/TclLuX_rusage-0.1/tests
    $ts env		TCLLIBPATH=../b
    $ts verbose		
    $ts debug		0
    $ts singleproc	0
    $ts match		
    $ts skip		
    $ts matchfiles	
    $ts skipfiles	
    $ts tmpdir		
    $ts add [testpackage new /home/stu/tcl/lux/packages/TclLuX_rusage-0.1/tests/tcllux_rusage.test -name tcllux_rusage.test -scan 1]
    return $ts
}} [testset new "TclLuX Rusage"]
