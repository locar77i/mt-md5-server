//---------------------------------------------------------------------------
//  Class:       lcr::Logger
//  File:        lcr/Logger.h
//
//---------------------------------------------------------------------------

#ifndef LIB__lcr_Logger__H_
#define LIB__lcr_Logger__H_

// Stl
#include <string>
#include <cstdarg>
#include <vector>


// Trace levels
#define  LOG_LEVEL_1  1,__FILE__,__LINE__
#define  LOG_LEVEL_2  2,__FILE__,__LINE__
#define  LOG_LEVEL_3  3,__FILE__,__LINE__
#define  LOG_LEVEL_4  4,__FILE__,__LINE__
#define  LOG_LEVEL_5  5,__FILE__,__LINE__
#define  LOG_LEVEL_6  6,__FILE__,__LINE__
// Errors
#define  LOG_CRITICAL 1,__FILE__,__LINE__
#define  LOG_ERROR    2,__FILE__,__LINE__
#define  LOG_WARNING  3,__FILE__,__LINE__

namespace lcr
{

// Base class that defines the logger generic interface
class Logger
{
   public:
      // Virtual pure method to write traces in the log
      virtual void trace(unsigned int level, const char * file, unsigned int line, const char * fmt, ...) = 0; // Trazas
      // Virtual pure method to write errors in the log
      virtual void error(unsigned int level, const char * file, unsigned int line, const char * fmt, ...) = 0; // Errores

   protected:
      // Constructor
      Logger()
      {}

      // Destroyer 
      virtual ~Logger()
      {}

   private:
      // Copy constructor (disabled)
      Logger(const Logger&)= delete;
      // Assignment operator (disabled)
      Logger& operator=(const Logger&)= delete;
};

} // namespace lcr

#endif // LIB__lcr_Logger__H_

