#ifndef HKS_H
#define HKS_H

#include <stddef.h>
#include <string.h>
#include <limits.h>

#if defined(LUACOD_T7)
#define LUA_OFFS_STKTOP       0x48 /* Lua stack top */
#define LUA_OFFS_STKBASE      0x50 /* Lua stack base */
#define LUA_OFFS_STKALLOCTOP  0x58 /* Lua stack limit */
#define LUA_OFFS_STKBOTTOM    0x60 /* Lua stack bottom */
#define LUA_OFFS_GLOBAL       16   /* Lua global_State */
#define GLOBAL_OFFS_ENDIANNESS   0x5b0 /* bytecode endianness */
#define GLOBAL_OFFS_SHARING      0x1d8 /* bytecode sharing mode */
#elif defined(LUACOD_T6)
#define LUA_OFFS_STKTOP       0x24 /* Lua stack top */
#define LUA_OFFS_STKBASE      0x28 /* Lua stack base */
#define LUA_OFFS_STKALLOCTOP  0x2c /* Lua stack limit */
#define LUA_OFFS_STKBOTTOM    0x30 /* Lua stack bottom */
#define LUA_OFFS_GLOBAL       8    /* Lua global_State */
#define GLOBAL_OFFS_ENDIANNESS   0x304 /* bytecode endianness */
#define GLOBAL_OFFS_SHARING      0x11c /* bytecode sharing mode */
#else
#error "Unsupport CoD title"
#endif

#define LUA_ERRERR (-300)
#define LUA_ERRMEM (-200)
#define LUA_ERRRUN (-100)
#define LUA_ERRFILE (-5)
#define LUA_ERRSYNTAX (-4)

#define LUA_REGISTRYINDEX (-10000)
#define LUA_ENVIRONINDEX  (-10001)
#define LUA_GLOBALSINDEX  (-10002)
#define lua_upvalueindex(i) (LUA_GLOBALSINDEX-(i))

/* bytecode sharing formats */
#define BYTECODE_DEFAULT 0
#define BYTECODE_INPLACE 1
#define BYTECODE_REFERENCED 2

/* bytecode sharing modes */
#define HKS_BYTECODE_SHARING_OFF 0
#define HKS_BYTECODE_SHARING_ON 1
#define HKS_BYTECODE_SHARING_SECURE 2

/* bytecode stripping levels */
#define BYTECODE_STRIPPING_NONE 0
#define BYTECODE_STRIPPING_PROFILING 1
#define BYTECODE_STRIPPING_ALL 2
#define BYTECODE_STRIPPING_DEBUG_ONLY 3
#define BYTECODE_STRIPPING_CALLSTACK_RECONSTRUCTION 4
#define NUM_STRIP_LEVELS 5

/* int literal options */
#define INT_LITERALS_NONE 0
#define INT_LITERALS_LUD 1
#define INT_LITERALS_UI64 2
#define INT_LITERALS_ALL 3

/* bytecode endianness */
#define HKS_BYTECODE_DEFAULT_ENDIAN 0
#define HKS_BYTECODE_BIG_ENDIAN 1
#define HKS_BYTECODE_LITTLE_ENDIAN 2

typedef struct lua_State lua_State;

typedef int (*lua_CFunction) (lua_State *L);

#if INT_MAX-20 < 32760
#define LUAI_BITS_INT 16
#elif INT_MAX > 2147483640L
/* int has at least 32 bits */
#define LUAI_BITS_INT 32
#else
#error "you must define LUA_BITSINT with number of bits in an integer"
#endif

#if LUAI_BITS_INT >= 32
typedef unsigned int lu_int32;
typedef int l_int32;
#else
typedef unsigned long lu_int32;
typedef long l_int32;
#endif

typedef lu_int32 Instruction;

/*
** functions that read/write blocks when loading/dumping Lua chunks
*/
typedef const char * (*lua_Reader) (lua_State *L, void *ud, size_t *sz);

typedef int (*lua_Writer) (lua_State *L, const void* p, size_t sz, void* ud);


/*
** prototype for memory-allocation functions
*/
typedef void * (*lua_Alloc) (void *ud, void *ptr, size_t osize, size_t nsize);

typedef void (*lua_Caller) (lua_State *s, void *ud, int nresults,
                            Instruction *instr);

typedef struct HksCompilerSettings HksCompilerSettings;

typedef int (*hks_debug_map)(const char *, int);

struct HksCompilerSettings {
  int emit_struct;
  const char ** stripnames;
  int sharing;
  int literals;
  hks_debug_map debugmap;
};

