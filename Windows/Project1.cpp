//#undef UNICODE
#include "framework.h"
#include "Project1.h"
#include <iostream>
#include <shellapi.h>
#include <string>
#include <atlstr.h>
#include <WinUser.h>
#include <memory>
#include "mongoose.h"
#include "mjson.h"
#include <thread>

//mac lib
#include <winsock2.h>
#include <iphlpapi.h>
#pragma comment(lib, "IPHLPAPI.lib")

#define MAX_LOADSTRING 100
#define WM_MINMAXNotify  1006
#define IDM_HIDE_ICON 2006
#define COPY_MAC 2007

using namespace std;

// global variable:
HINSTANCE hInst;                                
WCHAR szTitle[MAX_LOADSTRING];                  
WCHAR szWindowClass[MAX_LOADSTRING];            

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

NOTIFYICONDATA nID = {};

struct mg_mgr mgr;
static const char* s_listening_address = "http://127.0.0.1:32156"; //Http port number
//thread httpThread;


ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    //wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PROJECT1));
    wcex.hIcon          = (HICON)LoadImage(NULL, TEXT("logo48.ico"), IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_PROJECT1);
    wcex.lpszClassName  = szWindowClass;
    //wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
    wcex.hIconSm        = (HICON)LoadImage(NULL, TEXT("logo48.ico"), IMAGE_ICON, 0, 0, LR_LOADFROMFILE);

    return RegisterClassExW(&wcex);
}

void  replace_string(std::string& res) {
    //Remove all spaces
    res.erase(std::remove_if(res.begin(), res.end(), std::isspace), res.end());

    //Remove uuid
    res.replace(0, 4, "");

}

bool getUUID(std::string& uuid) {
    const char* CommandLine = "wmic csproduct get UUID"; //Modify here
    SECURITY_ATTRIBUTES  sa;
    HANDLE hRead, hWrite;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;
    if (!CreatePipe(&hRead, &hWrite, &sa, 0)) {
        return false;
    }
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    si.cb = sizeof(STARTUPINFO);
    GetStartupInfoA(&si);
    si.hStdError = hWrite;
    si.hStdOutput = hWrite;
    si.wShowWindow = SW_HIDE;
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    if (!CreateProcessA(NULL, (char*)CommandLine, NULL, NULL, TRUE, NULL, NULL, NULL, &si, &pi)) {
        return false;
    }
    CloseHandle(hWrite);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    char buffer[4096] = { 0 };
    DWORD bytesRead;
    while (true) {
        memset(buffer, 0, strlen(buffer));
        if (ReadFile(hRead, buffer, 4095, &bytesRead, NULL) == NULL) break;
        uuid.append(buffer);
    }
    //printf("CSP UUID %s\n", uuid.c_str());
    replace_string(uuid);
    //printf("CSP UUID %s\n", uuid.c_str());
    return true;
}

