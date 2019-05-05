
#include "LordPe.h"
#include <winternl.h>
#pragma comment(lib,"ntdll.lib")

CLordPe::CLordPe()
{
	m_pDosHdr = new IMAGE_DOS_HEADER;
}


CLordPe::~CLordPe()
{
	delete m_pBuf;
	m_pBuf = NULL;

	delete m_pDosHdr;
	m_pDosHdr = NULL;
}


//************************************
// Method:    GetDosHead
// FullName:  CLordPe::GetDosHead
// Access:    public 
// Returns:   BOOL
// Qualifier:
// Parameter: LPCTSTR filePath
// Function:  �������Խ���PE�ļ����ݶ�ȡ�������������ڴ�ռ�
//************************************
BOOL CLordPe::GetDosHead(LPCTSTR filePath)
{
	// 1. ���ļ�,���ļ���ȡ���ڴ�.
	// CreateFile,ReadFile.
	HANDLE hFile = INVALID_HANDLE_VALUE;
	hFile = CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}
	DWORD dwFileSize = 0;
	dwFileSize = GetFileSize(hFile, NULL);

	// 2. �����ڴ�ռ�
	BYTE* pBuf = new BYTE[dwFileSize];
	m_pBuf = pBuf;//�������������������ͷſռ�

	// 3. ���ļ����ݶ�ȡ���ڴ���
	DWORD dwRead = 0;
	ReadFile(hFile, pBuf, dwFileSize, &dwRead, NULL);

	// ������������DOSͷ�ṹ��������
	m_pDosHdr = (IMAGE_DOS_HEADER*)pBuf;//��DOSͷָ�뱣������

										// ntͷ,�������ļ�ͷ����չͷ
	IMAGE_NT_HEADERS* pNtHdr;
	pNtHdr = (IMAGE_NT_HEADERS*)(m_pDosHdr->e_lfanew + (DWORD)m_pDosHdr);

	// �ж��Ƿ���һ����Ч��pe�ļ�
	if (m_pDosHdr->e_magic != IMAGE_DOS_SIGNATURE || pNtHdr->Signature != IMAGE_NT_SIGNATURE)
	{
		return FALSE;
	}
	return TRUE;
}

//************************************
// Method:    GetOep
// FullName:  CLordPe::GetOep
// Access:    public 
// Returns:   DWORD
// Qualifier:
// Function:  ��ȡ�����Խ���OEP
//************************************
DWORD CLordPe::GetOep()
{
	// ntͷ,�������ļ�ͷ����չͷ
	IMAGE_NT_HEADERS* pNtHdr = (IMAGE_NT_HEADERS*)(m_pDosHdr->e_lfanew + (DWORD)m_pDosHdr);

	IMAGE_OPTIONAL_HEADER* pOptHdr = &(pNtHdr->OptionalHeader);//��չͷ

	return pOptHdr->AddressOfEntryPoint;

}


