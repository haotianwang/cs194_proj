#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

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

    int numConflicts() //Brennan
    {
    	int sum = 0;
    	for (int i = 0; i < dim; i++)
    	{
    		sum+=conflicts[i];
    	}
    	return sum;
    }

};

struct Puzzle {
	// passed in
  int** sudoku;
  int dim;
  bool initialized;
  
  // need to initialize this
 int currentX;
	int currentY;
	bool** preassigned;
	CandidateList*** initCandidates;
	CandidateList*** currentCandidates;
  
  public: 
    Puzzle(int** grid, int dimInput) {
      sudoku = grid;
      dim = dimInput;
      initialized = false;
    }
    
    void initialize() {
      currentX = 0;
      currentY = 0;
      setupPreassigned();
      setupCandidateLists();
      initialized = true;
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
      for (int i = 0; i < dim; i++) {
        initCandidates[i] = new CandidateList*[dim];
        for (int j = 0; j < dim; j++) {
          initCandidates[i][j] = new CandidateList(dim);
          initCandidates[i][j]->initialize();
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
          std::cout << initCandidates[i][j]->toString() << " ";
        }
        std::cout << "\n";
      }
    }
    
    bool checkCandidates(int x, int y) //Brennan
    {
		int conflicts = currentCandidates[x][y]->numConflicts();
		return (conflicts>-1 && conflicts<dim);
    }
    void resetCandidates(int x, int y) //Brennan
    {
    	currentCandidates[x][y]=initCandidates[x][y];
    }
    void copyCandidates(int x, int y) //Brennan
    {
    	initCandidates[x][y]=currentCandidates[x][y];
    }
    bool nextSlot() //Brennan
    {
    	if (currentX==dim)
    	{
    		if (currentY!=dim)
    		{
    			currentX=0;
    			currentY+=1;
    		}
    		else
    		{
    			return false;
    		}
    	}
    	else
    	{
    		currentX+=1;
    	}

    	if (preassigned[currentX][currentY]==true)
    	{
    		return nextSlot();
    	}

    	return true;
    }
    bool prevSlot() //Brennan
    {
    	if (currentX==0)
    	{
			if (currentY!=0)
			{
				currentX=dim;
				currentY-=1;
			}
			else
			{
				return false;
			}
		}
		else
    	{
    		currentX-=1;
    	}

    	if (preassigned[currentX][currentY]==true)
    	{
    		return prevSlot();
    	}

    	return true;
    }
    int getCurrentX() //Brennan
    {
    	return currentX;
    }
    int getCurrentY() //Brennan
    {
    	return currentY;
    }

    void removeAndInvalidate(int x, int y) //Brennan
    {
    	//may not need to deinitialize as initialize seems to overwrite
    	//unclear about representation of assignment as opposed to conflicts?
    	//for now, assuming that resetting CandidateList serves this purpose
    	currentCandidates[x][y]->deinitialize();
    	currentCandidates[x][y]->initialize();

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
    }

    // wrappers to add/subtract conflict counts to currentCandidates[x,y,i] or 
    // initCandidate[x,y,i] 
    void incrConflict(int x, int y, int i, bool initialList) {
      changeConflicts(x, y, i, 1, initialList);
    }
    void decrConflict(int x, int y, int i, bool initialList) {
      changeConflicts(x, y, i, -1, initialList);
    }// haotian

    // assigns next number in currentCandidateList to x,y, returns success/failure
    bool assign(int x, int y); // haotian
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
  delete[] grid;
}

void delete2dBoolArray(bool** grid, int dim) {
  for (int i = 0; i < dim; i++) {
    delete grid[i];
  }
  delete[] grid;
}

std::string convertInt(int number)
{
   std::stringstream ss;//create a stringstream
   ss << number;//add number to the stream
   return ss.str();//return a string with the contents of the stream
}
