//------------------------------------------------------------------------------------------
//  Class:       ncs::Server
//  File:        ncs/Server.cpp
//
//------------------------------------------------------------------------------------------
#include "Server.h"

// Stl
#include <thread>
#include <errno.h>
#include <strings.h>
// sockets
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <poll.h>

// lib locar
#include "lcr/Exceptions.hpp"


namespace ncs
{

Server::Server(unsigned int port, unsigned int cache_capacity, unsigned int cache_timeout, lcr::Logger& logger)
   : logger_(logger)
   , port_(port)
   , sockfd_()
   , finish_()
   , cancel_()
   , clear_()
   , print_()
   , cache_(cache_capacity, cache_timeout, logger)
   , sequence_()
   , tasks_()
   , workers_()
   , errors_()
   , unattended_requests_()
{
   logger_.trace(LOG_LEVEL_4, "[SERVER] The server is ready");
}

Server::~Server()
{
   if(!tasks_.empty()) {
      wait_for_tasks_();
   }
   logger_.trace(LOG_LEVEL_4, "[SERVER] The server has finished");
}


template<typename T>
bool future_is_ready(std::future<T>& t){
   return t.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

int Server::run()
{
   int sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
   if(sockfd_==-1) {
      throw lcr::RuntimeError("Failed to open the server socket", errno);
   }

   // Bind the server socket
   struct sockaddr_in server_addr;
   bzero(reinterpret_cast<char*>(&server_addr), sizeof(server_addr)); // clear address structure
   server_addr.sin_family = AF_INET;
   server_addr.sin_port = htons(port_);
   server_addr.sin_addr.s_addr = INADDR_ANY;
   bzero(&server_addr.sin_zero, 0);
   if(bind(sockfd_, reinterpret_cast<struct sockaddr *>(&server_addr), sizeof(struct sockaddr_in)) == -1) {
      throw lcr::RuntimeError("Failed to bind the server socket", errno);
   }

   // Listen on the server socket
   if(listen(sockfd_, 10) == -1) {
      throw lcr::RuntimeError("Unable to listen on the server socket", errno);
   }

   struct sockaddr_in client_addr;
   int len=sizeof(sockaddr_in);

   struct pollfd fds[1];
   fds[0].fd = sockfd_;
   fds[0].events = POLLIN;

   // Server main operation loop
   while(!finish_ && !cancel_) {
//      logger_.trace(LOG_LEVEL_6, "[SERVER] executing server main operations ...");
      int nfds = poll(fds, 1, 1000);
      if(nfds==-1) {
         if(errno!=EINTR) { // If not is an interrupt call
            throw lcr::RuntimeError("Failed while polling in the server socket", errno);
         }
      }
      else if(nfds>0) {
         if(fds[0].revents & POLLIN) { // Data received
            int client_sockfd = accept(sockfd_, reinterpret_cast<struct sockaddr *>(&client_addr), &len);
            if(client_sockfd==-1){
               throw lcr::RuntimeError("Unable to accept connections on the server socket", errno);
            }
            // Create a worker to process the request, with a unique sequence identifier and a random time delay
            std::shared_ptr<Worker> worker(new Worker(++sequence_, client_sockfd, client_addr, cache_, logger_));
            try {
               // Create the new worker task
               tasks_.push_back(
                  {worker, std::async(std::launch::async, &Worker::exec, worker.get())}
               );
               // Increment the total number of workers
               ++workers_;
            }
            catch(const std::system_error& ex) {
               logger_.error(LOG_WARNING, "[WORKER] Unable to fulfill the request: %s", ex.what(), unattended_requests_);
               ++unattended_requests_;
               logger_.error(LOG_WARNING, "[WORKER] %llu unattended requests until now", unattended_requests_);
            }
         }
      }
      else { // nfds == 0
         // Nothing to do: the system call timed out before any file descriptors became read
      }
      // Check for clear requests
      if(clear_) {
         cache_.clearContent();
         cache_.printStatistics();
         clear_ = false;
      }
      // Check for print requests
      if(print_) {
         cache_.printContent();
         cache_.printStatistics();
         printStatistics();
         print_ = false;
      }
      // Update the data cache: useful for automatic when discard is enabled
      cache_.update();
      // Update the worker threads
      update_tasks_();
   }
   close(sockfd_);
   logger_.trace(LOG_LEVEL_1, "[SERVER] The server will not attend any more requests", tasks_.size());
   int rc; // The method return code
   if(finish_) {
      wait_for_tasks_();
      rc = 0;
   }
   else if(cancel_) {
      cancel_tasks_();
      rc = 1;
   }
   cache_.printContent();
   cache_.printStatistics();
   printStatistics();
   return rc;
}


void Server::printStatistics() const
{
   logger_.trace(LOG_LEVEL_1, "[SERVER]---- Server statistics ------------------------------------------------------");
   logger_.trace(LOG_LEVEL_1, "[SERVER] Total number of executed workers: %llu", workers_);
   if(errors_) {
      logger_.trace(LOG_LEVEL_1, "[SERVER] Total errors reported by workers: %llu", errors_);
   }
   else {
      logger_.trace(LOG_LEVEL_1, "[SERVER] No errors reported by workers");
   }
   logger_.trace(LOG_LEVEL_1, "[SERVER] Unattended input requests: %llu", unattended_requests_);
   logger_.trace(LOG_LEVEL_1, "[SERVER]-----------------------------------------------------------------------------");
}

void Server::update_tasks_()
{
   for(auto it=tasks_.begin(); it!=tasks_.end(); ) {
      auto current = it++;
      if(future_is_ready(current->future)) {
         const auto& worker = *(current->worker);
         logger_.trace(LOG_LEVEL_5, "[SERVER] Finishing worker #%u (%u tasks left)", worker.id(), tasks_.size());
         if(worker.error()) {
            ++errors_;
         }
         tasks_.erase(current);
      }
   }
}

void Server::wait_for_tasks_()
{
   logger_.trace(LOG_LEVEL_1, "[SERVER] Waiting for pending tasks... (%u tasks)", tasks_.size());
   for(auto it=tasks_.begin(); it!=tasks_.end(); ) {
      auto current = it++;
      const auto& worker = *(current->worker);
      logger_.trace(LOG_LEVEL_4, "[SERVER] Waiting for worker #%u (%u tasks left)", worker.id(), tasks_.size());
      current->future.wait();
      if(worker.error()) {
         ++errors_;
      }
      tasks_.erase(current);
   }
}

void Server::cancel_tasks_()
{
   logger_.trace(LOG_LEVEL_1, "[SERVER] Canceling pending tasks... (%u tasks)", tasks_.size());
   for(auto it=tasks_.begin(); it!=tasks_.end(); ++it) {
      const auto& worker = *(it->worker);
      worker.cancel();
   }
   for(auto it=tasks_.begin(); it!=tasks_.end(); ) {
      auto current = it++;
      const auto& worker = *(current->worker);
      logger_.trace(LOG_LEVEL_4, "[SERVER] Canceling worker #%u (%u tasks left)", worker.id(), tasks_.size());
      current->future.wait();
      if(worker.error()) {
         ++errors_;
      }
      tasks_.erase(current);
   }
}


} // namespace ncs

