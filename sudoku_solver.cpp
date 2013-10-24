#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "sudoku_helper.cpp"

int main(int argc, char *argv[]) {
  int** grid = fileTo2dArray(argv[1], atoi(argv[2]));
  Puzzle p = Puzzle(grid, atoi(argv[2]));
  p.initialize();
  
  p.printGridDim();
  p.printGrid();
  p.printPreassigned();
  p.printInitCandidates();
  
  p.deinitialize();
  delete2dIntArray(grid, atoi(argv[2]));
}
