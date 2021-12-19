//---------------------------------------------------------------------------
//  Class:       ncs::Worker
//  File:        ncs/Worker.h
//
//---------------------------------------------------------------------------

#ifndef SERVER__ncs_Worker__H_
#define SERVER__ncs_Worker__H_


// Stl
#include <atomic>
#include <chrono>

// sockets
#include <netinet/in.h>

// lib locar
#include "lcr/Logger.h"
#include "lcr/Cache.hpp"


namespace ncs
{

// This class represents the NCS worker, which process a received request from one client
class Worker
{
   public:
      // The constructor receives as parameters an unique identifier and a socket decriptor.
      // It also receives a reference to the logger to show traces of its operation.
      Worker(unsigned int id, int sockfd, const sockaddr_in& addr, lcr::Cache<std::string, std::string>& cache, lcr::Logger& logger);
      virtual ~Worker();

   public:
      
      // Main worker method: receives a request from a client, process it and send the response back
      void exec();

      // This method requests the worker to cancel the work in progress
      void cancel() const {
         cancelled_ = true;
      }

   public:
      // Getter method for the worker identifier
      unsigned int id() const {
         return id_;
      }

      // Getter method for the error flag
      bool error() const {
         return error_;
      }

   private:
      int async_recv_(char* buffer, int timeout);
      bool process_request_(char* buffer, int bytes_received);
      void send_response_();

   private:
      // The logger reference
      lcr::Logger& logger_;

      // The client socket addr
      sockaddr_in addr_;

      // The client socket decriptor
      int sockfd_;

      // The worker identifier
      unsigned int id_;

      // The worker time delay in milliseconds
      std::chrono::milliseconds delay_;

      // The request reception timeout
      unsigned int timeout_;

      // The cache reference
      lcr::Cache<std::string, std::string>& cache_;

      // The worker internal status
      std::string text_;     // The request text
      std::string digest_;   // The md5 digest of the previous request text
      bool error_;           // Flag that indicates that an error have ocurred
      int ec_;               // When error_, this may contains the related errno code (or zero)

      // Mutable and atomic flag for the external cancel resquest
      mutable std::atomic<bool> cancelled_;
};

} // namespace ncs

#endif // !defined SERVER__ncs_Worker__H_

