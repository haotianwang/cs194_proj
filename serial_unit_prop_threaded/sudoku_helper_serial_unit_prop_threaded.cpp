#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <math.h>
#include <omp.h>

void delete2dIntArray(int** array, int dim);
void delete2dBoolArray(bool** array, int dim);
void freeEmpty3dBoolArray(bool*** array, int dim);
int** new2dIntArray(int dim);
bool** new2dBoolArray(int dim);
bool*** mallocEmpty3dBoolArray(int dim); 
void print2dArray(int** grid, int dim);
int* file2dArray(char* fileName, int dim);
void copy2dIntArray(int** source, int** destination, int dim);
std::string convertInt(int number);

static int testLevel = 0;

struct CandidateList {
	int* conflicts;
  int dim;
  int num;
  bool init;
  
  public:
    CandidateList(int inputDim) {
      dim = inputDim;
    }

    void copyFrom(CandidateList source) {
      size_t size = dim*sizeof(int);
      memcpy(conflicts, source.conflicts, size);
      num = source.num;
    }
    
    void initialize(bool initial) {
      num=dim;
      init=initial;
      conflicts = new int[dim];
      int i=0;
      for (i; i < dim-4; i+=4) {
        conflicts[i] = 0;
        conflicts[i+1] = 0;
        conflicts[i+2] = 0;
        conflicts[i+3] = 0;
      }
      for (i; i < dim; i++) {
        conflicts[i] = 0;
      }
    }
    
    void deinitialize() {
      delete[] conflicts;
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
    
    void changeConflict(int nums, int change) 
    {
      // correct for starting at 0
      int val=conflicts[nums-1];
      val=val + change;
      if(conflicts[nums-1]>0 && val==0)
      {
        num=num+1;
      }
      if(conflicts[nums-1]<1 && val>0)
      {
        num=num-1;
      }
      conflicts[nums-1]=val;
    }

   //returns if there is still a valid candidate for this slot
    bool checkCandidates() //Brennan
	{
    // if (num>0 && !init)
    // {
      // printf("valid num candidates: num=%i\n", num);    
      // return true;
    // }
    // else
    // {
      // printf("RETURNED FALSE\n");
      // return false;
    // }
    int i=0;
    for (i; i<dim-4; i+=4)
		{
			if (conflicts[i]==0) return true;
      if (conflicts[i+1]==0) return true;
      if (conflicts[i+2]==0) return true;
      if (conflicts[i+3]==0) return true;
		}
    for (i; i<dim; i++)
		{
			if (conflicts[i]==0) return true;
		}
		return false;
	}
  
  int numCandidates()
  {
    return num;
  }
  
  void invalidateCandidate(int i)
  {
    if (conflicts[i-1]==0)
    {
      num=num-1;
    }
    conflicts[i-1]=-1;
  }

};

struct Puzzle {
	// passed in
  int** sudoku;
  int dim;
  bool initialized;

  int* rowsVisited;
  int* colsVisited;
  int positionOnVisited;
  int numUnassigned;
  
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

    Puzzle* makePreassignedCopy() {
      int** newSudoku = new2dIntArray(dim);
      copy2dIntArray(sudoku, newSudoku, dim);
      Puzzle* result = new Puzzle(newSudoku, dim);
      result->setupPreassigned();
      result->setupVisited();
      result->createCandidateLists();
      int j;
      for (int i = 0; i < dim; i++) 
      {
        for (j=0; j < dim-4; j+=4) 
        {
          result->initCandidates[i][j]->copyFrom(*initCandidates[i][j]);
          result->initCandidates[i][j+1]->copyFrom(*initCandidates[i][j+1]);
          result->initCandidates[i][j+2]->copyFrom(*initCandidates[i][j+2]);
          result->initCandidates[i][j+3]->copyFrom(*initCandidates[i][j+3]);
        }
        for (j; j < dim; j++) 
        {
          result->initCandidates[i][j]->copyFrom(*initCandidates[i][j]);
        }
      }
      result->currentRow = currentRow;
      result->currentCol = currentCol;

      result->currentCandidates[currentRow][currentCol]->copyFrom(*currentCandidates[currentRow][currentCol]);

      result->positionOnVisited = 0;
      result->initialized = true;

      return result;
    }
    
