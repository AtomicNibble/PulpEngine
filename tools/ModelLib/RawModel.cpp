#include "stdafx.h"
#include "RawModelTypes.h"
#include "RawModel.h"

#include <IModel.h>
#include <IFileSys.h>

#include <String\Lexer.h>

#include <Time\StopWatch.h>

#include <Threading\JobSystem2.h>
#include <Threading\Spinlock.h>
#include <Memory\ThreadPolicies\MultiThreadPolicy.h>

X_NAMESPACE_BEGIN(model)

using namespace core::Hash::Fnv1Literals;

namespace RawModel
{
	const int32_t Model::VERSION = MODEL_RAW_VERSION;


	Model::Model(core::MemoryArenaBase* arena, core::V2::JobSystem* pJobSys) :
		pJobSys_(pJobSys),
		arena_(arena),
		bones_(arena),
		unitOfMeasure_(core::UnitOfMeasure::Centimeters),
		hasColisionMeshes_(false)
	{

	}

	void Model::Clear(void)
	{
		bones_.clear();
		lods_.clear();
		hasColisionMeshes_ = false;
	}

	bool Model::LoadRawModel(core::Path<wchar_t>& path)
	{
		core::Path<char> narrowPath(path);
		return LoadRawModel(narrowPath);
	}

	bool Model::SaveRawModel(core::Path<wchar_t>& path)
	{
		core::Path<char> narrowPath(path);
		return SaveRawModel(narrowPath);
	}

	size_t Model::totalMeshes(void) const
	{
		size_t num = 0;
		for (auto& lod : lods_) {
			num += lod.meshes_.size();
		}

		return num;
	}

	size_t Model::numLods(void) const
	{
		return lods_.size();
	}

	bool Model::hasColMeshes(void) const
	{
		return hasColisionMeshes_;
	}

	size_t Model::getNumBones(void) const
	{
		return bones_.size();
	}

	const char* Model::getBoneName(size_t idx) const
	{
		return bones_[idx].name_;
	}

	const Quatf Model::getBoneAngle(size_t idx) const
	{
		return Quatf(bones_[idx].rotation_);
	}

	const Vec3f Model::getBonePos(size_t idx) const
	{
		return bones_[idx].worldPos_;
	}

	const Lod& Model::getLod(size_t idx) const
	{
		X_ASSERT(idx < numLods(), "Invalid lod idx")(idx, numLods());
		return lods_[idx];
	}
	
	core::UnitOfMeasure::Enum Model::getUnits(void) const
	{
		return unitOfMeasure_;
	}

