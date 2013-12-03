#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <math.h>
#include <omp.h>
#include <bitset>

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
	//int* conflicts;
  int dim;
  int num;
  bool init;

  // conflicts as 2 bit binary number.
  // 11: 0 conflicts. 10: 1 conflict. 01: 2 conflicts. 00: 3 conflicts
  unsigned int bitZero;
  unsigned int bitOne;
  unsigned int noCandidates;
  
  public:
    CandidateList(int inputDim) {
      dim = inputDim;
      switch (dim) {
        case 4:
          noCandidates = 0b1111;
          break;
        case 9:
          noCandidates = 0b111111111;
          break;
        case 16:
          noCandidates = 0b1111111111111111;
          break;
        case 25:
          noCandidates = 0b1111111111111111111111111;
          break;
        default:
          printf("candidatelist constructor invalid dim %i\n", dim);
          exit(1);
      }
    }

    void copyFrom(CandidateList source) {
      num = source.num;
      bitZero = source.bitZero;
      bitOne = source.bitOne;
    }
    
    void initialize(bool initial) {
      num=dim;
      init=initial;

      // vectorization
      switch (dim) {
        case 4:
          bitZero = 0b0000;
          break;
        case 9:
          bitZero = 0b000000000;
          break;
        case 16:
          bitZero = 0b0000000000000000;
          break;
        case 25:
          bitZero = 0b0000000000000000000000000;
          break;
        default:
          printf("invalid dimension for candidatelist: %i\n", dim);
          exit(1);
      }

      bitOne = bitZero;
    }
    
    void deinitialize() {
      //delete[] conflicts;
    }
    
    std::string toString() {
      std::string result = "[";
      for (int i = 0; i < dim; i++) {
        result.append(convertInt(getNumConflictsVectorized(i+1)));
        if (i+1 < dim) {
          result.append(",");
        }
      }
      result.append("]");
      return result;
    }

    void printVectorString() {
      std::bitset<32> x(bitZero);
      std::bitset<32> y(bitOne);
      printf("zeroth bit: ");
      std::cout << x << "\n";
      printf("first bit: ");
      std::cout << y << "\n";
    }
    
    void changeConflict(int nums, int change) 
    {
      changeConflictVectorized(nums, change);
    }

    int changeConflictsVectorizedNum(int oldNumTracker, int nums, int change) {\
    }

    unsigned int getNumConflictsVectorized(int nums) {
      unsigned int oldNum = (bitZero & (0b1 << nums-1)) | ((bitOne & (0b1 << nums-1)) << 1);
      oldNum = oldNum >> (nums-1);
      oldNum = oldNum & 0b11;
      return oldNum;
    }

    void changeConflictVectorized(int nums, int change) {
      unsigned int oldNum = getNumConflictsVectorized(nums);
      unsigned int newNum = oldNum + change;

      if (oldNum > 0 && newNum == 0) {
        num = num + 1;
      }

      if (oldNum == 0 && newNum > 0) {
        num = num - 1;
      }

      if (newNum > 3) {
        printf("vectorized changed conflict num for number %i is invalid, it's %u\n", nums, newNum);
        printf("candidate list was\n");
        std::cout<< toString() << "\n";
        exit(1);
      }

      unsigned int shiftBit = 1 << (nums-1);

      // if bitOne is on
      if ((newNum & 0b10) != 0b0) {
        //printf("setting bitOne on\n");
        bitOne |= shiftBit;
      }
      else {
        // if bitOne is off
        bitOne &= ~(shiftBit);
      }

      // if bitZero is on
      if ((newNum & 0b1) != 0b0) {
        //printf("setting bitZero on\n");
        bitZero |= shiftBit;
      }
      else {
        // if bitZero is off
        bitZero &= ~(shiftBit);
      }
    }

    //returns if there is still a valid candidate for this slot
    bool checkCandidates() //Brennan
  	{
      return checkCandidatesVectorized();
  	}

    bool checkCandidatesVectorized() {
      unsigned int hasCandidates = bitZero | bitOne; 
      return hasCandidates != noCandidates;
    }
    
    int numCandidates()
    {
      return num;
    }
    
    void invalidateCandidate(int i)
    {
      //vectorization
      changeConflictVectorized(i, 1);
    }

    int nextCandidate() {
      return nextCandidateVectorized();
    }

    int nextCandidateVectorized() {
      unsigned int validCandidatesIndices = ~(bitZero | bitOne);
      return __builtin_ffs(validCandidatesIndices);
    }
};

struct Puzzle {
	// passed in
  int** sudoku;
  int dim;
  bool initialized;
  bool mostConstrained;

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
      mostConstrained = true;
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
      setupPreassigned();
      setupVisited();
      setupCandidateLists();
      currentCol = 0;
      currentRow = 0;
      positionOnVisited = -1;
      nextSlot();
      copyCandidates(currentRow, currentCol);
      initialized = true;
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

