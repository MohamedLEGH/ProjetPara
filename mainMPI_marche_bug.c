#include "projet.h"
#include <time.h>
#include <sys/time.h>

#include <mpi.h>
/* 2017-02-23 : version 1.0 */
double my_gettimeofday(){
	struct timeval tmp_time;
	gettimeofday(&tmp_time,NULL);
	return tmp_time.tv_sec + (tmp_time.tv_usec * 1.0e-6L);
}
unsigned long long int node_searched = 0;
//long long int node_searched = 0;
void evaluate(tree_t * T, result_t *result)
{
        node_searched++;
  
        move_t moves[MAX_MOVES];
        int n_moves;

        result->score = -MAX_SCORE - 1;
        result->pv_length = 0;
        
        if (test_draw_or_victory(T, result))
          return;

        if (TRANSPOSITION_TABLE && tt_lookup(T, result))     /* la réponse est-elle déjà connue ? */
          return;
        
        compute_attack_squares(T);

        /* profondeur max atteinte ? si oui, évaluation heuristique */
        if (T->depth == 0) {
          result->score = (2 * T->side - 1) * heuristic_evaluation(T);
          return;
        }
        
        n_moves = generate_legal_moves(T, &moves[0]);

        /* absence de coups légaux : pat ou mat */
	if (n_moves == 0) {
          result->score = check(T) ? -MAX_SCORE : CERTAIN_DRAW;
          return;
        }
        
        if (ALPHA_BETA_PRUNING)
          sort_moves(T, n_moves, moves);

        /* évalue récursivement les positions accessibles à partir d'ici */
        for (int i = 0; i < n_moves; i++) {
		tree_t child;
                result_t child_result;
                
                play_move(T, moves[i], &child);
                
                evaluate(&child, &child_result);
                         
                int child_score = -child_result.score;

		if (child_score > result->score) {
			result->score = child_score;
			result->best_move = moves[i];
                        result->pv_length = child_result.pv_length + 1;
                        for(int j = 0; j < child_result.pv_length; j++)
                          result->PV[j+1] = child_result.PV[j];
                          result->PV[0] = moves[i];
                }

                if (ALPHA_BETA_PRUNING && child_score >= T->beta)
                  break;    

                T->alpha = MAX(T->alpha, child_score);
        }

        if (TRANSPOSITION_TABLE)
          tt_store(T, result);
}


int begin(tree_t *T,result_t *result,int rank,int p,MPI_Status status)
{
	printf("Je suis %d et je commence evalPara\n",rank);        


	node_searched++;


	result->score = -MAX_SCORE - 1;
	result->pv_length = 0;

	if (test_draw_or_victory(T, result))
		return 1;

	if (TRANSPOSITION_TABLE && tt_lookup(T, result))     /* la réponse est-elle déjà connue ? */
		return 1;

	compute_attack_squares(T);

/* profondeur max atteinte ? si oui, évaluation heuristique */
	if (T->depth == 0) 
	{
		result->score = (2 * T->side - 1) * heuristic_evaluation(T);
		return 1;
	}

	return 0;

}


void eval_update(int child_score,result_t child_result, tree_t *T, result_t *result,int i,move_t * moves)
{
	if (child_score > result->score) 
	{
		result->score = child_score;
		result->best_move = moves[i];
		result->pv_length = child_result.pv_length + 1;
		for(int j = 0; j < child_result.pv_length; j++)
			result->PV[j+1] = child_result.PV[j];
		result->PV[0] = moves[i];
	}
}


