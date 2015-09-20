#include "stdafx.h"
#include "ModelExporter.h"


#include <maya\MItDag.h>
#include <maya\MFnMesh.h>
#include <maya\MPointArray.h>
#include <maya\MFloatVectorArray.h>
#include <maya\MFloatArray.h>
#include <maya\MFloatPointArray.h>
#include <maya\MDagPath.h>
#include <maya\MDagPathArray.h>
#include <maya\MSelectionList.h>
#include <maya\MItSelectionList.h>
#include <maya\MGlobal.h>
#include <maya\MPlug.h>
#include <maya\MFnSet.h>
#include <maya\MItMeshPolygon.h>
#include <maya\MItDependencyGraph.h>
#include <maya/MProgressWindow.h>
#include <maya/MFnAttribute.h>
#include <maya/MFnMatrixData.h>
#include <maya/MMatrix.h>
#include <maya/MFnSkinCluster.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MItGeometry.h>
#include <maya/MArgList.h>

X_DISABLE_WARNING(4702)
#include <algorithm>
#include <map>
X_ENABLE_WARNING(4702)

#include <String\StringTokenizer.h>
#include <String\StackString.h>
#include <String\Path.h>
#include <Containers\ByteStream.h>
#include <Containers\FixedStack.h>

#include <Threading\Thread.h>

#include "Profiler.h"

double MayaProfiler::s_frequency = 0.0;

struct SCoreGlobals* gEnv = nullptr;

bool g_StartOfBlock = false; // used for logging formating.


X_LINK_LIB(X_STRINGIZE(MAYA_SDK) "\\Foundation")
X_LINK_LIB(X_STRINGIZE(MAYA_SDK) "\\OpenMaya")
X_LINK_LIB(X_STRINGIZE(MAYA_SDK) "\\OpenMayaUI")
X_LINK_LIB(X_STRINGIZE(MAYA_SDK) "\\OpenMayaAnim")

X_USING_NAMESPACE;

namespace
{
	static ModelStats g_stats;
	static PotatoOptions g_options;

	static const float MERGE_VERTEX_EPSILON = 0.9f;
	static const float MERGE_TEXCORDS_EPSILON = 0.5f;
	static const float JOINT_WEIGHT_THRESHOLD = 0.005f;
	static const int   VERTEX_MAX_WEIGHTS = 4;

	::std::ostream& operator<<(::std::ostream& os, const Vec3f& bar) {
		return os << "(" << bar.x << ", " << bar.y << ", " << bar.z << ")";
	}

	static void MayaPrintError(const char *fmt, ...)
	{
		va_list	argptr;
		char	msg[2048];

		va_start(argptr, fmt);
		vsnprintf_s(msg, sizeof(msg), fmt, argptr);
		va_end(argptr);


		if (g_StartOfBlock) {
			std::cout << "\n";
			g_StartOfBlock = false;
		}

		std::cerr << "Error: " << msg << std::endl;
		MGlobal::displayError(msg);
	}

	static void MayaPrintWarning(const char *fmt, ...)
	{
		va_list	argptr;
		char	msg[2048];

		va_start(argptr, fmt);
		vsnprintf_s(msg, sizeof(msg), fmt, argptr);
		va_end(argptr);

		if (g_StartOfBlock) {
			std::cout << "\n";
			g_StartOfBlock = false;
		}

		std::cerr << "Warning: " << msg << std::endl;
		MGlobal::displayWarning(msg);
	}

	static void MayaPrintMsg(const char *fmt, ...)
	{
		va_list	argptr;
		char	msg[2048];

		va_start(argptr, fmt);
		vsnprintf_s(msg, sizeof(msg), fmt, argptr);
		va_end(argptr);

		if (g_StartOfBlock) {
			std::cout << "\n";
			g_StartOfBlock = false;
		}

		std::cout << msg << std::endl;
	}


	Vec3f ConvertToGameSpace(const Vec3f &pos) {
		Vec3f idpos;
		// we are Z up.
		idpos.x = pos.x;
	
		// idpos.y = -pos.z;
		// idpos.z = pos.y;

		idpos.y = pos.y;
		idpos.z = pos.z;
		return idpos;
	}

	Matrix33f ConvertToGameSpace(const Matrix33f& m) {
		Matrix33f mat;

		mat.m00 = m.m00;
		mat.m01 = m.m02;
		mat.m02 = m.m01;

		mat.m10 = m.m10;
		mat.m11 = m.m12;
		mat.m12 = m.m11;

		mat.m20 = m.m20;
		mat.m21 = m.m22;
		mat.m22 = m.m21;
		return mat;
	}

	Vec3f XVec(const MFloatPoint& point) {
		return Vec3f(point[0], point[1], point[2]);
	}

	Color XVec(const MColor& col) {
		return Color(col[0], col[1], col[2], col[3]);
	}

	Vec3f XVec(const MMatrix& matrix) {
		return Vec3f((float)matrix[3][0], (float)matrix[3][1], (float)matrix[3][2]);
	}

	Matrix33f XMat(const MMatrix& matrix) {
		int		j, k;
		Matrix33f	mat;

		for (j = 0; j < 3; j++)
			for (k = 0; k < 3; k++)
				mat.at(j, k) = (float)matrix[j][k];

		return mat;
	}


	MObject FindShader(MObject& setNode)
	{
		MStatus				status;
		MFnDependencyNode	fnNode(setNode);
		MPlug				shaderPlug;

		shaderPlug = fnNode.findPlug("surfaceShader");
		if (!shaderPlug.isNull()) {
			MPlugArray connectedPlugs;
			bool asSrc = false;
			bool asDst = true;
			shaderPlug.connectedTo(connectedPlugs, asDst, asSrc, &status);

			if (connectedPlugs.length() != 1) {
				MayaPrintError("FindShader: Error getting shader (%s)", status.errorString().asChar());
			}
			else {
				return connectedPlugs[0].node();
			}
		}

		return MObject::kNullObj;
	}

	bool GetMeshMaterial(MayaMesh* mesh, MDagPath &dagPath)
	{
		MStatus	 status;
		MDagPath path = dagPath;
		int	i;
		int	instanceNum;

		path.extendToShape();

		instanceNum = 0;
		if (path.isInstanced())
			instanceNum = path.instanceNumber();

		MFnMesh fnMesh(path);
		MObjectArray sets;
		MObjectArray comps;
		status = fnMesh.getConnectedSetsAndMembers(instanceNum, sets, comps, true);
		if (!status) {
			MayaPrintError("MFnMesh::getConnectedSetsAndMembers failed (%s)", status.errorString().asChar());
		}

		for (i = 0; i < (int)sets.length(); i++) {
			MObject set = sets[i];
			MObject comp = comps[i];

			MFnSet fnSet(set, &status);
			if (status == MS::kFailure) {
				MayaPrintError("MFnSet constructor failed (%s)", status.errorString().asChar());
				continue;
			}

			// Make sure the set is a polygonal set.  If not, continue.
			MItMeshPolygon piter(path, comp, &status);
			if (status == MS::kFailure) {
				continue;
			}

			MObject shaderNode = FindShader(set);
			if (shaderNode == MObject::kNullObj) {
				continue;
			}

			MFnDependencyNode Shader(shaderNode);

			MPlug colorPlug = Shader.findPlug("color", &status);
			if (status == MS::kFailure) {
				MayaPrintError("material(%s) has no color channel", Shader.name().asChar());
				continue;
			}

			if (Shader.name().length() > 64) {
				MayaPrintError("Material name too long MAX(64): '%s'", Shader.name().asChar());
				return false;
			}

			mesh->material.append(Shader.name().asChar());
			return true;
		}
		return false;
	}


	core::StackString<60> getMeshDisplayName(const MString& fullname)
	{
		typedef core::StackString<60> NameType;
		core::FixedStack<NameType, 16> Stack;

		core::StringTokenizer<char> tokens(fullname.asChar(), fullname.asChar() + fullname.length(), '|');
		core::StringRange<char> range(nullptr, nullptr);

		while (tokens.ExtractToken(range))
		{
			Stack.push(NameType(range.GetStart(), range.GetEnd()));
		}

		// ok the name is 2nd one.
		// check we have 2 tho.
		if (Stack.size() > 1)
			Stack.pop();

		return Stack.top();
	}


	// ---------------------------------------------------


