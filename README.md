# 채팅 서버 README

## 개요
이 프로그램은 C언어로 구현된 간단한 다중 스레드 채팅 서버로, 여러 클라이언트가 실시간으로 연결하여 메시지를 주고받을 수 있게 해줍니다. 서버는 POSIX 스레드(pthread)를 사용하여 다수의 클라이언트 연결을 동시에 처리하며 기본적인 채팅방 기능을 제공합니다.

## 주요 기능
- 다중 클라이언트 지원 (최대 5개의 동시 연결)
- 스레드 기반 클라이언트 처리
- 고유 ID를 통한 사용자 식별
- 모든 연결된 클라이언트에게 메시지 브로드캐스팅
- 시그널 처리를 통한 우아한 서버 종료

## 기술적 세부사항
- 네트워크 통신을 위한 TCP 소켓 사용
- 병렬 클라이언트 처리를 위한 POSIX 스레드
- 스레드 동기화를 위한 뮤텍스 락
- 깔끔한 서버 종료를 위한 시그널 핸들링 (SIGINT)

## 필수 조건
- C 컴파일러 (gcc 권장)
- POSIX 호환 운영체제 (리눅스/유닉스)
- pthread 라이브러리
- 표준 소켓 라이브러리

## 컴파일 방법
채팅 서버를 컴파일하려면:
```bash
gcc -o chat_server chat_server.c -lpthread
```

"chat.h" 헤더 파일이 동일한 디렉토리에 있거나 컴파일 경로에 올바르게 포함되어 있는지 확인하세요.

## 사용 방법
1. 서버 시작:
```bash
./chat_server
```

2. 서버는 "chat.h" 헤더 파일에 정의된 포트(SERV_TCP_PORT)에서 들어오는 연결을 수신합니다.

3. 클라이언트는 서버의 IP 주소와 포트 번호를 사용하여 서버에 연결할 수 있습니다.

4. 클라이언트가 연결되면 먼저 사용자 ID를 보내야 합니다. 이후 클라이언트가 보낸 모든 메시지는 다른 모든 연결된 클라이언트에게 브로드캐스트됩니다.

## 클라이언트 프로토콜
1. 연결 시 사용자 ID 전송 (최대 32자)
2. 채팅 메시지 전송 (최대 256자)
3. 로그아웃하려면 연결 종료

## 구현 참고사항
- 서버는 최대 5개의 클라이언트를 지원합니다 (MAX_CLIENT로 정의)
- 최대 사용자 ID 길이는 32자입니다 (MAX_ID)
- 최대 메시지 길이는 256자입니다 (MAX_BUF)
- DEBUG를 정의하거나 해제하여 디버그 모드를 활성화/비활성화할 수 있습니다

## 보안 고려사항
- 인증 메커니즘이 구현되어 있지 않음
- 메시지는 평문으로 전송됨
- 클라이언트 메시지에 대한 입력 유효성 검사가 수행되지 않음

## 종료
서버는 Ctrl+C를 눌러 종료할 수 있으며, 이는 모든 리소스를 올바르게 정리하고 클라이언트에게 알립니다.