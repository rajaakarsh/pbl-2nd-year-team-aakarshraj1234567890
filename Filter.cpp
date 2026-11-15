#include "Filter.hpp"

#include <algorithm>
#include <cmath>     
#include <cstdint>
#include <iostream>
#include <memory>     
#include <queue>      
#include <utility>   
#include <vector>


void GrayscaleFilter::apply(Image& image) {
    for (auto& row : image.pixels()) {
        for (auto& px : row) {
            const auto avg = static_cast<uint8_t>(
                std::round((px.rgbtRed + px.rgbtGreen + px.rgbtBlue) / 3.0));
            px.rgbtBlue  = avg;
            px.rgbtGreen = avg;
            px.rgbtRed   = avg;
        }
    }
}


void SepiaFilter::apply(Image& image) {
    for (auto& row : image.pixels()) {
        for (auto& px : row) {
            const int oR = px.rgbtRed;
            const int oG = px.rgbtGreen;
            const int oB = px.rgbtBlue;

            px.rgbtRed = static_cast<uint8_t>(
                std::min(255, static_cast<int>(
                    std::round(0.393 * oR + 0.769 * oG + 0.189 * oB))));
            px.rgbtGreen = static_cast<uint8_t>(
                std::min(255, static_cast<int>(
                    std::round(0.349 * oR + 0.686 * oG + 0.168 * oB))));
            px.rgbtBlue = static_cast<uint8_t>(
                std::min(255, static_cast<int>(
                    std::round(0.272 * oR + 0.534 * oG + 0.131 * oB))));
        }
    }
}


void ReflectFilter::apply(Image& image) {
    const int32_t w = image.width();
    for (auto& row : image.pixels()) {
        for (int32_t left = 0, right = w - 1; left < right; ++left, --right) {
            std::swap(row[static_cast<std::size_t>(left)],
                      row[static_cast<std::size_t>(right)]);
        }
    }
}


void BlurFilter::apply(Image& image) {
    
    const auto original = image.getPixelsCopy();

    const int32_t h = image.height();
    const int32_t w = image.width();

    for (int32_t r = 0; r < h; ++r) {
        for (int32_t c = 0; c < w; ++c) {
            int sumR = 0, sumG = 0, sumB = 0, count = 0;

           
            for (int32_t dr = -1; dr <= 1; ++dr) {
                for (int32_t dc = -1; dc <= 1; ++dc) {
                    const int32_t nr = r + dr;
                    const int32_t nc = c + dc;
                    if (nr < 0 || nr >= h || nc < 0 || nc >= w) {
                        continue;
                    }
                    const auto& src = original[static_cast<std::size_t>(nr)]
                                              [static_cast<std::size_t>(nc)];
                    sumR += src.rgbtRed;
                    sumG += src.rgbtGreen;
                    sumB += src.rgbtBlue;
                    ++count;
                }
            }

            auto& dst = image.pixels()[static_cast<std::size_t>(r)]
                                      [static_cast<std::size_t>(c)];
            dst.rgbtRed   = static_cast<uint8_t>(std::round(
                                static_cast<double>(sumR) / count));
            dst.rgbtGreen = static_cast<uint8_t>(std::round(
                                static_cast<double>(sumG) / count));
            dst.rgbtBlue  = static_cast<uint8_t>(std::round(
                                static_cast<double>(sumB) / count));
        }
    }
}


namespace {


struct QuadNode {
    int x, y, w, h;          
    bool isLeaf;             
    RGBTRIPLE avgColor;      
    std::unique_ptr<QuadNode> nw;   
    std::unique_ptr<QuadNode> ne;   
    std::unique_ptr<QuadNode> sw;   
    std::unique_ptr<QuadNode> se;  

    QuadNode() : x(0), y(0), w(0), h(0), isLeaf(false) {
        avgColor.rgbtBlue = avgColor.rgbtGreen = avgColor.rgbtRed = 0;
    }
};


std::unique_ptr<QuadNode> buildQuadtree(
    const std::vector<std::vector<RGBTRIPLE>>& pixels,
    int rx, int ry, int rw, int rh)
{
    auto node  = std::make_unique<QuadNode>();
    node->x    = rx;
    node->y    = ry;
    node->w    = rw;
    node->h    = rh;

    
    int64_t sumR = 0, sumG = 0, sumB = 0;
    int minLum = 255, maxLum = 0;
    const int count = rw * rh;

    for (int r = ry; r < ry + rh; ++r) {
        for (int c = rx; c < rx + rw; ++c) {
            const auto& p = pixels[static_cast<std::size_t>(r)]
                                  [static_cast<std::size_t>(c)];
            sumR += p.rgbtRed;
            sumG += p.rgbtGreen;
            sumB += p.rgbtBlue;
            int lum = (p.rgbtRed + p.rgbtGreen + p.rgbtBlue) / 3;
            if (lum < minLum) minLum = lum;
            if (lum > maxLum) maxLum = lum;
        }
    }

    node->avgColor.rgbtRed   = static_cast<uint8_t>(sumR / count);
    node->avgColor.rgbtGreen = static_cast<uint8_t>(sumG / count);
    node->avgColor.rgbtBlue  = static_cast<uint8_t>(sumB / count);

    const int contrast = maxLum - minLum;

 
    if (contrast <= 35 || rw <= 1 || rh <= 1) {
        node->isLeaf = true;
        return node;
    }

 
    const int midW   = rw / 2;
    const int midH   = rh / 2;
    const int rightW = rw - midW;
    const int botH   = rh - midH;

    node->nw = buildQuadtree(pixels, rx,        ry,        midW,   midH);
    node->ne = buildQuadtree(pixels, rx + midW, ry,        rightW, midH);
    node->sw = buildQuadtree(pixels, rx,        ry + midH, midW,   botH);
    node->se = buildQuadtree(pixels, rx + midW, ry + midH, rightW, botH);

    return node;
}


void applyQuadtree(std::vector<std::vector<RGBTRIPLE>>& pixels,
                   const QuadNode* node)
{
    if (!node) return;

    if (node->isLeaf) {
        for (int r = node->y; r < node->y + node->h; ++r) {
            for (int c = node->x; c < node->x + node->w; ++c) {
                pixels[static_cast<std::size_t>(r)]
                      [static_cast<std::size_t>(c)] = node->avgColor;
            }
        }
        return;
    }

    applyQuadtree(pixels, node->nw.get());
    applyQuadtree(pixels, node->ne.get());
    applyQuadtree(pixels, node->sw.get());
    applyQuadtree(pixels, node->se.get());
}


void collectStats(const QuadNode* node, int depth,
                  int& totalNodes, int& leafNodes, int& maxDepth)
{
    if (!node) return;
    ++totalNodes;
    if (depth > maxDepth) maxDepth = depth;

    if (node->isLeaf) {
        ++leafNodes;
        return;
    }
    collectStats(node->nw.get(), depth + 1, totalNodes, leafNodes, maxDepth);
    collectStats(node->ne.get(), depth + 1, totalNodes, leafNodes, maxDepth);
    collectStats(node->sw.get(), depth + 1, totalNodes, leafNodes, maxDepth);
    collectStats(node->se.get(), depth + 1, totalNodes, leafNodes, maxDepth);
}

} 