//mac Address acquisition
bool GetMacByGetAdaptersAddresses(std::string& macOUT)
{
    bool ret = false;

    ULONG outBufLen = sizeof(IP_ADAPTER_ADDRESSES);
    PIP_ADAPTER_ADDRESSES pAddresses = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);
    if (pAddresses == NULL)
        return false;
    // Make an initial call to GetAdaptersAddresses to get the necessary size into the ulOutBufLen variable
    if (GetAdaptersAddresses(AF_UNSPEC, 0, NULL, pAddresses, &outBufLen) == ERROR_BUFFER_OVERFLOW)
    {
        free(pAddresses);
        pAddresses = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);
        if (pAddresses == NULL)
            return false;
    }

    if (GetAdaptersAddresses(AF_UNSPEC, 0, NULL, pAddresses, &outBufLen) == NO_ERROR)
    {
        // If successful, output some information from the data we received
        for (PIP_ADAPTER_ADDRESSES pCurrAddresses = pAddresses; pCurrAddresses != NULL; pCurrAddresses = pCurrAddresses->Next)
        {
            // Ensure the length of MAC address is 00-00-00-00-00-00
            if (pCurrAddresses->PhysicalAddressLength != 6)
                continue;

            //Print all MAC addresses
            //printf("%s\n", macOUT.c_str());

            char acMAC[32];
            sprintf(acMAC, "%02X-%02X-%02X-%02X-%02X-%02X",
                int(pCurrAddresses->PhysicalAddress[0]),
                int(pCurrAddresses->PhysicalAddress[1]),
                int(pCurrAddresses->PhysicalAddress[2]),
                int(pCurrAddresses->PhysicalAddress[3]),
                int(pCurrAddresses->PhysicalAddress[4]),
                int(pCurrAddresses->PhysicalAddress[5]));
            macOUT = acMAC;

            //Take the first
            if (!macOUT.empty()) {
                ret = true;
                break;
            }

            //Check the networking status - get the MAC address of the network, otherwise take the last MAC address
            //if (pCurrAddresses->OperStatus == IfOperStatusUp) {
                //printf("Found networked MAC address: %s\n", macOUT.c_str());
            //    ret = true;
            //    break;
            //}
        }
    }

    free(pAddresses);
    return ret;
}

BOOL Copy(CString source)
{
    if (OpenClipboard(NULL))
    {
        //Prevent non-ASCII language from being copied to the clipboard as garbled code
        int buff_size = source.GetLength();
        CStringW strWide = CStringW(source);
        int nLen = strWide.GetLength();
        //Empty the cutting board
        ::EmptyClipboard();
        HANDLE clipbuffer = ::GlobalAlloc(GMEM_DDESHARE, (nLen + 1) * 2);
        if (!clipbuffer) {
            ::CloseClipboard();
            return FALSE;
        }
        char* buffer = (char*)::GlobalLock(clipbuffer);
        if (buffer) {
            memset(buffer, 0, (nLen + 1) * 2);
            memcpy_s(buffer, nLen * 2, strWide.GetBuffer(0), nLen * 2);
            strWide.ReleaseBuffer();
            ::GlobalUnlock(clipbuffer);
            ::SetClipboardData(CF_UNICODETEXT, clipbuffer);
            ::CloseClipboard();
            return TRUE;
        }
    }
    return FALSE;
}

void Paste()
{
    keybd_event(0x11, 0, 0, 0);// press ctrl
    keybd_event(0x56, 0, 0, 0); // press v
    keybd_event(0x56, 0, 2, 0); //release v
    keybd_event(0x11, 0, 2, 0); //release ctrl
}


// Handle interrupts, like Ctrl-C
static int s_signo;
static void signal_handler(int signo) {
    s_signo = signo;
}

