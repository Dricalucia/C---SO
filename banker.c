#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

typedef struct ResourceNode {
    int resourceValue;
    struct ResourceNode* next;
} ResourceNode;

int NUMBER_OF_CUSTOMERS;
int NUMBER_OF_RESOURCES;
int *available;    
int **maximum;     
int **allocation;  
int **need;       

void readCustomerFile(char *filename, int NUMBER_OF_RESOURCES);
void readCommandFile(char *filename, int NUMBER_OF_RESOURCES);

int requestResources(int customer_num, int resourceValues[]);
int releaseResources(int customer_num, int resourceValues[]);
void saveResult(FILE *resultFile, const char *action, int customer, const int resources[], int option);
void displayState();
int isSafe();
int isDeadlock();

void calculateMaxWidths(int **matrix, int numRows, int numCols, int *widths);
int countDigits(int number);

int main(int argc, char *argv[]) {
    if (argc == 1) {
        printf("Use: %s <resources>\n", argv[0]);
        return EXIT_FAILURE;
    }

    NUMBER_OF_RESOURCES = argc-1;
    available = (int*) malloc (NUMBER_OF_RESOURCES * sizeof(int));

    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        available[i] = atoi(argv[i + 1]);
    }

    char customerFile[] = "customer.txt";
    char commandFile[] = "commands.txt";

    readCustomerFile(customerFile, NUMBER_OF_RESOURCES);
    readCommandFile(commandFile, NUMBER_OF_RESOURCES);

    if (isDeadlock()) {
        printf("Deadlock detected!\n");
    } else {
        printf("No deadlock.\n");
    }

    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        free(maximum[i]);
        free(allocation[i]);
        free(need[i]);
    }
    free(maximum);
    free(allocation);
    free(need);
    free(available);

    return EXIT_SUCCESS;
}


void readCustomerFile(char *filename, int NUMBER_OF_RESOURCES) {
    int rowCustomer = 0;
    char line[256];
    char lineBckp[256];

    FILE *file = fopen(filename, "r");   
    if (file == NULL) {
        printf("Fail to read customer.txt\n");
        exit(EXIT_FAILURE);
    } else { 
      while (fgets(line, sizeof(line), file) != NULL){
          rowCustomer++;
      }
    }

    NUMBER_OF_CUSTOMERS = rowCustomer;
    int numberOfColumns = 0;
    int columnCustomer = 0;
    rowCustomer = 0;

    maximum = (int**) malloc (NUMBER_OF_CUSTOMERS * sizeof(int*));
    allocation = (int**) malloc (NUMBER_OF_CUSTOMERS * sizeof(int*));
    need = (int**) malloc (NUMBER_OF_CUSTOMERS * sizeof(int*));

    rewind(file); 

    while (fgets(line, sizeof(line), file) != NULL) {
        numberOfColumns = 0;

        char *lineCopy = strdup(line);
        char *token = strtok(line, ",");
        while (token != NULL) {
            token = strtok(NULL, ",");
            numberOfColumns++;
        }

        if (numberOfColumns != NUMBER_OF_RESOURCES) {
            printf("Incompatibility between customer.txt and command line\n");
            fclose(file);
            exit(EXIT_FAILURE);
        }      

        maximum[rowCustomer] = (int*) malloc (NUMBER_OF_RESOURCES * sizeof(int));
        allocation[rowCustomer] = (int*) malloc (NUMBER_OF_RESOURCES * sizeof(int));
        need[rowCustomer] = (int*) malloc (NUMBER_OF_RESOURCES * sizeof(int));

        token = strtok(lineCopy, ",");
        numberOfColumns = 0;
        while (token != NULL) { 
            maximum[rowCustomer][numberOfColumns] = atoi(token);
            need[rowCustomer][numberOfColumns] = atoi(token);
            allocation[rowCustomer][numberOfColumns] = 0;
            token = strtok(NULL, ",");
            numberOfColumns++;
        } 

      rowCustomer++;
    }  
    fclose(file);
}

