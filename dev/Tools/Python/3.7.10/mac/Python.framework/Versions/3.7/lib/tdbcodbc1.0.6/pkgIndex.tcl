# Package index file for tdbc::odbc

if {[catch {package require Tcl 8.6}]} {
    return
}
package ifneeded tdbc::odbc 1.0.6 \
    "[list source [file join $dir tdbcodbc.tcl]]\;\
    [list load [file join $dir libtdbcodbc1.0.6.dylib] tdbcodbc]"
