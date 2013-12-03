#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <time.h>
#include <sys/time.h>
#include <vector>

#include "sudoku_helper_serial_unit_prop_threaded.cpp"

int numThreads = 64;
int parallelStartDepth = 40;
int vectorSizeLimit = 100;

// step 1
bool checkCurrentCandidates(Puzzle* p) {
  if (testLevel > 0) printf("step 1: on (%i, %i)\n", p->getCurrentRow(), p->getCurrentCol());
  return p->checkCandidates(p->getCurrentRow(), p->getCurrentCol(), false);
}


// step 2. Returns whether reversing the slot succeeds.
bool reverseSlot(Puzzle* p) {
  if (testLevel > 0) printf("step 2: reversing slot\n");
  return p->prevSlot();
}


// step 3. Returns whether assignment succeeds
bool assignAndValidate(Puzzle* p) {
  if (testLevel > 1) printf("pre-step 3:\n");
  if (p->assign(p->getCurrentRow(), p->getCurrentCol())) {
    if (testLevel > 0) 
      printf("step 3: on (%i, %i), assigned %i\n", p->getCurrentRow(), p->getCurrentCol(), p->getCurrentAssigned(p->getCurrentRow(), p->getCurrentCol()));
    p->updateNeighborConflicts(p->getCurrentRow(), p->getCurrentCol(), p->getCurrentAssigned(p->getCurrentRow(), p->getCurrentCol()), true);
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
  p->updateNeighborConflicts(p->getCurrentRow(), p->getCurrentCol(), p->getCurrentAssigned(p->getCurrentRow(), p->getCurrentCol()), false);
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
      // step 3
      bool succeeded = assignAndValidate(p);
      if (succeeded) {
        // step 5
        advanceSlot(p);
      }
      else {
        // step 4
        backtrack(p);
      }
    }
    else {
      // step 2
      if (reverseSlot(p)) {
        // step 4
        backtrack(p);
      }
      else {
        break;
      }
    }
  }

  return p->isSolved();
}

Puzzle* solveInitializedPuzzles(std::vector<Puzzle*>* toSolve, int highestVisitedPosition, int* numTried) {
  Puzzle* p = NULL;

  omp_set_num_threads(numThreads);

  #pragma omp parallel for
  for (int i = toSolve->size()-1; i >= 0; i--) {

    int thisPuzzleNum;
    #pragma omp critical
    {
      numTried[0] += 1;
      thisPuzzleNum = numTried[0];
    }

    if (testLevel > 1) printf("attempting to solve initialized puzzle: puzzle num %i\n", thisPuzzleNum);
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
      toSolve->at(i)->deinitialize();
      delete toSolve->at(i);
        numTried[0] += 1;
        if (testLevel > 1) printf("led to dead end: puzzle num %i\n", thisPuzzleNum);
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

  if (dim<16) parallelStartDepth=86;

  int highestVisitedPosition = 0;
  int currentDepth = 0;
  int numTried = 0;
  std::vector<Puzzle*> toSolve;
  Puzzle* solved = NULL;

  while (!p->isSolved()) {

    if (testLevel > 1 && highestVisitedPosition < p->positionOnVisited) {
      if (testLevel > 1) printf("highestVisitedPosition is now %i\n", p->positionOnVisited);
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

      if (testLevel > 1) printf("toSolve size: %i\n", (int)toSolve.size());

      if (reverseSlot(p)) {
        currentDepth--;
        backtrack(p);
      }
      else {
        break;
      }

      if (toSolve.size() >= vectorSizeLimit) {
        if (testLevel > 1) printf("toSolve reached %i elements, attempting solve now\n", vectorSizeLimit);
        solved = solveInitializedPuzzles(&toSolve, highestVisitedPosition, &numTried);
        if (solved != NULL) {
          break;
        }
        else while (toSolve.size() > 0) {
          toSolve.pop_back();
        }
      }
    }
    else if (checkCurrentCandidates(p)) {
      // step 3
      bool succeeded = assignAndValidate(p);
      if (succeeded) {
        // step 5
        advanceSlot(p);
        currentDepth++;
      }
      else {
        // step 4
        backtrack(p);
      }
    }
    else {
      // step 2
      if (reverseSlot(p)) {
        currentDepth--;
        // step 4
        backtrack(p);
      }
      else {
        //printf("step 2 failed\n");
        break;
      }
    }
  }

  if (!p->isSolved()) {
    p->deinitialize();
    delete p;

    if (testLevel > 1) printf("number of branches: %i\n", (int)toSolve.size());

    if (solved == NULL) {
      solved = solveInitializedPuzzles(&toSolve, highestVisitedPosition, &numTried);
    }

    if (testLevel > 1) printf("parallel start depth was %i\n", parallelStartDepth);
  }
  else {
    solved = p;
    printf("puzzle solved while creating parallel branches, no parallelism done\n");
  }
  
  if (solved != NULL && solved->isSolved()) {
    printf("solution found!\n");
    solved->printGrid();
  }
  else {
    printf("no solution found\n");
  }

  solved->deinitialize();
  delete solved;

  //timer
  double seconds_since_start = difftime( time(0), startTime);
  printf("%f seconds to run program\n", seconds_since_start);

  gettimeofday(&end, NULL);
  seconds  = end.tv_sec  - start.tv_sec;
  useconds = end.tv_usec - start.tv_usec;
  mtime = ((seconds) * 1000 + useconds/1000.0) + 0.5;
  printf("Elapsed time: %ld milliseconds\n", mtime);
}
