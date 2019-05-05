#pragma once
#include "windows.h"
#include <vector>
#include "atlstr.h"
using std::vector;

//����������Ϣ
typedef struct _EXPORTFUNINFO
{
	DWORD ExportOrdinals;//�������
	DWORD FunctionRVA;//����RVA
	DWORD FunctionOffset;//�����ļ�ƫ��
	CString FunctionName;//������
}EXPORTFUNINFO, *PEXPORTFUNINFO;

//�����������Ϣ
typedef struct _MY_IM_EX_DI
{
	CString name;//dll��
	DWORD Base;//��Ż���
	DWORD NumberOfFunctions;//��������
	DWORD NumberOfNames;//������������
	DWORD AddressOfFunctions;//��ַ��RVA
	DWORD AddressOfNames;//���Ʊ�RVA
	DWORD AddressOfNameOrdinals;//��ű�RVA
}MY_IM_EX_DI, *PMY_IM_EX_DI;

//����������Ϣ
typedef struct _MY_IMPORT_DESCRIPTOR
{
	CString Name;//DLL����
	DWORD OriginalFirstThunk;//INT(�������Ʊ�RVA)
	DWORD OffsetOriginalFirstThunk;//INT(�������Ʊ�ƫ��)
	DWORD FirstThunk;//IAT(�����ַ��RVA)
	DWORD OffsetFirstThunk;//IAT(�����ַ��ƫ��)

}MY_IMPORT_DESCRIPTOR, *PMY_IMPORT_DESCRIPTOR;

//���뺯����Ϣ
typedef struct _IMPORTFUNINFO
{
	DWORD Ordinal;
	CString Name;

}IMPORTFUNINFO, *PIMPORTFUNINFO;

class CLordPe
{
public:
	CLordPe();
	~CLordPe();
	BOOL GetDosHead(LPCTSTR filePath);
	DWORD GetOep();
	void ExportTable();
	void ImportTable();
	DWORD RVAToOffset(IMAGE_DOS_HEADER* pDos, DWORD dwRva);
public:
	//----------------������---------------------//
	vector<EXPORTFUNINFO> m_vecExportFunInfo;
	MY_IM_EX_DI m_my_im_ex_di;

	//----------------�����---------------------//
	vector<MY_IMPORT_DESCRIPTOR> m_vecImportDescriptor;
	vector<IMPORTFUNINFO> m_vecImportFunInfo;
	vector<vector<IMPORTFUNINFO>> m_vvImportFunInfo;

	BYTE* m_pBuf;//�����ͷ�����Ŀռ�
	PIMAGE_DOS_HEADER m_pDosHdr;//DOSͷ��ַ
};