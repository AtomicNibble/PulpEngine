

X_NAMESPACE_BEGIN(render)

X_INLINE D3D12_PRIMITIVE_TOPOLOGY_TYPE topoTypeFromDesc(const StateDesc& desc)
{
    switch (desc.topo) {
        case TopoType::TRIANGLELIST:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        case TopoType::TRIANGLESTRIP:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        case TopoType::LINELIST:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        case TopoType::LINESTRIP:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        case TopoType::POINTLIST:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;

        default:
#if X_DEBUG
            X_ASSERT_UNREACHABLE();
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
#else
            X_NO_SWITCH_DEFAULT;
#endif // X_DEBUG
    }
}

X_INLINE D3D12_PRIMITIVE_TOPOLOGY topoFromDesc(const StateDesc& desc)
{
    switch (desc.topo) {
        case TopoType::TRIANGLELIST:
            return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        case TopoType::TRIANGLESTRIP:
            return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
        case TopoType::LINELIST:
            return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
        case TopoType::LINESTRIP:
            return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
        case TopoType::POINTLIST:
            return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;

        default:
#if X_DEBUG
            X_ASSERT_UNREACHABLE();
            return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
#else
            X_NO_SWITCH_DEFAULT;
#endif // X_DEBUG
    }
}

X_NAMESPACE_END
