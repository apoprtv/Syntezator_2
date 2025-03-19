#define _USE_MATH_DEFINES
#define IMGUI_DEFINE_MATH_OPERATORS

#include "imgui_internal.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

#include <windows.h>
#include <xaudio2.h>
#include <iostream>
#include <cmath>

#include "Synthesizer.h"
#include "voice.h"
#include "gui.h"


using namespace std;

class VoiceCallback : public IXAudio2VoiceCallback
{
public:
    HANDLE hBufferEndEvent;
    VoiceCallback() : hBufferEndEvent(CreateEvent(NULL, FALSE, FALSE, NULL)) {}
    ~VoiceCallback() { CloseHandle(hBufferEndEvent); }

    //Called when the voice has just finished playing a contiguous audio stream.
    void OnStreamEnd() { SetEvent(hBufferEndEvent); }

    //Unused methods are stubs
    void OnVoiceProcessingPassEnd() { }
    void OnVoiceProcessingPassStart(UINT32 SamplesRequired) {    }
    void OnBufferEnd(void* pBufferContext)
    {

    }
    void OnBufferStart(void* pBufferContext) {    }
    void OnLoopEnd(void* pBufferContext) {    }
    void OnVoiceError(void* pBufferContext, HRESULT Error) { }
};

class note
{
public:

    template<class T>
    note(T freq)
    {
        frequency = freq;
    }

    float frequency;
};

// Sine wave generator function

class Generation {
public:
    float frequency;
    float time;

    virtual float generate(float frequency, float time) = 0;
};

class Saw: public Generation {
public:
    float generate(float frequency, float time) {
        try {
            if (frequency <= 0) {
                throw 0;
            }
            else {
                return 2.0 * (time * frequency - floor(time * frequency + 0.5));
            }
        }
        catch(int a){
            cout << "frequency <= 0";
        }
    }
};

class Sin: public Generation {
public:
    float generate(float frequency, float time) {
        try {
            if (frequency <= 0) {
                throw 0;
            }
            else {
                return sin(TWO_PI * frequency * time);
            }
        }
        catch (int a) {
            cout << "frequency <= 0";
        }
    }
};

class Square: public Generation {
public:
    float generate(float frequency, float time) {
        double phase = fmod(time, 1.0 / frequency);
        try {
            if (frequency <= 0) {
                throw 0;
            }
            else {
                return (phase < 0.5 / frequency) ? 1.0 : -1.0;
            }
        }
        catch (int a) {
            cout << "frequency <= 0 error";
        }
    }
};

int findFreeVoice(voice voices[])
{
    for (int i = 0; i < 12; i++)
    {
        if (voices[i].isActive == 0)
        {
            return i;
        }
    }
    if (voices[11].isActive == 1)
    {
        return 12;
    }
    return 220;
}

int findFreeVoice2(voice* Voices[], int maxVoices)
{
    for (int i = 0; i < maxVoices; i++)
    {
        if (Voices[i]->isActive == 0)
        {
            return i;
        }

        if (Voices[i]->isActive == 1 && i == maxVoices - 1)
        {
            return -1;
        }
    }
}

