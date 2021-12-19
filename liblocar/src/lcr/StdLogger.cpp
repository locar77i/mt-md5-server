//------------------------------------------------------------------------------------------
//  Class:       lcr::StdLogger
//  File:        lcr/StdLogger.cpp
//
//------------------------------------------------------------------------------------------
#include "StdLogger.h"

// Std
#include <string>
#include <iostream>
#include <cstdarg>

// Componentes
#include "String.hpp"


namespace lcr
{


StdLogger::StdLogger()
   : Logger()
   , level_(3)
   , buffer_()
   , mutex_()
{
}

StdLogger::~StdLogger()
{
}


void StdLogger::trace(unsigned int level, const char * file, unsigned int line, const char * fmt, ...)
{
   if(level<=level_) {
      va_list args;
      va_start(args, fmt);
      std::lock_guard<std::mutex> guard(mutex_);
      vsnprintf(buffer_, sizeof(buffer_), fmt, args);
      va_end(args);
      std::cout << "- " << string::timestamp() << " T" << level << ": " << buffer_ << "  [" << file << " +" << line << "]" << std::endl;
   }
}

void StdLogger::error(unsigned int level, const char * file, unsigned int line, const char * fmt, ...)
{
   va_list args;
   va_start(args, fmt);
   std::lock_guard<std::mutex> guard(mutex_);
   vsnprintf(buffer_, sizeof(buffer_), fmt, args);
   va_end(args);
   std::string header(" ERROR: ");
   switch(level)
   {
      case 1:
         header = " CRITICAL: ";
         break;
      case 2:
         header = " ERROR: ";
         break;
      case 3:
         header = " WARNING: ";
         break;
   }
   std::cerr << "- " << string::timestamp() << header << buffer_ << "  [" << file << " +" << line << "]" << std::endl;
}


} // namespace lcr

