#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>

#define DIMENSION 100 //DIMENSION OF MATRIX (100by100)
#define PRECISION 0.01//0.01 seems to be a popular value
#define DEBUGMODE 0 //switch to 1 to see debugging messages

struct checklist_cell
{
    double old_val;
    int state;
};

/*
0:  done computing avg, not in the "pool" (when all the array of structs is 0,
    then it's ready for a new iteration)
1:  value in the "pool" of available cells
2:  busy computing avg, not in the "pool"
*/
int total_th_num;// >=1 AND <=16, NO GUARDS WERE IMPLEMENTED FOR SILLY VALUES
double **gl_arr;//array containing the actual values
struct checklist_cell **gl_checklist;//support array
pthread_mutex_t mymutex=PTHREAD_MUTEX_INITIALIZER;//mutex to protect "done"
int done = 0;//variable that helps threads figure out when they are all done
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;//to wake up threads that
                                               //finished with their work



/////////////////////////////////////////////
/////                                   /////
/////            MAIN AT THE            /////
/////            BOTTOM                 /////
/////                                   /////
/////            ///////////            /////
/////            ///////////            /////
/////            ///////////            /////
/////            ///////////            /////
/////            ///////////            /////
/////            ///////////            /////
/////            ///////////            /////
/////        ///////////////////        /////
/////         /////////////////         /////
/////          ///////////////          /////
/////           /////////////           /////
/////            ///////////            /////
/////             /////////             /////
/////              ///////              /////
/////               /////               /////
/////                ///                /////
/////                 /                 /////
/////                                   /////
/////////////////////////////////////////////





void check_ptr_1D_mem(double **allocation)
{
    if (allocation == NULL)
    {
        printf("NO MEMORY\n");
        exit(0);
    }
}

void check_1D_mem(double *allocation)
{
    if (allocation == NULL)
    {
        printf("NO MEMORY\n");
        exit(0);
    }
}

void check_ptr_1D_mem_str(struct checklist_cell **allocation)
{
    if (allocation == NULL)
    {
        printf("NO MEMORY\n");
        exit(0);
    }
}

void check_1D_mem_str(struct checklist_cell *allocation)
{
    if (allocation == NULL)
    {
        printf("NO MEMORY\n");
        exit(0);
    }
}

void link_ptr_to_values(double **arr, double *buf, int len)
{
    int i;
    for (i = 0; i < len; i++)
    {
        arr[i] = buf + len*i;
    }
}

void link_ptr_to_values_str(struct checklist_cell **arr, struct checklist_cell *buf, int len)
{
    int i;
    for (i = 0; i < len; i++)
    {
        arr[i] = buf + len*i;
    }
}
    

void init_values(double **allocation)
{
    int i,j;
    for (i = 0; i < DIMENSION; i++)
    {
        for (j = 0; j < DIMENSION; j++)
        {
            allocation[i][j] = (rand() % 100);
        }
    }
}

void init_checklist_values()
{
    int i,j;
    for (i = 0; i < (DIMENSION-2); i++)
    {
        for (j = 0; j < (DIMENSION-2); j++)
        {
            gl_checklist[i][j].old_val = 42;
            gl_checklist[i][j].state = 1;
        }
    }
}


//debug
void print_2D_arr(double **all, int len)
{
    int i,j;
    for( i = 0; i < len; i++ )
    {
        for( j = 0; j < len; j++ )
        {
            printf("%f ",all[i][j]);
        }
        printf("\n");
    }
    printf("\n");

}

//debug
void print_struct_arr()
{
    int i,j;
    printf("OLD VALUES\n");
    for( i = 0; i < (DIMENSION-2); i++ )
    {
        for( j = 0; j < (DIMENSION-2); j++ )
        {
            printf("%f ",gl_checklist[i][j].old_val);
        }
        printf("\n");
    }
    printf("\n");
    printf("STATES\n");
    for( i = 0; i < (DIMENSION-2); i++ )
    {
        for( j = 0; j < (DIMENSION-2); j++ )
        {
            if(gl_checklist[i][j].state==1)
            {
                printf(". ");
            }
            else
            {
                printf("%d ",gl_checklist[i][j].state);
            }

        }
        printf("\n");
    }
    printf("\n");

}