	bool Model::LoadRawModel(core::Path<char>& path)
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pFileSys);

		Clear();

		path.setExtension(model::MODEL_RAW_FILE_EXTENSION);

		core::fileModeFlags mode;
		mode.Set(core::fileMode::SHARE);
		mode.Set(core::fileMode::READ);

		core::XFileScoped file;

		if (!file.openFile(path.c_str(), mode)) {
			X_ERROR("RawModel", "Failed to open raw model file: \"%s\"",
				path.c_str());
			return false;
		}

		core::Array<char> fileData(arena_);
		{
			size_t fileSize = safe_static_cast<size_t, uint64_t>(file.remainingBytes());

			fileData.resize(fileSize);

			size_t bytesRead = file.read(fileData.ptr(), fileSize);
			if (bytesRead != fileSize) {
				X_ERROR("RawModel", "failed to read file data. got: %" PRIuS " requested: %" PRIuS, bytesRead, fileSize);
				return false;
			}
		}

		core::XLexer lex(fileData.begin(), fileData.end());

		return ParseRawModel(lex);
	}

	bool Model::LoadRawModel(const core::Array<uint8_t>& data)
	{
		core::XLexer lex(reinterpret_cast<const char*>(data.begin()), reinterpret_cast<const char*>(data.end()));

		return ParseRawModel(lex);
	}

	bool Model::ParseRawModel(core::XLexer& lex)
	{
		int32_t version, numLods, numBones;

		if (!ReadheaderToken(lex, "VERSION", version)) {
			return false;
		}
		if (!ReadheaderToken(lex, "UNITS", unitOfMeasure_, true)) {
			return false;
		}
		if (!ReadheaderToken(lex, "LODS", numLods)) {
			return false;
		}
		if (!ReadheaderToken(lex, "BONES", numBones)) {
			return false;
		}

		if (version < model::MODEL_RAW_VERSION) {
			X_ERROR("RawModel", "RawModel file version is too old: %" PRIi32 " required: %" PRIu32,
				version, model::MODEL_RAW_VERSION);
			return false;
		}

		if (numLods < 1) {
			X_WARNING("RawModel", "RawModel has no lods");
			return true;
		}
		if (numLods > model::MODEL_MAX_LODS) {
			X_WARNING("RawModel", "RawModel has no many lods: %" PRIu32 " max",
				model::MODEL_MAX_LODS);
			return true;
		}
		if (numBones > model::MODEL_MAX_BONES) {
			X_ERROR("RaWModel", "model has too many bones: %" PRIi32 " max: %" PRIu32,
				numBones, model::MODEL_MAX_BONES);
			return false;
		}

		if (!ReadBones(lex, numBones)) {
			X_ERROR("RawModel", "failed to parse bone data");
			return false;
		}
		if (!ReadLods(lex, numLods)) {
			X_ERROR("RawModel", "failed to parse lod data");
			return false;
		}

		for (auto lod : lods_) {
			for (auto mesh : lod.meshes_) {
				if (isColisionMesh(mesh.displayName_)) {
					hasColisionMeshes_ = true;
					break;
				}
			}
		}

		return true;
	}

	bool Model::ReadBones(core::XLexer& lex, int32_t numBones)
	{
		bones_.resize(numBones);

		core::XLexToken token(nullptr, nullptr);

		int32_t lastParentIdx = -1;

		for (auto& bone : bones_)
		{
			if (!lex.ReadToken(token)) {
				X_ERROR("RawModel", "Failed to read 'BONE' token");
				return false;
			}

			if (!token.isEqual("BONE")) {
				X_ERROR("RawModel", "Failed to read 'BONE' token");
				return false;
			}

			// read the parent idx.
			if (!lex.ReadTokenOnLine(token)) {
				X_ERROR("RawModel", "Failed to read parent index token");
				return false;
			}

			if (token.GetType() != core::TokenType::NUMBER) {
				X_ERROR("RawModel", "Parent index token is invalid");
				return false;
			}

			bone.parIndx_ = token.GetIntValue();

			if (bone.parIndx_ < lastParentIdx) {
				X_ERROR("RawModel", "Parent index must ascend (aka sorted)");
				return false;
			}

			lastParentIdx = bone.parIndx_;

			// read the string
			if (!lex.ReadTokenOnLine(token)) {
				X_ERROR("RawModel", "Failed to read bone name token");
				return false;
			}

			if (token.GetType() != core::TokenType::STRING) {
				X_ERROR("RawModel", "Bone name token is invalid");
				return false;
			}

			bone.name_ = core::string(token.begin(), token.end());

			// pos
			if (!lex.ReadToken(token)) {
				X_ERROR("RawModel", "Failed to read 'POS' token");
				return false;
			}

			if (!token.isEqual("POS")) {
				X_ERROR("RawModel", "Failed to read 'POS' token");
				return false;
			}

			if (!lex.Parse1DMatrix(3, &bone.worldPos_[0])) {
				X_ERROR("RawModel", "Failed to read 'POS' token data");
				return false;
			}

			// scale
			if (!lex.ReadToken(token)) {
				X_ERROR("RawModel", "Failed to read 'SCALE' token");
				return false;
			}

			if (!token.isEqual("SCALE")) {
				X_ERROR("RawModel", "Failed to read 'SCALE' token");
				return false;
			}

			if (!lex.Parse1DMatrix(3, &bone.scale_[0])) {
				X_ERROR("RawModel", "Failed to read 'SCALE' token data");
				return false;
			}

			// ang
			if (!lex.ReadToken(token)) {
				X_ERROR("RawModel", "Failed to read 'ANG' token");
				return false;
			}

			if (!token.isEqual("ANG")) {
				X_ERROR("RawModel", "Failed to read 'ANG' token");
				return false;
			}

			if(!lex.Parse2DMatrix(3, 3, &bone.rotation_[0])) {
				X_ERROR("RawModel", "Failed to read 'ANG' token data");
				return false;
			}
		}

		return true;
	}

	bool Model::ReadLods(core::XLexer& lex, int32_t numLods)
	{
		/*
		LOD
		DISTANCE
		NUMMESH
		*/

		lods_.resize(numLods, Lod(arena_));

		core::XLexToken token(nullptr, nullptr);

		for (auto& lod : lods_)
		{
			if (!lex.ExpectTokenString("LOD")) {
				X_ERROR("RawModel", "Failed to read 'LOD' token");
				return false;
			}

			// read the mesh count.
			int32_t numMesh;

			if (!ReadheaderToken(lex, "NUMMESH", numMesh)) {
				return false;
			}

			if (numMesh > model::MODEL_MAX_MESH) {
				X_ERROR("RaWModel", "lod has too many mesh: %" PRIi32 " max: %" PRIu32,
					numMesh, model::MODEL_MAX_MESH);
				return false;
			}

			lod.meshes_.resize(numMesh, Mesh(arena_));

			for (auto& mesh : lod.meshes_)
			{
				if (!ReadMesh(lex, mesh)) {
					return false;
				}
			}

			// now have all this lods materials

			for (auto& mesh : lod.meshes_)
			{
				if (!ReadMaterial(lex, mesh.material_)) {
					return false;
				}
			}
		}

		return true;
	}

	bool Model::ReadMesh(core::XLexer& lex, Mesh& mesh)
	{
	 /* MESH
		VERTS
		TRIS */
		// read the mesh count.
		uint32_t numVerts, numTris;

		if (!lex.ExpectTokenString("MESH")) {
			X_ERROR("RawModel", "Failed to read 'MESH' token");
			return false;
		}

		// optional name
		core::XLexToken token(nullptr, nullptr);
		if (lex.ReadToken(token)) {
			if (token.GetType() == core::TokenType::STRING) {
				mesh.name_ = Mesh::NameString(token.begin(), token.end());
			}
		}

		if (!ReadheaderToken(lex, "VERTS", numVerts)) {
			return false;
		}
		if (!ReadheaderToken(lex, "TRIS", numTris)) {
			return false;
		}

		const int32_t numBones = safe_static_cast<int32_t, size_t>(bones_.size());

		// all the verts
		mesh.verts_.resize(numVerts);
		for (size_t i = 0; i < mesh.verts_.size(); i++)
		{
		/*  v
			pos
			binds
		 */
			auto& vert = mesh.verts_[i];

			int32_t numBinds;
			if (!ReadheaderToken(lex, "v", numBinds)) {
				return false;
			}

			if (!lex.Parse1DMatrix(3, &vert.pos_[0])) {
				X_ERROR("RawModel", "Failed to read vert pos");
				return false;
			}

			// read binds.
			if (numBinds > safe_static_cast<int32_t, size_t>(vert.binds_.capacity())) {
				X_ERROR("RawModel", "Vert(%" PRIuS ") has too many binds. max: %" PRIuS,
					i, vert.binds_.capacity());
				return false;
			}

			vert.binds_.resize(numBinds);
			for (auto& bind : vert.binds_)
			{
				// boneIDx + weigth
				bind.boneIdx_ = lex.ParseInt();
				bind.weight_ = lex.ParseFloat();

				// check the bone idx is valid
				if (bind.boneIdx_ > numBones) {
					X_ERROR("RawModel", "Vert(%s" PRIuS ") bind has invalid bone idx: %" PRIi32 " max: %" PRIi32,
						i, bind.boneIdx_, numBones);
					return false;
				}
			}
		}


		// all the tri's
		mesh.tris_.resize(numTris);
		for (size_t i = 0; i < mesh.tris_.size(); i++)
		{
			if (!lex.ExpectTokenString("TRI")) {
				X_ERROR("RawModel", "Failed to read 'TRI' token");
				return false;
			}

			Tri& tri = mesh.tris_[i];

			// now we have a tri.
			for (size_t t = 0; t < 3; t++)
			{
				/* 
				index
				norn
				tan
				biNorm
				col
				uv
				*/

				TriVert& vert = tri[t];
				vert.index_ = lex.ParseInt();

				// validate the index.
				if (vert.index_ > numVerts) {
					X_ERROR("RawModel", "Tri(%" PRIuS ":%" PRIuS ") has invalid index of: %" PRIu32 " max: %" PRIu32, 
						i, t, vert.index_, numVerts);
					return false;
				}

				if (!lex.Parse1DMatrix(3, &vert.normal_[0])) {
					X_ERROR("RawModel", "Failed to read tri normal");
					return false;
				}
				if (!lex.Parse1DMatrix(3, &vert.tangent_[0])) {
					X_ERROR("RawModel", "Failed to read tri tangent");
					return false;
				}
				if (!lex.Parse1DMatrix(3, &vert.biNormal_[0])) {
					X_ERROR("RawModel", "Failed to read tri bi-normal");
					return false;
				}
				if (!lex.Parse1DMatrix(4, &vert.col_[0])) {
					X_ERROR("RawModel", "Failed to read tri col");
					return false;
				}
				if (!lex.Parse1DMatrix(2, &vert.uv_[0])) {
					X_ERROR("RawModel", "Failed to read tri uv");
					return false;
				}
			}
		}

		return true;
	}

	bool Model::ReadMaterial(core::XLexer& lex, Material& mat)
	{
		// name
		if (!lex.ExpectTokenString("MATERIAL")) {
			X_ERROR("RawModel", "Failed to read 'MATERIAL' token");
			return false;
		}

		core::XLexToken token(nullptr, nullptr);

		if (!lex.ReadToken(token)) {
			X_ERROR("RawModel", "Failed to read 'MATERIAL' name");
			return false;
		}
		if (token.GetType() != core::TokenType::STRING) {
			X_ERROR("RawModel", "Failed to read 'MATERIAL' name");
			return false;
		}

		mat.name_ = core::string(token.begin(), token.end());

		// col
		if (!ReadMaterialCol(lex, "COL", mat.col_)) {
			return false;
		}
		if (!ReadMaterialCol(lex, "TRANS", mat.tansparency_)) {
			return false;
		}
		if (!ReadMaterialCol(lex, "AMBIENTCOL", mat.ambientColor_)) {
			return false;
		}
		if (!ReadMaterialCol(lex, "SPECCOL", mat.specCol_)) {
			return false;
		}
		if (!ReadMaterialCol(lex, "REFLECTIVECOL", mat.reflectiveCol_)) {
			return false;
		}

		return true;
	}

	bool Model::ReadMaterialCol(core::XLexer& lex, const char* pName, Color& col)
	{
		if (!lex.SkipUntilString(pName)) {
			X_ERROR("RawModel", "Failed to find material '%s' token", pName);
			return false;
		}
		if (!lex.Parse1DMatrix(4, &col[0])) {
			X_ERROR("RawModel", "Failed to read material '%s' col", pName);
			return false;
		}
		return true;
	}

	bool Model::ReadheaderToken(core::XLexer& lex, const char* pName, core::UnitOfMeasure::Enum& units, bool optional)
	{
		core::XLexToken token(nullptr, nullptr);

		if (!lex.ReadToken(token)) {
			X_ERROR("RawModel", "Failed to read token");
			return false;
		}

		if (token.GetType() != core::TokenType::NAME || !token.isEqual(pName)) {
			if (optional) {
				lex.UnreadToken(token);
				return true;
			}

			X_ERROR("RawModel", "Failed to read '%s'", pName);
			return false;
		}

		// get value
		if (!lex.ReadToken(token)) {
			X_ERROR("RawModel", "Failed to read '%s' value", pName);
			return false;
		}

		if (token.GetType() != core::TokenType::NAME && token.GetType() != core::TokenType::STRING) {
			X_ERROR("RawModel", "Failed to read '%s' value, it's not of string type", pName);
			return false;
		}

		core::StackString256 unitStr(token.begin(), token.end());
		unitStr.toLower();

		switch (core::Hash::Fnv1aHash(unitStr.begin(), unitStr.length()))
		{
			case "centimeters"_fnv1a:
			case "cm"_fnv1a:
				units = core::UnitOfMeasure::Centimeters;
			case "in"_fnv1a:
			case "inches"_fnv1a:
				units = core::UnitOfMeasure::Inches;
			default:
				X_ERROR("RawModel", "Invalid unit of measure: \"%s\"", unitStr.c_str());
				return false;
		}

		return true;
	}

	bool Model::ReadheaderToken(core::XLexer& lex, const char* pName, int32_t& valOut)
	{
		core::XLexToken token(nullptr, nullptr);

		valOut = 0;

		if (!lex.SkipUntilString(pName)) {
			X_ERROR("RawModel", "Failed to find '%s' token", pName);
			return false;
		}

		// get value
		if (!lex.ReadToken(token)) {
			X_ERROR("RawModel", "Failed to read '%s' value", pName);
			return false;
		}

		if (token.GetType() != core::TokenType::NUMBER) {
			X_ERROR("RawModel", "Failed to read '%s' value, it's not of interger type", pName);
			return false;
		}

		valOut = token.GetIntValue();
		return true;
	}

	bool Model::ReadheaderToken(core::XLexer& lex, const char* pName, uint32_t& valOut)
	{
		core::XLexToken token(nullptr, nullptr);

		valOut = 0;

		if (!lex.SkipUntilString(pName)) {
			X_ERROR("RawModel", "Failed to find '%s' token", pName);
			return false;
		}

		// get value
		if (!lex.ReadToken(token)) {
			X_ERROR("RawModel", "Failed to read '%s' value", pName);
			return false;
		}

		if (token.GetType() != core::TokenType::NUMBER) {
			X_ERROR("RawModel", "Failed to read '%s' value, it's not of interger type", pName);
			return false;
		}

		valOut = static_cast<uint32_t>(token.GetIntValue());
		return true;
	}

	bool Model::SaveRawModel(core::Path<char>& path)
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pFileSys);

		path.setExtension(model::MODEL_RAW_FILE_EXTENSION);

		{
			core::XFileScoped file;
			core::fileModeFlags mode;
			mode.Set(core::fileMode::RECREATE);
			mode.Set(core::fileMode::WRITE);
			mode.Set(core::fileMode::SHARE);

			if (!gEnv->pFileSys->createDirectoryTree(path.c_str())) {
				X_ERROR("RawModel", "Failed to create export directory");
			}

			if (!file.openFile(path.c_str(), mode)) {
				X_ERROR("RawModel", "Failed to open file for rawmodel");
				return false;
			}
			
			ModelDataStrArr dataArr(arena_);
			if (!SaveRawModel_Int(dataArr)) {
				return false;
			}

			for(auto& buf : dataArr) {
				if (file.write(buf->c_str(), buf->length()) != buf->length()) {
					X_ERROR("RawModel", "Failed to write rawmodel header");
					break;
				}
			}

			for (auto& buf : dataArr) {
				X_DELETE(buf, arena_);
			}

		}

		return true;
	}

	bool Model::SaveRawModel(core::Array<uint8_t>& data)
	{
		ModelDataStrArr dataArr(arena_);

		if (!SaveRawModel_Int(dataArr)) {
			return false;
		}

		size_t totalBytes = 0;
		for (auto& buf : dataArr) {
			totalBytes += buf->length();
		}

		data.clear();
		data.resize(totalBytes);

		uint8_t* pBuf = data.ptr();
		for (auto& buf : dataArr) {		
			std::memcpy(pBuf, buf->c_str(), buf->length());
			pBuf += buf->length();
		}

		for (auto& buf : dataArr) {
			X_DELETE(buf, arena_);
		}

		return true;
	}

	bool Model::SaveRawModel_Int(ModelDataStrArr& dataArr) const
	{
		ModelDataStr* pCurBuf = X_NEW(ModelDataStr, arena_, "MeshDataStr");
		dataArr.append(pCurBuf);

		pCurBuf->appendFmt("// " X_ENGINE_NAME " engine RawModel.\n");

		// save some info it's not part of format
		for (size_t i = 0; i < lods_.size(); i++)
		{
			const model::RawModel::Lod& lod = lods_[i];

			pCurBuf->appendFmt("// LOD%" PRIuS " numMesh: %" PRIuS " verts: %" PRIuS " tris: %" PRIuS " \n",
				i, lod.meshes_.size(), lod.totalVerts(), lod.totalTris());
		}

		pCurBuf->append("\n");
		pCurBuf->appendFmt("VERSION %" PRIi32 "\n", VERSION);
		pCurBuf->appendFmt("UNITS %s\n", core::UnitOfMeasure::ToString(unitOfMeasure_));
		pCurBuf->appendFmt("LODS %" PRIuS "\n", lods_.size());
		pCurBuf->appendFmt("BONES %" PRIuS "\n", bones_.size());
		pCurBuf->append("\n");

		if (!WriteBones(dataArr))
		{
			X_ERROR("RawModel", "Failed to write bones");
			return false;
		}

		if (!WriteLods(dataArr))
		{
			X_ERROR("RawModel", "Failed to write lods");
			return false;
		}

		return true;
	}

	bool Model::WriteBones(ModelDataStrArr& dataArr) const
	{
		ModelDataStr* pCurBuf = X_NEW(ModelDataStr, arena_, "MeshDataStr");
		dataArr.append(pCurBuf);

		for (const auto& bone : bones_)
		{
			pCurBuf->appendFmt("BONE %" PRIi32 " \"%s\"\n", bone.parIndx_, bone.name_.c_str());
			pCurBuf->appendFmt("POS (%f %f %f)\n", bone.worldPos_.x, bone.worldPos_.y, bone.worldPos_.z);
			pCurBuf->appendFmt("SCALE (%f %f %f)\n", bone.scale_.x, bone.scale_.y, bone.scale_.z);
			const auto& ang = bone.rotation_;
			pCurBuf->appendFmt("ANG ((%f %f %f) (%f %f %f) (%f %f %f))\n",
				ang.m00, ang.m01, ang.m02, 
				ang.m10, ang.m11, ang.m12, 
				ang.m20, ang.m21, ang.m22);

			pCurBuf->append("\n");
		}

		pCurBuf->append("\n");
		return true;
	}

	bool Model::WriteLods(ModelDataStrArr& dataArr) const
	{
		for (const auto& lod : lods_)
		{
			ModelDataStr* pCurBuf = X_NEW(ModelDataStr, arena_, "MeshDataStr");
			dataArr.append(pCurBuf);

			pCurBuf->appendFmt("LOD\n");
			pCurBuf->appendFmt("NUMMESH %" PRIuS "\n", lod.meshes_.size());

			if (!WriteMeshes(dataArr, lod)) {
				return false;
			}

			for (const auto& mesh : lod.meshes_)
			{
				if (!WriteMaterial(dataArr, mesh.material_)) {
					return false;
				}
			}
		}

	//	f->writeStringNNT("\n");
		return true;
	}

	bool Model::WriteMeshes(ModelDataStrArr& dataArr, const Lod& lod) const
	{
		// if no job system use single threaded.
		if (!pJobSys_) {
			for (const auto& mesh : lod.meshes_) {
				if (!WriteMesh(dataArr, mesh, arena_)) {
					return false;
				}
			}

			return true;
		}

		const size_t numMesh = lod.meshes_.size();
		bool result = true;

		{
			MeshWriteDataArr data(arena_);
			data.setGranularity(numMesh);
			data.resize(numMesh, MeshWriteData(arena_));

			const RawModel::Mesh* pMesh = lod.meshes_.ptr();

			// generate the data for each mesh.
			for (size_t i = 0; i < numMesh; i++)
			{
				data[i].pMesh = &pMesh[i];

				core::V2::Job* pJob = pJobSys_->CreateJob(&WriteMeshDataJob, static_cast<void*>(&data[i]) JOB_SYS_SUB_ARG(core::profiler::SubSys::TOOL));
				pJobSys_->Run(pJob);
				
				data[i].pJob = pJob;
			}

			// wait for them to complete.
			for (size_t i = 0; i < data.size(); i++)
			{
				const MeshWriteData& jobData = data[i];

				pJobSys_->Wait(jobData.pJob);

				// merge in
				dataArr.append(jobData.data);
			}

			data.clear();
		}

		return result;
	}

	void Model::WriteMeshDataJob(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pJobData)
	{
		X_UNUSED(jobSys);
		X_UNUSED(threadIdx);
		X_UNUSED(pJob);

		MeshWriteData& data = *reinterpret_cast<MeshWriteData*>(pJobData);

		const Mesh& mesh = *data.pMesh;
		auto& dataArr = data.data;

		WriteMesh(dataArr, mesh, data.arena);
	}

	bool Model::WriteMesh(ModelDataStrArr& dataArr, const Mesh& mesh, core::MemoryArenaBase* arena)
	{
		ModelDataStr* pCurBuf = X_NEW(ModelDataStr, arena, "MeshDataStr");

		pCurBuf->appendFmt("MESH \"%s\"\n", mesh.name_.c_str());
		pCurBuf->appendFmt("VERTS %" PRIuS "\n", mesh.verts_.size());
		pCurBuf->appendFmt("TRIS %" PRIuS "\n", mesh.tris_.size());
		pCurBuf->append("\n");

		// ok now we start writing the data.
		for (const auto& vert : mesh.verts_)
		{
			const Vec3f& pos = vert.pos_;

			pCurBuf->appendFmt("v %" PRIuS "\n(%f %f %f)\n",
				vert.binds_.size(),
				pos.x, pos.y, pos.z
			);

			for (const auto& bind : vert.binds_)
			{
				pCurBuf->appendFmt("%" PRIi32 " %f\n", bind.boneIdx_, bind.weight_);
			}

			pCurBuf->append("\n");

			if ((pCurBuf->length() + 500) > pCurBuf->capacity())
			{
				dataArr.append(pCurBuf);
				pCurBuf = X_NEW(ModelDataStr, arena, "MeshDataStr");
			}
		}

		if ((pCurBuf->length() + 64) > pCurBuf->capacity())
		{
			dataArr.append(pCurBuf);
			pCurBuf = X_NEW(ModelDataStr, arena, "MeshDataStr");
		}


		for (const auto& tri : mesh.tris_)
		{
			pCurBuf->append("TRI\n");

			for (size_t i = 0; i < 3; i++)
			{
				const TriVert& triVert = tri[i];

				const Index& index = triVert.index_;
				const Vec3f& normal = triVert.normal_;
				const Vec3f& tan = triVert.tangent_;
				const Vec3f& biNormal = triVert.biNormal_;
				const Color& col = triVert.col_;
				const Vec2f& uv = triVert.uv_;

				pCurBuf->appendFmt("%" PRIu32 "\n(%f %f %f)\n(%f %f %f)\n(%f %f %f)\n(%f %f %f %f)\n(%f %f)\n",
					index,
					normal.x, normal.y, normal.z,
					tan.x, tan.y, tan.z,
					biNormal.x, biNormal.y, biNormal.z,
					col.r, col.g, col.b, col.a,
					uv.x, uv.y
				);

				if ((pCurBuf->length() + 256) > pCurBuf->capacity())
				{
					dataArr.append(pCurBuf);
					pCurBuf = X_NEW(ModelDataStr, arena, "MeshDataStr");
				}
			}

			pCurBuf->append("\n");
		}

		pCurBuf->append("\n");

		dataArr.append(pCurBuf);
		return true;
	}

	bool Model::WriteMaterial(ModelDataStrArr& dataArr, const Material& mat) const
	{
		const Color& col = mat.col_;
		const Color& tansparency = mat.tansparency_;
		const Color& ambientColor = mat.ambientColor_;
		const Color& specCol = mat.specCol_;
		const Color& reflectiveCol = mat.reflectiveCol_;

		ModelDataStr* pCurBuf = X_NEW(ModelDataStr, arena_, "MeshDataStr");
		dataArr.append(pCurBuf);

		pCurBuf->appendFmt("MATERIAL \"%s\"\n", mat.name_.c_str());
		pCurBuf->appendFmt("COL (%f %f %f %f)\n", col.r, col.g, col.b, col.a);
		pCurBuf->appendFmt("TRANS (%f %f %f %f)\n", tansparency.r, tansparency.g, tansparency.b, tansparency.a);
		pCurBuf->appendFmt("AMBIENTCOL (%f %f %f %f)\n", ambientColor.r, ambientColor.g, ambientColor.b, ambientColor.a);
		pCurBuf->appendFmt("SPECCOL (%f %f %f %f)\n", specCol.r, specCol.g, specCol.b, specCol.a);
		pCurBuf->appendFmt("REFLECTIVECOL (%f %f %f %f)\n", reflectiveCol.r, reflectiveCol.g, reflectiveCol.b, reflectiveCol.a);
		pCurBuf->append("\n");
		return true;
	}

	bool Model::isColisionMesh(const RawModel::Mesh::NameString& name, ColMeshType::Enum* pType)
	{
		static_assert(ColMeshType::ENUM_COUNT == 3, "Added additional col mesh types? this code needs updating");

		const char* pFind = nullptr;

		// PBX_ * _ *
		if ((pFind = name.find(MODEL_MESH_COL_BOX_PREFIX)) != nullptr)
		{
			if (pFind != name.begin()) {
				goto ignore;
			}

			if (pType) {
				*pType = ColMeshType::BOX;
			}
			return true;
		}
		// PSP_ * _ *
		if ((pFind = name.find(MODEL_MESH_COL_SPHERE_PREFIX)) != nullptr)
		{
			if (pFind != name.begin()) {
				goto ignore;
			}

			if (pType) {
				*pType = ColMeshType::SPHERE;
			}
			return true;
		}
		// PCX_ * _ *
		if ((pFind = name.find(MODEL_MESH_COL_CONVEX_PREFIX)) != nullptr)
		{
			if (pFind != name.begin()) {
				goto ignore;
			}

			if (pType) {
				*pType = ColMeshType::CONVEX;
			}
			return true;
		}

		return false;

	ignore:
		X_WARNING("Model", "Mesh name \"%s\" contains collision prefix that is not leading, ignoring", name.c_str());
		return false;
	}

} // namespace RawModel

X_NAMESPACE_END