#include "pch.h"
#include <iostream>
#include <fbxsdk.h>
#include <string>
#include <io.h>
#include <deque>
#include <direct.h>
using namespace std;

#define PATH_DELIMITER '\\'
#define COVER_SOURCE_FILE 1


void PrintTabs(int count)
{
    for (int i = 0; i < count; i++)
        printf("  ");
}

void PrintNodeInfo(FbxNode* fbxNode, int depth)
{
    const char* name = fbxNode->GetName();
    PrintTabs(depth);
    printf("%s \n", name);

    for (int i = 0; i < fbxNode->GetChildCount(); i++)
    {
        PrintNodeInfo(fbxNode->GetChild(i), depth + 1);
    }
}

void PrintAnimInfo(FbxAnimStack* fbxAnimStack)
{
    const char* name = fbxAnimStack->GetName();
    printf("Name :%s \n", name);
}

void PrintFbxInfo(FbxScene* scene)
{
    //网格;
    printf("Mesh : \n");
    auto rootNode = scene->GetRootNode();
    if (rootNode) 
    {
        for (int i = 0; i < rootNode->GetChildCount(); i++)
        {
            auto child = rootNode->GetChild(i);
            int depth = 1;
            PrintNodeInfo(child, depth);
        }
    }

    //动画;
    printf("Animation : \n");
    FbxAnimStack* fbxAnimStack = scene->GetCurrentAnimationStack();
    if (fbxAnimStack)
    {
        PrintAnimInfo(fbxAnimStack);
    }
}

void NodeNameNormalize(FbxNode* fbxNode)
{
    string name = string(fbxNode->GetName());
    auto index = name.find(':');
    if (index != string::npos)
    {
        index++;
        auto size = name.size();
        if (index < size)
        {
            int length = size - index;
            name = string(name, index, length);
            fbxNode->SetName(name.c_str());
        }
    }

    for (int i = 0; i < fbxNode->GetChildCount(); i++)
    {
        NodeNameNormalize(fbxNode->GetChild(i));
    }
}

void FbxNormalize(FbxScene* scene, const char* name)
{
    auto rootNode = scene->GetRootNode();
    if (rootNode)
    {
        NodeNameNormalize(rootNode);
    }

    auto fbxAnimStack = scene->GetCurrentAnimationStack();
    if (fbxAnimStack)
    {
        fbxAnimStack->SetName(name);
    }
}

class FileInfo
{
public:
    string Name;
    string FullName;
    string FullPath;
    string DirectoryPath;
};

string GetFileNameWithoutExtension(string name)
{
    auto index = name.find_last_of('.');
    if (index == string::npos)
    {
        return name;
    }
    else
    {
        return name.substr(0, index);
    }
}

void GetFiles(string directoryPath, string exd, bool onlyTopFile, deque<FileInfo>& files)
{
    long   hFile = 0;
    struct _finddata_t fileinfo;
    string pathName, exdName;

    if (0 != strcmp(exd.c_str(), ""))
    {
        exdName = "\\*." + exd;
    }
    else
    {
        exdName = "\\*";
    }

    if ((hFile = _findfirst(pathName.assign(directoryPath).append(exdName).c_str(), &fileinfo)) != -1)
    {
        do
        {
            if (!onlyTopFile && (fileinfo.attrib &  _A_SUBDIR))
            {
                if (strcmp(fileinfo.name, ".") != 0 && strcmp(fileinfo.name, "..") != 0)
                {
                    string newPath = pathName.assign(directoryPath).append("\\").append(fileinfo.name);
                    GetFiles(newPath, exd, onlyTopFile, files);
                }
            }
            else
            {
                if (strcmp(fileinfo.name, ".") != 0 && strcmp(fileinfo.name, "..") != 0)
                {
                    FileInfo* info = new FileInfo();
                    info->FullName = fileinfo.name;
                    info->DirectoryPath = directoryPath;
                    info->FullPath = pathName.assign(directoryPath).append("\\").append(fileinfo.name);
                    info->Name = GetFileNameWithoutExtension(info->FullName);
                    files.push_back(*info);
                }
            }
        } while (_findnext(hFile, &fileinfo) == 0);
        _findclose(hFile);
    }
}

bool CreateDirectory(const string folder)
{
    string folder_builder;
    string sub;
    sub.reserve(folder.size());
    for (auto it = folder.begin(); it != folder.end(); ++it)
    {
        const char c = *it;
        sub.push_back(c);
        if (c == PATH_DELIMITER || it == folder.end() - 1)
        {
            folder_builder.append(sub);
            if (0 != _access(folder_builder.c_str(), 0))
            {
                if (0 != _mkdir(folder_builder.c_str()))
                {
                    return false;
                }
            }
            sub.clear();
        }
    }
    return true;
}

