#pragma once

#define SINGLETON(Class) \
public: \
	static Class& the() { \
		static Class s_class; \
		return s_class; \
	}
