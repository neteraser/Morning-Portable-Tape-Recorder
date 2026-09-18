//////////////////////////////////////
//UpdatingResources.cpp
//copyright 2006 by Ryadovoy
///////////////////////////////////////
//#include "stdafx.h"
#include "UpdatingResources.h"
#include "Resource.h"
//////////////////////////////////////////////////
//resource icon struct (exe file resources)
//RT_GROUP_ICON
#define IDR_MAINFRAME 1

#pragma pack( push )
#pragma pack( 2 )
typedef struct
{
   BYTE   bWidth;               // Width, in pixels, of the image
   BYTE   bHeight;              // Height, in pixels, of the image
   BYTE   bColorCount;          // Number of colors in image (0 if >=8bpp)
   BYTE   bReserved;            // Reserved
   WORD   wPlanes;              // Color Planes
   WORD   wBitCount;            // Bits per pixel
   DWORD  dwBytesInRes;         // how many bytes in this resource?
   WORD   nID;                  // the ID
} GRPICONDIRENTRY, *LPGRPICONDIRENTRY;
#pragma pack( pop )
 
#pragma pack( push )
#pragma pack( 2 )
typedef struct
{
   WORD                 wReserved;   // Reserved (must be 0)
   WORD                 wType;       // Resource type (1 for icons)
   WORD                 wIconDirCount;      // How many images?
   GRPICONDIRENTRY      IconDirEntries[1]; // The entries for each image
} GRPICONDIR, *LPGRPICONDIR;
#pragma pack( pop )
 
//RT_ICON
typedef struct
{
   BITMAPINFOHEADER         icHeader;      // DIB header
   RGBQUAD                  icColors[1];   // Color table
   BYTE                     icXOR[1];      // DIB bits for XOR mask
   BYTE                     icAND[1];      // DIB bits for AND mask
} ICONIMAGE, *LPICONIMAGE;
////////////////////////////////////////////////////////////
//.ico file struct
typedef struct
{
    BYTE        bWidth;          // Width, in pixels, of the image
    BYTE        bHeight;         // Height, in pixels, of the image
    BYTE        bColorCount;     // Number of colors in image (0 if >=8bpp)
    BYTE        bReserved;       // Reserved ( must be 0)
    WORD        wPlanes;         // Color Planes
    WORD        wBitCount;       // Bits per pixel
    DWORD       dwBytesInRes;    // How many bytes in this resource?
    DWORD       dwImageOffset;   // Where in the file is this image?
} ICONDIRENTRY, *LPICONDIRENTRY;
 
//error codes for IconResource class
#define ERR_CANNOT_OPEN_FILE        9000L
#define ERR_ALLOCATING_MEMORY       9001L
#define ERR_CANNOT_READ_FILE        9002L
#define ERR_CANNOT_SEEK_TO_IMAGE    9003L
 
/////////////////////////////////////////////////////////////////
//class IconResource incapsulate from itself icon resource data.
//It load resource data from icon file .ico
class IconResource{
public:
    IconResource();
    ~IconResource();
public:
    LONG                LoadIconResourceFromIcoFile(LPCSTR lpszFileName);
    BOOL                SetIconID(DWORD dwIndex, WORD nIconID);
 
    inline DWORD        GetIconCount() const;
    const LPGRPICONDIR  GetRtGroupIconResource() const;
    DWORD               GetSizeOfRtGroupIconResource() const;
    const LPICONIMAGE   GetRtIconResource(DWORD dwIndex) const;
    DWORD               GetSizeOfRtIconResource(DWORD dwIndex) const;
private:
    void Clear();
private:
    LPGRPICONDIR                m_lpGrpIconDir;
    std::vector<LPICONIMAGE>    m_vlpIconImage;
};
 
