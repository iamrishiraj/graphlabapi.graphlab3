#include<graphlab/database/client/graphdb_client.hpp>
namespace graphlab {
  // ----------------------------- Batch Methods --------------------------------------
  bool graphdb_client::add_edges(const std::vector<edge_insert_descriptor>& edges,
                                 std::vector<int>& errorcodes) {

    QueryMessage::header header(QueryMessage::BADD, QueryMessage::EDGE);
    bool success = scatter_messages<edge_insert_descriptor, char>(header, edges, boost::bind(&graphdb_client::ein2shard, this, _1), NULL, errorcodes);
    
    // add mirrors
    mirror_table_type mirror_table = mirror_table_from_edges(edges);
    std::vector<mirror_insert_descriptor> vid_mirror_pairs;
    for (mirror_table_type::iterator it = mirror_table.begin(); it != mirror_table.end(); it++) {
      graph_vid_t vid = it->first;
      std::vector<graph_shard_id_t> mirrors(it->second.begin(), it->second.end());
      vid_mirror_pairs.push_back (mirror_insert_descriptor(vid, mirrors));
    }

    success &= add_vertex_mirrors(vid_mirror_pairs, errorcodes);
    ASSERT_TRUE(success);

    // check error codes
    // for (size_t i = 0; i < errorcodes.size(); ++i) {
    //   if (errorcodes[i] != 0)
    //     return false;
    // }
    return true;
  }

  bool graphdb_client::add_vertices(const std::vector<vertex_insert_descriptor>& vertices,
                                    std::vector<int>& errorcodes) {
    QueryMessage::header header(QueryMessage::BADD, QueryMessage::VERTEX);
    bool success = scatter_messages<vertex_insert_descriptor, char>(header, vertices, boost::bind(&graphdb_client::vin2shard, this, _1), NULL, errorcodes);
    // check error codes
    // for (size_t i = 0; i < errorcodes.size(); ++i) {
    //   if (errorcodes[i] != 0)
    //     return false;
    // }
    return success;
  }

  bool graphdb_client::add_vertex_mirrors(const std::vector<mirror_insert_descriptor>& vid_mirror_pairs,
                                          std::vector<int>& errorcodes) {
    QueryMessage::header header(QueryMessage::BADD, QueryMessage::VMIRROR);
    bool success = scatter_messages<mirror_insert_descriptor, char>(header, vid_mirror_pairs, boost::bind(&graphdb_client::vidpair2shard<std::vector<graph_shard_id_t> >, this, _1), NULL, errorcodes);
    // check error codes
    // for (size_t i = 0; i < errorcodes.size(); ++i) {
    //   if (errorcodes[i] != 0)
    //     return false;
    // }
    return success;
  }


  bool graphdb_client::get_edges(const std::vector<graph_eid_t>& eids,
                                 std::vector<graph_row>& out, 
                                 std::vector<int>& errorcodes) {

    QueryMessage::header header(QueryMessage::BGET, QueryMessage::EDGE);
    bool success = scatter_messages<graph_eid_t, graph_row>(header, eids, boost::bind(&graphdb_client::eid2shard, this, _1), &out, errorcodes);

    // for (size_t i = 0; i < errorcodes.size(); ++i) {
    //   if (errorcodes[i] != 0)
    //     return false;
    // }
    return success;
  }

  bool graphdb_client::get_vertices(const std::vector<graph_vid_t>& vids,
                                    std::vector<graph_row>& out, 
                                    std::vector<int>& errorcodes) {
    QueryMessage::header header(QueryMessage::BGET, QueryMessage::VERTEX);
    bool success = scatter_messages<graph_vid_t, graph_row>(header, vids, boost::bind(&graphdb_client::vid2shard, this, _1), &out, errorcodes);

    // for (size_t i = 0; i < errorcodes.size(); ++i) {
    //   if (errorcodes[i] != 0)
    //     return false;
    // }
    return success;
  }