	MFnDagNode* GetParent(MFnDagNode *bone)
	{
		X_ASSERT_NOT_NULL(bone);

		MStatus		status;
		MObject		parentObject;

		parentObject = bone->parent(0, &status);
		if (!status && status.statusCode() == MStatus::kInvalidParameter) {
			return NULL;
		}

		while (!parentObject.hasFn(MFn::kTransform)) {
			MFnDagNode parentNode(parentObject, &status);
			if (!status) {
				return NULL;
			}

			parentObject = parentNode.parent(0, &status);
			if (!status && status.statusCode() == MStatus::kInvalidParameter) {
				return NULL;
			}
		}

		MFnDagNode *parentNode;

		parentNode = X_NEW(MFnDagNode, g_arena, "ParentDagNode")(parentObject, &status);
		if (!status) {
			X_DELETE(parentNode, g_arena);
			return NULL;
		}

		return parentNode;
	}

	// ---------------------------------------------------

	bool vertSortByWeights(const MayaVertex& a, const MayaVertex& b) {
		return (a.numWeights<b.numWeights);
	}

	unsigned int hashsingle(float f)
	{
		union {
			unsigned int ui;
			float fv;
		};
		fv = f;
		return ((ui & 0xfffff000) >> 12);
	}

	int64 Hash(float x, float y, float z)
	{
		int64 h1 = hashsingle(x);
		int64 h2 = hashsingle(y);
		int64 h3 = hashsingle(z);
		return (h1 << 40) | (h2 << 20) | h3;
	}

	int64 vertHash(const MayaVertex& vert)
	{
		return Hash(vert.pos[0], vert.pos[1], vert.pos[2]);
	}


	struct Hashcontainer
	{
		Hashcontainer() {
			pVert = nullptr;
			idx = 0;
		}

		MayaVertex* pVert;
		int idx;
	};


} // namespace

// ---------------------------------------------------

PotatoOptions::PotatoOptions()
{

}

void PotatoOptions::setcmdArgs(const MArgList &args)
{
	reset();

	uint idx;

	// allow for options to be parsed.
	idx = args.flagIndex("scale");
	if (idx != MArgList::kInvalidArgIndex) {
		double temp;
		if (!args.get(++idx, temp)) {
			MayaPrintWarning("failed to get scale flag");
		} else {
			scale_ = static_cast<float>(temp);
		}
	}

	idx = args.flagIndex("use_cm");
	if (idx != MArgList::kInvalidArgIndex) {
		bool useCm = false;
		if (!args.get(++idx, useCm)) {
			MayaPrintWarning("failed to get use_cm flag");
		}
		else {
			if (useCm) {
				unitOfMeasurement_ = CM;
			}
		}
	}

	idx = args.flagIndex("weight_thresh");
	if (idx != MArgList::kInvalidArgIndex) {
		double temp;
		if (!args.get(++idx, temp)) {
			MayaPrintWarning("failed to get weight_thresh flag");
		}
		else {
			jointThreshold_ = static_cast<float>(temp);
		}
	}

	idx = args.flagIndex("zero_origin");
	if (idx != MArgList::kInvalidArgIndex) {
		if (!args.get(++idx, zeroOrigin_)) {
			MayaPrintWarning("failed to get zero_origin flag");
		}
	}

	idx = args.flagIndex("dir");
	if (idx != MArgList::kInvalidArgIndex) {
		MString dir;
		if (!args.get(++idx, dir)) {
			MayaPrintWarning("failed to get dir flag");
		}
		else {
			filePath_.append(dir.asChar());
			filePath_.replaceSeprators();
			filePath_.ensureSlash();
		}
	}

	idx = args.flagIndex("force_bones");
	if (idx != MArgList::kInvalidArgIndex) {
		if (!args.get(++idx, forceBoneFilters_)) {
			MayaPrintWarning("failed to get force_bones flag");
		}
	}

	idx = args.flagIndex("progress");
	if (idx != MArgList::kInvalidArgIndex) {
		if (!args.get(++idx, progressCntl_)) {
			MayaPrintWarning("failed to get progress cntl flag");
		}
	}


	// use the scale to make it cm -> inches.
	// this applyies it post user scale.
	if (unitOfMeasurement_ == INCHES) {
		scale_ = scale_ * 0.393700787f;
	}
}

void PotatoOptions::reset(void)
{
	lodInfo_.clear();
	filePath_.clear();
	scale_ = 1.0f;
	jointThreshold_ = JOINT_WEIGHT_THRESHOLD;
	forceBoneFilters_.clear();
	progressCntl_.clear();
	exportMode_ = EXPORT_INPUT;
	unitOfMeasurement_ = INCHES;
}

// --------------------------------------------

MayaBone::MayaBone() : dagnode(nullptr)
{
	mayaNode.setOwner(this);
	exportNode.setOwner(this);

	keep = false;
	bindpos = Vec3f::zero();
	bindm33 = Matrix33f::identity();
}

MayaBone::MayaBone(const MayaBone& oth)
{
	name = oth.name;
	dagnode = oth.dagnode;
	index = oth.index;

	bindpos = oth.bindpos;
	bindm33 = oth.bindm33;

	mayaNode = oth.mayaNode;
	exportNode = oth.exportNode;

	keep = oth.keep;

	mayaNode.setOwner(this);
	exportNode.setOwner(this);
}

MayaBone& MayaBone::operator = (const MayaBone &oth)
{
	name = oth.name;
	dagnode = oth.dagnode;
	index = oth.index;

	bindpos = oth.bindpos;
	bindm33 = oth.bindm33;

	mayaNode = oth.mayaNode;
	exportNode = oth.exportNode;

	keep = oth.keep;

	mayaNode.setOwner(this);
	exportNode.setOwner(this);
	return *this;
}

// --------------------------------------------

MayaWeight::MayaWeight()
{
	bone = nullptr;
	weight = 0.f;
}

// --------------------------------------------

MayaVertex::MayaVertex() :
numWeights(0), 
startWeightIdx(0) 
{

}

// --------------------------------------------

MayaMesh::MayaMesh() :
	verts(g_arena),
	faces(g_arena),
	weights(g_arena)
{
}

MayaMesh::~MayaMesh()
{

}

void MayaMesh::clear(void)
{
	verts.clear();
	faces.clear();
	weights.clear();
}

void MayaMesh::merge(MayaMesh *mesh)
{
	uint i;
	uint numverts;
	uint numtris;
	uint numWeights;

	numverts = (int)verts.size();
	numtris = (int)faces.size();

	verts.resize(numverts + mesh->verts.size());
	faces.resize(numtris + mesh->faces.size());

	// merge verts
	for (i = 0; i < mesh->verts.size(); i++) {
		verts[numverts + i] = mesh->verts[i];
	}

	// merge triangles
	for (i = 0; i < mesh->faces.size(); i++) {
		faces[numtris + i][0] = mesh->faces[i][0] + numverts;
		faces[numtris + i][1] = mesh->faces[i][1] + numverts;
		faces[numtris + i][2] = mesh->faces[i][2] + numverts;
	}

	// merge weights
	numWeights = (int)weights.size();
	weights.resize(numWeights + mesh->weights.size());
	for (i = 0; i < mesh->weights.size(); i++) {
		weights[numWeights + i] = mesh->weights[i];
	}

}

void MayaMesh::shareVerts(void)
{
	//	PROFILE_MAYA_NAME("process verts");
	// I use the multimap with a little hash i made.
	// so that i can get a subset of any verts
	// that are potentialy equal and test with elipsons against them.
	// reduces the comparions down from O(N*N) O(N)(avg) 
	typedef std::multimap<uint64_t, Hashcontainer> hashType;
	hashType hash;

	// we want to get rid of duplicate verts.
	// so we just add the verts one by one, checking if they match.
	// O(T*(3* V))
	// then we update the triangles.
	uint i, x; // , k;
	core::Array<MayaVertex>	v(g_arena);

	size_t before = verts.size();

	v = verts;
	verts.clear();

	// sort the weights
	// a vertex has 1-4 weights.
	// so we need to sort the verts so that the weight groups are grouped.
	// we sort now so that the face index are correct.
	if (this->hasBinds) {
		std::sort(v.begin(), v.end(), vertSortByWeights);
	}

	size_t numEqual = 0;
	size_t numUnique = 0;

	for (i = 0; i < faces.size(); i++)
	{
		for (x = 0; x < 3; x++) // for each face.
		{
			const MayaVertex& vert = v[faces[i][x]];
			const uint64 vert_hash = vertHash(vert);

			// is it unique?
			std::pair <hashType::iterator, hashType::iterator> ret;
			hashType::iterator it;
			ret = hash.equal_range(vert_hash);

			bool equal = false;

			for (it = ret.first; it != ret.second; ++it)
			{
				const MayaVertex* vv = it->second.pVert;
				
				if (vert.numWeights != vv->numWeights)
					continue;

				if (vert.startWeightIdx != vv->startWeightIdx)
					continue;

				if (!vert.pos.compare(vv->pos, MERGE_VERTEX_EPSILON))
					continue; // not same

				if (!vert.uv.compare(vv->uv, MERGE_TEXCORDS_EPSILON))
					continue; // not same

				equal = true;
				break; // equal.
			}

			if (equal)
			{
				faces[i][x] = it->second.idx;

				numEqual++;
			}
			else
			{
				numUnique++;


				faces[i][x] = (int)verts.append(vert);
				if (vert.numWeights > 0)
					CompBinds[vert.numWeights - 1]++;

				Hashcontainer temp;
				temp.pVert = &verts[verts.size() - 1];
				temp.idx = faces[i][x];

				// add hash.
				hash.insert(hashType::value_type(vert_hash, temp));

			}
		}
	}

	MayaPrintMsg("num merged: %i -> unique: %i (%i,%i)", numEqual, numUnique, before, verts.size());
}


