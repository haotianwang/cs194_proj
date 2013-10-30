#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "sudoku_helper.cpp"

// step 1
bool checkCurrentCandidates(Puzzle p) {
  return p.checkCandidates(p.getCurrentRow(), p.getCurrentCol());
}


// step 2. Returns whether reversing the slot succeeds.
bool reverseSlot(Puzzle p) {
  p.resetCandidates(p.getCurrentRow(), p.getCurrentCol());
  if (!p.prevSlot()) {
    return false;
  }
}


// step 3. Returns whether assignment succeeds
bool assignAndValidate(Puzzle p) {
  if (p.assign(p.getCurrentRow(), p.getCurrentCol())) {
    printf("step 3: assignment succeeded\n");
    printf("calling updateNeighborConflicts with %i, %i, %i\n", p.getCurrentRow(), p.getCurrentCol(), p.getCurrentAssigned(p.getCurrentRow(), p.getCurrentCol()));
    p.updateNeighborConflicts(p.getCurrentRow(), p.getCurrentCol(), p.getCurrentAssigned(p.getCurrentRow(), p.getCurrentCol()), true);
    printf("step 3: updateNeighborConflicts succeeded");
    return p.checkNeighborCandidates(p.getCurrentRow(), p.getCurrentCol());
  }
  else {
    printf("error: reached slot with no candidates. shouldn't happen. exiting.\n");
    return false;
  }
}


// step 4
void backtrack(Puzzle p) {
  p.removeAndInvalidate(p.getCurrentRow(), p.getCurrentCol());
  p.updateNeighborConflicts(p.getCurrentRow(), p.getCurrentCol(), p.getCurrentAssigned(p.getCurrentRow(), p.getCurrentCol()), false);
  // loops back to front
}


// step 5
void advanceSlot(Puzzle p) {
  p.nextSlot();
  p.copyCandidates(p.getCurrentRow(), p.getCurrentCol());
}


int main(int argc, char *argv[]) {
  
  if (argc != 3) {
    printf("wrong number of inputs\n");
    exit(1);
  }

  int** grid = fileTo2dArray(argv[1], atoi(argv[2]));
  Puzzle p = Puzzle(grid, atoi(argv[2]));
  p.initialize();
  
  p.printGridDim();
  p.printGrid();
  p.printPreassigned();
  p.printInitCandidates();
  
  printf("start: %i, %i\n", p.getCurrentRow(), p.getCurrentCol());
  while (p.nextSlot()) {
    printf("nextslot: row is %i, col is %i\n", p.getCurrentRow(), p.getCurrentCol());
  }
  
  while (p.prevSlot()) {
    printf("prevslot: row is %i, col is %i\n", p.getCurrentRow(), p.getCurrentCol());
  }
  
  //p.updateNeighborConflicts(p.getCurrentRow(), p.getCurrentCol(), p.getCurrentAssigned(p.getCurrentRow(), p.getCurrentCol()), true);
  
  while (!p.isSolved()) {
    printf("step 1\n");
    // step 1
    if (checkCurrentCandidates(p)) {
      printf("step 3\n");
      // step 3
      bool succeeded = assignAndValidate(p);
      if (succeeded) {
        printf("step 5\n");
        // step 5
        advanceSlot(p);
      }
      else {
        printf("step 4\n");
        // step 4
        backtrack(p);
      }
    }
    else {
      printf("step 2\n");
      // step 2
      if (reverseSlot(p)) {
        printf("step 4\n");
        // step 4
        backtrack(p);
      }
      else {
        printf("step 2 failed\n");
        break;
      }
    }
  }
  
  if (p.isSolved()) {
    printf("solution found!\n");
  }
  else {
    printf("no solution found\n");
  }
  
  p.deinitialize();
  delete2dIntArray(grid, atoi(argv[2]));
}
