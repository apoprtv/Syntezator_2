#pragma once
#include "imgui_internal.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
//#include <iostream>

#include <windows.h>
#include <xaudio2.h>
#include <iostream>
#include <cmath>

#include "Synthesizer.h"

// Define constants
const int SAMPLE_RATE = 44100;
const float PI = 3.14159265358979323846f;
const float TWO_PI = 2.0f * PI;

class voice
{
public:

    IXAudio2SourceVoice* pSourceVoice;

    XAUDIO2_VOICE_STATE state;

    bool GetState()
    {
        pSourceVoice->GetState(&state);

        if (state.BuffersQueued == 0)
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }

    float duration = 1.0f; // Duration in seconds

    const int numSamples = static_cast<int>(SAMPLE_RATE * duration);
    float* pBuffer = new float[numSamples];
    bool pBufferEmpty = 1;

    float attack = 1.1f;
    float decay = 1.0f;
    float sustain = 1.0f;
    float release = 1.0f;

    bool released = false;
    float lastVolume = 0;

    void releasedFunc()
    {
        if (released == false)
        {
            released = true;
            timer = 1;
            pSourceVoice->GetVolume(&lastVolume);
        }
    }

    // Attack

    void update(Synthesizer* synth)
    {
        attack = synth->attack;

        XAUDIO2_FILTER_PARAMETERS filterParameters = {};
        filterParameters.Type = LowPassFilter; // Choose the type of filter: LowPassFilter, HighPassFilter, BandPassFilter

        // Calculate the normalized frequency value
        float nyquistFrequency = 44100 / 2;
        float normalizedFrequency = synth->filterCutoff / nyquistFrequency;

        filterParameters.Frequency = normalizedFrequency; // Frequency (1.0 = Nyquist frequency)
        filterParameters.OneOverQ = 1.0f;   // Quality factor (1.0 = normal quality)

        pSourceVoice->SetFilterParameters(&filterParameters);


        if (isActive == 0)
        {
            timer = -10;
        }

        if (isActive == 1)
        {

            if (timer < 0)
            {
                pSourceVoice->SetVolume(0);
            }

            if (timer / (1000 * attack) < 1 && timer >= 0)
            {
                pSourceVoice->SetVolume(exp(-1.0f * timer / (1000 * attack)) * 0.1 * (timer / (1000 * attack)));
            }

            if (released == true)
            {
                pSourceVoice->SetVolume(lastVolume / (timer));
            }

            if (released == true && timer >= 1000 * release)
            {
                pSourceVoice->SetVolume(0);
                pSourceVoice->Stop(0);
                isActive = 0;
                prepare();
            }

            timer++;
        }
    }

    void prepare()
    {
        for (int i = 0; i < numSamples; i++)
        {
            pBuffer[i] = NULL;
        }

        pSourceVoice->SetVolume(0.0f);
        pBufferEmpty = 1;
        isActive = 0;
        key = -1;
        timer = -10;
        released = false;
    }

    voice(IXAudio2& pXAudio2, WAVEFORMATEX& wfx, IXAudio2VoiceCallback& voiceCallback)
    {

        pXAudio2.CreateSourceVoice(&pSourceVoice, &wfx, XAUDIO2_VOICE_USEFILTER, XAUDIO2_DEFAULT_FREQ_RATIO, &voiceCallback);

        pSourceVoice->SetVolume(0.1f);

        pSourceVoice->SetSourceSampleRate(SAMPLE_RATE);

        XAUDIO2_FILTER_PARAMETERS filterParameters = {};
        filterParameters.Type = LowPassFilter; // Choose the type of filter: LowPassFilter, HighPassFilter, BandPassFilter
        filterParameters.Frequency = 1.0f; // Frequency (1.0 = Nyquist frequency)
        filterParameters.OneOverQ = 1.0f;   // Quality factor (1.0 = normal quality)

        pSourceVoice->SetFilterParameters(&filterParameters);
    }

    bool isActive = 0;

    int noteIndex = 0;

    int key = -1;

    ~voice()
    {
        delete pSourceVoice;
    }

    int getTimer() {
        return timer;
    }

private:
    int timer = 0;
};