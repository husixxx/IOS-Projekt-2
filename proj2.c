#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <time.h>
#include <string.h>
#include <ctype.h>

int* closer;

int* waiting;
int* action;
int* frontas;
int* nu2;
FILE *file;



sem_t* sem_fronta[3];
sem_t* sem_officer;

sem_t* sem_log;
sem_t* sem_customer;
sem_t* sem_main;
typedef struct parametrs {
    int NZ,NU,TZ,TU,F;
}   params;
int action_number(){
    return (*action)++;
}
int get_params(int argc, char *argv[], params *parametrs){
    int error = 0;
    char *x = NULL;
    if (argc == 6){
        if(isdigit(*argv[1]) && (*argv[1]) > 0){
            parametrs->NZ = strtoul(argv[1], &x, 10);
        }else{
            error = 1;
        }
        if(isdigit(*argv[2]) && (*argv[2]) > 0){
            parametrs->NU = strtoul(argv[2], &x, 10);
        }else{
            error = 1;
        }
        if(isdigit(*argv[3]) && (*argv[3]) > 0){
            parametrs->TZ = strtoul(argv[3], &x, 10);
        }else{
            error = 1;
        }
        if(isdigit(*argv[4]) && (*argv[4]) > 0){
            parametrs->TU = strtoul(argv[4], &x, 10);
        }else{
            error = 1;
        }
        if(isdigit(*argv[5])){
            parametrs->F = strtoul(argv[5], &x, 10);
        }else{
            error = 1;
        }
    }else{
        error = 1;
    }
    if (error == 1){
        fprintf(stderr,"Bad arguments! Program will exit \n");
        return 1;
        
        
    }
    return 0;
}



void officer(int nz,int tu){

    srand(getpid() * time(NULL)); // generating random numbers
    int rozsah = 0;
    rozsah = (random() % (10) + 1);
   
    
    sem_wait(sem_log);
    fprintf(file,"%d: U %d: started\n",action_number(),nz);
    fflush(file);
    sem_post(sem_log);

    while( 1 ){ 

        if ( (*closer) == 1 && (*waiting) == 0){ // check if its not closed

            sem_wait(sem_log);
            fprintf(file,"%d: U %d: going home\n",action_number(),nz);
            fflush(file);
            sem_post(sem_log);
            exit(0);

        }
        if( (*waiting) == 0 ){ // if office is empty take break
            sem_wait(sem_log);
            fprintf(file,"%d: U %d: taking break\n",action_number(), nz);
            fflush(file);
            sem_post(sem_log);
            usleep((rand()%(tu + 1)) * 1000);
            sem_wait(sem_log);
            fprintf(file,"%d: U %d: break finished\n",action_number(), nz);
            fflush(file);
            sem_post(sem_log);

        }else{
        
        
        for(int i = 0; i < 3; i++){  
            
            if ((frontas[i]) > 0){               // pick first non empty queue
                frontas[i]--;
                (*waiting)--;   
                sem_post(sem_fronta[i]);  // signals being ready to serve the queue
                sem_wait(sem_customer); // wait for customers to arrive in queue
                
                sem_wait(sem_log);
                fprintf(file,"%d: U %d: serving a service of type %d\n",action_number(),nz,(i+1)); // someone is waiting, serve him
                fflush(file);
                sem_post(sem_log);

                sem_post(sem_officer); // call customer in
                sem_wait(sem_fronta[i]); // lock the queue so customers are not overtaking

                usleep(rozsah * 1000); // Serve service 
                sem_wait(sem_log);
                fprintf(file,"%d: U %d: service finished\n",action_number(),nz);
                fflush(file);
                sem_post(sem_log);
                break;
            
            }
        }    
        }
    }
    
    
    exit(0);
}
void customer(int tz){
    

    srand(getpid() * time(NULL));
    int rozsah = (rand()%(tz + 1));
    sem_wait(sem_log);
    int nu = *nu2;
    (*nu2)++;
    sem_post(sem_log);
    sem_wait(sem_log);
    fprintf(file,"%d: Z %d: started\n",action_number(), nu);
    fflush(file);
   
   
    sem_post(sem_log);
    usleep(rozsah * 1000); // New customer arrive

 
   
    
    

    if( (*closer) == 1 ){  // going home if closed
        sem_wait(sem_log);
        fprintf(file,"%d: Z %d: going home\n",action_number(),nu);
        fflush(file);
        sem_post(sem_log);
        exit(0);
    }else{ 

    int service_type = rand() % 3 + 1; // pick random service and enter office
    (*waiting)++;
    frontas[service_type-1]++;
    sem_wait(sem_log);
    fprintf(file,"%d: Z %d: entering office for a service %d\n",action_number(), nu, service_type);
    fflush(file);
    sem_post(sem_log);
    
    sem_wait(sem_fronta[service_type-1]);  // customer waits in the queue
    sem_post(sem_customer); // signals for the barber that someone is waiting
    sem_post(sem_fronta[service_type-1]); // release queue for others


    
    sem_wait(sem_officer); // wait for call by officer

    sem_wait(sem_log);
    fprintf(file,"%d: Z %d: called by office worker\n",action_number(), nu);
    fflush(file);
    sem_post(sem_log);

    
 
    usleep((rand() % 10 + 1)*1000); // make bussiness

    sem_wait(sem_log);
    fprintf(file,"%d: Z %d: going home\n",action_number(),nu);
    fflush(file);
    sem_post(sem_log);

    exit(0);
    }
}

