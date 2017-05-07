#include "projetv2.h"
#include <time.h>
#include <sys/time.h>

#include <mpi.h>


#define TAG_REQ 0
#define TAG_DATA 1
#define TAG_END 2
/* 2017-04-25 : version 1.0 */
/*Version MPI*/


// Fonction pour avoir le temps du programme
double my_gettimeofday()
{
	struct timeval tmp_time;
	gettimeofday(&tmp_time,NULL);
	return tmp_time.tv_sec + (tmp_time.tv_usec * 1.0e-6L);
}

unsigned long long int node_searched = 0;


// données pour MPI
int p,rank;
MPI_Status status;
MPI_Request req;

// structure pour le MPI_Reduce (ne servira plus à rien plus tard à priori)

typedef struct 
   { 
        int val; 
        int rank; 
   }inDeux; 
    
    
// Objets pour la structure MPI	 
	 int nb_items = 4;
     MPI_Datatype types[4] = {MPI_INT,MPI_INT,MPI_INT,MPI_INT};
     MPI_Datatype mpi_resultat;
     MPI_Aint offsets[4];
     int blocklens[4] = {1,1,1,MAX_DEPTH};


// Fonction de base, récursive et séquentielle (ne pas y toucher)
    
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
                int alpha = T->alpha;
                	//	MPI_Allreduce(&alpha,&T->alpha,1,MPI_INT,MPI_MAX,MPI_COMM_WORLD);
                
        }

        if (TRANSPOSITION_TABLE)
          tt_store(T, result);
}

