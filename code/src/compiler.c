/*
** compiler.c
** Call of Duty T7 Lua compiler interface
*/


#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <sys/stat.h>

#ifndef LUACOD_T7
# define LUACOD_T7
#endif /* LUACOD_T7 */

#include "hks.h"

/* log file name */
#define DEBUGFILENAME "debug.log"
#define ERRORFILENAME "error.log"
/* test folder name */
#define TESTDIRNAME "tests"

/* match code */
#define MODS_BASE_PATH "mods"

/* == DATA ================================================================== */

static const char *modname; /* internal mod name */

/* == END DATA ============================================================== */


/* == SYMBOLS =============================================================== */

/* hksi_hksL_loadbuffer */
typedef int (__fastcall* t_hksi_hksL_loadbuffer)(lua_State *s,
  HksCompilerSettings *options, const char *buff, size_t sz, const char *name);
static t_hksi_hksL_loadbuffer p_hksi_hksL_loadbuffer;
#define hksi_hksL_loadbuffer(s,o,b,sz,n) \
  ((*p_hksi_hksL_loadbuffer)((s),(o),(b),(sz),(n)))

/* hksi_hks_dump */
typedef int (__fastcall *t_hksi_hks_dump)(lua_State *s, lua_Writer writer,
                                          void *data, int strippingLevel);
static t_hksi_hks_dump p_hksi_hks_dump;
#define hksi_hks_dump(s,w,d,l) ((*p_hksi_hks_dump)((s),(w),(d),(l)))

/* lua_pushfstring, lua_pushvfstring */
typedef int (__fastcall *t_lua_pushfstring)(lua_State *s, const char *fmt, ...);
static t_lua_pushfstring p_lua_pushfstring;
#define lua_pushfstring (*p_lua_pushfstring)
typedef int (__fastcall *t_lua_pushvfstring)(lua_State *s, const char *fmt,
                                             va_list argp);
static t_lua_pushvfstring p_lua_pushvfstring;
#define lua_pushvfstring(s,fmt,argp) ((*p_lua_pushvfstring)((s),(fmt),(argp)))

/* luaL_error */
typedef int (__fastcall *t_luaL_error)(lua_State *s, const char *fmt, ...);
static t_luaL_error p_luaL_error;
#define lua_error (*p_luaL_error)

/* hks_error */
typedef int (__fastcall *t_hks_error)(lua_State *s, int errcode);
static t_hks_error p_hks_error;
#define lua_throw(s,e) ((*p_hks_error)((s),(e)))

/* luaD_pcall */
typedef int (__fastcall *t_luaD_pcall)(lua_State *s, lua_Caller f, void *arg,
                                       int numResults);
static t_luaD_pcall p_luaD_pcall;
#define luaD_pcall(s,f,u,n)  ((*p_luaD_pcall)((s),(f),(u),(n)))


/* == END SYMBOLS =========================================================== */

/* not necessarily THE luaVM, just whatever state called the last C function */
static lua_State *luaVM = NULL;

/******************************************************************************/
/* assertions */
/******************************************************************************/
#if defined(__cplusplus) || \
  (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L)
# define FUNCTION __func__
#elif (defined(__GNUC__) && __GNUC__ >= 2) || defined(_MSC_VER)
# define FUNCTION __FUNCTION__
#else
# define FUNCTION "<unknown>"
#endif

/* only pass literal strings */
#define lassertmsg(cond, msg) \
  if (!(cond)) lua_error(luaVM, "%s%sAssertion failed: (%s), function %s, " \
  "file %s, line %d.", "" msg, sizeof(msg) == 1 ? "" : "\n", #cond, FUNCTION, \
  __FILE__, __LINE__)

#define lassert(cond) lassertmsg(cond, "")

/******************************************************************************/
/* logging functions */
/******************************************************************************/

#ifdef WITH_LOGGING
static FILE *fdebuglog = NULL;

static const char *currfunc = NULL;
static int currline = 0;

static void vdebuglog_ (const char *fmt, va_list argp) {
  if (fdebuglog) {
    lassert(currfunc != NULL && currline > 0);
    fprintf(fdebuglog, "[%s:%-3d] (%s) ", __FILE__, currline, currfunc);
    vfprintf(fdebuglog, fmt, argp);
    fputc('\n', fdebuglog);
    fflush(fdebuglog);
  }
}

