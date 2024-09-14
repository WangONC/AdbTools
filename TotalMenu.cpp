// TotalMenu.cpp: CTotalMenu 的实现

#include "pch.h"
#include "TotalMenu.h"
#include <strsafe.h>
#include "AdbClient.h"

#include "globalVar.h"





// CTotalMenu
STDMETHODIMP CTotalMenu::Initialize(PCIDLIST_ABSOLUTE pidlFolder, IDataObject* pdtobj, HKEY hkeyProgID)
{
    TCHAR adbPath[MAX_PATH] = _T("adb.exe");
    adbEnable = PathFindOnPath(adbPath, nullptr); // 这个速度很快，每次右键实时检测几乎不影响速度
    return S_OK;
}

STDMETHODIMP CTotalMenu::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    UINT currentCmdId = idCmdFirst++;
    UINT cindex = indexMenu++;
    if (!adbEnable)
    {
        InsertMenu(hmenu, indexMenu + 1, MF_STRING | MF_BYPOSITION | MF_GRAYED, idCmdFirst + 1, _T("No adb.exe detected"));
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, USHORT(1));
    }

    // 更新全局子菜单，每次都需要一个新的句柄
    hSubMenu = CreatePopupMenu();
    if (!hSubMenu)
        return E_FAIL;
    TCHAR szMenuText[] = L"ADB Tools";
    MENUITEMINFO miiTop = { sizeof(MENUITEMINFO) };
    miiTop.fMask = MIIM_SUBMENU | MIIM_ID | MIIM_DATA | MIIM_STRING;
    miiTop.wID = currentCmdId;
    miiTop.hSubMenu = hSubMenu; // 子菜单句柄
    miiTop.dwItemData = (ULONG_PTR)&CLSID_TotalMenu; // 指定数据，用于寻找当前带单项
    miiTop.dwTypeData = szMenuText;
    InsertMenu(hmenu, cindex, MF_STRING | MF_POPUP | MF_BYPOSITION, idCmdFirst + 1, _T("ADB Tools"));
    //AppendMenu(hmenu, MF_STRING | MF_POPUP | MF_BYPOSITION, currentCmdId, _T("ADB Tools"));
    SetMenuItemInfo(hmenu, cindex, TRUE, &miiTop);


    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 1);
}
STDMETHODIMP CTotalMenu::InvokeCommand(LPCMINVOKECOMMANDINFO lpici)
{
    return S_OK;
}
STDMETHODIMP CTotalMenu::GetCommandString(UINT_PTR idCmd, UINT uType, UINT* pwReserved, LPSTR pszName, UINT cchMax)
{
    return E_NOTIMPL;
}

// TODO sideload 稍复杂，不知道回复报文格式，需要跟源码
// TODO √ reboot reboot：reboot：bootloader reboot：recovery reboot：sideload reboot：sideload-auto-reboot 好像没有回复报文
// TODO root? root unroot 流程稍多，但不复杂，大多数设备都用不了，可以不弄
// TODO forward？ host/reverse:list-forward host/reverse:killforward: host/reverse:forward:norebind:tcp:123:tcp:123 host/reverse:forward:tcp:123:tcp:123  或许在某些特定的场景下有用,有报文返回
// TODO push文件推送 很复杂，不是直接push:,和shell有关，需要研究，看adb_client项目和串口数据差不多？
// TODO install 安装，有些复杂，需要研究
// TODO √ 除此之外还有一些shell命令功能，比如复制当前活动名、软重启，或者是启动adb server？(shell整好了)
// TODO 开启剪贴板同步？不知道毫不耗时，而且听起来似乎需要个服务才行

// TODO 可以搞一个read_and_dump函数，shell就是用这个读数据，看起来像是稳定一次读一个，除此之外shell有自己的protobuf，read_and_dump_protocol，protobuf需要加参数v2，要看功能支不支持shell_v2

