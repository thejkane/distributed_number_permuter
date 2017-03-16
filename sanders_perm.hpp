// Copyright (C) 2017 - Thejaka Kanewala.
//  Authors: Thejaka Kanewala
/**************************************************************************
/* This source file implements the permutation algorithm described P. Sanders [1] 
   in a distributed setting. This is a direct implementation is based on the 
   algorithm described in [2].

[1] Sanders, Peter. "Random permutations on distributed, \
external and hierarchical memory." Information Processing Letters 67.6 (1998): 305-309.
[2] Langr, Daniel, et al. "Algorithm 947: Paraperm---Parallel Generation of 
Random Permutations with MPI." ACM Transactions on Mathematical Software (TOMS) 41.1 (2014): 5.
*/

#include <mpi.h>
#include <cmath>
#include <vector>
#include <algorithm>

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

#define SP_DATA_TYPE MPI_UNSIGNED_LONG

template<typename perm_t>
class sanders_permutation {

  typedef std::vector<perm_t> permute_vector_t;

public:
  // n - number to permute
  sanders_permutation(perm_t& pn):n(pn) {}

  //  N - total number of processors
  void permute(int N, permute_vector_t& p_out);

  void verify(int N, permute_vector_t& p_out);

private:
  perm_t& n;
  int rank;

#ifdef PRINT_DEBUG
  int debug_rank;
#endif

  void error(std::string mpifn, std::string desc) {
    std::cout << "[ERROR] Permuting numbers -- MPI function : "
	      << mpifn << ", description : " << desc
	      << std::endl;
  }

  void run_phase1(unsigned int count, perm_t pos, unsigned int N, 
		  perm_t** temp, unsigned int& total);
  void run_phase2(perm_t** temp, unsigned int total);
  void run_phase3(perm_t* temp, unsigned int size, 
		  unsigned int m, 
		  perm_t pos,
		  unsigned int count,
		  permute_vector_t& perm);

};


#define SANDERS_PERM_PARAMS \
  typename perm_t

#define SANDERS_PERM_TYPE \
  sanders_permutation<perm_t>

template<SANDERS_PERM_PARAMS>
void 
SANDERS_PERM_TYPE::verify(int N, permute_vector_t& p_out) {
  std::sort(p_out.begin(), p_out.end());
  permute_vector_t::iterator ite = p_out.begin();
  for (; ite != p_out.end(); ++ite) {
    //TODO
  }
}


template<SANDERS_PERM_PARAMS>
void 
SANDERS_PERM_TYPE::permute(int N, permute_vector_t& p_out) {

  if (MPI_Comm_rank(MPI_COMM_WORLD, &rank) != 0)
    error("MPI_Comm_rank", "Error getting the rank");

#ifdef PRINT_DEBUG
  std::cout << "Current process rank : " << rank << std::endl;
#endif

  double j = (double)n/(double)N;

  unsigned int m = std::ceil(j);
  perm_t pos = rank * (perm_t)m;
  unsigned int count = m;

  //if (r + 1)m > n then count ← n − pos
  if (((rank+1)*m) > n)
    count = (n-pos);

  //if pos ≥ n then count ← 0
  if (pos >= n)
    count = 0;

  perm_t* temp;
  unsigned int sz = 0;

#ifdef PRINT_DEBUG
  std::cout << "r:" << rank << "m:" << m 
	    << ", pos:" << pos << ", count:" << count
	    << std::endl;
#endif
  //  int* perm; // final output

  run_phase1(count, pos, N, &temp, sz);
  run_phase2(&temp, sz);

  unsigned int allocsz = m;
  if ((rank == (N-1)) && 
      count > 0)
    allocsz = count;

  p_out.resize(allocsz);

  run_phase3(temp, sz, m, pos, count, p_out);

  delete[] temp;

#ifdef PRINT_DEBUG
  if (rank == debug_rank) {
    std::cout << "@Rank " << rank << std::endl;
    for(unsigned int q=0; q < allocsz; ++q) {
      std::cout << p_out[q] << ",";
    }

    std::cout << std::endl;
  }
#endif

}


struct sort_indices
{
private:
  unsigned int* parr;
public:
  sort_indices(unsigned int* p_parr) : parr(p_parr) {}
  bool operator()(int i, int j) const { return parr[i]<parr[j]; }
};

