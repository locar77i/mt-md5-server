//---------------------------------------------------------------------------
//  Class:       lcr::StdLogger
//  File:        lcr/StdLogger.h
//
//---------------------------------------------------------------------------

#ifndef LIB__lcr_StdLogger__H_
#define LIB__lcr_StdLogger__H_

// Stl
#include <mutex>
#include <atomic>

// Componentes
#include "Logger.h"


namespace lcr
{

class StdLogger : public Logger
{
   public:
      // This class implements a Meyer's Singleton
      // The StdLogger is a type instance for which there is one, and only one, object in the system
      // The instance method receives as parameter the new logger level
      static StdLogger& instance(unsigned int level=3){
         static StdLogger instance;
         if(level) {
            instance.level_ = level;
         }
         return instance;
      }

   public:
      // Method to write traces in the log
      void trace(unsigned int level, const char * file, unsigned int line, const char * fmt, ...);
      // Method to write errors in the log
      void error(unsigned int level, const char * file, unsigned int line, const char * fmt, ...);

   private:
      // Constructor
      StdLogger();
      // Destroyer
      virtual ~StdLogger();

   private:
      // Copy constructor (disabled)
      StdLogger(const StdLogger&)= delete;
      // Assignment operator (disabled)
      StdLogger& operator=(const StdLogger&)= delete;

   private:
      // The logger current level
      std::atomic<unsigned int> level_;
      // Internal buffer to parse messages
      char buffer_[8192];
      // The buffer mutex
      mutable std::mutex mutex_;
};

} // namespace lcr

#endif // LIB__lcr_StdLogger__H_