  bool graphdb_client::set_edges(const std::vector<std::pair<graph_eid_t, graph_row> >& pairs,
                                 std::vector<int>& errorcodes) {
    QueryMessage::header header(QueryMessage::BSET, QueryMessage::EDGE);
    bool success = scatter_messages<std::pair<graph_eid_t, graph_row>, char>(header, pairs, boost::bind(&graphdb_client::eidpair2shard<graph_row>, this, _1), NULL, errorcodes);
    // for (size_t i = 0; i < errorcodes.size(); ++i) {
    //   if (errorcodes[i] != 0)
    //     return false;
    // }
    return success;
  }


  bool graphdb_client::set_vertices(const std::vector<std::pair<graph_vid_t, graph_row> >& pairs,
                                    std::vector<int>& errorcodes) {
    QueryMessage::header header(QueryMessage::BSET, QueryMessage::VERTEX);
    bool success = scatter_messages<std::pair<graph_vid_t, graph_row>, char>(header, pairs, boost::bind(&graphdb_client::vidpair2shard<graph_row>, this, _1), NULL, errorcodes);
    // for (size_t i = 0; i < errorcodes.size(); ++i) {
    //   if (errorcodes[i] != 0)
    //     return false;
    // }
    return success;  
  }

  int graphdb_client::add_edge(graph_vid_t source, graph_vid_t dest, const graph_row& data) {
    QueryMessage qm(QueryMessage::ADD, QueryMessage::EDGE);
    qm << source << dest << data;
    graph_shard_id_t target = shard_manager.get_master(source, dest); 
    query_result future = queryobj.update(target, qm.message(), qm.length());


    std::vector<graph_shard_id_t> mirrors(target);
    // send out mirrors
    add_vertex_mirror(source, mirrors);
    add_vertex_mirror(target, mirrors);

    return queryobj.parse_reply(future);
  }

  int graphdb_client::add_vertex(graph_vid_t vid, const graph_row& data) {
    QueryMessage qm(QueryMessage::ADD, QueryMessage::VERTEX);
    qm << vid << data;
    query_result future = queryobj.query(shard_manager.get_master(vid), 
                                         qm.message(), qm.length());
    return queryobj.parse_reply(future);
  }

  int graphdb_client::get_edge(graph_eid_t eid, graph_row& out) {
    QueryMessage qm(QueryMessage::GET, QueryMessage::EDGE);
    qm << eid;
    query_result future = queryobj.query(split_eid(eid).first, qm.message(), qm.length());
    return queryobj.parse_reply(future, out);
  }

  int graphdb_client::get_vertex(graph_vid_t vid, graph_row& out) {
    QueryMessage qm(QueryMessage::GET, QueryMessage::VERTEX);
    qm << vid;
    query_result future = queryobj.query(shard_manager.get_master(vid), qm.message(), qm.length());
    return queryobj.parse_reply(future, out);
  }

  int graphdb_client::get_vertex_adj(graph_vid_t vid, bool in_edges, vertex_adj_descriptor& out) {
    QueryMessage qm(QueryMessage::GET, QueryMessage::VERTEXADJ);
    qm << vid << in_edges;

    // // find the shards we need query about vid's adj 
    graph_shard_id_t master = shard_manager.get_master(vid);
    std::vector<graph_shard_id_t> spans; 
    shard_manager.get_neighbors(master, spans);

    std::vector<query_result> futures;
    std::vector<int> errorcodes;

    queryobj.query_multi(spans, qm.message(), qm.length(), futures);
    queryobj.parse_and_aggregate(futures, out, errorcodes);

    for (size_t i = 0; i < errorcodes.size(); ++i) {
      // Expect EINVID, queried shards may not have adj structure of the query vertex.
      if (errorcodes[i] != 0 && errorcodes[i] != EINVID) {
        return errorcodes[i];
      }
    }
    return 0;
  }

  int graphdb_client::set_edge(graph_eid_t eid, const graph_row& data) {
    QueryMessage qm(QueryMessage::SET, QueryMessage::EDGE);
    qm << eid << data;
    query_result future = queryobj.update(split_eid(eid).first, qm.message(), qm.length());
    return queryobj.parse_reply(future);
  }

  int graphdb_client::set_vertex(graph_vid_t vid, const graph_row& data) {
    QueryMessage qm(QueryMessage::SET, QueryMessage::VERTEX);
    qm << vid << data;
    query_result future = queryobj.update(shard_manager.get_master(vid), qm.message(), qm.length());
    return queryobj.parse_reply(future);
  }

