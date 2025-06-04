#ifndef BASICS_DELETIONQUEUE_HPP
#define BASICS_DELETIONQUEUE_HPP

#include <deque>
#include <functional>

namespace RtEngine {
	struct DeletionQueue {
		std::deque<std::function<void()>> deletors;

		void pushFunction(std::function<void()> &&function) { deletors.push_back(function); }

		void flush() {
			for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
				(*it)();
			}
			deletors.clear();
		}
	};

} // namespace RtEngine
#endif // BASICS_DELETIONQUEUE_HPP
