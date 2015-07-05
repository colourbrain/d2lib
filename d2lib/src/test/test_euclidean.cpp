#include "../common/d2.hpp"
#include "time.h"

int main(int argc, char** argv) {
  using namespace d2;

  size_t len[2] = {8, 8}, size=100;  

  BlockMultiPhase<Elem<def::Euclidean, 3>, Elem<def::Euclidean, 3> > data (size, len);
  data.read("data/test/euclidean_testdata.d2", size);
  //  data.write("");

  server::Init(argc, argv);
  srand(time(NULL));
  int i1 = rand() % data.get_size(), i2 = rand() % data.get_size();
  auto & block0 = data.get_block<0>();
  std::cerr << "phase 0 - squared EMD between #" << i1 << " and #" << i2 
	    << ": "  << EMD(block0[i1], block0[i2], block0.meta);

  std::cerr << "and its lower bound"
	    << ": "  << LowerThanEMD_v0(block0[i1], block0[i2], block0.meta)
	    << " and "  << LowerThanEMD_v1(block0[i1], block0[i2], block0.meta) 
	    << std::endl;


  auto & block1 = data.get_block<1>();
  std::cerr << "phase 1 - squared EMD between #" << i1 << " and #" << i2 
	    << ": "  << EMD(block1[i1], block1[i2], block1.meta);

  std::cerr << "and its lower bound"
	    << ": "  << LowerThanEMD_v0(block1[i1], block1[i2], block1.meta) 
	    << " and "  << LowerThanEMD_v1(block1[i1], block1[i2], block1.meta) 
	    << std::endl;

  std::vector<real_t> emds(data.get_size());
  std::vector<index_t> ranks(data.get_size());
  double startTime;
  startTime = getRealTime();
  KNearestNeighbors_Linear(2, block0[i1], block0, &emds[0], &ranks[0]);  
  std::cerr << "phase 0 - nearest neighbors of #" << i1
	    << " is #" << ranks[1] 
	    << ": " << emds[ranks[1]] 
	    << "\t\t" << getRealTime() - startTime << "s"
	    << std::endl;
  startTime = getRealTime();
  KNearestNeighbors_Linear(2, block1[i1], block1, &emds[0], &ranks[0]);
  std::cerr << "phase 1 - nearest neighbors of #" << i1
	    << " is #" << ranks[1] 
	    << ": " << emds[ranks[1]] 
	    << "\t\t" << getRealTime() - startTime << "s"
	    << std::endl;

  server::Finalize();

  return 0;
}
