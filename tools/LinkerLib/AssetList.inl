
X_NAMESPACE_BEGIN(linker)


void AssetList::add(assetDb::AssetType::Enum type, core::string& name)
{
    assets_[type].emplace_back(name);
}

const AssetList::AssetNameLists& AssetList::getAssetList(void) const
{
    return assets_;
}


X_NAMESPACE_END