static void debuglog_ (const char *fmt, ...) {
  va_list argp;
  va_start(argp, fmt);
  vdebuglog_(fmt, argp);
  va_end(argp);
}

/* workaround when variadic macros are not supported */
#define debuglog ; \
  currfunc = FUNCTION; \
  currline = __LINE__; \
  debuglog_

#define vdebuglog(fmt, argp) do { \
  currfunc = FUNCTION; \
  currline = __LINE__; \
  vdebuglog_(fmt, argp); \
} while(0)

#else
#define debuglog (void)
#define vdebuglog(fmt,argp)
#endif

/* compiler error logging */
static FILE *ferrorlog = NULL;

static void verrorlog (const char *fmt, va_list argp) {
  if (ferrorlog) {
    fputs("[ERROR] ", ferrorlog);
    vfprintf(ferrorlog, fmt, argp);
    fputc('\n', ferrorlog);
    fflush(ferrorlog);
  }
}

static void errorlog (const char *fmt, ...) {
  va_list argp;
  va_start(argp, fmt);
  verrorlog(fmt, argp);
  va_end(argp);
}


#define OFFS(x) ((uintptr_t)GetModuleHandle(NULL) + x)
#define INIT_DATA(name,x,t) name = (t)OFFS(x)
#define INIT_SYMBOL(name,x) p_##name = (t_##name)OFFS(x)

/* Initialize symbols in Black Ops III */
static void init_symbols(void) {
  /* data */
  INIT_DATA(modname, 0x1678ed04, const char *);
  /* symbols */
  INIT_SYMBOL(hksi_hksL_loadbuffer,   0x1d4bd80);
  INIT_SYMBOL(hksi_hks_dump,          0x1d4be40);
  INIT_SYMBOL(lua_pushfstring,        0x1d4e600);
  INIT_SYMBOL(lua_pushvfstring,       0x1d4e630);
  INIT_SYMBOL(luaL_error,             0x1d53050);
  INIT_SYMBOL(hks_error,              0x1d4c060);
  INIT_SYMBOL(luaD_pcall,             0x1d6a320);
}

/******************************************************************************/
/* xmalloc functions */
/******************************************************************************/
static void *xrealloc(void *oldptr, size_t n) {
  void *newptr = realloc(oldptr, n);
  if (newptr == NULL) {
    if (oldptr)
      lua_pushfstring(luaVM,"error: cannot reallocate %zu bytes for block %p\n",
                      n, oldptr);
    else
      lua_pushfstring(luaVM, "error: cannot allocate %zu bytes\n", n);
    lua_throw(luaVM, LUA_ERRMEM);
  }
  return newptr;
}

static void *xmalloc(size_t n) {
  return xrealloc(NULL, n);
}

/******************************************************************************/
/* Lua parsing and dumping */
/******************************************************************************/

/* lua_Writer for dumping precompiled chunks to a file */
static int writer(lua_State *L, const void *p, size_t sz, void *ud) {
  size_t size;
  FILE *f = (FILE *)ud;
  (void)L;
  size = fwrite(p, 1, sz, f);
  /*debuglog("writer called,  %zu in, %zu written", sz, size);*/
  return (size > 0) ? 0 : 1;
}


#define logtopmsg(L) do { \
  const char *msg_ = lua_tolstring((L), -1, NULL); \
  if (msg_ != NULL) errorlog("%s", msg_); } while (0)


/* see if a chunk can be loaded by Call of Duty (does not execute) */
static int check_chunk_loads(const char *buff, size_t size, const char *name){
  int status, mode, endianness;
  HksCompilerSettings settings;
  settings.emit_struct = 0;
  settings.stripnames = NULL;
  settings.sharing = BYTECODE_DEFAULT;
  settings.literals = INT_LITERALS_ALL;
  settings.debugmap = hks_identity_map;
  mode = sharing_mode(luaVM);
  sharing_mode(luaVM) = HKS_BYTECODE_SHARING_ON;
  endianness = bytecode_endianness(luaVM);
  bytecode_endianness(luaVM) = HKS_BYTECODE_DEFAULT_ENDIAN;
  status = hksi_hksL_loadbuffer(luaVM, &settings, buff, size, name);
  if (status) { /* parse error */
    logtopmsg(luaVM);
    goto err;
  }
  err:
  debuglog("returning with code %d", status);
  dectop(luaVM);
  bytecode_endianness(luaVM) = endianness;
  sharing_mode(luaVM) = mode;
  return status == 0;
}