void MayaMesh::calBoundingbox()
{
	AABB aabb;

	for (core::Array<MayaVertex>::ConstIterator
		it = verts.begin(); it != verts.end(); ++it)
	{
		aabb.add(it->pos);
	}

	boundingBox = aabb;
}


// --------------------------------------------

MayaLOD::MayaLOD() :
	meshes_(g_arena)
{

}

MayaLOD::~MayaLOD()
{
	// clear meshes.
	for (uint i = 0; i < meshes_.size(); i++) {
		X_DELETE(meshes_[i], g_arena);
	}

	meshes_.clear();
}


void MayaLOD::calBoundingbox(void)
{
	for (core::Array<MayaMesh*>::ConstIterator
		it = meshes_.begin(); it != meshes_.end(); ++it)
	{
		(*it)->calBoundingbox(); // calculate this mesh BB

		// add it to the model BB.
		boundingBox.add((*it)->boundingBox);
	}
}


void MayaLOD::pruneBones(void)
{
	MayaMesh		*mesh;
	uint				i, j;

	for (i = 0; i < meshes_.size(); i++) {
		mesh = meshes_[i];
		for (j = 0; j < mesh->weights.size(); j++) {
			mesh->weights[j].bone->keep = true;
		}
	}
}

size_t MayaLOD::getSubDataSize(const Flags8<model::StreamType>& streams)
{
	size_t size = 0;
	size_t i;
	for (i = 0; i < meshes_.size(); i++)
	{
		const MayaMesh* mesh = meshes_[i];

		size += sizeof(model::Face) * mesh->faces.size();
		size += sizeof(model::Vertex) * mesh->verts.size();

		// streams.
		if (streams.IsSet(model::StreamType::COLOR))
			size += sizeof(model::VertexColor) * mesh->verts.size();
		if (streams.IsSet(model::StreamType::NORMALS))
			size += sizeof(model::VertexNormal) * mesh->verts.size();
		if (streams.IsSet(model::StreamType::TANGENT_BI))
			size += sizeof(model::VertexTangentBi) * mesh->verts.size();

		// bind data
		size += safe_static_cast<size_t, size_t>(mesh->CompBinds.dataSizeTotal());
	}
	return size;
}


