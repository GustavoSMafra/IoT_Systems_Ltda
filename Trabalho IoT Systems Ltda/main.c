/*
    Acadêmicos: Gustavo da Silva Mafra
                Nathália Adriana de Oliveira
    Curso: Engenharia de Computação
    Disciplina: Sistemas em Tempo Real

    Trabalho - Sincronização com Mutex, Semáforo e Monitor

    Compilar usando: gcc -o main sensor.h main.c -lpthread
*/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#define qntAndares 3
#define qntApartamentos 2

// Mutex e condições para cada variável 
static pthread_mutex_t mtx_agua = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mtx_elet = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mtx_gas = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mtx_read = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond_read = PTHREAD_COND_INITIALIZER;
static pthread_cond_t cond_agua_buff = PTHREAD_COND_INITIALIZER;
static pthread_cond_t cond_agua_media = PTHREAD_COND_INITIALIZER;
static pthread_cond_t cond_elet_buff = PTHREAD_COND_INITIALIZER;
static pthread_cond_t cond_elet_media = PTHREAD_COND_INITIALIZER;
static pthread_cond_t cond_gas_buff = PTHREAD_COND_INITIALIZER;
static pthread_cond_t cond_gas_media = PTHREAD_COND_INITIALIZER;

// Buffers de cada elemento e sua respectiva média
static double agua_buff[qntAndares][qntApartamentos] = {};
static double elet_buff[qntAndares][qntApartamentos] = {};
static double gas_buff[qntAndares][qntApartamentos] = {};

static double agua_soma[qntAndares] = {};
static double elet_soma[qntAndares] = {};
static double gas_soma[qntAndares] = {};

static double agua_media[qntAndares] = {};
static double elet_media[qntAndares] = {};
static double gas_media[qntAndares] = {};

// Contadores
static int agua_count_input[qntAndares] = {};
static int elet_count_input[qntAndares] = {};
static int gas_count_input[qntAndares] = {};

// Flags 
static int agua_flag_media[qntAndares] = {};
static int elet_flag_media[qntAndares] = {};
static int gas_flag_media[qntAndares] = {};

//Mutex com teto de proridade
pthread_mutexattr_t  mutexattr_prioceiling;
int high_prio;
void init_mutex_inh();

//Valores do gás, eletrcidade e água necessários
static float valor_agua[qntAndares] = {};
static float valor_elet[qntAndares] = {};
static float valor_gas[qntAndares] = {};

// Monitor
void sensor_input(int type){
    int rtn = 0;
    int loop = 0;
    while(1){
        switch(type){
            case 00: // Água
                usleep((500)*1000);
                if((rtn = pthread_mutex_lock(&mtx_agua)) != 0){
                    fprintf(stderr,"\npthread_mutex_lock: %s",strerror(rtn));
                } else {
                    for(int i = 0; i < qntAndares; i++){
                        for(int j = 0; j < qntApartamentos; j++){
                            agua_buff[i][j] = rand()%10;
                            agua_soma[i] += agua_buff[i][j];
                            agua_count_input[i]+= 1;
                            if(agua_count_input[i] >= 50){
                                agua_flag_media[i] = 1;
                                pthread_cond_signal(&cond_agua_media);
                            }
                            //printf("\nApartamento [%d][%d] = Água => %.2f", i,j,agua_buff[i][j]);
                        }
                        //printf("\nAndar %d -> quantidade água %d", i, agua_count_input[i]);
                    }
                    pthread_mutex_unlock(&mtx_agua);
                }
                break;

            case 01: // Eletricidade
                usleep((100)*1000);
                if((rtn = pthread_mutex_lock(&mtx_elet)) != 0){
                    fprintf(stderr,"\npthread_mutex_lock: %s",strerror(rtn));
                } else {
                    for(int i = 0; i < qntAndares; i++){
                        for(int j = 0; j < qntApartamentos; j++){
                            elet_buff[i][j] = rand()%50;
                            elet_soma[i] += elet_buff[i][j];
                            elet_count_input[i]++;
                            if(elet_count_input[i] >= 50){
                                elet_flag_media[i] = 1;
                                pthread_cond_signal(&cond_elet_media);
                            }
                            //printf("\nApartamento [%d][%d] = Elet => %.2f", i,j,elet_buff[i][j]);
                        }
                        //printf("\nAndar %d -> quantidade elet %d", i, elet_count_input[i]);
                    }
                    pthread_mutex_unlock(&mtx_elet);
                }
            break;
            case 02: // Gás
                usleep((1000)*1000);
                if((rtn = pthread_mutex_lock(&mtx_gas)) != 0){
                    fprintf(stderr,"\npthread_mutex_lock: %s",strerror(rtn));
                } else {
                    for(int i = 0; i < qntAndares; i++){
                        for(int j = 0; j < qntApartamentos; j++){
                            gas_buff[i][j] = rand()%5;
                            gas_soma[i] += gas_buff[i][j];
                            gas_count_input[i]++;
                            if(gas_count_input[i] >= 50){
                                gas_flag_media[i] = 1;
                                pthread_cond_signal(&cond_gas_media);
                            }
                            //printf("\nApartamento [%d][%d] = Gás => %.2f", i,j,gas_buff[i][j]);
                        }
                        //printf("\nAndar %d -> quantidade gás %d", i, gas_count_input[i]);
                    }
                    pthread_mutex_unlock(&mtx_gas);
                }
            break;
        }
        loop++;
    }
}

