// GTAOSoloSession by Christian Kosman

#include <iostream>
#include <vector>
#include <Windows.h>
#include <TlHelp32.h>
#include <string>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <map>

#define SETTINGS_FILE "gtaoss_config.txt"

using namespace std;

map<string, string> settings;

// Sets the text color to something
void setConsoleColor(int color) {
	switch (color) {
	case 0: SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED); break;
	case 1: SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_INTENSITY); break;
	}
}

// Loads settings from gtaoss_config.txt into the settings map
void loadSettings() {
	ifstream file(SETTINGS_FILE, ifstream::in);
	if (file.good()) {
		string line;
		while (getline(file, line)) {
			if (line[0] != '/' && line[1] != '/') {
				istringstream isLine(line);
				string key;
				if (getline(isLine, key, '=')) {
					string value;
					if (getline(isLine, value)) {
						settings[key] = value;
					}
				}
			}
		}
	} else {
		file.close();
		ofstream file(SETTINGS_FILE);
		file << "hotkey=114" << endl << "sleep=15000" << endl;
		file.close();
		loadSettings();
		return;
	}

	file.close();
}

// Saves data from settings map to "gtaoss_config.txt"
void saveSettings() {
	map<string, string>::iterator it;
	string outData;
	for (it = settings.begin(); it != settings.end(); it++) {
		outData += it->first + "=" + it->second + "\n";
	}
	ofstream file(SETTINGS_FILE);
	file << outData;
	file.close();
}

// Resumes a process by its process ID
void resumePid(DWORD pid) {
	HANDLE hThreadSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);

	THREADENTRY32 threadEntry;
	threadEntry.dwSize = sizeof(THREADENTRY32);

	Thread32First(hThreadSnapshot, &threadEntry);

	do {
		if (threadEntry.th32OwnerProcessID == pid) {
			HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, false, threadEntry.th32ThreadID);
			ResumeThread(hThread);
			CloseHandle(hThread);
		}
	} while (Thread32Next(hThreadSnapshot, &threadEntry));

	CloseHandle(hThreadSnapshot);
}

// Suspends a process by its process ID
void suspendPid(DWORD pid) {
	HANDLE hThreadSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);

	THREADENTRY32 threadEntry;
	threadEntry.dwSize = sizeof(THREADENTRY32);

	Thread32First(hThreadSnapshot, &threadEntry);

	do {
		if (threadEntry.th32OwnerProcessID == pid) {
			HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, false, threadEntry.th32ThreadID);
			SuspendThread(hThread);
			CloseHandle(hThread);
		}
	} while (Thread32Next(hThreadSnapshot, &threadEntry));

	CloseHandle(hThreadSnapshot);
}

// Looks for GTA5.exe process and suspends it
// After "sleep" milliseconds, resume it
void suspendGTA(int sleep) {
	wstring processName = L"GTA5.exe";
	string processNameStr = string(processName.begin(), processName.end());
	bool foundProcess = false;

	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(entry);

	HANDLE hProcessSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnapshot == INVALID_HANDLE_VALUE) return;
	cout << "Process snapshot handle is valid.\n";

	Process32First(hProcessSnapshot, &entry);
	if (!processName.compare(entry.szExeFile)) {
		foundProcess = true;
		cout << "Found process: " << processNameStr << ".\n";
		HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, entry.th32ProcessID);
		DWORD pid = GetProcessId(hProcess);
		CloseHandle(hProcess);
		suspendPid(pid);
		cout << "Sleeping for " << sleep << " milliseconds...\n";
		Sleep(sleep);
		resumePid(pid);
	}

	while (Process32Next(hProcessSnapshot, &entry) == true) {
		if (!processName.compare(entry.szExeFile)) {
			foundProcess = true;
			cout << "Found process: " << processNameStr << ".\n";
			HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, entry.th32ProcessID);
			DWORD pid = GetProcessId(hProcess);
			CloseHandle(hProcess);
			suspendPid(pid);
			cout << "Sleeping for " << sleep << " milliseconds...\n";
			Sleep(sleep);
			resumePid(pid);
		}
	}

	if (!foundProcess) {
		setConsoleColor(1);
		cout << "Failed to find process: " << processNameStr << ".\n";
		setConsoleColor(0);
	} else {
		cout << "Successfully suspended/resumed process.\n";
	}

	CloseHandle(hProcessSnapshot);
}

// Returns true once if key has been pressed
bool KeyPressed(int* flag, int virtKey) {
	if ((GetKeyState(virtKey) & 0x8000) && (*flag == 0)) {
		*flag = 1;
		return true;
	}
	else if (GetKeyState(virtKey) == 0 || GetKeyState(virtKey) == 1) *flag = 0;
	return false;
}

void main() {
	int choice = -1;
	int k[2] = { 0, 0 };
	bool listening = false;
	loadSettings();
	int sleep = stoi(settings["sleep"]);
	int vk = stoi(settings["hotkey"]);
	while (choice != 5) {
		if (listening) {
			cout << "Press F1 to stop listening.\n";
			while (!KeyPressed(&k[1], VK_F1)) {
				if (KeyPressed(&k[0], vk)) {
					system("cls");
					suspendGTA(sleep);
					cout << "Press F1 to stop listening.\n";
				}
			}
			system("cls");
		}

		setConsoleColor(1);
		cout << "GTAOSoloSession by Triple Axis\n\n";
		setConsoleColor(0);
		cout << "Choose an action:\n";
		cout << "1. Suspend and Resume Grand Theft Auto V" << endl;
		cout << "2. Toggle hotkey listener" << endl;
		cout << "3. Set suspension length" << endl;
		cout << "4. Set hotkey" << endl;
		cout << "5. How to use" << endl;
		cout << "6. Quit program" << endl;
		cout << endl << "> ";
		cin >> choice;

		switch (choice) {
		case 1: {
			system("cls");
			suspendGTA(sleep);
			system("pause");
			break;
		}
		case 2: {
			listening = !listening;
			break;
		}
		case 3: {
			system("cls");
			cout << "Enter a wait time (in milliseconds) between suspension and resumption:\n";
			cin >> sleep;
			settings["sleep"] = to_string(sleep);
			saveSettings();
			break;
		}
		case 4: {
			system("cls");
			cout << "Enter a virtual key number:\n";
			cin >> vk;
			settings["hotkey"] = to_string(vk);
			saveSettings();
			break;
		}
		case 5: {
			system("cls");
			setConsoleColor(1);
			cout << "How to use:\n\n";
			setConsoleColor(0);
			cout << "1. Select option 2 from the GTAOSoloSession main menu\n";
			cout << "2. Join a GTA Online public session\n";
			cout << "3. Press the shortcut hotkey (default is F3)\n";
			cout << "4. The game will hang for the amount of milliseconds defined (default is 15 seconds)\n";
			cout << "5. When the game resumes, people in the session will have left leaving you alone in the session\n\n";
			system("pause");
		}
		case 6: {
			exit(0);
		}
		}
		system("cls");
	}
	system("pause");
}