  uint64_t graphdb_client::num_vertices() {
    QueryMessage qm(QueryMessage::GET, QueryMessage::NVERTS);
    std::vector<query_result> futures;
    std::vector<int> errorcodes;
    queryobj.query_all(qm.message(), qm.length(), futures);
    uint64_t acc = 0;
    ASSERT_TRUE(queryobj.parse_and_aggregate(futures, acc, errorcodes));
    return acc;
  }

  uint64_t graphdb_client::num_edges() {
    QueryMessage qm(QueryMessage::GET, QueryMessage::NEDGES);
    std::vector<query_result> futures;
    std::vector<int> errorcodes;
    queryobj.query_all(qm.message(), qm.length(), futures);
    uint64_t acc = 0;
    ASSERT_TRUE(queryobj.parse_and_aggregate(futures, acc, errorcodes));
    return acc;
  }

  int graphdb_client::add_edge_field(const graph_field& field) {
    QueryMessage qm(QueryMessage::ADD, QueryMessage::EFIELD);
    qm << field;
    std::vector<query_result> futures;
    queryobj.update_all(qm.message(), qm.length(), futures);

    for (size_t i = 0; i < futures.size(); ++i) {
      int error = queryobj.parse_reply(futures[i]);
      if (error != 0)
        return error;
    }
    return 0;
  }

  int graphdb_client::add_vertex_field(const graph_field& field) {
    QueryMessage qm(QueryMessage::ADD, QueryMessage::VFIELD);
    qm << field;
    std::vector<query_result> futures;
    queryobj.update_all(qm.message(), qm.length(), futures);

    for (size_t i = 0; i < futures.size(); ++i) {
      int error = queryobj.parse_reply(futures[i]);
      if (error != 0)
        return error;
    }
    return 0;
  }

  const std::vector<graph_field> graphdb_client::get_edge_fields() {
    QueryMessage qm(QueryMessage::GET, QueryMessage::EFIELD);
    query_result future = queryobj.query_any(qm.message(), qm.length());
    std::vector<graph_field> ret;
    ASSERT_EQ(queryobj.parse_reply(future, ret), 0);
    return ret;
  }

  const std::vector<graph_field> graphdb_client::get_vertex_fields() {
    QueryMessage qm(QueryMessage::GET, QueryMessage::VFIELD);
    query_result future = queryobj.query_any(qm.message(), qm.length());
    std::vector<graph_field> ret;
    ASSERT_EQ(queryobj.parse_reply(future, ret), 0);
    return ret;
  }

  int graphdb_client::add_vertex_mirror(graph_vid_t vid, const std::vector<graph_shard_id_t>& mirrors) {
    QueryMessage qm(QueryMessage::ADD, QueryMessage::VMIRROR);
    qm << vid << mirrors;
    query_result future = queryobj.update(shard_manager.get_master(vid), qm.message(), qm.length());
    int errorcode = queryobj.parse_reply(future);
    return errorcode;
  }


  // --------------- Helper functions -----------------------
  graph_shard_id_t graphdb_client::eid2shard(const graph_eid_t& eid) { 
    return split_eid(eid).first;
  }

  graph_shard_id_t graphdb_client::vid2shard(const graph_vid_t& vid) {
    return shard_manager.get_master(vid);
  }

  graph_shard_id_t graphdb_client::edge2shard(const std::pair<graph_vid_t, graph_vid_t>& edge) {
    return shard_manager.get_master(edge.first, edge.second);
  }

  graph_shard_id_t graphdb_client::vin2shard(const vertex_insert_descriptor& des) {
    return shard_manager.get_master(des.vid);
  }

  graph_shard_id_t graphdb_client::ein2shard(const edge_insert_descriptor& des) {
    return shard_manager.get_master(des.src, des.dest);
  }

  graphdb_client::mirror_table_type graphdb_client::mirror_table_from_edges (const std::vector<edge_insert_descriptor>& edges) {
    mirror_table_type map;
    for (size_t i = 0; i < edges.size(); ++i) {
      graph_shard_id_t target = ein2shard(edges[i]); 
      map[edges[i].src].insert(target);
      map[edges[i].dest].insert(target);
    }
    return map;
  }
} // end of graphlab namespace
