#ifndef GRAPHLAB_DATABASE_GRAPH_VERTEX_INDEX
#define GRAPHLAB_DATABASE_GRAPH_VERTEX_INDEX
#include <vector>
#include <graphlab/database/basic_types.hpp>
#include <graphlab/database/graph_field.hpp>
#include <graphlab/serialization/iarchive.hpp>
#include <graphlab/serialization/oarchive.hpp>
#include <graphlab/logger/assertions.hpp>
#include <boost/unordered_map.hpp>
#include <graphlab/macros_def.hpp>
namespace graphlab {
  /** 
   * \ingroup group_graph_database_sharedmem
   * An index on vertex id. 
   *
   * This class provide lookup for the vertex locations in a shard. 
   * The primary key is the vid. TODO: add support for secondary keys specified in the 
   * <code>graph_field</code>.
   */
  class graph_vertex_index {
   public:

     // Return the existence of a vertex with given id.
     inline bool has_vertex(graph_vid_t vid) const {
       return !(index_map.find(vid) == index_map.end());
     };

     // Return the index of a vertex in a shard.
     inline size_t get_index (graph_vid_t vid) const {
       ASSERT_TRUE(has_vertex(vid));
       return index_map.find(vid)->second;
     }

     // Update the index by adding a vertex
     inline bool add_vertex(graph_vid_t vid, graph_row* value, size_t pos) {
       if (has_vertex(vid)) {
         return false;
       }

       // Add vertex to primary index
       index_map[vid] = pos;

       // Add vertex to all existing index
       // typedef boost::unordered_map<std::string, size_t>::iterator indexiterator;
       // indexiterator iter;
       // while (iter != field_name_map.end()) {
       //   graph_int_t key = -1;
       //   if (value->get_field(iter->first.c_str())->get_integer(&key)) {
       //     (int_key_map[iter->second])[key] = vid;
       //   }
       //   iter++;
       // } 
       return true;
     }

     inline void clear() {
       index_map.clear();
     }

     inline void save (oarchive& oarc) const {
       oarc << index_map;
     }
     inline void load (iarchive& iarc) {
       iarc >> index_map;
     }


     // bool build_index(graph_field& field, const std::vector<graph_row*> vertices) {
     //   if (field.type != INT_TYPE || field.is_indexed) {
     //     return false;
     //   } else {
     //     boost::unordered_map<graph_int_t, graph_vid_t>  newindex;
     //     typedef boost::unordered_map<graph_vid_t, size_t>::iterator iterator;
     //     iterator iter = index_map.begin(); 
     //     while (iter != index_map.end()) {
     //       graph_row* row = vertices[iter->second];
     //       graph_int_t key = -1;
     //       if (row->get_field(field.name.c_str())->get_integer(&key)) {
     //         newindex[key] = iter->first;
     //       }
     //       iter++;
     //     }
     //     int_key_map.push_back(newindex);
     //     field_name_map[field.name] = int_key_map.size()-1;
     //     return true;
     //   }
     // }

    private:
      // map from vid -> index in the vertex_store 
      boost::unordered_map<graph_vid_t, size_t> index_map; 

      // // map from int key -> index in the vertex_store 
      // std::vector< boost::unordered_map<graph_int_t, graph_vid_t> > int_key_map; 

      // // map from field to the index location
      // boost::unordered_map<std::string, size_t> field_name_map;
  };
} // namespace graphlab
#include <graphlab/macros_undef.hpp>
#endif