void readCommandFile(char *filename, int NUMBER_OF_RESOURCES) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Fail to read commands.txt\n");
        exit(EXIT_FAILURE);
    }

    FILE *resultFile = fopen("result.txt", "w");
    if (resultFile == NULL) {
        printf("Fail to write in result.txt\n");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    char line[256];
    while (fgets(line, sizeof(line), file) != NULL) {

        char command[3];
        int customer, numberResources = 0, fCustomer = 0, option = 0;
        char *lineCopy = strdup(line);

        if (line[0] == '*') {
              displayState(resultFile);
        } else if (strncmp(line, "RQ", 2) == 0 || strncmp(line, "RL", 2) == 0) {
              char *command = strtok(line, " ");
              int customer = atoi(strtok(NULL, " "));

              int resourceValues[NUMBER_OF_RESOURCES];
              int numberResources = 0;

              char *token = strtok(NULL, " ");
              while (token != NULL) {
                  resourceValues[numberResources++] = atoi(token);
                  token = strtok(NULL, " ");
              }

              if (numberResources != NUMBER_OF_RESOURCES) {  
                  printf("Incompatibility between commands.txt and command line\n");
                  fclose(file);
                  fclose(resultFile);
                  exit(EXIT_FAILURE);
              }
              int option = 0;
              
              if (strcmp(command, "RQ") == 0){        
                  option = requestResources(customer, resourceValues);
                  saveResult(resultFile, "Allocate to", customer, resourceValues, option);
              } 

              if (strcmp(command, "RL") == 0){ 
                  option = releaseResources(customer, resourceValues); 
                  saveResult(resultFile, "Release from", customer, resourceValues, option);
              }

        } else {
            printf("Invalid command");
            exit(EXIT_FAILURE);
        }
    }
    fclose(file);
    fclose(resultFile);
}

void saveResult(FILE *resultFile, const char *action, int customer, const int resources[], int option) {
    if (option == 0){
       fprintf(resultFile, "%s customer %d the resources ", action, customer);
      for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
          fprintf(resultFile, "%d ", resources[i]);
      }
    } else if (option == -1) { 
      fprintf(resultFile, "The customer %d request ", customer);
      for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
          fprintf(resultFile, "%d ", resources[i]);
      }
      fprintf(resultFile, "was denied because exceed its maximum need");
      } else if (option == -2) {
          fprintf(resultFile, "The customer %d request", customer);
          for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
              fprintf(resultFile, " %d", resources[i]);
          }
          fprintf(resultFile, " was denied because result in an unsafe state");
      } else if (option == -3) {   
        fprintf(resultFile, "The resources");
        for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
            fprintf(resultFile, " %d", available[i]);
        }
        fprintf(resultFile, " are not enough to customer %d request ", customer);
        for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
            fprintf(resultFile, "%d ", resources[i]);
        }
    } else if (option == -4) {   
        fprintf(resultFile, "The customer %d released", customer);
        for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
            fprintf(resultFile, " %d", resources[i]);
        }
        fprintf(resultFile, " was denied because exceed its maximum allocation");

    } else if (option == -5) { 
      fprintf(resultFile, "%s customer %d the resources ", action, customer);
      for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        fprintf(resultFile, "%d ", resources[i]);
      }
    }
    fprintf(resultFile, "\n");
}

int requestResources(int customer_num, int resourceValues[]) {
    for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
        if (resourceValues[j] > available[j]) {
          return -3;
        } else if (resourceValues[j] > need[customer_num][j]) {
            return -1; 
        }
    }

    for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
        available[j] -= resourceValues[j];
        allocation[customer_num][j] += resourceValues[j];
        need[customer_num][j] -= resourceValues[j];
    }

    if (!isSafe()) {
        for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
            available[j] += resourceValues[j];
            allocation[customer_num][j] -= resourceValues[j];
            need[customer_num][j] += resourceValues[j];
        }
        return -2; 
    }

    return 0; 
}

int releaseResources(int customer_num, int resourceValues[]) {
  for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
      if (resourceValues[j] > allocation[customer_num][j]) {
          return -4;
      }
  }

    for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
        available[j] += resourceValues[j];
        allocation[customer_num][j] -= resourceValues[j];
        need[customer_num][j] += resourceValues[j];
    }
  return -5;
}

int isSafe() {
    int work[NUMBER_OF_RESOURCES];
    int finish[NUMBER_OF_CUSTOMERS];

    for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
        work[j] = available[j];
    }

    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        finish[i] = 0;
    }

    int count = 0;
    while (count < NUMBER_OF_CUSTOMERS) {
        int found = 0;
        for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
            if (!finish[i]) {
                int j;
                for (j = 0; j < NUMBER_OF_RESOURCES; j++) {
                    if (need[i][j] > work[j]) {
                        break;
                    }
                }
                if (j == NUMBER_OF_RESOURCES) {
                    for (int k = 0; k < NUMBER_OF_RESOURCES; k++) {
                        work[k] += allocation[i][k];
                    }
                    finish[i] = 1;
                    found = 1;
                    count++;
                }
            }
        }
        if (!found) {
            return 0; 
        }
    }
    return 1; 
}