MStatus MayaLOD::LoadMeshes(void)
{
	PROFILE_MAYA("load meshes");

	MStatus	status;
	MayaMesh* mesh;

	uint numMesh = exportObjects_.length();

	if (numMesh == 0) {
		MayaPrintError("LOD%i failed to load meshes for lod no meshes found.", lodIdx_);
		return MS::kFailure;
	}

	meshes_.reserve(numMesh);

	uint i; int x;
	float scale = g_options.scale_;
	float jointThreshold = g_options.jointThreshold_;

	MFloatPointArray	vertexArray;
	MFloatVectorArray	normalsArray, tangentsArray, binormalsArray;
	MStringArray		UVSets;
	MFloatArray			u, v;
	MPointArray			points;
	MIntArray			polygonVertices;
	MIntArray			vertexList;
	MColorArray			vertColorsArray;

	for (i = 0; i < numMesh; i++)
	{
		MFnMesh fnmesh(exportObjects_[i], &status);

		if (!status) {
				
			continue;
		}

		mesh = X_NEW(MayaMesh, g_arena, "Mesh");
		meshes_.append(mesh);

		mesh->displayName = getMeshDisplayName(fnmesh.fullPathName());
		mesh->name.append(fnmesh.name().asChar());
		if (!GetMeshMaterial(mesh, exportObjects_[i])) {
			MayaPrintError("Mesh(%s): failed to get material name", fnmesh.name().asChar());
			return MS::kFailure;
		}

		int numVerts = fnmesh.numVertices(&status);
		int numPoly = fnmesh.numPolygons(&status);
//		int numNormals = fnmesh.numNormals(&status);

		if (!status) {
			MayaPrintError("Mesh(%s): failed to get mesh info (%s)", fnmesh.name().asChar(), status.errorString().asChar());
			return status;
		}

		// resize baby.
		mesh->verts.resize(numVerts);
		mesh->faces.setGranularity(numPoly * 3);
		mesh->weights.setGranularity(2048 * 2);

		status = fnmesh.getUVSetNames(UVSets);

		fnmesh.getUVs(u, v, &UVSets[0]);
		fnmesh.getPoints(vertexArray, MSpace::kPreTransform);
		fnmesh.getNormals(normalsArray, MSpace::kPreTransform);
		fnmesh.getTangents(tangentsArray, MSpace::kPreTransform);
		fnmesh.getBinormals(binormalsArray, MSpace::kPreTransform);
		fnmesh.getVertexColors(vertColorsArray);

		// some times we don't have vert colors it seams so.
		// fill with white.
		MColor white;
		while (safe_static_cast<int,size_t>(vertColorsArray.length()) < numVerts)
		{
			vertColorsArray.append(white);
		}

		// verts
		for (x = 0; x < numVerts; x++) {
			MayaVertex& vert = mesh->verts[x];
			vert.pos = ConvertToGameSpace(XVec(vertexArray[x])) * scale;
			vert.normal = XVec(normalsArray[x]);
			vert.tangent = XVec(tangentsArray[x]);
			vert.binormal = XVec(binormalsArray[x]);
			vert.uv = Vec2f(u[x], v[x]);
			vert.col = XVec(vertColorsArray[x]);
		}

		// load the Faces and UV's
		MItMeshPolygon  itPolygon(exportObjects_[i], MObject::kNullObj);
		for (; !itPolygon.isDone(); itPolygon.next())
		{
			int numTriangles;

			itPolygon.numTriangles(numTriangles);
			itPolygon.getVertices(polygonVertices);

			while (numTriangles--)
			{
				status = itPolygon.getTriangle(numTriangles, points, vertexList, MSpace::kWorld);

				mesh->faces.append(Vec3<int>(vertexList[0], vertexList[1], vertexList[2]));
			}
		}


		// load weights
		{
			// clean.
			core::Array<MayaVertex>::Iterator it = mesh->verts.begin();
			for (; it != mesh->verts.end(); ++it) {
				it->numWeights = 0;
			}

			//search the skin cluster affecting this geometry
			MItDependencyNodes kDepNodeIt(MFn::kSkinClusterFilter);

			// Get any attached skin cluster
			bool hasSkinCluster = false;
			// Go through each skin cluster in the scene until we find the one connected to this mesh.
			for (; !kDepNodeIt.isDone() && !hasSkinCluster; kDepNodeIt.next())
			{
				MObject kObject = kDepNodeIt.item();
				MFnSkinCluster kSkinClusterFn(kObject, &status);
				if (status != MS::kSuccess)
					continue;

				unsigned int uiNumGeometries = kSkinClusterFn.numOutputConnections();

				// go through each connection on the skin cluster until we get the one connecting to this mesh.
				// pretty sure maya api dose not let me get this info without checking.
				// might be wrong.
				for (unsigned int uiGeometry = 0; uiGeometry < uiNumGeometries && !hasSkinCluster; ++uiGeometry)
				{
					unsigned int uiIndex = kSkinClusterFn.indexForOutputConnection(uiGeometry, &status);

					MObject kInputObject = kSkinClusterFn.inputShapeAtIndex(uiIndex, &status);
					MObject kOutputObject = kSkinClusterFn.outputShapeAtIndex(uiIndex, &status);

					if (kOutputObject == fnmesh.object())
					{
						hasSkinCluster = true;

						MDagPathArray infs;
						unsigned int nInfs = kSkinClusterFn.influenceObjects(infs, &status);

						if (!status) {
							MayaPrintError("Mesh '%s': Error getting influence objects (%s)", mesh->displayName.c_str(), status.errorString().asChar());
						}

						MDagPath		path = exportObjects_[i];
						core::Array<MayaBone*>	joints(g_arena);
						MayaBone				*joint;
						MayaVertex				*vert;
						MayaWeight				weight;
						uint32_t				numVertsTrimmed = 0;

						
						joints.reserve(nInfs);
						for (uint32_t kk = 0; kk < nInfs; ++kk) {
							const char *c;
							MString s;

							s = infs[kk].partialPathName();
							c = s.asChar();
							joint = pModel->findJointReal(c);
							if (!joint) {
								MayaPrintError("Mesh '%s': joint %s not found", mesh->displayName.c_str(), c);
							}

							joints.append(joint);
						}
						

						//	PROFILE_MAYA_NAME("load weights");

						MItGeometry kGeometryIt(kInputObject);
						for (; !kGeometryIt.isDone(); kGeometryIt.next())
						{
							MObject comp = kGeometryIt.component(&status);
							if (!status) {
								MayaPrintError("Mesh '%s': Error getting component (%s)", mesh->displayName.c_str(), status.errorString().asChar());
							}

							// Get the weights for this vertex (one per influence object)
							MFloatArray wts;
							unsigned infCount;
							status = kSkinClusterFn.getWeights(path, comp, wts, infCount);

							if (!status) {
								MayaPrintError("Mesh '%s': Error getting weights (%s)", mesh->displayName.c_str(), status.errorString().asChar());
							}
							if (0 == infCount) {
								MayaPrintError("Mesh '%s': Error: 0 influence objects.", mesh->displayName.c_str());
							}

							int num = kGeometryIt.index();
							vert = &mesh->verts[num];

							// the start index for the weights in 'mesh->weights'
							vert->startWeightIdx = (int32)mesh->weights.size();

							float totalweight = 0.0f;

							// copy the weight data for this vertex
							int numNonZeroWeights = 0;
							int k, numAdded = 0;
							for (k = 0; k < (int)infCount; ++k)
							{
								float w = (float)wts[k];
								if (w > 0.0f) {
									numNonZeroWeights++;
								}

								if (w > jointThreshold)
								{
									weight.bone = joints[k];
									weight.weight = wts[k];
									//	weight.joint->bindmat.ProjectVector(vert->pos - weight.joint->bindpos, weight.offset);

									if (numAdded >= VERTEX_MAX_WEIGHTS)
									{
										// you shit !
										// why did you bind so many god dam bones!
										// ok all we want to do is 
										// check if this bind has more weight than any of the others
										// if so replace it.
										// basically survival of the fattest.
										// make sure we replace the current lowest.
										numVertsTrimmed++;

										int w, smallest = -1;
										for (w = 0; w < numAdded; w++) {
											const float checkWeight = mesh->weights[vert->startWeightIdx + w].weight;
											if (checkWeight < weight.weight) {
												smallest = w; // save the index.
											}
										}

										// replace it.
										if (smallest != -1) {
											MayaWeight& temp = mesh->weights[vert->startWeightIdx + smallest];

											totalweight -= temp.weight;
											temp = weight;
											totalweight += temp.weight;
										}
										continue;
									}

									mesh->weights.append(weight);
									totalweight += weight.weight;
									numAdded++;
								}

							}

							// calculate total total vets TWAT!
							vert->numWeights = (int32)mesh->weights.size() - vert->startWeightIdx;

							if (vert->numWeights > VERTEX_MAX_WEIGHTS)
							{
								// what the actual FUCK.
								// these should of been removed !!
								// ps: this has never been triggerd, I just need to make sure 
								//		this post condition is true since the engine requires
								//		it to be so.
								MayaPrintError("failed to process vert weights correct. Source Code Defect."); // lol aka i suck.
							}
							if (vert->numWeights < 0) {

								MayaPrintError("negative bind count.");
							}

							if (!vert->numWeights) {
								if (numNonZeroWeights)
									MayaPrintError("Error on mesh '%s': Vertex %d doesn't have any joint weights exceeding weight Threshold(%f).", mesh->displayName.c_str(), num, g_options.jointThreshold_);
								else
									MayaPrintError("Error on mesh '%s': Vertex %d doesn't have any joint weights.", mesh->displayName.c_str(), num);
							}
							else if (!totalweight) {
								MayaPrintError("Error on mesh '%s': Combined weight of 0 on vertex %d.", mesh->displayName.c_str(), num);
							}


							// normalize the joint weights
							for (k = 0; k < vert->numWeights; k++) {
								mesh->weights[vert->startWeightIdx + k].weight /= totalweight;
							}

						} // for (; !kGeometryIt.isDone(); kGeometryIt.next())


						if (numVertsTrimmed > 0)
						{
							MayaPrintMsg("> Dropped %d skin weights from mesh: '%s' Max 4 per vert.", numVertsTrimmed, mesh->displayName.c_str());
							g_stats.totalWeightsDropped += numVertsTrimmed;
						}


					}
				}
			}

			mesh->hasBinds = hasSkinCluster;

			if (!hasSkinCluster) {
				MayaPrintWarning("No bindInfo found for mesh: '%s' binding to root.", mesh->displayName.c_str());
			}
		}
	}

	return status;
}

core::Thread::ReturnValue mergeVertsThreadFnc(const core::Thread& thread)
{
	core::Array<MayaMesh*>* meshes = (core::Array<MayaMesh*>*)thread.getData();

	for (uint i = 0; i <meshes->size(); i++)
		(*meshes)[i]->shareVerts();

	return 0;
}

void MayaLOD::MergeMeshes(void)
{
	int	numMerged;
	{
	//	PROFILE_MAYA("merge meshes");

		uint						i, j;

		MayaMesh				*mesh;
		MayaMesh				*combine;
		core::Array<MayaMesh*>	meshes(g_arena);

		meshes = meshes_;
		meshes_.clear();

		numMerged = 0;

		for (i = 0; i < meshes.size(); i++) {

			mesh = meshes[i];
			combine = nullptr;

			// check it again others.
			for (j = 0; j < meshes_.size(); j++) {
				if (meshes_[j]->material == mesh->material) {
					combine = meshes_[j];
					break;
				}
			}

			if (combine) {
				combine->merge(mesh);
				X_DELETE(mesh, g_arena);
				numMerged++;
			}
			else {
				meshes_.append(mesh);
			}
		}

		uint numMeshes = (int32)meshes_.size();

		const int numThreads = 2;
		if (numMeshes >= numThreads) // atleast 2 meshes, before we bother threading.
		{
			// thread it.
			core::Thread threads[numThreads];
			core::Array<MayaMesh*>	meshes[numThreads] = { g_arena, g_arena };

			uint numPerThread = numMeshes / numThreads;
			uint currentThread = 0;
			for (i = 0; i < numMeshes; i++) {
				if (i >= numPerThread && (currentThread < numThreads - 1)) {
					currentThread++;
					numPerThread += (numMeshes / numThreads);
				}
				meshes[currentThread].append(meshes_[i]);
			}

			for (i = 0; i < numThreads; i++) {
				threads[i].Create("merge worker"); // default stack size
				threads[i].setData(&meshes[i]);
			}

			for (i = 0; i < numThreads; i++)
				threads[i].Start(mergeVertsThreadFnc);

			for (i = 0; i < numThreads; i++)
				threads[i].Join();
		}
		else
		{
			for (i = 0; i <numMeshes; i++)
				meshes_[i]->shareVerts();
		}
	}

	if (numMerged > 0)
		MayaPrintMsg("(%i) meshes merged", numMerged);
}



// --------------------------------------------

MayaModel::MayaModel() :
	bones_(g_arena)
{
	for (uint i = 0; i < 4; i++)
		lods_[i].setModel(this,i);

	g_stats.clear();
	numExportJoints_ = 0;


	tagOrigin_.index = 0;
	tagOrigin_.name.append("tag_origin");
	tagOrigin_.dagnode = nullptr;
	tagOrigin_.keep = false;
}

