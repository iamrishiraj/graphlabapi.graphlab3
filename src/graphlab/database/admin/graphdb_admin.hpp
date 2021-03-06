#include <graphlab/database/graphdb_config.hpp>
#include <graphlab/database/graphdb_query_object.hpp>
#include <fault/query_object_client.hpp>

namespace graphlab {
  class graphdb_admin {
   public:
    enum cmd_type {
      START,
      RESET,
      UNKNOWN,
    };
    
   public:
     typedef libfault::query_object_client::query_result query_result;

     graphdb_admin(const graphdb_config& config) : config(config), qo(config) { }

     bool process(int argc, const char* argv[]) {
       return process(parse(argv[0]), argc-1, argv+1);
     }

     bool process(cmd_type cmd, int argc, const char* argv[]);


   private:
     cmd_type parse(std::string); 

     void start_server(std::string serverbin);

   private:
     graphdb_config config;

     graphlab::graphdb_query_object qo;
  };
} // end of namespace
