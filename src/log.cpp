#include "header.h"

QeLog::~QeLog() {
    if (ofile.is_open()) {
        ofile.close();
    }
}

QeDebugMode QeLog::mode() { return QeDebugMode(atoi(AST->getXMLValue(4, AST->CONFIG, "setting", "environment", "debug"))); }

bool QeLog::isDebug() { return mode() != eModeNoDebug_bit; }
bool QeLog::isLogPanel() { return mode() & eModeLogPanel_bit; }
bool QeLog::isOutput() { return mode() & eModeOutput_bit; }

std::string QeLog::stack(int from, int to) {
    std::string ret = "";

    void **backTrace = new void *[to];

    const USHORT nFrame = CaptureStackBackTrace(from, to, backTrace, nullptr);

    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);

    HANDLE hProcess = GetCurrentProcess();
    SymInitialize(hProcess, NULL, TRUE);

    for (USHORT iFrame = 0; iFrame < nFrame; ++iFrame) {
        DWORD64 displacement64;
        char symbol_buffer[sizeof(SYMBOL_INFO) + 256];
        SYMBOL_INFO *symbol = reinterpret_cast<SYMBOL_INFO *>(symbol_buffer);
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = 255;
        SymFromAddr(hProcess, (DWORD64)backTrace[iFrame], &displacement64, symbol);

        DWORD displacement;
        IMAGEHLP_LINE64 line;
        line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
        SymGetLineFromAddr64(GetCurrentProcess(), (DWORD64)backTrace[iFrame], &displacement, &line);

        char s[512];

        sprintf_s(s, "\n%1d %s %d", iFrame, symbol->Name, line.LineNumber);
        ret.append(s);
    }
    SymCleanup(hProcess);

    delete[] backTrace;

    return ret;
}

void QeLog::print(std::string &msg, bool bShowStack, int stackLevel) {
    if (this == nullptr || !isDebug()) return;

    time_t rawtime;
    struct tm timeinfo;
    char buffer[128];

    time(&rawtime);
    localtime_s(&timeinfo, &rawtime);

    strftime(buffer, sizeof(buffer), "%y%m%d%H%M%S ", &timeinfo);
    std::string s = buffer;
    s += msg;

    if (bShowStack) s += stack(2, stackLevel);

    if (isLogPanel()) WIN->Log(s);
    if (isOutput()) {
        if (!ofile.is_open()) {
            time_t rawtime;
            struct tm timeinfo;
            char buffer[128];

            time(&rawtime);
            localtime_s(&timeinfo, &rawtime);

            strftime(buffer, sizeof(buffer), "%y%m%d%H%M%S", &timeinfo);
            std::string outputPath = AST->getXMLValue(4, AST->CONFIG, "setting", "path", "log");
            outputPath += "log";
            outputPath += buffer;
            outputPath += ".txt";

            ofile.open(outputPath);
        }
        //ofile.seekp(ofile.beg);
        ofile << s << std::endl;
        //ofile.flush();
    }
}