/*Cette fonction sert juste à initialiser les trucs dans le evaluate paralléle*/
int begin(tree_t *T,result_t *result)
{
	//printf("Je suis %d et je commence evalPara\n",rank);        


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


void evaluate_Para(tree_t * T, result_t *result)
{ 
   	// Les initialisations du début
	int maitre =0;
	int i;
	int number;
	int debut = 1;
	move_t moves[MAX_MOVES];
	int n_moves;
	
	result_t rer1;
	
	
	int test = begin(T,result);
	if(test == 1) return;
	n_moves = generate_legal_moves(T, &moves[0]);
	
	
	// On créer notre structure MPI
   	offsets[0] = offsetof(result_t,score);
    offsets[1] = offsetof(result_t,best_move);
    offsets[2] = offsetof(result_t,pv_length);
    offsets[3] = offsetof(result_t,PV); 
    MPI_Type_create_struct(nb_items,blocklens,offsets,types,&mpi_resultat);  
    MPI_Type_commit(&mpi_resultat);
	
//printf(" nmoves = %d à la profondeur %d \n \n",n_moves, T->depth);


/* absence de coups légaux : pat ou mat */
	if (n_moves == 0) 
	{
		result->score = check(T) ? -MAX_SCORE : CERTAIN_DRAW;
		return;
	}

	if (ALPHA_BETA_PRUNING)
		sort_moves(T, n_moves, moves);

//	printf("Je suis %d , test avant les broadcost\n",rank);        

// On envoie a tout le monde le tableau de moves et le nombre de moves
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
		number = 0;

		for(i=1;i<p;i++)
		{
			MPI_Send( &number,1,MPI_INT, i, 0, MPI_COMM_WORLD);
			number++;
		}
//int rankee;
		debut = p;

		tree_t child;
		result_t child_result;

		play_move(T, moves[number], &child);

		evaluate(&child, &child_result);

		int child_score = -child_result.score;

		//printf(" Je suis rank %d et mon score est %d \n \n", rank,child_score);

		eval_update(child_score,child_result,T,result,number,moves);

		if (ALPHA_BETA_PRUNING && child_score >= T->beta)
			return;    

		T->alpha = MAX(T->alpha, child_score);
	
		number++;

	
		while(number<n_moves)
		{

			MPI_Recv(&rer1,1,mpi_resultat,MPI_ANY_SOURCE,TAG_DATA,MPI_COMM_WORLD,&status);
			
			MPI_Send(&number,1,MPI_INT,status.MPI_SOURCE,TAG_REQ,MPI_COMM_WORLD);
			if(rer1.score > result->score)
			{
				result->score = rer1.score;
				result->best_move = rer1.best_move;
				result->pv_length = rer1.pv_length;
				for(int j = 0; j < rer1.pv_length; j++) result->PV[j] = rer1.PV[j];
			}

			number++;
		}
	
		for(i=1;i<p;i++)
		{
		

			MPI_Recv(&rer1,1,mpi_resultat,MPI_ANY_SOURCE,TAG_DATA,MPI_COMM_WORLD,&status);
			
			MPI_Send( &number,1,MPI_INT, i, TAG_END, MPI_COMM_WORLD);
			if(rer1.score > result->score)
			{
				result->score = rer1.score;
				result->best_move = rer1.best_move;
				result->pv_length = rer1.pv_length;
				for(int j = 0; j < rer1.pv_length; j++) result->PV[j] = rer1.PV[j];
			}

		}
	}
	else
	{
	//	printf("Je suis %d , test avant les send de pieces\n",rank);        
	//	printf("TEST TEST LOL\n \n");
		for(i=1;i<n_moves;i++)
		{			
			number = i;
			MPI_Send( &number,1,MPI_INT, i, 0, MPI_COMM_WORLD);
			
         //   printf("J'ai envoyé le numéro : %d \n",number);
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
		inDeux inDeuxB;
		inDeuxB.val = result->score;
		inDeuxB.rank = rank;
		inDeux inDeuxF; 
			inDeuxF.val = result->score;
		inDeuxF.rank = rank;
			//	printf("Je suis %d , test avant le reduce\n",rank);        

		MPI_Allreduce(&inDeuxB,&inDeuxF,1,MPI_2INT,MPI_MAXLOC,MPI_COMM_WORLD);

	//printf("Je suis le 0, test après reduce \n");
		
	
//	    								printf("Je suis %d , et le rank qui doit envoyer est %d \n",rank,inDeuxF.rank);
		if(rank != inDeuxF.rank)
		{
	//									printf("Je suis %d , test avant recv data \n",rank);
				result->score = inDeuxF.val;
				MPI_Recv(&result,1,mpi_resultat,inDeuxF.rank,0,MPI_COMM_WORLD,&status);

		}
	}
			
	if (TRANSPOSITION_TABLE)
	tt_store(T, result);

	//printf("Je suis %d et j'ai fini evalPara\n",rank);
        MPI_Type_free(&mpi_resultat);
}

void eval_slave(tree_t * T, result_t *result)
{
	result_t rer1;

		//		printf("Je suis %d ,slave test \n",rank);        
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
	
     offsets[0] = offsetof(result_t,score);
     offsets[1] = offsetof(result_t,best_move);
     offsets[2] = offsetof(result_t,pv_length);
     offsets[3] = offsetof(result_t,PV); 
     MPI_Type_create_struct(nb_items,blocklens,offsets,types,&mpi_resultat);  
     MPI_Type_commit(&mpi_resultat);
	
	
	
	//printf("Je suis %d , le nb de moves est %d et il y a %d processeurs \n",rank,n_moves,p);
	
	if(p<n_moves)
	{
		tree_t child;
    	result_t child_result;
    	//	 	printf(" TT Je suis %d et j'ai (presque) fini\n",rank);
    
    	MPI_Recv(&number,1,MPI_INT,0,MPI_ANY_TAG,MPI_COMM_WORLD,&status);
        
        //printf("Je suis %d et j'ai reçu le numéro : %d \n",rank,number);
        while(status.MPI_TAG!=TAG_END)
        {
	    	play_move(T, moves[number], &child);
	            
	    	evaluate(&child, &child_result);
	                     
	    	int child_score = -child_result.score;
			
			eval_update(child_score,child_result,T,result,number,moves);

			if (ALPHA_BETA_PRUNING && child_score >= T->beta)
			return;    

			T->alpha = MAX(T->alpha, child_score);
			
			
			rer1.score = result->score;
			rer1.best_move = result->best_move;
			rer1.pv_length = result->pv_length;
			for(int j = 0; j < result->pv_length; j++)	rer1.PV[j] = result->PV[j];
			
			
			MPI_Send(&rer1,1,mpi_resultat,0,TAG_DATA,MPI_COMM_WORLD);

			MPI_Recv(&number,1,MPI_INT,0,MPI_ANY_TAG,MPI_COMM_WORLD,&status);
		}
	}
	else 
	{
//		printf("Je suis %d et pou l'instant tout vas bien dans le programme \n",rank);
		if(rank<n_moves)
		{
//							printf("Je suis %d ,slave test avant receiv move\n",rank);        
			tree_t child;
	    	result_t child_result;
	    
	    	MPI_Recv(&number,1,MPI_INT,0,0,MPI_COMM_WORLD,&status);
	        //printf("Je suis %d et j'ai reçu le numéro : %d \n",rank,number);
	    	//printf("ca marche sur rank %d \n",rank);
	        play_move(T, moves[number], &child);
	            
	    	evaluate(&child, &child_result);
	    	int child_score = -child_result.score;
	    	//printf(" Je suis rank %d et mon score est %d \n \n", rank,child_score);
	    
			eval_update(child_score,child_result,T,result,number,moves);

			if (ALPHA_BETA_PRUNING && child_score >= T->beta)
			return;    

			T->alpha = MAX(T->alpha, child_score);

		}
		
		inDeux inDeuxB;
		inDeuxB.val = result->score;
		inDeuxB.rank = rank;
		
		inDeux inDeuxF; 
		inDeuxF.val = result->score;
		inDeuxF.rank = rank;
//								printf("Je suis %d , slave test avant reduce avec un score de %d \n",rank,result->score);     

		MPI_Allreduce(&inDeuxB,&inDeuxF,1,MPI_2INT,MPI_MAXLOC,MPI_COMM_WORLD);
//	    								printf("Je suis %d , et le rank qui doit envoyer est %d \n",rank,inDeuxF.rank);
		if(rank<=n_moves)
		{
			if(rank == inDeuxF.rank)
			{
		//									printf("Je suis %d , slave test avant envoie data \n",rank);        
					MPI_Send(&result,1,mpi_resultat,0,0,MPI_COMM_WORLD);

				//printf("Je suis %d et j'ai fini eval_slave\n",rank);
			}
			
		}
	}
        MPI_Type_free(&mpi_resultat);
}

void decide(tree_t * T, result_t *result)
{
	int maitre = 0;

	//	printf(" Je suis %d et je commence le decide\n",rank);        
	for (int depth = 1;; depth++) 
	{
		T->depth = depth;
		T->height = 0;
		T->alpha_start = T->alpha = -MAX_SCORE - 1;
		T->beta = MAX_SCORE + 1;

		if(rank == maitre) {       printf("=====================================\n"); }
		if(p == 1) {evaluate(T,result);}
		else
		{ 
			if(rank == maitre) { evaluate_Para(T, result); }
			else {eval_slave(T,result); }
		}
		    if(rank == maitre)        printf("depth: %d / score: %.2f / best_move : ", T->depth, 0.01 * result->score);
		    if(rank == maitre)        print_pv(T, result);

	 /*       if(rank == maitre)
		    {
	 //         printf("Le 0 a comme score %d \n" ,result->score);
	 // printf("Le 0 a comme best move %d \n" ,result->best_move);
	 //  printf("Le 0 a comme length %d \n" ,result->pv_length);
		    
		    
		    }
	   */     
		    int finit = result->score;
		if(p>1)
		{        
		
		MPI_Bcast(
		&finit,
		1,
		MPI_INT,
		0,
		MPI_COMM_WORLD);
		
		}            
		
		if (DEFINITIVE(finit)) break;

	}
	//printf("Je suis %d et j'ai fini le decide\n",rank);
}


int main(int argc, char **argv)
{  

		MPI_Init(&argc, &argv);
		//int rank,p;
		int maitre = 0;
		//MPI_Status status;

		MPI_Comm_size(MPI_COMM_WORLD, &p);
		MPI_Comm_rank(MPI_COMM_WORLD,&rank);
		//		printf("Je suis %d et je commence le programme\n",rank);        
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
     //   if(rank == maitre) printf("Début du programme PARA \n \n");
     
     
    debut = my_gettimeofday();     
    //		printf("Je suis %d et je commence B\n",rank);        
	decide(&root, &result);
	fin = my_gettimeofday();
	//printf("Je suis %d et j'ai fini le programme\n",rank);
    
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
          
	//MPI_Request_free(&req); // ne marche pas donc je l'enlève
    MPI_Finalize();      
	return 0;

}

// Code ancien  , on le garde si besoin


/*
int result_test = result->score;
int result_best = result->best_move;
int result_pvlength = result->pv_length;
int result_PPV[MAX_DEPTH];
*/
//for(int j = 0; j < result->pv_length; j++)	result_PPV[j] = result->PV[j];

/*
MPI_Recv(&result_test,1,MPI_INT,MPI_ANY_SOURCE,TAG_DATA,MPI_COMM_WORLD,&status);
MPI_Recv(&result_best,1,MPI_INT,status.MPI_SOURCE,TAG_DATA,MPI_COMM_WORLD,&status);
MPI_Recv(&result_pvlength,1,MPI_INT,status.MPI_SOURCE,TAG_DATA,MPI_COMM_WORLD,&status);
MPI_Recv(result_PPV,MAX_DEPTH,MPI_INT,status.MPI_SOURCE,TAG_DATA,MPI_COMM_WORLD,&status);
*/

/*result->best_move = rer1.best_move;
result->pv_length = rer1.pv_length;
for(int j = 0; j < rer1.pv_length; j++) result->PV[j] = rer1.PV[j];
*/
/*
MPI_Recv(&result->best_move,1,MPI_INT,inDeuxF.rank,0,MPI_COMM_WORLD,&status);
MPI_Recv(&result->pv_length,1,MPI_INT,inDeuxF.rank,0,MPI_COMM_WORLD,&status);
MPI_Recv(&result->PV,MAX_DEPTH,MPI_INT,inDeuxF.rank,0,MPI_COMM_WORLD,&status);
*/

			/*
MPI_Send(&result_test,1,MPI_INT,0,TAG_DATA,MPI_COMM_WORLD);
MPI_Send(&result_best,1,MPI_INT,0,TAG_DATA,MPI_COMM_WORLD);
MPI_Send(&result_pvlength,1,MPI_INT,status.MPI_SOURCE,TAG_DATA,MPI_COMM_WORLD);
MPI_Send(result_PPV,MAX_DEPTH,MPI_INT,0,TAG_DATA,MPI_COMM_WORLD);
*/
		