//����������
void CLordPe::ExportTable()
{
	// ntͷ,�������ļ�ͷ����չͷ
	IMAGE_NT_HEADERS* pNtHdr;
	pNtHdr = (IMAGE_NT_HEADERS*)(m_pDosHdr->e_lfanew + (DWORD)m_pDosHdr);

	IMAGE_OPTIONAL_HEADER* pOptHdr;// ��չͷ
	pOptHdr = &(pNtHdr->OptionalHeader);

	//����Ŀ¼��
	PIMAGE_DATA_DIRECTORY pDataDirectory = pOptHdr->DataDirectory;

	// 5. �ҵ�������
 	DWORD dwExpRva = pDataDirectory[0].VirtualAddress;

	// 5.1 �õ�RVA���ļ�ƫ��
	DWORD dwExpOfs = RVAToOffset(m_pDosHdr, dwExpRva);
	IMAGE_EXPORT_DIRECTORY* pExpTab = NULL;
	pExpTab = (IMAGE_EXPORT_DIRECTORY*)(dwExpOfs + (DWORD)m_pDosHdr);

	// ����DLL�ĵ�����
	/*
	typedef struct _IMAGE_EXPORT_DIRECTORY {
	DWORD   Characteristics;
	DWORD   TimeDateStamp;
	WORD    MajorVersion;
	WORD    MinorVersion;
	DWORD   Name; // dll��[�ַ�����RVA]
	DWORD   Base;
	DWORD   NumberOfFunctions;
	DWORD   NumberOfNames;
	DWORD   AddressOfFunctions;     // ��ַ��(DWORD����)��rva
	DWORD   AddressOfNames;         // ���Ʊ�(DWORD����)��rva
	DWORD   AddressOfNameOrdinals;  // ��ű�(WORD����)��rva
	} IMAGE_EXPORT_DIRECTORY, *PIMAGE_EXPORT_DIRECTORY;
	*/
	// 1. ����DLL����
	DWORD dwNameOfs = RVAToOffset(m_pDosHdr, pExpTab->Name);
	char* pDllName = (char*)(dwNameOfs + (DWORD)m_pDosHdr);
	//�����������Ϣ
	//ZeroMemory(&m_my_im_ex_di, sizeof(m_my_im_ex_di));//ȫ�ֶ���Ϊ��
	m_my_im_ex_di.name = pDllName;
	m_my_im_ex_di.Base = pExpTab->Base;
	m_my_im_ex_di.NumberOfFunctions = pExpTab->NumberOfFunctions;
	m_my_im_ex_di.NumberOfNames = pExpTab->NumberOfNames;
	m_my_im_ex_di.AddressOfFunctions = pExpTab->AddressOfFunctions;
	m_my_im_ex_di.AddressOfNames = pExpTab->AddressOfNames;
	m_my_im_ex_di.AddressOfNameOrdinals = pExpTab->AddressOfNameOrdinals;

	// �������ű�
	DWORD dwExpAddrTabOfs = RVAToOffset(m_pDosHdr, pExpTab->AddressOfFunctions);
	DWORD dwExpNameTabOfs = RVAToOffset(m_pDosHdr, pExpTab->AddressOfNames);
	DWORD dwExpOrdTabOfs = RVAToOffset(m_pDosHdr, pExpTab->AddressOfNameOrdinals);

	// ��ַ��,���Ʊ���һ��DWORD��������
	DWORD* pExpAddr = (DWORD*)(dwExpAddrTabOfs + (DWORD)m_pDosHdr);

	DWORD* pExpName = (DWORD*)(dwExpNameTabOfs + (DWORD)m_pDosHdr);

	// ��ű���WORD���͵�����
	WORD* pExpOrd = (WORD*)(dwExpOrdTabOfs + (DWORD)m_pDosHdr);

	// �������еĺ�����ַ
	m_vecExportFunInfo.clear();
	for (int i = 0; i< pExpTab->NumberOfFunctions; ++i)
	{
		EXPORTFUNINFO exportFunInfo = { 0 };
		exportFunInfo.FunctionRVA = pExpAddr[i];//����RVA
		exportFunInfo.FunctionOffset = RVAToOffset(m_pDosHdr, pExpAddr[i]);//��������ļ�ƫ��
																		   // ���ҵ�ǰ�������ĵ�ַ��û������
		int j = 0;
		for (; j < pExpTab->NumberOfNames; ++j)
		{
			// �жϵ�ַ����±��Ƿ񱻱������ں���������ű���.
			// �����������, ��˵������±�������ĵ�ַ,��һ���к������Ƶĵ�ַ
			if (i == pExpOrd[j])
				break;
		}
		// �ж�ѭ������Ȼ����,����ͨ��break����
		if (j < pExpTab->NumberOfNames)
		{
			// ��ַ������,j���Ƕ�Ӧ�ĺ���������±�.ͨ��j�����ҵ�һ���������Ƶ�Rva
			DWORD dwNameRva = pExpName[j];
			DWORD dwNameOfs = RVAToOffset(m_pDosHdr, dwNameRva);
			char* pFunctionName = nullptr;
			pFunctionName = (char*)(dwNameOfs + (DWORD)m_pDosHdr);
			exportFunInfo.FunctionName = pFunctionName;//������
			exportFunInfo.ExportOrdinals = pExpTab->Base + i;//�����������
		}
		else
		{
			// ˵����ַû�������Ƶķ�ʽ����.
			if (pExpAddr[i] != 0)
			{
				// �����ĵ������:
				// ��Ż��� + �����(��ַ����±�)
				exportFunInfo.FunctionName = L"";//������Ϊ��
				exportFunInfo.ExportOrdinals = pExpTab->Base + i;//�����������
			}
		}
		m_vecExportFunInfo.push_back(exportFunInfo);
	}
}


