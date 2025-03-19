#pragma once
#include "imgui_internal.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

#include <windows.h>
#include <xaudio2.h>
#include <iostream>
#include <cmath>


class Synthesizer
{
public:

    int maxVoices = 12;
    float volume = 0.1f;
    int waveform = 0;
    float attack = 0.1f;
    int octave = 0;
    int filterCutoff = 9000;
};