#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include "chat.h"

#define DEBUG

// 최대 클라이언트 수, ID 길이, 버퍼 크기 정의
#define MAX_CLIENT   5
#define MAX_ID      32
#define MAX_BUF     256

// 클라이언트 정보를 저장하는 구조체
typedef struct  {
    int         sockfd;     // 클라이언트 소켓
    int         inUse;      // 사용 중인지 여부
    pthread_t   tid;        // 클라이언트 처리 스레드 ID
    char        uid[MAX_ID];// 사용자 ID
}
    ClientType;

// 서버 소켓과 뮤텍스 전역 변수
int             Sockfd;
pthread_mutex_t Mutex;

// 클라이언트 정보 배열
ClientType      Client[MAX_CLIENT];


// 새로운 클라이언트에게 할당할 수 있는 ID를 찾는 함수
int
GetID()
{
    int i;

    for (i = 0 ; i < MAX_CLIENT ; i++)  {
        if (! Client[i].inUse)  {
            Client[i].inUse = 1;
            return i;
        }
    }
}

// 메시지를 보낸 클라이언트를 제외한 모든 클라이언트에게 메시지 전송
void
SendToOtherClients(int id, char *buf)
{
    int     i;
    char    msg[MAX_BUF+MAX_ID];

    // 메시지 형식 지정 (사용자ID> 메시지)
    sprintf(msg, "%s> %s", Client[id].uid, buf);
#ifdef DEBUG
    printf("%s", msg);
    fflush(stdout);
#endif

    // 다른 클라이언트들에게 메시지 전송 (뮤텍스로 보호)
    pthread_mutex_lock(&Mutex);
    for (i = 0 ; i < MAX_CLIENT ; i++)  {
        if (Client[i].inUse && (i != id))  {
            if (send(Client[i].sockfd, msg, strlen(msg)+1, 0) < 0)  {
                perror("send");
                exit(1);
            }
        }
    }
    pthread_mutex_unlock(&Mutex);
}

// 각 클라이언트를 처리하는 스레드 함수
void
ProcessClient(int id)
{
    char    buf[MAX_BUF];
    int     n;

    // 스레드 취소 설정
    if (pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL))  {
        perror("pthread_setcancelstate");
        exit(1);
    }
    if (pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL))  {
        perror("pthread_setcanceltype");
        exit(1);
    }

    // 클라이언트로부터 사용자 ID 받기
    if ((n = recv(Client[id].sockfd, Client[id].uid, MAX_ID, 0)) < 0)  {
        perror("recv");
        exit(1);
    }
    printf("Client %d log-in(ID: %s).....\n", id, Client[id].uid);

    // 클라이언트의 메시지 수신 및 처리
    while (1)  {
        if ((n = recv(Client[id].sockfd, buf, MAX_BUF, 0)) < 0)  {
            perror("recv");
            exit(1);
        }
        // 클라이언트 연결 종료 처리
        if (n == 0)  {
            printf("Client %d log-out(ID: %s).....\n", id, Client[id].uid);

            pthread_mutex_lock(&Mutex);
            close(Client[id].sockfd);
            Client[id].inUse = 0;
            pthread_mutex_unlock(&Mutex);

            strcpy(buf, "log-out.....\n");
            SendToOtherClients(id, buf);

            pthread_exit(NULL);
        }

        SendToOtherClients(id, buf);
    }
}

// 서버 종료 시그널 핸들러 (Ctrl+C)
void
CloseServer(int signo)
{
    int     i;

    close(Sockfd);

    // 모든 클라이언트 스레드 종료 및 정리
    for (i = 0 ; i < MAX_CLIENT ; i++)  {
        if (Client[i].inUse)  {
            if (pthread_cancel(Client[i].tid))  {
                perror("pthread_cancel");
                exit(1);
            }
            if (pthread_join(Client[i].tid, NULL))  {
                perror("pthread_join");
                exit(1);
            }
            close(Client[i].sockfd);
        }
    }
    if (pthread_mutex_destroy(&Mutex) < 0)  {
        perror("pthread_mutex_destroy");
        exit(1);
    }

    printf("\nChat server terminated.....\n");
    exit(0);
}

main(int argc, char *argv[])
{
    int                 newSockfd, cliAddrLen, id, one = 1;
    struct sockaddr_in  cliAddr, servAddr;

    // 시그널 핸들러와 뮤텍스 초기화
    signal(SIGINT, CloseServer);
    if (pthread_mutex_init(&Mutex, NULL) < 0)  {
        perror("pthread_mutex_init");
        exit(1);
    }

    // 서버 소켓 생성
    if ((Sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0)  {
        perror("socket");
        exit(1);
    }

    // 소켓 옵션 설정 (주소 재사용)
    if (setsockopt(Sockfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) < 0)  {
        perror("setsockopt");
        exit(1);
    }

    // 서버 주소 설정
    bzero((char *)&servAddr, sizeof(servAddr));
    servAddr.sin_family = PF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(SERV_TCP_PORT);

    // 소켓에 주소 바인딩
    if (bind(Sockfd, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0)  {
        perror("bind");
        exit(1);
    }

    // 연결 대기열 설정
    listen(Sockfd, 5);

    printf("Chat server started.....\n");

    // 클라이언트 연결 수락 및 처리 루프
    cliAddrLen = sizeof(cliAddr);
    while (1)  {
        newSockfd = accept(Sockfd, (struct sockaddr *) &cliAddr, &cliAddrLen);
        if (newSockfd < 0)  {
            perror("accept");
            exit(1);
        }

        // 새 클라이언트에 ID 할당
        id = GetID();
        Client[id].sockfd = newSockfd;
        
        // 클라이언트 처리를 위한 스레드 생성
        if (pthread_create(&Client[id].tid, NULL, (void *)ProcessClient, (void *)id) < 0)  {
            perror("pthread_create");
            exit(1);
        }
    }
}