MayaModel::~MayaModel()
{
	for (uint i = 0; i < bones_.size(); i++) {
		X_DELETE(bones_[i].dagnode, g_arena);
	}

	bones_.clear();
}


MayaBone *MayaModel::findJointReal(const char *name) {
	MayaBone	*bone;
	uint			i;

	bone = bones_.ptr();
	for (i = 0; i < bones_.size(); i++, bone++) {
		if (bone->name.isEqual(name)) {
			return bone;
		}
	}
	return nullptr;
}



void MayaModel::getBindPose(const MObject &jointNode, MayaBone *bone, float scale) {
	MStatus				status;
	MFnDependencyNode	fnJoint(jointNode);
	MObject				aBindPose = fnJoint.attribute("bindPose", &status);

	bone->bindpos = Vec3f::zero();
	bone->bindm33 = Matrix33f::identity();

	if (MS::kSuccess == status) {
		unsigned	ii;
		unsigned	jointIndex;
		unsigned	connLength;
		MPlugArray	connPlugs;
		MPlug		pBindPose(jointNode, aBindPose);

		pBindPose.connectedTo(connPlugs, false, true);
		connLength = connPlugs.length();
		for (ii = 0; ii < connLength; ++ii) {
			if (connPlugs[ii].node().apiType() == MFn::kDagPose) {
				MObject			aMember = connPlugs[ii].attribute();
				MFnAttribute	fnAttr(aMember);

				if (fnAttr.name() == "worldMatrix") {
					jointIndex = connPlugs[ii].logicalIndex();

					MFnDependencyNode nDagPose(connPlugs[ii].node());

					// construct plugs for this joint's world matrix
					MObject aWorldMatrix = nDagPose.attribute("worldMatrix");
					MPlug	pWorldMatrix(connPlugs[ii].node(), aWorldMatrix);

					pWorldMatrix.selectAncestorLogicalIndex(jointIndex, aWorldMatrix);

					// get the world matrix data
					MObject worldMatrix;
					MStatus status = pWorldMatrix.getValue(worldMatrix);
					if (MS::kSuccess != status) {
						// Problem retrieving world matrix
						return;
					}

					MFnMatrixData	dMatrix(worldMatrix);
					MMatrix			wMatrix = dMatrix.matrix(&status);

					bone->bindm33 = ConvertToGameSpace(XMat(wMatrix));
					bone->bindpos = ConvertToGameSpace(XVec(wMatrix)) * scale;
						
					//if (!options.ignoreScale) {
					//	joint->bindpos *= joint->scale;
					//}

					return;
				}
			}
		}
	}
}


MStatus MayaModel::lodLODs(void)
{
	PROFILE_MAYA("load LODS");

	MStatus status;
	for (uint i = 0; i < g_options.numLods(); i++)
	{
		status = lods_[i].LoadMeshes();
		if (!status)
			break;
	}
	return status;
}

MStatus MayaModel::loadBones(void)
{
	PROFILE_MAYA("load joints");

	MStatus			status;
	MDagPath		dagPath;
	MFnDagNode		*parentNode;
	MayaBone		*bone;
	uint				i, j;

	float scale = g_options.scale_;


	// allocate max.
	bones_.reserve(model::MODEL_MAX_BONES);

	MItDag dagIterator(MItDag::kDepthFirst, MFn::kTransform, &status);
	for (; !dagIterator.isDone(); dagIterator.next()) {
		status = dagIterator.getPath(dagPath);
		if (!status) {
			MayaPrintError("Joint: MItDag::getPath failed (%s)", status.errorString().asChar());
			return status;
		}

		if (!dagPath.hasFn(MFn::kJoint))
			continue;
		
		MayaBone new_bone;
		new_bone.dagnode = X_NEW(MFnDagNode,g_arena, "BoneDagNode")(dagPath, &status);

		if (!status) {
			X_DELETE(new_bone.dagnode, g_arena);// dunno if maya returns null, don't matter tho.
			MayaPrintError("Joint: MFnDagNode constructor failed (%s)", status.errorString().asChar());
			return status;
		}

		new_bone.name.append(new_bone.dagnode->name().asChar());

		if (bones_.size() == model::MODEL_MAX_BONES) {
			MayaPrintError("Joints: max bones reached: %i", model::MODEL_MAX_BONES);
			return MStatus::kFailure;
		}

		if (new_bone.name.length() > model::MODEL_MAX_BONE_NAME_LENGTH)
		{
			MayaPrintError("Joints: max bone name length exceeded, MAX: %i '%' -> %i", 
				model::MODEL_MAX_BONE_NAME_LENGTH, new_bone.name.c_str(), new_bone.name.length());
			return MStatus::kFailure;
		}

		size_t idx = bones_.append(new_bone);

		bone = &bones_[idx];
		bone->index = safe_static_cast<int,size_t>((bones_.size() - 1));
	}


	// create hierarchy
	bone = bones_.ptr();
	for (i = 0; i < bones_.size(); i++, bone++) {
		if (!bone->dagnode) 
			continue;
		
		bone->mayaNode.setParent(mayaHead);
		bone->exportNode.setParent(exportHead);

		parentNode = GetParent(bone->dagnode);
		if (parentNode) {

			// do we have this joint?
			for (j = 0; j < bones_.size(); j++) {
				if (!bones_[j].dagnode) 
					continue;
				
				if (bones_[j].dagnode->name() == parentNode->name()) {
					bone->mayaNode.setParent(bones_[j].mayaNode);
					bone->exportNode.setParent(bones_[j].exportNode);
					break;
				}
			}
			X_DELETE(parentNode, g_arena);
		}

		bone->dagnode->getPath(dagPath);
		getBindPose(dagPath.node(&status), bone, scale);
	}
	
	uint32_t index = 0;
	for (bone = exportHead.next(); bone != nullptr; bone = bone->exportNode.next()) {
		bone->exportIdx = index++; 
	}

	return status;
}


void MayaModel::pruneBones(void)
{
	PROFILE_MAYA("process bones");

//	MayaMesh		*mesh;
	MayaBone		*bone;
	MayaBone		*parent;
	uint				i;

	for (i = 0; i < g_options.numLods(); i++)
		lods_[i].pruneBones();

	numExportJoints_ = 0;
	bone = bones_.ptr();
	for (i = 0; i < bones_.size(); i++, bone++) 
	{
		if (!bone->keep) 
		{
			bone->exportNode.removeFromHierarchy();
			g_stats.droppedBoneNames.append(bone->name);
		}
		else 
		{
			numExportJoints_++;

			// make sure we are parented to an exported joint
			for (parent = bone->exportNode.parent(); parent != nullptr; parent = parent->exportNode.parent()) {
				if (parent->keep) {
					break;
				}
			}

			if (parent != nullptr) {
				bone->exportNode.setParent(parent->exportNode);
			}
			else {
				bone->exportNode.setParent(exportHead);
			}
		}
	}

	if (numExportJoints_ == 0) {
		tagOrigin_.keep = true;
	//	tagOrigin_.exportNode.setParent(exportHead);
	}

	g_stats.totalJointsDropped = (int32_t)bones_.size() - numExportJoints_;
}

void MayaModel::MergeMeshes()
{
	PROFILE_MAYA("merge meshes");
	for (int i = 0; i < 4; i++)
		lods_[i].MergeMeshes();
}

void MayaModel::printStats(PotatoOptions& options)
{
	X_UNUSED(options);

	std::cout << "\nModel Info:\n" <<
		"> Total Lods: " << g_stats.totalLods <<
		"\n> Total Mesh: " << g_stats.totalMesh <<
		"\n> Total Joints: " << g_stats.totalJoints <<
		"\n> Total Joints Dropped: " << g_stats.totalJointsDropped;
		
	if (g_stats.droppedBoneNames.size() > 0) {
		std::cout << " -> (";
		for (uint i = 0; i < g_stats.droppedBoneNames.size(); i++) {
			std::cout << g_stats.droppedBoneNames[i].c_str();
			if (i < (g_stats.droppedBoneNames.size() - 1))
				std::cout << ", ";

			if (i > 9 && (i % 10) == 0)
				std::cout << "\n";
		}
		std::cout << ")";
	}
	if (g_stats.droppedBoneNames.size() > 10)
		std::cout << "\n";

	std::cout <<
		"\n> Total Verts: " << g_stats.totalVerts <<
		"\n> Total Faces: " << g_stats.totalFaces <<
		"\n> Total Weights Dropped: " << g_stats.totalWeightsDropped <<
		std::endl;

	if (g_stats.totalWeightsDropped > 0) {
		std::cout << "!> bind weights where dropped, consider binding with max influences: 4\n";
	}

	{
		const AABB& b = g_stats.bounds;
		const auto min = b.min;
		const auto max = b.max;
		std::cout << "> Bounds: ";
		std::cout << "(" << min[0] << "," << min[1] << "," << min[2] << ") <-> ";
		std::cout << "(" << max[0] << "," << max[1] << "," << max[2] << ")\n";

		const auto size = b.size();
		std::cout << "> Dimensions: ";
		std::cout << "w: " << size[0] << " d: " << size[1] << " h: " << size[2];

		if (g_options.unitOfMeasurement_ == PotatoOptions::INCHES) {
			std::cout << " (inches)";
		}
		else {
			std::cout << " (cm)";
		}
	}
}

