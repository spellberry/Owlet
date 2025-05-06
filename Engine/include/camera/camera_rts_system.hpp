#pragma once
#include "core/ecs.hpp"
#include "glm/glm.hpp"

namespace bee
{
    class CameraSystemRTS : public bee::System
    {
        public:
            CameraSystemRTS();
            void ResetCamera();
            void Update(float dt) override;

            float GetRTSFOV() { return m_fov; }

        private:

            /// <summary>
            /// Moves the camera on a specified axis.
            /// </summary>
            /// <param name="axis - ">Axis of translation.</param>
            void Translate(const glm::vec3& axis, float dt);

            /// <summary>
            /// Move around the anchor point.
            /// </summary>
            void Orbit(const glm::vec2& mouseCurrentPosition, const float dt);

            /// <summary>
            /// More around the anchor point by the the given angle.
            /// </summary>
            void Orbit(const float angle, const float dt);

            /// <summary>
            /// Uses scroll wheel to zoom the camera.
            /// </summary>
            void ChangeFOV(const float offset);

            /// <summary>
            /// Sets the Lerping values.
            /// </summary>
            /// <param name="returnTo - ">The value to LERP towards.</param>
            void Return(const glm::vec3& returnTo);

            /// <summary>
            /// Lerp back to the center of the map.
            /// </summary>
            void LERP(const float dt);
            
            /// <summary>
            /// Makes the camera slide based on its current velocity.
            /// </summary>
            void ApplyMomentum(const float dt);

            void AnchorDraw();

            // LERPING Data
            glm::vec3 m_start = glm::vec3(0.0f);
            glm::vec3 m_end = glm::vec3(0.0f);
            bool m_LERP = false;
            float m_lerpAmount = 0.0f;

            // Velocities
            glm::vec3 m_velocity = glm::vec3(0.0f);
            glm::vec2 m_spin = glm::vec2(0.0f);
            const float m_maxSpeed = 24.0f;  // 16.0f;
            const float m_drag = 4.0f; // 4.0f;
            float m_slide = 0.0f;
            bool m_slideLast = false;

            // Important Vectors
            const glm::vec3 m_up = glm::vec3(0.0f, 0.0f, 1.0f);
            glm::vec3 m_anchor = glm::vec3(0.0f);
            glm::vec3 m_Yaxis = glm::vec3(0.0f, 1.0f, 0.0f);
            glm::vec3 m_Xaxis = glm::vec3(1.0f, 0.0f, 0.0f);

            // Mouse Data
            glm::vec2 m_mousePositionCurrent = glm::vec2(0.0f);  // Where the mouse is currently
            glm::vec2 m_mousePositionLast = glm::vec2(0.0f);     // Position of mouse last tick
            glm::vec2 m_mouseOffset = glm::vec2(0.0f);           //
            float m_mouseWheelLast = 0.0f;                       // Mouse wheel value from previous tick

            // Camera Data
            bee::Entity m_camera;
            const float m_minFov = 1.0f;
            const float m_maxFov = 25.0f; // 120.0
            const float m_fovIncrement = 0.5f;
            float m_radius = 0.0f;

            // Step Data
            float m_stepOrbit = 364.0f;
            float m_stepTranslate = 400.0f;  // 300.0f;
            float m_stepMouseWheel = 500.0f;
            float m_fov = 15.0f;
    };
}  // namespace bee