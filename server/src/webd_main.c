
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

#include "webd.h"
#include "netcode_tcp.h"

/* *****************************************************************
 * A more robust command-line handling function than I normally use.
 * There are only two functions, so this is suitable for copying and
 * pasting into the file containing main().
 *
 * =======================================================================
 * Get an option from the command-line:
 *    const char *cline_getopt (int          argc,
 *                              char       **argv,
 *                              const char  *longopt,
 *                              char         shortopt);
 *
 * argc:       number of arguments
 * argv:       array of arguments
 * longopt:    string containing name of option. If NULL, short-opt is
 *             used to find the option.
 * shortopt:   character of the option to search for. If zero, longopt is
 *             used to find the option.
 *
 * RETURNS: If option is not found, then NULL is returned. If option is
 *          found then the string that is returned:
 *          1. Will be empty if option did not have an argument
 *             such as "--option=" or "--option".
 *          2. Will contain the value of the argument if the option had an
 *             argument, such as "--option=value", or "-o value", or
 *             "-ovalue".
 *
 *          For short options, the caller must determine whether to use
 *          the returned string's value or not. If, for example, the
 *          option "-c" does not have arguments and the user entered
 *          "-cab" then the caller must only check for nullness in the
 *          return value.
 *
 *          If the caller expects "-c" to have arguments, then the
 *          returned string for "-cab" will contain "ab".
 *
 *          See EXAMPLES below for clarification.
 *
 * EXAMPLES:
 *
 * 1. Get a long option using cline_getopt()
 *    --long-option           Returns empty string ""
 *    --long-option=          Returns empty string ""
 *    --long-option=value     Returns const string "value"
 *
 * 2. Get a short option using cline_getopt()
 *    -a -b -c       Returns non-NULL for a, b and c
 *    -abc           Same as above
 *    -ofoo          Returns "foo" for o
 *    -o foo         Same as above
 *    -abco foo      Returns non-NULL for a, b and c, AND returns foo for o
 *    -abcofoo       Same as above
 *
 * 3. When the same long-option and short-option is specified the
 *    long-option takes precedence.
 *
 * 4. Options processing ends with "--". Any arguments after a "--" is
 *    encountered must be processed by the caller.
 */
static const char *cline_getopt (int argc, char **argv,
                                 const char *longopt,
                                 char shortopt)
{
   for (int i=1; i<argc; i++) {
      if (argv[i][0]!='-')
         continue;

      if ((memcmp (argv[i], "--", 3))==0)
         return NULL;

      char *value = NULL;

      if (argv[i][1]=='-' && longopt) {
         char *name = &argv[i][2];
         if ((memcmp (name, longopt, strlen (longopt)))==0) {
            argv[i][0] = 0;
            value = strchr (name, '=');
            if (!value)
               return "";
            *value++ = 0;
            return value;
         }
      }

      if (!shortopt || argv[i][1]=='-')
         continue;

      for (size_t j=1; argv[i][j]; j++) {
         if (argv[i][j] == shortopt) {
            memmove (&argv[i][j], &argv[i][j+1], strlen (&argv[i][j+1])+1);
            if (argv[i][j] == 0) {
               return argv[i+1] ? argv[i+1] : "";
            } else {
               return &argv[i][j];
            }
         }
      }
   }
   return NULL;
}

static print_help_msg (void)
{
   printf ("TODO: Some help text\n");
}


int main (int argc, char **argv)
{
   const char *opt_listen_port = cline_getopt (argc, argv, "listen-port", 0);
   const char *opt_log_fname = cline_getopt (argc, argv, "logfile", 0);
   const char *opt_help = cline_getopt (argc, argv, "help", 0);

   uint64_t listen_port = 8080;
   FILE *logfile = stdout;

   if (!opt_listen_port) {
      fprintf (stderr, "Did not specify port to listen on with '--listen-port', using default\n");
   } else {
      if ((sscanf (opt_listen_port, "%" PRIu64, &listen_port))!=1) {
         fprintf (stderr, "Cannot listen on port [%s]: invalid port number\n", opt_listen_port);
         return 1;
      }
   }

   if (opt_help) {
      print_help_msg ();
      return EXIT_FAILURE;
   }

   // TODO: Log the program parameters here.
   printf ("TODO: create the web daemon\n");
   return EXIT_SUCCESS;
}