void MayaModel::calculateBoundingBox(void)
{
	PROFILE_MAYA("bounding box");

	for (int i = 0; i < 4; i++)
	{
		lods_[i].calBoundingbox();

		// add it to the model BB.
		boundingBox.add(lods_[i].boundingBox);
	}
}


uint32_t MayaModel::calculateTagNameDataSize(void)
{
	size_t size = 0;
	MayaBone* bone;

	for (bone = exportHead.next(); bone != nullptr; bone = bone->exportNode.next())
	{
		size += bone->name.length() + 1;
	}

	return safe_static_cast<uint32_t, size_t>(size);
}

uint32_t MayaModel::calculateMaterialNameDataSize(void)
{
	size_t size = 0;
	size_t i, x;

	for (i = 0; i < 4; i++) 
	{
		for (x = 0; x < lods_[i].meshes_.size(); x++)
		{
			size += lods_[i].meshes_[x]->material.length();
			size++; // null-term.
		}
	}

	return safe_static_cast<uint32_t, size_t>(size);
}

uint32_t MayaModel::calculateSubDataSize(const Flags8<model::StreamType>& streams)
{
	// gonna just calculate it.
	// saves a tell and a seek on the file.
	int i;
	uint32_t size = this->calculateBoneDataSize();

	// size of all the mesh headers.
	size += sizeof(model::SubMeshHeader) * (uint32_t)totalMeshes();

	for (i = 0; i < 4; i++)
	{
		// for each lod we have Vert, Face, bind
		size += (uint32_t)lods_[i].getSubDataSize(streams);
	}

	return size;
}


uint32_t MayaModel::calculateBoneDataSize(void)
{
	uint32_t size = 0;
	uint32_t totalbones = numExportJoints_ + (tagOrigin_.keep ? 1 : 0);

	// don't store pos,angle,hier for blank bones currently.
	size += (numExportJoints_ * sizeof(uint8_t)); // hierarchy
	size += (numExportJoints_ * sizeof(XQuatCompressedf)); // angle
	size += (numExportJoints_ * sizeof(Vec3f));	// pos.
	size += (totalbones * sizeof(uint16_t));	// string table idx's

	return size;
}