template<SANDERS_PERM_PARAMS>
void 
SANDERS_PERM_TYPE::run_phase1(unsigned int count, perm_t pos, unsigned int N, 
			      perm_t** temp, unsigned int& total) {

  perm_t* sendbuf = new perm_t[count+1];
  perm_t* sortedsendbuf = new perm_t[count+1];
  unsigned int* destprocs = new unsigned int[count+1];
  unsigned int* indices = new unsigned int[count+1];

  // for random number generation
  boost::random::mt19937 gen;
  boost::random::uniform_int_distribution<> dist(0, (N-1));

  for (unsigned int k=0; k <= (count-1); ++k) {
    sendbuf[k] = pos+(perm_t)k;
    destprocs[k] = dist(gen);
    indices[k] = k;
  }

  sendbuf[count] = 0;
  destprocs[count] = N;
  indices[count] = count;

#ifdef PRINT_DEBUG
  std::cout << "printing unsorted ..." << std::endl;
  for (unsigned int i=0; i < (count+1); ++i) {
    std::cout << "(" << destprocs[i] << ","
	      << sendbuf[i] << "), ";
  }

  std::cout << std::endl;
#endif

  // sort arrays by rank
  std::sort(indices, (indices+count+1), sort_indices(destprocs)); 

  // copy sendbuf to sortedsendbuf
  for(unsigned int i=0; i < (count+1); ++i) {
    sortedsendbuf[i] = sendbuf[indices[i]];
  }

#ifdef PRINT_DEBUG
  if (rank == debug_rank) {
    std::cout << "printing sorted ..." << std::endl;
    for (unsigned int i=0; i < (count+1); ++i) {
      std::cout << "(" << destprocs[indices[i]] << ","
		<< sortedsendbuf[i] << "), ";
    }
  
    std::cout << std::endl;
  }
#endif

  // we do not need sendbuf now
  delete[] sendbuf;

  std::vector<int> sendcnts;
  sendcnts.resize(N, 0);

  int k = 0;
  for (unsigned int rp=0; rp <= (N-1); ++rp) {
    while(rp == destprocs[indices[k]]) {
      ++sendcnts[rp];
      ++k;
    }
  }

#ifdef PRINT_DEBUG
  if (rank == debug_rank) {
    std::cout << "printing sendcnts..." << std::endl;
    for (unsigned int i=0; i < N; ++i) {
      std::cout << sendcnts[i] << ", ";
    }
  
    std::cout << std::endl;
  }
#endif


  delete[] destprocs;
  delete[] indices;

  //calculate displacements
  std::vector<int> sdispls;
  sdispls.resize(N, 0);
  for (unsigned int hp = 1; hp <= (N-1); ++hp) {
    sdispls[hp] = sdispls[hp-1]+sendcnts[hp-1];
  } 

#ifdef PRINT_DEBUG
  if (rank == debug_rank) {
    std::cout << "printing displacements..." << std::endl;
    for (unsigned int i=0; i < N; ++i) {
      std::cout << sdispls[i] << ", ";
    }
  
    std::cout << std::endl;
  }
#endif

  std::vector<int> recvcnts;
  recvcnts.resize(N,0);
  if (MPI_Alltoall(&sendcnts[0], 1, MPI_UNSIGNED, 
		   &recvcnts[0], 1, MPI_UNSIGNED, MPI_COMM_WORLD) != 0) {
    error("MPI_Alltoall", "Error exchanging send counts and receive counts in phase 1");
  }

#ifdef PRINT_DEBUG
  if (rank == debug_rank) {
    std::cout << "printing recev..." << std::endl;
    for (unsigned int i=0; i < N; ++i) {
      std::cout << recvcnts[i] << ", ";
    }
  
    std::cout << std::endl;
  }
#endif

  std::vector<int> rdispls;
  rdispls.resize(N, 0);
  for(unsigned int rp=1; rp <= (N-1); ++rp) {
    rdispls[rp] = rdispls[rp-1]+recvcnts[rp-1];
  }

#ifdef PRINT_DEBUG
  if (rank == debug_rank) {
    std::cout << "printing rdispls..." << std::endl;
    for (unsigned int i=0; i < N; ++i) {
      std::cout << rdispls[i] << ", ";
    }
  
    std::cout << std::endl;
  }
#endif


  for (unsigned int rp=0; rp <= (N-1); ++rp) {
    total += recvcnts[rp];
  }

#ifdef PRINT_DEBUG
  std::cout << "r:" << rank << "total : " << total << std::endl;
#endif

  (*temp) = new perm_t[total];
  //std::vector<perm_t> temp;
  //temp.resize(total);
  
  if (MPI_Alltoallv(&sortedsendbuf[0], &sendcnts[0], &sdispls[0], SP_DATA_TYPE,
			     (*temp), &recvcnts[0], &rdispls[0], SP_DATA_TYPE,
		    MPI_COMM_WORLD) != 0)
    error("MPI_Alltoallv", "Error exchanging permuted values in phase 1");

#ifdef PRINT_DEBUG
  if (rank == debug_rank) {
    for(unsigned int q=0; q < total; ++q) {
      std::cout << (*temp)[q] << ",";
    }
  }
  std::cout << std::endl;
#endif

  delete[] sortedsendbuf;

  if (MPI_Barrier(MPI_COMM_WORLD) != 0)
    error("MPI_Barrier", "Error synchronizing processes in phase 1");
}


