#ifndef GRAPHLAB_DATABASE_GRAPH_VERTEX_SHARED_MEM_HPP
#define GRAPHLAB_DATABASE_GRAPH_VERTEX_SHARED_MEM_HPP
#include <vector>
#include <graphlab/database/basic_types.hpp>
#include <graphlab/database/graph_row.hpp>
#include <graphlab/database/graph_vertex.hpp>
#include <graphlab/database/sharedmem_database/graph_edge_sharedmem.hpp>
#include <boost/unordered_set.hpp>
#include <graphlab/macros_def.hpp>
namespace graphlab {
/**
 * \ingroup group_graph_database
 *  An shared memory implementation of <code>graph_vertex</code>.
 *  The vertex data is directly accessible through pointers. 
 *  Adjacency information is accessible through <code>edge_index</code>
 *  object passed from the <code>graph_database_sharedmem</code>.
 *
 * This object is not thread-safe, and may not copied.
 */
class graph_vertex_sharedmem : public graph_vertex {
 private:

  // Internal id of the vertex in the shard.
  size_t local_id;

  // Master shard id of this vertex.
  graph_shard_id_t master;

  // Pointer to the database.
  graph_database* database;

  // Pointer to the mirrors.
  const boost::unordered_set<graph_shard_id_t>* mirrors;

  // modified_values[i] is set the new value of field i or NULL if field i is not modified.
  // the stored pointer is responsible for free the resources.
  std::vector<graph_value*> modified_values;

 public:
  /**
   * Creates a graph vertex object 
   */
  inline graph_vertex_sharedmem(graph_vid_t vid,
                         graph_shard_id_t master,
                         graph_database* db)
      : master(master), database(db) { 
        local_id = db->get_shard(master)->shard_impl.vertex_index.get_index(vid);
        modified_values.resize(num_fields());
        mirrors = &(database->get_shard(master)->shard_impl.vertex_mirrors[local_id]);
      }

  inline ~graph_vertex_sharedmem() {
    for (size_t i = 0; i < modified_values.size(); i++) {
      if (modified_values[i] != NULL) {
        delete modified_values[i];
        modified_values[i] = NULL;
      }
    }
  }

  /**
   * Returns the ID of the vertex
   */
  inline graph_vid_t get_id() const {
    return database->get_shard(master)->vertex(local_id);
  }

  /**
   * Returns the immutable pointer to the underlying data.
   */
  inline const graph_row* immutable_data() const {
    return database->get_shard(master)->vertex_data(local_id);
  }

  /**
   * Set the field at fieldpos to new value. 
   * Modifications made to the data, are only committed 
   * to the database through a write_* call.
   */
  inline bool set_field(size_t fieldpos, const graph_value& value) {
    if (fieldpos >= num_fields()) {
      return false;
    }
    if (fieldpos >= modified_values.size()) {
      modified_values.resize(fieldpos+1);
    }
    if (modified_values[fieldpos] == NULL) {
      graph_value* val = new graph_value();
      modified_values[fieldpos] = val;
    }
    // copy over old data
    *modified_values[fieldpos] = *(immutable_data()->get_field(fieldpos)); 
    return modified_values[fieldpos]->set_val(value);
  }

  /**
   * Return a pointer to the graph value at the requested field.
   *
   * \note Instead of pointing to the original value, the pointer points
   * to an ghost copy of the actual field.
   *
   * \note Modification made to the data should be commited through a write_* call.
   */
  inline graph_value* get_field(size_t fieldpos) {
    if (fieldpos >= num_fields()) {
      return NULL;
    }
    if (fieldpos >= modified_values.size()) {
      modified_values.resize(fieldpos+1);
    }
    if (modified_values[fieldpos] == NULL) {
      graph_value* val = new graph_value();
      modified_values[fieldpos] = val;
    }
    // copy over old data
    *modified_values[fieldpos] = *(immutable_data()->get_field(fieldpos)); 
    return modified_values[fieldpos];
  }

  /// Returns number of fields in the data.
  inline size_t num_fields() const {
    return immutable_data()->num_fields();
  }

  // --- synchronization ---
  /**
   * Commits changes made to the data on this vertex synchronously.
   * This resets the modification and delta flags on all values in the 
   * graph_row.
   */ 
  inline void write_changes() {
    graph_row* oldvalues = data();
    for (size_t i = 0; i < modified_values.size(); i++) {
      if (modified_values[i] != NULL) {
        *(oldvalues->get_field(i)) = *modified_values[i];
        delete modified_values[i];
        modified_values[i] = NULL;
      }
    }
  }

  /**
   * Same as synchronous commit in shared memory.
   */ 
  inline void write_changes_async() { 
    write_changes();
  }

  /**
   * Fetch the data pointer from the right shard. 
   */ 
  inline void refresh() { }

  /**
   * Commits the change immediately.
   * Refresh has no effects in shared memory.
   */ 
  inline void write_and_refresh() { 
    write_changes();
  }

  // --- sharding ---
  /**
   * Returns the ID of the shard that owns this vertex
   */
  inline graph_shard_id_t master_shard() const { return master; }

  /**
   * Returns the IDs of the shards with mirror of this vertex
   */
  inline std::vector<graph_shard_id_t> mirror_shards() const {
    std::vector<graph_shard_id_t> ret;
    foreach(const graph_shard_id_t& mirror, *mirrors) {
      ret.push_back(mirror);
    }
    return ret;
  };

  /**
   * returns the number of shards this vertex spans
   */
  inline size_t get_num_shards() const {
    return mirrors->size() +1;
  };

  /**
   * returns a vector containing the shard IDs this vertex spans
   */
  inline std::vector<graph_shard_id_t> get_shard_list() const {
    std::vector<graph_shard_id_t> ret = mirror_shards();
    ret.push_back(master);
    return ret;
  };

  // --- adjacency ---
  /** gets part of the adjacency list of this vertex belonging on shard shard_id
   *  Returns NULL on failure. The returned edges must be freed using
   *  graph_database::free_edge() for graph_database::free_edge_vector()
   *
   *  out_inadj will be filled to contain a list of graph edges where the 
   *  destination vertex is the current vertex. out_outadj will be filled to
   *  contain a list of graph edges where the source vertex is the current 
   *  vertex.
   *
   *  Either out_inadj or out_outadj may be NULL in which case those edges
   *  are not retrieved (for instance, I am only interested in the in edges of 
   *  the vertex).
   *
   *  The prefetch behavior is ignored. We always pass the data pointer to the new edge. 
   *
   *  Assuming the shardid  is a local shard.
   */ 
  void get_adj_list(graph_shard_id_t shard_id, 
                            bool prefetch_data,
                            std::vector<graph_edge*>* out_inadj,
                            std::vector<graph_edge*>* out_outadj) {
    database->get_adj_list(get_id(), shard_id, prefetch_data, out_inadj, out_outadj);
  }

 private:
  /**
   * Returns a pointer to the graph_row representing the data
   * stored on this vertex. 
   *
   * \note Only for internal use. To efficiently track the modifications to the data,  
   * all changes to the data must be made throught the public interface.
   */
  inline graph_row* data() {
    return database->get_shard(master)->vertex_data(local_id);
  };
}; // end of class
} // namespace graphlab
#include <graphlab/macros_undef.hpp>
#endif
