#ifndef FILTER_HPP
#define FILTER_HPP



#include "Image.hpp"


class BaseFilter {
public:
    virtual ~BaseFilter() = default;

    virtual void apply(Image& image) = 0;
};


class GrayscaleFilter : public BaseFilter {
public:
    void apply(Image& image) override;
};


class SepiaFilter : public BaseFilter {
public:
    void apply(Image& image) override;
};


class ReflectFilter : public BaseFilter {
public:
    void apply(Image& image) override;
};

class BlurFilter : public BaseFilter {
public:
    void apply(Image& image) override;
};


class QuadtreeFilter : public BaseFilter {
public:
    void apply(Image& image) override;
};


class GraphBFSFilter : public BaseFilter {
public:
    void apply(Image& image) override;
};

#endif 