static void cb(struct mg_connection* c, int ev, void* ev_data, void* fn_data) {
    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message* hm = (struct mg_http_message*)ev_data;

        //Read the incoming information
        //mg_http_reply(c, 200, NULL, "partmer: %.*s", (int)hm->query.len, hm->query.ptr);

        //mg_printf(c, "%s%s%s\r\n",
        //    "HTTP/1.1 200 OK\r\n"               // Output response line
        //    "Transfer-Encoding: chunked\r\n"   // Chunked header
        //"Content-Type: text/plain\r\n");    // and Content-Type header
        //mg_printf(c, "Request headers:\n");
        //size_t i, max = sizeof(hm->headers) / sizeof(hm->headers[0]);
        //for (i = 0; i < max && hm->headers[i].name.len > 0; i++) {
        //    struct mg_str* k = &hm->headers[i].name, * v = &hm->headers[i].value;
        //    mg_http_printf_chunk(c, "%.*s -> %.*s\n", (int)k->len, k->ptr, (int)v->len, v->ptr);
        //}
        //mg_http_write_chunk(c, "", 0);  // Final empty chunk

        // Create JSON response using mjson library call
        //char* requestJson = mjson_aprintf("{ code:1,data:%Q,message:%Q }", "54tgt534356h465gt5", "success");

        // Send response to the client
        //mg_printf(c, "%s", "HTTP/1.0 200 OK\r\n");                  // Response line
        //mg_printf(c, "%s", "Content-Type: application/json\r\n");   // One header
        //mg_printf(c, "Content-Length: %d\r\n", (int)strlen(requestJson)); // Another header
        //mg_printf(c, "%s", "\r\n");                                 // End of headers
        //mg_printf(c, "%s", requestJson);                                   // Body
        //free(requestJson);                                                 // Don't forget!

        //mg_http_reply(c, 200, NULL, "{ code:1,data:\"%s\",message:\"%s\" }", "54tgt534356h465gt5", "success");

        const char *setHeader = "Content-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Private-Network:true\r\n"; //Access-Control-Allow-Headers:ps-dataurlconfigqid\r\n";

        if (mg_http_match_uri(hm, "/v1/info")) {
            struct mg_str params = hm->query;
            string macAddr;
            bool isOK = false;
            isOK = getUUID(macAddr);
            if (!isOK || macAddr.empty() || "FFFFFFFF-FFFF-FFFF-FFFF-FFFFFFFFFFFF" == macAddr.c_str()) {
                GetMacByGetAdaptersAddresses(macAddr);
                //printf("MAC Address is %s\n", macAddr.c_str());
            }
            else {
                //printf("getUUID is %s\n", macAddr.c_str());
            }
            //GetMacByGetAdaptersAddresses(macAddr);
            if (macAddr.empty()) {
                mg_http_reply(c, 200, setHeader, "{ \"code\":-1,\"data\":%Q,\"message\":%Q }\n", "", "Fail");
            } else {
                mg_http_reply(c, 200, setHeader, "{ \"code\":0,\"data\":%Q,\"message\":%Q }\n", macAddr.c_str(), "Success");
            }
        } else {
            mg_http_reply(c, 200, setHeader, "{ \"code\":-1,\"data\":%Q,\"message\":%Q }\n", "", "Invalid URI");
        }
    }
    (void)fn_data;
}

void closeHttpServer() {
    mg_mgr_free(&mgr);
}

