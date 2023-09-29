#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>


/*                                              INTER BATCH 
1. Prompt, enter                                  OK    OK
2. History !!                                     OK    OK
3. Saída PRG (CTRL+D ou exit())                   OK    OK
4. Processos                                      OK    OK
5. Sytle Sequencial(;) ou Parallel(&&)            OK    OK
6. Thread                                         OK    OK
7. Pipe                                           OK    OK
8. Background                                     OK    NOK
9. Redirecionamento (mais complexo)               OK    OK
10. Leitura arquivo                               OK    OK
11. Tratativa de erros                            OK    OK
12. Remover espaços em branco na linha de comando OK    OK
*/

#define MAX_COMMAND_LEN 1024
#define MAX_THREADS 100
#define MAX_BACKGROUND_PROCESSES 100

typedef struct {
    pid_t pid;  // ID do processo
    int id;     // Identificador sequencial
    int status; // Status do processo (concluído ou em execução)
} BackgroundProcess;

/* Variaveis globais */
char *lastComando = NULL;
int numComandos = 0;
char *history = NULL; // Alocação dinâmica para histórico
char estilo[4] = "seq"; // Estilo default - sequencial
int numBackgroundProcesses = 0; // Contador de processos em segundo plano
/* Array para armazenar processos em background */
BackgroundProcess backgroundProcesses[MAX_BACKGROUND_PROCESSES]; 

/* Prototipos das funções */
void execCommandsProcess(char *comandos); /* Processos */
void removeExtraSpaces(char *str); /* Remover espaços em branco */
void execCommandsThreads(char *comandos); /* Threads */
void *executeCommand(void *command);  /* Threads */
void executeCommandWithPipe(char *command1, char *command2, int estilo);  /* Pipe */
void executeCommandBackground(const char *command, int inBackground); /* Background */
void bringToForeground(int id); /* Background */
void updateBackgroundProcessStatus(); /* Background */
void commandsExecuteRedirection(char *comandos, char *estilo); /* Redirecionamentos */

/* PROGRAMA PRINCIPAL */
int main(int argc, char *argv[]) {
  
    /* Valida a quantidade de argumentos */
    if (argc > 2) {
        fprintf(stderr, "Incorrect input. Invalid argument numbers!\n");
        exit(1);
    }

    char *commandRow = NULL; 
    size_t commandRowSize = 0;
    char *commandRow2 = NULL; 
    size_t commandRow2Size = 0;


    if (argc == 2) { /* EXECUÇÃO NO MODO BATCH */
        char *fileIn = argv[1];
        FILE *ftpr;

        ftpr = fopen(fileIn, "r");

        /* VALIDAR ARQUIVO */
        if (ftpr == NULL) { /* erro na abertura do arquivo */
            fprintf(stderr, "Error opening the file");
            exit(1);
        } else {
            /* Executar leitura arquivo enquanto
             * Finalização arquivo:
             * - Fim do arquivo (EOF)
             * - Comando exit
             * - Usuario teclar CTRL+D
             */
            char *lineFile = NULL;
            size_t lineSize = 0;
            ssize_t bytesRead;

            while (!feof(ftpr)) {
              
                bytesRead = getline(&lineFile, &lineSize, ftpr);

                /*  Verifica se o usuario teclou CTRL+D */
                if (feof(stdin)) {
                    free(lineFile);   // Liberar memória alocada
                    free(history);    // Liberar memória alocada
                    exit(0);
                }
                  
                if (bytesRead > 0 && lineFile[bytesRead - 1] == '\n') {
                    lineFile[bytesRead - 1] = '\0';
                }

                /* Verifica se o comando é exit() */
                if (strcmp(lineFile, "exit") == 0) {
                    fclose(ftpr);
                    exit(0);  
                }          
                
              // Imprimi e executa cada comando
              printf("alfr %s>%s\n", estilo, lineFile);
              execCommandsProcess(lineFile);

            }
            free(lineFile);
            fclose(ftpr);
        }
    } else { /* EXECUÇÃO NO MODO INTERATIVO */
        int should_run = 1;

        while (should_run) {
            int result;

            printf("alfr %s>", estilo);
            fflush(stdout);

            /* ler linha comando e saída se for EOF ou exit() */
            ssize_t bytesRead = getline(&commandRow, &commandRowSize, stdin);

            if (bytesRead == -1) {
                /*  Verifica se o usuario teclou CTRL+D */
                if (feof(stdin)) {
                    free(commandRow); // Liberar memória alocada
                    free(history);    // Liberar memória alocada
                    exit(0);
                } else {
                    fprintf(stderr, "Command reading failed");
                    continue;
                }
            } else {

                /* Comparar se o usuário teclou ENTER */
                result = strncmp(commandRow, "\n", 1);

                if (result != 0) { /* Diferente de ENTER */
                    /* CTRL + D ou digitar exit() encerrar programa */
                    if (feof(stdin) || (strcmp(commandRow, "exit\n") == 0)) {
                        /* Liberar memória alocada */
                        free(commandRow);
                        exit(0);
                    }

                    /* Verificar se o comando é "!!" */
                    if (strcmp(commandRow, "!!\n") == 0) {
                        if (history == NULL) {
                            printf("No commands\n");
                        } else {
                            execCommandsProcess(history);
                        }
                    } else {
                        /* Remover espaços em branco extras da linha de comando */
                        removeExtraSpaces(commandRow);

                        /* DAQUI!!! */
                        commandsExecuteRedirection(commandRow, estilo);
                    }
                }
            }
        }
    }
    //free(commandRow);
    //free(commandRow2);

    return 0;
}