//���������
void CLordPe::ImportTable()
{
	// ntͷ,�������ļ�ͷ����չͷ
	IMAGE_NT_HEADERS* pNtHdr;
	pNtHdr = (IMAGE_NT_HEADERS*)(m_pDosHdr->e_lfanew + (DWORD)m_pDosHdr);

	IMAGE_OPTIONAL_HEADER* pOptHdr;// ��չͷ
	pOptHdr = &(pNtHdr->OptionalHeader);

	//����Ŀ¼��
	PIMAGE_DATA_DIRECTORY pDataDirectory = pOptHdr->DataDirectory;

	// 5. �ҵ������
	// 5. �õ�������RVA
	DWORD dwImpRva = pDataDirectory[1].VirtualAddress;

	IMAGE_IMPORT_DESCRIPTOR* pImpArray;

	pImpArray = (IMAGE_IMPORT_DESCRIPTOR*)(RVAToOffset(m_pDosHdr, dwImpRva) + (DWORD)m_pDosHdr);

	// ���������ĸ�����û�������ֶμ�¼.
	// �����ı�־����һ��ȫ0��Ԫ����Ϊ��β
	m_vecImportDescriptor.clear();
	m_vvImportFunInfo.clear();
	while (pImpArray->Name != 0)
	{
		MY_IMPORT_DESCRIPTOR myImportDescriptor;//!!!!!!!ע������û�г�ʼ���ṹ��
		// �����Dll������(Rva)
		DWORD dwNameOfs = RVAToOffset(m_pDosHdr, pImpArray->Name);
		char* pDllName = (char*)(dwNameOfs + (DWORD)m_pDosHdr);
		myImportDescriptor.Name = pDllName;

		// ����,�����dll��,һ��������Щ����
		myImportDescriptor.OriginalFirstThunk = pImpArray->OriginalFirstThunk;
		myImportDescriptor.FirstThunk = pImpArray->FirstThunk;

		// IAT(��������)��¼��һ����һ��dll�е�������Щ����,��Щ����Ҫô�������Ƶ���,Ҫô������ŵ����
		// ��¼��һ��������. ���������IMAGE_THUNK_DATA���͵Ľṹ������.
		DWORD INTOfs = RVAToOffset(m_pDosHdr, pImpArray->OriginalFirstThunk);
		myImportDescriptor.OffsetOriginalFirstThunk = INTOfs;

		DWORD IATOfs = RVAToOffset(m_pDosHdr, pImpArray->FirstThunk);
		myImportDescriptor.OffsetFirstThunk = IATOfs;

		m_vecImportDescriptor.push_back(myImportDescriptor);
		/*
		����һ��ֻ��4���ֽڵĽṹ��.�����������е�ÿһ���ֶα����ֵ����һ��.��Щֵ, ���ǵ��뺯������Ϣ.
		���뺯������Ϣ�����²���:
		1. ���뺯�������
		2. ���뺯��������(�����п���û��)
		���Ը��ݽṹ���е��ֶε����λ�ж�, ������Ϣ�������Ƶ��뻹������ŵ���
		typedef struct _IMAGE_THUNK_DATA32 {
		union {
		DWORD ForwarderString;      // PBYTE
		DWORD Function;             // PDWORD
		DWORD Ordinal;
		DWORD AddressOfData;        // PIMAGE_IMPORT_BY_NAME
		} u1;
		} IMAGE_THUNK_DATA32;
		*/
		IMAGE_THUNK_DATA* pInt = NULL;
		IMAGE_THUNK_DATA* pIat = NULL;
		pInt = (IMAGE_THUNK_DATA*)(INTOfs + (DWORD)m_pDosHdr);
		pIat = (IMAGE_THUNK_DATA*)(IATOfs + (DWORD)m_pDosHdr);

		//INT���Կ�����IAT�ı��ݣ�����û�б��ݵ��������˽���IAT
		m_vecImportFunInfo.clear();
		while (pInt->u1.Function != 0)
		{
			IMPORTFUNINFO importFunInfo = { 0 };
			// �ж��Ƿ�������ŵ���
			if (IMAGE_SNAP_BY_ORDINAL32(pInt->u1.Function))
			{
				// ����ŷ�ʽ����ṹ�屣���ֵ��16λ����һ����������
				importFunInfo.Ordinal = pInt->u1.Ordinal & 0xFFFF;
				importFunInfo.Name = L"";
			}
			else
			{
				// �������������Ƶ����ʱ��, pInt->u1.Function �������һ��rva , 
				//���RVAָ��һ�����溯��������Ϣ�Ľṹ��
				DWORD dwImpNameOfs = RVAToOffset(m_pDosHdr, pInt->u1.Function);
				IMAGE_IMPORT_BY_NAME* pImpName;
				pImpName = (IMAGE_IMPORT_BY_NAME*)(dwImpNameOfs + (DWORD)m_pDosHdr);
				importFunInfo.Ordinal = pImpName->Hint;
				importFunInfo.Name = pImpName->Name;
				m_vecImportFunInfo.push_back(importFunInfo);
			}
			++pInt;
		}
		m_vvImportFunInfo.push_back(m_vecImportFunInfo);
		++pImpArray;
	}
}

//RVAת�ļ�ƫ��
DWORD CLordPe::RVAToOffset(IMAGE_DOS_HEADER* pDos,
	DWORD dwRva)
{
	IMAGE_SECTION_HEADER* pScnHdr;

	IMAGE_NT_HEADERS* pNtHdr = (IMAGE_NT_HEADERS*)(pDos->e_lfanew + (DWORD)pDos);

	pScnHdr = IMAGE_FIRST_SECTION(pNtHdr);
	DWORD dwNumberOfScn = pNtHdr->FileHeader.NumberOfSections;

	// 1. �������������ҵ���������
	for (int i = 0; i < dwNumberOfScn; ++i)
	{
		DWORD dwEndOfSection = pScnHdr[i].VirtualAddress + pScnHdr[i].SizeOfRawData;
		// �ж����RVA�Ƿ���һ�����εķ�Χ��
		if (dwRva >= pScnHdr[i].VirtualAddress
			&& dwRva < dwEndOfSection)
		{
			// 2. �����RVA�������ڵ�ƫ��:rva ��ȥ�׵�ַ
			DWORD dwOffset = dwRva - pScnHdr[i].VirtualAddress;
			// 3. ��������ƫ�Ƽ������ε��ļ���ʼƫ��
			return dwOffset + pScnHdr[i].PointerToRawData;
		}
	}
	return -1;
}

