#pragma once

void setupImGui(GLFWwindow* window) {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");
}

class sineDrawer
{
public:
    float phase = 0.0f;
};

void renderImGui(float& frequency, float& attack, int& waveform, Synthesizer* synth, sineDrawer* sD) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Set the next window position and size
    ImGui::SetNextWindowPos(ImVec2(5, 5), ImGuiCond_Always);    // Set position
    ImGui::SetNextWindowSize(ImVec2(1280, 720), ImGuiCond_Always); // Set size
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(1.0f, 1.0f, 1.0f, 0.00f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 1.0f, 1.0f, 0.00f));

    ImGui::Begin("Synthesizer", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    ImGui::Text("Simple Synthesizer");
    ImGui::PushItemWidth(300);
    // Change the color of the slider

    const auto color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

    ImGui::PushStyleColor(ImGuiCol_SliderGrab, color); // Change grab handle color to red
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(1.0f, 1.0f, 1.0f, 1.0f)); // Change active grab handle color to green
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1.0f, 1.0f, 1.0f, 0.1f)); // Change grab handle color to red
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.15f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(1.0f, 1.0f, 1.0f, 0.2f));
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(1.0f, 1.0f, 1.0f, 0.1f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.15f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(1.0f, 1.0f, 1.0f, 0.2f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 1.0f, 1.0f, 0.1f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.15f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 1.0f, 1.0f, 0.2f));

    ImGui::SliderFloat("Attack", &attack, 0.01f, 10.0f, "Attack: %.2f");

    // Get the draw list
    ImDrawList* draw_list2 = ImGui::GetWindowDrawList();

    // Define the position and size of the button
    ImVec2 slider_pos = ImGui::GetCursorScreenPos();
    ImVec2 slider_size = ImVec2(200, 10);

    ImGui::SliderInt("Max Voices", &synth->maxVoices, 1, 12, "Max Voices: %i");

    ImGui::SliderInt("Filter Cutoff", &synth->filterCutoff, 20, 20000, "Filter Cutoff: %i");

    const char* waveformTypes[] = { "Sine", "Square", "Sawtooth" };
    ImGui::Combo("Waveform", &waveform, waveformTypes, IM_ARRAYSIZE(waveformTypes));

    ImGui::SetCursorPos(ImVec2(420, 20));

    ImGui::VSliderInt("Octave", ImVec2(20, 150), &synth->octave, -3, 2, "%i");

    // Get the draw list for the current window
    ImDrawList* draw_list3 = ImGui::GetWindowDrawList();

    // Define the position and size of the box
    ImVec2 p = ImVec2(40 - 2, 400 - 57);
    ImVec2 size = ImVec2(300 + 4, 114);
    ImVec2 p1 = ImVec2(p.x, p.y);
    ImVec2 p2 = ImVec2(p.x + size.x, p.y + size.y);

    // Draw the box (rectangle)
    draw_list3->AddRect(p1, p2, IM_COL32(255, 255, 255, 255)); // Red box

    ImGui::PopStyleColor(13);

    ImVec4 window_bg_color = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    ImGui::GetStyle().Colors[ImGuiCol_WindowBg] = window_bg_color;

    // filter graphic

    ImDrawList* draw_list_filter = ImGui::GetWindowDrawList();

    ImVec2 filterOrigin = ImVec2(400, 343);

    draw_list_filter->AddRect(filterOrigin, filterOrigin + ImVec2(300, 114), IM_COL32(255, 255, 255, 255));

    draw_list_filter->AddBezierCubic(filterOrigin + ImVec2(2, 30), filterOrigin + ImVec2(float(synth->filterCutoff) / 20000 * 300, 30), filterOrigin + ImVec2(float(synth->filterCutoff) / 20000 * 300, 30), filterOrigin + ImVec2(float(synth->filterCutoff) / 20000 * 300, 114), IM_COL32(255, 255, 255, 255), 2.0f, 0);

    // Get the draw list
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // Define the wave parameters
    ImVec2 origin = ImVec2(40, 400);   // Origin point of the wave
    float wave_length = 300.0f;       // Length of the wave in pixels

    int num_points = 1000 / 25;
    //float phase = 0.0f;

    sD->phase += 0.005f;

    if (sD->phase > 2 * PI)
    {
        sD->phase = 0;
    }

    // Draw the sine wave
    if (synth->waveform == 0)
    {
        for (int i = 0; i < num_points - 1; ++i) {
            float t1 = (float)i / (float)(num_points - 1);
            float t2 = (float)(i + 1) / (float)(num_points - 1);
            ImVec2 p1 = ImVec2(origin.x + t1 * wave_length, origin.y + 50 * sin(2 * M_PI * 1 * t1 + sD->phase));
            ImVec2 p2 = ImVec2(origin.x + t2 * wave_length, origin.y + 50 * sin(2 * M_PI * 1 * t2 + sD->phase));
            draw_list->AddLine(p1, p2, IM_COL32(255, 255, 255, 255), 2.0f);
        }
    }

    if (synth->waveform == 1)
    {
        for (int i = 0; i < 1; i++)
        {
            ImVec2 p1 = ImVec2(origin.x + 40 * i, origin.y + 50);
            ImVec2 p2 = ImVec2(origin.x + 150 + 40 * i, origin.y + 50);
            ImVec2 p3 = ImVec2(origin.x + 150 + 40 * i, origin.y - 50);
            ImVec2 p4 = ImVec2(origin.x + 300 + 40 * i, origin.y - 50);
            draw_list->AddLine(p1, p2, IM_COL32(255, 255, 255, 255), 2.0f);
            draw_list->AddLine(p2, p3, IM_COL32(255, 255, 255, 255), 2.0f);
            draw_list->AddLine(p3, p4, IM_COL32(255, 255, 255, 255), 2.0f);
        }
    }

    if (synth->waveform == 2)
    {
        for (int i = 0; i < 1; i++)
        {
            ImVec2 p1 = ImVec2(origin.x + 40 * i, origin.y);
            ImVec2 p2 = ImVec2(origin.x + 150 + 40 * i, origin.y - 50);
            ImVec2 p3 = ImVec2(origin.x + 150 + 40 * i, origin.y + 50);
            ImVec2 p4 = ImVec2(origin.x + 300 + 40 * i, origin.y);
            draw_list->AddLine(p1, p2, IM_COL32(255, 255, 255, 255), 2.0f);
            draw_list->AddLine(p2, p3, IM_COL32(255, 255, 255, 255), 2.0f);
            draw_list->AddLine(p3, p4, IM_COL32(255, 255, 255, 255), 2.0f);
        }

    }

    ImDrawList* draw_list4 = ImGui::GetWindowDrawList();

    ImVec2 p1_line = ImVec2(origin.x, origin.y);
    ImVec2 p2_line = ImVec2(origin.x + 300, origin.y);

    draw_list4->AddLine(p1_line, p2_line, IM_COL32(255, 255, 255, 105), 1.0f);

    ImGui::End();

    // Rendering
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.00f, 0.00f, 0.00f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}