template<SANDERS_PERM_PARAMS>
void 
SANDERS_PERM_TYPE::run_phase2(perm_t** temp, unsigned int total) {
  boost::random::mt19937 gen;

  if (total > 1) {
    for (int k=(total-1); k >=1; --k) {
      boost::random::uniform_int_distribution<> dist(0, k);
      int l = dist(gen);
      int t = (*temp)[k];
      (*temp)[k] = (*temp)[l];
      (*temp)[l] = t;
    }
  }

  if (MPI_Barrier(MPI_COMM_WORLD) != 0)
    error("MPI_Barrier", "Error synchronizing processes in phase 2");

#ifdef PRINT_DEBUG
  std::cout << "printing after local permuation " << std::endl;
  if (rank == 1) {
    for(unsigned int q=0; q < total; ++q) {
      std::cout << (*temp)[q] << ",";
    }
  }
  std::cout << std::endl;
#endif

}


template<SANDERS_PERM_PARAMS>
void 
SANDERS_PERM_TYPE::run_phase3(perm_t* temp, unsigned int sz,
			      unsigned int m,
			      perm_t pos,
			      unsigned int count,
			      permute_vector_t& perm) {

  perm_t size = (perm_t)sz;
  perm_t first;
  
  if (MPI_Scan(&size, &first, 1, 
	       SP_DATA_TYPE, MPI_SUM, MPI_COMM_WORLD) != 0)
    error("MPI_Scan", "Error getting prefix sums in phase 3");
  
#ifdef PRINT_DEBUG
  std::cout << "rank : " << rank << " first : " << first << std::endl;
#endif

  first = first - size;
  perm_t last = first + size - 1;
  double fmdiv = (double)first / (double)m;
  unsigned int rp = std::floor(fmdiv);
  perm_t firstp = first;
  unsigned int remains = count;

  std::vector<MPI_Request> requests;
  MPI_Request request;
  perm_t* buf = new perm_t[2];

#ifdef PRINT_DEBUG
  if (rank == debug_rank) {
    std::cout << "firstp:" << firstp << ", last:" << last
	      << ", remains:" << remains << ", rp:" << rp
	      << ", pos:" << pos
	      << ", first:" << first 
	      << std::endl;
  }
#endif

  do {
    perm_t lastp = (rp+1)*m - 1;
    if (lastp > last)
      lastp = last;

    unsigned int countp = lastp-firstp+1;
    if (rank == (int)rp) {
      for (unsigned int k=firstp; k<=lastp; ++k) {
	perm[k-pos]=temp[k-first];
      }

      remains -= countp;
    } else {
      buf[0] = firstp;
      buf[1] = (perm_t)countp;

      if (MPI_Isend(&buf[0], 2, SP_DATA_TYPE, rp, 1, 
		    MPI_COMM_WORLD, &request) != 0)
	error("MPI_Isend", "Error exchanging first and last values in phase 3");

      requests.push_back(request);

      if (MPI_Isend(&temp[firstp-first], countp, SP_DATA_TYPE, rp, 2, 
		    MPI_COMM_WORLD, &request) != 0)
	error("MPI_Isend", "Error sending surplus elements to others in phas 3");

      requests.push_back(request);
    }
    
    rp += 1;
    firstp += countp;
#ifdef PRINT_DEBUG
    if (rank == debug_rank) {
      std::cout << "rp: " << rp << "firstp:" << firstp << std::endl;
    }
#endif
  }while(firstp < last);

#ifdef PRINT_DEBUG
  if (rank == debug_rank)
    std::cout << "remains 1: " << remains << std::endl;
#endif

  while(remains > 0) {
    MPI_Status status;
    if (MPI_Recv(&buf[0], 2, SP_DATA_TYPE, MPI_ANY_SOURCE, 1, 
		 MPI_COMM_WORLD, &status)!=0)
      error("MPI_Recv", "Error while receiving first and last values in phase 3");

    firstp = buf[0];
    unsigned int countp = buf[1];

    if (MPI_Recv(&perm[firstp-pos], countp, SP_DATA_TYPE, 
		 status.MPI_SOURCE, 2, MPI_COMM_WORLD, &status) != 0)
            error("MPI_Recv", "Error while receiving additional values in phase 3");

    remains -= countp;
#ifdef PRINT_DEBUG
    if (rank == debug_rank)
      std::cout << "remains : " << remains << std::endl;
#endif
  }

  MPI_Status status;
  for (unsigned int k=0; k < requests.size(); ++k) {
    if (MPI_Wait(&requests[k], &status) != 0)
      error("MPI_Wait", "Error waiting for requests in phase 3");
  }

  requests.clear();
  delete[] buf;

  if (MPI_Barrier(MPI_COMM_WORLD) !=0)
    error("MPI_Barrier", "Error invoking barrier in phase 3");

}