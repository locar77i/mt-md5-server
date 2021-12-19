//------------------------------------------------------------------------------------------
//  File:        main.cpp
//
//------------------------------------------------------------------------------------------

// Stl
#include <memory>
#include <csignal>
#include <string.h>

// Components
#include "ncs/Server.h"

// lib locar
#include "lcr/StdLogger.h"
#include "lcr/CommandLine.hpp"



// Definitions /////////////////////////////////////////////////////////////////////
struct Arguments // The Arguments type stores the parameters from the command line after parsing
{
   int log_level{};      // The log level serves as threshold to decide which logger traces are displayed on the screen and which are not. Posible values: [1-6]
   int port{};           // The server port number. Posible values: [1024-65535]
   int cache_capacity{}; // The max size for the internal cache
   int cache_timeout{};  // Timeout used to automatically discard entries from the cache based on their temporal age. When zero, the automatic discard is disabled.
};


// Prototypes //////////////////////////////////////////////////////////////////////
void show_usage(); // Function that shows the program usage
void check(Arguments& args); // Function that checks the arguments validity
void signal_handler(int signum); // Function that handles all required signals
std::string decode_return_code(int rc); // Function that decodes the program return code


// Static constants ////////////////////////////////////////////////////////////////
static const int C_S_DEFAULT_PORT = 3456;
static const int C_S_DEFAULT_CACHE_CAPACITY = 10;
static const int C_S_DEFAULT_CACHE_TIMEOUT = 600;

// Static objects /////////////////////////////////////////////////////////////////
static std::shared_ptr<ncs::Server> s_server_ptr;
static int s_log_level = 3;



