//---------------------------------------------------------------------------
//  Class:       lcr::RuntimeError
//  Class:       lcr::LogicError
//  File:        lcr/Exceptions.hpp
//
//---------------------------------------------------------------------------

#ifndef LIB__lcr_Exceptions__HPP_
#define LIB__lcr_Exceptions__HPP_

// Stl
#include <string>
#include <stdexcept>

// Componentes
#include "String.hpp"


namespace lcr
{

   // Class that efines a type of object to be thrown as exception.
   // It reports errors that are due to events beyond the scope of the program and can not be easily predicted. 
   class RuntimeError : public std::runtime_error
   {
      public:
         // The constructor receives as parameters an string containig the exception text and an integer with the error code
         RuntimeError(const std::string& msg, int ec)
            : std::runtime_error(msg + " [ec:" + to_string(ec) + "]")
            , ec_(ec)
         {}

         // Getter method for the error code
         int ec() const {
            return ec_;
         }

      private:
         int ec_;
   };


   // Class that defines a type of object to be thrown as exception.
   // It reports errors that are a consequence of faulty logic within the program such as violating logical preconditions or class invariants and may be preventable. 
   class LogicError : public std::logic_error
   {
      public:
         // The constructor receives as parameters an string containig the exception text
         LogicError(const std::string& msg)
            : std::logic_error(msg)
         {}
   };


} // namespace lcr

#endif // LIB__lcr_Exceptions__HPP_

