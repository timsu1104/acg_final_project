struct Onb {
    vec3 _u, _v, _w;
};

struct CosineHemispherePdf {
    Onb uvw;
};

struct LightPdf {
    vec3 normal_;
};

struct MixturePdf {
    LightPdf light_;
    CosineHemispherePdf cosine_;
    float prob;
};