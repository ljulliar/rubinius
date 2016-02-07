#ifndef RBX_REGEXP_HPP
#define RBX_REGEXP_HPP

#include "object_utils.hpp"

#include "builtin/lookup_table.hpp"
#include "builtin/object.hpp"
#include "builtin/string.hpp"
#include "builtin/tuple.hpp"

// HACK gross.
// Forward declare ONLY if we haven't already included oniguruma.h
// We do this because onigurama seems to have regex_t be a weird
// typedef. It's easier to just not bother with trying to duplicate
// what it does and do this.
#ifndef ONIGURUMA_H
struct regex_t;
#endif

namespace rubinius {
  class Encoding;
  class String;
  class Tuple;
  class LookupTable;
  class Regexp;

  // Cache up to 4 entries, for ASCII, Binary, UTF-8
  // and other encodings.
  const static int cCachedOnigDatas = 4;

  class MatchData : public Object {
  public:
    const static object_type type = MatchDataType;

  private:
    String* source_; // slot
    Regexp* regexp_; // slot
    Tuple* full_;    // slot
    Tuple* region_;  // slot

  public:
    /* accessors */

    attr_accessor(source, String);
    attr_accessor(regexp, Regexp);
    attr_accessor(full, Tuple);
    attr_accessor(region, Tuple);

    String* matched_string(STATE);
    String* pre_matched(STATE);
    String* post_matched(STATE);
    String* last_capture(STATE);
    String* nth_capture(STATE, native_int which);

    /* interface */
    static void initialize(STATE, MatchData* obj) {
      obj->source_ = nil<String>();
      obj->regexp_ = nil<Regexp>();
      obj->full_ = nil<Tuple>();
      obj->region_ = nil<Tuple>();
    }

    class Info : public TypeInfo {
    public:
      BASIC_TYPEINFO(TypeInfo)
    };
  };

  class Regexp : public Object {
  public:
    const static object_type type = RegexpType;

  private:
    String* source_;     // slot
    LookupTable* names_; // slot
    regex_t* onig_data[cCachedOnigDatas];
    utilities::thread::SpinLock lock_;
    bool fixed_encoding_;
    bool no_encoding_;

  public:
    /* accessors */

    attr_accessor(source, String);
    attr_accessor(names, LookupTable);

    /* interface */

    static void bootstrap(STATE);
    static void initialize(STATE, Regexp* obj) {
      obj->source_ = nil<String>();
      obj->names_ = nil<LookupTable>();

      for(int i = 0; i < cCachedOnigDatas; ++i) {
        obj->onig_data[i] = NULL;
      }

      obj->lock_.init();
      obj->fixed_encoding_ = false;
      obj->no_encoding_ = false;
    }

    static Regexp* create(STATE);
    static char*  version(STATE);

    /* The Regexp options bit flags fit well within a Fixnum. If more
     * options are added, this should be double-checked.
     */
    // Rubinius.primitive :regexp_initialize
    Regexp* initialize(STATE, String* pattern, Fixnum* options);
    regex_t* maybe_recompile(STATE, String* string);

    // Rubinius.primitive :regexp_options
    Fixnum* options(STATE);

    // Rubinius.primitive+ :regexp_fixed_encoding_p
    Object* fixed_encoding_p(STATE);

    // Rubinius.primitive :regexp_search_region
    MatchData* match_region(STATE, String* string, Fixnum* start, Fixnum* end, Object* forward);

    // Rubinius.primitive :regexp_match_start
    MatchData* match_start(STATE, String* string, Fixnum* start);

    // Rubinius.primitive :regexp_search_from
    MatchData* search_from(STATE, String* string, Fixnum* start);

    // Rubinius.primitive :regexp_allocate
    static Regexp* allocate(STATE, Object* self);

    // Rubinius.primitive :regexp_last_match_result
    static Object* last_match_result(STATE, Fixnum* mode, Fixnum* which, CallFrame* calling_environment);

    // Rubinius.primitive :regexp_last_match
    static Object* last_match(STATE, Arguments& args, CallFrame* calling_environment);

    // Rubinius.primitive :regexp_set_last_match
    static Object* set_last_match(STATE, Object* obj, CallFrame* calling_environment);

    // Rubinius.primitive :regexp_propagate_last_match
    static Object* propagate_last_match(STATE, CallFrame* calling_environment);

    // Rubinius.primitive :regexp_set_block_last_match
    static Object* set_block_last_match(STATE, CallFrame* calling_environment);

    // Rubinius.primitive+ :regexp_encoding
    Encoding* encoding(STATE);

    Encoding* encoding(STATE, Encoding* enc);

    regex_t* make_managed(STATE, Encoding* enc, regex_t* reg);
    regex_t* onig_source_data(STATE);
    regex_t* onig_data_encoded(STATE, Encoding* enc);

    class Info : public TypeInfo {
    public:
      Info(object_type type) : TypeInfo(type) { }
      virtual void mark(Object* obj, memory::ObjectMark& mark);
      virtual void auto_mark(Object* obj, memory::ObjectMark& mark);
      virtual void populate_slot_locations();
      virtual void set_field(STATE, Object*, size_t, Object*);
      virtual Object* get_field(STATE, Object*, size_t);
    };

    friend class Info;

  };

}

#endif
