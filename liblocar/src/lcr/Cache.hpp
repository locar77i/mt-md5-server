//---------------------------------------------------------------------------
//  Class:       lcr::Cache
//  File:        lcr/Cache.hpp
//
//---------------------------------------------------------------------------

#ifndef LIB__lcr_Cache__HPP_
#define LIB__lcr_Cache__HPP_


// Stl
#include <mutex>
#include <chrono>
#include <unordered_map>
#include <sstream>

// lib locar
#include "Logger.h"
#include "Exceptions.hpp"


namespace lcr
{

// This template class represents a generic and parametrizable cache, useful for a multitude of projects 
template <class KEY, class DATA>
class Cache
{
   public:
      // The constructor receives as parameters the cache capacity and the automatic discard timeout.
      // It also receives a reference to the logger to show traces of its operation.
      Cache(unsigned int capacity, unsigned long long timeout, Logger& logger)
         : logger_(logger)
         , capacity_(capacity)
         , timeout_(timeout)
         , map_()
         , hits_()
         , faults_()
         , erased_()
         , overwritten_()
         , mutex_()
      {
         logger_.trace(LOG_LEVEL_4, "[CACHE] The cache is ready");
      }

      // Destroyer
      virtual ~Cache() {
         logger_.trace(LOG_LEVEL_4, "[CACHE] The cache has finished");
      }

   public:
      // Getter method that returns the cache size: the current number of entries
      std::size_t size() const {
         std::lock_guard<std::mutex> guard(mutex_);
         return map_.size();
      }

      // Public method that prints the cache content
      void printContent() const {
         logger_.trace(LOG_LEVEL_1, "[CACHE]---- Cache content ---------------------------------------------------------");
         std::lock_guard<std::mutex> guard(mutex_);
         if(!map_.empty()) {
            for(auto&& pair : map_) {
               logger_.trace(LOG_LEVEL_1, "[CACHE] {key: '%s', data: %s}", to_string(pair.first).c_str(), to_string(pair.second.data_).c_str());
            }
            logger_.trace(LOG_LEVEL_1, "[CACHE]----------------------------------------------------------------------------");
         }
         logger_.trace(LOG_LEVEL_1, "[CACHE] Total: %u entries.", map_.size());
         logger_.trace(LOG_LEVEL_1, "[CACHE]----------------------------------------------------------------------------");
      }

   public:
      // Public setter method that stores in the map a new key and its related data, both passed as parameters
      void set(const KEY& key, const DATA& data) {
         std::lock_guard<std::mutex> guard(mutex_);
         if(capacity_>0) { // Write in cache
            auto it = map_.find(key);
            if(it==map_.end()) { // Insert data in the map
               if(map_.size()==capacity_) {// delete oldest
                  auto older_it = map_.begin();
                  for(auto it=map_.begin(); it!=map_.end(); ++it) {
                     if(it->second.last() < older_it->second.last()) {
                        older_it = it;
                     }
                  }
                  logger_.trace(LOG_LEVEL_4, "[CACHE] Erasing the least used entry: key '%s' => data '%s'", to_string(older_it->first).c_str(), to_string(older_it->second.data_).c_str());
                  map_.erase(older_it);
                  ++erased_;
               }
               logger_.trace(LOG_LEVEL_4, "[CACHE] Inserting new entry: key '%s' => data '%s'", to_string(key).c_str(), to_string(data).c_str());
               auto iit = map_.insert(std::pair<KEY, Entry>(key, Entry(data))).first;
            }
            else { // Overwrite data in the map
               it->second = Entry(data);
               ++overwritten_;
            }
         }
      }

      // Public getter method that finds the data associated with the key passed as a parameter 
      bool get(const KEY& key, DATA& data) const {
         std::lock_guard<std::mutex> guard(mutex_);
         auto it = map_.find(key);
         if(it==map_.end()) {
            ++faults_;
            return false;
         }
         ++hits_;
         data = it->second.data();
         return true;
      }

      // Public method that updates the internal map of entries: required when discard functionality is active 
      void update() {
         std::lock_guard<std::mutex> guard(mutex_);
         if(timeout_.count()) {
            auto now = std::chrono::system_clock::now();
            for(auto it=map_.begin(); it!=map_.end(); ) {
               auto current = it++;
               const auto& last = current->second.last();
               std::chrono::time_point<std::chrono::system_clock> limit = last + timeout_;
               if(now>limit) {
                  logger_.trace(LOG_LEVEL_4, "[CACHE] Erasing the oldest entry: key '%s' => data '%s'", to_string(current->first).c_str(), to_string(current->second.data_).c_str());
                  map_.erase(current);
                  ++erased_;
               }
            }
         }
      }

      // Public method that set the discard timeout
      void setTimeout(unsigned long long timeout) {
         std::lock_guard<std::mutex> guard(mutex_);
         timeout_ = std::chrono::seconds(timeout);
         logger_.trace(LOG_LEVEL_1, "[CACHE] Updating the cache timeout [timeout:%d]", (int)timeout_.count());
      }

      // Public method to clear cache internal map with the entries data
      void clearContent() {
         std::lock_guard<std::mutex> guard(mutex_);
         erased_ += map_.size();
         map_.clear();
      }

      // Public method to print the cache statisctics
      void printStatistics() const {
         std::lock_guard<std::mutex> guard(mutex_);
         logger_.trace(LOG_LEVEL_1, "[CACHE]---- Cache statistics ------------------------------------------------------");
         logger_.trace(LOG_LEVEL_1, "[CACHE] Total: %u entries [hits:%llu] [faults:%llu] [erased:%llu] [overwritten:%llu]", map_.size(), hits_, faults_, erased_, overwritten_);
         logger_.trace(LOG_LEVEL_1, "[CACHE]----------------------------------------------------------------------------");
      }

      // Public method to clear the cache statisctics
      void clearStatistics() const {
         std::lock_guard<std::mutex> guard(mutex_);
         hits_ = 0;
         faults_ = 0;
         erased_ = 0;
         logger_.trace(LOG_LEVEL_1, "[CACHE] Cache statistics have been cleared");
      }

   private:
      // Private class that represents a cache entry
      struct Entry
      {
         friend class Cache;

         public:
            // The entry constructor receives as parameter the internal data
            Entry(const DATA& data)
               : data_(data)
               , last_(std::chrono::system_clock::now())
            {}

            // Getter method for the entry internal data
            DATA& data() {
               last_ = std::chrono::system_clock::now();
               return data_;
            }
            // Getter method for the entry internal data
            const DATA& data() const {
               last_ = std::chrono::system_clock::now();
               return data_;
            }

            // Getter method for the entry creation time
            const std::chrono::time_point<std::chrono::system_clock>& last() const {
               return last_;
            }

         private:
            DATA data_;  // The entry internal data
            mutable std::chrono::time_point<std::chrono::system_clock> last_;  // A point in time that marks the entry age
      };

   private:
      // Copy constructor (disabled)
      Cache(const Cache&) = delete;
      // Assignment operator (disabled)
      Cache& operator=(const Cache&) = delete;

   private:
      // The logger reference
      Logger& logger_;

      // The cache maximum capacity
      unsigned int capacity_;

      // The timeout for the automatic discard of entries
      std::chrono::seconds timeout_;

      // The internal memory map for cache entries
      std::unordered_map<KEY, Entry> map_;

      // Mutable flags for statistics purposes
      mutable unsigned long long hits_;
      mutable unsigned long long faults_;
      mutable unsigned long long erased_;
      mutable unsigned long long overwritten_;

      mutable std::mutex mutex_;
};

} // namespace lcr

#endif // LIB__lcr_Cache__HPP_