#define NUM_OUTPUTS 3 /* bytecode, callstackdb, and debug info */

static int striplevels[NUM_OUTPUTS] = {
  BYTECODE_STRIPPING_ALL,
  BYTECODE_STRIPPING_CALLSTACK_RECONSTRUCTION,
  BYTECODE_STRIPPING_DEBUG_ONLY
};

static const char *const striplevels_suffixes[NUM_OUTPUTS] = {
  "c", /* .luac */
  "callstackdb", /* .luacallstackdb */
  "debug" /* .luadebug */
};

static const char *const striplevels_what[NUM_OUTPUTS] = {
  "bytecode", /* stripped */
  "callstack reconstruction", /* callstackdb */
  "debug information" /* debug info */
};

/* parse and dump a Lua file */
static int precompile_lua(const char *buff, size_t size, const char *source,
                          char *outputnames[NUM_OUTPUTS]) {
  int i, status, mode, endianness;
  HksCompilerSettings settings;
  settings.emit_struct = 0;
  settings.stripnames = NULL;
  settings.sharing = BYTECODE_INPLACE;
  settings.literals = INT_LITERALS_ALL;
  settings.debugmap = hks_identity_map;
  mode = sharing_mode(luaVM);
  sharing_mode(luaVM) = HKS_BYTECODE_SHARING_OFF; /* optimize bytecode */
  endianness = bytecode_endianness(luaVM);
  bytecode_endianness(luaVM) = HKS_BYTECODE_DEFAULT_ENDIAN;
  status = hksi_hksL_loadbuffer(luaVM, &settings, buff, size, source);
  if (status) { /* parse error */
    logtopmsg(luaVM);
    goto err;
  }
  for (i = 0; i < NUM_OUTPUTS; i++) { /* dump bytecode */
    FILE *f = fopen(outputnames[i], "wb");
    if (!f) {
      debuglog("cannot open file '%s'", outputnames[i]);
      goto err;
    }
    debuglog("dumping %s to file '%s'", striplevels_what[i], outputnames[i]);
    status = hksi_hks_dump(luaVM, writer, f, striplevels[i]);
    fclose(f);
    if (status) { /* dump error */
      logtopmsg(luaVM);
      debuglog("hksi_hks_dump(L, writer, f, striplevels[%d]) returned "
               "status %d", i, status);
      goto err;
    }
  }
err:
  debuglog("returning with code %d", status);
  dectop(luaVM);
  bytecode_endianness(luaVM) = endianness;
  sharing_mode(luaVM) = mode;
  return status;
}

/******************************************************************************/
/* Lua compilation test execution */
/******************************************************************************/

/* load a file's contents into memory */
static char *loadfile(const char *name, size_t *size) {
  struct _stat st;
  FILE *f;
  size_t fsize;
  char *buff;
  if (_stat(name, &st)) return NULL;
  f = fopen(name, "rb");
  if (f == NULL) return NULL;
  fsize = st.st_size;
  buff = xmalloc(fsize+1);
  fread(buff, 1, fsize, f);
  fclose(f);
  buff[fsize] = '\0';
  *size = fsize;
  return buff;
}

static void loadfilechunk(const char *filename) {
  size_t size;
  char *buff = loadfile(filename, &size);
  if (check_chunk_loads(buff, size, filename)) {
    debuglog("'%s' loaded successfully", filename);
  } else {
    debuglog("'%s' was not loaded successfully", filename);
  }
}

/* execute a compilation test */
static void compilefile(const char *name) {
  int i, status;
  char *chunkname;
  char *buff; /* loaded file contents */
  size_t size; /* size of loaded file contents */
  char *names[NUM_OUTPUTS]; /* output file names */
  size_t sizes[NUM_OUTPUTS]; /* name sizes of output file names */
  size_t sizetotal; /* total size needed for all names */
  size_t namelen = strlen(name);
  sizetotal = namelen+2; /* space for chunk name '@<name>' */
  for (i = 0; i < 3; i++) { /* calculate name sizes */
    sizes[i] = namelen + strlen(striplevels_suffixes[i]) + 1;
    sizetotal += sizes[i];
  }
  names[0] = xmalloc(sizetotal);
  for (i = 1; i < NUM_OUTPUTS; i++) /* initialize name pointers */
    names[i] = names[i-1] + sizes[i-1];
  for (i = 0; i < NUM_OUTPUTS; i++) { /* write names */
    strcpy(names[i], name);
    strcpy(names[i]+namelen, striplevels_suffixes[i]);
  }
  chunkname = names[i-1] + sizes[i-1];
  *chunkname = '@';
  strcpy(chunkname+1,name); /* chunk name '@<file>' */
  buff = loadfile(name, &size); /* load file */
  debuglog("calling precompile_lua(%p, %zu, \"%s\", files", buff, size,
           chunkname);
  status = precompile_lua(buff, size, chunkname, names); /* compile and dump */
  free(names[0]);
  free(buff);
}


