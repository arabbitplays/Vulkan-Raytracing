//
// Created by oschdi on 5/6/25.
//

#ifndef PROPERTIES_HPP
#define PROPERTIES_HPP

#include <glm/vec3.hpp>
#include <string>
#include <vector>

namespace RtEngine {
#define NONE_PROPERTY_FLAG 0u
#define SERIALIZABLE_PROPERTY_FLAG 1u
#define PERSISTENT_PROPERTY_FLAG 2u
#define ALL_PROPERTY_FLAGS SERIALIZABLE_PROPERTY_FLAG | PERSISTENT_PROPERTY_FLAG

	template<typename T>
	struct Property {
		Property() = default;
		Property(std::string name, T *var, uint32_t flags = ALL_PROPERTY_FLAGS) : name(name), var(var), flags(flags) {}

		std::string name;
		uint32_t flags;
		T *var;
	};

	struct BoolProperty : Property<int32_t> {
		BoolProperty(std::string name, int32_t *var, uint32_t flags = ALL_PROPERTY_FLAGS) :
			Property<int32_t>(name, var, flags) {
			imgui_option = *var > 0;
		}
		bool imgui_option;
	};

	struct IntProperty : Property<int32_t> {
		IntProperty(std::string name, int32_t *var, uint32_t flags = ALL_PROPERTY_FLAGS, int32_t min = 0,
					int32_t max = 0) : Property<int32_t>(name, var, flags), min(min), max(max) {}
		int32_t min, max;
	};

	struct FloatProperty : Property<float> {
		FloatProperty(std::string name, float *var, uint32_t flags = ALL_PROPERTY_FLAGS, uint32_t min = 0,
					  uint32_t max = 0) : Property<float>(name, var, flags), min(min), max(max) {}
		float min, max;
	};

	struct StringProperty : Property<std::string> {
		StringProperty(std::string name, std::string *var, uint32_t flags = ALL_PROPERTY_FLAGS) :
			Property<std::string>(name, var, flags) {}
	};

	struct VectorProperty : Property<glm::vec3> {
		VectorProperty(std::string name, glm::vec3 *var, uint32_t flags = ALL_PROPERTY_FLAGS) :
			Property<glm::vec3>(name, var, flags) {}
	};

	struct SelectionProperty : Property<std::string> {
		SelectionProperty(std::string name, std::string *var, std::vector<std::string> selection_options,
						  uint32_t flags = ALL_PROPERTY_FLAGS) :
			Property<std::string>(name, var, flags), selection_options(selection_options) {}
		int option_idx = 0;
		std::vector<std::string> selection_options{};
	};
} // namespace RtEngine

#endif // PROPERTIES_HPP