void evaluate_Para(tree_t * T, result_t *result,int rank, int p, MPI_Status status)
{    	
	int maitre =0;
	int i;
	int number;
	int debut = 1;
	move_t moves[MAX_MOVES];
	int n_moves;
	
	
	int test = begin(T,result,rank,p,status);
	if(test == 1) return;
	n_moves = generate_legal_moves(T, &moves[0]);
	
	
//printf(" nmoves = %d à la profondeur %d",n_moves, T->depth);


/* absence de coups légaux : pat ou mat */
	if (n_moves == 0) 
	{
		result->score = check(T) ? -MAX_SCORE : CERTAIN_DRAW;
		return;
	}

	if (ALPHA_BETA_PRUNING)
		sort_moves(T, n_moves, moves);



	MPI_Bcast(
	moves,
	MAX_MOVES,
	MPI_INT,
	0,
	MPI_COMM_WORLD);


	
	MPI_Bcast(
	&n_moves,
	1,
	MPI_INT,
	0,
	MPI_COMM_WORLD);


	
	if(p<n_moves)
	{

		for(i=1;i<p;i++)
		{
			number = i;
			MPI_Send( &number,1,MPI_INT, i, 0, MPI_COMM_WORLD);
		}
//int rankee;
		int child_score;
		for(i=1;i<p;i++)
		{
			tree_t child;
			result_t child_result;
			play_move(T, moves[i], &child);
			//MPI_Recv(&rankee,1,MPI_INT,i,0,MPI_COMM_WORLD,&status);
			MPI_Recv(&child_score,1,MPI_INT,i,0,MPI_COMM_WORLD,&status);
			// recv number pross puis chil_result
			eval_update(child_score,child_result,T,result,i,moves);

			if (ALPHA_BETA_PRUNING && child_score >= T->beta)
			break;    

			T->alpha = MAX(T->alpha, child_score);
		}
		debut = p;

		for (i = debut; i < n_moves; i++) 
		{
///
			tree_t child;
			result_t child_result;

			play_move(T, moves[i], &child);

			evaluate(&child, &child_result);

			child_score = -child_result.score;

			eval_update(child_score,child_result,T,result,i,moves);

			if (ALPHA_BETA_PRUNING && child_score >= T->beta)
				break;    

			T->alpha = MAX(T->alpha, child_score);
		}
	}
	else
	{
	//	printf("TEST TEST LOL\n \n");
		for(i=1;i<n_moves;i++)
		{			
			number = i;
			MPI_Send( &number,1,MPI_INT, i, 0, MPI_COMM_WORLD);
			
         //   printf("J'ai envoyé le numéro : %d \n",number);
		}
		int child_score;
		for(i=1;i<n_moves;i++)
		{


			tree_t child;
			result_t child_result;
			play_move(T, moves[i], &child);
			//MPI_Recv(&rankee,1,MPI_INT,i,0,MPI_COMM_WORLD,&status);
			MPI_Recv(&child_score,1,MPI_INT,i,0,MPI_COMM_WORLD,&status);
			// recv number pross puis chil_result
			eval_update(child_score,child_result,T,result,i,moves);

			if (ALPHA_BETA_PRUNING && child_score >= T->beta)
				break;    

			T->alpha = MAX(T->alpha, child_score);
		}
//debut = n_moves;
	}


	tree_t child;
	result_t child_result;

	play_move(T, moves[maitre], &child);

	evaluate(&child, &child_result);

	int child_score = -child_result.score;

	//printf(" Je suis rank %d et mon score est %d \n \n", rank,child_score);

	eval_update(child_score,child_result,T,result,maitre,moves);

	if (ALPHA_BETA_PRUNING && child_score >= T->beta)
		return;    

	T->alpha = MAX(T->alpha, child_score);


	if (TRANSPOSITION_TABLE)
	tt_store(T, result);

	//printf("Je suis %d et j'ai fini evalPara\n",rank);
}

void eval_slave(tree_t * T, result_t *result,int rank, int p, MPI_Status status)
{

		
		//printf("Je suis %d et je commence eval_slave\n",rank);        
		int maitre =0;
		
		int number;
							        
        result->score = -MAX_SCORE - 1;
        result->pv_length = 0;
        
        move_t moves[MAX_MOVES];
                
        MPI_Bcast(
		moves,
		MAX_MOVES,
		MPI_INT,
		0,
		MPI_COMM_WORLD);
		int n_moves;
		
		MPI_Bcast(
		&n_moves,
		1,
		MPI_INT,
		0,
		MPI_COMM_WORLD);
		
		if(p<n_moves)
		{
			tree_t child;
        	result_t child_result;
        	//	 	printf(" TT Je suis %d et j'ai (presque) fini\n",rank);
        
        	MPI_Recv(&number,1,MPI_INT,0,0,MPI_COMM_WORLD,&status);
            
            //printf("Je suis %d et j'ai reçu le numéro : %d \n",rank,number);
                
        	play_move(T, moves[number], &child);
                
        	evaluate(&child, &child_result);
                         
        	int child_score = -child_result.score;

			//int rankee = rank;
			// 	MPI_Send( &rankee,1,MPI_INT, 0, 0, MPI_COMM_WORLD);

		 	MPI_Send( &child_score,1,MPI_INT, 0, 0, MPI_COMM_WORLD);
		}
		else if(rank<=n_moves)
		{
			tree_t child;
        	result_t child_result;
        
        	MPI_Recv(&number,1,MPI_INT,0,0,MPI_COMM_WORLD,&status);
            //printf("Je suis %d et j'ai reçu le numéro : %d \n",rank,number);
        	//printf("ca marche sur rank %d \n",rank);
            play_move(T, moves[number], &child);
                
        	evaluate(&child, &child_result);
        	int child_score = -child_result.score;
        	//printf(" Je suis rank %d et mon score est %d \n \n", rank,child_score);
        
		 	MPI_Send( &child_score,1,MPI_INT, 0, 0, MPI_COMM_WORLD);

		}

		//printf("Je suis %d et j'ai fini eval_slave\n",rank);
}