/******************************************************************************/
/* utilities for searching/operating on files in a directory tree */
/******************************************************************************/

#define MAXEXT 32 /* arbitrary limit on the length of a file extension */

/* returns whether a file name has a given extension (C89, case-insensative) */
static int hasext(const char *name, size_t namelen, const char *ext) {
  char buff[MAXEXT];
  size_t n, extlen = strlen(ext);
  lassert(extlen < MAXEXT);
  if (namelen < extlen || extlen < 2) return 0;
  for (n = 0; n < extlen; n++)
    buff[n]=tolower(name[namelen-(extlen-n)]);
  buff[n]='\0';
  return (strcmp(buff, ext) == 0);
}

/* action to perform with a single file */
typedef void (*filefunc)(const char *name);

/* context needed by the recursive search function */
typedef struct SearchFileParams {
  const char *ext; /* file extension to match */
  char *buff; /* search pattern string */
  size_t len; /* length of search pattern string */
  size_t alloc; /* size of buff */
  filefunc action; /* function to call on matches */
} SearchFileParams;

static void dofiles(SearchFileParams *sfp);

static void f_dofiles(lua_State *s, void *ud, int nresults, Instruction *instr){
  SearchFileParams *sfp;
  (void)s; (void)nresults; (void)instr;
  sfp = (SearchFileParams *)ud;
  dofiles(sfp);
}

static void f_dofileaction(lua_State *s, void *ud, int nresults,
                           Instruction *instr) {
  SearchFileParams *sfp = (SearchFileParams *)ud;
  (void)s; (void)nresults; (void)instr;
  (*sfp->action)(sfp->buff);
}

static int forfiles(const char *ext, filefunc action) {
  int status;
  SearchFileParams sfp;
  sfp.ext=ext;
  sfp.buff=NULL;
  sfp.action=action;
  sfp.len=sfp.alloc=0;
  debuglog("searching for files matching '%s'", ext);
  status = luaD_pcall(luaVM, f_dofiles, &sfp, 0);
  free(sfp.buff);
  if (status == LUA_ERRMEM) {
    lua_error(luaVM, "test directory tree too deep");
  }
  return 0;
}

/* append a path component */
#define pat_addname(sfp, name) do { \
  size_t needed; \
  size_t namelen = strlen(name); \
  lassert(sfp->len <= sfp->alloc); \
  if (sfp->len > 0) namelen++; /* also add '/' */ \
  needed = (sfp->len + namelen + 1); /* total space now needed */ \
  if (needed > sfp->alloc) { \
    sfp->alloc = needed; \
    sfp->buff = xrealloc(sfp->buff, sfp->alloc); \
  } \
  if (sfp->len > 0) { \
    sfp->buff[sfp->len++] = '/'; \
    namelen--; \
  } \
  strcpy(sfp->buff+sfp->len, name); \
  sfp->len += namelen; \
  /*debuglog("sfp->len = %zu, sfp->buff = \"%s\"", sfp->len, sfp->buff);*/ \
} while (0)

/* remove a path component */
#define pat_removename(sfp, name) do { \
  size_t namelen = strlen(name); \
  lassert(sfp->len >= namelen); \
  if (sfp->len == namelen) sfp->len = 0; \
  else sfp->len -= namelen+1; \
  sfp->buff[sfp->len] = '\0'; \
  /*debuglog("sfp->len = %zu, sfp->buff = \"%s\"", sfp->len, sfp->buff);*/ \
} while (0)


