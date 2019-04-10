#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <string.h>

#define MAX_CLIENTS 5
#define BUFLEN 256

typedef struct {
    char nume[13];
    char prenume[13];
    char numar_card[7];
    char pin[5];
    char parola[9];
    double sold;
    char isBlocked;
    char greseli;
    char isLogged;
    int sfd;
    double toTransfer;
    int nr_cont_transfer;
} TUser;

typedef struct {
    int sfd;
    int cont;
} TPereche;

TUser* readInputFile(char *name,int *n) {

    int no;
    FILE *fp = fopen(name, "rt");

    fscanf(fp,"%d",&no);
    *n = no;

    TUser* users = malloc(no*(sizeof(TUser)));

    for(int i=0;i<no;i++) {
        fscanf(fp,"%s",users[i].nume);
        fscanf(fp,"%s",users[i].prenume);
        fscanf(fp,"%s",users[i].numar_card);
        fscanf(fp,"%s",users[i].pin);
        fscanf(fp,"%s",users[i].parola);
        fscanf(fp,"%lf",&users[i].sold);
        users[i].isBlocked = 0;
        users[i].isLogged = 0;
        users[i].greseli = 3;
        users[i].sfd = -1;
        users[i].toTransfer = 0;
        users[i].nr_cont_transfer = -1;

    }

    return users;

}

void toStringUser(TUser u) {


    printf("nume: %s, prenume: %s, card: %s, pin: %s, parola: %s, sold: %.2lf\n",u.nume,u.prenume,u.numar_card,u.pin,u.parola,u.sold);
}



