#pragma once

#include <vector>
#include <string>
#include <cstdbool>

#include "axjoint.hpp"
#include "../../axpi.hpp"

namespace axpi {
typedef struct {
    int            modelType;
    void           *jointHandle;
    axjoint_attr_t jointAttr;
} axjoint_model_base_t;

typedef struct {
    bool                 runJoint;
    int                  modelTypeMain;

    axjoint_model_base_t majorModel;
    axjoint_model_base_t minorModel;

    std::vector<int>     minorModelClassIds;

    int                  algoFmt;
    int                  algoWidth;
    int                  algoHeight;
    int                  restoreWidth;
    int                  restoreHeight;

    int                  ivpsAlgoWidth;
    int                  ivpsAlgoHeight;

    std::string          modelPath;
    std::string          modelPathL2;
} axjoint_models_t;

int axjoint_parse_param(std::string confiJsonFile, axjoint_models_t *handler);
int axjoint_inference_single_func(axjoint_models_t *handler, const void *pstFrame, axpi_results_t *results);
}
