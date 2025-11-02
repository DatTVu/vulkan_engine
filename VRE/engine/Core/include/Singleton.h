#pragma once
#include <cassert>

namespace VRE 
{
	template<typename T> class Singleton {
		
		public:
			//Don't allow copy constructor/assignment
			Singleton(const Singleton<T>&) = delete;
			Singleton& operator=(const Singleton<T>&) = delete;
			Singleton(const Singleton<T>&&) = delete;
			Singleton&& operator=(const Singleton<T>&&) = delete;

		protected:
			static T* s_Instance;

		public:
			Singleton() 
			{
				assert(!s_Instance);
				s_Instance = static_cast<T*>(this);
			}

			~Singleton()
			{
				assert(s_Instance);
				s_Instance = nullptr;
			}

			static T& Get()
			{
				assert(s_Instance);
				return *s_Instance;
			}

			static T* GetPtr()
			{
				return s_Instance;
			}
	};
}
