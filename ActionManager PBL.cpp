#include "ActionManager.hpp"

#include <iostream>
#include <stdexcept>
#include <string>



ActionManager::ActionManager() {
    filterMap_['g'] = std::make_unique<GrayscaleFilter>();
    filterMap_['s'] = std::make_unique<SepiaFilter>();
    filterMap_['r'] = std::make_unique<ReflectFilter>();
    filterMap_['b'] = std::make_unique<BlurFilter>();
    filterMap_['t'] = std::make_unique<QuadtreeFilter>();
    filterMap_['f'] = std::make_unique<GraphBFSFilter>();
}


void ActionManager::enqueueFilters(const std::string& flags) {
    for (char c : flags) {
        filterQueue_.push(c);
    }
}

void ActionManager::executePipeline(Image& image) {
    while (!filterQueue_.empty()) {
        const char flag = filterQueue_.front();
        filterQueue_.pop();

        if (flag == 'u') {
            undo(image);
            continue;
        }

        if (flag == 'h') {
            image.printHistogram();
            continue;
        }

        auto it = filterMap_.find(flag);
        if (it == filterMap_.end()) {
            throw std::invalid_argument(
                std::string("Unknown filter flag: '") + flag + "'");
        }

      
        saveState(image);

        
        it->second->apply(image);

      
        std::cout << "[ActionManager] Applied: '" << flag
                  << "'  |  Undo history depth: " << undoStack_.size()
                  << '\n';
    }
}

void ActionManager::undo(Image& image, std::size_t steps) {
    const std::size_t actual = std::min(steps, undoStack_.size());

    if (actual == 0) {
        std::cout << "[ActionManager] Undo: no history available\n";
        return;
    }

   
    for (std::size_t i = 1; i < actual; ++i) {
        undoStack_.pop();
    }

    
    image.setPixels(undoStack_.top());
    undoStack_.pop();

    std::cout << "[ActionManager] Undo: rolled back " << actual
              << " step(s)  |  Undo history depth: " << undoStack_.size()
              << '\n';
}



void ActionManager::saveState(const Image& image) {
    undoStack_.push(image.getPixelsCopy());
}