void print_closing(int f){

    srand(getpid() * time(NULL));
   
    
    usleep((rand() % ((f/2)+1) + (f/2)) * 1000);

    sem_wait(sem_main);
    (*closer) = 1;
   
    

    sem_wait(sem_log);
    fprintf(file,"%d: closing\n",action_number());
    fflush(file);
    sem_post(sem_log);
    sem_post(sem_main);
    exit(0);




}
void destroy_sem(){

    
    if(sem_close(sem_fronta[0]) == -1){fprintf(stderr, "Bad allocation error.");}
    if(sem_close(sem_fronta[1]) == -1){fprintf(stderr, "Bad allocation error.");}
    if(sem_close(sem_fronta[2]) == -1){fprintf(stderr, "Bad allocation error.");}
    if(sem_close(sem_officer) == -1){fprintf(stderr, "Bad allocation error.");}
    if(sem_close(sem_main) == -1){fprintf(stderr, "Bad allocation error.");}
    if(sem_close(sem_customer) == -1){fprintf(stderr, "Bad allocation error.");}
    
    if(sem_close(sem_log) == -1){fprintf(stderr, "Bad allocation error.");}
    sem_unlink("sem_customer");
    sem_unlink("sem_main");
    sem_unlink("sem_fronta");
    sem_unlink("sem_fronta1");
    sem_unlink("sem_fronta2");
    sem_unlink("sem_officer");
 
    sem_unlink("sem_log");


}
void destroy_memory(){
    munmap(waiting, sizeof(*waiting));
    munmap(nu2, sizeof(*waiting));
    munmap(action, sizeof(*action));
    munmap(closer, sizeof(*closer));
    munmap(frontas, 3 * sizeof(*frontas));
}
void load(){
    nu2 = mmap(NULL, sizeof(*nu2), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    waiting = mmap(NULL, sizeof(*waiting), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    action = mmap(NULL, sizeof(*action), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    closer = mmap(NULL, sizeof(*closer), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    frontas = mmap(NULL, 3 * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    frontas[0] = 0;
    frontas[1] = 0;
    frontas[2] = 0;
    (*nu2) = 1;
    (*waiting) = 0;
    (*action) = 1;
    (*closer) = 0;
   
    sem_customer = sem_open("sem_customer", O_CREAT, 0660, 0);
    sem_officer = sem_open("sem_officer", O_CREAT, 0660, 0);
    sem_main = sem_open("sem_main", O_CREAT, 0660, 1);
    sem_fronta[0] = sem_open("sem_fronta", O_CREAT, 0660, 0);
    sem_fronta[1] = sem_open("sem_fronta1", O_CREAT, 0660, 0);
    sem_fronta[2] = sem_open("sem_fronta2", O_CREAT, 0660, 0);
    sem_log = sem_open("sem_log", O_CREAT, 0660, 1);
    if(sem_customer == SEM_FAILED || sem_officer == SEM_FAILED || sem_main == SEM_FAILED || sem_fronta[0] == SEM_FAILED ||sem_fronta[1] == SEM_FAILED|| sem_fronta[2] == SEM_FAILED || sem_log == SEM_FAILED){
        fprintf(stderr, "Failed to load semaphores.\n");
        destroy_sem();
        destroy_memory();
    }
    if(nu2 == MAP_FAILED || waiting == MAP_FAILED || action == MAP_FAILED || closer == MAP_FAILED || frontas == MAP_FAILED){
        fprintf(stderr, "Failed to load shared memory.\n");    
        destroy_memory();
        destroy_sem(); 
    }
}

 
int main(int argc, char* argv[]){
    
    
    file = fopen("proj2.out","a"); 
    if (file == NULL) {
        fprintf(stderr,"Error opening file\n");
        destroy_memory();
        destroy_sem();
        fclose(file);
        return 1;
    }
    setbuf(file,NULL);
    load();

    params parametrs;
    int rozsah = 0;
    if(get_params(argc, argv, &parametrs) == 1){
        destroy_memory();
        destroy_sem();
        fclose(file);
        return 1;
    }
    pid_t pid_officer;
    int processes[parametrs.NZ];
    int processes2[parametrs.NU];
    pid_t pid_cust;
    
    
    
    
    
        
    
    

   
    
    for(int nu = 1; nu <= parametrs.NU; nu++){
        
        pid_officer = fork();
        rozsah = (random() % (parametrs.TU ) +1);
        
        if(pid_officer == 0){ // officer process
            officer(nu,rozsah);
        
        } else if(pid_officer > 0){
            processes2[nu] = pid_officer;
        } else if(pid_officer == -1){
            fprintf(stderr, "Couldnt make child proccess, program will end.\n");
            destroy_memory();
            destroy_sem();
            fclose(file);
            return 1;
        }





    }

    
    

    for(int nz=1; nz<=parametrs.NZ; nz++){
        
        
        pid_cust = fork();
        if(pid_cust == 0) {
            customer(parametrs.TZ); // customer proccess
        }else if(pid_cust > 0){
            processes[nz-1] = pid_cust;
        }else if(pid_cust == -1){
            fprintf(stderr, "Couldnt make child proccess, program will end.\n");
            destroy_memory();
            destroy_sem();
            fclose(file);
            return 1;
        }
       
    }

   print_closing(parametrs.F); // function to announce closing time
   
    
    
   




   
    
  for(int i = 0; i < parametrs.NU; i++) {
    waitpid(processes2[i], NULL, 0);        // wait for all kids to end
  }
  for(int i = 0; i < parametrs.NZ; i++) {
    waitpid(processes[i], NULL, 0);
  }
 

    
    destroy_sem();
    destroy_memory();
    
   
   
    fclose(file);
    return 0;
}