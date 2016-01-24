#include "stdafx.h"
#include "RawModelTypes.h"
#include "RawModel.h"

#include <IModel.h>
#include <IFileSys.h>

#include <String\Lexer.h>


X_NAMESPACE_BEGIN(model)

namespace RawModel
{
	const int32_t Model::VERSION = MODEL_RAW_VERSION;


	Model::Model(core::MemoryArenaBase* arena) :
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
			size_t fileSize = file.remainingBytes();

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
		if (numLods < model::MODEL_MAX_LODS) {
			X_WARNING("RawModel", "RawModel has no many lods: %i max", 
				model::MODEL_MAX_LODS);
			return true;
		}

		if (!ReadBones(lex, numBones)) {
			X_ERROR("RawModel", "failed to parse bone data");
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
				X_ERROR("RawModel", "Failed to read 'BONE' token");
				return false;
			}

			if (token.GetType() != core::TokenType::NUMBER) {
				X_ERROR("RawModel", "Failed to read 'BONE' token");
				return false;
			}

			bone.parIndx_ = token.GetIntValue();

			// read the string
			if (!lex.ReadTokenOnLine(token)) {
				X_ERROR("RawModel", "Failed to read 'BONE' token");
				return false;
			}

			if (token.GetType() != core::TokenType::STRING) {
				X_ERROR("RawModel", "Failed to read 'BONE' token");
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

			if(lex.Parse2DMatrix(3, 3, &bone.rotation_[0])) {
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

			lod.meshes_.resize(numMesh, Mesh(arena_));

			for (auto& mesh : lod.meshes_)
			{
				if (!ReadMesh(lex, mesh)) {
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
			if (!lex.ExpectTokenString("v")) {
				X_ERROR("RawModel", "Failed to read 'v' token");
				return false;
			}

			face[0] = lex.ParseInt();
			face[1] = lex.ParseInt();
			face[2] = lex.ParseInt();
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

		FILE* f;
		errno_t err = fopen_s(&f, path.c_str(), "wb");
		if (f)
		{
			fprintf(f, "VERSION %i\n", VERSION);
			fprintf(f, "LODS %" PRIuS "\n", lods_.size());
			fprintf(f, "BONES %" PRIuS "\n", bones_.size());
			fputs("\n", f);

			if (!WriteBones(f)) {
				::fclose(f);
				return false;
			}

			if (!WriteLods(f)) {
				::fclose(f);
				return false;
			}

			::fclose(f);
			return true;
		}

		return false;
	}

	bool Model::WriteBones(FILE* f) const
	{
		X_ASSERT_NOT_NULL(f);

		for (const auto& bone : bones_)
		{
			fprintf(f, "BONE %i \"%s\"\n", bone.parIndx_, bone.name_.c_str());
			fprintf(f, "POS (%g,%g,%g)\n", bone.worldPos_.x, bone.worldPos_.y, bone.worldPos_.z);
			auto ang = bone.rotation_;
			fprintf(f, "ANG (%g,%g,%g) (%g,%g,%g) (%g,%g,%g)\n", 
				ang.m00, ang.m01, ang.m02, 
				ang.m10, ang.m11, ang.m12, 
				ang.m20, ang.m21, ang.m22);

			fputs("\n", f);
		}

		fputs("\n", f);
		return true;
	}

	bool Model::WriteLods(FILE* f) const
	{
		X_ASSERT_NOT_NULL(f);

		for (const auto& lod : lods_)
		{
			fprintf(f, "LOD");
			fprintf(f, "DISTANCE %g\n", lod.distance_);
			fprintf(f, "NUMMESH %" PRIuS "\n", lod.meshes_.size());

			for (const auto& mesh : lod.meshes_)
			{
				if (!WriteMesh(f, mesh)) {
					return false;
				}
			}

			for (const auto& mesh : lod.meshes_)
			{
				if (!WriteMaterials(f, mesh.materials_)) {
					return false;
				}
			}
		}

		fputs("\n", f);
		return true;
	}

	bool Model::WriteMesh(FILE* f, const Mesh& mesh) const
	{
		X_ASSERT_NOT_NULL(f);

		fprintf(f, "MESH\n");
		fprintf(f, "VERTS %" PRIuS "\n", mesh.verts_.size());
		fprintf(f, "FACES %" PRIuS "\n", mesh.face_.size());
		fputs("\n", f);

		for (const auto& vert : mesh.verts_)
		{
			const Vec3f& pos = vert.pos_;
			const Vec3f& normal = vert.normal_;
			const Color& col = vert.col_;
			const Vec2f& uv = vert.uv_;

			fprintf(f, "v %" PRIuS "\n", vert.binds_.size());
			fprintf(f, "(%g,%g,%g)\n", pos.x, pos.y, pos.z);
			fprintf(f, "(%g,%g,%g)\n", normal.x, normal.y, normal.z);
			fprintf(f, "(%g,%g,%g,%g)\n", col.r, col.g, col.b, col.a);
			fprintf(f, "(%g,%g)\n", uv.x, uv.y);

			for (const auto& bind : vert.binds_)
			{
				fprintf(f, "%i %f\n",bind.boneIdx_, bind.weight_);
			}

			fputs("\n", f);
		}

		for (const auto& face : mesh.face_)
		{
			fprintf(f, "f (%i,%i,%i)\n", face[0], face[1], face[2]);
		}

		fputs("\n", f);
		return true;
	}

	bool Model::WriteMaterials(FILE* f, const Material& mat) const
	{
		X_ASSERT_NOT_NULL(f);

		const Color& col = mat.col_;
		const Color& tansparency = mat.tansparency_;
		const Color& ambientColor = mat.ambientColor_;
		const Color& specCol = mat.specCol_;
		const Color& reflectiveCol = mat.reflectiveCol_;


		fprintf(f, "MATERIAL \"%s\"\n", mat.name_.c_str());
		fprintf(f, "COL (%g,%g,%g,%g)\n", col.r, col.g, col.b, col.a);
		fprintf(f, "TRANS (%g,%g,%g,%g)\n", tansparency.r, tansparency.g, tansparency.b, tansparency.a);
		fprintf(f, "AMBIENTCOL (%g,%g,%g,%g)\n", ambientColor.r, ambientColor.g, ambientColor.b, ambientColor.a);
		fprintf(f, "SPECCOL (%g,%g,%g,%g)\n", specCol.r, specCol.g, specCol.b, specCol.a);
		fprintf(f, "REFLECTIVECOL (%g,%g,%g,%g)\n", reflectiveCol.r, reflectiveCol.g, reflectiveCol.b, reflectiveCol.a);

		fputs("\n", f);
		return true;
	}

} // namespace RawModel

X_NAMESPACE_END