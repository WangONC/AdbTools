// TotalMenu.h: CTotalMenu 的声明

#pragma once
#include "resource.h"       // 主符号


#include "AdbEx_i.h"


#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Windows CE 平台(如不提供完全 DCOM 支持的 Windows Mobile 平台)上无法正确支持单线程 COM 对象。定义 _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA 可强制 ATL 支持创建单线程 COM 对象实现并允许使用其单线程 COM 对象实现。rgs 文件中的线程模型已被设置为“Free”，原因是该模型是非 DCOM Windows CE 平台支持的唯一线程模型。"
#endif

using namespace ATL;


// CTotalMenu

class ATL_NO_VTABLE CTotalMenu :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CTotalMenu, &CLSID_TotalMenu>,
	public IDispatchImpl<ITotalMenu, &IID_ITotalMenu, &LIBID_AdbExLib, /*wMajor =*/ 1, /*wMinor =*/ 0>,
	public IShellExtInit,
	public IContextMenu
{
public:
	CTotalMenu()
	{
	}

DECLARE_REGISTRY_RESOURCEID(107)


BEGIN_COM_MAP(CTotalMenu)
	COM_INTERFACE_ENTRY(ITotalMenu)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IShellExtInit)
	COM_INTERFACE_ENTRY(IContextMenu)
END_COM_MAP()



	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct()
	{
		return S_OK;
	}

	void FinalRelease()
	{
	}

public:
	// IShellExtInit
	STDMETHOD(Initialize)(PCIDLIST_ABSOLUTE pidlFolder, IDataObject* pdtobj, HKEY hkeyProgID);

	// IContextMenu
	STDMETHOD(QueryContextMenu)(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
	STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO lpici);
	STDMETHOD(GetCommandString)(UINT_PTR idCmd, UINT uType, UINT* pwReserved, LPSTR pszName, UINT cchMax);


};

OBJECT_ENTRY_AUTO(__uuidof(TotalMenu), CTotalMenu)