void sensor_media(int type){
    int rtn = 0;
    while(1){
        switch(type){
            case 00:
                if((rtn = pthread_mutex_lock(&mtx_agua)) != 0){
                    fprintf(stderr,"\npthread_mutex_lock: %s",strerror(rtn));
                } else {
                    for(int i = 0; i < qntAndares; i++){
                        while(agua_flag_media[i] != 1){
                            pthread_cond_wait(&cond_agua_media, &mtx_agua);
                        }
                        agua_media[i] = agua_soma[i]/50;
                        agua_soma[i] = 0;
                        agua_count_input[i] = 0;
                        agua_flag_media[i] = 0;
                        //printf("\nA média de água no andar %d é igual a %2.f", i, agua_media[i]);
                    }
                    pthread_mutex_unlock(&mtx_agua);
                }
            break;
            case 01:
                if((rtn = pthread_mutex_lock(&mtx_elet)) != 0){
                    fprintf(stderr,"\npthread_mutex_lock: %s",strerror(rtn));
                } else {
                    for(int i = 0; i < qntAndares; i++){
                        while(elet_flag_media[i] != 1)
                            pthread_cond_wait(&cond_elet_media, &mtx_elet);
                        elet_media[i] = elet_soma[i]/50;
                        elet_soma[i] = 0;
                        elet_count_input[i] = 0;
                        elet_flag_media[i] = 0;
                        //printf("\nA média de eletricidade no andar %d é igual a %2.f", i, elet_media[i]);
                    }
                    pthread_mutex_unlock(&mtx_elet);
                }
            break;
            case 02:
                if((rtn = pthread_mutex_lock(&mtx_gas)) != 0){
                    fprintf(stderr,"\npthread_mutex_lock: %s",strerror(rtn));
                } else {
                    for(int i = 0; i < qntAndares; i++){
                        while(gas_flag_media[i] != 1)
                            pthread_cond_wait(&cond_gas_media, &mtx_gas);
                        gas_media[i] = gas_soma[i]/50;
                        gas_soma[i] = 0;
                        gas_count_input[i] = 0;
                        gas_flag_media[i] = 0;
                        //printf("\nA média de gás no andar %d é igual a %2.f", i, gas_media[i]);
                    }
                    pthread_mutex_unlock(&mtx_gas);
                }
            break;
        }
    }
}


void Valores_utilizados(){
    for(int i = 0; i<qntAndares; i++){
        valor_agua[i] = 0;
        valor_elet[i] = 0;
        valor_gas[i] = 0;
    }
    for(int i = 0; i<qntAndares; i++){
        for(int j = 0; j<qntApartamentos; j++){
            valor_agua[i] += agua_buff[i][j];
            valor_elet[i] += elet_buff[i][j];
            valor_gas[i] += gas_buff[i][j];
        }
    }
}

void sensor_read(){
    int rtn = 0;
    while(1){
        if((rtn = pthread_mutex_lock(&mtx_read)) != 0){
            fprintf(stderr,"\npthread_mutex_lock: %s",strerror(rtn));
        } else {
            int tecla = 0;
            tecla = getchar();
            if(tecla != 0){
                system("clear");
                Valores_utilizados();
                printf("\n\nGASTOS MEDIANOS POR ANDAR: \n");
                for(int i = 0; i < qntAndares; i++){
                    printf("\n\nA média de água no andar %d é igual a %.2f m3", i+1, agua_media[i]);
                    printf("\nA média de eletricidade no andar %d é igual a %.2f kWh", i+1, elet_media[i]);
                    printf("\nA média de gás no andar %d é igual a %.2f Kg", i+1, gas_media[i]);
                }
                printf("\n\nGASTOS POR APARTAMENTO (ULTIMA MEDIÇÃO): \n");
                int aux = 1;
                for(int i = 0; i < qntAndares; i++){
                    for(int j = 0; j < qntApartamentos; j++){
                        printf("\nO apartamento %d, tem um consumo atual de água: %.2f m3, energia: %.2f kWh e gás: %.2f Kg", aux+j, agua_buff[i][j], elet_buff[i][j], gas_buff[i][j]);
                    }
                    aux += 2;
                }
                printf("\n\nVALORES DISPONIBILIZADOS POR ANDAR BASEADOS NA MÉDIA: \n");
                for(int i = 0; i < qntAndares; i++){
                    printf("\nÁgua no andar %d é igual a %.2f m3", i+1, valor_agua[i]);
                    printf("\nEletricidade no andar %d é igual a %.2f kWh", i+1, valor_elet[i]);
                    printf("\nGás no andar %d é igual a %.2f Kg", i+1, valor_gas[i]);
                }
            }
            pthread_mutex_unlock(&mtx_read);
        }
    }   
}

