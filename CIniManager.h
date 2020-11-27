#ifndef _CINIMANAGER_H_
#define _CINIMANAGER_H_

#include <map>
#include <string.h>
using namespace std;

#define CONFIGLEN           256

enum INI_RES
{
    INI_SUCCESS,            //??
    INI_ERROR,              //????
    INI_OPENFILE_ERROR,     //??????
    INI_NO_ATTR            //??????
};

//              ????    ???
typedef map<std::string,std::string> KEYMAP;
//              ???? ???
typedef map<std::string,KEYMAP> MAINKEYMAP;
// config ????????

class CIniManager
{
public:
    // ????
    CIniManager();

    // ????
    virtual ~CIniManager();
public:
    //???????
    int  GetInt(const char* mAttr, const char* cAttr );
    //????????
    char *GetStr(const char* mAttr, const char* cAttr );
    // ??config ??
    INI_RES OpenFile(const char* pathName, const char* type);
    // ??config ??
    INI_RES CloseFile();
protected:
    // ??config??
    INI_RES GetKey(const char* mAttr, const char* cAttr, char* value);
protected:
    // ????????
    FILE* m_fp;
    char  m_szKey[ CONFIGLEN ];
    MAINKEYMAP m_Map;
};

#endif // FILLE_H
