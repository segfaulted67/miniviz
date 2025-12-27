#ifndef NIB_H
#define NIB_H


#ifdef NIB_IMPLEMENTATION
#ifdef __cplusplus
extern "C" {
#endif

// ------------------------------------------------------------------------------------------------------------------
// Includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <errno.h>

#ifdef _WIN32
// niche OS Windows
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#endif
// ------------------------------------------------------------------------------------------------------------------
// Constants
#ifdef _WIN32
#define NIB_INVALID_PROC NULL
#else
#define NIB_INVALID_PROC (-1)
#endif
#define NIB_SB_MIN_CAP    32
#define DA_INITIAL_CAP    256
// ------------------------------------------------------------------------------------------------------------------
// Colors
#ifdef ANSI_ENABLE
#define NIB_C_RESET   "\033[0m"
#define NIB_C_RED     "\033[31m"
#define NIB_C_GREEN   "\033[32m"
#define NIB_C_YELLOW  "\033[33m"
#define NIB_C_BLUE    "\033[34m"
#define NIB_C_MAGENTA "\033[35m"
#define NIB_C_CYAN    "\033[36m"
#define NIB_C_WHITE   "\033[37m"
#define NIB_C_BOLD    "\033[1m"
#define NIB_C_BRED    "\033[91m"
#define NIB_C_BYELLOW "\033[93m"
#define NIB_C_BBLUE   "\033[94m"

// ------------------------------------------------------------------------------------------------------------------
// Logging
#define nib_log(stream, color, level, fmt, ...)   \
    fprintf(stream, color "[%s]: " NIB_C_RESET fmt, level, ##__VA_ARGS__)

#define nib_error(fmt, ...) \
    nib_log(stderr, NIB_C_RED, "ERROR",           \
            "%s:%d (%s()) " fmt "\n",             \
            __FILE__, __LINE__, __func__, ##__VA_ARGS__)

#define nib_info(fmt, ...)                        \
    nib_log(stdout, NIB_C_BLUE, "INFO", fmt, ##__VA_ARGS__)
#define nib_cmd_log(fmt, ...)                     \
    nib_log(stdout, NIB_C_GREEN, "CMD", fmt, ##__VA_ARGS__)

#else

#define nib_log(stream, level, fmt, ...)          \
  fprintf(stream, "[%s]: " fmt, level, ##__VA_ARGS__)
#define nib_error(fmt, ...)                       \
  nib_log(stderr, "ERROR", "%s:%d (%s()) " fmt "\n",   \
          __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#define nib_info(fmt, ...)                        \
  nib_log(stdout, "INFO", fmt, ##__VA_ARGS__)
#define nib_cmd_log(fmt, ...)                     \
  nib_log(stdout, "CMD", fmt, ##__VA_ARGS__)
#endif // LOGGING
#define nib_Assert  assert

// Foreach loop
#define nib_foreach(type, it, da)     \
  for (type *it = (da)->items; it < (da)->items + (da)->count; ++it)

#ifdef __cplusplus
#define NIB_DECLTYPE_CAST(T) (decltype(T))
#else
#define NIB_DECLTYPE_CAST(T)
#endif // NIB_DECLTYPE_CAST

// typecasting useful C functions
#define NIB_MALLOC            malloc
#define NIB_REALLOC           realloc
#define NIB_CALLOC            calloc
#define NIB_FREE              free
// ------------------------------------------------------------------------------------------------------------------
// Pid
#ifdef _WIN32
  typedef HANDLE Pid;
#else
  typedef int Pid;
#endif

// ------------------------------------------------------------------------------------------------------------------
#define nib_da_reserve(da, expected_capacity)                                       \
  do {                                                                              \
    if(expected_capacity > (da)->capacity)                                          \
    {                                                                               \
      if((da)->capacity == 0) {                                                     \
        (da)->capacity = DA_INITIAL_CAP;                                            \
      }                                                                             \
      while(expected_capacity > (da)->capacity) {                                   \
        (da)->capacity *= 2;                                                        \
      }                                                                             \
      (da)->items = NIB_DECLTYPE_CAST((da)->items)NIB_REALLOC((da)->items, (da)->capacity * sizeof(*(da)->items));      \
      nib_Assert((da)->items != NULL && "Bro dont have enough RAM in BIG 25 lol");  \
    }                                                                               \
  } while (0)

#define nib_da_append(da, item)                                                           \
  do {                                                                                    \
    nib_da_reserve((da), (da)->count + 1);                                                \
    (da)->items[(da)->count++] = (item);                                                  \
  } while (0)

#define nib_da_append_many(da, new_items, new_items_count)                                        \
  do {                                                                                            \
    nib_da_reserve((da), (da)->count + new_items_count);                                          \
    memcpy((da)->items + (da)->count, (new_items), (new_items_count)*sizeof(*(da)->items));       \
    (da)->count += (new_items_count);                                                             \
  } while (0)

#define nib_da_free(da)   \
  NIB_FREE((da)->items)
// ------------------------------------------------------------------------------------------------------------------
// CMD
typedef struct{
  const char **items;
  size_t count;
  size_t capacity;
} nib_cmd;

void nib_cmd_append_null(nib_cmd *cmd, ...);
Pid nib_cmd_run(nib_cmd cmd);
void nib_cmd_render(nib_cmd cmd);

// ------------------------------------------------------------------------------------------------------------------
// String Builder
typedef struct{
  char *items;
  size_t count;
  size_t capacity;
} nib_stringbuilder;

size_t nib_sb_get_count(nib_stringbuilder sb);
size_t nib_sb_get_capacity(nib_stringbuilder sb);
const char *nib_sb_peek(nib_stringbuilder sb);

#define nib_sb_append_buf(sb, buf, size)  nib_da_append_many(sb, buf, size)
#define nib_sb_append_cstr(sb, cstr)    \
  do {                                  \
    const char *s = (cstr);             \
    size_t size = strlen(s);            \
    nib_da_append_many(sb, s, size);    \
  } while (0)
#define nib_sb_append_null(sb)  nib_da_append_many(sb, "", 1)
#define nib_sb_free(sb) NIB_FREE(sb.items)

// ------------------------------------------------------------------------------------------------------------------

void nib_cmd_append_null(nib_cmd *cmd, ...)
{
  va_list args;
  va_start(args, cmd);

  const char *arg = va_arg(args, const char *);
  while(arg != NULL)
  {
    nib_da_append(cmd, arg);
    arg = va_arg(args, const char *);
  }
  va_end(args);
}

#define nib_cmd_append(cmd, ...)  \
  nib_cmd_append_null(cmd, __VA_ARGS__, NULL)

#ifdef _WIN32
static void nib_win32_cmd(nib_cmd cmd, nib_stringbuilder *sb)
{
  for(size_t i = 0; i < cmd.count; i++) {
    const char *arg = cmd.items[i];
    if(arg == NULL) break;
    nib_sb_append_cstr(sb, arg);

    if (i+1 < cmd.count) {
      nib_sb_append_cstr(sb, " ");
    }
  }
  nib_sb_append_null(sb);
}
#endif

Pid nib_cmd_run(nib_cmd cmd)
{
  if(cmd.count < 1) {
    nib_error("Could not run empty command");
    return NIB_INVALID_PROC;
  }
#ifndef NIB_NO_ECHO
  nib_cmd_render(cmd);
#endif // NIB_NO_ECHO

#ifdef _WIN32
// - https://learn.microsoft.com/en-us/windows/win32/procthread/creating-processes
// - https://learn.microsoft.com/en-us/windows/win32/procthread/creating-a-child-process-with-redirected-input-and-output

  nib_stringbuilder sb = { 0 };
  nib_win32_cmd(cmd, &sb);

  STARTUPINFO si;
  PROCESS_INFORMATION pi;

  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  ZeroMemory(&pi, sizeof(pi));

  BOOL bSuccess = CreateProcessA(NULL, (LPSTR)sb.items, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

  if(!bSuccess) {
    nib_error("Could not create child process for %s: %d", cmd.items[0], (int)GetLastError());
    return NIB_INVALID_PROC;
  }
  WaitForSingleObject(pi.hProcess, INFINITE);
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  nib_sb_free(sb);
  return NIB_INVALID_PROC;
#else
  Pid cpid = fork();

  nib_cmd cmd_null = { 0 };
  nib_da_append_many(&cmd_null, cmd.items, cmd.count);
  nib_cmd_append(&cmd_null, NULL);

  if(cpid < 0)
  {
    nib_error("Could not fork child process: %s", strerror(errno));
    return NIB_INVALID_PROC;
  }

  if(cpid == 0)
  {
    if(execvp(cmd_null.items[0], (char * const*)cmd_null.items) < 0) {
      nib_error("Could not exec child process for %s: %s", cmd.items[0], strerror(errno));
      nib_da_free(&cmd_null);
      exit(1);
    }
  }

  int status;
  waitpid(cpid, &status, 0);

  nib_da_free(&cmd_null);
  return cpid;
#endif
}

void nib_cmd_render(nib_cmd cmd)
{
  nib_cmd_log("%s", cmd.items[0]);
  for(size_t i = 1; i < cmd.count; i++) {
    printf(" %s", cmd.items[i]);
  }
  printf("\n");
}
// ------------------------------------------------------------------------------------------------------------------
size_t nib_sb_get_count(nib_stringbuilder sb)
{
  return sb.count;
}
size_t nib_sb_get_capacity(nib_stringbuilder sb)
{
  return sb.capacity;
}
const char *nib_sb_peek(nib_stringbuilder sb)
{
  return sb.items;
}
// ------------------------------------------------------------------------------------------------------------------


// Backward Compatibility
#define nib_ERROR       nib_error
#define NIB_ERROR       nib_error
#define INFO            nib_info
#define NIB_INFO        nib_info
#define CMD             nib_cmd
#define NIB_CMD         nib_cmd
#define SB              nib_stringbuilder
#define String_Builder  nib_stringbuilder
#define stringbuilder   nib_stringbuilder
#define NIB_CMD_APPEND  nib_cmd_append
#define NIB_CMD_RUN     nib_cmd_run
#define CMD_RUN         nib_cmd_run
// ------------------------------------------------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif // __cplusplus
#endif // NIB_IMPLEMENTATION

#endif // NIB_H
