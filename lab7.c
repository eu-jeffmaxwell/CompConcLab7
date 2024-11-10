#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>

#define N 1000 
#define NTHREAD 3

sem_t slotVazio, cheio, mutex1, mutex2; // Semáforos para controle do buffer
FILE *arq;
char buffer1[N];
char buffer2[N];
int eof_reached = 0; // Sinalizador para indicar que o fim do arquivo foi alcançado

void *THREAD1(void *arg) {
    while (1) {
        sem_wait(&slotVazio);
        sem_wait(&mutex1);

        size_t lidos = fread(buffer1, 1, N, arq); // le N characteres no buffer1
        if (lidos < N) { 
            eof_reached = 1; 
        }
        
        sem_post(&cheio); // Os dados do sinal estão prontos no buffer1
        sem_post(&mutex1); // Libera mutex para acesso buffer1

        if (eof_reached) break; // Sai se o fim do arquivo for alcançado
    }
    pthread_exit(NULL);
}

void *THREAD2(void *arg) {
    int n = 0;
    int contador = 0;
    int pos2 = 0;

    while (1) {
        sem_wait(&cheio); // Semaforo bloqueante: Espera pelo buffer1
        sem_wait(&mutex1); // Lock do buffer1

        pos2 = 0; // Redefine a posição do buffer2 para cada bloco de processamento
        contador = 0;

        // Processa os caracteres no buffer1 e adiciona ao buffer2 com quebras de linha
        for (int i = 0; i < N && i < strlen(buffer1); i++) {
            buffer2[pos2++] = buffer1[i];
            contador++;

            // Insire uma nova linha com base no padrão atual
            if ((n < 10 && contador == 2 * n + 1) || (n >= 10 && contador == 10)) {
                buffer2[pos2++] = '\n';
                contador = 0;
                n++;
            }
        }
        sem_post(&mutex2); // Sinaliza ao consumidor que o buffer2 está pronto
        sem_post(&slotVazio); // Permite que o produtor preencha o buffer1 novamente

        if (eof_reached) break; // Sai se EOF e buffer2 estiverem totalmente processados
    }
    pthread_exit(NULL);
}

void *THREAD3(void *arg) {
    while (1) {
        sem_wait(&mutex2); // Semaforo bloqueante: Espera pelo buffer2

        // Printa o  conteúdo do buffer2
        for (int i = 0; i < N; i++) {
            if (buffer2[i] == '\0') break; // Para quando o buffer2 chega ao fim
            putchar(buffer2[i]);
        }

        if (eof_reached) {
            pthread_exit(NULL); 
        }
        sem_post(&mutex1); 
    }
}


int main(int argc, char *argv[]) {

    if (argc < 2) {printf("Usage: %s <input file>\n", argv[0]);return 1;}

    arq = fopen(argv[1], "rb");
    if (arq == NULL) {printf("Error opening file\n");return 1;}

    //Inicialização dos Semaforos
    pthread_t threads[NTHREAD];
    sem_init(&slotVazio, 0, 1);
    sem_init(&cheio, 0, 0);
    sem_init(&mutex1, 0, 1); 
    sem_init(&mutex2, 0, 0);

    //Inicialição das Funções
    pthread_create(&threads[0], NULL, THREAD1, NULL);
    pthread_create(&threads[1], NULL, THREAD2, NULL);
    pthread_create(&threads[2], NULL, THREAD3, NULL);

    //Aguarda todas as Threads terminarem 
    for (int i = 0; i < NTHREAD; i++) {
        if (pthread_join(threads[i], NULL)) {
            printf(" -- ERROR: pthread_join\n");
            return 2;
        }
    }

    //Libera espaços antes alocados
    fclose(arq);
    sem_destroy(&slotVazio);
    sem_destroy(&cheio);
    sem_destroy(&mutex1);
    sem_destroy(&mutex2);

    return 0;
}
