//---------------------------------------------------------------------------
//  Class:
//  File:        lcr/String.h
//
//---------------------------------------------------------------------------

#ifndef LIB__lcr_String__HPP_
#define LIB__lcr_String__HPP_


// Stl
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <locale>
#include <chrono>
#include <iomanip>


namespace lcr
{

// Templated function that convert generic data to string
template <typename T>
static inline std::string to_string(const T& data) {
   std::ostringstream os;
   os << data;
   return os.str();
}

   namespace string
   {

   // Utility functions to split input texts into a vector of tokens using the separator parameter
   static inline std::vector<std::string> split(const char * buffer, char separator=' ') {
      std::istringstream f(buffer);
      std::string s;
      std::vector<std::string> strings;
      while(std::getline(f, s, separator)) {
         strings.push_back(s);
      }
      return strings;
   }
   static inline std::vector<std::string> split(const std::string& str) { return split(str.c_str()); }


   // Utility function that checks if the input string contains only digits, that is to say, a number
   static inline bool is_number(const std::string& s) {
      return !s.empty() && std::find_if(s.begin(), s.end(), [](unsigned char c) { return !std::isdigit(c); }) == s.end();
   }


   // Utility function that transform the input string converting each character in its equivalent lowercase
   static inline void to_lower(std::string& s) {
      std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return std::tolower(c); });
   }


   // Utility function that transform the input string converting each character in its equivalent uppercase
   static inline void to_upper(std::string& s) {
      std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return std::toupper(c); });
   }


   // Utility functions to create a formated string containing the current timestamp using the system clock
   static inline std::string timestamp(const std::string& format)
   {
      std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
      auto seconds_since_epoch = std::chrono::system_clock::to_time_t(now);
      return to_string(std::put_time(std::localtime(&seconds_since_epoch), format.c_str()));
   }
   static inline std::string timestamp() { return timestamp("%Y %b %d %H:%M:%S"); }



   // trim from start (in place)
   static inline void ltrim(std::string &s) {
      s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
          return !std::isspace(ch);
      }));
   }

   // trim from end (in place)
   static inline void rtrim(std::string &s) {
      s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
          return !std::isspace(ch);
      }).base(), s.end());
   }

   // trim from both ends (in place)
   static inline void trim(std::string &s) {
      ltrim(s);
      rtrim(s);
   }

   // trim from start (copying)
   static inline std::string ltrim_copy(std::string s) {
      ltrim(s);
      return s;
   }

   // trim from end (copying)
   static inline std::string rtrim_copy(std::string s) {
      rtrim(s);
      return s;
   }

   // trim from both ends (copying)
   static inline std::string trim_copy(std::string s) {
      trim(s);
      return s;
   }


   } // namespace string
} // namespace lcr

#endif // LIB__lcr_String__HPP_