    void initialize() {
      //printf("starting initialize\n");
      setupPreassigned();
      //printf("preassigned set\n");
      setupVisited();
      setupCandidateLists();
      //printf("candidate lists set\n");
      currentCol = 0;
      currentRow = 0;
      positionOnVisited = -1;
      //while (preassigned[currentRow][currentCol]) {
        nextSlot();
      //}
      //printf("slot initialized\n");
      copyCandidates(currentRow, currentCol);
      //printf("candidates copied\n");



      initialized = true;
      //printf("initialized\n");
    }
    
    void deinitialize() {
      teardownPreassigned();
      teardownCandidateLists();
      teardownVisited();
      teardownSudoku();
      initialized = false;
    }

    void setupVisited() {
      rowsVisited = new int[numUnassigned+1];
      colsVisited = new int[numUnassigned+1];
      int positionOnVisited = -1;
    }

    void teardownVisited() {
      delete[] rowsVisited;
      delete[] colsVisited;
    }

    void teardownSudoku() {
      delete2dIntArray(sudoku, dim);
    }
    
    void setupPreassigned() {
      numUnassigned = dim*dim;
      preassigned = new2dBoolArray(dim);
      int j;
      for (int i = 0; i < dim; i++) {
        for (j = 0; j < dim-2; j+=2) 
        {
          if (sudoku[i][j] > 0) 
          {
            preassigned[i][j] = true;
            numUnassigned--;
          }
          else 
          {
            preassigned[i][j] = false;
          }
          
          if (sudoku[i][j+1] > 0) 
          {
            preassigned[i][j+1] = true;
            numUnassigned--;
          }
          else 
          {
            preassigned[i][j+1] = false;
          }
        }
        
        for (j; j < dim; j++) 
        {
          if (sudoku[i][j] > 0) 
          {
            preassigned[i][j] = true;
            numUnassigned--;
          }
          else 
          {
            preassigned[i][j] = false;
          }
        }
      }
    }
    
    void createCandidateLists() 
    {
      initCandidates = new CandidateList**[dim];
      currentCandidates = new CandidateList**[dim];
      int j;
      for (int i = 0; i < dim; i++) 
      {
        initCandidates[i] = new CandidateList*[dim];
        currentCandidates[i] = new CandidateList*[dim];
        for (j = 0; j < dim-4; j+=4) 
        {
          initCandidates[i][j] = new CandidateList(dim);
          initCandidates[i][j]->initialize(true);
          currentCandidates[i][j] = new CandidateList(dim);
          currentCandidates[i][j]->initialize(false);
          
          initCandidates[i][j+1] = new CandidateList(dim);
          initCandidates[i][j+1]->initialize(true);
          currentCandidates[i][j+1] = new CandidateList(dim);
          currentCandidates[i][j+1]->initialize(false);
          
          initCandidates[i][j+2] = new CandidateList(dim);
          initCandidates[i][j+2]->initialize(true);
          currentCandidates[i][j+2] = new CandidateList(dim);
          currentCandidates[i][j+2]->initialize(false);
          
          initCandidates[i][j+3] = new CandidateList(dim);
          initCandidates[i][j+3]->initialize(true);
          currentCandidates[i][j+3] = new CandidateList(dim);
          currentCandidates[i][j+3]->initialize(false);
        }
        
        for (j; j < dim; j++) 
        {
          initCandidates[i][j] = new CandidateList(dim);
          initCandidates[i][j]->initialize(true);
          currentCandidates[i][j] = new CandidateList(dim);
          currentCandidates[i][j]->initialize(false);
        }
      }
    }

    void setupCandidateLists() {
      createCandidateLists();
      int j;
      int element;
      for (int i = 0; i < dim; i++) 
      {
        for (j = 0; j < dim-2; j+=2) 
        {
          element = sudoku[i][j];
          if (element > 0) 
          {
            updateNeighborConflicts(i, j, element, true);
          }
          
          element = sudoku[i][j+1];
          if (element > 0) 
          {
            updateNeighborConflicts(i, j+1, element, true);
          }
        }
        
        for (j; j < dim; j++) 
        {
          element = sudoku[i][j];
          if (element > 0) 
          {
            updateNeighborConflicts(i, j, element, true);
          }
        }
      }
    }
    
    void teardownPreassigned() {
      delete2dBoolArray(preassigned, dim);
    }
    