void QuadtreeFilter::apply(Image& image) {
    auto root = buildQuadtree(image.pixels(), 0, 0,
                              image.width(), image.height());

    int totalNodes = 0, leafNodes = 0, treeHeight = 0;
    collectStats(root.get(), 1, totalNodes, leafNodes, treeHeight);

    applyQuadtree(image.pixels(), root.get());

    std::cout << "[QuadtreeFilter] Total Nodes: " << totalNodes
              << "  |  Leaf Regions: " << leafNodes
              << "  |  Tree Height: "  << treeHeight << '\n';
}

void GraphBFSFilter::apply(Image& image) {
    const int32_t h = image.height();
    const int32_t w = image.width();
    auto& pixels    = image.pixels();

    std::vector<std::vector<bool>> visited(
        static_cast<std::size_t>(h),
        std::vector<bool>(static_cast<std::size_t>(w), false));

  
    const int32_t dr[] = {-1, 1,  0, 0};
    const int32_t dc[] = { 0, 0, -1, 1};


    const int kThresholdSq = 45 * 45;

    int componentCount   = 0;
    int largestComponent = 0;

    for (int32_t r = 0; r < h; ++r) {
        for (int32_t c = 0; c < w; ++c) {
            if (visited[static_cast<std::size_t>(r)]
                       [static_cast<std::size_t>(c)]) {
                continue;
            }

          
            std::queue<std::pair<int32_t, int32_t>> bfsQ;
            std::vector<std::pair<int32_t, int32_t>> component;

            bfsQ.push(std::make_pair(r, c));
            visited[static_cast<std::size_t>(r)]
                   [static_cast<std::size_t>(c)] = true;

            int64_t sumR = 0, sumG = 0, sumB = 0;

            while (!bfsQ.empty()) {
                std::pair<int32_t, int32_t> cur = bfsQ.front();
                bfsQ.pop();

                const int32_t cr = cur.first;
                const int32_t cc = cur.second;

                component.push_back(cur);
                const auto& cpx = pixels[static_cast<std::size_t>(cr)]
                                        [static_cast<std::size_t>(cc)];
                sumR += cpx.rgbtRed;
                sumG += cpx.rgbtGreen;
                sumB += cpx.rgbtBlue;

               
                for (int d = 0; d < 4; ++d) {
                    const int32_t nr = cr + dr[d];
                    const int32_t nc = cc + dc[d];

                    if (nr < 0 || nr >= h || nc < 0 || nc >= w) continue;
                    if (visited[static_cast<std::size_t>(nr)]
                               [static_cast<std::size_t>(nc)]) continue;

                
                    const auto& npx = pixels[static_cast<std::size_t>(nr)]
                                            [static_cast<std::size_t>(nc)];
                    int dR = cpx.rgbtRed   - npx.rgbtRed;
                    int dG = cpx.rgbtGreen - npx.rgbtGreen;
                    int dB = cpx.rgbtBlue  - npx.rgbtBlue;
                    int distSq = dR * dR + dG * dG + dB * dB;

                    if (distSq <= kThresholdSq) {
                        visited[static_cast<std::size_t>(nr)]
                               [static_cast<std::size_t>(nc)] = true;
                        bfsQ.push(std::make_pair(nr, nc));
                    }
                }
            }

           
            const int compSize = static_cast<int>(component.size());
            RGBTRIPLE meanColor;
            meanColor.rgbtRed   = static_cast<uint8_t>(sumR / compSize);
            meanColor.rgbtGreen = static_cast<uint8_t>(sumG / compSize);
            meanColor.rgbtBlue  = static_cast<uint8_t>(sumB / compSize);

            for (std::size_t i = 0; i < component.size(); ++i) {
                pixels[static_cast<std::size_t>(component[i].first)]
                      [static_cast<std::size_t>(component[i].second)] = meanColor;
            }

            ++componentCount;
            if (compSize > largestComponent) {
                largestComponent = compSize;
            }
        }
    }

    std::cout << "[GraphBFSFilter] Vertices |V|: "
              << (static_cast<int64_t>(h) * w)
              << "  |  Connected Components: " << componentCount
              << "  |  Largest Component: "    << largestComponent << '\n';
}