void decide(tree_t * T, result_t *result, int rank, int p, MPI_Status status)
{
int maitre = 0;

		//printf(" Je suis %d et je commence le decide\n",rank);        
	for (int depth = 1;; depth++) {
		T->depth = depth;
		T->height = 0;
		T->alpha_start = T->alpha = -MAX_SCORE - 1;
		T->beta = MAX_SCORE + 1;

        if(rank == maitre) {       printf("=====================================\n"); }

		if(rank == maitre) { evaluate_Para(T, result,rank,p,status); }
		else {eval_slave(T,result,rank,p,status); }

        if(rank == maitre)        printf("depth: %d / score: %.2f / best_move : ", T->depth, 0.01 * result->score);
        if(rank == maitre)        print_pv(T, result);
        if(rank == maitre)
        {
//          printf("Le 0 a comme score %d \n" ,result->score);
//  printf("Le 0 a comme best move %d \n" ,result->best_move);
//  printf("Le 0 a comme length %d \n" ,result->pv_length);
        
        
        }
        
        int finit = result->score;
        
            MPI_Bcast(
    &finit,
    1,
    MPI_INT,
    0,
    MPI_COMM_WORLD);
                
                if (DEFINITIVE(finit))

                  break;

	}
	//printf("Je suis %d et j'ai fini le decide\n",rank);
}


int main(int argc, char **argv)
{  

MPI_Init(&argc, &argv);
		int rank,p;
		int maitre =0;
		MPI_Status status;

		MPI_Comm_size(MPI_COMM_WORLD, &p);
		MPI_Comm_rank(MPI_COMM_WORLD,&rank);
	//			printf("Je suis %d et je commence le programme\n",rank);        
	double debut ,fin;
	tree_t root;
        result_t result;

        if (argc < 2) {
        if(rank == maitre) printf("usage: %s \"4k//4K/4P w\" (or any position in FEN)\n", argv[0]);
          exit(1);
        }

        if (ALPHA_BETA_PRUNING)
          if(rank == maitre) printf("Alpha-beta pruning ENABLED\n");

        if (TRANSPOSITION_TABLE) {
          if(rank == maitre) printf("Transposition table ENABLED\n");
          init_tt();
        }
        
        parse_FEN(argv[1], &root);
        if(rank == maitre) print_position(&root);
        
    debut = my_gettimeofday();     
    //		printf("Je suis %d et je commence B\n",rank);        
	decide(&root, &result,rank,p,status);
	fin = my_gettimeofday();
	//printf("Je suis %d et j'ai fini le programme\n",rank);
	
//long long int testii = node_searched;	
/*        MPI_Reduce(
    &testii,
    &testii,
    1,
    MPI_LONG_LONG_INT,
    MPI_SUM,
    0,
    MPI_COMM_WORLD);
    */
    
    unsigned long long int nod_totaux;
MPI_Reduce(&node_searched, &nod_totaux, 1, MPI_LONG_LONG_INT, MPI_SUM,0,
              MPI_COMM_WORLD);

//printf("Je suis rank %d et mon nod vaut : %lli \n\n",rank,node_searched);
	
	if(rank == maitre){
	printf("\nDécision de la position: ");
        switch(result.score * (2*root.side - 1)) {
        case MAX_SCORE: printf("blanc gagne\n"); break;
        case CERTAIN_DRAW: printf("partie nulle\n"); break;
        case -MAX_SCORE: printf("noir gagne\n"); break;
        default: printf("BUG\n");
        }

        //printf("Node searched: %llu\n", node_searched);
        printf("Node searched: %llu\n", nod_totaux);
        fprintf(stderr,"Temps total : %g sec\n", fin-debut);
        fprintf(stdout, "%g\n", fin-debut);
        }        
        if (TRANSPOSITION_TABLE)
          free_tt();
    
    MPI_Finalize();      
	return 0;

}
