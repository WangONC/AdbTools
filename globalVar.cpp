#include "pch.h"
#include "AdbClient.h"


// 设备信息列表，这里有200ms左右的延时，但这个才是最需要实时的
std::vector<CString> m_devices = AdbClient::DetectDevices();

// 全局菜单项的子菜单句柄
HMENU hSubMenu = nullptr;


// adb是否可用，不需要它实时生效，这样可以节省时间，每次执行的时间在180毫秒左右
//bool adbEnable = CommandExecution::ExecCommand(_T("Get-Command adb.exe -ErrorAction SilentlyContinue")).result == 0;

//TCHAR adbPath[MAX_PATH] = _T("adb.exe");
//bool adbEnable = PathFindOnPath(adbPath, nullptr); // 这个很快，不到1毫秒，可以实时检测
bool adbEnable = false;

