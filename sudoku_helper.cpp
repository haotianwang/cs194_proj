#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <math.h>

void delete2dIntArray(int** array, int dim);
void delete2dBoolArray(bool** array, int dim);
void freeEmpty3dBoolArray(bool*** array, int dim);
int** new2dIntArray(int dim);
bool** new2dBoolArray(int dim);
bool*** mallocEmpty3dBoolArray(int dim); 
void print2dArray(int** grid, int dim);
int* file2dArray(char* fileName, int dim);
std::string convertInt(int number);

struct CandidateList {
	int* conflicts;
  int dim;
  
  public:
    CandidateList(int inputDim) {
      dim = inputDim;
    }
    
    void initialize() {
      conflicts = new int[dim];
      for (int i = 0; i < dim; i++) {
        conflicts[i] = 0;
      }
    }
    
    void deinitialize() {
      delete conflicts;
    }
    
    std::string toString() {
      std::string result = "[";
      for (int i = 0; i < dim; i++) {
        result.append(convertInt(conflicts[i]));
        if (i+1 < dim) {
          result.append(",");
        }
      }
      result.append("]");
      return result;
    }
    
    void changeConflict(int num, int change) {
      // correct for starting at 0
      num = num - 1;
      conflicts[num] = conflicts[num] + change;
    }

   //returns if there is still a valid candidate for this slot
    bool checkCandidates() //Brennan
	{
		for (int i = 0; i<dim; i++)
		{
			if (conflicts[i]==0) return true;
		}
		return false;
	}
  
  void invalidateCandidate(int i)
  {
    conflicts[i-1]=-1;
  }

};

struct Puzzle {
	// passed in
  int** sudoku;
  int dim;
  bool initialized;
  
  // need to initialize this
  int currentRow;
	int currentCol;
  int blockSize;
	bool** preassigned;
	CandidateList*** initCandidates;
	CandidateList*** currentCandidates;
  
  public: 
    Puzzle(int** grid, int dimInput) {
      sudoku = grid;
      dim = dimInput;
      blockSize = (int) sqrt ((float) dim);
      initialized = false;
    }
    
    void initialize() {
      //printf("starting initialize\n");
      setupPreassigned();
      //printf("preassigned set\n");
      setupCandidateLists();
      //printf("candidate lists set\n");
      currentCol = 0;
      currentRow = 0;
      while (preassigned[currentRow][currentCol]) {
        nextSlot();
      }
      //printf("slot initialized\n");
      copyCandidates(currentRow, currentCol);
      //printf("candidates copied\n");
      initialized = true;
      //printf("initialized\n");
    }
    
    void deinitialize() {
      teardownPreassigned();
      teardownCandidateLists();
      initialized = false;
    }
    
    void setupPreassigned() {
      preassigned = new2dBoolArray(dim);
      for (int i = 0; i < dim; i++) {
        for (int j = 0; j < dim; j++) {
          if (sudoku[i][j] > 0) {
            preassigned[i][j] = true;
          }
          else {
            preassigned[i][j] = false;
          }
        }
      }
    }
    
    void setupCandidateLists() {
      initCandidates = new CandidateList**[dim];
      currentCandidates = new CandidateList**[dim];
      for (int i = 0; i < dim; i++) {
        initCandidates[i] = new CandidateList*[dim];
	      currentCandidates[i] = new CandidateList*[dim];
        for (int j = 0; j < dim; j++) {
          initCandidates[i][j] = new CandidateList(dim);
          initCandidates[i][j]->initialize();
          currentCandidates[i][j] = new CandidateList(dim);
          currentCandidates[i][j]->initialize();
        }
      }
      
      for (int i = 0; i < dim; i++) {
        for (int j = 0; j < dim; j++) {
          int element = sudoku[i][j];
          if (element > 0) {
            updateNeighborConflicts(i, j, element, true); 
          }
        }
      }
    }
    
    void teardownPreassigned() {
      delete2dBoolArray(preassigned, dim);
    }
    
    void teardownCandidateLists() {
      for (int i = 0; i < dim; i++) {
        for (int j = 0; j < dim; j++) {
          initCandidates[i][j]->deinitialize();
          delete initCandidates[i][j];
        }
        delete initCandidates[i];
      }
      delete initCandidates;
    }
    
    void printGridDim() {
      printf("grid is of dimension %i\n", dim);
    }
    
    void printGrid() {
      print2dArray(sudoku, dim);
    }
    
    void printPreassigned() {
      for (int i = 0; i < dim; i++) {
        for (int j = 0; j < dim; j++) {
          std::cout << preassigned[i][j] << " ";
        }
        std::cout << "\n";
      }
    }
    