// Fim do monitor

int main(){
    srand(time(NULL));
    pthread_t input_agua, input_elet, input_gas;
    pthread_t media_agua, media_elet, media_gas;
    pthread_t read;
    pthread_attr_t tattr;
    struct sched_param param;

    pthread_mutex_init(&mtx_agua, NULL);
    pthread_mutex_init(&mtx_elet, NULL);
    pthread_mutex_init(&mtx_gas, NULL);
    pthread_mutex_init(&mtx_read, NULL);
    init_mutex_inh();
    pthread_attr_init (&tattr);
    pthread_attr_getschedparam (&tattr, &param);

    param.sched_priority = 10;
    pthread_attr_setschedparam (&tattr, &param);
    pthread_create(&read, &tattr, (void *)sensor_read, NULL);

    param.sched_priority = 5;
    pthread_attr_setschedparam (&tattr, &param);
    pthread_create(&media_agua, &tattr, (void *)sensor_media, (void*)0);
    pthread_create(&media_elet, &tattr, (void *)sensor_media, (void*)1);
    pthread_create(&media_gas, &tattr, (void *)sensor_media, (void*)2);

    param.sched_priority = 3;
    pthread_attr_setschedparam (&tattr, &param);
    pthread_create(&input_agua, &tattr, (void *)sensor_input, (void*)0);
    pthread_create(&input_elet, &tattr, (void *)sensor_input, (void*)1);
    pthread_create(&input_gas, &tattr, (void *)sensor_input, (void*)2);

    pthread_join(input_agua, NULL);
    pthread_join(input_elet, NULL);
    pthread_join(input_gas, NULL);

    pthread_join(media_agua, NULL);
    pthread_join(media_elet, NULL);
    pthread_join(media_gas, NULL);

    pthread_join(read, NULL);

    printf("\n\n\n");
    return 0;
}

// função para inicializar o mutex com o protocolo de herança de prioridade
void init_mutex_inh(){
   int rtn;
   int mutex_protocol;

   if ((rtn = pthread_mutexattr_init(&mutexattr_prioceiling)) != 0)
      fprintf(stderr,"pthread_mutexattr_init: %s",strerror(rtn)); //mensagem em caso de haver erro
   if ((rtn = pthread_mutexattr_getprotocol(&mutexattr_prioceiling, &mutex_protocol)) != 0)
      fprintf(stderr,"pthread_mutexattr_getprotocol: %s",strerror(rtn)); //mensagem em caso de haver erro
   
   /* pega a prioridade atual */
   high_prio = sched_get_priority_max(SCHED_FIFO);

   /* Define o protocolo do mutex como PROTECT - teto de prioridade*/  
   if ((rtn = pthread_mutexattr_setprotocol(&mutexattr_prioceiling, PTHREAD_PRIO_PROTECT)) != 0)
    fprintf(stderr,"pthread_mutexattr_setprotocol: %s",strerror(rtn)); //mensagem em caso de haver erro

   /* Define o o teto de prioridade inicial*/
   if ((rtn = pthread_mutexattr_setprioceiling(&mutexattr_prioceiling, high_prio)) != 0)
      fprintf(stderr,"pthread_mutexattr_setprotocol: %s",strerror(rtn)); //mensagem em caso de haver erro

   /* Inicialize mutex com o objeto de atributo */
   if ((rtn = pthread_mutex_init(&mtx_agua, &mutexattr_prioceiling)) != 0)
      fprintf(stderr,"pthread_mutexattr_init: %s",strerror(rtn)); //mensagem em caso de haver erro
       /* Inicialize mutex com o objeto de atributo */
   if ((rtn = pthread_mutex_init(&mtx_elet, &mutexattr_prioceiling)) != 0)
      fprintf(stderr,"pthread_mutexattr_init: %s",strerror(rtn)); //mensagem em caso de haver erro
         /* Inicialize mutex com o objeto de atributo */
   if ((rtn = pthread_mutex_init(&mtx_gas, &mutexattr_prioceiling)) != 0)
      fprintf(stderr,"pthread_mutexattr_init: %s",strerror(rtn)); //mensagem em caso de haver erro
         /* Inicialize mutex com o objeto de atributo */
   if ((rtn = pthread_mutex_init(&mtx_read, &mutexattr_prioceiling)) != 0)
      fprintf(stderr,"pthread_mutexattr_init: %s",strerror(rtn)); //mensagem em caso de haver erro
}