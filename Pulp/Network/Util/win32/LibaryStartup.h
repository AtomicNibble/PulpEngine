#pragma once


X_NAMESPACE_BEGIN(net)

namespace PlatLib
{

	bool addRef(void);
	void deRef(void);

	class ScopedRef
	{
	public:
		ScopedRef();
		~ScopedRef();
	};

} // namespace PlatLib

X_NAMESPACE_END