    void printInitCandidates() {
      for (int i = 0; i < dim; i++) {
        for (int j = 0; j < dim; j++) {
          if (preassigned[i][j]) {
            std::cout << "[";
            for (int k = 0; k < dim; k++) {
              std::cout << "-9";
              if (k + 1 < dim) {
                std::cout << ",";
              }
            }
            std::cout << "] ";
          }
          else {
            std::cout << initCandidates[i][j]->toString() << " ";
          }
        }
        std::cout << "\n";
      }
    }
    
    void copyCandidates(int x, int y) //Brennan
    {
      for (int i = 0; i < dim; i++) {
        currentCandidates[x][y]->conflicts[i] = initCandidates[x][y]->conflicts[i];
      }
    }
    
    void resetCandidates(int x, int y) //Brennan
    {
    	//initCandidates[x][y]=currentCandidates[x][y];
    }
    
    bool nextSlot() //Brennan
    {
    	if (currentCol==dim-1)
    	{
    		if (currentRow!=dim-1)
    		{
    			currentCol=0;
    			currentRow+=1;
    		}
    		else
    		{
    			return false;
    		}
    	}
    	else
    	{
    		currentCol+=1;
    	}

    	if (preassigned[currentRow][currentCol] ==true)
    	{
    		return nextSlot();
    	}

    	return true;
    }
    
    
    bool prevSlot() //Brennan
    {
    	if (currentCol==0)
    	{
			  if (currentRow!=0)
			  {
				  currentCol=dim-1;
				  currentRow-=1;
			  }
			  else
			  {
				  return false;
			  }
  		}
  		else
    	{
    		currentCol-=1;
    	}

    	if (preassigned[currentRow][currentCol]==true)
    	{
    		return prevSlot();
    	}

    	return true;
    }
    
    
    int getCurrentRow() //Brennan
    {
    	return currentRow;
    }
    int getCurrentCol() //Brennan
    {
    	return currentCol;
    }

    int getCurrentAssigned(int x, int y) {
      return sudoku[x][y];
    }
    
    void removeAndInvalidate(int x, int y) //Brennan
    {
      currentCandidates[x][y]->invalidateCandidate(sudoku[x][y]);
      sudoku[x][y]=-1;
    }

    
    //the conflicts count of x, y, number i's conflicts gets incremented by delta
    void changeConflicts(int x, int y, int i, int delta, bool initialList) {
      CandidateList* list;
      if (initialList) {
        list = initCandidates[x][y];
      }
      else {
        list = currentCandidates[x][y];
      }
      list->changeConflict(i, delta);
    }
    
    // wrappers to add/subtract conflict counts to currentCandidates[x,y,i] or 
    // initCandidate[x,y,i] 
    void incrConflict(int x, int y, int i, bool initialList) {
      changeConflicts(x, y, i, 1, initialList);
    }
    void decrConflict(int x, int y, int i, bool initialList) {
      changeConflicts(x, y, i, -1, initialList);
    }// haotian
    
    bool checkCandidates(int x, int y, bool initialList) {
      CandidateList*** myCandidates;
      if (initialList) {
        myCandidates = initCandidates;
      }
      else {
        myCandidates = currentCandidates;
      }

      return myCandidates[x][y]->checkCandidates();
    }
    
    // update the candidates list of row x, column y, and block (x,y) and add/subtract a conflict 
    // to i. update initial candidate lists
    void updateNeighborConflicts(int x, int y, int i, bool addConflict) {
      for (int index = 0; index < dim; index++) {
        if (!preassigned[x][index]) {
          if (addConflict) {
            incrConflict(x, index, i, true);
          }
          else {
            decrConflict(x, index, i, true);
          }
        }
        
        if (!preassigned[index][y]) {
          if (addConflict) {
            incrConflict(index, y, i, true);
          }
          else {
            decrConflict(index, y, i, true);        
          }
        }
      }
      
      //printf("updateNeighborConflicts starting block logic\n");
      
      int startBlockRow = startOfCurrentBlockRow(x);
      int startBlockCol = startOfCurrentBlockCol(y);
      int endBlockRow = startBlockRow + blockSize;
      int endBlockCol = startBlockCol + blockSize;
      
      //printf("slot to update is %i, %i\n", x, y);
  //    printf("startBlockRow: %i, startBlockCol: %i, endBlockRow: %i, endBlockCol: %i\n", startBlockRow, startBlockCol, endBlockRow, endBlockCol);
      
      for (int j = startBlockRow; j < endBlockRow; j++) {
        for (int k = startBlockCol; k < endBlockCol; k++) {
          //printf("looping to slot %i, %i\n", j, k);
          if (j != x && k != y && !preassigned[j][k]) {
            //printf("adding block conflict to %i, %i\n", j, k);
            if (addConflict) {
              incrConflict(j, k, i, true);
            }
            else {
              decrConflict(j, k, i, true);        
            }
          }
        }
      }

      if (addConflict)
      {
        decrConflict(x, y, i, true);      
        decrConflict(x, y, i, true);
      }
      else
      {
        incrConflict(x,y,i,true);
        incrConflict(x,y,i,true);
      }
      //printf("updateNeighborConflicts end block logic\n");
    }

