#
# This is an included makefile that generates targets for things
# that are target-machine-dependent.  For example, for the "install"
# target this file generates a target of the form installx for
# each target macine x, and a target of the form installall that
# will regenerate all of the machines.
#
# Before including this makefile, the calling makefile should define
# the following variables:
#
# MACHINES		List of all known target machines
# PASSVARS		Stuff to put on command lines for sub-makes to
#			pass them relevant variable values.
# no_md_srcs		If defined, there's only one dependency file
#			and one lint file, so separate machine-dependent
#			targets need not be generated.
#
# $Header: /sprite/lib/pmake/RCS/all.mk,v 1.10 90/02/20 11:50:11 douglis Exp $
#

#
# For each general sort of target, one of the variables below holds
# a list of machine-dependent targets, one item for each possible
# machine.
#
CLEANALL	= $(MACHINES:S/^/clean/g)
TIDYALL		= $(MACHINES:S/^/tidy/g)
DEPENDALL	= $(MACHINES:S/^/depend/g)
INSTALLALL	= $(MACHINES:S/^/install/g)
INSTALLHDRSALL	= $(MACHINES:S/^/installhdrs/g)
INSTALLSRCALL	= $(MACHINES:S/^/installsrc/g)
INSTALLDEBUGALL	= $(MACHINES:S/^/installdebug/g)
INSTLINTALL	= $(MACHINES:S/^/instlint/g)
LINTALL		= $(MACHINES:S/^/lint/g)
PROFILEALL	= $(MACHINES:S/^/profile/g)
DEBUGALL	= $(MACHINES:S/^/debug/g)

# Some of these are .NOEXPORT because they're likely to create multiple
# subprocesses and we don't want exponential growth.  Things like 
# "pmake dependall" can be done in parallel, however.

$(MACHINES)		:: .MAKE .NOEXPORT
	$(MAKE) -l $(PASSVARS) TM=$(.TARGET)
$(CLEANALL)		:: .MAKE 
	$(MAKE) -l $(PASSVARS) TM=$(.TARGET:S/^clean//) clean
$(TIDYALL)		:: .MAKE 
	$(MAKE) -l $(PASSVARS) TM=$(.TARGET:S/^tidy//) tidy
$(DEPENDALL)		:: .MAKE 
	$(MAKE) -l $(PASSVARS) TM=$(.TARGET:S/^depend//) depend
$(INSTALLALL)		:: .MAKE .NOEXPORT
	$(MAKE) -l $(PASSVARS) TM=$(.TARGET:S/^install//) install
$(INSTALLHDRSALL)		:: .MAKE .NOEXPORT
	$(MAKE) -l $(PASSVARS) TM=$(.TARGET:S/^installhdrs//) installhdrs
$(INSTALLSRCALL)		:: .MAKE .NOEXPORT
	$(MAKE) -l $(PASSVARS) TM=$(.TARGET:S/^installsrc//) installsrc
$(INSTALLDEBUGALL)		:: .MAKE .NOEXPORT
	$(MAKE) -l $(PASSVARS) TM=$(.TARGET:S/^installdebug//) installdebug
$(INSTLINTALL)		:: .MAKE .NOEXPORT
	$(MAKE) -l $(PASSVARS) TM=$(.TARGET:S/^instlint//) instlint
$(LINTALL)		:: .MAKE 
	$(MAKE) -l $(PASSVARS) TM=$(.TARGET:S/^lint//) lint
$(PROFILEALL)		:: .MAKE .NOEXPORT
	$(MAKE) -l $(PASSVARS) TM=$(.TARGET:S/^profile//) profile
$(DEBUGALL)		:: .MAKE .NOEXPORT
	$(MAKE) -l $(PASSVARS) TM=$(.TARGET:S/^debug//) debug


all			:: $(MACHINES)
cleanall		:: $(CLEANALL)
tidyall			:: $(TIDYALL)
dependall		:: $(DEPENDALL)
installall		:: $(INSTALLALL)
installhdrsall		:: $(INSTALLHDRSALL)
installsrcall		:: $(INSTALLSRCALL)
installdebugall		:: $(INSTALLDEBUGALL)
instlintall		:: $(INSTLINTALL)
lintall			:: $(LINTALL)
profileall		:: $(PROFILEALL)
debugall		:: $(DEBUGALL)