      for (int i = 0; i < dim; i++) 
      {
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
    
    void printPreassigned() 
    {
      int j;
      for (int i = 0; i < dim; i++) {
        for (j = 0; j < dim-4; j+=4) {
          std::cout << preassigned[i][j] << " ";
          std::cout << preassigned[i][j+1] << " ";
          std::cout << preassigned[i][j+2] << " ";
          std::cout << preassigned[i][j+3] << " ";
        }
        
        for (j; j < dim; j++) {
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
      currentCandidates[x][y]->copyFrom(*initCandidates[x][y]);
    }
    
    bool nextSlot() //Brennan
    {
      if (numUnassigned == positionOnVisited + 1) 
      {
        return false;
      }
      else 
      {
        if (mostConstrained) {
          nextSlotMostConstrained();
        }
        else {
          nextSlotLeastConstrained();
        }
        return true;
      }
    }

    void nextSlotLeastConstrained() {
      int newRow = 0;
      int newCol = 0;
      int newNumCandidates = -1;
      
      for (int row = 0; row < dim; row++) 
      {
        for (int col = 0; col < dim; col++) 
        {
          if (!preassigned[row][col] && sudoku[row][col] == -1) 
          {
            int thisNumCandidates = numCandidates(row, col, true);
            if (thisNumCandidates > newNumCandidates) 
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
    }

    void nextSlotMostConstrained() {
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
      int index;
      for (index=0; index < dim-4; index+=4) 
      {
        if (!preassigned[x][index] && index != y) 
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
        
        if (!preassigned[index][y] && index != x) 
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
        
        if (!preassigned[x][index+1] && index+1 != y) 
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
        
        if (!preassigned[index+1][y] && index+1 != x) 
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
        
        if (!preassigned[x][index+2] && index+2 != y) 
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
        
        if (!preassigned[index+2][y] && index+2 != x) 
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
        
        if (!preassigned[x][index+3] && index+3 != y) 
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
        
        if (!preassigned[index+3][y] && index+3 != x) 
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
        if (!preassigned[x][index] && index != y) 
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
        
        if (!preassigned[index][y] && index != x) 
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
      
      int startBlockRow = startOfCurrentBlockRow(x);
      int startBlockCol = startOfCurrentBlockCol(y);
      int endBlockRow = startBlockRow + blockSize;
      int endBlockCol = startBlockCol + blockSize;

      int k;
      for (int j = startBlockRow; j < endBlockRow; j++) 
      {
        for (k = startBlockCol; k < endBlockCol-4; k+=4) 
        {
          if (j != x && k != y && !preassigned[j][k]) 
          {
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
          if (j != x && k != y && !preassigned[j][k]) 
          {
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
            return false;
          }
        }
        if (!preassigned[i][y]) 
        {
          if (!checkCandidates(i, y, true)) 
          {
            return false;
          }
        }
        
        if (!preassigned[x][i+1]) 
        {
          if (!checkCandidates(x, i+1, true)) 
          {
            return false;
          }
        }
        if (!preassigned[i+1][y]) 
        {
          if (!checkCandidates(i+1, y, true)) 
          {
            return false;
          }
        }
        
        if (!preassigned[x][i+2]) 
        {
          if (!checkCandidates(x, i+2, true)) 
          {
            return false;
          }
        }
        if (!preassigned[i+2][y]) 
        {
          if (!checkCandidates(i+2, y, true)) 
          {
            return false;
          }
        }
        
        if (!preassigned[x][i+3]) 
        {
          if (!checkCandidates(x, i+3, true)) 
          {
            return false;
          }
        }
        if (!preassigned[i+3][y]) 
        {
          if (!checkCandidates(i+3, y, true)) 
          {
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
            return false;
          }
        }
        if (!preassigned[i][y]) 
        {
          if (!checkCandidates(i, y, true)) 
          {;
            return false;
          }
        }
      }
      
      
      int startBlockRow = startOfCurrentBlockRow(x);
      int k = startOfCurrentBlockCol(y);
      int endBlockRow = startBlockRow + blockSize;
      int endBlockCol = k + blockSize;
      
      for (int j = startBlockRow; j < endBlockRow; j++) 
      {
        for (k; k < endBlockCol-4; k+=4) 
        {
          if (j != x && k != y) 
          {
            if (!checkCandidates(j, k, true)) 
            {
              return false;
            }
          }
          
          if (j != x && k+1 != y) 
          {
            if (!checkCandidates(j, k+1, true)) 
            {
              return false;
            }
          }
          
          if (j != x && k+2 != y) 
          {
            if (!checkCandidates(j, k+2, true)) 
            {
              return false;
            }
          }
          
          if (j != x && k+3 != y) 
          {
            if (!checkCandidates(j, k+3, true)) 
            {
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
              return false;
            }
          }
        }
      }
      return true;
    }

    // assigns next number in currentCandidateList to x,y, returns success/failure
    bool assign(int x, int y) {

      int toBeAssigned = nextCandidate(x, y, false);
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
    
    int nextCandidate(int x, int y, bool initial) {

      CandidateList* list;
      if (initial) {
        list = initCandidates[x][y];
      } else {
        list = currentCandidates[x][y];
      }

      return list->nextCandidate();
    }

    bool isSolved() {

      if (positionOnVisited>numUnassigned) return true;
      
      for (int i = 0; i < dim; i++) {
        for (int j = 0; j < dim; j++) {
          if (getCurrentAssigned(i, j) < 1) {
            return false;
          }
        }
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