//---------------------------------------------------------------------------
//  Class:       ncs::Server
//  File:        ncs/Server.h
//
//---------------------------------------------------------------------------

#ifndef SERVER__ncs_Server__H_
#define SERVER__ncs_Server__H_


// Stl
#include <list>
#include <future>
#include <memory>

// Components
#include "Worker.h"


namespace ncs
{

// This class represents the NCS server, which listens for client requests and creates a task in a thread for each of them. 
class Server
{
   public:
      // The constructor receives as parameters the port number where it listens for requests,
      // the maximum size of the cache where it stores the results and a timeout for the automatic cache discard functionality.
      // It also receives a reference to the logger to show traces of its operation.
      Server(unsigned int port, unsigned int cache_capacity, unsigned int cache_timeout, lcr::Logger& logger);
      virtual ~Server();

   public:
      // This method executes the server main operation
      int run();

      // This method requests the server to finish the request tasks in an orderly manner
      void finish() const {
         finish_ = true;
      }

      // This method requests the server to cancel the request tasks in an orderly manner
      void cancel() const {
         cancel_ = true;
      }

      // This method requests the server to print the cache contents
      void clearCache() const {
         clear_ = true;
      }

      // This method requests the server to clear the cache contents
      void printCache() const {
         print_ = true;
      }

      // This method requests the server to print the internal statistics
      void printStatistics() const;

   private:
      // Private method that update the list of active tasks according to its status 
      void update_tasks_();
      // Private method that waits until all pending task have finished
      void wait_for_tasks_();
      // Private method that cance all pending task to finish as soon as possible
      void cancel_tasks_();

   private: // Non-copyable.
      Server(const Server&) = delete;
      Server& operator=(const Server&) = delete;

   private:
      // The logger reference
      lcr::Logger& logger_;

      // Mutable flags for print, cancel and finish request
      mutable bool finish_;
      mutable bool cancel_;
      mutable bool clear_;
      mutable bool print_;

      // The server port number
      int port_;

      // The server socket decriptor
      int sockfd_;

      // The server cache, parametrized as: KEY(text) => VALUE(digest)
      lcr::Cache<std::string, std::string> cache_;

   private: // Utilities to keep track of threads status
      // This is the sequence of unique identifiers for workers
      unsigned int sequence_;

      // This private type represent an asynchronous task for a request
      struct Task
      {
         std::shared_ptr<Worker> worker; // The worker that process the request
         std::future<void> future;       // The way the server can keep track of the progress and result from the worker thread
      };

      // This is the list of active tasks
      std::list<Task> tasks_;

   private: // Utilities for statistics purposes
      unsigned long long workers_;   // Total number of created workers
      unsigned long long errors_;    // The number of errors reported by workers 
      unsigned long long unattended_requests_;    // The number of unattended request
};

} // namespace ncs

#endif // !defined SERVER__ncs_Server__H_

