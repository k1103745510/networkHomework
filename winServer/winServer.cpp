#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>

#pragma comment( lib,"ws2_32.lib" )
#include <winsock2.h>

#define IP			"127.0.0.1"
#define PORT		3000
#define BUFSIZE		100
#define NAMESIZE	20

char name[ NAMESIZE ] = "[Server]";

unsigned int  WINAPI ClientConn( void *arg );
unsigned int  WINAPI WriteAndSend( void *arg );
void SendMSG( char* message, int len );
void ExitWithError( char *message );
void ExitWithCloseSock( char *message );

char sendMessage[ BUFSIZE ];

int numberOfClient = 0;
SOCKET sockForClient[ 10 ];
HANDLE hMutex;

HANDLE hConsole;
CONSOLE_CURSOR_INFO	ConsoleCursor;

COORD insertPos = { 0, 2 };

int main( )
{
	WSADATA wsaData;
	SOCKET serverSock;
	SOCKET clientSock;
	
	struct sockaddr_in serverAddr;
	struct sockaddr_in clientAddr;
	int clientAddrSize = sizeof( clientAddr );

	HANDLE hThread;
	DWORD dwThreadID;


	
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE); // �ڵ��� ���ϰ�

    ConsoleCursor.bVisible = true; // true ���� , false �Ⱥ���
    ConsoleCursor.dwSize = 1; // Ŀ�� ������

    SetConsoleCursorInfo(hConsole , &ConsoleCursor); // ����

	if( WSAStartup( MAKEWORD( 2, 2 ), &wsaData ) != 0 )
		ExitWithError("WSAStartup() error!");

	hMutex = CreateMutex( NULL, FALSE, NULL );
	if( hMutex == NULL )
		ExitWithError("CreateMutex() error");

	serverSock = socket( PF_INET, SOCK_STREAM, 0 );   
	if( serverSock == INVALID_SOCKET )
		ExitWithError("socket() error");
 
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr( IP );
	serverAddr.sin_port = htons( PORT );
	int bindResult = bind( serverSock, ( struct sockaddr* ) &serverAddr, sizeof( serverAddr ));
	
	if( bindResult == SOCKET_ERROR )
	{
		ExitWithError("bind() error");
	}

	if( listen( serverSock, 1 ) == SOCKET_ERROR )
		ExitWithError("listen() error");

	while( true )
	{
		clientSock = accept(
			serverSock,
			( struct sockaddr* ) &clientAddr,
			&clientAddrSize );
		// Ŭ���̾�Ʈ�� ������ ������ ��ٸ� .

		if( clientSock == INVALID_SOCKET )
			ExitWithError("accept() error");

		WaitForSingleObject( hMutex, INFINITE );
		sockForClient[ numberOfClient++ ] = clientSock;
		ReleaseMutex( hMutex );

		printf("���ο� ����, Ŭ���̾�Ʈ IP : %s \n", inet_ntoa( clientAddr.sin_addr ));

		hThread = (HANDLE) _beginthreadex(
			NULL, 0,
			ClientConn,
			(void*)clientSock,
			0,
			(unsigned *)&dwThreadID );
		if(hThread == 0) 
			ExitWithError("������ ���� ����");
	
		hThread = (HANDLE) _beginthreadex(
			NULL, 0,
			WriteAndSend,
			(void*) clientSock,
			0,
			(unsigned *)&dwThreadID );
		if(hThread == 0) 
			ExitWithError("������ ���� ����");
	}
	
	// ��� ���� Ŭ����
	for( int i = 0; i < numberOfClient; ++i )
		closesocket( sockForClient[i] );

	getchar( );

	return 0;
}

unsigned int WINAPI ClientConn(void *arg)
{
	SOCKET clientSock = (SOCKET) arg;
	int strLen = 0;
	char message[ BUFSIZE ];
	int i;

	while(( strLen = recv( clientSock, message, BUFSIZE, 0 )) != 0 )
	{
		for( i = 0; i < strLen; ++i )
		{
			printf_s("%c", message[i]);
		}
	}
	
	printf_s("End\n");

	WaitForSingleObject( hMutex, INFINITE );

	for( i = 0; i < numberOfClient; i++ )
	{   
		if(clientSock == sockForClient[i])
		{
		for( ; i < numberOfClient - 1; i++ )
			sockForClient[ i ] = sockForClient[ i + 1 ];
			break;
		}
	}

	numberOfClient--;

	ReleaseMutex( hMutex );
	closesocket( clientSock );

	return 0;
}

unsigned int  WINAPI WriteAndSend( void *arg )
{
	SOCKET sock = (SOCKET) arg;
	char nameMessage[ NAMESIZE + BUFSIZE ];

	while( true ) 
	{
		fgets( sendMessage, BUFSIZE, stdin );
		sprintf_s( nameMessage,"%s %s", name, sendMessage );
		if( !strcmp( sendMessage, "q\n" )) // 'q' �Է½� ����
		{  
			closesocket( sock );
			exit(0);   
		}
		send( sock, nameMessage, strlen( nameMessage ), 0 );
	}
}

void ExitWithError( char *message )
{
	fputs( message, stderr );
	fputc('\n', stderr);
	exit( 1 );
}

void ExitWithCloseSock( char* message )
{
	for( int i = 0; i < numberOfClient; ++i )
		closesocket( sockForClient[ i ] );

	ExitWithError( message );
}