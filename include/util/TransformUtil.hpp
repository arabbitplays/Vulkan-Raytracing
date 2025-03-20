//
// Created by oschdi on 3/20/25.
//

#ifndef TRANSFORMUTIL_HPP
#define TRANSFORMUTIL_HPP

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

class TransformUtil {
   TransformUtil() = delete;

public:
    struct DecomposedTransform
    {
        glm::vec3 translation, rotation, scale;
    };

    static glm::mat4 recomposeMatrix(DecomposedTransform decomposed_transform) {
        glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), decomposed_transform.translation);

        glm::quat quaternion = glm::quat(glm::radians(decomposed_transform.rotation)); // Convert degrees to radians
        glm::mat4 rotationMatrix = glm::toMat4(quaternion);

        glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), decomposed_transform.scale);

        return translationMatrix * rotationMatrix * scaleMatrix;
    }

    static DecomposedTransform decomposeMatrix(const glm::mat4& transform) {
        // Extract translation (last column of the matrix)
        glm::vec3 translation = glm::vec3(transform[3]);

        // Extract scale factors (length of the columns, excluding the translation part)
        glm::vec3 scale(
            glm::length(glm::vec3(transform[0])),
            glm::length(glm::vec3(transform[1])),
            glm::length(glm::vec3(transform[2]))
        );

        // Extract rotation by normalizing the 3x3 portion of the matrix
        glm::mat3 rotationMatrix = glm::mat3(transform);
        rotationMatrix[0] /= scale.x;
        rotationMatrix[1] /= scale.y;
        rotationMatrix[2] /= scale.z;

        // Convert rotation matrix to quaternion
        glm::quat rotation = glm::quat_cast(rotationMatrix);
        glm::vec3 eulerAngles = glm::eulerAngles(rotation);
        eulerAngles = glm::degrees(eulerAngles);

        DecomposedTransform result(translation, eulerAngles, scale);
        return result;
    }
};

#endif //TRANSFORMUTIL_HPP