/* Execução de comandos por Threads (estilo paralelo) */
void execCommandsThreads(char *comandos) {
    pthread_t threads[MAX_THREADS];
    long thread_count = 0;
    char *token;
    int rc;
  
    token = strtok(comandos, ";");
    

    while (token != NULL) {
        /* Remover espaços em branco extras */
        while (*token == ' ') {
            token++;
        }

        if (*token != '\0') {
            /* Alocar memória para armazenar uma cópia do comando */
            char *commandCopy = strdup(token);

            if (commandCopy == NULL) {
                fprintf(stderr, "Allocation error.\n");
                exit(-1);
            }

            /* Criar uma thread para executar o comando */
            rc = pthread_create(&threads[thread_count], NULL, executeCommand, (void *)token);
            if (rc) {
                printf("Erro - pthread_cread() return code %d\n", rc);
                exit(-1);
            }

            thread_count++;

            if (thread_count >= MAX_THREADS) {
                printf("Maximum number of threads reached.\n");
                exit(-1);
            }
        }

        token = strtok(NULL, ";");
    }

    /* Aguardar a finalização de todas as threads */
    for (long i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }
}

/* Função para executar um comando numa thread  */
void *executeCommand(void *command) {
    char *cmd = (char *)command;
    int result = system(cmd); // Executar o comando
  
    if (result == -1) {
        fprintf(stderr, "Error executing command: %s\n", cmd);
    } else if (WIFEXITED(result) && WEXITSTATUS(result) != 0) {
        fprintf(stderr, "Command failed: %s\n", cmd);
    }
  
    //free(cmd);
    pthread_exit(NULL);
}

/* Execução de comandos por Processo (estilo sequencial) */
void execCommandsProcess(char *comandos) {
    pid_t pid = fork(); /* criação processo filho */

    if (pid < 0) { 
        fprintf(stderr, "Fork failed");
        exit(1);
    }

    if (pid == 0) { // Processo filho
        execl("/bin/sh", "sh", "-c", comandos, NULL);
        fprintf(stderr, "Error executing the command %s\n", comandos);
    }

    /* aguarda a finalização do processo filho */
    int status;
    waitpid(pid, &status, 0); 

    /* Verificar se o comando foi executado com sucesso */
    if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
        fprintf(stderr, "Error executing command\n");
    }
}

/* Remoção espaços em branco antes ou depois do ponto-e-vírgula */
void removeExtraSpaces(char *comand) {
    int len = strlen(comand);
    int i, j;
    int space_flag = 0; // flag para verificar os espaços em branco

    /* Remover espaços em branco no início da linha de comando */
    while (comand[j] == ' ') {
        j++;
    }

    /* Remover espaços em branco antes ou depois do ponto e virgula */
    for (i = j; i < len; i++) {
        
      if (comand[i] == ';') {
            /* remover espaços em branco antes do ponto e virgula */
            while (i > j + 1 && comand[i - 1] == ' ') {
                i--;
            }
      } else if (comand[i] != ' ') {
          /* caractere não é um espaço em branco */
                space_flag = 0; // Reiniciar a flag quando um caractere não for espaço em branco
                comand[j++] = comand[i];
      } else {
          /* se o caractere for um espaço em branco */
          if (!space_flag) {
              comand[j++] = ' ';
              space_flag = 1;
          }
      }
    }
    
    /* Remover espaços em branco depois o ponto e virgula  */
    if (j > 0 && comand[j - 1] == ' ') {
        j--;
    }
      
    comand[j] = '\0';
}

