#ifndef GRAPHLAB_DATABASE_GRAPH_VALUE_HPP
#define GRAPHLAB_DATABASE_GRAPH_VALUE_HPP
#include <graphlab/database/basic_types.hpp>
#include <graphlab/serialization/iarchive.hpp>
#include <graphlab/serialization/oarchive.hpp>
#include <graphlab/database/basic_types.hpp>
#include <boost/lexical_cast.hpp>
#include <cstdlib>
#include <cstring>
namespace graphlab {
/**
 * \ingroup group_graph_database
 * This struct stores the value in a single field in a single vertex/edge 
 * of the graph. 
 *
 * \note
 *  This struct is intentionally make fully public to allows the graph_value 
 *  type to be used natively in the database implementations easily.
 *
 */
class graph_value {
 public:
  /// Default construction creates a null integer
  graph_value();

  /// Default construction creates a null with given type 
  graph_value(graph_datatypes_enum);

  /// Copy constructor
   graph_value(const graph_value& other);

  /// Copy assignment 
  graph_value& operator=(const graph_value& other); 


      
  /// Initialize the data with given type and NULL value.
  void init(graph_datatypes_enum type);

  /// Destructor. Frees the data if it is a string / blob
  ~graph_value();

 public:
  union value_union_type {
    graph_vid_t vid_value;
    graph_int_t int_value;
    graph_double_t double_value;
    char* bytes;
  };

  /**  the primary storage is a union between the scalar types and a 
   *   void* pointer. If the data is a string / blob, the "bytes" field
   *   is used and it contains a pointer to the actual data and \ref _len 
   *   is the length of the data. The destructor automatically frees the
   *   the data if the datatype is is a string / blob.
   */
  value_union_type _data;
 
  /** The number of bytes needed to represent the data. If a scalar type,
   *  this is the number of bytes needed to represent the scalar type. Otherwise,
   *  for a string/blob, this ithe length of the data. Note that a 0 length
   *  blob is not the same as a NULL
   */
  size_t _len;

  /// The type of the data 
  graph_datatypes_enum _type;

  /// If true, this is a null value and the data field is ignored
  bool _null_value;

  
  /// Frees the data pointer resetting it to NULL if it is a string / blob.
  void free_data();

  /** The number of bytes needed to represent the data. If a scalar type,
   *  this is the number of bytes needed to represent the scalar type. Otherwise,
   *  for a string/blob, this ithe length of the data. Note that a 0 length
   *  blob is not the same as a NULL
   */
  inline size_t data_length() const {
    return _len;
  }

  /// Returns true if the object is NULL value
  inline bool is_null() const {
    return _null_value;
  }

  /** Returns a constant raw pointer to the data. Returns NULL if data is NULL.
   *  Realloc or free should not be called on the returned pointer.
   *  Modifications to the data also should not be made.
   */
  const void* get_raw_pointer() const;

  /** Returns the raw pointer to the data. Returns NULL if data is NULL.
   *  Realloc or free should not be called on the returned pointer.
   *  Modifications to the data may be made.
   */
  void* get_mutable_raw_pointer();

  /// Returns the datatype of the entry
  inline graph_datatypes_enum type() const {
    return _type;
  }
 
  /**
   * Returns the value as a VID type in the out_ret argument.
   * Returns true on success. Returns false if the data
   * is not a VID type, or the data is NULL.
   */
  bool get_vid(graph_vid_t* out_ret) const; 

  /**
   * Returns the value as a integer type in the out_ret argument.
   * Returns true on success. Returns false if the data
   * is not a integer type, or the data is NULL.
   */
  bool get_integer(graph_int_t* out_ret) const; 

  /**
   * Returns the value as a double type in the out_ret argument.
   * Returns true on success. Returns false if the data
   * is not a double type, or the data is NULL.
   */
  bool get_double(graph_double_t* out_ret) const;


  /**
   * Returns the value as a string type in the out_ret argument.
   * Returns true on success. Returns false if the data
   * is not a string type, or the data is NULL.
   */
  bool get_string(graph_string_t* out_ret) const;

  /**
   * Returns the value as a blob type in the out_ret argument.
   * Returns true on success. Returns false if the data
   * is not a blob type, or the data is NULL.
   */
  bool get_blob(graph_blob_t* out_ret) const;