    int startOfCurrentBlockRow(int rowIndex) {
      return startOfCurrentBlockIndex(rowIndex);
    }
    
    int startOfCurrentBlockCol(int colIndex) {
      return startOfCurrentBlockIndex(colIndex);
    }
    
    int startOfCurrentBlockIndex(int currentIndex) {
      return (currentIndex / blockSize) * blockSize;
    }
    
    bool checkNeighborCandidates(int x, int y) {
      for (int i = 0; i < dim; i++) {
        if (!preassigned[x][i]) {
          if (!checkCandidates(x, i, true)) {
            //printf("checkNeighborCandidates returning false at: %i, %i\n", x, i);
            //std::cout << initCandidates[x][i]->toString() << " ";
            return false;
          }
        }
        if (!preassigned[i][y]) {
          if (!checkCandidates(i, y, true)) {
            //printf("checkNeighborCandidates returning false at: %i, %i\n", i, y);
            //std::cout << initCandidates[i][y]->toString() << " ";
            return false;
          }
        }
      }
      
      int startBlockRow = startOfCurrentBlockRow(x);
      int startBlockCol = startOfCurrentBlockCol(y);
      int endBlockRow = startBlockRow + blockSize;
      int endBlockCol = startBlockCol + blockSize;
      
      for (int j = startBlockRow; j < endBlockRow; j++) {
        for (int k = startBlockCol; k < endBlockCol; k++) {
          if (j != x && k != y) {
            if (!checkCandidates(j, k, true)) {
              //printf("checkNeighborCandidates returning false at: %i, %i\n", j, k);
              //std::cout << initCandidates[j][k]->toString() << " ";
              return false;
            }
          }
        }
      }
//      printf("checkNeighbor
      return true;
    }

    // assigns next number in currentCandidateList to x,y, returns success/failure
    bool assign(int x, int y) {
      int toBeAssigned = nextCandidate(x, y);
      //printf("next candidate found to be %i\n", toBeAssigned);
      if (toBeAssigned == -1) {
        return false;
      }
      else {
        sudoku[x][y] = toBeAssigned;
        return true;
      }
    }
    
    int nextCandidate(int x, int y) {
      CandidateList* list = currentCandidates[x][y];
      for (int i = 0; i < list->dim; i++) {
        if (list->conflicts[i] == 0) {
          return i+1;
        }
      }
      return -1;
    }

    bool isSolved() {
      if (currentRow != dim - 1 || currentCol != dim - 1) {
        return false;
      }
      
      for (int i = 0; i < dim; i++) {
        for (int j = 0; j < dim; j++) {
          if (getCurrentAssigned(i, j) < 1) {
    //        printf("error: got to end but a number is unassigned\n");
            return false;
          }
        }
      }
      
      if (getCurrentAssigned(dim-1, dim-1) < 1) {
        return false;
      }
      
      return true;
    }
};




int** fileTo2dArray(char* fileName, int dim) {
  std::string line;
  std::ifstream infile;
  infile.open(fileName);
  
  int** grid = new2dIntArray(dim);
  
  int i = 0;
  while (getline(infile, line)) {
    std::string field;
    std::stringstream stream(line);
    int j = 0;
    while(getline(stream, field, ',')) {
        int num = atoi(field.c_str());
        grid[i][j] = num;
        j++;
    }
    i++;
  }
  infile.close();
  return grid;
}

int** new2dIntArray(int dim) {
  int** theArray = new int*[dim];
  for (int i = 0; i < dim; i++) {
    theArray[i] = new int[dim];
  }
  return theArray;
}

bool** new2dBoolArray(int dim) {
  bool** theArray = new bool*[dim];
  for (int i = 0; i < dim; i++) {
    theArray[i] = new bool[dim];
  }
  return theArray;
}

void print2dArray(int** grid, int dim) {
  for (int first = 0; first < dim; first++) {
    for (int second = 0; second < dim; second++) {
      std::cout << grid[first][second] << " ";
    }
    std::cout << "\n";
  }
}

void delete2dIntArray(int** grid, int dim) {
  for (int i = 0; i < dim; i++) {
    delete grid[i];
  }
  delete grid;
}

void delete2dBoolArray(bool** grid, int dim) {
  for (int i = 0; i < dim; i++) {
    delete grid[i];
  }
  delete grid;
}

std::string convertInt(int number)
{
   std::stringstream ss;//create a stringstream
   ss << number;//add number to the stream
   return ss.str();//return a string with the contents of the stream
}
