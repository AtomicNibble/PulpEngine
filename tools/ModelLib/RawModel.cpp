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

namespace RawModel
{
	const int32_t Model::VERSION = MODEL_RAW_VERSION;


	Model::Model(core::MemoryArenaBase* arena, core::V2::JobSystem* pJobSys) :
		pJobSys_(pJobSys),
		arena_(arena),
		bones_(arena)
	{

	}

	void Model::Clear(void)
	{
		bones_.clear();
		lods_.clear();
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

	bool Model::ParseRawModel(core::XLexer& lex)
	{
		int32_t version, numLods, numBones;

		if (!ReadheaderToken(lex, "VERSION", version)) {
			return false;
		}
		if (!ReadheaderToken(lex, "LODS", numLods)) {
			return false;
		}
		if (!ReadheaderToken(lex, "BONES", numBones)) {
			return false;
		}


		if (version < model::MODEL_RAW_VERSION) {
			X_ERROR("RawModel", "RawModel file version is too old: %i required: %i",
				version, model::MODEL_RAW_VERSION);
			return false;
		}

		if (numLods < 1) {
			X_WARNING("RawModel", "RawModel has no lods");
			return true;
		}
		if (numLods > model::MODEL_MAX_LODS) {
			X_WARNING("RawModel", "RawModel has no many lods: %i max", 
				model::MODEL_MAX_LODS);
			return true;
		}
		if (numBones > model::MODEL_MAX_BONES) {
			X_ERROR("RaWModel", "model has too many bones: %i max: %i",
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


		return true;
	}

	bool Model::ReadBones(core::XLexer& lex, int32_t numBones)
	{
		bones_.resize(numBones);

		core::XLexToken token(nullptr, nullptr);

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

			// read the distance.
			if (!lex.ReadToken(token)) {
				X_ERROR("RawModel", "Failed to read 'DISTANCE' token");
				return false;
			}

			if (!token.isEqual("DISTANCE")) {
				X_ERROR("RawModel", "Failed to read 'DISTANCE' token");
				return false;
			}

			// read the distance value
			if (!lex.ReadTokenOnLine(token)) {
				X_ERROR("RawModel", "Failed to read 'DISTANCE' token");
				return false;
			}

			if (token.GetType() != core::TokenType::NUMBER) {
				X_ERROR("RawModel", "Failed to read 'DISTANCE' token");
				return false;
			}

			lod.distance_ = token.GetFloatValue();

			// read the mesh count.
			int32_t numMesh;

			if (!ReadheaderToken(lex, "NUMMESH", numMesh)) {
				return false;
			}

			if (numMesh > model::MODEL_MAX_MESH) {
				X_ERROR("RaWModel", "lod has too many mesh: %i max: %i",
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
		FACES */
		// read the mesh count.
		int32_t numVerts, numFaces;

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
		if (!ReadheaderToken(lex, "FACES", numFaces)) {
			return false;
		}

		// all the verts
		mesh.verts_.resize(numVerts);
		for (auto& vert : mesh.verts_)
		{
		/*  v
			pos
			normal
			tan
			biNorm
			col
			uv */
			int32_t numBinds;

			if (!ReadheaderToken(lex, "v", numBinds)) {
				return false;
			}

			if (!lex.Parse1DMatrix(3, &vert.pos_[0])) {
				X_ERROR("RawModel", "Failed to read vert pos");
				return false;
			}
			if (!lex.Parse1DMatrix(3, &vert.normal_[0])) {
				X_ERROR("RawModel", "Failed to read vert normal");
				return false;
			}
			if (!lex.Parse1DMatrix(3, &vert.tangent_[0])) {
				X_ERROR("RawModel", "Failed to read vert tangent");
				return false;
			}
			if (!lex.Parse1DMatrix(3, &vert.biNormal_[0])) {
				X_ERROR("RawModel", "Failed to read vert bi-normal");
				return false;
			}
			if (!lex.Parse1DMatrix(4, &vert.col_[0])) {
				X_ERROR("RawModel", "Failed to read vert col");
				return false;
			}
			if (!lex.Parse1DMatrix(2, &vert.uv_[0])) {
				X_ERROR("RawModel", "Failed to read vert uv");
				return false;
			}

			// read bines.
			if (numBinds > safe_static_cast<int32_t, size_t>(vert.binds_.capacity())) {
				X_ERROR("RawModel", "Vert has too many binds. max: %" PRIuS,
					vert.binds_.capacity());
				return false;
			}

			vert.binds_.resize(numBinds);
			for (auto bind : vert.binds_)
			{
				// boneIDx + weigth
				bind.boneIdx_ = lex.ParseInt();
				bind.weight_ = lex.ParseFloat();
			}
		}

		// all the faces
		mesh.face_.resize(numFaces);
		for (auto& face : mesh.face_)
		{
			if (!lex.ExpectTokenString("f")) {
				X_ERROR("RawModel", "Failed to read 'f' token");
				return false;
			}

			face[0] = lex.ParseInt();
			face[1] = lex.ParseInt();
			face[2] = lex.ParseInt();
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

			{
				if (!file.openFile(path.c_str(), mode))
				{
					X_ERROR("RawModel", "Failed to open file for rawmodel");
					return false;
				}
			}

			core::StackString<1024, char> buf;
			buf.appendFmt("// Potato engine RawModel.\n");

			// save some info it's not part of format
			for (size_t i = 0; i < lods_.size(); i++)
			{
				const model::RawModel::Lod& lod = lods_[i];

				buf.appendFmt("// LOD%" PRIuS " dis: %f numMesh: %" PRIuS " verts: %" PRIuS " faces: %" PRIuS " \n",
					i, lod.distance_, lod.meshes_.size(), lod.totalVerts(), lod.totalIndexs());
			}

			buf.append("\n");
			buf.appendFmt("VERSION %i\n", VERSION);
			buf.appendFmt("LODS %" PRIuS "\n", lods_.size());
			buf.appendFmt("BONES %" PRIuS "\n", bones_.size());
			buf.append("\n");

			{
				if (file.write(buf.c_str(), buf.length()) != buf.length()) {
					X_ERROR("RawModel", "Failed to write rawmodel header");
					return false;
				}
			}

			{
				if (!WriteBones(file.GetFile()))
				{
					X_ERROR("RawModel", "Failed to write bones");
					return false;
				}
			}
			{
				if (!WriteLods(file.GetFile()))
				{
					X_ERROR("RawModel", "Failed to write lods");
					return false;
				}
			}

		}

		return true;
	}

	bool Model::WriteBones(core::XFile* f) const
	{
		X_ASSERT_NOT_NULL(f);
		core::StackString<1024, char> buf;

		for (const auto& bone : bones_)
		{
			buf.clear();
			buf.appendFmt("BONE %i \"%s\"\n", bone.parIndx_, bone.name_.c_str());
			buf.appendFmt("POS (%f %f %f)\n", bone.worldPos_.x, bone.worldPos_.y, bone.worldPos_.z);
			auto ang = bone.rotation_;
			buf.appendFmt("ANG ((%f %f %f) (%f %f %f) (%f %f %f))\n", 
				ang.m00, ang.m01, ang.m02, 
				ang.m10, ang.m11, ang.m12, 
				ang.m20, ang.m21, ang.m22);

			buf.append("\n");

			if (f->write(buf.c_str(), buf.length()) != buf.length()) {
				X_ERROR("RawModel", "Failed to write mesh bone");
				return false;
			}
		}

		f->writeStringNNT("\n");
		return true;
	}

	bool Model::WriteLods(core::XFile* f) const
	{
		X_ASSERT_NOT_NULL(f);

		for (const auto& lod : lods_)
		{
			core::StackString<1024, char> buf;

			buf.appendFmt("LOD\n");
			buf.appendFmt("DISTANCE %f\n", lod.distance_);
			buf.appendFmt("NUMMESH %" PRIuS "\n", lod.meshes_.size());

			if (f->write(buf.c_str(), buf.length()) != buf.length()) {
				X_ERROR("RawModel", "Failed to write mesh header");
				return false;
			}

			if (!WriteMeshes(f, lod)) {
				return false;
			}

			for (const auto& mesh : lod.meshes_)
			{
				if (!WriteMaterials(f, mesh.material_)) {
					return false;
				}
			}
		}

		f->writeStringNNT("\n");
		return true;
	}

	bool Model::WriteMeshes(core::XFile* f, const Lod& lod) const
	{
		// if no job system use single threaded.
		if (!pJobSys_) {
			for (const auto& mesh : lod.meshes_) {
				if (!WriteMesh(f, mesh)) {
					return false;
				}
			}

			return true;
		}


		typedef core::MemoryArena<
			core::MallocFreeAllocator,
			core::MultiThreadPolicy<core::Spinlock>,
#if X_DEBUG
			core::SimpleBoundsChecking,
			core::SimpleMemoryTracking,
			core::SimpleMemoryTagging
#else
			core::NoBoundsChecking,
			core::NoMemoryTracking,
			core::NoMemoryTagging
#endif // !X_DEBUG
		> MultiThreadedArena;

		const size_t numMesh = lod.meshes_.size();
		bool result = true;

		{
			core::MallocFreeAllocator allocator;
			MultiThreadedArena arena(&allocator, "RawModelMeshArena");

			MeshWriteDataArr data(&arena);
			data.setGranularity(numMesh);
			data.resize(numMesh, MeshWriteData(&arena));

			const RawModel::Mesh* pMesh = lod.meshes_.ptr();

			// generate the data for each mesh.
			for (size_t i = 0; i < numMesh; i++)
			{
				data[i].pMesh = &pMesh[i];

				core::V2::Job* pJob = pJobSys_->CreateJob(&WriteMeshDataJob, static_cast<void*>(&data[i]));
				pJobSys_->Run(pJob);
				
				data[i].pJob = pJob;
			}

			// wait for them to complete.
			for (size_t i = 0; i < data.size(); i++)
			{
				const MeshWriteData& jobData = data[i];

				pJobSys_->Wait(jobData.pJob);

				// we can write this job without waiting for the others since we are waiting in order.
				for (const auto& buf : jobData.data)
				{
					if (f->write(buf->c_str(), buf->length()) != buf->length()) {
						X_ERROR("RawModel", "Failed to write mesh data");
						result = false;
					}
				}

				// delete them
				for (const auto& buf : jobData.data)
				{
					X_DELETE(buf, &arena);
				}
			}

			data.clear();
		}

		return result;
	}

	void Model::WriteMeshDataJob(core::V2::JobSystem* pJobSys, size_t threadIdx, core::V2::Job* pJob, void* pJobData)
	{
		X_UNUSED(pJobSys);
		X_UNUSED(threadIdx);
		X_UNUSED(pJob);

		MeshWriteData& data = *reinterpret_cast<MeshWriteData*>(pJobData);

		const size_t numVerts = data.pMesh->verts_.size();
		const size_t numFace = data.pMesh->face_.size();


		data.data.setGranularity(4);

		const Mesh& mesh = *data.pMesh;
		MeshDataStrArr& dataArr = data.data;

		MeshDataStr* pCurBuf = X_NEW(MeshDataStr, data.arena, "MeshDataStr");

		pCurBuf->appendFmt("MESH \"%s\"\n", mesh.name_.c_str());
		pCurBuf->appendFmt("VERTS %" PRIuS "\n", mesh.verts_.size());
		pCurBuf->appendFmt("FACES %" PRIuS "\n", mesh.face_.size());
		pCurBuf->append("\n");

		// ok now we start writing the data.
		for (const auto& vert : mesh.verts_)
		{
			const Vec3f& pos = vert.pos_;
			const Vec3f& normal = vert.normal_;
			const Vec3f& tan = vert.tangent_;
			const Vec3f& biNormal = vert.biNormal_;
			const Color& col = vert.col_;
			const Vec2f& uv = vert.uv_;

			pCurBuf->appendFmt("v %" PRIuS "\n(%f %f %f)\n(%f %f %f)\n(%f %f %f)\n(%f %f %f)\n(%f %f %f %f)\n(%f %f)\n",
				vert.binds_.size(), 
				pos.x, pos.y, pos.z,
				normal.x, normal.y, normal.z,
				tan.x, tan.y, tan.z,
				biNormal.x, biNormal.y, biNormal.z,
				col.r, col.g, col.b, col.a,
				uv.x, uv.y
			);


			for (const auto& bind : vert.binds_)
			{
				pCurBuf->appendFmt("%i %f\n", bind.boneIdx_, bind.weight_);
			}

			pCurBuf->append("\n");

			if ((pCurBuf->length() + 500) > pCurBuf->capacity())
			{
				dataArr.append(pCurBuf);
				pCurBuf = X_NEW(MeshDataStr, data.arena, "MeshDataStr");
			}
		}

		if ((pCurBuf->length() + 64) > pCurBuf->capacity())
		{
			dataArr.append(pCurBuf);
			pCurBuf = X_NEW(MeshDataStr, data.arena, "MeshDataStr");
		}

		// lets unroll this a bit.
		const size_t unrollNum = 4;
		const size_t numLoops = mesh.face_.size() / unrollNum;
		const size_t trailing = mesh.face_.size() % unrollNum;

		for (size_t i = 0; i < numLoops; i++)
		{
			const Index* pFaceIdx = &mesh.face_[i * unrollNum][0];

			// write 4 out
			pCurBuf->appendFmt("f %i %i %i\n"
				"f %i %i %i\n"
				"f %i %i %i\n"
				"f %i %i %i\n", 
				pFaceIdx[0], pFaceIdx[1], pFaceIdx[2],
				pFaceIdx[3], pFaceIdx[4], pFaceIdx[5],
				pFaceIdx[6], pFaceIdx[7], pFaceIdx[8],
				pFaceIdx[9], pFaceIdx[10], pFaceIdx[11]
			);

			if ((pCurBuf->length() + 128) > pCurBuf->capacity())
			{
				dataArr.append(pCurBuf);
				pCurBuf = X_NEW(MeshDataStr, data.arena, "MeshDataStr");
			}
		}

		for (size_t i = 0; i < trailing; i++)
		{
			const Face& face = mesh.face_[(unrollNum * numLoops) + i];

			pCurBuf->appendFmt("f %i %i %i\n", face[0], face[1], face[2]);

			if ((pCurBuf->length() + 64) > pCurBuf->capacity())
			{
				dataArr.append(pCurBuf);
				pCurBuf = X_NEW(MeshDataStr, data.arena, "MeshDataStr");
			}
		}

		pCurBuf->append("\n");

		dataArr.append(pCurBuf);
	}

	bool Model::WriteMesh(core::XFile* f, const Mesh& mesh) const
	{
		X_ASSERT_NOT_NULL(f);
		core::StackString<1024 * 16, char> buf;

		buf.appendFmt("MESH \"%s\"\n", mesh.name_.c_str());
		buf.appendFmt("VERTS %" PRIuS "\n", mesh.verts_.size());
		buf.appendFmt("FACES %" PRIuS "\n", mesh.face_.size());
		buf.append("\n");

		if (f->write(buf.c_str(), buf.length()) != buf.length()) {
			X_ERROR("RawModel", "Failed to write mesh header");
			return false;
		}

		buf.clear();

		for (const auto& vert : mesh.verts_)
		{
			const Vec3f& pos = vert.pos_;
			const Vec3f& normal = vert.normal_;
			const Vec3f& tan = vert.tangent_;
			const Vec3f& biNormal = vert.biNormal_;
			const Color& col = vert.col_;
			const Vec2f& uv = vert.uv_;

			buf.appendFmt("v %" PRIuS "\n", vert.binds_.size());
			buf.appendFmt("(%f %f %f)\n", pos.x, pos.y, pos.z);
			buf.appendFmt("(%f %f %f)\n", normal.x, normal.y, normal.z);
			buf.appendFmt("(%f %f %f)\n", tan.x, tan.y, tan.z);
			buf.appendFmt("(%f %f %f)\n", biNormal.x, biNormal.y, biNormal.z);
			buf.appendFmt("(%f %f %f %f)\n", col.r, col.g, col.b, col.a);
			buf.appendFmt("(%f %f)\n", uv.x, uv.y);

			for (const auto& bind : vert.binds_)
			{
				buf.appendFmt("%i %f\n",bind.boneIdx_, bind.weight_);
			}

			buf.append("\n");

			// write it, if less that 1024 bytes free.
			if (buf.length() + 1024 > buf.capacity())
			{
				if (f->write(buf.c_str(), buf.length()) != buf.length()) {
					X_ERROR("RawModel", "Failed to write mesh vert");
					return false;
				}

				buf.clear();
			}
		}

		// buf might not be empty.

		for (const auto& face : mesh.face_)
		{		
			buf.appendFmt("f %i %i %i\n", face[0], face[1], face[2]);

			// write it, if less that 256 bytes free.
			if (buf.length() + 256 > buf.capacity())
			{
				if (f->write(buf.c_str(), buf.length()) != buf.length()) {
					X_ERROR("RawModel", "Failed to write mesh face");
					return false;
				}

				buf.clear();
			}
		}

		// flush
		if (buf.isNotEmpty()) {
			if (f->write(buf.c_str(), buf.length()) != buf.length()) {
				X_ERROR("RawModel", "Failed to flush mesh buf");
				return false;
			}

			buf.clear();
		}


		f->writeStringNNT("\n");
		return true;
	}

	bool Model::WriteMaterials(core::XFile* f, const Material& mat) const
	{
		X_ASSERT_NOT_NULL(f);

		const Color& col = mat.col_;
		const Color& tansparency = mat.tansparency_;
		const Color& ambientColor = mat.ambientColor_;
		const Color& specCol = mat.specCol_;
		const Color& reflectiveCol = mat.reflectiveCol_;

		core::StackString<1024, char> buf;

		buf.appendFmt("MATERIAL \"%s\"\n", mat.name_.c_str());
		buf.appendFmt("COL (%f %f %f %f)\n", col.r, col.g, col.b, col.a);
		buf.appendFmt("TRANS (%f %f %f %f)\n", tansparency.r, tansparency.g, tansparency.b, tansparency.a);
		buf.appendFmt("AMBIENTCOL (%f %f %f %f)\n", ambientColor.r, ambientColor.g, ambientColor.b, ambientColor.a);
		buf.appendFmt("SPECCOL (%f %f %f %f)\n", specCol.r, specCol.g, specCol.b, specCol.a);
		buf.appendFmt("REFLECTIVECOL (%f %f %f %f)\n", reflectiveCol.r, reflectiveCol.g, reflectiveCol.b, reflectiveCol.a);
		buf.append("\n");
		
		return f->write(buf.c_str(), buf.length()) == buf.length();
	}

} // namespace RawModel

X_NAMESPACE_END