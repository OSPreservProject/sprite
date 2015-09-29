
void initialize_all_files () {

	_initialize_blockframe();
	_initialize_breakpoint();
	_initialize_coffread();
	_initialize_command();
	_initialize_core();
	_initialize_dbxread();
	_initialize_exec();
	_initialize_expread();
	_initialize_infcmd();
	_initialize_inflow();
	_initialize_infrun();
	_initialize_inftarg();
	_initialize_printcmd();
	_initialize_remote();
	_initialize_signame();
	_initialize_source();
	_initialize_stack();
	_initialize_symfile();
	_initialize_symmisc();
	_initialize_symtab();
	_initialize_targets();
	_initialize_utils();
	_initialize_valprint();
	_initialize_values();
#ifdef KGDB
	_initialize_kgdbcmd ();
#endif
}
