// dllmain.h: 模块类的声明。

class CAdbExModule : public ATL::CAtlDllModuleT< CAdbExModule >
{
public :
	DECLARE_LIBID(LIBID_AdbExLib)
	DECLARE_REGISTRY_APPID_RESOURCEID(IDR_ADBEX, "{d1c7558b-a4b6-43bd-b52f-ce852cff69d0}")
};

extern class CAdbExModule _AtlModule;
