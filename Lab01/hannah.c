#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {
  setvbuf(stdout, NULL, _IONBF, 0);

  int N = 0;
  int p;
  int status;

  if (fork() != 0) {
    for (int i = 0; i < 100; i++) {
      N++;
      p = N;
      printf("Pai: %d\n", p);
    }
    waitpid(-1, &status, 0);
    return 0;
  } else if (fork() != 0) {
    for (int i = 0; i < 100; i++) {
      N += 2;
      p = N;
      printf("Filho: %d\n", p);
    }
    waitpid(-1, &status, 0);
    _exit(0);
  } else {
    for (int i = 0; i < 100; i++) {
      N += 5;
      p = N;
      printf("Neto: %d\n", p);
    }
    _exit(0);
  }
}