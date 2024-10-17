#include <windows.h>
#include <iostream>
#include <string>

DWORD WINAPI ThreadProc(LPVOID lpParam) {
    int threadNum = *reinterpret_cast<int*>(lpParam);
    std::cout << "Поток №" << threadNum << " выполняет свою работу" << std::endl;
    ExitThread(0);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Использование: " << argv[0] << " <количество потоков>" << std::endl;
        return 1;
    }

    int numThreads = std::stoi(argv[1]);
    if (numThreads < 1) {
        std::cerr << "Количество потоков должно быть больше 0" << std::endl;
        return 1;
    }

    HANDLE* threadHandles = new HANDLE[numThreads];
    int* threadParams = new int[numThreads];

    for (int i = 0; i < numThreads; ++i) {
        threadParams[i] = i + 1;
        threadHandles[i] = CreateThread(NULL, 0, ThreadProc, &threadParams[i], 0, NULL);

        if (threadHandles[i] == NULL) {
            std::cerr << "Ошибка создания потока " << i + 1 << std::endl;
            return 1;
        }
    }

    WaitForMultipleObjects(numThreads, threadHandles, TRUE, INFINITE);

    for (int i = 0; i < numThreads; ++i) {
        CloseHandle(threadHandles[i]);
    }
    delete[] threadHandles;
    delete[] threadParams;

    return 0;
}