bool MayaModel::save(const char *filename)
{
	PROFILE_MAYA("save");

	FILE* f;
	MayaBone* bone;
	size_t i, x, k, j;
	size_t numMesh;
	size_t meshHeadOffsets = sizeof(model::ModelHeader);
	size_t numLods = g_options.numLods();

	// place file data in a stream.
	// much faster(benchmarked), since we are doing lots of small writes.
	core::ByteStream stream(g_arena);

	errno_t err = fopen_s(&f, filename, "wb");
	if (f)
	{
		model::ModelHeader header;
		core::zero_object(header);

		g_stats.totalJoints = numExportJoints_;

		Flags8<model::StreamType> streamsFlags;
		streamsFlags.Set(model::StreamType::COLOR);
		streamsFlags.Set(model::StreamType::NORMALS);
		streamsFlags.Set(model::StreamType::TANGENT_BI);

		header.version = model::MODEL_VERSION;
		header.flags.Set(model::ModelFlags::LOOSE);
		header.flags.Set(model::ModelFlags::STREAMS);
		header.numBones = safe_static_cast<uint8_t, int>(numExportJoints_);
		header.numBlankBones = tagOrigin_.keep ? 1:0;
		header.numLod = safe_static_cast<uint8_t, size_t>(numLods);
		header.numMesh = safe_static_cast<uint8_t, size_t>(totalMeshes());
		header.modified = core::dateTimeStampSmall::systemDateTime();

		// Sizes
		header.tagNameDataSize = safe_static_cast<uint16_t, uint32_t>(this->calculateTagNameDataSize());
		header.materialNameDataSize = safe_static_cast<uint16_t, uint32_t>(this->calculateMaterialNameDataSize());
		header.boneDataSize = safe_static_cast<uint16_t, uint32_t>(this->calculateBoneDataSize());
		header.subDataSize = this->calculateSubDataSize(streamsFlags);
		header.dataSize = (header.subDataSize + 
			header.tagNameDataSize + header.materialNameDataSize);


		for (bone = exportHead.next(); bone != nullptr; bone = bone->exportNode.next())
			meshHeadOffsets += bone->getDataSize();
		

		for (i = 0; i < numLods; i++)
		{
			model::LODHeader& lod = header.lodInfo[i];
			lod.lodDistance = g_options.lodInfo_[i].distance;
			lod.numSubMeshes = safe_static_cast<uint16_t,size_t>(lods_[i].numMeshes());
			// we want to know the offset o.o
			lod.subMeshHeads = meshHeadOffsets;

			// version 5.0 info
			lod.numVerts = lods_[i].totalVerts();
			lod.numIndexes = lods_[i].totalIndexs();

			// Version 8.0 info
			lod.streamsFlag = streamsFlags;

			// work out bounds for all meshes.
			lod.boundingBox.clear();
			for (x = 0; x < lods_[i].meshes_.size(); x++)
			{
				const MayaMesh* pMesh = lods_[i].meshes_[x];

				lod.boundingBox.add(pMesh->boundingBox);
			}
			// create sphere.
			lod.boundingSphere = Sphere(lod.boundingBox);


			meshHeadOffsets += lod.numSubMeshes * sizeof(model::MeshHeader);
		}

		// create combined bounding box.
		header.boundingBox.clear();
		for (i = 0; i < numLods; i++)
		{
			model::LODHeader& lod = header.lodInfo[i];

			header.boundingBox.add(lod.boundingBox);
		}

		// update bounds in stats.
		g_stats.bounds = header.boundingBox;

	//	const size_t temp = sizeof(header);

		fwrite(&header, sizeof(header), 1, f);

		// material names( ALL LOD)
		{
			for (i = 0; i < numLods; i++)
			{
				// meshes 
				core::Array<MayaMesh*>*	meshes = &lods_[i].meshes_;
				for (x = 0; x < meshes->size(); x++)
				{
					MayaMesh* pMesh = (*meshes)[x];

#if defined(X_MODEL_MTL_LOWER_CASE_NAMES) && X_MODEL_MTL_LOWER_CASE_NAMES
					pMesh->material.toLower();
#endif // ~X_MODEL_MTL_LOWER_CASE_NAMES

					fwrite(pMesh->material.c_str(), pMesh->material.length() + 1, 1, f);
				}
			}

		}

		// bone data.
		{
			// TAG NAMES
			for (bone = exportHead.next(); bone != nullptr; bone = bone->exportNode.next()) 
			{
#if defined(X_MODEL_BONES_LOWER_CASE_NAMES) && X_MODEL_BONES_LOWER_CASE_NAMES
				bone->name.toLower();
#endif // ~X_MODEL_BONES_LOWER_CASE_NAMES
				fwrite(bone->name.begin(), 1, bone->name.length() + 1, f);
			}

			// space for name index data.
			uint16_t blankData[255] = { 0 };
			fwrite(blankData, 2, header.numBones + header.numBlankBones, f);


			// hierarchy
			for (bone = exportHead.next(); bone != nullptr; bone = bone->exportNode.next()) 
			{
				MayaBone* parent = bone->exportNode.parent();

				uint8_t idx = 0;
				if (parent)
					idx = safe_static_cast<uint8_t,uint32_t>(parent->exportIdx);

				fwrite(&idx, sizeof(idx), 1, f);
			}

			// angles.
			for (bone = exportHead.next(); bone != nullptr; bone = bone->exportNode.next()) 
			{
				// compress me baby.
				XQuatCompressedf quat(bone->bindm33);

				fwrite(&quat, sizeof(quat), 1, f);
			}

			// pos.
			for (bone = exportHead.next(); bone != nullptr; bone = bone->exportNode.next()) 
			{
				fwrite(&bone->bindpos, sizeof(bone->bindpos), 1, f);
			}
		}

		// write mesh headers for each lod.
		for (i = 0; i < numLods; i++)
		{
			// meshes 
			core::Array<MayaMesh*>*	meshes = &lods_[i].meshes_;

			if (meshes->size())
			{
				g_stats.totalLods++;

				uint32_t vertOffset, indexOffset;

				vertOffset = 0;
				indexOffset = 0;

				for (x = 0; x < meshes->size(); x++)
				{
					model::SubMeshHeader mesh;
					core::zero_object(mesh);

					MayaMesh* pMesh = (*meshes)[x];

					mesh.numBinds = 0;
					mesh.numVerts = safe_static_cast<uint16_t, size_t>(pMesh->verts.size());
					mesh.numIndexes = safe_static_cast<uint16_t, size_t>(pMesh->faces.size() * 3);
			//		mesh.material = pMesh->material;
					mesh.CompBinds = pMesh->CompBinds;
					mesh.boundingBox = pMesh->boundingBox;
					mesh.boundingSphere = Sphere(pMesh->boundingBox);

					// Version 5.0 info
					mesh.startVertex = vertOffset;
					mesh.startIndex = indexOffset;

					// Version 8.0 info
					mesh.streamsFlag = streamsFlags; // currently a 3d model has identical flags for all meshes.
					mesh.boundingBox = pMesh->boundingBox;
					mesh.boundingSphere = Sphere(pMesh->boundingBox);

					// inc the offsets
					vertOffset += mesh.numVerts;
					indexOffset += mesh.numIndexes;

					g_stats.totalMesh++;
					g_stats.totalVerts += mesh.numVerts;
					g_stats.totalFaces += mesh.numIndexes;

					fwrite(&mesh, sizeof(mesh), 1, f);
				}
			}
		}

		// now we write Vert + faces + bindData
		// we convert them now to the real format.
		// as processing the data in the commpressed state is slower.
		// core::Array<model::Vertex>	verts;

		for (j= 0; j < numLods; j++)
		{
			numMesh = lods_[j].numMeshes();

			// work out a total steam size.
			size_t requiredStreamSize = 0;
			for (i = 0; i < numMesh; i++)
			{
				MayaMesh* mesh = lods_[j].meshes_[i];
				
				
				requiredStreamSize += mesh->CompBinds.dataSizeTotal();
				requiredStreamSize += (mesh->faces.size() * sizeof(model::Face));
				requiredStreamSize += (mesh->verts.size() * sizeof(model::Vertex));

				if (streamsFlags.IsSet(model::StreamType::COLOR))
				{
					requiredStreamSize += (mesh->verts.size() * sizeof(model::VertexColor));
				}
				if (streamsFlags.IsSet(model::StreamType::NORMALS))
				{
					requiredStreamSize += (mesh->verts.size() * sizeof(model::VertexNormal));
				}
				if (streamsFlags.IsSet(model::StreamType::TANGENT_BI))
				{
					requiredStreamSize += (mesh->verts.size() * sizeof(model::VertexTangentBi));
				}
			}

			// writing this info to a stream makes write time 5x times faster.
			stream.resize(requiredStreamSize);
			stream.reset();

			// write all the verts.
			for (i = 0; i < numMesh; i++)
			{
				MayaMesh* mesh = lods_[j].meshes_[i];

				model::Vertex vert;
				core::zero_object(vert);

				for (x = 0; x < mesh->verts.size(); x++)
				{
					const MayaVertex Mvert = mesh->verts[x];

					vert.pos = Mvert.pos;
					vert.st[0] = XHalfCompressor::compress(Mvert.uv[0]);
					vert.st[1] = XHalfCompressor::compress(Mvert.uv[1]);

					stream.write(vert);
				}
			}


			// write all the colors.
			if (streamsFlags.IsSet(model::StreamType::COLOR))
			{
				for (i = 0; i < numMesh; i++)
				{
					MayaMesh* mesh = lods_[j].meshes_[i];

					model::VertexColor col;
					col.set(0xFF, 0xFF, 0xFF, 0xFF);

					if (g_options.whiteVertColors_)
					{
						for (x = 0; x < mesh->verts.size(); x++)
						{
							stream.write(col);
						}
					}
					else
					{
						for (x = 0; x < mesh->verts.size(); x++)
						{
							const MayaVertex Mvert = mesh->verts[x];
							col = Mvert.col;
							stream.write(col);
						}
					}
				}
			}

			// write normals
			if (streamsFlags.IsSet(model::StreamType::NORMALS))
			{
				for (i = 0; i < numMesh; i++)
				{
					MayaMesh* mesh = lods_[j].meshes_[i];

					model::VertexNormal normal;

					for (x = 0; x < mesh->verts.size(); x++)
					{
						const MayaVertex Mvert = mesh->verts[x];
						normal = Mvert.normal;
						stream.write(normal);
					}
				}
			}

			// write tangents and bi-normals
			if (streamsFlags.IsSet(model::StreamType::TANGENT_BI))
			{
				for (i = 0; i < numMesh; i++)
				{
					MayaMesh* mesh = lods_[j].meshes_[i];

					model::VertexTangentBi tangent;

					for (x = 0; x < mesh->verts.size(); x++)
					{
						const MayaVertex Mvert = mesh->verts[x];

						tangent.binormal = Mvert.binormal;
						tangent.tangent = Mvert.tangent;

						stream.write(tangent);
					}
				}
			}


			// write all the faces
			for (i = 0; i < numMesh; i++)
			{
				MayaMesh* mesh = lods_[j].meshes_[i];
		
				for (x = 0; x < mesh->faces.size(); x++)
				{
					const Vec3<int32_t>& f = mesh->faces[x];

					stream.write<model::Index>(safe_static_cast<model::Index, int32_t>(f[0]));
					stream.write<model::Index>(safe_static_cast<model::Index, int32_t>(f[1]));
					stream.write<model::Index>(safe_static_cast<model::Index, int32_t>(f[2]));
				}

			}

			// write all the bind info.
			for (i = 0; i < numMesh; i++)
			{
				MayaMesh* mesh = lods_[j].meshes_[i];

				if (mesh->CompBinds.dataSizeTotal())
				{
					for (x = 0; x < mesh->verts.size(); x++)
					{
						const MayaVertex Mvert = mesh->verts[x];
						const MayaWeight* weights = &mesh->weights[Mvert.startWeightIdx];

						const uint num = Mvert.numWeights;

						if (num >= 1) // we always write one bone.
						{
							model::bindBone bone(safe_static_cast<uint16_t, uint32_t>(weights->bone->exportIdx));
							stream.write(bone);
						}


						for (k = 1; k < num; k++) // for any weights greater than 1 we add a bone & weight, the base weight is 1 - (all others)
						{
							model::bindBone bone(safe_static_cast<uint16_t, uint32_t>(weights[k].bone->exportIdx));
							model::bindWeight weight(weights[k].weight);

							stream.write(bone);
							stream.write(weight);
						}
					}

				}
			}

			// Write the complete LOD's data all at once.
			fwrite(stream.begin(), 1, stream.size(), f);
		}

		fclose(f);
		return true;
	}
	else
	{
		MayaPrintError("Failed to open file for saving(%i): %s", err, filename);
	}
	return false;
}

// --------------------------------------------

bool PotatoExporter::s_progressActive = false;

PotatoExporter::PotatoExporter()
{
	MayaPrintMsg("=========== Exporting Model ===========");
}

PotatoExporter::~PotatoExporter()
{
	HideProgressDlg();
	MayaPrintMsg("================= End =================");
}


MStatus PotatoExporter::getInputObjects(void)
{
	// we just want to load all objects for every lod.
	uint i, x;
	MStatus status = MS::kSuccess;

	for (i = 0; i < (uint)g_options.numLods(); i++)
	{
		MDagPathArray pathArry;
		MSelectionList list;
		LODExportInfo& info = g_options.lodInfo_[i];
		
		MStringArray nameArr;
		info.objects.split(' ', nameArr);

		for (x = 0; x < nameArr.length(); x++) {
			list.add(nameArr[x]);
		}

		for (x = 0; x < list.length(); x++)
		{
			MDagPath path;
			status = list.getDagPath(x, path);
			if (status)
				pathArry.append(path);
			else 
				MayaPrintError("getDagPath failed: %s", status.errorString().asChar());
		}
		

		if (pathArry.length() < 1)
			return MS::kFailure;

	//	int goat = pathArry.length();
	//	MayaPrintMsg("TEST: %s", info.objects.asChar());
	//	MayaPrintMsg("LOD%i object count: %i", i, pathArry.length());

		model_.lods_[i].exportObjects_ = pathArry;
	}

	return MS::kSuccess;
}

