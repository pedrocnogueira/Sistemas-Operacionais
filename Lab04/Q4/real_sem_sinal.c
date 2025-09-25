#include <stdio.h>

int main(void) {
  double a, b;
  if (scanf("%lf %lf", &a, &b) != 2)
    return 1;

  printf("a + b = %.10g\n", a + b);
  printf("a - b = %.10g\n", a - b);
  printf("a * b = %.10g\n", a * b);
  printf("a / b = %.10g\n", a / b);
  return 0;
}
