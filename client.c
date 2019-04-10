
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

#define BUFLEN 200


char * errorHandler(char *arg) {

    if(strcmp(arg,"-2")==0) {
        return "-2 : Sesiune deja deschisa\n";
    }

    if(strcmp(arg,"-3")==0) {
        return "-3 : Pin gresit\n";
    }

    if(strcmp(arg,"-4")==0) {
        return "-4 : Numar card inexistent\n";
    }

    if(strcmp(arg,"-5")==0) {
        return "-5 : Card blocat\n";
    }

    if(strcmp(arg,"-6")==0) {
        return "-6 : Operatie esuata\n";
    }

    if(strcmp(arg,"-7")==0) {
        return "-7 : Deblocare esuata\n";
    }

    if(strcmp(arg,"-8")==0) {
        return "-8 : Fonduri insuficiente\n";
    }

    if(strcmp(arg,"-9")==0) {
        return "-9 : Operatie anulata\n";
    }

    return arg;
}
int main(int argc, char *argv[])
{
    // int sockfd = 0;
    char buf[BUFLEN];
    char isLogged = 0;
    struct sockaddr_in serv_addr;
    char last_login[10];
    char needs_to_answer_T = 0;
    char needs_to_answer_U = 0;

    // deschidere socket

    int sfdT = socket (PF_INET,SOCK_STREAM,0);
    int sfdU = socket (PF_INET,SOCK_DGRAM,0);


    serv_addr.sin_family = AF_INET,
    serv_addr.sin_port = atoi(argv[2]);

    if(inet_aton(argv[1],&(serv_addr.sin_addr))<0) {
        printf("inet aton problema\n");
        return -1;
        
    }

    // conectare la server

    if(connect(sfdT,(struct sockaddr *)&serv_addr, sizeof(serv_addr))<0) {
        printf("Connect problem-incearca alt port\n");
        return -1;
    }

    // deschidere fisier log

    int pid = getpid();
    char file[15];
    sprintf(file,"client-%d.log",pid);
    FILE *f = fopen(file,"wt");

    // citire de la tastatura, trimitere de cereri catre server, asteptare raspuns

    system("clear");
    while(1) {
        // scanf("%s",buf);
        // check server status
        sprintf(buf,"serverStatusForClosing");
        send(sfdT,buf,BUFLEN,0);
        recv(sfdT,buf,BUFLEN,0);

        if(strcmp(buf,"1")==0) {
            printf("Serverul se va inchide, iar operatiile nu mai pot fi continuate\n");
            break;
        }

        fgets(buf,BUFLEN,stdin);

        //login command
        if(strncmp(buf,"login ",6)==0) {

            //get card number for last login

            char *p = buf+6;
            int index = 0;
            while(*p!=' ') {
                last_login[index] = *p;
                p++;
                index++;
                if(index > 8) {
                    break;
                }
            }
            last_login[index] = 0;


            if(isLogged==1) {

                char bufcop[BUFLEN];
                strcpy(bufcop,buf);
                bufcop[strlen(bufcop)-1] = 0;
                fprintf(f,"%s\n",bufcop);
                fprintf(f,"IBANK> −2 : Sesiune deja deschisa\n");
                printf("IBANK> −2 : Sesiune deja deschisa\n");
                
            } else {

                char bufcop[BUFLEN];
                strcpy(bufcop,buf);
                bufcop[strlen(bufcop)-1] = 0;

                fprintf(f,"%s\n",bufcop);
                send(sfdT,buf,strlen(buf)+1,0);
                recv(sfdT,buf,BUFLEN,0);

                printf("IBANK> %s\n",errorHandler(buf));
                fprintf(f,"IBANK> %s",errorHandler(buf));

                if(strncmp(buf,"Welcome",7)==0) {
                    isLogged = 1;
                }
            }
        } else if(strncmp(buf,"logout\n",7)==0) {


            char bufcop[BUFLEN];
            strcpy(bufcop,buf);
            bufcop[strlen(bufcop)-1] = 0;
            fprintf(f,"%s\n",bufcop);

            if(isLogged==1) {
                send(sfdT,buf,strlen(buf)+1,0);
                printf("IBANK> Clientul a fost deconectat\n");
                fprintf(f,"IBANK> Clientul a fost deconectat\n");
                isLogged = 0;
            } else {
                printf("−1 : Clientul nu este autentificat\n");
                fprintf(f,"−1 : Clientul nu este autentificat\n");
            }
        } else if(strncmp(buf,"listsold\n",9)==0) {

            if(isLogged==0) {
                printf("−1 : Clientul nu este autentificat\n");
                fprintf(f,"−1 : Clientul nu este autentificat\n");
            } else {

                char bufcop[BUFLEN];
                strcpy(bufcop,buf);
                bufcop[strlen(bufcop)-1] = 0;
                fprintf(f,"%s\n",bufcop);

                send(sfdT,buf,strlen(buf)+1,0);
                recv(sfdT,buf,BUFLEN,0);

                printf("IBANK> %s\n",buf);
                fprintf(f,"IBANK> %s\n",buf);
            }

        } else if(strncmp(buf,"transfer ",9)==0) {

            if(isLogged==0) {
                printf("−1 : Clientul nu este autentificat\n");
                fprintf(f,"−1 : Clientul nu este autentificat\n");
            } else {
                char bufcop[BUFLEN];
                strcpy(bufcop,buf);
                bufcop[strlen(bufcop)-1] = 0;

                fprintf(f,"%s\n",bufcop);
                send(sfdT,buf,strlen(buf)+1,0);
                recv(sfdT,buf,BUFLEN,0);

                if(strlen(buf) > 3) {
                    needs_to_answer_T = 1;
                }

                printf("IBANK> %s\n",errorHandler(buf));
                fprintf(f,"IBANK> %s",errorHandler(buf));

            }

        } else if(strncmp(buf,"unlock\n",7)==0) {

            unsigned int size;
            buf[strlen(buf)-1] = 0;
            char buf_cop[BUFLEN];
            sprintf(buf_cop,"%s %s",buf,last_login);
            fprintf(f,"%s\n",buf);
            sendto(sfdU,buf_cop,BUFLEN, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
            recvfrom(sfdU, buf, BUFLEN, 0, (struct sockaddr *)&serv_addr, &size);

            needs_to_answer_U = 1;
            fprintf(f,"UNLOCK> %s",errorHandler(buf));
            printf("UNLOCK> %s\n",errorHandler(buf));


        } else if(strncmp(buf,"quit\n",5)==0) {
            if(isLogged==1) {
                printf("Log out first!\n");
                continue;
            }
            fprintf(f,"%s",buf);
            send(sfdT,buf,strlen(buf)+1,0);

            break;

        } else if(needs_to_answer_T==1){
            // printf("ERROR : Unknown command\n");
            needs_to_answer_T = 0;
            fprintf(f,"%s",buf);
            send(sfdT,buf,strlen(buf)+1,0);
            recv(sfdT,buf,BUFLEN,0);
            printf("IBANK> %s\n",errorHandler(buf));
            fprintf(f,"IBANK> %s",errorHandler(buf));

        } else if(needs_to_answer_U==1) {
            unsigned int size;
            needs_to_answer_U = 0;
            buf[strlen(buf)-1] = 0;

            char buf_cop[BUFLEN];
            fprintf(f,"%s\n",buf);
            sprintf(buf_cop,"%s %s",last_login,buf);

            sendto(sfdU,buf_cop,BUFLEN, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
            recvfrom(sfdU, buf, BUFLEN, 0, (struct sockaddr *)&serv_addr, &size);

            printf("UNLOCK> %s\n",errorHandler(buf));
            fprintf(f,"UNLOCK> %s",errorHandler(buf));
        }


        else {
            printf("ERROR : Unknown command\n");
        }


        // check server status
        // printf("before\n");
        // recv(sfdT,buf,BUFLEN,0);
        // printf("after\n");
        // if(strcmp(buf,"0")==0) {
        //     printf("Serverul se va inchide, iar operatiile nu mai pot fi continuate\n");
        //     break;
        // }
    }


    // inchidere socket

    close(sfdT);
    close(sfdU);
    fclose(f);
    return 0;
}
