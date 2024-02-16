#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>
#include <err.h>

double *M_1d;
double *m_1d;
double *n_1d;

size_t dimension;

// max dimension of matrix
#define DIM_MAX (1 << (sizeof(size_t) * 8 / 4))

// define 2d matrix - not trivial in c - simulation of 2d array
#define DEFINE_MATRIX()                                                                \
  register double(*M_2d)[dimension][dimension + 1] __attribute__((unused)) = (void *)M_1d; \
  register double(*m_2d)[dimension][dimension] __attribute__((unused)) = (void *)m_1d;     \
  register double(*n_3d)[dimension][dimension + 1][dimension] __attribute__((unused)) = (void *)n_1d

// define parts of matrix
#define M (*M_2d)
#define m (*m_2d)
#define n (*n_3d)

char current_operation_type;

size_t current_pivot_row;

// thread pool data
typedef struct
{
  pthread_t thread;
  pthread_cond_t for_thread_cond;
  pthread_mutex_t for_thread_mutex;
  bool start_flag;
  bool finished_flag;
} thread_data_t;

thread_data_t *thread_pool;

void *thread_main(void *arg);

// thread destroy in case or finish job or error
void destroy_thread(thread_data_t *thread_data)
{
  pthread_cancel(thread_data->thread);
  pthread_join(thread_data->thread, NULL);
  pthread_cond_destroy(&thread_data->for_thread_cond);
  pthread_mutex_destroy(&thread_data->for_thread_mutex);
}

// allocate resources for thread pool and matrices (c)
int allocate_resources(size_t dimension)
{
  size_t memory_needed = 0;

  size_t
      M_size = dimension * (dimension + 1),
      m_size = dimension * dimension,
      n_size = dimension * (dimension + 1) * dimension,
      thread_pool_size = (dimension - 1) * (dimension + 1);

  // calculate memory needed
  memory_needed += sizeof(*M_1d) * M_size;
  memory_needed += sizeof(*m_1d) * m_size;
  memory_needed += sizeof(*n_1d) * n_size;
  memory_needed += sizeof(*thread_pool) * thread_pool_size;

  if (memory_needed >= SIZE_MAX)
    return 1;

  void *memory;

  if (!(memory = malloc(memory_needed)))
    return 1;

  // assign memory to pointers
  M_1d = memory;
  m_1d = M_1d + M_size;
  n_1d = m_1d + m_size;
  thread_pool = (thread_data_t *)(n_1d + n_size);

  size_t i;

  // initialize thread pool
  for (i = 0; i < thread_pool_size; i++)
  {
    // initialize mutex and condition variable
    if (pthread_mutex_init(&thread_pool[i].for_thread_mutex, NULL))
      goto cleanup_rest_of_pool;

    if (pthread_cond_init(&thread_pool[i].for_thread_cond, NULL))
      goto cleanup_mutex;

    thread_pool[i].start_flag = false;
    thread_pool[i].finished_flag = false;

    // create thread and pass it's id as argument
    if (pthread_create(&thread_pool[i].thread, NULL,
                       thread_main, (void *)i))
      goto cleanup_cond;
  }

  return 0;

// cleanup in case of error
cleanup_cond:
  pthread_cond_destroy(&thread_pool[i].for_thread_cond);

cleanup_mutex:
  pthread_mutex_destroy(&thread_pool[i].for_thread_mutex);

cleanup_rest_of_pool:

  for (size_t j = 0; j < i; j++)
    destroy_thread(&thread_pool[j]);

  free(memory);

  return 1;
}

// deallocate resources
void deallocate_resources(void)
{
  size_t thread_pool_size = (dimension - 1) * (dimension + 1);

  for (size_t j = 0; j < thread_pool_size; j++)
    destroy_thread(&thread_pool[j]);

  free(M_1d);
}

// operations on matrices
inline static void A(size_t k, size_t i)
{
  k--;
  i--;
  DEFINE_MATRIX();

  m[k][i] = M[k][i] / M[i][i];
}

inline static void B(size_t k, size_t j, size_t i)
{
  k--;
  j--;
  i--;
  DEFINE_MATRIX();

  n[k][j][i] = M[i][j] * m[k][i];
}

inline static void C(size_t k, size_t j, size_t i)
{
  k--;
  j--;
  i--;
  DEFINE_MATRIX();

  M[k][j] -= n[k][j][i];
}

// perform operation on matrix M
void perform_operation(size_t thread_id)
{
  int i = current_pivot_row;
  int j;
  int k;
  // A operation
  if (current_operation_type == 'A')
  {
    k = i + 1 + thread_id;
    A(k, i);
    return;
  }
  // B and C operations
  k = i + 1 + thread_id / (dimension + 2 - i);
  j = i + thread_id % (dimension + 2 - i);

  if (current_operation_type == 'B')
    B(k, j, i);
  else if (current_operation_type == 'C')
    C(k, j, i);
}