int main(int argc, char *argv[])
{
    
    int user_no,fdmax;
    TUser *users = readInputFile(argv[2],&user_no);
    char *waiting_response = calloc(user_no,sizeof(char));
    char *waiting_response_Unlock = calloc(user_no,sizeof(char));
    char buffer[1500];

    fd_set read_fds,tmp_fds;
    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);

    struct sockaddr_in serv_addr,cli_addr;
    serv_addr.sin_family = AF_INET,
    serv_addr.sin_port = atoi(argv[1]);
    inet_aton("127.0.0.1",&(serv_addr.sin_addr));
    char should_close = 0;

    //read users
    for(int i=0;i<user_no;i++) {
        toStringUser(users[i]);
    }

    //TCP iBank socket
    int sfd_bank = socket (PF_INET,SOCK_STREAM,0);

    //UDP Unlock socket
    int sfd_unlock = socket(PF_INET,SOCK_DGRAM,0);

    //bind
    bind(sfd_bank,(struct sockaddr *)&serv_addr,sizeof(serv_addr));      //TCP
    bind(sfd_unlock,(struct sockaddr *)&serv_addr,sizeof(serv_addr));    //UDP

    listen(sfd_bank,5);

    FD_SET(sfd_bank, &read_fds);
    FD_SET(sfd_unlock, &read_fds);
    FD_SET(STDIN_FILENO,&read_fds);


    //stabileste maximul
    if(sfd_bank < STDIN_FILENO) {
        fdmax = STDIN_FILENO;
    } else {
        fdmax = sfd_bank;
    }

    if(sfd_unlock > fdmax) {
        fdmax = sfd_unlock;
    } 
    fdmax++;
    while(1) {

        tmp_fds = read_fds; 
        int clienti_conectati = 0;
        for(int i=0;i<fdmax;i++) {

            if(FD_ISSET(i, &read_fds) && i!=sfd_unlock && i!=STDIN_FILENO && i!=sfd_bank) {
                // printf("Client conectat: %d\n",i);
                clienti_conectati++;
            }
        }

        if(clienti_conectati==0 && should_close==1) {
            return 0;
        }

        select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);


        for(int i = 0; i <= fdmax; i++) {

            if (FD_ISSET(i, &tmp_fds)) {


                if(i==sfd_unlock) {
                    unsigned int size;
                    recvfrom(i, buffer, BUFLEN, 0, (struct sockaddr*)&cli_addr, &size);
                    char raspuns[50];
                    char card_to_unlock[10];
                    char *p = buffer;
                    int isParola = 1;

                    while(*p!=' ' && *p!='\n') {
                        p++;
                    }

                    if(buffer[0]=='u') {
                        isParola = 0;
                    }

                    if(isParola==0) {
                        p++;
                        strcpy(card_to_unlock,p);

                        int check = -1;
                        for(int j=0;j<user_no;j++) {
                            if(strcmp(users[j].numar_card,card_to_unlock)==0) {
                                check = j;
                                break;
                            }
                        }

                        if(check == -1) {
                            sprintf(raspuns,"-4");
                        } else {
                            if(users[check].isBlocked==0) {
                                sprintf(raspuns,"-6");
                            } else {
                                sprintf(raspuns,"Trimite parola secreta\n");
                                waiting_response_Unlock[check] = 1;
                            }
                        }

                        sendto(i,raspuns,50, 0, (struct sockaddr*)&cli_addr, sizeof(cli_addr));
                    } else {

                        int check = -1;
                        char raspuns[50];

                        char card[7];
                        char parola[10];
                        strncpy(card,buffer,6);
                        strcpy(parola,buffer+7);


                        printf("card:%s parola:%s\n",card,parola);

                        for(int j=0;j<user_no;j++) {
                            if(waiting_response_Unlock[j]==1 && strcmp(users[j].numar_card,card)==0) {
                                check = j;
                                break;
                            }
                        }

                        // printf("%d\n",check);
                        if(check==-1) {
                            // never here 
                            printf("Eroare\n");
                        } else {
                            // buffer[strlen(buffer)-1] = 0;

                            if(strcmp(users[check].parola,parola)==0) {
                                printf("Parola corecta. Deblocare cont\n");
                                waiting_response_Unlock[check] = 0;
                                users[check].isBlocked = 0;
                                users[check].greseli = 3;
                                sprintf(raspuns,"Client deblocat\n");

                            } else {
                                printf("Parola gresita\n");
                                waiting_response_Unlock[check] = 0;
                                sprintf(raspuns,"-7");
                            }
                        }
                        sendto(i,raspuns,50, 0, (struct sockaddr *)&cli_addr, sizeof(cli_addr));
                    }
                }


                else if(i==STDIN_FILENO) {
                    char buf[250];
                    scanf("%s",buf);
                    printf("Ai scris: %s\n",buf);

                    if(strcmp("quit",buf)==0) {
                        should_close = 1;
                    } else {
                        should_close = 0;
                    }
                    
                }


                else if (i == sfd_bank) {
                    // a venit ceva pe socketul inactiv(cel cu listen) = o noua conexiune
                    // actiunea serverului: accept()
                    int newsockfd;
                    unsigned int clilen = 0;
                    if ((newsockfd = accept(sfd_bank, (struct sockaddr *)&cli_addr, &clilen)) == -1) {
                        error("ERROR in accept");
                    } 
                    else {
                        //adaug noul socket intors de accept() la multimea descriptorilor de citire
                        FD_SET(newsockfd, &read_fds);
                        if (newsockfd >= fdmax) { 
                            fdmax = newsockfd+1;
                        }
                    }
                    printf("Noua conexiune de la %s, port %d, socket_client %d\n ", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), newsockfd);
                }
                    
                else {
                    // am primit date pe unul din socketii cu care vorbesc cu clientii
                    //actiunea serverului: recv()
                    memset(buffer, 0, BUFLEN);
                    int n;
                    if ((n = recv(i, buffer, sizeof(buffer), 0)) <= 0) {
                        if (n == 0) {
                            //conexiunea s-a inchis
                            printf("selectserver: socket %d hung up\n", i);
                        } else {
                            error("ERROR in recv");
                        }
                        close(i); 
                        FD_CLR(i, &read_fds); // scoatem din multimea de citire socketul pe care 
                    } 
                    
                    else { //recv intoarce >0

                        printf ("Am primit de la clientul de pe socketul %d, mesajul: %s\n", i, buffer);

                        //login command
                        if(strncmp(buffer,"login ",6)==0) {
                            char nr_card[7];
                            char pin[5];
                            char *p;
                            char raspuns[50];
                            int count1,count2;

                            p = buffer;

                            while(*p!=' ') {
                                p++;
                            }
                            p++;

                            strncpy(nr_card,p,6);

                            count1 = 0;
                            while(*p!=' ') {
                                p++;
                                count1++;
                            }
                            p++;


                            strncpy(pin,p,4);
                            count2=0;
                            while(*p!='\n') {
                                count2++;
                                p++;
                            }

                            // printf("%s %s\n",nr_card,pin);

                            int check = -1;
                            for(int j=0;j<user_no;j++) {
                                if(strncmp(users[j].numar_card,nr_card,6)==0) {
                                    check = j;
                                    break;
                                }
                            }

                            if(count1 - 6 > 0) {
                                check = -1;
                            }

                            if(check > -1) {

                                if(users[check].isBlocked == 1) {
                                    printf("Card blocked\n");
                                    sprintf(raspuns,"-5");
                                }

                                else if(strncmp(users[check].pin,pin,4)==0 && count2==4) {

                                    if(users[check].isLogged == 0) {
                                        users[check].isLogged = 1;
                                        users[check].sfd = i;
                                        printf("Logging in succesfull\n");
                                        sprintf(raspuns,"Welcome %s %s!\n",users[check].nume,users[check].prenume);
                                    } else {
                                        printf("Already logged\n");
                                        sprintf(raspuns,"-2");
                                    }

                                } else {

                                    users[check].greseli--;
                                    if(users[check].greseli > 0) {
                                        printf("Incorrect pin\n");
                                        sprintf(raspuns,"-3");
                                    } else {
                                        users[check].isBlocked = 1;
                                        users[check].greseli = 0;
                                        printf("Cont blocat\n");
                                        sprintf(raspuns,"-5");
                                    }

                                    // printf("Incorrect pin\n");
                                    // sprintf(raspuns,"-3");
                                }

                            } else {
                                printf("No such card number\n");
                                sprintf(raspuns,"-4");
                            }

                            printf("here\n");
                            send(i,raspuns,strlen(raspuns)+1,0);
                            
                        }

                        else if(strncmp(buffer,"logout",6)==0) {
                            // char raspuns[50];


                            for(int j=0;j<user_no;j++) {
                                if(users[j].sfd == i) {
                                    users[j].isBlocked = 0;
                                    users[j].isLogged = 0;
                                    users[j].greseli = 3;
                                    users[j].sfd = -1;
                                    break;
                                }
                            }
                            printf("Deconectare de la bancomat\n");

                        } else if(strncmp(buffer,"listsold",8)==0) {

                            char raspuns[50];
                            int j;
                            for(j=0;j<user_no;j++) {
                                if(users[j].sfd == i) {
                                    sprintf(raspuns,"%.2lf",users[j].sold);
                                    break;
                                }
                            }

                            printf("Cerere sold: %.2lf\n",users[j].sold);
                            send(i,raspuns,strlen(raspuns)+1,0);
                            
                        } else if(strncmp(buffer,"transfer",8)==0) {
                            char raspuns[50];
                            char card[7];
                            int count_card = 0;
                            char *p = buffer;
                            char *cop;
                            double suma;

                            while(*p!=' ') {
                                p++;
                            }
                            p++;
                            cop = p;
                            while(*cop!=' ') {
                                count_card++;
                                cop++;
                            }
                            cop++;

                            cop[strlen(cop)-1] = 0;
                            suma = atof(cop);
                            strncpy(card, p,6);
                            card[6] = 0;

                            printf("%s %.2lf\n",card,suma);

                            int check = -1;
                            for(int j=0;j<user_no;j++) {
                                if(strncmp(users[j].numar_card,card,6)==0) {
                                    check = j;
                                    break;
                                }
                            }

                            int current_client;
                            for(int j=0;j<user_no;j++) {
                                if(i==users[j].sfd) {
                                    current_client = j;
                                    break;
                                }
                            }

                            // printf("count:%d\n",count_card);

                            if(check > -1 && count_card==6) {

                                if(users[current_client].sold < suma) {
                                    printf("Not enough cash\n");
                                    sprintf(raspuns,"-8");
                                } else {

                                    sprintf(raspuns,"Transfer %.2f to %s %s? [y/n]\n",suma,users[check].nume,users[check].prenume);
                                    waiting_response[current_client] = 1;
                                    users[current_client].toTransfer = suma;
                                    users[current_client].nr_cont_transfer = check;
                                    // printf("Incorrect pin\n");
                                    // sprintf(raspuns,"-3");
                                }

                            } else {
                                printf("No such card number\n");
                                sprintf(raspuns,"-4");
                            }

                            send(i,raspuns,strlen(raspuns)+1,0);
                        } else if(strcmp(buffer,"quit\n")==0) {
                            printf("Client will be closing\n");
                            FD_CLR(i,&read_fds);
                            close(i);
                        }else if(strcmp(buffer,"serverStatusForClosing")==0) {
                            sprintf(buffer,"%d",should_close);
                            send(i,buffer,strlen(buffer)+1,0);

                        } else {

                            char raspuns[50];
                            int j;
                            for(j=0;j<user_no;j++) {
                                if(i==users[j].sfd) {
                                    break;
                                }
                            }

                            if(waiting_response[j]==1) {

                                if(strcmp(buffer,"y\n")==0) {
                                    waiting_response[j] = 0;
                                    printf("Transfer realizat cu succes\n");
                                    sprintf(raspuns,"Transfer realizat cu succes\n");

                                    int user_target = users[j].nr_cont_transfer;
                                    users[user_target].sold += users[j].toTransfer;
                                    users[j].sold -= users[j].toTransfer;


                                    users[j].toTransfer = 0;
                                    users[j].nr_cont_transfer = -1;

                                    send(i,raspuns,strlen(raspuns)+1,0);
                                    continue;

                                } else {
                                    //operatie anulata
                                    sprintf(raspuns,"-9");
                                    waiting_response[j] = 0;
                                    users[j].toTransfer = 0;
                                    users[j].nr_cont_transfer = -1;
                                    send(i,raspuns,strlen(raspuns)+1,0);
                                    continue;
                                }
                            } else {
                                send(i,"-6",3,0);
                            }
                        } 
                    }
                } 
            } // if is set
        } // iterare sfd
    } //while(1)
} //main