void  startHttpServer() {
    struct mg_connection* c;

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    mg_log_set(3);
    mg_mgr_init(&mgr);
    if ((c = mg_http_listen(&mgr, s_listening_address, cb, &mgr)) == NULL) {
        MG_ERROR(("Cannot listen on %s. Use http://ADDR:PORT or :PORT",
            s_listening_address));
        exit(EXIT_FAILURE);
    }
    MG_INFO(("Mongoose version : v%s", MG_VERSION));
    MG_INFO(("Listening on     : %s", s_listening_address));
    while (s_signo == 0) mg_mgr_poll(&mgr, 1000);
    closeHttpServer();
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // Store instance handles in global variables

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        return FALSE;
    }

    //The associated tray icon resource must be 16 * 16 or 32 * 32 pixels                                                       
    //nID.hIcon = LoadIcon(GetModuleHandle(0), MAKEINTRESOURCE(IDI_SMALL)); //System default icon
    nID.hIcon = (HICON)LoadImage(NULL, TEXT("logo32.ico"), IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
    //Prompt of tray icon, that is, the prompt that pops up when the mouse is placed on it
    //const char *appName = "Web Login Plug v1.0";
    //strncpy_s(nID.szTip, appName,sizeof(appName));
    //Text displayed when the mouse is placed on the tray icon
    wcscpy_s(nID.szTip, _T("Web Login Plug v1.0.2"));
    //Window associated with tray icon
    nID.hWnd = hWnd;
    //If the application has only one tray icon, you can set it at will
    nID.uID = 1;
    //Type of tray icon
    nID.uFlags = NIF_GUID | NIF_ICON | NIF_MESSAGE | NIF_TIP;
    //The message ID associated with the tray icon, which is the message ID of the left-click and right-click messages of the tray
    nID.uCallbackMessage = WM_MINMAXNotify;
    //Notify windows to add a tray icon
    Shell_NotifyIcon(NIM_ADD, &nID);

    ShowWindow(hWnd, SW_HIDE); //Hide Form
    //ShowWindow(hWnd, nCmdShow); //Display Form
    UpdateWindow(hWnd);

   return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);

            string macAddr;
            string tip1;
            wstring tip2;
            bool isOK = false;

            switch (wmId)
            {
                case IDM_HIDE_ICON:
                    Shell_NotifyIcon(NIM_DELETE, &nID);
                    break;
                case IDM_ABOUT:
                    //DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                    break;
                case COPY_MAC:
                    isOK = getUUID(macAddr);
                    if (!isOK || macAddr.empty() || "FFFFFFFF-FFFF-FFFF-FFFF-FFFFFFFFFFFF" == macAddr.c_str()) {
                        GetMacByGetAdaptersAddresses(macAddr);
                        //printf("MAC Address is %s\n", macAddr.c_str());
                    }
                    else {
                        //printf("getUUID is %s\n", macAddr.c_str());
                    }
                    //GetMacByGetAdaptersAddresses(macAddr);
                    if (Copy(macAddr.c_str())) {
                        tip1 = "Copy OK!\nYour MAC is\n" + macAddr;
                        tip2 = wstring(tip1.begin(), tip1.end());
                        MessageBox(GetForegroundWindow(), tip2.c_str(), TEXT("Result"), MB_OK);
                    } else {
                        //copy failed
                        MessageBox(GetForegroundWindow(), TEXT("Copy Fail!"), TEXT("Result"), MB_OK);
                    }
                    break;
                case IDM_EXIT:
                    switch (MessageBox(GetForegroundWindow(), TEXT("Are you sure you want to quit?"), TEXT("Tip"), MB_OKCANCEL)) {
                        case 1://IDOK
                            closeHttpServer();
                            Shell_NotifyIcon(NIM_DELETE, &nID);
                            DestroyWindow(hWnd);
                            break;
                        case 2://IDCANCEL
                            break;
                    }
                    break;
                default:
                    return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_MINMAXNotify:
        switch (lParam)
        {
            case WM_LBUTTONDOWN: // Left button
                //MessageBox(NULL, TEXT("Recv notify icon message"), TEXT("notify"), MB_ICONHAND);
                break;
            case WM_RBUTTONDOWN:// Right button
                POINT pt;
                GetCursorPos(&pt);
                HMENU hMenu;
                hMenu = CreatePopupMenu();
                AppendMenu(hMenu, MF_STRING, COPY_MAC, TEXT("Copy MAC Address"));
                AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
                AppendMenu(hMenu, MF_STRING, NULL, TEXT("version 1.0.2"));
                AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
                AppendMenu(hMenu, MF_STRING, IDM_HIDE_ICON, TEXT("Hide The Icon"));
                AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
                AppendMenu(hMenu, MF_STRING, IDM_EXIT, TEXT("Exit"));
                AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
                SetForegroundWindow(hWnd);
                TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, NULL, hWnd, NULL);
                break;
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code using hdc here
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        KillTimer(hWnd, 1);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for the About box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

// Equivalent to MAIN function Each Windows program contains an entry point function named WinMain or wWinMain
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    //todo
    //http server
    // Create asynchronous thread
    thread httpThread(&startHttpServer);
    httpThread.detach();
    //httpThread.join();
    //startHttpServer();

    //mac address
    //thread macThread(&GetMacByGetAdaptersAddresses,macAddr);
    //macThread.detach();

    // Initialize global string
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_PROJECT1, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    //Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_PROJECT1));

    MSG msg;

    //Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}
