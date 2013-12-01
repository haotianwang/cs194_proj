#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <time.h>
#include <sys/time.h>
#include <vector>

#include "sudoku_helper_serial_unit_prop_threaded.cpp"

// step 1
bool checkCurrentCandidates(Puzzle* p) {
  if (testLevel > 0) printf("step 1: on (%i, %i)\n", p->getCurrentRow(), p->getCurrentCol());
  //printf("step 1: current row %i, current col %i, %i\n", p->getCurrentRow(), p->getCurrentCol(), p->getCurrentAssigned(p->getCurrentRow(), p->getCurrentCol()));
  return p->checkCandidates(p->getCurrentRow(), p->getCurrentCol(), false);
}


// step 2. Returns whether reversing the slot succeeds.
bool reverseSlot(Puzzle* p) {
  if (testLevel > 0) printf("step 2: reversing slot\n");
  p->resetCandidates(p->getCurrentRow(), p->getCurrentCol());
  return p->prevSlot();
}


// step 3. Returns whether assignment succeeds
bool assignAndValidate(Puzzle* p) {
  if (testLevel > 1) printf("pre-step 3:\n");
  if (p->assign(p->getCurrentRow(), p->getCurrentCol())) {
    if (testLevel > 0) 
      printf("step 3: on (%i, %i), assigned %i\n", p->getCurrentRow(), p->getCurrentCol(), p->getCurrentAssigned(p->getCurrentRow(), p->getCurrentCol()));
    //printf("step 3: assignment succeeded\n");
    //printf("calling updateNeighborConflicts with %i, %i, %i\n", p->getCurrentRow(), p->getCurrentCol(), p->getCurrentAssigned(p->getCurrentRow(), p->getCurrentCol()));
    //printf("pre-assign candidate list: \n"); 
    //p->printInitCandidates();
    p->updateNeighborConflicts(p->getCurrentRow(), p->getCurrentCol(), p->getCurrentAssigned(p->getCurrentRow(), p->getCurrentCol()), true);
    //printf("post-assign candidate list: \n"); 
    //p->printInitCandidates();    
//    printf("step 3: updateNeighborConflicts succeeded\n");
    return p->checkNeighborCandidates(p->getCurrentRow(), p->getCurrentCol());
  }
  else {
    printf("error: reached slot with no candidates. shouldn't happen. exiting.\n");
    exit(1);
  }
}


// step 4
void backtrack(Puzzle* p) {
  if (testLevel > 0) 
    printf("step 4: on (%i, %i), unassigning %i\n", p->getCurrentRow(), p->getCurrentCol(), p->getCurrentAssigned(p->getCurrentRow(), p->getCurrentCol()));
  //printf("pre-update candidate list: \n"); 
  //p->printInitCandidates();
  p->updateNeighborConflicts(p->getCurrentRow(), p->getCurrentCol(), p->getCurrentAssigned(p->getCurrentRow(), p->getCurrentCol()), false);
  //printf("post-update candidate list: \n"); 
  //p->printInitCandidates();
  p->removeAndInvalidate(p->getCurrentRow(), p->getCurrentCol());
  // loops back to front
}


// step 5
void advanceSlot(Puzzle* p) {
  if (testLevel > 0) printf("step 5: next slot\n");
  p->nextSlot();
  p->copyCandidates(p->getCurrentRow(), p->getCurrentCol());
}

bool solveInitializedPuzzle(Puzzle* p, int highestVisitedPosition) {
  while (!p->isSolved()) {

    if (testLevel > 1 && highestVisitedPosition < p->positionOnVisited) {
      printf("highestVisitedPosition is now %i\n", p->positionOnVisited);
      highestVisitedPosition = p->positionOnVisited;
    }
    // step 1
    if (checkCurrentCandidates(p)) {
      //printf("step 3\n");
      // step 3
      bool succeeded = assignAndValidate(p);
      if (succeeded) {
        //printf("step 5\n");
        // step 5
        advanceSlot(p);
      }
      else {
        //printf("step 4\n");
        // step 4
        backtrack(p);
      }
    }
    else {
      //printf("step 2\n");
      // step 2
      if (reverseSlot(p)) {
        //printf("step 4\n");
        // step 4
        backtrack(p);
      }
      else {
        //printf("step 2 failed\n");
        break;
      }
    }
  }

  return p->isSolved();
}

Puzzle* solveInitializedPuzzles(std::vector<Puzzle*>* toSolve, int highestVisitedPosition) {
  Puzzle* p = NULL;
  #pragma omp parallel for
  for (int i = toSolve->size()-1; i >= 0; i--) {
    printf("puzzle to solve:\n");
    toSolve->at(i)->printGrid();
    if (solveInitializedPuzzle(toSolve->at(i), highestVisitedPosition)) {
      #pragma omp critical 
      {
        if (p == NULL) {
          printf("solved!\n");
          p = toSolve->at(i);
        }
      }
    }
    else {
      printf("led to dead end\n");
      toSolve->at(i)->deinitialize();
      delete toSolve->at(i);
    }
  }
  return p;
}