void displayState(FILE *resultFile) {
    int *maxWidths = malloc(NUMBER_OF_RESOURCES * sizeof(int));
    int *allocWidths = malloc(NUMBER_OF_RESOURCES * sizeof(int));
    int *needWidths = malloc(NUMBER_OF_RESOURCES * sizeof(int));

    calculateMaxWidths(maximum, NUMBER_OF_CUSTOMERS, NUMBER_OF_RESOURCES, maxWidths);
    calculateMaxWidths(allocation, NUMBER_OF_CUSTOMERS, NUMBER_OF_RESOURCES, allocWidths);
    calculateMaxWidths(need, NUMBER_OF_CUSTOMERS, NUMBER_OF_RESOURCES, needWidths);

    int columnTitle = 0;
    fprintf(resultFile, "MAXIMUM");
    for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
        columnTitle += maxWidths[j];
    }
    if (columnTitle > 7){
      for (int i = 0; i < (7 - columnTitle); i++){
          fprintf(resultFile, " ");
      }
    }
    fprintf(resultFile, " | ");
    columnTitle = 0;
    fprintf(resultFile, "ALLOCATION");
    for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
        columnTitle += allocWidths[j];
    }
    if (columnTitle > 10){
      for (int i = 0; i < (10 - columnTitle); i++){
          fprintf(resultFile, " ");
      }
    }
    fprintf(resultFile, " | ");
    fprintf(resultFile, "NEED\n");

    int numberDigitos, addSpace;
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        numberDigitos = 0;
        addSpace = 0;
        
        for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
            if (maxWidths[j] > 1){
              numberDigitos = countDigits(maximum[i][j]);
              if (numberDigitos != maxWidths[j]){
                  for (int p = 0; p < numberDigitos; p++){
                      fprintf(resultFile, " ");
                  }
              }
            }
            addSpace += maxWidths[j];
            fprintf(resultFile, "%d ", maximum[i][j]);
        }

        if ((addSpace+NUMBER_OF_RESOURCES) < 7){
          for (int i = 0; i < (7 - (addSpace+NUMBER_OF_RESOURCES)); i++){
            fprintf(resultFile, " ");
          }
          fprintf(resultFile, " | ");
        } else {
          fprintf(resultFile, "| ");
        }

        numberDigitos = 0;
        addSpace = 0;
        for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
          if (allocWidths[j] > 1){
            numberDigitos = countDigits(allocation[i][j]);
            if (numberDigitos != maxWidths[j]){
                for (int p = 0; p < numberDigitos; p++){
                    fprintf(resultFile, " ");
                }
            }
          }
          addSpace += allocWidths[j];
          fprintf(resultFile, "%d ", allocation[i][j]);
        }

        if ((addSpace+NUMBER_OF_RESOURCES) < 10){
          for (int i = 0; i < (10 - (addSpace+NUMBER_OF_RESOURCES)); i++){
            fprintf(resultFile, " ");
          }
             fprintf(resultFile, " | ");
          } else {
            fprintf(resultFile, "| ");
          }
        
        numberDigitos = 0;
        addSpace = 0;
        for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
          if (needWidths[j] > 1){
            numberDigitos = countDigits(need[i][j]);
            if (numberDigitos != maxWidths[j]){
                for (int p = 0; p < numberDigitos; p++){
                    fprintf(resultFile, " ");
                }
            }
          }
            addSpace += needWidths[j];
            fprintf(resultFile, "%d ", need[i][j]);
        }
        fprintf(resultFile, "\n");
    }

    fprintf(resultFile, "AVAILABLE ");
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        fprintf(resultFile, "%d ", available[i]);
    }
    fprintf(resultFile, "\n");
}


int isDeadlock() {
    int work[NUMBER_OF_RESOURCES];
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        work[i] = available[i];
    }
    int finish[NUMBER_OF_CUSTOMERS];
    int count = 0;
    while (count < NUMBER_OF_CUSTOMERS) {
        int i;
        for (i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
            if (!finish[i]) {
                int j;
                for (j = 0; j < NUMBER_OF_RESOURCES; j++) {
                    if (allocation[i][j] != 0 && allocation[i][j] <= work[j]) {
                        break;
                    }
                }
                if (j == NUMBER_OF_RESOURCES) {
                    for (int k = 0; k < NUMBER_OF_RESOURCES; k++) {
                        work[k] += allocation[i][k];
                    }
                    finish[i] = true;
                    count++;
                    break;
                }
            }
        }
        if (i == NUMBER_OF_CUSTOMERS) {
            break;
        }
    }

    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        if (!finish[i]) {
            return true;
        }
    }
    return 0;
}

void calculateMaxWidths(int **matrix, int numRows, int numCols, int *widths) {
    for (int j = 0; j < numCols; j++) {
        widths[j] = 0;
        for (int i = 0; i < numRows; i++) {
            int digits = snprintf(NULL, 0, "%d", matrix[i][j]);
            if (digits > widths[j]) {
                widths[j] = digits;
            }
        }
    }
}
int countDigits(int number){
    if (number == 0) {
        return 1;
    }
    int count = 0;
    while (number != 0) {
        number /= 10;  
        count++;
    }
    return count;
}