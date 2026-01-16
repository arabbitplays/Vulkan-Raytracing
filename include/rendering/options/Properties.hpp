#ifndef PROPERTIES_HPP
#define PROPERTIES_HPP

#include <glm/vec3.hpp>
#include <string>
#include <utility>
#include <vector>

namespace RtEngine {
#define NONE_PROPERTY_FLAG 0u
#define SERIALIZABLE_PROPERTY_FLAG 1u
#define PERSISTENT_PROPERTY_FLAG 2u
#define ALL_PROPERTY_FLAGS (SERIALIZABLE_PROPERTY_FLAG | PERSISTENT_PROPERTY_FLAG)

	template<typename T>
	struct Property {
		Property() = default;
		Property(std::string  name, T *var, const uint32_t flags = ALL_PROPERTY_FLAGS) : name(std::move(name)), flags(flags), var(var) {}

		std::string name;
		uint32_t flags = ALL_PROPERTY_FLAGS;
		T *var;
	};

	struct BoolProperty : Property<int32_t> {
		BoolProperty(const std::string& name, int32_t *var, const uint32_t flags = ALL_PROPERTY_FLAGS) :
			Property<int32_t>(name, var, flags) {
			imgui_option = *var > 0;
		}
		bool imgui_option;
	};

	struct IntProperty : Property<int32_t> {
		IntProperty(const std::string& name, int32_t *var, const uint32_t flags = ALL_PROPERTY_FLAGS,
					const int32_t min = 0, const int32_t max = 0)
					: Property<int32_t>(name, var, flags), min(min), max(max) {}
		int32_t min, max;
	};

	struct FloatProperty : Property<float> {
		FloatProperty(const std::string& name, float *var, const uint32_t flags = ALL_PROPERTY_FLAGS,
					  const float min = 0, const float max = 0)
					  : Property<float>(name, var, flags), min(min), max(max) {}
		float min, max;
	};

	struct StringProperty : Property<std::string> {
		StringProperty(const std::string& name, std::string *var, const uint32_t flags = ALL_PROPERTY_FLAGS) :
			Property<std::string>(name, var, flags) {}
	};

	struct VectorProperty : Property<glm::vec3> {
		VectorProperty(const std::string& name, glm::vec3 *var, const uint32_t flags = ALL_PROPERTY_FLAGS) :
			Property<glm::vec3>(name, var, flags) {}
	};

	struct SelectionProperty : Property<std::string> {
		SelectionProperty(const std::string& name, std::string *var, const std::vector<std::string>& selection_options,
						  const uint32_t flags = ALL_PROPERTY_FLAGS) :
			Property<std::string>(name, var, flags), selection_options(selection_options) {}
		int option_idx = 0;
		std::vector<std::string> selection_options{};
	};
} // namespace RtEngine

#endif // PROPERTIES_HPP
