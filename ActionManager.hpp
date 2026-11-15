#ifndef ACTIONMANAGER_HPP
#define ACTIONMANAGER_HPP


#include "Filter.hpp"
#include "Image.hpp"

#include <cstddef>      
#include <memory>       
#include <queue>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

class ActionManager {
public:
 
    ActionManager();

  
    void enqueueFilters(const std::string& flags);


    void executePipeline(Image& image);


    void undo(Image& image, std::size_t steps = 1);

   
    std::size_t historyCount() const { return undoStack_.size(); }

private:
   
    std::queue<char> filterQueue_;

    
    std::stack<std::vector<std::vector<RGBTRIPLE>>> undoStack_;

  
    std::unordered_map<char, std::unique_ptr<BaseFilter>> filterMap_;

   
    void saveState(const Image& image);
};

#endif