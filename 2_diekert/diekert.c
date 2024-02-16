#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>

// get a random byte
static inline unsigned char randbyte(void)
{
  return rand();
}

// get a random color
int random_color(void)
{
  long r, g, b;
  b = randbyte();
  g = randbyte();
  r = randbyte();

  return r << 16 | g << 8 | b;
}

int main(int argc, char **argv)
{
  srand(time(NULL));

  // in case of no argument, use n = 2
  int n = 2;

  if (argc > 1)
  {
    int n_in = atoi(argv[1]);
    if (n_in > 1)
      n = n_in;
  }

// generate the graph .dot
  puts("digraph {\nsize=\"10,8\"");

  for (int i = 1; i < n; i++)
    for (int k = i + 1; k <= n; k++)
      for (int j = i; j <= n + 1; j++)
      {
        printf("A_%1$d_%3$d -> B_%1$d_%2$d_%3$d;\n", k, j, i);
        printf("B_%1$d_%2$d_%3$d -> C_%1$d_%2$d_%3$d;\n", k, j, i);
      }

  for (int i_c = 1; i_c < n; i_c++)
  {
    int i_b = i_c + 1;
    int k_c = i_b;
    for (int j = i_b + 1; j <= n + 1; j++)
      for (int k_b = i_b + 1; k_b <= n; k_b++)
        printf("C_%d_%d_%d -> B_%d_%d_%d;\n",
               k_c, j, i_c, k_b, j, i_b);
  }

  for (int i_1 = 1; i_1 < n - 1; i_1++)
  {
    int i_2 = i_1 + 1;
    for (int k = i_2 + 1; k <= n; k++)
      for (int j = i_2 + 1; j <= n + 1; j++)
        printf("C_%d_%d_%d -> C_%d_%d_%d;\n", k, j, i_1, k, j, i_2);
  }

  for (int i_c = 1; i_c < n - 1; i_c++)
  {
    int i_a = i_c + 1;
    int j = i_a;
    {
      int k_c = i_a;
      for (int k_a = i_a + 1; k_a <= n; k_a++)
        printf("C_%d_%d_%d -> A_%d_%d;\n", k_c, j, i_c, k_a, i_a);
    };
    for (int k_c = i_a + 1; k_c <= n; k_c++)
    {
      int k_a = k_c;
      printf("C_%d_%d_%d -> A_%d_%d;\n", k_c, j, i_c, k_a, i_a);
    }
  }

  for (int i = 1; i < n; i++)
  {
    long color_A = random_color();
    long color_B = random_color();
    long color_C = random_color();

    for (int k = i + 1; k <= n; k++)
    {
      printf("A_%1$d_%2$d [label=<A<sub>%1$d,%2$d</sub>>, "
             "fillcolor=\"#%3$.6lx\", style=filled];\n",
             k, i, color_A);
      for (int j = i; j <= n + 1; j++)
        printf("C_%1$d_%2$d_%3$d "
               "[label=<C<sub>%1$d,%2$d,%3$d</sub>>, "
               "fillcolor=\"#%4$.6lx\", style=filled];\n"
               "B_%1$d_%2$d_%3$d "
               "[label=<B<sub>%1$d,%2$d,%3$d</sub>>, "
               "fillcolor=\"#%5$.6lx\", style=filled];\n",
               k, j, i, color_C, color_B);
    }
  }

  puts("}");

  return 0;
}