/* Função Pipe para execução no estilo sequential ou paralelo */
void executeCommandWithPipe(char *command1, char *command2, int estilo) {
    int pipe_fd[2];
    pid_t child1, child2;

    /* Criar um pipe */
    if (pipe(pipe_fd) == -1) {
       fprintf(stderr, "Pipe creation failed");
        exit(1);
    }

    child1 = fork(); // 1° processo filho

    if (child1 == -1) {
        fprintf(stderr, "Fork failed");
        exit(1);
    }

    if (child1 == 0) {  
        close(pipe_fd[0]); 
        dup2(pipe_fd[1], STDOUT_FILENO); 
        close(pipe_fd[1]); 

        execl("/bin/sh", "sh", "-c", command1, NULL);
        fprintf(stderr, "Error executing command1");
        exit(1);
    }

    child2 = fork(); // 2° processo filho 

    if (child2 == -1) {
        perror("Fork failed");
        exit(1);
    }

    if (child2 == 0) {
        close(pipe_fd[1]); 
        dup2(pipe_fd[0], STDIN_FILENO); 
        close(pipe_fd[0]);

        execl("/bin/sh", "sh", "-c", command2, NULL);
        fprintf(stderr, "Error executing command2");
        exit(1);
    }

    close(pipe_fd[0]);
    close(pipe_fd[1]);

    int status1, status2;
    waitpid(child1, &status1, 0);
    waitpid(child2, &status1, 0);


    if (WIFEXITED(status1) && WEXITSTATUS(status1) != 0) {
        fprintf(stderr, "Error executing command1: %s\n", command1);
    }

    if (WIFEXITED(status2) && WEXITSTATUS(status2) != 0) {
        fprintf(stderr, "Error executing command2: %s\n", command2);
    }
}

void executeCommandBackground(const char *command, int inBackground) {
    pid_t pid = fork();
    
    if (pid == -1) {
        fprintf(stderr, "Fork failed\n");
        exit(1);
    } else if (pid == 0) { // Processo filho
        /* Executar o comando no processo filho */
        if (system(command) == -1) {
            fprintf(stderr, "Error executing command: %s\n", command);
            exit(1);
        }
        exit(0);
    } else { // Processo pai
        if (!inBackground) {
            /* Aguardar o processo filho terminar, a menos que seja um comando em segundo plano */
            waitpid(pid, NULL, 0);
        } else {
            /* Adicionar o processo em segundo plano à lista */
            backgroundProcesses[numBackgroundProcesses].pid = pid;
            backgroundProcesses[numBackgroundProcesses].id = numBackgroundProcesses + 1;
            backgroundProcesses[numBackgroundProcesses].status = 0; 
            numBackgroundProcesses++;

            /* Imprimir informações sobre o processo em segundo plano */
            printf("[%d] %d\n", numBackgroundProcesses, pid);
        }
    }
}

void bringToForeground(int id) {
    updateBackgroundProcessStatus();
  
    if (id > 0 && id <= numBackgroundProcesses) {
        if (backgroundProcesses[id - 1].status == 0) {
            /* Processo ainda em execução, trazer para o 1° plano */
            pid_t pid = backgroundProcesses[id - 1].pid;

            /* Esperar pelo processo em 2° plano */
            waitpid(pid, NULL, 0);

            /* Atualizar o status do processo em 2° plano */
            backgroundProcesses[id - 1].status = 1; // Concluído

            /* Remover o processo da lista de processos em 2° plano */
            for (int i = id - 1; i < numBackgroundProcesses - 1; i++) {
                backgroundProcesses[i] = backgroundProcesses[i + 1];
            }
            numBackgroundProcesses--;
        }
      
    } else {
        fprintf(stderr, "Invalid background process ID\n");
    }
}

/* Atualizar o status dos processos em background */
void updateBackgroundProcessStatus() {
    for (int i = 0; i < numBackgroundProcesses; i++) {
        int status;
        pid_t result = waitpid(backgroundProcesses[i].pid, &status, WNOHANG);

        if (result > 0) {
            // Processo concluído
            backgroundProcesses[i].status = 1; // Atualizar o status como concluído
        }
    }
}


  /* Redirecionamento de execução por operação */