/////////////////////////////
//class ResourceUpdater
void ResourceUpdater::SetExecutableFileName(LPCSTR strExecutableFileName){
    m_strExecutableFileName = strExecutableFileName;
}
void ResourceUpdater::SetIconFileName(LPCSTR strIconFileName){
    m_strIconFileName = strIconFileName;
}
LONG ResourceUpdater::UpdateResources(){
    IconResource icoResource;
    LONG lResult = icoResource.LoadIconResourceFromIcoFile(m_strIconFileName);
    if(lResult != ERROR_SUCCESS){
        return ERR_CANNOT_LOAD_ICON_FROM_FILE;
    }
    
    HANDLE hUpdateRes = BeginUpdateResource(m_strExecutableFileName, FALSE);
    if (hUpdateRes == NULL){
        return ERR_CANNOT_OPEN_EXE_FILE;
    }
 
    DWORD dwIconCount = icoResource.GetIconCount();
		DWORD i;
    for( i = 0; i< dwIconCount;  i++){
        icoResource.SetIconID(i, IDR_MAINFRAME+(WORD)i);
    }
 
    //add (or rewrite) RT_GROUP_ICON to resuurce
    if(!UpdateResource(hUpdateRes,      
                        RT_GROUP_ICON,                  
                        MAKEINTRESOURCE(101),                  
                        1049, //MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),  
                        icoResource.GetRtGroupIconResource(),                  
                        icoResource.GetSizeOfRtGroupIconResource())){
        return ERR_CANNOT_UPDATE_RESOURCE;
    }
 
    //add (or rewrite) RT_ICON to resuurce
    for( i = 0; i< dwIconCount;  i++){
        if(!UpdateResource(hUpdateRes,      
                            RT_ICON,                  
                            MAKEINTRESOURCE(i + 1),                  
                            1049, // MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),  
                            icoResource.GetRtIconResource(i),                  
                            icoResource.GetSizeOfRtIconResource(i))){  
            return ERR_CANNOT_UPDATE_RESOURCE;
        }      
    }
 
    if(!EndUpdateResource(hUpdateRes, FALSE)){
        return ERR_CANNOT_SAVE_CHANGES;
    }
    return ERROR_SUCCESS;
}
 
