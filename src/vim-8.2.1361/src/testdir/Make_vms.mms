#
# Makefile to run all tests for Vim on VMS
#
# Authors:	Zoltan Arpadffy, <arpadffy@polarhome.com>
#		Sandor Kopanyi,  <sandor.kopanyi@mailbox.hu>
#
# Last change:  2020 Jul 03
#
# This has been tested on VMS 6.2 to 8.3 on DEC Alpha, VAX and IA64.
# Edit the lines in the Configuration section below to select.
#
# Execute with:
#		mms/descrip=Make_vms.mms
# Cleanup with:
#		mms/descrip=Make_vms.mms clean
#
# Make files are MMK compatible.
#
# NOTE: You can run this script just in X/Window environment. It will
# create a new terminals, therefore you have to set up your DISPLAY
# logical. More info in VMS documentation or with: help set disp.
#
#######################################################################
# Configuration section.
#######################################################################

# Uncomment if you want tests in GUI mode.  Terminal mode is default.
# WANT_GUI  = YES

# Comment out if you want to run Unix specific tests as well, but please
# be aware, that on OpenVMS will fail, because of cat, rm, etc commands
# and directory handling.
# WANT_UNIX = YES

# Comment out if you have gzip on your system
# HAVE_GZIP = YES

# Comment out if you have GNU compatible diff on your system
# HAVE_GDIFF = YES

# Comment out if you have ICONV support
# HAVE_ICONV = YES

# Comment out if you have LUA support
# HAVE_LUA = YES

# Comment out if you have PYTHON support
# HAVE_PYTHON = YES

#######################################################################
# End of configuration section.
#
# Please, do not change anything below without programming experience.
#######################################################################

VIMPROG = <->vim.exe

.SUFFIXES : .out .in

SCRIPT = test1.out test49.out test77a.out

.IFDEF WANT_GUI
GUI_OPTION = -g
.ENDIF

.IFDEF WANT_UNIX
SCRIPT_UNIX = test49.out
.ENDIF

.in.out :
	-@ !clean up before doing the test
	-@ if "''F$SEARCH("test.out.*")'" .NES. "" then delete/noconfirm/nolog test.out.*
	-@ if "''F$SEARCH("$*.out.*")'"   .NES. "" then delete/noconfirm/nolog $*.out.*
	-@ ! define TMP if not set - some tests use it
	-@ if "''F$TRNLNM("TMP")'" .EQS. "" then define/nolog TMP []
	-@ write sys$output " "
	-@ write sys$output "-----------------------------------------------"
	-@ write sys$output "                "$*" "
	-@ write sys$output "-----------------------------------------------"
	-@ !run the test
	-@ create/term/wait/nodetach mcr $(VIMPROG) $(GUI_OPTION) -u vms.vim --noplugin -s dotest.in $*.in
	-@ !analyse the result
	-@ directory /size/date test.out
	-@ if "''F$SEARCH("test.out.*")'" .NES. "" then rename/nolog test.out $*.out 
	-@ if "''F$SEARCH("$*.out.*")'"   .NES. "" then differences /par $*.out $*.ok;
	-@ !clean up after the test
	-@ if "''F$SEARCH("Xdotest.*")'"  .NES. "" then delete/noconfirm/nolog Xdotest.*.*
	-@ if "''F$SEARCH("Xtest.*")'"    .NES. "" then delete/noconfirm/nolog Xtest.*.*

all : clean nolog $(START_WITH) $(SCRIPT) $(SCRIPT_UNIX) nolog
	-@ write sys$output " "
	-@ write sys$output "-----------------------------------------------"
	-@ write sys$output "                All done"
	-@ write sys$output "-----------------------------------------------"
	-@ deassign sys$output
	-@ delete/noconfirm/nolog x*.*.*
	-@ type test.log

nolog :
	-@ define sys$output test.log
	-@ write sys$output "-----------------------------------------------"
	-@ write sys$output "           Standard VIM test cases"
	-@ write sys$output "-----------------------------------------------"
	-@ write sys$output " OpenVMS version: ''F$GETSYI("VERSION")'"
	-@ write sys$output " Vim version:"
	-@ mcr $(VIMPROG) --version
	-@ write sys$output " Test date:"
	-@ show time
	-@ write sys$output "-----------------------------------------------"
	-@ write sys$output "                Test results:"
	-@ write sys$output "-----------------------------------------------"
	-@ write sys$output "MAKE_VMS.MMS options:"
	-@ write sys$output "   WANT_GUI   = ""$(WANT_GUI)"" "
	-@ write sys$output "   WANT_UNIX  = ""$(WANT_UNIX)"" "
	-@ write sys$output "   HAVE_GZIP  = ""$(HAVE_GZIP)"" "
	-@ write sys$output "   HAVE_GDIFF = ""$(HAVE_GDIFF)"" "
	-@ write sys$output "   HAVE_ICONV = ""$(HAVE_ICONV)"" "
	-@ write sys$output "   HAVE_LUA   = ""$(HAVE_LUA)"" "
	-@ write sys$output "   HAVE_PYTHON= ""$(HAVE_PYTHON)"" "
	-@ write sys$output "Default vimrc file is VMS.VIM:"
	-@ write sys$output "-----------------------------------------------"
	-@ type VMS.VIM

clean :
	-@ if "''F$SEARCH("*.out")'"        .NES. "" then delete/noconfirm/nolog *.out.*
	-@ if "''F$SEARCH("test.log")'"     .NES. "" then delete/noconfirm/nolog test.log.*
	-@ if "''F$SEARCH("test.ok")'"      .NES. "" then delete/noconfirm/nolog test.ok.*
	-@ if "''F$SEARCH("Xdotest.*")'"    .NES. "" then delete/noconfirm/nolog Xdotest.*.*
	-@ if "''F$SEARCH("Xtest*.*")'"     .NES. "" then delete/noconfirm/nolog Xtest*.*.*
	-@ if "''F$SEARCH("XX*.*")'"        .NES. "" then delete/noconfirm/nolog XX*.*.*
	-@ if "''F$SEARCH("_un_*.*")'"      .NES. "" then delete/noconfirm/nolog _un_*.*.*
	-@ if "''F$SEARCH("*.*_sw*")'"      .NES. "" then delete/noconfirm/nolog *.*_sw*.*
	-@ if "''F$SEARCH("*.failed")'"     .NES. "" then delete/noconfirm/nolog *.failed.*
	-@ if "''F$SEARCH("*.rej")'"        .NES. "" then delete/noconfirm/nolog *.rej.*
	-@ if "''F$SEARCH("tiny.vim")'"     .NES. "" then delete/noconfirm/nolog tiny.vim.*
	-@ if "''F$SEARCH("small.vim")'"    .NES. "" then delete/noconfirm/nolog small.vim.*
	-@ if "''F$SEARCH("mbyte.vim")'"    .NES. "" then delete/noconfirm/nolog mbyte.vim.*
	-@ if "''F$SEARCH("mzscheme.vim")'" .NES. "" then delete/noconfirm/nolog mzscheme.vim.*
	-@ if "''F$SEARCH("viminfo.*")'"    .NES. "" then delete/noconfirm/nolog viminfo.*.*