int main() {
    // Initialize COM
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    sineDrawer sineDrawer;

    glfwInit();

    Sin sineGenerator;

    Saw sawGenerator;

    Square squareGenerator;

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Dear ImGui Synthesizer", NULL, NULL);

    glfwMakeContextCurrent(window);

    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    setupImGui(window);

    Synthesizer synthesizer;

    note* notes[96];

    notes[57] = new note(440.0f);

    float previousNoteFrequency = 440.0f;

    for (int i = 0; i < 9; i++)
    {
        notes[57 - i - 1] = new note(previousNoteFrequency / pow<float, float>(2, 1.0f / 12));
        previousNoteFrequency = notes[57 - i - 1]->frequency;
    }

    previousNoteFrequency = 440.0f;

    for (int i = 0; i < 2; i++)
    {
        notes[57 + i + 1] = new note(previousNoteFrequency * pow<float, float>(2, 1.0f / 12));
        previousNoteFrequency = notes[57 + i + 1]->frequency;
    }

    for (int i = 0; i < 48; i++)
    {
        notes[48 - i - 1] = new note(notes[48 - i - 1 + 12]->frequency / 2);
    }

    for (int i = 0; i < 48; i++)
    {
        notes[48 + i + 1] = new note(notes[48 + i + 1 - 12]->frequency * 2);
    }

    // indeksy nut ida tak, ze notes[60] to jest c5

    int x = 1;

    // Initialize XAudio2
    IXAudio2* pXAudio2;
    HRESULT hr = XAudio2Create(&pXAudio2);
    if (FAILED(hr)) {
        std::cerr << "Failed to initialize XAudio2." << std::endl;
        CoUninitialize();
        return 1;
    }

    // Create mastering voice
    IXAudio2MasteringVoice* pMasterVoice;
    hr = pXAudio2->CreateMasteringVoice(&pMasterVoice);
    if (FAILED(hr)) {
        std::cerr << "Failed to create mastering voice." << std::endl;
        pXAudio2->Release();
        CoUninitialize();
        return 1;
    }

    WAVEFORMATEX wfx = {};
    wfx.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    wfx.nChannels = 1;
    wfx.nSamplesPerSec = SAMPLE_RATE;
    wfx.nAvgBytesPerSec = SAMPLE_RATE * sizeof(float);
    wfx.nBlockAlign = sizeof(float);
    wfx.wBitsPerSample = 32;
    wfx.cbSize = 0;

    VoiceCallback voiceCallback;

    voice* voices[12];

    for (int i = 0; i < 12; i++)
    {
        voices[i] = new voice(*pXAudio2, wfx, voiceCallback);
    }

    XAUDIO2_BUFFER* buffers[12];

    for (int i = 0; i < 12; i++)
    {
        buffers[i] = new XAUDIO2_BUFFER();
        buffers[i]->pAudioData = nullptr;
    }

    bool keysPressed[29];

    string activeKeysString = "ZSXDCVGBHNJMQ2W3ER5T6Y7UI9O0P";

    while (!glfwWindowShouldClose(window))
    {

        if (synthesizer.maxVoices < 1 || synthesizer.maxVoices > 12) {
            synthesizer.maxVoices = 12;
        }

        glfwPollEvents();

        float frequency = 1.0f;

        float amplitude = 1;

        int waveform = 1;

        renderImGui(frequency, synthesizer.attack, synthesizer.waveform, &synthesizer, &sineDrawer);

        glfwSwapBuffers(window);
        
        // Wieloglosowosc

        // Detekcja wcisniecia klawiszy

        // indeksy keysPressed idą tak, że 0 to przycisk Z (c3), a nastepnie jak klawiatura pianina, czyli na przykład jeśli chcę zagrać c4 to powinienem wcinąć Q, jeśli c#4 - S itd

        for (int i = 0; i < 29; i++)
        {
            if (GetAsyncKeyState(activeKeysString[i]) & 0x8000)
            {
                keysPressed[i] = 1;
            }
            else
            {
                keysPressed[i] = 0;
            }
        }

        float duration = 1.0f;
        const int numSamples = static_cast<int>(SAMPLE_RATE * duration);

        //incjalizacja bufferow dla kazdego glosu
        for (int i = 0; i < 12; i++)
        {
            buffers[i]->pAudioData = reinterpret_cast<BYTE*>(voices[i]->pBuffer);
            buffers[i]->Flags = XAUDIO2_END_OF_STREAM;
            buffers[i]->AudioBytes = numSamples * sizeof(float);
            buffers[i]->LoopCount = 0;
            buffers[i]->LoopBegin = 0;
            buffers[i]->LoopLength = 0;
            buffers[i]->pContext = nullptr;
        }

        for (int j = 0; j < 29; j++)
        {
            if (keysPressed[j])
            {
                int freeVoiceIndex = -1;

                bool skip = false;

                // jesli nuta juz jest grana, pomijamy ja
                for (int i = 0; i < 12; i++)
                {
                    if (voices[i]->key == j)
                    {
                        skip = true;
                    }
                }

                if (skip == false)
                {
                    freeVoiceIndex = findFreeVoice2(voices, synthesizer.maxVoices);

                    // generacja dzwieku i wpisywanie danych do buffera
                    if (freeVoiceIndex >= 0)
                    {

                        // sprawdzenie czy indeks nuty nie wykracza poza array nut
                        try {
                            if (48 + j + 12 * synthesizer.octave > 0 && 48 + j + 12 * synthesizer.octave < 95) {
                                voices[freeVoiceIndex]->key = j;
                                voices[freeVoiceIndex]->isActive = 1;

                                for (int i = 0; i < voices[freeVoiceIndex]->numSamples; i++)
                                {
                                    voices[freeVoiceIndex]->duration = 1.0f / notes[48 + j + 12 * synthesizer.octave]->frequency * ceil(notes[48 + j + 12 * synthesizer.octave]->frequency);

                                    if (synthesizer.waveform == 0)
                                    {
                                        voices[freeVoiceIndex]->pBuffer[i] = sineGenerator.generate(notes[48 + j + 12 * synthesizer.octave]->frequency, voices[freeVoiceIndex]->duration / voices[freeVoiceIndex]->numSamples * i);
                                    }
                                    if (synthesizer.waveform == 1)
                                    {
                                        voices[freeVoiceIndex]->pBuffer[i] = squareGenerator.generate(notes[48 + j + 12 * synthesizer.octave]->frequency, voices[freeVoiceIndex]->duration / voices[freeVoiceIndex]->numSamples * i);
                                    }
                                    if (synthesizer.waveform == 2)
                                    {
                                        voices[freeVoiceIndex]->pBuffer[i] = sawGenerator.generate(notes[48 + j + 12 * synthesizer.octave]->frequency, voices[freeVoiceIndex]->duration / voices[freeVoiceIndex]->numSamples * i);
                                    }
                                    voices[freeVoiceIndex]->isActive = 1;
                                }
                            }
                            else {
                                throw 0;
                            }                        
                        }
                        catch (int a) {
                            cout << "nonexistent note error";
                        }
                    }
                }

            }
        }

        int keyPressSum = 0;

        for (int i = 0; i < 29; i++)
        {
            if (keysPressed[i])
            {
                keyPressSum++;
            }
        }
        if (keyPressSum > 0)
        {
            for (int i = 0; i < 12; i++)
            {
                if (voices[i]->getTimer() <= 0)
                {
                    voices[i]->pSourceVoice->SetVolume(0);
                }
                                
                buffers[i]->pAudioData = reinterpret_cast<BYTE*>(voices[i]->pBuffer);
                voices[i]->pSourceVoice->SubmitSourceBuffer(buffers[i]);

                voices[i]->pSourceVoice->Start(0);
            }

        }
        else
        {
            for (int i = 0; i < 12; i++)
            {
                voices[i]->pSourceVoice->Stop(0);
                voices[i]->pSourceVoice->FlushSourceBuffers();
                voices[i]->prepare();
            }
        }

        for (int i = 0; i < 12; i++)
        {
            if (voices[i]->key >= 0 && keysPressed[voices[i]->key] == 0)
            {
                voices[i]->pSourceVoice->Stop(0);
                voices[i]->pSourceVoice->FlushSourceBuffers();
                voices[i]->prepare();
            }
        }

        for (int i = 0; i < 12; i++)
        {
            if (voices[i]->isActive == 1)
            {
                voices[i]->update(&synthesizer);
            }
        }
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    // Cleanup
    pMasterVoice->DestroyVoice();
    pXAudio2->Release();
    CoUninitialize();

    return 0;
}