int main(int argc, const char* argv[])
{
   // Look for the sow_help parameter
   if(argc==2 && (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help")) {
      show_usage();
      return 0;
   } // else ...

   // Specify the input parameters and proceed to parse the command line
   auto args = lcr::CommandLine<Arguments>::Parser({
      {"-l", &Arguments::log_level},
      {"-p", &Arguments::port},
      {"-C", &Arguments::cache_capacity},
      {"-t", &Arguments::cache_timeout}
   })->parse(argc, argv);

   // Check the arguments validity
   check(args);

   // Get the logger reference after the command line argument has been applied
   auto & logger = lcr::StdLogger::instance(s_log_level);

   logger.trace(LOG_LEVEL_1, "[MAIN] Started!");

   // Instantiate the NCS server (NeCat Server)
   s_server_ptr.reset(new ncs::Server(args.port, args.cache_capacity, args.cache_timeout, logger));

   // Register our handler for the required signals 
   signal(SIGUSR1, signal_handler);
   signal(SIGUSR2, signal_handler);
   signal(SIGINT, signal_handler);
   signal(SIGTERM, signal_handler);

   // This will contain the program return code
   int rc;
   try { // Runs the server main and check for errors
      rc = s_server_ptr->run();
   }
   catch(const lcr::RuntimeError& ex) {
      logger.error(LOG_ERROR, "[MAIN] %s: %s", ex.what(), strerror(ex.ec()));
      rc = -1;
   }
   catch(const lcr::LogicError & ex) {
      logger.error(LOG_ERROR, "[MAIN] %s", ex.what());
      rc = -2;
   }
   catch(const std::bad_alloc & ex) {
      logger.error(LOG_ERROR, "[MAIN] Memory error: %s", ex.what());
      rc = -3;
   }
   catch(...) {
      logger.error(LOG_ERROR, "[MAIN] Unknow error - The program must be more robust [errno:%d]", (int)errno);
      perror("ERROR:");
      rc = -4;
   }

   // Destroy the NCS server
   s_server_ptr.reset();

   logger.trace(LOG_LEVEL_1, "[MAIN] %s!", decode_return_code(rc).c_str());
   return rc;
}



// Function that shows the program usage
void show_usage()
{
   std::cout << "---- Command line -----------------------------------------------------------------------------------------------------" << std::endl << std::endl;
   std::cout << " -h      Program help" << std::endl;
   std::cout << " --help  Show details of the program usage" << std::endl << std::endl;
   std::cout << " -l      Logger level:" << std::endl;
   std::cout << "         The log level serves as threshold to decide which logger traces are displayed on the screen and which are not." << std::endl;
   std::cout << "         Posible values: [1-6]" << std::endl;
   std::cout << "         Default value: " << s_log_level << std::endl << std::endl;
   std::cout << " -p      Port number" << std::endl;
   std::cout << "         The server port number." << std::endl;
   std::cout << "         Posible values: [1024-65535]" << std::endl;
   std::cout << "         Default value: " << C_S_DEFAULT_PORT << std::endl << std::endl;
   std::cout << " -C      Cache capacity" << std::endl;
   std::cout << "         The max size for the internal cache." << std::endl;
   std::cout << "         Default value: " << C_S_DEFAULT_CACHE_CAPACITY << " entries" << std::endl << std::endl;
   std::cout << " -t      Cache timeout" << std::endl;
   std::cout << "         Timeout used to automatically discard entries from the cache based on their temporal age." << std::endl;
   std::cout << "         When zero, the automatic discard is disabled." << std::endl;
   std::cout << "         Default value: " << C_S_DEFAULT_CACHE_TIMEOUT << " seconds" << std::endl << std::endl;
   std::cout << "Examples:" << std::endl;
   std::cout << "         server -h" << std::endl;
   std::cout << "         server --help" << std::endl;
   std::cout << "         server -p 3456 -C 10 -l 3 -t 120" << std::endl;
   std::cout << "-----------------------------------------------------------------------------------------------------------------------" << std::endl;
}


// Function that checks the arguments validity
void check(Arguments& args)
{
   // Get the logger reference with the default logger level
   auto & logger = lcr::StdLogger::instance(s_log_level);
   if(args.log_level<1 || args.log_level>6) {
      logger.error(LOG_WARNING, "[MAIN] Invalid log level (%d). Setting %d as default", args.log_level, s_log_level);
      args.log_level = s_log_level;
   }
   // Apply the command line level to the logger
   s_log_level = args.log_level;
   lcr::StdLogger::instance(s_log_level);
   // Check the port number argument
   if(args.port<0) {
      logger.error(LOG_WARNING, "[MAIN] Invalid port number (%d). Setting %d as default", args.port, C_S_DEFAULT_PORT);
      args.port = C_S_DEFAULT_PORT;
   }
   else if(args.port<=1023) {
      logger.error(LOG_WARNING, "[MAIN] Port number %u is reserved. Setting %d as default", args.port, C_S_DEFAULT_PORT);
      args.port = C_S_DEFAULT_PORT;
   }
   else if(args.port>65535) {
      logger.error(LOG_WARNING, "[MAIN] Port number %u is not in the range 1023-65535. Setting %d as default", args.port, C_S_DEFAULT_PORT);
      args.port = C_S_DEFAULT_PORT;
   }
   // Check the cache capacity argument
   if(args.cache_capacity<0) {
      logger.error(LOG_WARNING, "[MAIN] Invalid cache capacity (%d). Setting %d as default", args.cache_capacity, C_S_DEFAULT_CACHE_CAPACITY);
      args.cache_capacity = C_S_DEFAULT_CACHE_CAPACITY;
   }
   // Check the cache timeout argument
   if(args.cache_timeout<0) {
      logger.error(LOG_WARNING, "[MAIN] Invalid cache timeout (%d). Setting %d as default", args.cache_timeout, C_S_DEFAULT_CACHE_TIMEOUT);
      args.cache_timeout = C_S_DEFAULT_CACHE_TIMEOUT;
   }
   logger.trace(LOG_LEVEL_1, "[MAIN]---- Execution parameters ---------------------------------------------------");
   logger.trace(LOG_LEVEL_1, "[MAIN] Trace level   : %d", args.log_level);
   logger.trace(LOG_LEVEL_1, "[MAIN] Port number   : %d", args.port);
   logger.trace(LOG_LEVEL_1, "[MAIN] Cache capacity: %d entries", args.cache_capacity);
   logger.trace(LOG_LEVEL_1, "[MAIN] Cache timeout : %d seconds", args.cache_timeout);
   logger.trace(LOG_LEVEL_1, "[MAIN]-----------------------------------------------------------------------------");
}


// Function that handles all required signals
void signal_handler(int signum)
{
   auto & logger = lcr::StdLogger::instance(s_log_level);
   switch(signum)
   {
      case SIGUSR1:
         logger.trace(LOG_LEVEL_2, "[MAIN] Received SIGUSR1 [%d]", signum);
         s_server_ptr->clearCache();
         break;
      case SIGUSR2:
         logger.trace(LOG_LEVEL_2, "[MAIN] Received SIGUSR2 [%d]", signum);
         s_server_ptr->printCache();
         break;
      case SIGTERM:
         logger.trace(LOG_LEVEL_2, "[MAIN] Received SIGTERM [%d]", signum);
         s_server_ptr->finish();
         break;
      case SIGINT:
         logger.trace(LOG_LEVEL_2, "[MAIN] Received SIGINT [%d]", signum);
         s_server_ptr->cancel();
         break;
   }
}


// Function that decodes the program return code
std::string decode_return_code(int rc)
{
   switch(rc)
   {
      case 0:
         return "Finished";
      case 1:
         return "Cancelled";
   }
   return "Aborted";
}

