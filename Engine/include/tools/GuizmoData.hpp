#pragma once
#include <imgui/imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "imgui/ImGuizmo.h"
namespace bee
{
struct GuizmoData
{
public:
    bool enabledGuizmo = false;
    bool freeMovement = false;
    float camDistance = 8.f;
    ImGuizmo::OPERATION mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
    ImGuizmo::MODE mCurrentGizmoMode = ImGuizmo::LOCAL;

    bool useSnap = false;
    float snap[3] = {1.f, 1.f, 1.f};
    float bounds[6] = {-0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f};
    float boundsSnap[3] = {0.1f, 0.1f, 0.1f};
    bool boundSizing = false;
    bool boundSizingSnap = false;

    glm::vec3 transformTranslation;
    glm::vec3 transformScale;
    glm::quat transformRotation;

    float objectMatrix[4][16] = {{1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f},

                                 {1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 2.f, 0.f, 0.f, 1.f},

                                 {1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 2.f, 0.f, 2.f, 1.f},

                                 {1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 2.f, 1.f}};

    const float identityMatrix[16] = {1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f};

    float cameraView[16] = {1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f};

    float cameraProjection[16]; 
    float* floatProjection;
    float* floatViewM;
    float* floatTransformM;


    void EditTransform(float* cameraView, float* cameraProjection, float* matrix);
};
}  // namespace bee