    void teardownCandidateLists() 
    {
      int j;
      for (int i = 0; i < dim; i++) 
      {
        for (j = 0; j < dim-4; j+=4) 
        {
          initCandidates[i][j]->deinitialize();
          currentCandidates[i][j]->deinitialize();
          delete initCandidates[i][j];
          delete currentCandidates[i][j];
          
          initCandidates[i][j+1]->deinitialize();
          currentCandidates[i][j+1]->deinitialize();
          delete initCandidates[i][j+1];
          delete currentCandidates[i][j+1];
          
          initCandidates[i][j+2]->deinitialize();
          currentCandidates[i][j+2]->deinitialize();
          delete initCandidates[i][j+2];
          delete currentCandidates[i][j+2];
          
          initCandidates[i][j+3]->deinitialize();
          currentCandidates[i][j+3]->deinitialize();
          delete initCandidates[i][j+3];
          delete currentCandidates[i][j+3];
        }
        
        for (j; j < dim; j++) 
        {
          initCandidates[i][j]->deinitialize();
          currentCandidates[i][j]->deinitialize();
          delete initCandidates[i][j];
          delete currentCandidates[i][j];
        }
        delete[] initCandidates[i];
        delete[] currentCandidates[i];
      }
      delete[] initCandidates;
      delete[] currentCandidates;
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
      /*
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
      */
      if (numUnassigned == positionOnVisited + 1) 
      {
        return false;
      }
      else 
      {
        int newRow = 0;
        int newCol = 0;
        int newNumCandidates = dim + dim;
        
        for (int row = 0; row < dim; row++) 
        {
          for (int col = 0; col < dim; col++) 
          {
            if (!preassigned[row][col] && sudoku[row][col] == -1) 
            {
              int thisNumCandidates = numCandidates(row, col, true);
              if (thisNumCandidates < newNumCandidates) 
              {
                newNumCandidates = thisNumCandidates;
                newRow = row;
                newCol = col;
              }
            }
          }
        }

        if (positionOnVisited >= 0) 
        {
          rowsVisited[positionOnVisited] = getCurrentRow();
          colsVisited[positionOnVisited] = getCurrentCol();
        }

        positionOnVisited++;
        currentRow = newRow;
        currentCol = newCol;

        return true;
      }
    }

