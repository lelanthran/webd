
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h>
#include <signal.h>
#include <errno.h>

#include "webd.h"
#include "amq.h"
#include "netcode_tcp.h"
#include "netcode_util.h"

static FILE *g_logfile = NULL;
static volatile sig_atomic_t g_exit_flag = 0;

static void sighandler (int n)
{
   if (n==SIGINT)
      g_exit_flag = 1;
}

static enum amq_worker_result_t error_logger (const struct amq_worker_t *self,
                                              void *mesg, size_t mesg_len, void *cdata)
{
   // NOTE: Reusing the mesg_len field as a line number
   (void)self;
   (void)mesg_len;
   (void)cdata;
   struct amq_error_t *errobj = mesg;

   if (!g_logfile)
      g_logfile = stderr;

   // TODO: Move the libx timer library to github, use xtimer.
   fprintf (g_logfile, "%" PRIu64 " : %s", time (NULL), errobj->message);

   amq_error_del (errobj);

   return amq_worker_result_CONTINUE;
}

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

static void print_help_msg (void)
{
   printf ("TODO: Some help text\n");
}


int main (int argc, char **argv)
{
   int retval = EXIT_FAILURE;

   const char *opt_listen_port = cline_getopt (argc, argv, "listen-port", 0);
   const char *opt_log_fname = cline_getopt (argc, argv, "logfile", 0);
   const char *opt_help = cline_getopt (argc, argv, "help", 0);

   size_t listen_port = 8080;
   int listenfd = -1;
   int acceptfd = -1;

   char *remote_addr = NULL;
   uint16_t remote_port = 0;

   // Install a signal handler
   signal (SIGINT, sighandler);

   // Initialise AMQ and the error logging system
   if (opt_log_fname) {
      if (!(g_logfile = fopen (opt_log_fname, "w"))) {
         fprintf (stderr, "Failed to open [%s] for writing: %m\n", opt_log_fname);
         return retval;
      }
   }
   amq_lib_init ();
   amq_consumer_create (AMQ_QUEUE_ERROR, "ErrorLogger", error_logger, "Created by " __FILE__);
   AMQ_ERROR_POST (0, "Started new instance of [%s]\n", argv[0]);

   // Parse the command line arguments
   if (!opt_listen_port) {
      AMQ_ERROR_POST (-1, "Did not specify listening port with '--listen-port', using default\n");
   } else {
      if ((sscanf (opt_listen_port, "%zu", &listen_port))!=1) {
         AMQ_ERROR_POST (-1, "Cannot listen on port [%s]: invalid port number\n", opt_listen_port);
         goto cleanup;
      }
   }

   if (opt_help) {
      print_help_msg ();
      goto cleanup;
   }


   // Some startup information
   AMQ_ERROR_POST (0, "Listening on port [%zu]\n", listen_port);

   // Start the TCP server
   netcode_util_clear_errno ();
   if ((listenfd = netcode_tcp_server (listen_port))<0) {
      AMQ_ERROR_POST (-1, "Failed to establish listening socket on %zu: %m\n", listen_port);
      goto cleanup;
   }

   // Recieve and process all the incoming connections
   while (g_exit_flag==0) {
      free (remote_addr);
      remote_addr = NULL;
      remote_port = 0;
      netcode_util_clear_errno ();
      acceptfd = netcode_tcp_accept (listenfd, 1, &remote_addr, &remote_port);

      if (acceptfd == 0)
         continue;
      if (acceptfd < 0) {
         printf ("%i: %s\n", acceptfd, netcode_util_strerror (netcode_util_errno ()));
      }

      // TODO: Here is where we must post acceptfd, remote_addr and remote_port to the
      // request-worker queues.

      shutdown (acceptfd, SHUT_RDWR);
      close (acceptfd);
   }

   // Done
   retval = EXIT_SUCCESS;

cleanup:

   sleep (1);
   // amq_worker_sigset ("ErrorLogger", AMQ_SIGNAL_TERMINATE);
   amq_lib_destroy ();

   fclose (g_logfile);

   free (remote_addr);

   return retval;
}