void commandsExecuteRedirection(char *comandos, char *estilo) {
    char *pipeSymbol = strchr(comandos, '|');
    char *inSymbol = strchr(comandos, '<');
    char *outSymbol = strchr(comandos, '>');
    char *appendSymbol = strstr(comandos, ">>");
    char *backgroundSymbol = strchr(comandos, '&');

    char *token2;
    char *commands[MAX_THREADS];
    int numCommands = 0;

    if (pipeSymbol) {  /* PIPE */
        token2 = strtok(comandos, "|"); 
        while (token2 != NULL) {
            removeExtraSpaces(token2); 
            commands[numCommands++] = strdup(token2);
            token2 = strtok(NULL, "|"); 
        }
        if (strcmp(estilo, "seq") == 0){
            /* Estilo Sequential */   
            //printf("Seq:%s %s\n ", commands[0], commands[1]);
            executeCommandWithPipe(commands[0], commands[1], 0);
        } else {
          /* Estilo Paralelo */ 
          executeCommandWithPipe(commands[0], commands[1], 1); 
        }
    } else if (inSymbol) {   /* REDIRECIONAMENTO */
        token2 = strtok(comandos, "<"); 
        while (token2 != NULL) {
            removeExtraSpaces(token2); 
            commands[numCommands++] = strdup(token2);
            token2 = strtok(NULL, ">>"); 
        }

        if (numCommands == 2) {
            removeExtraSpaces(commands[0]);
            removeExtraSpaces(commands[1]);

            int status;
            pid_t pid = fork(); 
    
            if (pid == -1) {
                fprintf(stderr,"Fork failed");
                exit(1);
            } else if (pid == 0) { // Processo filho
                /* Abre o arquivo de entrada no modo leitura */
                FILE *inputFile = fopen(commands[1], "r");
                if (inputFile == NULL) {
                    fprintf(stderr,"Error opening the input file.");
                    exit(1);
                }

                /* Direciona a entrada padrão (stdin) para o arquivo*/ 
                fflush(stdin);
                int originalStdin = dup(fileno(stdin));
                dup2(fileno(inputFile), fileno(stdin));
    
                executeCommand(commands[0]);

                /* Restaura a entrada padrão e fecha o arquivo */
                dup2(originalStdin, fileno(stdin));
                fclose(inputFile);
                exit(0);
            } else {
                // Processo pai
                waitpid(pid, &status, 0);
                if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                    fprintf(stderr, "Command failed.\n");
                }
                int status;
                pid_t pid = fork(); 
    
                if (pid == -1) {
                    fprintf(stderr,"Fork failed");
                    exit(1);
                } else if (pid == 0) { // Processo filho
                    /* Abre o arquivo de entrada no modo leitura */
                    FILE *inputFile = fopen(commands[1], "r");
                    if (inputFile == NULL) {
                        fprintf(stderr,"Error opening the input file.");
                        exit(1);
                }

                /* Direciona a entrada padrão (stdin) para o arquivo*/ 
                fflush(stdin);
                int originalStdin = dup(fileno(stdin));
                dup2(fileno(inputFile), fileno(stdin));
    
                executeCommand(commands[0]);

                /* Restaura a entrada padrão e fecha o arquivo */
                dup2(originalStdin, fileno(stdin));
                fclose(inputFile);
                exit(0);
            } else {    // Processo pai
                waitpid(pid, &status, 0);
                if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                    fprintf(stderr, "Command failed.\n");
                }
            }
            }
        } else {
            executeCommand(comandos);
       }
    } else if (outSymbol) { /* REDIRECIONAMENTO */
        token2 = strtok(comandos, ">"); 
        while (token2 != NULL) {
            removeExtraSpaces(token2); 
            commands[numCommands++] = strdup(token2);
            token2 = strtok(NULL, ">"); 
        }
        if (numCommands == 2) {
            /* Remover espaços extras nos comandos */
            removeExtraSpaces(commands[0]);
            removeExtraSpaces(commands[1]);
            
            FILE *outputFile = fopen(commands[1], "w");
            if (outputFile == NULL) {
                fprintf(stderr, "Error opening the output file");
                exit(1);
            } 

            /* Redireciona a saída do comando 1 para o arquivo de saída */
            fflush(stdout);
            int originalStdout = dup(fileno(stdout));
            dup2(fileno(outputFile), fileno(stdout));
            
            executeCommand(commands[0]);
            
            fflush(stdout);
            dup2(originalStdout, fileno(stdout));
            fclose(outputFile);
        } else { // só há um comando em commandsRow
            executeCommand(comandos);
        }
    } else if (appendSymbol) {  /* REDIRECIONAMENTO */
        /* apos executar o comando, o programa esta sendo interrompido */
        token2 = strtok(comandos, ">>"); 
        while (token2 != NULL) {
            removeExtraSpaces(token2); 
            commands[numCommands++] = strdup(token2);
            token2 = strtok(NULL, ">>"); 
        }

        if (numCommands == 2) {        
            removeExtraSpaces(commands[0]);
            removeExtraSpaces(commands[1]);

            /* Abri o arquivo de entrada para leitura */
            FILE *inputFile = fopen(commands[1], "r");
            if (inputFile == NULL) {
                fprintf(stderr, "Error opening the input file.\n");
                return;
            }

            /* Direcionar a entrada padrão para o arquivo */
            if (dup2(fileno(inputFile), STDIN_FILENO) == -1) {
                fprintf(stderr, "Error redirecting input.\n");
                fclose(inputFile);
                return;
            }

            fclose(inputFile);

            /* Direcionamento do conteudo do arquivo como entrada para o programa */
            char commandInFile[1024];
            snprintf(commandInFile, sizeof(commandInFile), "%s", commands[0]);
        } else {
            executeCommand(comandos);
        }

    } else if (backgroundSymbol) { /* BACKGROUND */
      
       *backgroundSymbol = '\0'; // Remove o caractere '&' da linha de comando
        removeExtraSpaces(comandos); 
    
        pid_t pid = fork();
    
        if (pid == -1) {
            fprintf(stderr, "Fork failed\n");
            exit(1);
        } else if (pid == 0) { // Processo filho
            /* Executer o comando em segundo plano */
            executeCommandBackground(comandos, 1); // O segundo argumento 1 indica execução em segundo plano
            exit(0); // Encerra o processo filho
        } else { // Processo pai
            /* Adicionar o processo em segundo plano à lista */
            backgroundProcesses[numBackgroundProcesses].pid = pid;
            backgroundProcesses[numBackgroundProcesses].id = numBackgroundProcesses + 1;
            numBackgroundProcesses++;
    
            /* Imprimir a informação sobre o processo em segundo plano */
            printf("[%d] %d\n", numBackgroundProcesses, pid);
        }   
      
    } else { /* Colocar codigo do sequencial e paralello aqui */ 
        if (strncmp(comandos, "fg ", 3) == 0) { // trazer o processo do background
            char *idStr = comandos + 3; // Obter o número do processo
        
            if (*idStr != '\0') { 
                int id = atoi(idStr);
                bringToForeground(id);
            } else {
                fprintf(stderr, "Usage: fg <process_id>\n");
            }
        } else {
    
            char *commandRow2 = (char *)malloc(strlen(comandos) + 1);
            if (commandRow2 == NULL) {
                fprintf(stderr, "Allocation error.\n");
                exit(1);
            }
            strcpy(commandRow2, comandos);
    
           if (strncmp(comandos, "fg ", 3) == 0) { // trazer o processo do background
              char *idStr = strtok(NULL, " \t\n");
              if (idStr != NULL) {
                  int id = atoi(idStr);
                  bringToForeground(id);
              } else {
                  fprintf(stderr, "Usage: fg <process_id>\n");
              }
            }
        
            /* Separar a linha de comando em comandos individuais */
            char *token = strtok(comandos, ";");
    
            while (token != NULL) {
                while (*token == ' ') {
                    token++;
                }
    
                if (*token != '\0') {
                    if (strcmp(token, "style sequential\n") == 0) {
                        strcpy(estilo, "seq");
                        history = NULL;
                    } else if (strcmp(token, "style parallel\n") == 0) {
                        strcpy(estilo, "par");
                        history = NULL;
                    } else {
                        if (strcmp(estilo, "seq") == 0) {
                            execCommandsProcess(token);
                        } else if (strcmp(estilo, "par") == 0) {
                            execCommandsThreads(commandRow2);
                        }
                        lastComando = token;
    
                        /* Atualizar o histórico com o último comando executado */
                        history = strdup(lastComando); 
                    }
                }
    
                token = strtok(NULL, ";");
            }
            free(commandRow2);
    
        }
    }
}

