//------------------------------------------------------------------------------------------
//  Class:       ncs::Worker
//  File:        ncs/Worker.cpp
//
//------------------------------------------------------------------------------------------
#include "Worker.h"

// Stl
#include <thread>
#include <sstream>
#include <algorithm>

// sockets
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>

// lib locar
#include "lcr/md5.h"
#include "lcr/String.hpp"
#include "lcr/Exceptions.hpp"



namespace ncs
{

Worker::Worker(unsigned int id, int sockfd, const sockaddr_in& addr, lcr::Cache<std::string, std::string>& cache, lcr::Logger& logger)
   : logger_(logger)
   , addr_()
   , sockfd_(sockfd)
   , id_(id)
   , cache_(cache)
   , delay_()
   , timeout_(1000) // milliseconds => 1s
   , text_()
   , digest_()
   , error_()
   , ec_()
   , cancelled_()
{
   logger_.trace(LOG_LEVEL_6, "[WORKER] Worker #%u is ready", id_);
}

Worker::~Worker()
{
   logger_.trace(LOG_LEVEL_6, "[WORKER] Worker #%u has finished", id_);
}


void Worker::exec()
{
   char buffer[256];
   // The worker receives a request from a client, process it and send the response back
   int bytes = async_recv_(buffer, timeout_);
   int bytes_received = bytes;
   while(!error_ && buffer[bytes_received]!='\n' && bytes>0) {
      bytes = async_recv_(buffer+bytes_received, timeout_);
      bytes_received += bytes;
   }
   if(!error_ && process_request_(buffer, bytes_received)) {
      send_response_();
   }
   close(sockfd_);
}


int Worker::async_recv_(char* buffer, int timeout)
{
   int bytes_received = 0;

   struct pollfd fds[1];
   fds[0].fd = sockfd_;
   fds[0].events = POLLIN;

   int nfds = poll(fds, 1, 1000);
   if(nfds==-1) {
      if(errno!=EINTR) { // If not is an interrupt call
         ec_ = errno;
         error_ = true;
         logger_.error(LOG_WARNING, "[WORKER] ID#%u - Failed while polling in the socket (%d)", id_, ec_);
      }
   }
   else if(nfds>0) {
      if(fds[0].revents & POLLIN) { // Data received
         bytes_received = recv(sockfd_, buffer, (sizeof(buffer)-1), 0);
         if(bytes_received==-1) {
            ec_ = errno;
            error_ = true;
            logger_.trace(LOG_WARNING, "[WORKER] ID#%u - Reception error: (%d)", id_, ec_);
         }
      }
   }
   else { // nfds == 0
      // Nothing to do: the system call timed out before any file descriptors became read
   }
   if(!error_ && bytes_received>0) {
      if(buffer[bytes_received-1]=='\n') {
         buffer[bytes_received - 1] = '\0';
      }
      buffer[bytes_received] = '\0';
   }
   return bytes_received;
}


bool Worker::process_request_(char* buffer, int bytes_received)
{
   logger_.trace(LOG_LEVEL_3, "[WORKER] ID#%u - Message received => '%s'", id_, buffer);
   const std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();
   auto tokens = lcr::string::split(buffer);
   if(tokens.size()!=3) { // Invalid request formati: 'command text delay'
      error_ = true;
      std::ostringstream os;
      if(bytes_received == 0) {
         os << "<empty>";
      }
      else for(unsigned int ii=0; ii<bytes_received; ++ii) {
         os << "[" << (unsigned short)buffer[ii] << "] ";
      }
      logger_.error(LOG_WARNING, "[WORKER] ID#%u - Invalid message format: '%s' - tokens: %u - char buffer: %s (%d bytes received)",
         id_, buffer, tokens.size(), os.str().c_str(), bytes_received);
   }
   else { // OK  =>  tokens.size() = 3 
      lcr::string::to_lower(tokens[0]);
      text_ = tokens[1];
      bool found = cache_.get(text_, digest_);
      if(!found) {
         delay_ = std::chrono::milliseconds(std::stoi(tokens[2]));
         if(tokens[0]=="get" && lcr::string::is_number(tokens[2])) {
            while((std::chrono::system_clock::now() < (start + delay_)) && !cancelled_) {
               std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            if(!cancelled_) {
               digest_ = lcr::md5(text_);
               cache_.set(text_, digest_);
               logger_.trace(LOG_LEVEL_5, "[WORKER] ID#%u - Message proccesed in %d ms: '%s' =digest=> '%s'",
                     id_, (int)delay_.count(), text_.c_str(), digest_.c_str());
            }
            else {
               error_ = true; // Worker has been canceled 
            }
         }
         else {
           error_ = true; // Invalid command or invalid delay
           logger_.error(LOG_WARNING, "[WORKER] ID#%u - Invalid message format: '%s'", id_, buffer);
         }
      }
   }
   return !error_;
}


void Worker::send_response_()
{
   // Send the response
   int bytes_sent = send(sockfd_, digest_.c_str(), digest_.length(), 0);
   if(bytes_sent==-1) {
      ec_ = true;
      error_ = true;
      logger_.trace(LOG_WARNING, "[WORKER] ID#%u - Sending error: (%d)", id_, ec_);
   }
   else {
      logger_.trace(LOG_LEVEL_5, "[WORKER] ID#%u - Response sent in %d ms: '%s' =digest=> '%s'",
                    id_, (int)delay_.count(), text_.c_str(), digest_.c_str());
   }
}


} // namespace ncs

