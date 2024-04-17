#define _XOPEN_SOURCE
#define _GNU_SOURCE
#include "error.h"
#include "onSignal.h"
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#define ENABLE(register, flag)  ((register) |= (flag))
#define DISABLE(register, flag) ((register) &= ~(flag))

pid_t run(const char *path, char *const *argv, char *const *envp) {
  pid_t pid = fork();
  if (pid == 0) {
    if (execve(path, argv, envp) == -1)
      error(CHILD_FAILED, "execve(...) failed, errno: %d", errno);
  }
  return pid;
}

pid_t runChild() { return run("build/child", NULL, NULL); }

pid_t *childs = NULL;
int childCount = 0;

bool forkNew() {
  pid_t child = runChild();
  printf("Fork new: %d\n", child);
  childCount++;
  childs = realloc(childs, sizeof(pid_t) * childCount);
  childs[childCount - 1] = child;
  return true;
}

bool killLast() {
  if (childCount == 0) return true;
  childCount--;
  printf("Kill PID: %d\n", childs[childCount]);
  kill(childs[childCount], SIGKILL);
  waitpid(childs[childCount], NULL, 0);
  return true;
}

bool killAll() {
  while (childCount)
    killLast();
  free(childs);
  childs = NULL;
  return true;
}

bool list() {
  system("pstree");
  return true;
}

bool silent(int id) {
  if (!(0 <= id && id < childCount)) {
    printf("Silent: child %d not exists\n", id);
    return true;
  }
  kill(childs[id], SIGUSR1);
  printf("Silent: child %d can't talk\n", id);
  return true;
}

bool grant(int id) {
  if (!(0 <= id && id < childCount)) {
    printf("Grant: child %d not exists\n", id);
    return true;
  }
  kill(childs[id], SIGUSR2);
  printf("Grant: child %d can talk\n", id);
  return true;
}

bool silentAll() {
  for (int i = 0; i < childCount; i++)
    silent(i);
  return true;
}

bool grantAll() {
  alarm(0);
  for (int i = 0; i < childCount; i++)
    grant(i);
  return true;
}

bool priority(int id) {
  silentAll();
  grant(id);
  alarm(5);
  return true;
}

bool quit() { return false; }

bool unknown(const char *command) {
  printf("Unknown command: %s\n", command);
  return true;
}

void onAlarm() { grantAll(); }

bool handleCommand(const char *command) {
  if (strcmp(command, "+") == 0) return forkNew();
  if (strcmp(command, "-") == 0) return killLast();
  if (strcmp(command, "l") == 0) return list();
  if (strcmp(command, "k") == 0) return killAll();
  if (strcmp(command, "s") == 0) return silentAll();
  if (strcmp(command, "g") == 0) return grantAll();
  if (strcmp(command, "q") == 0) return quit();

  int id = 0;
  if (sscanf(command, "s%d", &id) == 1) return silent(id);
  if (sscanf(command, "g%d", &id) == 1) return grant(id);
  if (sscanf(command, "p%d", &id) == 1) return priority(id);
  return unknown(command);
}

bool isFastCommand(int ch) { return strchr("+-q", ch) != NULL; }

int getch() {
  struct termios old, current;
  tcgetattr(STDIN_FILENO, &current);
  old = current;
  DISABLE(current.c_lflag, ECHO);
  DISABLE(current.c_lflag, ICANON);
  tcsetattr(STDIN_FILENO, TCSANOW, &current);
  int ch = getchar();
  tcsetattr(STDIN_FILENO, TCSANOW, &old);
  return ch;
}

char *appendChar(char *str, char *end, char ch) {
  if (str == end) return str;
  putchar(ch);
  *str = ch;
  return str + 1;
}

void readCommand(char *buffer, int bufferSize) {
  int ch = 0;
  char *it = buffer;
  char *end = buffer + bufferSize - 1;
  while ((ch = getch()) != '\n') {
    if (ch == 127) {
      it = it - 1 < buffer ? buffer : it - 1;
      putchar('\b');
      putchar(' ');
      putchar('\b');
      continue;
    }

    if (isFastCommand(ch)) {
      if (it != buffer) putchar('\n');
      it = buffer + 1;
      char str[2] = {(char)ch, 0};
      strcpy(buffer, str);
      break;
    }

    if (ch == 0x1b) {
      it = appendChar(it, end, '^');
      it = appendChar(it, end, '[');
    } else {
      it = appendChar(it, end, ch);
    }

    if (it == end) {
      for (; ch != '\n'; ch = getch())
        ;
      putchar('\n');
      break;
    }
  }
  *it = 0;
  if (!isFastCommand(*buffer)) putchar('\n');
}

int main() {
  onSignal(SIGALRM, onAlarm);
  while (true) {
    char command[256];
    readCommand(command, sizeof(command));
    if (!handleCommand(command)) break;
  }
  return !killAll();
}