MStatus PotatoExporter::getExportObjects(void)
{
	PROFILE_MAYA("get objects");
	if (g_options.exportMode_ == PotatoOptions::EXPORT_INPUT){
		return getInputObjects();
	} else {
	//	if (g_options.exportMode_ == PotatoOptions::EXPORT_SELECTED)
	//		return getSelectedObjects();
	//	return getAllObjects();
	}

	return MS::kFailure;
}


MStatus PotatoExporter::convert()
{
	MStatus status = ShowProgressDlg();
	bool saveOk = false;

	{
		float appliedScale = g_options.scale_;
		float scale = g_options.scale_;

		if (g_options.unitOfMeasurement_ == PotatoOptions::INCHES) {
			scale = appliedScale * 2.54f;
		}

		MayaPrintMsg("Exporting to: '%s'", g_options.filePath_.c_str());
		MayaPrintMsg("Scale: '%f' Applied: '%f'", scale, appliedScale);
		MayaPrintMsg(""); // new line
	}


	// name length check
	if (strlen(g_options.filePath_.fileName()) > model::MODEL_MAX_NAME_LENGTH)
	{
		MayaPrintError("Model name is too long. MAX: %i, provided: %i",
			model::MODEL_MAX_NAME_LENGTH, g_options.filePath_.length());
		return MS::kFailure;;
	}

	{
		PROFILE_MAYA_NAME("Total Export time:");

		if (!status) {
			MayaPrintError("Failed to create progress window: %s", status.errorString().asChar());
			return status;
		}

		SetProgressText("Loading export list");

		status = getExportObjects();

		if (!status) {
			MayaPrintError("Failed to collect export objects from maya: %s", status.errorString().asChar());
			return status;
		}


		SetProgressText("Loading joints");
		status = model_.loadBones();
		if (!status) {
			MayaPrintError("Failed to load joints: %s", status.errorString().asChar());
			return status;
		}

		SetProgressText("Loading mesh data");

		// load them baby
		status = model_.lodLODs();

		if (!status) {
			MayaPrintError("Failed to load meshes: %s", status.errorString().asChar());
			return status;
		}

		SetProgressText("Processing joints");

		model_.pruneBones();

		SetProgressText("Processing meshes");

		// merge any meshes that share materials.
		// MergeMeshes();
		model_.MergeMeshes();

		SetProgressText("Creating info");


		// bounding boxes.
		model_.calculateBoundingBox();

		SetProgressText("Writing file");

		// write the file.
		saveOk = model_.save(g_options.filePath_.c_str());

	}

	if (saveOk) {
		status.setSuccess();
		model_.printStats(g_options);
	}
	else {
		MayaPrintError("Failed to write file");
		return MS::kFailure;
	}
	
//	SetProgressText("Finished"); // rekt
	return status;
}


MStatus PotatoExporter::ShowProgressDlg()
{
	using namespace std;
	if (g_options.progressCntl_.length() > 0) {
		MGlobal::executeCommand("progressBar -e -pr 0 " + g_options.progressCntl_);
	}
	else if (!s_progressActive) {
		MString title = X_ENGINE_NAME" Engine - Saving Model";
		MString process = "Starting...                                            ";

		int amount = 0;
		int maxProgress = 8;

		if (!MProgressWindow::reserve()) {
			MGlobal::displayError("Progress window already in use.");
			return MS::kFailure;
		}

		CHECK_MSTATUS_AND_RETURN_IT(MProgressWindow::setProgressRange(amount, maxProgress))
		CHECK_MSTATUS_AND_RETURN_IT(MProgressWindow::setTitle(title));
		CHECK_MSTATUS_AND_RETURN_IT(MProgressWindow::setInterruptable(false));
		CHECK_MSTATUS_AND_RETURN_IT(MProgressWindow::setProgress(amount));
		CHECK_MSTATUS_AND_RETURN_IT(MProgressWindow::setProgressStatus(process));
		CHECK_MSTATUS_AND_RETURN_IT(MProgressWindow::startProgress());
	
		CHECK_MSTATUS(MProgressWindow::advanceProgress(1));


		s_progressActive = true;
	}
	return MS::kSuccess;
}


MStatus PotatoExporter::HideProgressDlg()
{
	using namespace std;
	if (s_progressActive) {
		s_progressActive = false;
		CHECK_MSTATUS_AND_RETURN_IT(MProgressWindow::endProgress());
	}

	return MS::kSuccess;
}

void PotatoExporter::SetProgressText(MString str)
{
	g_StartOfBlock = true;

	if (g_options.progressCntl_.length() > 0) {
		MGlobal::executeCommand("progressBar -e -s 1 " + g_options.progressCntl_);
	} else {
		MProgressWindow::setProgressStatus(str);
		MProgressWindow::advanceProgress(1);
	}
	std::cout << str.asChar() << ":";
}


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>|

ModelExporter::ModelExporter()
{
}

ModelExporter::~ModelExporter()
{
}

MStatus ModelExporter::writer(const MFileObject& file, const MString& optionsString, FileAccessMode mode)
{
	X_UNUSED(file);
	X_UNUSED(optionsString);
	X_UNUSED(mode);

//	PotatoOptions options;
//	options.setcmdArgs(optionsString.asChar());
//	options.setMode(mode);
//	options.setFileName(file.name());

//	PotatoExporter exportModel;
//	return exportModel.convert();
	return MS::kFailure;
}

bool ModelExporter::haveWriteMethod() const
{
	return true;
}

MString ModelExporter::defaultExtension() const
{
	return "model";
}

void* ModelExporter::creator()
{
	return new ModelExporter;
}

// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>|

ModelExporterCmd::ModelExporterCmd()
{
}

ModelExporterCmd::~ModelExporterCmd()
{
}

MStatus ModelExporterCmd::doIt(const MArgList &args)
{
	MStatus status = MS::kFailure;
	MString Mfilename;
	uint currentLod = 0;
	uint idx;

	g_options.reset();
	g_stats.clear();


//	for (idx = 0; idx < args.length(); idx++) {
//		MayaPrintMsg("arg(%i): %s", idx, args.asString(idx));
//	}


	idx = args.flagIndex("f");
	if (idx == MArgList::kInvalidArgIndex) {
		MayaPrintError("missing file argument");
		return MS::kFailure;
	}

	status = args.get(++idx, Mfilename);
	if (!status) {
		MayaPrintError("missing filename");
		return status;
	}

//	MayaPrintMsg("filename: %s", Mfilename.asChar());

	// make sure we have correct extension.
	g_options.setcmdArgs(args);
	g_options.setFileName(Mfilename.asChar());

	// get the LOD options.
	for (uint i = 0; i < args.length(); i++)
	{
		core::StackString<32> lodName;
		lodName.appendFmt("LOD%i", i);
		idx = args.flagIndex(lodName.c_str());
		if (idx != MArgList::kInvalidArgIndex) 
		{
			double temp;
			LODExportInfo info;
			info.idx = currentLod;
			if (!args.get(++idx, temp) ||
				!args.get(++idx, info.objects)) {
				MayaPrintError("failed to parse LOD%i info", currentLod);
				continue;
			}
			if (g_options.numLods() == model::MODEL_MAX_LODS) {
				MayaPrintError("Exceeded max load count: %i", model::MODEL_MAX_LODS);
				return MS::kFailure;
			}

			info.distance = static_cast<float>(temp);
			g_options.AddLodInfo(info);

	//		MayaPrintMsg("LOD%i distance: %f, objects: %s", currentLod, info.distance, info.objects.asChar());
			currentLod++;
		}
	}

//	MayaPrintMsg("Total LOD: %i", currentLod);
	if (currentLod == 0) {
		MayaPrintError("Failed to parse and LOD info");
		return MS::kFailure;
	}


	PotatoExporter exportModel;

	return exportModel.convert();
}

void* ModelExporterCmd::creator()
{
	return new ModelExporterCmd;
}
