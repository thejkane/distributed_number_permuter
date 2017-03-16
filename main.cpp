#include <iostream>
#include <mpi.h>
#include "sanders_perm.hpp"


int main(int argc, char* argv[]) {

  std::cout << "Starting distributed permuations ..." << std::endl;
  MPI_Init(&argc, &argv);
  int N;
  MPI_Comm_size(MPI_COMM_WORLD, &N);

  std::cout << "Number of participating processes : " << N
	    << std::endl;

  unsigned long int n = 32;
  sanders_permutation<unsigned long int> sp(n);
  std::vector<unsigned long int> out;
  sp.permute(N, out);

  MPI_Finalize();

}
