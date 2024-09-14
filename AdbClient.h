#pragma once

#include "CommandExecution.h"

class AdbClient
{


public:
	AdbClient();
	static std::future<CommandExecution::CommandResult> run_async(const CString& command);
	static CString run(const CString& command);
	static std::vector<CString> DetectDevices();
	static bool InstallAPK(const CString& deviceId, std::vector<CString> apks, bool force = false);
	~AdbClient();

	struct AdbResponse {
		std::string status;
		std::string data;
	};
	bool isAdbServerRunning();
	bool startAdbServer();
	bool stopAdbServer();
	std::vector<std::string> getConnectedDevices();
	AdbResponse executeCommand(const std::string& command);
	;




private:
	static const int ADB_PORT = 5037;
	static const std::string ADB_HOST;
	WSADATA wsaData;
	bool wsaInitialized = false;




	bool initializeWsa();
	void cleanupWsa();
	SOCKET createAndConnectSocket();
	AdbResponse receiveData(SOCKET sock);
	AdbResponse receiveErrorMessage(SOCKET sock);
	bool isProcessRunning(const TCHAR* processName);
	AdbResponse sendCommand(const std::string& command);

};