// run threads and wait for them to finish
void run_threads(size_t how_many)
{
  // signal the required number of threads from thread_pool
  for (size_t i = 0; i < how_many; i++)
  {
    pthread_mutex_lock(&thread_pool[i].for_thread_mutex);
    {
      thread_pool[i].start_flag = true;
      pthread_cond_broadcast(&thread_pool[i].for_thread_cond);
    }
    pthread_mutex_unlock(&thread_pool[i].for_thread_mutex);
  }

  // wait for every thread to finish it's work
  for (size_t i = 0; i < how_many; i++)
  {
    pthread_mutex_lock(&thread_pool[i].for_thread_mutex);
    {
      while (!thread_pool[i].finished_flag)
        pthread_cond_wait(&thread_pool[i].for_thread_cond,
                          &thread_pool[i].for_thread_mutex);

      thread_pool[i].finished_flag = false;
    }
    pthread_mutex_unlock(&thread_pool[i].for_thread_mutex);
  }
}

// thread main function
void *thread_main(void *arg)
{
  size_t thread_id = (size_t)arg;

  pthread_mutex_lock(&thread_pool[thread_id].for_thread_mutex);

  while (1)
  {
    {
      while (!thread_pool[thread_id].start_flag)
        pthread_cond_wait(&thread_pool[thread_id].for_thread_cond,
                          &thread_pool[thread_id].for_thread_mutex);

      thread_pool[thread_id].start_flag = false;
    }
    pthread_mutex_unlock(&thread_pool[thread_id].for_thread_mutex);

    perform_operation(thread_id);

    pthread_mutex_lock(&thread_pool[thread_id].for_thread_mutex);
    { // in mutex
      thread_pool[thread_id].finished_flag = true;

      pthread_cond_broadcast(&thread_pool[thread_id].for_thread_cond);
    }
  }
}

// print matrix M
void print_M(void)
{
  DEFINE_MATRIX();

  printf("%-2zu\r\n", dimension);

  for (size_t i = 0; i < dimension; i++)
  {
    for (size_t j = 0; j < dimension; j++)
    {
      printf("%3.1lf", M[i][j]);
      if (j != dimension - 1)
        putchar(' ');
    }
    printf("\r\n");
  }

  for (size_t i = 0; i < dimension; i++)
  {
    printf("%3.5lf", M[i][dimension]);
    if (i != dimension - 1)
      putchar(' ');
  }

  printf("\r\n");
}

// schedule operations on matrix M
void schedule(void)
{
  // for every pivot row
  for (current_pivot_row = 1; current_pivot_row < dimension;
       current_pivot_row++)
  {
    size_t nrows = dimension - current_pivot_row;

    current_operation_type = 'A';

    run_threads(nrows);

    size_t ncells = nrows * (dimension + 2 - current_pivot_row);

    current_operation_type = 'B';

    run_threads(ncells);

    current_operation_type = 'C';

    run_threads(ncells);
  }
}

int main(int argc, char **argv)
{
  char *input_name = "in.txt";

  FILE *input = stdin;

  if (strcmp(input_name, "-"))
    if (!(input = fopen(input_name, "r")))
      err(-1, "could not open file");

  if (fscanf(input, "%zu", &dimension) != 1)
    errx(-1, "could not read dimension");

  if (dimension > DIM_MAX)
    errx(-1, "dimension bigger than %d", DIM_MAX);

  if (allocate_resources(dimension))
    errx(-1, "could not allocate resources");

  // define 2d matrix
  DEFINE_MATRIX();

  // read matrix
  for (size_t i = 0; i < dimension; i++)
    for (size_t j = 0; j < dimension; j++)
      if (fscanf(input, "%lf", &M[i][j]) != 1)
        errx(-1, "could not read matrix");

  // read vector
  for (size_t i = 0; i < dimension; i++)
    if (fscanf(input, "%lf", &M[i][dimension]) != 1)
      errx(-1, "could not read matrix");

  schedule();

  // single-threaded backward substitution
  for (size_t i = dimension - 1; i < dimension; i--)
  {
    for (size_t k = i - 1; k < i; k--)
    {
      double factor = M[k][i] / M[i][i];
      M[k][i] -= factor * M[i][i];
      M[k][dimension] -= factor * M[i][dimension];
    }
    M[i][dimension] /= M[i][i];
    M[i][i] /= M[i][i];
  }

  print_M();

  deallocate_resources();

  return 0;
}