static void dofiles(SearchFileParams *sfp) {
  WIN32_FIND_DATAA file;
  HANDLE h;
  pat_addname(sfp, "*.*");
  debuglog("using search string '%s'", sfp->buff);
  h = FindFirstFileA(sfp->buff, &file);
  pat_removename(sfp, "*.*");
  if (h == INVALID_HANDLE_VALUE)
    lua_throw(luaVM, LUA_ERRRUN);
  do {
    char *name = (char *)file.cFileName;
    if (*name == '.') continue;
    if ((file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
      debuglog("found subdirectory '%s'", name);
      pat_addname(sfp, name);
      debuglog("recursing into subdirectory '%s'", sfp->buff);
      dofiles(sfp);
      pat_removename(sfp, name);
    } else if (hasext(name, strlen(name), sfp->ext)) {
      int status;
      pat_addname(sfp, name);
      debuglog("processing file '%s'", sfp->buff);
      status = luaD_pcall(luaVM, f_dofileaction, sfp, 0);
      debuglog("processed file '%s', operation returned code %d", sfp->buff,
               status);
      pat_removename(sfp, name);
    }
  } while (FindNextFileA(h, &file));
  FindClose(h);
}


/******************************************************************************/
/* utilities for switching between the <game> and <test> directories */
/******************************************************************************/

#define MOD_PATHSIZE(x) (sizeof(MODS_BASE_PATH)-1+strlen(modname)+1+sizeof(x))
#define MOD_PATHFMT(x) MODS_BASE_PATH "/%s/" x, modname
#define open_log(filevar,x) do { \
  char logpath[MAX_PATH]; \
  size_t size = MOD_PATHSIZE(x); \
  lassert(size <= sizeof(logpath)); \
  sprintf(logpath, MOD_PATHFMT(x)); \
  filevar = fopen(logpath, "w"); \
} while (0)

/* path to game root */
static char gamedir[MAX_PATH] = {0};
/* path to test folder relative to game root */
static char testdir[MAX_PATH] = {0};

/* save the current directory as the game root */
static char *savecwd(void) {
  if (!GetCurrentDirectoryA(sizeof(gamedir), gamedir)) {
    lua_error(luaVM, "GetCurrentDirectoryA() returned code %d", GetLastError());
  }
  return (char *)gamedir;
}

/* cd to the given directory */
static void changecwd(const char *dir) {
  lassert(*gamedir != 0);
  if (!SetCurrentDirectoryA(dir))
    lua_error(luaVM, "cannot set current directory to '%s' "
      "(current directory is '%s'", dir, gamedir);
}

/* cd into the test directory */
static void cd2testdir(void) {
  size_t sz;
  lassert(*testdir == 0);
  savecwd();
  /* create the test root name in the form "mods/<modname>/<testsubdir>" */
  sz = MOD_PATHSIZE(TESTDIRNAME);
  lassert(sz < sizeof(testdir));
  sprintf(testdir, MOD_PATHFMT(TESTDIRNAME));
  changecwd(testdir);
}

/* cd into the game root directory */
static void cd2gamedir(void) {
  lassert(*testdir != 0);
  *testdir = 0;
  changecwd(gamedir);
}

static void setup(void) {
  init_symbols(); /* set pointers */
  lassert(modname != NULL);
#ifdef WITH_LOGGING
  open_log(fdebuglog, DEBUGFILENAME);
  debuglog("starting up tests");
  debuglog("current mod name: '%s'", modname);
#endif /* WITH_LOGGING */
  open_log(ferrorlog, ERRORFILENAME);
}

static void finished(void) {
#ifdef WITH_LOGGING
  if (fdebuglog) fclose(fdebuglog);
#endif /* WITH_LOGGING */
  if (ferrorlog) fclose(ferrorlog);
}


/******************************************************************************/
/* Lua C functions */
/******************************************************************************/


#ifdef __cplusplus
extern "C" {
#endif

int __declspec(dllexport) run_tests (lua_State *s) {
  luaVM = s;
  setup(); /* do initial setup stuff before changing current directory */
  cd2testdir();/* allow chunk names to match across machines as relative paths*/
  forfiles(".lua", compilefile); /* parse all Lua files in the test folder */
  cd2gamedir(); /* reset current diretory */
  finished(); /* clean up */
  return 0;
}

int __declspec(dllexport) load_chunks (lua_State *s) {
  luaVM = s;
  run_tests(s);
  setup();
  cd2testdir();
  forfiles(".luac", loadfilechunk);
  cd2gamedir();
  finished();
  return 0;
}

#ifdef __cplusplus
}
#endif