int main(int argc, char *argv[]) {
  // timer
  struct timeval start, end;
  long mtime, seconds, useconds;    
  gettimeofday(&start, NULL);

  time_t startTime = time(0);

  if (argc != 3) {
    printf("wrong number of inputs\n");
    exit(1);
  }

  int** grid = fileTo2dArray(argv[1], atoi(argv[2]));

  Puzzle* p = new Puzzle(grid, atoi(argv[2]));
  p->initialize();

  int dim = atoi(argv[2]);

  
  for (int i = 0; i < 5; i++) {
    int** gridCopy = new2dIntArray(dim);
    copy2dIntArray(grid, gridCopy, dim);

    Puzzle* test = new Puzzle(gridCopy, dim);
    test->initialize();

    
    Puzzle* testCopy = test->makePreassignedCopy();
    testCopy->deinitialize();
    delete testCopy;
    
    
    test->deinitialize();
    delete test;
  }

  /*  
  p->printGridDim();
  p->printGrid();
  p->printPreassigned();
  p->printInitCandidates();
  
  printf("start: %i, %i\n", p->getCurrentRow(), p->getCurrentCol());
  while (p->nextSlot()) {
    printf("nextslot: row is %i, col is %i\n", p->getCurrentRow(), p->getCurrentCol());
  }
  
  while (p->prevSlot()) {
    printf("prevslot: row is %i, col is %i\n", p->getCurrentRow(), p->getCurrentCol());
  }
  

  printf("TEST: check candidates\n");
  p->checkCandidates(p->getCurrentRow(), p->getCurrentCol());
  printf("TEST: assign\n");
  p->assign(p->getCurrentRow(), p->getCurrentCol());
  printf("TEST: update neighbors\n");
  p->updateNeighborConflicts(p->getCurrentRow(), p->getCurrentCol(), p->getCurrentAssigned(0, 0), true);
  printf("TEST: check neighbors\n");
  p->checkNeighborCandidates(p->getCurrentRow(), p->getCurrentCol());
  */


  int highestVisitedPosition = 0;
  int currentDepth = 0;
  int parallelStartDepth = 1;
  //int counter=10000;
  std::vector<Puzzle*> toSolve;


  while (!p->isSolved()) {

    if (testLevel > 1 && highestVisitedPosition < p->positionOnVisited) {
      printf("highestVisitedPosition is now %i\n", p->positionOnVisited);
      highestVisitedPosition = p->positionOnVisited;
    }
    // step 1
    if (currentDepth >= parallelStartDepth && checkCurrentCandidates(p)) {
      Puzzle* forSolving = p->makePreassignedCopy();

      if (testLevel > 1) {
        printf("making copy: old candidates are\n");
        p->printInitCandidates();
        printf("new candidates are\n");
        forSolving->printInitCandidates();
      }
      toSolve.push_back(forSolving);

      printf("toSolve size: %i\n", (int)toSolve.size());

      if (reverseSlot(p)) {
        currentDepth--;
        backtrack(p);
      }
      else {
        break;
      }
    }
    else if (checkCurrentCandidates(p)) {
      //printf("step 3\n");
      // step 3
      bool succeeded = assignAndValidate(p);
      if (succeeded) {
        //printf("step 5\n");
        // step 5
        advanceSlot(p);
        currentDepth++;
      }
      else {
        //printf("step 4\n");
        // step 4
        backtrack(p);
      }
    }
    else {
      //printf("step 2\n");
      // step 2
      if (reverseSlot(p)) {
        currentDepth--;
        //printf("step 4\n");
        // step 4
        backtrack(p);
      }
      else {
        //printf("step 2 failed\n");
        break;
      }
    }
  }

  //printf("deinitializing\n");
  p->deinitialize();
  //printf("deinitialized\n");
  delete p;
  p = NULL;

  printf("number of branches: %i\n", (int)toSolve.size());

  p = solveInitializedPuzzles(&toSolve, highestVisitedPosition);

  printf("parallel start depth was %i\n", parallelStartDepth);
  
  if (p != NULL && p->isSolved()) {
    printf("solution found!\n");
    p->printGrid();
  }
  else {
    printf("no solution found\n");
  }

    //printf("deinitializing\n");
  p->deinitialize();
  //printf("deinitialized\n");
  delete p;

  //timer
  double seconds_since_start = difftime( time(0), startTime);
  printf("%f seconds to run program\n", seconds_since_start);

  gettimeofday(&end, NULL);
  seconds  = end.tv_sec  - start.tv_sec;
  useconds = end.tv_usec - start.tv_usec;
  mtime = ((seconds) * 1000 + useconds/1000.0) + 0.5;
  printf("Elapsed time: %ld milliseconds\n", mtime);
}
