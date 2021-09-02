#pragma once

#include "FileExplorer.hpp"
#include "HeadBar.hpp"
#include <Renderer.hpp>
#include <string>
#include <vector>

template<Gapi G>
class BuildRenderer : public Renderer<G> {
public:
    GAllocator allocator;
    FileExplorer explorer;
    HeadBar head;
    std::vector<std::string> files;
    bool isProject = false;
    UiScene scene;

    BuildRenderer();
    void pointer(PointerEvent pointer) override;
    void change(int_t newWidth, int_t newHeight) override;
    void setPath(std::string path);
    void drawFrame() override;
};
