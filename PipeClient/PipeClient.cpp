#include <Windows.h>
#include <stdio.h>
#include <conio.h>
#include <tchar.h>

#define BUFSIZE 512

int _tmain(int argc, TCHAR* argv[]) {
	HANDLE hPipe;
	LPCTSTR lpvMessage = TEXT("Default message from client.");
	LPTSTR chBuf[BUFSIZE];
	BOOL fSuccess = FALSE;
	DWORD cbRead, cbToWrite, cbWritten, dwMode;
	LPCTSTR lpszPipename = TEXT("\\\\.\\pipe\\mynamedpipe");

	if (argc > 1)
		lpvMessage = argv[1];

	// Try to open (and wait if neccesary) the pipe.
	while (1) {
		hPipe = CreateFile(
			lpszPipename,
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			OPEN_EXISTING,
			0,
			NULL
		);

		if (hPipe != INVALID_HANDLE_VALUE)
			break;

		// Exit if the pipe error is anything other than ERROR_PIPE_BUSY.
		if (GetLastError() != ERROR_PIPE_BUSY) {
			_tprintf(TEXT("Could not open pipe. Error: %d\n"), GetLastError());
			return -1;
		}

		// This means all pipe instances are busy, so wait 20 seconds.
		if (!WaitNamedPipe(lpszPipename, 20000)) {
			printf("Could not open pipe, 20 second wait timed out.");
			return -1;
		}
	}

	// Pipe connected, change to message read mode.
	dwMode = PIPE_READMODE_MESSAGE;
	fSuccess = SetNamedPipeHandleState(
		hPipe,    // pipe handle
		&dwMode,  // new pipe mode
		NULL,     // don't set maximum bytes
		NULL      // don't set maximum time
	);

	if (!fSuccess) {
		_tprintf(TEXT("SetPipeNamedHandle failed. Error: %d\n"), GetLastError());
		return -1;
	}

	// Send message to server
	cbToWrite = (lstrlen(lpvMessage) + 1) * sizeof(TCHAR);
	_tprintf(TEXT("Sending %d byte message: \"%s\"\n"), cbToWrite, lpvMessage);

	fSuccess = WriteFile(
		hPipe,
		lpvMessage,
		cbToWrite,
		&cbWritten,
		NULL
	);

	if (!fSuccess) {
		_tprintf(TEXT("WriteFile to pipe failed. Error: %d\n"), GetLastError());
		return -1;
	}

	printf("\nMessage sent to server, recieving reply as follows:\n");

	do {
		// Read from the pipe
		fSuccess = ReadFile(
			hPipe,
			chBuf,
			BUFSIZE * sizeof(TCHAR),
			&cbRead,
			NULL
		);

		if (!fSuccess && GetLastError() != ERROR_MORE_DATA)
			break;

		_tprintf(TEXT("\"%s\"\n"), chBuf);

	} while (!fSuccess);

	printf("\n<End of messag, press ENTER to terminate connection and exit>\n");
	int _ = _getch();

	CloseHandle(hPipe);

	return 0;
}