////////////////////////////////
//class IconResource
IconResource::IconResource(){
    m_lpGrpIconDir = NULL;
}
IconResource::~IconResource(){
    Clear();
}
void IconResource::Clear(){
    DWORD dwSize = m_vlpIconImage.size();
    for(DWORD i = 0; i< dwSize; i++){
        free( m_vlpIconImage[i] );
    }
    m_vlpIconImage.clear();
    if(m_lpGrpIconDir){
        free( m_lpGrpIconDir );
        m_lpGrpIconDir = NULL;
    }
}
LONG IconResource::LoadIconResourceFromIcoFile(LPCSTR lpszFileName){
    Clear();
    DWORD dwBytesRead = 0;
    DWORD dwBytesToRead = 0;
    // Open icon file
    HANDLE hFile = CreateFile( lpszFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
    if(hFile == INVALID_HANDLE_VALUE){
        return ERR_CANNOT_OPEN_FILE;
    }
    // Allocate memory to store GRPICONDIR struct
    m_lpGrpIconDir = (LPGRPICONDIR)malloc(sizeof( GRPICONDIR ));
    if(m_lpGrpIconDir == NULL){
        CloseHandle(hFile);
        return ERR_ALLOCATING_MEMORY;      
    }
    // Read the Reserved word
    dwBytesToRead = sizeof( WORD );
    if(!ReadFile( hFile, &(m_lpGrpIconDir->wReserved), dwBytesToRead, &dwBytesRead, NULL )){
        CloseHandle(hFile);
        Clear();
        return ERR_CANNOT_READ_FILE;
    }
    if(dwBytesToRead != dwBytesRead){
        CloseHandle(hFile);
        Clear();
        return ERR_CANNOT_READ_FILE;    
    }
    // Read the Type word - make sure it is 1 for icons
    if(!ReadFile( hFile, &(m_lpGrpIconDir->wType), dwBytesToRead, &dwBytesRead, NULL )){
        CloseHandle(hFile);
        Clear();
        return ERR_CANNOT_READ_FILE;
    }
    if(dwBytesToRead != dwBytesRead){
        CloseHandle(hFile);
        Clear();
        return ERR_CANNOT_READ_FILE;    
    }
    // Read the count - how many images in this file?
    if(!ReadFile( hFile, &(m_lpGrpIconDir->wIconDirCount), dwBytesToRead, &dwBytesRead, NULL )){
        CloseHandle(hFile);
        Clear();
        return ERR_CANNOT_READ_FILE;
    }
    if(dwBytesToRead != dwBytesRead){
        CloseHandle(hFile);
        Clear();
        return ERR_CANNOT_READ_FILE;    
    }
    // Reallocate IconDir so that idEntries has enough room for idCount elements
    LPGRPICONDIR pNewGrpIconDir;
    pNewGrpIconDir = (LPGRPICONDIR)realloc( m_lpGrpIconDir, ( sizeof( WORD ) * 3 ) + ( sizeof( GRPICONDIRENTRY ) * m_lpGrpIconDir->wIconDirCount ) );
    if(pNewGrpIconDir == NULL){
        CloseHandle(hFile);
        Clear();
        return ERR_ALLOCATING_MEMORY;      
    }
    m_lpGrpIconDir = pNewGrpIconDir;
    
    std::vector<DWORD>  vdwImageOffset;
		WORD i;
    for(i = 0; i< m_lpGrpIconDir->wIconDirCount; i++){
        // Read current ICONDIRENTRY elements, process it, get GRPICONDIRENTRY struct, and store ImagesOffset
        ICONDIRENTRY IconEntry;
        dwBytesToRead = sizeof(ICONDIRENTRY);
        if(!ReadFile( hFile, &IconEntry, dwBytesToRead, &dwBytesRead, NULL )){
            CloseHandle(hFile);
            Clear();
            return ERR_CANNOT_READ_FILE;
        }
        if(dwBytesToRead != dwBytesRead){
            CloseHandle(hFile);
            Clear();
            return ERR_CANNOT_READ_FILE;    
        }  
        memcpy((LPGRPICONDIRENTRY)&m_lpGrpIconDir->IconDirEntries[i], &IconEntry, sizeof(ICONDIRENTRY) - sizeof(DWORD));
        vdwImageOffset.push_back(IconEntry.dwImageOffset);
    }
    for( i = 0; i< m_lpGrpIconDir->wIconDirCount; i++){
        // Loop through and read in each image
        LPICONIMAGE pIconImage;
        // Allocate memory to hold the image
        pIconImage = (LPICONIMAGE)malloc( m_lpGrpIconDir->IconDirEntries[i].dwBytesInRes );
        if(!pIconImage){
            CloseHandle(hFile);
            Clear();
            return ERR_ALLOCATING_MEMORY;          
        }
        m_vlpIconImage.push_back(pIconImage);
        // Seek to the location in the file that has the image
        if(SetFilePointer( hFile, vdwImageOffset[i], NULL, FILE_BEGIN ) == 0xFFFFFFFF){
            CloseHandle(hFile);
            Clear();
            return ERR_CANNOT_SEEK_TO_IMAGE;            
        }
        // Read the image data
        dwBytesToRead = m_lpGrpIconDir->IconDirEntries[i].dwBytesInRes;
        if(!ReadFile( hFile, pIconImage, dwBytesToRead, &dwBytesRead, NULL )){
            CloseHandle(hFile);
            Clear();
            return ERR_CANNOT_READ_FILE;            
        }
        if(dwBytesToRead != dwBytesRead){
            CloseHandle(hFile);
            Clear();
            return ERR_CANNOT_READ_FILE;    
        }
    }
    CloseHandle(hFile);
    return ERROR_SUCCESS;
}
 
inline DWORD IconResource::GetIconCount() const {
    return m_vlpIconImage.size();
}
BOOL IconResource::SetIconID(DWORD dwIndex, WORD nIconID){
    if(dwIndex >= GetIconCount()){
        return FALSE;
    }
    if(!m_lpGrpIconDir){
        return FALSE;
    }
    m_lpGrpIconDir->IconDirEntries[dwIndex].nID = nIconID;
    return TRUE;
}
const LPGRPICONDIR IconResource::GetRtGroupIconResource() const{
    return m_lpGrpIconDir;
}
DWORD IconResource::GetSizeOfRtGroupIconResource() const{
    if(!m_lpGrpIconDir){
        return 0;
    }
    return ( sizeof( WORD ) * 3 ) + ( sizeof( GRPICONDIRENTRY ) * m_lpGrpIconDir->wIconDirCount );
}
const LPICONIMAGE   IconResource::GetRtIconResource(DWORD dwIndex) const{
    if(dwIndex >= GetIconCount()){
        return NULL;
    }
    return m_vlpIconImage[dwIndex];
}
DWORD IconResource::GetSizeOfRtIconResource(DWORD dwIndex) const{
    if(dwIndex >= GetIconCount()){
        return FALSE;
    }
    if(!m_lpGrpIconDir){
        return 0;
    }
    return m_lpGrpIconDir->IconDirEntries[dwIndex].dwBytesInRes;
}