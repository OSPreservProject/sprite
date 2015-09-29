/* Commands special for KGDB.
ding!
*/

#include "defs.h"
#include "symtab.h"
/*#include "param.h"*/
#include "frame.h"
#include "inferior.h"
#include "environ.h"
#include "value.h"
#include "target.h"

#include <stdio.h>
#include <signal.h>

extern int remote_debugging;

void
attach_command (args, from_tty)
     char *args;
     int from_tty;
{
  char *exec_file;
  int pid;
  int remote = 0;

  dont_repeat();

  if (!args)
    error_no_arg ("hostname to attach");

  while (*args == ' ' || *args == '\t') args++;


  exec_file = (char *) get_exec_file (1);


  printf ("Attaching remote machine %s\n",args);
  remote_open (args, from_tty);
/* This is already done in remote_open. */
/*  start_remote ();                    */
}


static void
detach_command (args, from_tty)
     char *args;
     int from_tty;
{

  if (!remote_debugging)
    error ("Not currently debugging a machine\n");
  if (from_tty) {
      if (args) 
	  printf ("Detaching with execution restarted.\n");
      else
	  printf ("Detaching.\n");
      fflush (stdout);
    }
  remote_detach(args, from_tty);
  inferior_pid = 0;
}


/* remote the remote machine. */
void
reboot_command (args, from_tty)
     char *args;
     int from_tty;
{
  dont_repeat();
  if (!remote_debugging || !inferior_pid) {
    error ("Not currently debugging a machine\n");
  }	
  remote_reboot(args);
}
static void
pid_command (args, from_tty)
     char *args;
     int from_tty;
{
  int	pid;

  if (!args)
    error_no_arg ("Must specifiy a process id");

  pid = value_as_long (evaluate_expression (parse_expression (args)));
  if (from_tty) {
      printf ("Attaching process entry: 0x%x\n", pid);
  }
  target_attach(pid, from_tty);
}

static void
version_command (args, from_tty)
     char *args;
     int from_tty;
{
/* Not implemented yet.
  char	*version, *remote_version();
  version = remote_version();
  if (from_tty && version) 
     printf("%s\n",version);
*/
}

static void
debug_command (args, from_tty)
     char *args;
     int from_tty;
{
  int result;
  char *dbgString;
  if (!args)
    error_no_arg ("hostname");
  dbgString = (char *)xmalloc(8+strlen(args));
  sprintf(dbgString, "kmsg -d %s", args);
  result = system(dbgString);
  if (result == 0)
      attach_command(args,from_tty);
}

/* Set the start of the text segment  */
static void
text_command (args, from_tty)
     char *args;
     int from_tty;
{
  char *exec_file;
  extern CORE_ADDR text_start;
  extern CORE_ADDR text_end;
  extern CORE_ADDR exec_data_start;
  extern CORE_ADDR exec_data_end;
  CORE_ADDR new_start;


  dont_repeat();

  if (!args)
    error_no_arg ("Text start address");

  while (*args == ' ' || *args == '\t') args++;

  new_start = value_as_long (evaluate_expression (parse_expression (args)));


  exec_file = (char *) get_exec_file (1); /* Make sure we have an exec file */


  text_end =  (text_end - text_start) + new_start;
  text_start = new_start;
  exec_data_end =  (exec_data_end - exec_data_start) + text_end;
  exec_data_start =  text_end;

}


_initialize_kgdbcmd ()
{
  add_com ("reboot", class_obscure, reboot_command,
	   "Reboot the machine being debugged.");
  add_com ("pid", class_obscure, pid_command, 
	   "Set the process id to be debugged.");
  add_com ("version", class_obscure, version_command, 
	   "Print the version string of the kernel being debugged.");
  add_com ("debug", class_obscure, debug_command, 
	   "Force the specified host into the debugger and attach it.");
  add_com ("text", class_obscure, text_command, 
	   "Specify the starting address of the text segment.");
 add_com ("attach", class_run, attach_command,
	   "Attach to a machine in to debugger.\n\
Before using \"attach\", you must use the \"exec-file\" command\n\
to specify the program running in the process,\n\
and the \"symbol-file\" command to load its symbol table.");
  add_com ("detach", class_run, detach_command,
	   "Detach the machine previously attached.\n\
If a non zero argument is specified, the machine execution is not resumed.");
}

