// APKInstaller.cpp: CAPKInstaller 的实现

#include "pch.h"
#include "AdbClient.h"
#include "APKInstaller.h"
#include <ShlObj.h>
#include <strsafe.h>
#include "globalVar.h"


// CAPKInstaller

STDMETHODIMP CAPKInstaller::Initialize(PCIDLIST_ABSOLUTE pidlFolder, IDataObject* pdtobj, HKEY hkeyProgID)
{
    FORMATETC fmt = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM stg = { TYMED_HGLOBAL };
    if (SUCCEEDED(pdtobj->GetData(&fmt, &stg)))
    {
        HDROP hDrop = static_cast<HDROP>(GlobalLock(stg.hGlobal));
        if (hDrop != NULL)
        {
            UINT uCount = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
            for (int i = 0; i < uCount; i++)
            {
                TCHAR szFile[MAX_PATH];
                if (DragQueryFile(hDrop, i, szFile, MAX_PATH))
                {
                    m_szFiles.push_back(szFile);
                }
            }
            GlobalUnlock(stg.hGlobal);
        }
        ReleaseStgMedium(&stg);
    }
    return S_OK;
}

STDMETHODIMP CAPKInstaller::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    if (uFlags & CMF_DEFAULTONLY)
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, USHORT(0));

    if (!adbEnable) //检查adb是否可用
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, USHORT(0));


    UINT idCmd = idCmdFirst;
    UINT position = indexMenu;


    if (!m_devices.empty())
    {
        // 创建 "Install APK to..." 子菜单
        HMENU d_hSubMenu = CreatePopupMenu();
        for (const auto& device : m_devices)
        {
            InsertMenu(d_hSubMenu, -1, MF_STRING | MF_BYPOSITION, idCmd++, device);
        }
        InsertMenu(hSubMenu, position++, MF_POPUP | MF_BYPOSITION, (UINT_PTR)d_hSubMenu, _T("Install APK to..."));

        // 创建 "Force Install APK to..." 子菜单
        HMENU f_hSubMenu = CreatePopupMenu();
        for (const auto& device : m_devices)
        {
            InsertMenu(f_hSubMenu, -1, MF_STRING | MF_BYPOSITION, idCmd++, device);
        }
        InsertMenu(hSubMenu, position++, MF_POPUP | MF_BYPOSITION, (UINT_PTR)f_hSubMenu, _T("Force Install APK to..."));

    }
    else
    {
        InsertMenu(hSubMenu, position++, MF_STRING | MF_BYPOSITION | MF_GRAYED, idCmd++, _T("No devices detected"));
    }

    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, USHORT(idCmd - idCmdFirst));

}

STDMETHODIMP CAPKInstaller::InvokeCommand(LPCMINVOKECOMMANDINFO lpici)
{
    BOOL fUnicode = FALSE;

    if (lpici->cbSize == sizeof(CMINVOKECOMMANDINFOEX))
    {
        if ((lpici->fMask & CMIC_MASK_UNICODE) != 0)
        {
            fUnicode = TRUE;
        }
    }

    UINT idCmd = LOWORD(lpici->lpVerb);

    if (!fUnicode && HIWORD(lpici->lpVerb))
    {
        return E_FAIL;
    }
    if (m_devices.size() == 0) return E_FAIL; // 理论上讲这种情况不会出现，因为检测不到设备的时候这个菜单压根就不存在

    if ((int)(idCmd / m_devices.size()) == 0) return AdbClient::InstallAPK(m_devices[idCmd % m_devices.size()], m_szFiles, false) ? S_OK : E_FAIL;
    else if ((int)(idCmd / m_devices.size()) == 1) return AdbClient::InstallAPK(m_devices[idCmd % m_devices.size()], m_szFiles, true) ? S_OK : E_FAIL;
    /* 可以添加别的选项 */
    else return E_FAIL;

   /* if (idCmd >= m_devices.size())
    {
        return E_FAIL;
    }
    return ADBCommandExecutor::InstallAPK(m_devices[idCmd],m_szFiles,false) ? S_OK : E_FAIL;*/
    //return InstallAPK(m_devices[idCmd]) ? S_OK : E_FAIL;
}

STDMETHODIMP CAPKInstaller::GetCommandString(UINT_PTR idCmd, UINT uType, UINT* pwReserved, LPSTR pszName, UINT cchMax)
{
    return E_NOTIMPL;
}

BOOL CAPKInstaller::InstallAPK(const CString& deviceId)
{

    for (CString f : m_szFiles)
    {
        CString command;
        command.Format(_T("-s %s install \"%s\""), deviceId.GetString(), f.GetBuffer());
        AdbClient::run_async(command);

    }

    return true;
}

CString CAPKInstaller::ExecuteADBCommand(const CString& command)
{
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    HANDLE hReadPipe, hWritePipe;
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0))
    {
        return _T("Error creating pipe");
    }

    STARTUPINFO si = { sizeof(STARTUPINFO) };
    PROCESS_INFORMATION pi;
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;

    CString cmdLine = _T("adb.exe ") + command;
    //BOOL bSuccess = CreateProcess(NULL, cmdLine.GetBuffer(), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
    // 非阻塞，否则会阻塞资源管理器，很傻逼
    BOOL bSuccess = CreateProcess(NULL, cmdLine.GetBuffer(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
    cmdLine.ReleaseBuffer();

    if (!bSuccess)
    {
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        return _T("Error creating process");
    }

    CloseHandle(hWritePipe);

    CString output;
    char buffer[4096];
    DWORD bytesRead;
    while (ReadFile(hReadPipe, buffer, sizeof(buffer), &bytesRead, NULL) && bytesRead > 0)
    {
        buffer[bytesRead] = 0;
        output += CString(buffer);
    }

    CloseHandle(hReadPipe);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return output;
}