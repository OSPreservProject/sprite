#
# This is an included makefile to assist in recompiling collections
# of related programs (the world).  For each standard target "x",
# this file provides a corresponding target "xworld", which will
# remake "x" in each of the directories in the WORLD variable, in
# addition to remaking in the current directory.
#
# Before including this makefile, the calling makefile should define
# the following variables:
#
# WORLD -	List of related directories in which world makes should
#		be invoked.
#
# $Header: /sprite/lib/pmake/RCS/world.mk,v 1.4 90/02/20 11:50:18 douglis Exp $
#

WORLD		?=

others			:: .MAKE
	@for i in $(WORLD); do
		@echo --- pmake in $$i ---
		@(cd $$i; $(MAKE) -l $(PASSVARS) TM=$(TM))
	@done

installothers		:: .MAKE
	@for i in $(WORLD); do
		@echo --- pmake install in $$i ---
		@(cd $$i; $(MAKE) -l $(PASSVARS) TM=$(TM) install)
	@done

cleanothers		: .MAKE
	@for i in $(WORLD); do
		@echo --- pmake clean in ../cc1.$$i ---
		@(cd ../cc1.$$i; $(MAKE) -l $(PASSVARS) TM=$(TM) clean)
	@done

tidyothers		: .MAKE
	@for i in $(WORLD); do
		@echo --- pmake tidy in ../cc1.$$i ---
		@(cd ../cc1.$$i; $(MAKE) -l $(PASSVARS) TM=$(TM) tidy)
	@done

othersall		: .MAKE
	@for i in $(WORLD); do
		@echo --- pmake all in $$i ---
		@(cd $$i; $(MAKE) -l $(PASSVARS) all)
	@done

installothersall	:: .MAKE
	@for i in $(WORLD); do
		@echo --- pmake installall in $$i ---
		@(cd $$i; $(MAKE) -l $(PASSVARS) installall)
	@done

cleanothersall	:: .MAKE
	@for i in $(WORLD); do
		@echo --- pmake cleanall in $$i ---
		@(cd $$i; $(MAKE) -l $(PASSVARS) cleanall)
	@done

tidyothersall	:: .MAKE
	@for i in $(WORLD); do
		@echo --- pmake tidyall in $$i ---
		@(cd $$i; $(MAKE) -l $(PASSVARS) tidyall)
	@done

world			:: default others
installworld		:: install installothers
cleanworld		:: clean cleanothers
tidyworld		:: tidy tidyothers
worldall		:: all othersall
installworldall		:: installall installothersall
cleanworldall		:: cleanall cleanothersall
tidyworldall		:: tidyall tidyothersall
