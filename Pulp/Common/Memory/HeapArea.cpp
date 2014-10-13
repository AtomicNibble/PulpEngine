#include "EngineCommon.h"

#include "HeapArea.h"
#include "VirtualMem.h"

X_NAMESPACE_BEGIN(core)


HeapArea::HeapArea(size_t size)
{
	char* pMem = static_cast<char*>( VirtualMem::ReserveAddressSpace( size ) );

	this->m_start = pMem;
	this->m_end = pMem + size;

	X_ASSERT( (size % VirtualMem::GetPageSize() ) == 0 , "Size must be multiple of the virtual page" )( size, VirtualMem::GetPageSize() );
	
	VirtualMem::AllocatePhysicalMemory( pMem, size );
}

HeapArea::~HeapArea(void)
{
	VirtualMem::ReleaseAddressSpace( m_start );
}

X_NAMESPACE_END
