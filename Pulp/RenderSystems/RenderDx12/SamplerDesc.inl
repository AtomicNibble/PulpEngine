
X_NAMESPACE_BEGIN(render)



SamplerDescriptor::SamplerDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE hCpuDescriptor)
	: hCpuDescriptorHandle_(hCpuDescriptor)
{

}

D3D12_CPU_DESCRIPTOR_HANDLE SamplerDescriptor::getCpuDescriptorHandle(void) const
{
	return hCpuDescriptorHandle_;
}


X_NAMESPACE_END