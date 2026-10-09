#include "app/depsh/cmds/help.h"
#include "app/depsh/depshCommon.h"
#include "sys/syscall.h"

void cmd_help(char *args)
{
  if (!streq(args, ""))
  {
    write_string("\nhelp: option ");
    write_string(args);
    write_string(" does not exist\n");
    return;
  }

  write_string("help:"
               " Lists all available commands"
               " with a short description of each.\n"
               "echo [arg ...] {OPTIONAL >>} {OPTIONAL path}: Prints a given string. If the flag"
               ">> is given, then write the string to the given path anyway\n"
               "ticks: Prints the amount of timer ticks accumulated"
               " since boot.\n"
               "cat [path ...]: Reads the bytes of a given file.\n"
               "ls [path ...]: Lists the subdirectories/files of a directory.\n"
               "cd [path ...]: Changes the current directory into the entered path.\n"
               "stat [-t, -s, -l] [path ...]: Lists the properties of the given file/dir.\n"
               "mkdir [-p] [path ...]: Makes a new directory with the given path. "
               "Do note that in order to create the whole path, and not the leaf directory,"
               "you must use the flag -p.\n"
               "rm [-r] [path ...]: Removes the file at the given path. If the flag -r is passed,"
               "it recursively removes a directory.\n"
               "clear: Clears the screen\n"
               "touch [path ...]: Makes a new file at the given path (does not have the -p properties of mkdir)\n"
               "date: Prints the current date, in format [m/d/y h:m:s.ms]\n"
               "mv [old_path ...] [new_path ...]: Moves the given old_path to a given new path. Also serves"
               "as a rename function.\n"
               "cp [-r] [old_path ...] [new_path ...]: It's like mv, but it copies instead of moves."
               "Must provide the flag -r in order to copy directories, no matter whether or not"
               "it contains subdirectories.\n");
}