void FbxNormalize(string directoryPath, bool isCover)
{
    deque<FileInfo>* files = new deque<FileInfo>();
    GetFiles(directoryPath, "", true, *files);

    FbxManager* fbxManager = FbxManager::Create();
    FbxIOSettings *fbxIOSettings = FbxIOSettings::Create(fbxManager, IOSROOT);
    fbxManager->SetIOSettings(fbxIOSettings);
    FbxImporter* fbxImporter = FbxImporter::Create(fbxManager, "");
    FbxExporter* fbxExporter = FbxExporter::Create(fbxManager, "");
    FbxScene* scene = FbxScene::Create(fbxManager, "myScene");

    if (isCover)
    {

    }

    for (auto iter = files->begin(); iter != files->end(); iter++)
    {
        FileInfo info = *iter;
        printf(("Path : " + info.FullPath).c_str());

        if (!fbxImporter->Initialize(info.FullPath.c_str(), -1, fbxIOSettings)) 
        {
            printf(", 失败 : %s\n", fbxImporter->GetStatus().GetErrorString());
            continue;
        }

        if (!fbxImporter->IsFBX())
        {
            printf(", 不为Fbx \n");
            continue;
        }

        if (!fbxImporter->Import(scene))
        {
            printf(", 失败 : %s\n", fbxImporter->GetStatus().GetErrorString());
            continue;
        }

        FbxNormalize(scene, info.Name.c_str());

        string newPath;
        if (isCover)
        {
            newPath = info.FullPath;
        }
        else
        {
            newPath = info.DirectoryPath + "\\Normalized\\" + info.FullName;
            CreateDirectory(info.DirectoryPath + "\\Normalized\\");
        }

        if (!fbxExporter->Initialize(newPath.c_str(), -1, fbxIOSettings))
        {
            printf(", 失败 : %s\n", fbxExporter->GetStatus().GetErrorString());
            continue;
        }

        if (!fbxExporter->Export(scene))
        {
            printf(", 失败 : %s\n", fbxExporter->GetStatus().GetErrorString());
            continue;
        }

        printf(", 成功\n");
    }

    fbxImporter->Destroy();
    fbxManager->Destroy();
}


int main() 
{
    printf("将FBX文件标准化,仅处理同级目录的文件;\n");
    printf("是否覆盖源文件?(Y)\n");
    char value;
    scanf_s("%c", &value);
    bool isCover = false;

    if (value == 'Y' || value == 'y')
    {
        isCover = true;
    }

    FbxNormalize(FbxGetCurrentWorkPath().Buffer(), isCover);

    printf("完成!!");

    while(true)
        getchar();

    return 0;
}


////分析节点类型;
//FbxString GetAttributeTypeName(FbxNodeAttribute::EType type)
//{
//	switch (type)
//	{
//		case FbxNodeAttribute::eUnknown: 
//			return "unidentified";
//		case FbxNodeAttribute::eNull:
//			return "null";
//		case FbxNodeAttribute::eMarker: 
//			return "marker";
//		case FbxNodeAttribute::eSkeleton: 
//			return "skeleton";
//		case FbxNodeAttribute::eMesh: 
//			return "mesh";
//		case FbxNodeAttribute::eNurbs:
//			return "nurbs";
//		case FbxNodeAttribute::ePatch:
//			return "patch";
//		case FbxNodeAttribute::eCamera:
//			return "camera";
//		case FbxNodeAttribute::eCameraStereo:
//			return "stereo";
//		case FbxNodeAttribute::eCameraSwitcher:
//			return "camera switcher";
//		case FbxNodeAttribute::eLight:
//			return "light";
//		case FbxNodeAttribute::eOpticalReference: 
//			return "optical reference";
//		case FbxNodeAttribute::eOpticalMarker:
//			return "marker";
//		case FbxNodeAttribute::eNurbsCurve:
//			return "nurbs curve";
//		case FbxNodeAttribute::eTrimNurbsSurface:
//			return "trim nurbs surface";
//		case FbxNodeAttribute::eBoundary:
//			return "boundary";
//		case FbxNodeAttribute::eNurbsSurface:
//			return "nurbs surface";
//		case FbxNodeAttribute::eShape:
//			return "shape";
//		case FbxNodeAttribute::eLODGroup:
//			return "lodgroup";
//		case FbxNodeAttribute::eSubDiv:
//			return "subdiv";
//		default:
//			return "unknown";
//	}
//}
//
//void PrintAttribute(FbxNodeAttribute* pAttribute) 
//{
//	if (!pAttribute) return;
//
//	FbxString typeName = GetAttributeTypeName(pAttribute->GetAttributeType());
//	FbxString attrName = pAttribute->GetName();
//	PrintTabs();
//	printf("\n %s %s", typeName.Buffer(), attrName.Buffer());
//}