//init checklist array (state 1 means that the cell is waiting to be computed)
void init_old_values()
{
    int i,j;
    for (i = 0; i < DIMENSION-2; i++)
    {
        for (j = 0; j < DIMENSION-2; j++)
        {
            gl_checklist[i][j].old_val = gl_arr[i+1][j+1];
            gl_checklist[i][j].state = 1;
        }
    }
}

double find_largest_in_arr_diff()
{
    int i,j;
    for( i = 0; i < (DIMENSION-2); i++ )
    {
        for( j = 0; j < (DIMENSION-2); j++ )
        {
            if(fabs(gl_checklist[i][j].old_val-gl_arr[(i+1)][(j+1)])>PRECISION)
            {
                return 0;//not done yet
            }
        }
    }
    return 1;//done, all values are below the precision
}

void print_diff()
{
    int i,j;
    for( i = 0; i < (DIMENSION-2); i++ )
    {
        for( j = 0; j < (DIMENSION-2); j++ )
        {
            printf("%f ",fabs(gl_checklist[i][j].old_val-gl_arr[(i+1)][(j+1)]));
        }
        printf("\n");
    }
}

//checks validity of result
int print_outcome()
{
    int i,j;
    for( i = 0; i < (DIMENSION-2); i++ )
    {
        for( j = 0; j < (DIMENSION-2); j++ )
        {
            if(fabs(gl_checklist[i][j].old_val-gl_arr[(i+1)][(j+1)])>PRECISION)
            {
                printf("Value %f is larger than precision \n",
                       fabs(gl_checklist[i][j].old_val-gl_arr[(i+1)][(j+1)]));
                return 0;
            }
        }
    }
    return 1;
}


double get_average(double **allocation, int i, int j)
{
    return ( (  (allocation[i][j+1]) + (allocation[i][j-1]) + 
              (allocation[i+1][j]) + (allocation[i-1][j])  )/4 );
}

int count_number_of_zeros()
{
    int i,j;
    for (i = 0; i < (DIMENSION-2); i++)
    {
        for (j = 0; j < (DIMENSION-2); j++)
        {
            if(gl_checklist[i][j].state==1)
            {
                return 1;
            }
        }
    }
    return 0;
}


void change_flags_of_state_array(int x,int y,int state_change)
{
    if(state_change==1)
    {
        gl_checklist[x][y].state=2;//busy
    }
    else
    {
        gl_checklist[x][y].state=0;//done
    }
}



int pick_cell_for_avg(int index_to_check)
{
    int picked_1D_converted_location=(-1);
    int k,j;

    if(index_to_check<(DIMENSION-2)*(DIMENSION-2))
    {
        k=index_to_check/(DIMENSION-2);//converting 1D into 2D location
        j=index_to_check%(DIMENSION-2);
        if(gl_checklist[k][j].state==1)//not necessary, just to be sure
        {
            picked_1D_converted_location=(k)*(DIMENSION-2)+j;
            //packaging k and j in a single value to avoid messy array pass
            change_flags_of_state_array(k,j,(-1));
        }
    }
    
    return picked_1D_converted_location;
}

