#pragma once
#include "core/ecs.hpp"
#include "glm/glm.hpp"


namespace bee
{

    struct Transform;
    struct Camera;


    class CameraSystemEditor : public bee::System
    {
        public:
            CameraSystemEditor();

            void Update(float dt) override;

            float& GetOrbit() { return m_stepOrbit; }
            float& GetTranslate() { return m_stepTranslate; }
            float& GetRotate() { return m_stepRotate; }
            float& GetMouseWheel() { return m_stepMouseWheel; }
            float& GetFOV() { return m_fov; }
            float& GetFocusDistance() { return m_anchorDistance; }
            bool& GetMode() { return m_mode2; }
            float& GetFarPlane() { return m_farPlane; }

        private:

            /// <summary>
            /// VA prefered camera controls.
            /// </summary>
            void UpdateMode1(const glm::vec2& mouseCurrentPosition, const float dt);

            /// <summary>
            /// PR prefered camera controls.
            /// </summary>
            void UpdateMode2(const glm::vec2& mouseCurrentPosition, const float dt);

            /// <summary>
            /// Called when switching between the two modes.
            /// </summary>
            void Reset();

            /// <summary>
            /// Local camera rotation.
            /// </summary>
            void Rotate(const glm::vec2& mousePositionCurrent, const float dt);

            /// <summary>
            /// Translates camera on its local right and up vectors.
            /// </summary>
            void Pan(const glm::vec2& mousePositionCurrent, const float dt);

            /// <summary>
            /// Translates camera on the world z-axis.
            /// </summary>
            void Raise(const glm::vec2& mouseCurrentPosition, const float dt);

            /// <summary>
            /// Translates camera on its local front vector.
            /// </summary>
            void Forward(const glm::vec2& mouseCurrentPosition, const float dt, const float offset);

            /// <summary>
            /// Makes the the camera rotate around the anchor.
            /// </summary>
            void Orbit(const glm::vec2& mouseCurrentPosition, const float dt);

            /// <summary>
            /// Updates the anchor to the center of all selected entities. If there 
            /// is only one entity the anchor will copy the position of the entity. 
            /// </summary>
            void AnchorSet();

            /// <summary>
            /// Moves the anchor 
            /// </summary>
            void AnchorFocus();

            /// <summary>
            /// Rotates camera to look at the anchor.
            /// </summary>
            void AnchorLookAt(const glm::vec3& currentPosition);

            /// <summary>
            /// Visualizing anchor position.
            /// </summary>
            void AnchorDraw();

            /// <summary>
            /// Changes the camera FOV based on the mouse wheel delta.
            /// </summary>
            void ChangeFOV(const float offset);

            glm::vec3 m_anchor = glm::vec3(0.0f);
            const glm::vec3 m_up = glm::vec3(0.0f, 0.0f, 1.0f);

            // Mouse Data
            glm::vec2 m_mousePositionCurrent = glm::vec2(0.0f);  // Where the mouse is currently
            glm::vec2 m_mousePositionLast = glm::vec2(0.0f);     // Position of mouse last tick
            glm::vec2 m_mouseOffset = glm::vec2(0.0f);           //
            float m_mouseWheelLast = 0.0f;                       // Mouse wheel value from previous tick

            // Camera Data
            bee::Entity m_camera;
            const float m_minFov = 15.0f;
            const float m_maxFov = 70.0f;
            const float m_fovIncrement = 0.5f;
            float m_radius = 0.0f; 
            float m_farPlane = 200.0f;

            // Step Data
            float m_stepOrbit = 180.0f;
            float m_stepTranslate = 150.0f;
            float m_stepRotate = 135.0f;
            float m_stepMouseWheel = 100.0f;
            float m_fov = 15.0f;
            float m_anchorDistance = 5.0f;

            // --------------------------------------
            // Camera Mode 2 Methods and variables
            // --------------------------------------

            bool m_mode2 = true;

            /// <summary>
            /// Moves the camera on a specified axis.
            /// </summary>
            /// <param name="axis - ">axis of translation.</param>
            void M2Translate(const glm::vec3& axis, float dt);

            /// <summary>
            /// Move around the anchor point.
            /// </summary>
            void M2Orbit(const glm::vec2& mouseCurrentPosition, const float dt);

            /// <summary>
            /// Uses scroll wheel to zoom the camera.
            /// </summary>
            void M2ChangeFOV(const float offset);

            // Important Vectors
            glm::vec3 m_Yaxis = glm::vec3(0.0f, 1.0f, 0.0f);
            glm::vec3 m_Xaxis = glm::vec3(1.0f, 0.0f, 0.0f);
    };
}