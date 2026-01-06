#include <stdio.h>
#include <syscall.h>

int atoi(const char *str);

int main(int argc, char *argv[]) {
  if (argc != 5) {
    printf("Usage: ./additional [num 1] [num 2] [num 3] [num 4]\n");
    return -1;
  }
  int a = atoi(argv[1]);
  int b = atoi(argv[2]);
  int c = atoi(argv[3]);
  int d = atoi(argv[4]);
  printf("%d %d\n", fibonacci(a), max_of_four_int(a, b, c, d));
  return 0;
}

int atoi(const char *str) {
  int res = 0; // Initialize result
  for (int i = 0; str[i] != '\0' && str[i] >= '0' && str[i] <= '9'; i++) {
    res = res * 10 + (str[i] - '0');
  }
  return res;
}