void *update_averages(void *arg)
{
    int iteration_counter=0;
    int counter=0;
    int x,y;
    int picked_1D_converted_location;
    int a = *((int *) arg);
    int *last_checked = malloc((unsigned long)(total_th_num-1) * sizeof(int));
    int i;
    for (i=0; i<total_th_num; i++)
    {
        last_checked[i] = 0;
    }


    do
    {
        while(count_number_of_zeros()==1)
        {
            //uses array ID and local counter to locate the cell to work on
            last_checked[a]=(total_th_num*counter+a);
            counter++;
            //IMPORTANT STEP
            //      |
            //      |
            //      V
            picked_1D_converted_location=pick_cell_for_avg(last_checked[a]);
            if(picked_1D_converted_location!=(-1))
            {
                x = picked_1D_converted_location/(DIMENSION-2);
                y = picked_1D_converted_location%(DIMENSION-2);

                //IMPORTANT STEP
                //      |
                //      |
                //      V
                gl_arr[(x+1)][(y+1)]=get_average(gl_arr,(x+1),(y+1));
                //since we are using a smaller matrix for the internal values,
                //everything is shifted by 1

                //to check that processes are actually "shuffling"
                if(DEBUGMODE==1)
                {
                    switch(a%5) {
                        case 0  :printf(".");
                            break;
                        case 1  :printf("^");
                            break;
                        case 2  :printf("|");
                            break;
                        case 3  :printf("x");
                            break;
                        default :printf("?");
                    }
                }

            }
            else
            {
                last_checked[a] = 0;
                break;}
        }

        /*
        while being aware of the existance of barriers, this is a similar
        aproach that uses condition variables (was useful during debugging)
        HOW THE BELOW CONDITION WORKS:
        
        The thread waits for the lock, once inside he checks the variable "done"
        to know if he's the last one of all, if he isn't he just sits and waits.
        If he realised he's the last one finishing, he checks everything is
        below the precision (using "find_largest_in_arr_diff()" which already
        "knows") and depending on the outcome he shuts down everything or
        tells everyone to start a new iteration.
        */
        pthread_mutex_lock(&mymutex);
        if(done>(total_th_num-2))
        {
            if(DEBUGMODE==1)
                printf("\n----\nEND of Iteration %d\n\n\n",iteration_counter);
            done=0;
            if(find_largest_in_arr_diff()==1)
            {
                if(DEBUGMODE==1)
                    printf("DESIRED PRECISION WAS REACHED\n");
                done=-1;
            }
            else
            {
                if(DEBUGMODE==1)
                    printf("LAST THREAD DID HIS JOB, start next iteration\n");
                init_old_values();
                counter=0;
                last_checked[a] = 0;
            }
            pthread_cond_broadcast( &cond );
        }
        else//sit and wait
        {
            counter=0;
            last_checked[a] = 0;
            done++;
            if(DEBUGMODE==1)
                printf("\n%d)I am waiting and I arrived %d",a,done);
            pthread_cond_wait( & cond, & mymutex );

        }
        pthread_mutex_unlock(&mymutex);
        iteration_counter++;
    } while(done>=0);
    return NULL;
}


int main(int argc, char** argv)
{
    //accepting thread number as an argument
    total_th_num=argc;
    char *str = argv[1];
    total_th_num=atoi(str);

    //initialise the initial matrix
    gl_arr = malloc(DIMENSION*sizeof(double*));
    check_ptr_1D_mem(gl_arr);
    double *buf = malloc(DIMENSION*DIMENSION*sizeof(double));
    check_1D_mem(buf);
    link_ptr_to_values(gl_arr,buf,DIMENSION);
    init_values(gl_arr);

    //initialise array for storing previous values.
    /*  NOTE:
        This path has been taken given the "superstep style", first
        work, then check at the end of each iteration.
    */
    gl_checklist = malloc((DIMENSION-2)*sizeof(struct checklist_cell*));
    check_ptr_1D_mem_str(gl_checklist);
    struct checklist_cell *buf_checklist = malloc((DIMENSION-2)*(DIMENSION-2)
                                                  *sizeof(struct checklist_cell));
    check_1D_mem_str(buf_checklist);
    link_ptr_to_values_str(gl_checklist,buf_checklist,(DIMENSION-2));
    init_checklist_values();
    
    

    struct timeval start, end;
    //start timer
    gettimeofday(&start, NULL);
    pthread_t mythread[total_th_num];
    //THREADS ARE GENERATED
    int i;
    for ( i=0; i<total_th_num; i++)
    {
        int *arg = malloc(sizeof(*arg));
        *arg = i;
        if ( pthread_create( &mythread[i], NULL, update_averages, arg) )
        {
            printf("NO THREAD\n");
            exit(0);
        }
    }
    //THREADS ARE COLLECTED
    for ( i=0; i<total_th_num; i++)
    {
        pthread_join(mythread[i], NULL);
    }
    //stop timer
    gettimeofday(&end, NULL);
    print_2D_arr(gl_arr,DIMENSION);
    print_diff();//"excerpt" to test "correctness"
    if(print_outcome()==0)
    {
        printf("Precision warning\n");
    }
    else
    {
        printf("Successfully below precision\n");
    }
    free(gl_arr);
    free(gl_checklist);
    free(buf);
    free(buf_checklist);
    printf("\ntime: %ld\n", ((end.tv_sec * 1000000 + end.tv_usec)
                             - (start.tv_sec * 1000000 + start.tv_usec)));
    printf("dim: %d\nthreads: %d\n",DIMENSION,total_th_num);
    return 0;
}