//
//  app.cpp
//  Highwaycam
//
//  Created by Hao Zhou on 28/05/2020.
//  Copyright © 2020 John Boiles . All rights reserved.
//

#include "app.hpp"
#include "program.hpp"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>


App::App(GLFWwindow *window) : window(window), time(0.0f), server(this), compressQuality(50) {
    update();
    frames.push_back(std::pair<bool, Frame>(true, Frame(this, "Default camera", "Shaders/fragment.glsl")));
    frames.push_back(std::pair<bool, Frame>(true, Frame(this, "Fake camera", "Shaders/test.glsl")));
    frames.push_back(std::pair<bool, Frame>(false, Frame(this, "Invert pass", "Shaders/invert.glsl")));
    lastInstant = glfwGetTime();
    server.start();
    finalImageBuffer.first = (int) winSize.x * (int) winSize.y * 24;
    finalImageBuffer.second = new unsigned char[finalImageBuffer.first]; // RGB * W * H
}

void App::update() {
    float thisInstant = glfwGetTime();
    deltaTime = thisInstant - lastInstant;
    lastInstant = thisInstant;
    time += deltaTime;

    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    winSize = { (float) w / RETINA_MODIFIER, (float) h / RETINA_MODIFIER };
    
    server.respond();
}

void App::renderGUI() {
    glViewport(0, 0, winSize.x, winSize.y);
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (warnings.size() > 0) {
        configWindow({ 300.0f, 400.0f }, { 10.0f, 10.0f }, true);
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
    
    configWindow({ 150.0f, 80.0f }, { 10.0f, 220.0f }, false, true);
    ImGui::Begin("Export");
    if (ImGui::Button("Export to PPM")) {
        std::ofstream result("result.ppm");
        result << "P6" << std::endl << (int) winSize.x << " " << (int) winSize.y << std::endl << 255 << std::endl;
        result.write((char *) finalImageBuffer.second, finalImageBuffer.first);
        result.close();
    }
    if (ImGui::Button("Export to JPEG")) {
        updateCompressionStream();
        std::ofstream result("result.jpg");
        result << compressionStream.rdbuf();
        result.close();
    }
    ImGui::End();
    
    configWindow({ 400.0f, 250.0f }, { 10.0f, 240.0f }, false, true);
    ImGui::Begin("Stream settings");
    ImGui::SliderInt("Video quality", &compressQuality, 1, 100);
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void App::updateCompressionStream() {
    compressionStream.str("");
    stbi_write_jpg_to_func([](void *ctx, void *data, int size) {
        App *thiz = (App *) ctx;
        thiz->compressionStream.write((char *) data, size);
    }, this, winSize.x, winSize.y, 3, finalImageBuffer.second, compressQuality);
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
        updateFinalImageBuffer();
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

void App::configWindow(glm::vec2 size, glm::vec2 pos, bool inverse, bool collapsed) {
    ImGui::SetNextWindowSize({ size.x, size.y }, ImGuiCond_FirstUseEver);
    glm::vec2 rpos = pos;
    if (inverse) {
        rpos = winSize - size - rpos;
    }
    ImGui::SetNextWindowCollapsed(collapsed, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos({ rpos.x, rpos.y }, ImGuiCond_FirstUseEver);
}

void App::updateFinalImageBuffer() {
    Frame *latest = nullptr;
    int latestIndex = -1;
    for (int i = 0; i < frames.size(); i++) {
        if (frames[i].first && i > latestIndex) {
            latest = &frames[i].second;
            latestIndex = i;
        }
    }
    if (!latest) {
        return;
    }
    latest->render();
//    int size = (int) latest->size.x * (int) latest->size.y * 24 * 4;
    glBindTexture(GL_TEXTURE_2D, latest->texture);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, finalImageBuffer.second);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void App::quit() {
    delete[] finalImageBuffer.second;
    server.stop();
}
