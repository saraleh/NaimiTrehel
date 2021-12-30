#include <stdio.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <stdbool.h>

#define TOKEN 1
#define REQUEST 0

struct  msg{
	int msg_type;
	struct sockaddr_in sender;
};


struct sockaddr_in pere;
struct sockaddr_in suiv;
struct sockaddr_in info;
bool demandeur = false;
bool avoir_jeton = false;
bool is_set_pere = false;
bool is_set_suiv = false;

pthread_mutex_t verrou;
pthread_cond_t cond;

void * demander_sc(void * p);
void release_sc(int * p);
void * traitement_msg(void * par);

int main(int argc, char *argv[]){
	if (argc > 5){
		printf("utilisation : %s ip_serveur port_serveur port_client ip_client\n", argv[0]);
		exit(0);
	}
	info.sin_family = AF_INET;
	inet_pton(AF_INET,"127.0.0.1",&(info.sin_addr)) ; 
	if(argc == 3 && atoi(argv[2]) == 0){
		info.sin_port = htons(atoi(argv[1]));
		is_set_pere = false;
		is_set_suiv = false;
		avoir_jeton = true;
		demandeur = false;
		
	}
	else{
		pere.sin_family = AF_INET;
		pere.sin_addr.s_addr  = inet_addr(argv[1]);
		pere.sin_port = htons((short)atoi(argv[2]));
		info.sin_port = htons ((short)atoi(argv[3]));
		inet_pton(AF_INET,argv[4],&(info.sin_addr));
		is_set_pere = true;
		is_set_suiv = false;
		avoir_jeton = false;
	}
	int ds = socket(PF_INET, SOCK_STREAM, 0);
	if (ds == -1){
		perror("Serveur : probleme creation socket");
		exit(1);
	}
	if(bind(ds, (struct sockaddr *) &info, sizeof(info)) < 0){
		perror("site : erreur bind");
		 close(ds); // je libère les ressources avant de terminer.
		 exit(1); // je choisis de quitter le programme : la suite dépend de la réussite du nommage.
		}


		printf("adresse ip : %s port : %d\n", inet_ntoa(info.sin_addr),ntohs(info.sin_port));

		listen(ds,30);
		pthread_t th1, th2;
		void * retval;
		if (pthread_create(&th2, NULL, traitement_msg, &ds) != 0){
			printf("erreur de création");
		}
		while(getchar() == 'd'){
			if (pthread_create(&th1, NULL, demander_sc, &ds) != 0){
				printf("erreur de création");
			}
			pthread_join (th1, &retval);
		}



		pthread_join (th1, &retval);
		pthread_join (th2, &retval);
	}
	void * demander_sc(void * p){
		int * ds = (int *)p;
		
		demandeur  = true;
		if(is_set_pere){
				// envoyer demande au pere
			printf("je veux envoyer une demande au pere\n");
			int dsc = socket(PF_INET, SOCK_STREAM, 0);
			if (dsc == -1){
				perror("Serveur : probleme creation socket");
				exit(1);
			}
			int conn = connect(dsc, (struct sockaddr*) &pere, sizeof(struct sockaddr_in));
			if (conn < 0){
				perror("Client: probleme deconnexion");
				close(*ds);
				exit(1);
			}
			printf("Client : demande de connexion au pere reussie \n");
			struct msg request;
			request.msg_type = REQUEST;
			request.sender = info;

			int snd = send(dsc, &request, sizeof(struct msg), 0);

		/* Traiter TOUTES les valeurs de retour (voir le cours ou la documentation). */
			if (snd < 0) {
				printf("Client: erreur d'envoi");
				close(*ds);
				exit(1);
			}

			if (snd == 0){
				printf("Client: serveur déconnecter");
				close(*ds);
				exit(1);
			}
			printf("Demande bien envoyee au pere\n");
			is_set_pere = false;
			printf("J'attends de rentrer en section critique\n");
			while(!avoir_jeton){
				pthread_cond_wait(&cond, &verrou) ;
			}
			printf("Je suis en section critique\n");
				//getchar();
				//getchar();
			release_sc(ds);
		}

		return NULL;
	}	


	//Liberation du jeton 
	void release_sc(int * p)
	{
		int * ds = (int *)p;
		if (is_set_suiv) {
			
			avoir_jeton = false; //je remets mon token a false
			
			// connexion au suivant
			int dsc = socket(PF_INET, SOCK_STREAM, 0);
			if (dsc == -1){
				perror("Serveur : probleme creation socket");
				exit(1);
			}
			int conliberation = connect(dsc, (struct sockaddr*) &suiv, sizeof(struct sockaddr_in));
			if (conliberation < 0){
				perror("Client: probleme de connexion");
				close(*ds);
				exit(1);
			}
			printf("Client : demande de connexion au suivant reussie \n");
			//envoyer le jeton au next 
			struct msg jeton;
			jeton.msg_type = TOKEN;
			jeton.sender = info;

			int snd = send(dsc, &jeton, sizeof(struct msg), 0);
			//traiter les valeurs du send 
			if (snd < 0) {
				printf("Client: erreur d'envoie");
				close(*ds);
				exit(1);
			}

			if (snd == 0){
				printf("Client: serveur déconnecter");
				close(*ds);
				exit(1);
			}
			if (snd > 0) printf("J'ai bien envoyé mon jeton au suivant\n");
			
			is_set_suiv = false;
		}
		
		demandeur = false;
		//getchar();
		//getchar();
	}



	void * traitement_msg(void * par){
		int * ds = (int *)par;
		while(1){
			int acc= accept(*ds,NULL, NULL);
			if (acc< 0)
			{ 
				perror ( "Serveur, probleme accept :");
				close(acc);
				exit (1);
			}

			struct msg message;
			int rcv = recv (acc, &message, sizeof(struct msg), 0);

			if(rcv<0){
				perror("serveur: probleme de receive:");
				close(acc);
				exit(1);
			}

			if(rcv==0){
				printf("serveur: client out of reach \n");
				close(acc);
				exit(1);
			}
			if (rcv > 0) printf("J'ai bien reçu le message\n");
			switch(message.msg_type){
				case TOKEN:
				avoir_jeton = true;
				pthread_cond_broadcast(&cond );
				break;
				case REQUEST:
				if (demandeur){
					if(!is_set_suiv){
						suiv = message.sender;
						is_set_suiv = true;
					}
				}
				else{
					if(is_set_pere){
						int dsc = socket(PF_INET, SOCK_STREAM, 0);
						if (dsc == -1){
							perror("Serveur : probleme creation socket");
							exit(1);
						}
						int conn = connect(dsc, (struct sockaddr*) &pere, sizeof(struct sockaddr_in));
						if (conn < 0){
							perror("Client: probleme de connexion");
							close(*ds);
							close(dsc);
							exit(1);
						}
						if (conn> 0 ) printf("Je suis connectee au pere \n");
						
						int snd = send(dsc, &message, sizeof(struct msg), 0);
						if (snd < 0) {
							printf("Client: erreur d'envoi");
							close(*ds);
							exit(1);
						}

						if (snd == 0){
							printf("Client: serveur déconnecte");
							close(*ds);
							exit(1);
						}
						if (snd>0) printf("J'ai bien envoyé un message au pere \n");
					}
					else if(avoir_jeton){
						avoir_jeton = false;
						int dsc = socket(PF_INET, SOCK_STREAM, 0);
						if (dsc == -1){
							perror("Serveur : probleme creation socket");
							exit(1);
						}
						int conn = connect(dsc, (struct sockaddr*) &message.sender, sizeof(struct sockaddr_in));
						if (conn < 0){
							perror("Client: probleme de connexion");
							close(dsc);
							exit(1);
						}
						struct msg token;
						token.msg_type = TOKEN;
						token.sender = info;
						int snd = send(dsc, &token, sizeof(struct msg), 0);
						if (snd < 0) {
							printf("Client: erreur d'envoie");
							close(*ds);
							exit(1);
						}

						if (snd == 0){
							printf("Client: serveur déconnecter");
							close(*ds);
							exit(1);
						}
						if (snd>0) printf("J'ai bien envoyé le jeton, je ne suis plus racine\n");				}
					}
					pere = message.sender;
					is_set_pere = true;
					break;
					default:
					printf("mauvais type de message\n");
					break;
				}

			}
			return NULL;
		}




