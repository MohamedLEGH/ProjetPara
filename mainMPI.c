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

void evaluate_Para(tree_t * T, result_t *result)
{    	int rank,p;
		int maitre =0;
		MPI_Status status;

		MPI_Comm_size(MPI_COMM_WORLD, &p);
		MPI_Comm_rank(MPI_COMM_WORLD,&rank);

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

        MPI_Bcast(
    moves,
    MAX_MOVES,
    MPI_INT,
    0,
    MPI_COMM_WORLD);

        /* absence de coups légaux : pat ou mat */
	if (n_moves == 0) {
          result->score = check(T) ? -MAX_SCORE : CERTAIN_DRAW;
          return;
        }
        
        if (ALPHA_BETA_PRUNING)
          sort_moves(T, n_moves, moves);

        /* évalue récursivement les positions accessibles à partir d'ici */
        int i;
        
        int debut = 0;
        if(p<n_moves){
        int number;
	 	for(i=1;i<p;i++)
 		{
		 	MPI_Send( &number,1,MPI_INT, i, 0, MPI_COMM_WORLD);
		}
		//int rankee;
		 		int child_score;
			 	for(i=1;i<p;i++)
 		{
 		

 				tree_t child;
        result_t child_result;
        play_move(T, moves[rank], &child);
 		        //MPI_Recv(&rankee,1,MPI_INT,i,0,MPI_COMM_WORLD,&status);
 		        MPI_Recv(&child_score,1,MPI_INT,i,0,MPI_COMM_WORLD,&status);
		// recv number pross puis chil_result
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
        debut = p;
        }
        				tree_t child;
                result_t child_result;
                
                play_move(T, moves[maitre], &child);
                
                evaluate(&child, &child_result);
                         
                int child_score = -child_result.score;

		if (child_score > result->score) {
			result->score = child_score;
			result->best_move = moves[maitre];
                        result->pv_length = child_result.pv_length + 1;
                        for(int j = 0; j < child_result.pv_length; j++)
                          result->PV[j+1] = child_result.PV[j];
                          result->PV[0] = moves[maitre];
                }

                if (ALPHA_BETA_PRUNING && child_score >= T->beta)
                  return;    

                T->alpha = MAX(T->alpha, child_score);
        for (i = debut; i < n_moves; i++) {
///
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

void eval_slave(tree_t * T, result_t *result)
{		int rank,p;
		int maitre =0;
		MPI_Status status;
		int number;
		MPI_Comm_size(MPI_COMM_WORLD, &p);
		MPI_Comm_rank(MPI_COMM_WORLD,&rank);
		        
        result->score = -MAX_SCORE - 1;
        result->pv_length = 0;
        
        move_t moves[MAX_MOVES];
                
        MPI_Bcast(
    moves,
    MAX_MOVES,
    MPI_INT,
    0,
    MPI_COMM_WORLD);
		
		tree_t child;
        result_t child_result;
        
        
        MPI_Recv(&number,1,MPI_INT,0,0,MPI_COMM_WORLD,&status);
                
        play_move(T, moves[rank], &child);
                
        evaluate(&child, &child_result);
                         
        int child_score = -child_result.score;

		//int rankee = rank;
		// 	MPI_Send( &rankee,1,MPI_INT, 0, 0, MPI_COMM_WORLD);
		 	
		 	MPI_Send( &child_score,1,MPI_INT, 0, 0, MPI_COMM_WORLD);

}

void decide(tree_t * T, result_t *result)
{		int rank,p;
		int maitre =0;
		MPI_Status status;

		MPI_Comm_size(MPI_COMM_WORLD, &p);
		MPI_Comm_rank(MPI_COMM_WORLD,&rank);

	for (int depth = 1;; depth++) {
		T->depth = depth;
		T->height = 0;
		T->alpha_start = T->alpha = -MAX_SCORE - 1;
		T->beta = MAX_SCORE + 1;

        if(rank == maitre)        printf("=====================================\n");
		if(rank == maitre) evaluate_Para(T, result);
		else eval_slave(T,result);

        if(rank == maitre)        printf("depth: %d / score: %.2f / best_move : ", T->depth, 0.01 * result->score);
        if(rank == maitre)        print_pv(T, result);
                
                if (DEFINITIVE(result->score))
                  break;
	}
}


int main(int argc, char **argv)
{  

MPI_Init(&argc, &argv);
		int rank,p;
		int maitre =0;
		MPI_Status status;

		MPI_Comm_size(MPI_COMM_WORLD, &p);
		MPI_Comm_rank(MPI_COMM_WORLD,&rank);
		
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
	decide(&root, &result);
	fin = my_gettimeofday();
	if(rank == maitre){
	printf("\nDécision de la position: ");
        switch(result.score * (2*root.side - 1)) {
        case MAX_SCORE: printf("blanc gagne\n"); break;
        case CERTAIN_DRAW: printf("partie nulle\n"); break;
        case -MAX_SCORE: printf("noir gagne\n"); break;
        default: printf("BUG\n");
        }

        printf("Node searched: %llu\n", node_searched);
        fprintf(stderr,"Temps total : %g sec\n", fin-debut);
        fprintf(stdout, "%g\n", fin-debut);
        }        
        if (TRANSPOSITION_TABLE)
          free_tt();
    MPI_Finalize();      
	return 0;

}
