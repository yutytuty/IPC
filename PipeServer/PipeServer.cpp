#include <Windows.h>
#include <stdio.h>
#include <tchar.h>
#include <strsafe.h>

#define BUFSIZE 512

VOID GetAnswerToRequest(LPTSTR pchRequest, LPTSTR pchReply, LPDWORD pchBytes);
DWORD WINAPI InstanceThread(LPVOID lpvParam);

int _tmain(VOID) {
	BOOL isConnected = false;
	DWORD dwThreadId = 0;
	HANDLE hPipe = INVALID_HANDLE_VALUE, hThread = NULL;
	LPCTSTR lpszPipeName = TEXT("\\\\.\\pipe\\mynamedpipe");

	for (;;) {
		_tprintf(TEXT("\n Main Thread: Waiting for client connection.\n"));
		hPipe = CreateNamedPipe(
			lpszPipeName,             // Pipe name
			PIPE_ACCESS_DUPLEX,       // Read/Write access
			PIPE_TYPE_MESSAGE |       // Message type pipe
			PIPE_READMODE_MESSAGE |   // Message read mode
			PIPE_WAIT,                // Blocking mode
			PIPE_UNLIMITED_INSTANCES, // Unlimited instances
			BUFSIZE,                  // Output buffer size
			BUFSIZE,                  // Input buffer size
			0,                        // Client time out
			NULL                      // Default security settings
		);
		if (hPipe == INVALID_HANDLE_VALUE) {
			_tprintf(TEXT("CreateNamedPipeFailed, last error: %d.\n"), GetLastError());
			return -1;
		}

		// This blocks execution until it gets a connection
		isConnected = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

		if (isConnected) {
			printf("Client connected, creating process.\n");

			hThread = CreateThread(
				NULL,
				0,
				InstanceThread,
				(LPVOID)hPipe,
				0,
				&dwThreadId
			);

			if (hThread == NULL) {
				_tprintf(TEXT("CreateThread failed. Error: %d\n"), GetLastError());
				return -1;
			}
			else CloseHandle(hThread);
		}
		else
			CloseHandle(hPipe);
	}

	return 0;
}

DWORD WINAPI InstanceThread(LPVOID lpvParam) {
	HANDLE hHeap = GetProcessHeap();
	TCHAR* pchRequest = (TCHAR*)HeapAlloc(hHeap, 0, BUFSIZE * sizeof(TCHAR));
	TCHAR* pchReply = (TCHAR*)HeapAlloc(hHeap, 0, BUFSIZE * sizeof(TCHAR));

	DWORD cbBytesRead = 0, cbReplyBytes = 0, cbWritten = 0;
	BOOL fSuccess = FALSE;
	HANDLE hPipe = NULL;

	// Error checking
	if (lpvParam == NULL) {
		printf("Pipe server faliure:\n");
		printf("\t-Instance thread got an unexpected null character in lpvParam.\n");
		printf("\t-Instance thread exiting.\n");
		if (pchReply != NULL) HeapFree(hHeap, 0, pchReply);
		if (pchRequest != NULL) HeapFree(hHeap, 0, pchRequest);
		return (DWORD)-1;
	}

	if (pchRequest == NULL) {
		printf("Pipe server failure:\n");
		printf("\t-Instance thread got an unexpected NULL heap allocation.\n");
		printf("\t-Instance thread exiting.\n");
		if (pchReply != NULL) HeapFree(hHeap, 0, pchReply);
		return (DWORD)-1;
	}

	if (pchReply == NULL) {
		printf("Pipe server faliure:\n");
		printf("\t-Instance thread got an unexpected NULL heap allocation.\n");
		printf("\t-Instance thread exiting.\n");
		if (pchRequest != NULL) HeapFree(hHeap, 0, pchRequest);
	}

	printf("Instance thread created, recieving and processing messages.\n");

	hPipe = (HANDLE)lpvParam;

	while (1) {
		fSuccess = ReadFile(
			hPipe,                   // Handle to pipe 
			pchRequest,              // Buffer to receive data 
			BUFSIZE * sizeof(TCHAR), // Size of buffer 
			&cbBytesRead,            // Number of bytes read 
			NULL                     // Non overlapping I/O
		);

		if (!fSuccess || cbBytesRead == 0) {
			if (GetLastError() == ERROR_BROKEN_PIPE) {
				_tprintf(TEXT("Instance thread read file failed: Client disconnected.\n"));
			}
			else {
				_tprintf(TEXT("Instance thread read file failed: %d.\n"), GetLastError());
			}
			break;
		}
		GetAnswerToRequest(pchRequest, pchReply, &cbReplyBytes);

		fSuccess = WriteFile(
			hPipe,
			pchReply,
			cbReplyBytes,
			&cbWritten,
			NULL
		);

		if (!fSuccess || cbReplyBytes != cbWritten) {
			_tprintf(TEXT("Instance thread WriteFile failed. Error: %d\n"), GetLastError());
			break;
		}
	}

	// Flush the pipe to allow the client to read the pipe's contents before exiting.
	// Then disconnect the pipe and close the handle to the pipe.
	FlushFileBuffers(hPipe);
	DisconnectNamedPipe(hPipe);
	CloseHandle(hPipe);

	HeapFree(hHeap, 0, pchRequest);
	HeapFree(hHeap, 0, pchReply);

	printf("Instance thread exiting.\n");
	return 1;
}

VOID GetAnswerToRequest(LPTSTR pchRequest, LPTSTR pchReply, LPDWORD pchBytes) {
	_tprintf(TEXT("Client request string:\"%s\"\n"), pchRequest);
	if (FAILED(StringCchCopy(pchReply, BUFSIZE, TEXT("Default answer from server")))) {
		*pchBytes = 0;
		pchReply[0] = 0;
		printf("StringCchCopy failed, no outgoing message.\n");
		return;
	}
	*pchBytes = (lstrlen(pchReply) + 1) * sizeof(TCHAR);
}