//
//  app.cpp
//  Highwaycam
//
//  Created by Hao Zhou on 28/05/2020.
//  Copyright © 2020 John Boiles . All rights reserved.
//

#include "app.hpp"
#include "program.hpp"


App::App(GLFWwindow *window) : window(window), time(0.0f) {
    update();
    frames.push_back(std::pair<bool, Frame>(true, Frame(this, "Default camera", "Shaders/fragment.glsl")));
    frames.push_back(std::pair<bool, Frame>(false, Frame(this, "Invert pass", "Shaders/invert.glsl")));
    lastInstant = glfwGetTime();
    doInvert = false;
}

void App::update() {
    float thisInstant = glfwGetTime();
    deltaTime = thisInstant - lastInstant;
    lastInstant = thisInstant;
    time += deltaTime;

    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    winSize = { (float) w, (float) h };
}

void App::renderGUI() {
    glViewport(0, 0, winSize.x, winSize.y);
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    if (warnings.size() > 0) {
        configWindow({ 300.0f, 500.0f }, { 10.0f, 10.0f }, true);
        ImGui::Begin("Warnings");
        ImGui::TextWrapped("Uh oh, looks like there are warnings. The application might or might not run normally.");
        ImGui::Separator();
        for (int i = 0; i < warnings.size(); i++) {
            ImGui::TextWrapped("%d: %s", (i + 1), warnings[i].c_str());
        }
        if (ImGui::Button("Dismiss")) {
            warnings.clear();
        }
        ImGui::End();
    }
    configWindow({ 400.0f, 200.0f }, { 10.0f, 10.0f });
    ImGui::Begin("Passes");
    for (int i = 0; i < frames.size(); i++) {
        ImGui::Checkbox(frames[i].second.name.c_str(), &frames[i].first);
    }
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void App::mainLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        update();
        Frame *currentFrame = nullptr;
        for (int i = 0; i < frames.size(); i++) {
            if (!frames[i].first) { continue; }
            if (!currentFrame) {
                currentFrame = &frames[i].second;
                continue;
            }
            currentFrame = &(currentFrame->chain(frames[i].second));
        }
        if (currentFrame) {
            currentFrame->renderToScreen();
        }
        renderGUI();
        glfwSwapBuffers(window);
    }
}

void App::start() {
    // === IMGUI CREATION === //
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    io.IniFilename = nullptr;

    mainLoop();
}

void App::configWindow(glm::vec2 size, glm::vec2 pos, bool inverse) {
    ImGui::SetNextWindowSize({ size.x, size.y });
    glm::vec2 rpos = pos;
    if (inverse) {
        rpos = winSize / 2.0f - size - rpos;
    }
    ImGui::SetNextWindowPos({ rpos.x, rpos.y });
}

