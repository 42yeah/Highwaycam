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
#include <filesystem>
#include <imgui_widgets.cpp>


App::App(GLFWwindow *window) : window(window), time(0.0f), server(this), compressQuality(50), renderSendRatio(1), horizontalFlip(true) {
    update();
//    frames.push_back(std::pair<bool, Frame>(true, Frame(this, "Default camera", "Shaders/fragment.glsl")));
//    frames.push_back(std::pair<bool, Frame>(true, Frame(this, "Fake camera", "Shaders/test.glsl")));
//    frames.push_back(std::pair<bool, Frame>(false, Frame(this, "Invert pass", "Shaders/invert.glsl")));
//    frames.push_back(std::pair<bool, Frame>(false, Frame(this, "Noise pass", "Shaders/noisify.glsl")));
    lastInstant = glfwGetTime();
    server.start();
    finalImageBuffer.first = (int) winSize.x * (int) winSize.y * 24;
    finalImageBuffer.second = new unsigned char[finalImageBuffer.first]; // RGB * W * H
    realCamera = Camera(this, 1);
    updateFrames();
    numFramesRendered = 0;
}

void App::update() {
    float thisInstant = glfwGetTime();
    deltaTime = thisInstant - lastInstant;
    lastInstant = thisInstant;
    time += deltaTime;

    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    winSize = { (float) w / RETINA_MODIFIER, (float) h / RETINA_MODIFIER };
    
    // === STREAM CAMERA TEXTURE === //
    realCamera.read();
    
    if (numFramesRendered >= renderSendRatio) {
        numFramesRendered = 0;
        server.respond();
    }
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
    helpMarker("You can drag checkboxes around to reorder passes.");
    
    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    ImGui::Button("Duplicate"); ImGui::SameLine(); // Just for the asthetics
    ImGui::Button("Delete"); ImGui::SameLine();
    bool trash = true;
    ImGui::Checkbox(realCamera.frame.name.c_str(), &trash);
    ImGui::PopItemFlag();
    ImGui::PopStyleVar();
    
    for (int i = 0; i < frames.size(); i++) {
        ImGui::PushID(i);
        if (ImGui::Button("Duplicate")) {
            std::pair<bool, Frame> frame = frames[i];
            frame.second.initTexture();
            frames.insert(frames.begin() + i, frame);
        }
        ImGui::SameLine();
        if (ImGui::Button("Delete")) {
            frames[i].second.destroyTexture();
            frames.erase(frames.begin() + i, frames.begin() + i + 1);
            ImGui::PopID();
            continue;
        }
        ImGui::SameLine();
        ImGui::Checkbox(frames[i].second.name.c_str(), &frames[i].first);
        
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
            ImGui::SetDragDropPayload("FRAME", &i, sizeof(int));
            ImGui::Text("%s", frames[i].second.name.c_str());
            ImGui::EndDragDropSource();
        }
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("FRAME")) {
                int swap = *(int *) payload->Data;
                std::pair<bool, Frame> tmp = frames[swap];
                frames[swap] = frames[i];
                frames[i] = tmp;
            }
            ImGui::EndDragDropTarget();
        }
        ImGui::PopID();
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
    ImGui::SliderInt("Render-Send ratio", &renderSendRatio, 1, 100);
    ImGui::Checkbox("Flip video horizontally", &horizontalFlip);
    for (int i = 0; i < sliders.size(); i++) {
        ImGui::SliderFloat(sliders[i].first.c_str(), &sliders[i].second, 0.0f, 1.0f);
    }
    ImGui::End();
    
    configWindow({ 500.0f, 250.0f }, { 10.0f, 260.0f }, false, true);
    ImGui::Begin("Default camera");
    ImGui::End();

//    ImGui::ShowDemoWindow();
    
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
                currentFrame->prevPass = realCamera.frame.texture;
                continue;
            }
            currentFrame = &(currentFrame->chain(frames[i].second));
        }
        if (currentFrame) {
            currentFrame->renderToScreen();
        } else {
            realCamera.frame.renderToScreen();
        }
        renderGUI();
        updateFinalImageBuffer();
        glfwSwapBuffers(window);
        numFramesRendered++;
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
        latest = &realCamera.frame;
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

void App::helpMarker(std::string desc) {
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc.c_str());
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void App::updateFrames() {
    namespace fs = std::__fs::filesystem;
    std::string blacklist[] = { "vertex.glsl", "camera.glsl" };
    for (const auto &entry : fs::directory_iterator("Shaders")) {
        std::string name = entry.path().filename().string();
        std::string suffix = ".glsl";
        bool blacklisted = false;
        for (std::string &banned : blacklist) {
            if (name == banned || !std::equal(suffix.rbegin(), suffix.rend(), name.rbegin())) {
                blacklisted = true;
                break;
            }
        }
        if (!blacklisted) {
            frames.push_back(readFrame(entry.path().string()));
        }
    }
    
    struct {
        bool operator()(std::pair<bool, Frame> a, std::pair<bool, Frame> b) {
            return a.second.weight > b.second.weight;
        }
    } cmp;
    std::sort(frames.begin(), frames.end(), cmp);
}

std::pair<bool, Frame> App::readFrame(std::string path) {
    std::ifstream reader(path);
    std::string line;
    std::getline(reader, line, '\n');
    std::getline(reader, line, '\n');
    
    bool activated = false;
    std::string name = path;
    int weight = 0;
    
    std::cout << line << std::endl;
    Frame frame;
    while (line.rfind("// ", 0) == 0 || line == "") {
        if (line == "" || line.length() <= 3) {
            std::getline(reader, line, '\n');
            continue;
        }
        std::string data = line.substr(3);
        
        auto pos = data.find(": ");
        if (pos == std::string::npos) {
            std::getline(reader, line, '\n');
            continue;
        }

        std::string key = data.substr(0, pos);
        std::string value = data.substr(pos + 2);
        if (key == "name") {
            name = value;
        } else if (key == "enabled") {
            activated = value == "true";
        } else if (key == "weight") {
            try {
                weight = std::stoi(value);
            } catch (std::exception e) {
                warnings.push_back(name + " has invalid weight defined: " + value);
            }
        } else if (key == "warning") {
            warnings.push_back(value);
        } else if (key == "slider") {
            sliders.push_back(std::pair<std::string, float>(value + " (" + name + ")", 0.0f));
            frame.bind(value, &sliders[sliders.size() - 1].second);
        }

        std::getline(reader, line, '\n');
    }
    
    frame.init(this, name, path, weight);
    return std::pair<bool, Frame>(activated, frame);
}