  /**
   * Returns a copy of the first len bytes in the out_blob argument.
   * out_blob must point to a memory region with at least len bytes
   * available.  The length of the blob can be obtained from the 
   * \ref data_length() function. If len exceeds the actual length of the blob,
   * only the actual length of the blob will be copied. 
   * Returns true on success. Returns false if the data
   * is not a blob type, or the data is NULL.
   */
  bool get_blob(size_t len, char* out_blob) const;

  /**
   * Sets the value of an integer field to the provided argument.
   * Also, sets the modification flag if the integer value is different.
   * Returns true on success. Returns false if the data
   * is not an integer type, or the data is NULL.
   */
  bool set_integer(graph_int_t val, bool is_delta = false);

  /**
   * Sets the value of an double field to the provided argument.
   * Also, sets the modification flag if the double value is different.
   * Returns true on success. Returns false if the data
   * is not an double type, or the data is NULL.
   */
  bool set_double(graph_double_t val, bool is_delta = false);

  /**
   * Sets the value of an vid field to the provided argument.
   * Also, sets the modification flag if the vid value is different.
   * Returns true on success. Returns false if the data
   * is not an double type, or the data is NULL.
   */
  bool set_vid (graph_vid_t val);

  /**
   * Sets the value of an string field to the provided argument.
   * Also, sets the modification flag if the string value is different.
   * Returns true on success. Returns false if the data
   * is not an string type, or the data is NULL.
   */
  bool set_string(const graph_string_t& val);


  /**
   * Sets the value of a blob field to the provided argument.
   * Returns true on success. Returns false if the data
   * is not a string type, or the data is NULL.
   */
  bool set_blob(const graph_blob_t& val);

  /**
   * Sets the value of a blob field to the provided argument.
   * Returns true on success. Returns false if the data
   * is not a blob type, or the data is NULL.
   */
  bool set_blob(const char* val, size_t length);

  /**
   * Sets the value field to the provided argument based on the type and delta commit flag.
   * Returns true on success. Returns false if the data
   * cannot be cast to the matching type, or the data is NULL.
   */
  bool set_val(const graph_value& val, bool delta=false);

  /**
   * Sets the value field to the provided argument based on the type and delta commit flag.
   * Returns true on success. Returns false if the data
   * cannot be cast to the matching type, or the data is NULL.
   */
  bool set_val(const char* val, size_t length, bool delta=false);

  /* 
   * Sets the value field to the argument (ascii representation) based on the type and delta commit flag.
   * Returns true on success. Returns false if the data
   * cannot be cast to the matching type, or the data is NULL.
   */
  bool set_val(const std::string& val_str, bool delta = false);

  // Subtract other's value from this. Only have effect on scalar type values.
  void diff(const graph_value& other, graph_value& out_delta);

  /**
   * Serialization interface. 
   */
  inline void save(oarchive& oarc) const {
    oarc << _type << _null_value << _len;
    if (!_null_value) {
      if (is_scalar_graph_datatype(_type)) {
        oarc.write((char*)(&_data), _len);
      } else {
        oarc.write(_data.bytes, _len);
      }
    }
  }

  /**
   * Serialization interface.
   */
  inline void load(iarchive& iarc) {
    iarc >> _type >> _null_value >> _len;
     if (!_null_value) {
       if (is_scalar_graph_datatype(_type)) {
          iarc.read((char*)(&_data), _len);
       } else {
         if (_data.bytes == NULL) {
            _data.bytes = (char*)malloc(_len);
         } else {
            _data.bytes = (char*)realloc(_data.bytes, _len);
         }
          iarc.read(_data.bytes, _len);
       }
     }
  }

 private:
  // output the string format to ostream.
  friend std::ostream& operator<<(std::ostream &strm, const graph_value& v) {
    std::string value_str; 
    if (v.is_null()) {
      value_str = "NULL";
    } else {
      switch (v._type) {
       case VID_TYPE: value_str = boost::lexical_cast<std::string> (v._data.vid_value); break;
       case INT_TYPE: value_str = boost::lexical_cast<std::string> (v._data.int_value); break;
       case DOUBLE_TYPE: value_str = boost::lexical_cast<std::string> (v._data.double_value); break;
       case STRING_TYPE: value_str = std::string(v._data.bytes); break; 
       default: value_str = "***";
      }
    }
    strm << value_str << ": " << graph_datatypes_string[v._type];
    return strm;
  }
 };
} // namespace graphlab
#endif