static int hks_identity_map(const char *filename, int line) {
  (void)filename;
  return line;
}

/*
** basic types
*/
#define LUA_TANY    (-2)
#define LUA_TNONE   (-1)

#define LUA_TNIL    0
#define LUA_TBOOLEAN    1
#define LUA_TLIGHTUSERDATA  2
#define LUA_TNUMBER   3
#define LUA_TSTRING   4
#define LUA_TTABLE    5
#define LUA_TFUNCTION   6
#define LUA_TUSERDATA   7
#define LUA_TTHREAD   8
#define LUA_TIFUNCTION  9
#define LUA_TCFUNCTION  10
#define LUA_TUI64       11
#define LUA_TSTRUCT     12

#define LUA_NUM_TYPE_OBJECTS 14


#define GenericChunkHeader size_t flags

typedef struct ChunkHeader {
  GenericChunkHeader;
  struct ChunkHeader *next;
} ChunkHeader;

typedef struct InternString {
  GenericChunkHeader;
  size_t lengthbits;
  lu_int32 hash;
  char data[30];
} InternString;

typedef struct cclosure cclosure;

typedef union HksValue {
  cclosure *cClosure;
  InternString *str;
  void *ptr;
  lua_State *thread;
} HksValue;

typedef struct HksObject {
  unsigned int t;
  union HksValue v;
} HksObject;

typedef HksObject *StkId;

static const HksObject luaO_nilobject_ = {LUA_TNIL, {NULL}};
#define luaO_nilobject &luaO_nilobject_

#define cast_int(i) ((int)(i))

#define cast_obj(exp) ((HksObject *)(exp))
#define structoffs(s,offset,t) (*(t*)(((char *)(s)) + (offset)))

#define stktop(L)       structoffs(L, LUA_OFFS_STKTOP,      StkId)
#define stkbase(L)      structoffs(L, LUA_OFFS_STKBASE,     StkId)
#define stkalloctop(L)  structoffs(L, LUA_OFFS_STKALLOCTOP, StkId)
#define stkbottom(L)    structoffs(L, LUA_OFFS_STKBOTTOM,   StkId)

#define dectop(L)       (stktop(L) = stktop(L) - 1)

#define G(L)  structoffs(L,LUA_OFFS_GLOBAL, char *)

/* bytecode sharing mode */
#define sharing_mode(L) structoffs(G(L), GLOBAL_OFFS_SHARING, int)
/* bytecode endianness */
#define bytecode_endianness(L) structoffs(G(L), GLOBAL_OFFS_ENDIANNESS, int)

#define getstr(ts) ((const char *)((ts)->data))
#define svalue(o) getstr((o)->v.str)

/* Macros to test type */
#define ttisnil(o)  (ttype(o) == LUA_TNIL)
#define ttisnumber(o)  (ttype(o) == LUA_TNUMBER)
#define ttisstring(o)  (ttype(o) == LUA_TSTRING)
#define ttistable(o)  (ttype(o) == LUA_TTABLE)
#define ttisfunction(o)  (ttype(o) == LUA_TFUNCTION)
#define ttisboolean(o)  (ttype(o) == LUA_TBOOLEAN)
#define ttisuserdata(o)  (ttype(o) == LUA_TUSERDATA)
#define ttisthread(o)  (ttype(o) == LUA_TTHREAD)
#define ttislightuserdata(o)  (ttype(o) == LUA_TLIGHTUSERDATA)
#define ttisui64(o)  (ttype(o) == LUA_TUI64)
#define ttisstruct(o)  (ttype(o) == LUA_TSTRUCT)
#define ttisifunction(o)  (ttype(o) == LUA_TIFUNCTION)
#define ttiscfunction(o)  (ttype(o) == LUA_TCFUNCTION)

/* Macros to access values */
#define ttype(o)  ((o)->t)

static int lua_gettop (lua_State *L) {
  return cast_int(stktop(L) - stkbase(L));
}

static HksObject *index2adr(lua_State *L, int idx) {
  if (idx > 0) {
    HksObject *o = stkbase(L) + (idx - 1);
    if (o >= stktop(L)) return cast_obj(luaO_nilobject);
    else return o;
  }
  else if (idx > LUA_REGISTRYINDEX) {
    return stktop(L) + idx;
  }
  else return cast_obj(luaO_nilobject);
}

static const char *lua_tolstring(lua_State *L, int idx, size_t *len) {
  StkId o = index2adr(L, idx);
  if (!ttisstring(o))
    return NULL;
  if (len != NULL) *len = o->v.str->lengthbits;
  return svalue(o);
}

#endif /* HKS_H */