    int numCandidates(int x, int y, int initList) {
      CandidateList* list = initCandidates[x][y];
      if (!initList) {
        list = currentCandidates[x][y];
      }
      return list->numCandidates();
    }
    
    
    bool prevSlot() //Brennan
    {
      /*
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
      */
      if (positionOnVisited < 1) {
        return false;
      }
      else {
        positionOnVisited--;
        currentRow = rowsVisited[positionOnVisited];
        currentCol = colsVisited[positionOnVisited];
        return true;
      }
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
      int index=0;
      for (index; index < dim-4; index+=4) 
      {
        if (!preassigned[x][index]) 
        {
          if (addConflict) 
          {
            incrConflict(x, index, i, true);
          }
          else 
          {
            decrConflict(x, index, i, true);
          }
        }
        
        if (!preassigned[index][y]) 
        {
          if (addConflict) 
          {
            incrConflict(index, y, i, true);
          }
          else 
          {
            decrConflict(index, y, i, true);        
          }
        }
        
        if (!preassigned[x][index+1]) 
        {
          if (addConflict) 
          {
            incrConflict(x, index+1, i, true);
          }
          else 
          {
            decrConflict(x, index+1, i, true);
          }
        }
        
        if (!preassigned[index+1][y]) 
        {
          if (addConflict) 
          {
            incrConflict(index+1, y, i, true);
          }
          else 
          {
            decrConflict(index+1, y, i, true);        
          }
        }
        
        if (!preassigned[x][index+2]) 
        {
          if (addConflict) 
          {
            incrConflict(x, index+2, i, true);
          }
          else 
          {
            decrConflict(x, index+2, i, true);
          }
        }
        
        if (!preassigned[index+2][y]) 
        {
          if (addConflict) 
          {
            incrConflict(index+2, y, i, true);
          }
          else 
          {
            decrConflict(index+2, y, i, true);        
          }
        }
        
        if (!preassigned[x][index+3]) 
        {
          if (addConflict) 
          {
            incrConflict(x, index+3, i, true);
          }
          else 
          {
            decrConflict(x, index+3, i, true);
          }
        }
        
        if (!preassigned[index+3][y]) 
        {
          if (addConflict) 
          {
            incrConflict(index+3, y, i, true);
          }
          else 
          {
            decrConflict(index+3, y, i, true);        
          }
        }
      }
      
      for (index; index < dim; index++) 
      {
        if (!preassigned[x][index]) 
        {
          if (addConflict) 
          {
            incrConflict(x, index, i, true);
          }
          else 
          {
            decrConflict(x, index, i, true);
          }
        }
        
        if (!preassigned[index][y]) 
        {
          if (addConflict) 
          {
            incrConflict(index, y, i, true);
          }
          else 
          {
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
      int k;
      for (int j = startBlockRow; j < endBlockRow; j++) 
      {
        for (k = startBlockCol; k < endBlockCol-4; k+=4) 
        {
          //printf("looping to slot %i, %i\n", j, k);
          if (j != x && k != y && !preassigned[j][k]) 
          {
            //printf("adding block conflict to %i, %i\n", j, k);
            if (addConflict) 
            {
              incrConflict(j, k, i, true);
            }
            else 
            {
              decrConflict(j, k, i, true);        
            }
          }
          
          if (j != x && k+1 != y && !preassigned[j][k+1]) 
          {
            //printf("adding block conflict to %i, %i\n", j, k+1);
            if (addConflict) 
            {
              incrConflict(j, k+1, i, true);
            }
            else 
            {
              decrConflict(j, k+1, i, true);        
            }
          }
          
          
          if (j != x && k+2 != y && !preassigned[j][k+2]) 
          {
            //printf("adding block conflict to %i, %i\n", j, k+2);
            if (addConflict) 
            {
              incrConflict(j, k+2, i, true);
            }
            else 
            {
              decrConflict(j, k+2, i, true);        
            }
          }
          
          
          if (j != x && k+3 != y && !preassigned[j][k+3]) 
          {
            //printf("adding block conflict to %i, %i\n", j, k+3);
            if (addConflict) 
            {
              incrConflict(j, k+3, i, true);
            }
            else 
            {
              decrConflict(j, k+3, i, true);        
            }
          }
        }
        
        for (k; k < endBlockCol; k++) 
        {
          //printf("looping to slot %i, %i\n", j, k);
          if (j != x && k != y && !preassigned[j][k]) 
          {
            //printf("adding block conflict to %i, %i\n", j, k);
            if (addConflict) 
            {
              incrConflict(j, k, i, true);
            }
            else 
            {
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
      int i=0;
      for (i; i < dim-4; i+=4) 
      {
        if (!preassigned[x][i]) 
        {
          if (!checkCandidates(x, i, true)) 
          {
            //printf("checkNeighborCandidates returning false at: %i, %i\n", x, i);
            //std::cout << initCandidates[x][i]->toString() << " ";
            return false;
          }
        }
        if (!preassigned[i][y]) 
        {
          if (!checkCandidates(i, y, true)) 
          {
            //printf("checkNeighborCandidates returning false at: %i, %i\n", i, y);
            //std::cout << initCandidates[i][y]->toString() << " ";
            return false;
          }
        }
        
        if (!preassigned[x][i+1]) 
        {
          if (!checkCandidates(x, i+1, true)) 
          {
            //printf("checkNeighborCandidates returning false at: %i, %i\n", x, i+1);
            //std::cout << initCandidates[x][i+1]->toString() << " ";
            return false;
          }
        }
        if (!preassigned[i+1][y]) 
        {
          if (!checkCandidates(i+1, y, true)) 
          {
            //printf("checkNeighborCandidates returning false at: %i, %i\n", i+1, y);
            //std::cout << initCandidates[i+1][y]->toString() << " ";
            return false;
          }
        }
        
        if (!preassigned[x][i+2]) 
        {
          if (!checkCandidates(x, i+2, true)) 
          {
            //printf("checkNeighborCandidates returning false at: %i, %i\n", x, i+2);
            //std::cout << initCandidates[x][i+2]->toString() << " ";
            return false;
          }
        }
        if (!preassigned[i+2][y]) 
        {
          if (!checkCandidates(i+2, y, true)) 
          {
            //printf("checkNeighborCandidates returning false at: %i, %i\n", i+2, y);
            //std::cout << initCandidates[i+2][y]->toString() << " ";
            return false;
          }
        }
        
        if (!preassigned[x][i+3]) 
        {
          if (!checkCandidates(x, i+3, true)) 
          {
            //printf("checkNeighborCandidates returning false at: %i, %i\n", x, i+3);
            //std::cout << initCandidates[x][i+3]->toString() << " ";
            return false;
          }
        }
        if (!preassigned[i+3][y]) 
        {
          if (!checkCandidates(i+3, y, true)) 
          {
            //printf("checkNeighborCandidates returning false at: %i, %i\n", i+3, y);
            //std::cout << initCandidates[i+3][y]->toString() << " ";
            return false;
          }
        }
      }
      
      for (i; i < dim; i++) 
      {
        if (!preassigned[x][i]) 
        {
          if (!checkCandidates(x, i, true)) 
          {
            //printf("checkNeighborCandidates returning false at: %i, %i\n", x, i);
            //std::cout << initCandidates[x][i]->toString() << " ";
            return false;
          }
        }
        if (!preassigned[i][y]) 
        {
          if (!checkCandidates(i, y, true)) 
          {
            //printf("checkNeighborCandidates returning false at: %i, %i\n", i, y);
            //std::cout << initCandidates[i][y]->toString() << " ";
            return false;
          }
        }
      }
      
      
      int startBlockRow = startOfCurrentBlockRow(x);
      int k = startOfCurrentBlockCol(y);
      int endBlockRow = startBlockRow + blockSize;
      int endBlockCol = k + blockSize;
      
      for (int j = startBlockRow; j < endBlockRow; j++) {
        for (k; k < endBlockCol-4; k+=4) 
        {
          if (j != x && k != y) 
          {
            if (!checkCandidates(j, k, true)) 
            {
              //printf("checkNeighborCandidates returning false at: %i, %i\n", j, k);
              //std::cout << initCandidates[j][k]->toString() << " ";
              return false;
            }
          }
          
          if (j != x && k+1 != y) 
          {
            if (!checkCandidates(j, k+1, true)) 
            {
              //printf("checkNeighborCandidates returning false at: %i, %i\n", j, k+1);
              //std::cout << initCandidates[j][k+1]->toString() << " ";
              return false;
            }
          }
          
          if (j != x && k+2 != y) 
          {
            if (!checkCandidates(j, k+2, true)) 
            {
              //printf("checkNeighborCandidates returning false at: %i, %i\n", j, k+2);
              //std::cout << initCandidates[j][k+2]->toString() << " ";
              return false;
            }
          }
          
          if (j != x && k+3 != y) 
          {
            if (!checkCandidates(j, k+3, true)) 
            {
              //printf("checkNeighborCandidates returning false at: %i, %i\n", j, k+3);
              //std::cout << initCandidates[j][k+3]->toString() << " ";
              return false;
            }
          }
        }
        
        for (k; k < endBlockCol; k++) 
        {
          if (j != x && k != y) 
          {
            if (!checkCandidates(j, k, true)) 
            {
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
      if (testLevel > 1) printf("next candidate found to be %i\n", toBeAssigned);
      if (toBeAssigned == -1) {
        return false;
      }
      else {
        if (testLevel > 1) printf("attempting to set %i at (%i, %i)\n", toBeAssigned, x, y);
        if (testLevel > 1) printf("prev value at (%i, %i) is %i\n", x, y, sudoku[x][y]);
        sudoku[x][y] = toBeAssigned;
        if (testLevel > 1) printf("successfully set %i at (%i, %i)\n", toBeAssigned, x, y);
        return true;
      }
    }
    
    int nextCandidate(int x, int y) {
      CandidateList* list = currentCandidates[x][y];
      int i;
      for (i = 0; i < list->dim; i++) {
        if (list->conflicts[i] == 0) {
          return i+1;
        }
      }
      return -1;
    }

    bool isSolved() {
      // if (currentRow != dim - 1 || currentCol != dim - 1) {
        // return false;
      // }
      
      if (positionOnVisited>numUnassigned) return true;
      
      for (int i = 0; i < dim; i++) {
        for (int j = 0; j < dim; j++) {
          if (getCurrentAssigned(i, j) < 1) {
    //        printf("error: got to end but a number is unassigned\n");
            return false;
          }
        }
      }
      
      // if (getCurrentAssigned(dim-1, dim-1) < 1) {
        // return false;
      // }
      
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
  int i;
  for (i = 0; i < dim; i++) {
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
    delete[] grid[i];
  }
  delete[] grid;
}

void delete2dBoolArray(bool** grid, int dim) {
  for (int i = 0; i < dim; i++) {
    delete[] grid[i];
  }
  delete[] grid;
}

std::string convertInt(int number)
{
   std::stringstream ss;//create a stringstream
   ss << number;//add number to the stream
   return ss.str();//return a string with the contents of the stream
}

void copy2dIntArray(int** source, int** destination, int dim) {
  size_t rowSize = dim*sizeof(int);
  for (int i = 0; i < dim; i++) {
    memcpy(destination[i], source[i], rowSize);
  }
}