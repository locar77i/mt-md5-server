//---------------------------------------------------------------------------
//  Class:       lcr::CommandLine<Opts>
//  File:        lcr/CommandLine.hpp
//
//  Desc:        Command line utility obtained using google
//
//---------------------------------------------------------------------------

#ifndef LIB__lcr_CommandLine__HPP_
#define LIB__lcr_CommandLine__HPP_


#include <functional>   // std::function
#include <iostream>     // std::cout, std::endl
#include <map>          // std::map
#include <memory>       // std::unique_ptr
#include <string>       // std::string
#include <sstream>      // std::stringstream
#include <string_view>  // std::string_view
#include <variant>      // std::variant
#include <vector>       // std::vector


namespace lcr
{
   template <class Opts>
   struct CommandLine : Opts
   {
      using MyProp = std::variant<std::string Opts::*, int Opts::*, double Opts::*, bool Opts::*>;
      using MyArg = std::pair<std::string, MyProp>;

      ~CommandLine() = default;

      Opts parse(int argc, const char* argv[])
      {
         std::vector<std::string_view> vargv(argv, argv+argc);
         for(int idx = 0; idx < argc; ++idx)
            for(auto& cbk : callbacks)
               cbk.second(idx, vargv);

         return static_cast<Opts>(*this);
      }

      static std::unique_ptr<CommandLine> Parser(std::initializer_list<MyArg> args)
      {
         auto cmdOpts = std::unique_ptr<CommandLine>(new CommandLine());
         for (auto arg : args) cmdOpts->register_callback(arg);
         return cmdOpts;
      }

      private:
         using callback_t = std::function<void(int, const std::vector<std::string_view>&)>;
         std::map<std::string, callback_t> callbacks;

         CommandLine() = default;
         CommandLine(const CommandLine&) = delete;
         CommandLine(CommandLine&&) = delete;
         CommandLine& operator=(const CommandLine&) = delete;
         CommandLine& operator=(CommandLine&&) = delete;

         auto register_callback(std::string name, MyProp prop)
         {
            callbacks[name] = [this, name, prop](int idx, const std::vector<std::string_view>& argv)
            {
               if (argv[idx] == name)
               {
                  visit(
                     [this, idx, &argv](auto&& arg)
                     {
                        if(idx < argv.size() - 1)
                        {
                           std::stringstream value;
                           value << argv[idx+1];
                           value >> this->*arg;
                        }
                     },
                     prop
                  );
               }
            };
         };

         auto register_callback(MyArg p) { return register_callback(p.first, p.second); }
};

} // namespace lcr

#endif // LIB__lcr_CommandLine__HPP_

