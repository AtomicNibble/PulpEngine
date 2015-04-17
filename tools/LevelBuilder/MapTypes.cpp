#include "stdafx.h"
#include "MapTypes.h"

#include <Util\BitUtil.h>

namespace
{

	static const float COPLANAR_EPSILON = 0.1f;


}


X_NAMESPACE_BEGIN(mapfile)

void XMapPatch::GenerateEdgeIndexes(void)
{
	int i, j, i0, i1, i2, s, v0, v1, edgeNum;
	int *index, *vertexEdges, *edgeChain;
	surfaceEdge_t e[3];

	vertexEdges = (int *)_alloca16(verts.size() * sizeof(int));
	memset(vertexEdges, -1, verts.size() * sizeof(int));
	edgeChain = (int *)_alloca16(indexes.size() * sizeof(int));

	edgeIndexes.resize(indexes.size());

	edges.clear();

	// the first edge is a dummy
	e[0].verts[0] = e[0].verts[1] = e[0].tris[0] = e[0].tris[1] = 0;
	edges.push_back(e[0]);

	for (i = 0; i < indexes.size(); i += 3) 
	{
		index = indexes.ptr() + i;
		// vertex numbers
		i0 = index[0];
		i1 = index[1];
		i2 = index[2];
		// setup edges each with smallest vertex number first
		s = core::bitUtil::isSignBitSet(i1 - i0);
		e[0].verts[0] = index[s];
		e[0].verts[1] = index[s ^ 1];
		s = core::bitUtil::isSignBitSet(i2 - i1) + 1;
		e[1].verts[0] = index[s];
		e[1].verts[1] = index[s ^ 3];
		s = core::bitUtil::isSignBitSet(i2 - i0) << 1;
		e[2].verts[0] = index[s];
		e[2].verts[1] = index[s ^ 2];
		// get edges
		for (j = 0; j < 3; j++) {
			v0 = e[j].verts[0];
			v1 = e[j].verts[1];
			for (edgeNum = vertexEdges[v0]; edgeNum >= 0; edgeNum = edgeChain[edgeNum]) {
				if (edges[edgeNum].verts[1] == v1) {
					break;
				}
			}
			// if the edge does not yet exist
			if (edgeNum < 0) {
				e[j].tris[0] = e[j].tris[1] = -1;
				//	edgeNum = edges.push_back(e[j]);
				edges.push_back(e[j]);
				edgeNum = (int)(edges.size() - 1);

				edgeChain[edgeNum] = vertexEdges[v0];
				vertexEdges[v0] = edgeNum;
			}
			// update edge index and edge tri references
			if (index[j] == v0) {
				//			assert(edges[edgeNum].tris[0] == -1); // edge may not be shared by more than two triangles
				edges[edgeNum].tris[0] = i;
				edgeIndexes[i + j] = edgeNum;
			}
			else {
				//			assert(edges[edgeNum].tris[1] == -1); // edge may not be shared by more than two triangles
				edges[edgeNum].tris[1] = i;
				edgeIndexes[i + j] = -edgeNum;
			}
		}
	}
}


void XMapPatch::PutOnCurve(void)
{
	int i, j;
	xVert prev, next;

	X_ASSERT(expanded, "needs to be exapanded")(expanded);

	// put all the approximating points on the curve
	for (i = 0; i < width; i++) {
		for (j = 1; j < height; j += 2) {
			LerpVert(verts[j*maxWidth + i], verts[(j + 1)*maxWidth + i], prev);
			LerpVert(verts[j*maxWidth + i], verts[(j - 1)*maxWidth + i], next);
			LerpVert(prev, next, verts[j*maxWidth + i]);
		}
	}

	for (j = 0; j < height; j++) {
		for (i = 1; i < width; i += 2) {
			LerpVert(verts[j*maxWidth + i], verts[j*maxWidth + i + 1], prev);
			LerpVert(verts[j*maxWidth + i], verts[j*maxWidth + i - 1], next);
			LerpVert(prev, next, verts[j*maxWidth + i]);
		}
	}
}

void XMapPatch::ProjectPointOntoVector(const Vec3f &point, const Vec3f &vStart,
	const Vec3f &vEnd, Vec3f &vProj)
{
	Vec3f pVec, vec;

	pVec = point - vStart;
	vec = vEnd - vStart;
	vec.normalize();
	// project onto the directional vector for this segment
	vProj = vStart + (pVec * vec) * vec;
}


/*

Expects an expanded patch.
*/
void XMapPatch::RemoveLinearColumnsRows(void)
{
	int i, j, k;
	float len, maxLength;
	Vec3f proj, dir;

	X_ASSERT(expanded, "needs to be exapanded")(expanded);

	for (j = 1; j < width - 1; j++) {
		maxLength = 0;
		for (i = 0; i < height; i++) {
			ProjectPointOntoVector(verts[i*maxWidth + j].pos,
				verts[i*maxWidth + j - 1].pos, verts[i*maxWidth + j + 1].pos, proj);
			dir = verts[i*maxWidth + j].pos - proj;
			len = dir.lengthSquared();
			if (len > maxLength) {
				maxLength = len;
			}
		}
		if (maxLength < math<float>::square(0.2f)) {
			width--;
			for (i = 0; i < height; i++) {
				for (k = j; k < width; k++) {
					verts[i*maxWidth + k] = verts[i*maxWidth + k + 1];
				}
			}
			j--;
		}
	}
	for (j = 1; j < height - 1; j++) {
		maxLength = 0;
		for (i = 0; i < width; i++) {
			ProjectPointOntoVector(verts[j*maxWidth + i].pos,
				verts[(j - 1)*maxWidth + i].pos, verts[(j + 1)*maxWidth + i].pos, proj);
			dir = verts[j*maxWidth + i].pos - proj;
			len = dir.lengthSquared();
			if (len > maxLength) {
				maxLength = len;
			}
		}
		if (maxLength < math<float>::square(0.2f)) {
			height--;
			for (i = 0; i < width; i++) {
				for (k = j; k < height; k++) {
					verts[k*maxWidth + i] = verts[(k + 1)*maxWidth + i];
				}
			}
			j--;
		}
	}
}

void XMapPatch::ResizeExpanded(int newHeight, int newWidth)
{
	int i, j;

	X_ASSERT(expanded,"needs to be exapanded")(expanded);
	if (newHeight <= maxHeight && newWidth <= maxWidth) {
		return;
	}
	if (newHeight * newWidth > maxHeight * maxWidth) {
		verts.resize(newHeight * newWidth);
	}
	// space out verts for new height and width
	for (j = maxHeight - 1; j >= 0; j--) {
		for (i = maxWidth - 1; i >= 0; i--) {
			verts[j*newWidth + i] = verts[j*maxWidth + i];
		}
	}
	maxHeight = newHeight;
	maxWidth = newWidth;
}

void XMapPatch::Collapse(void)
{
	int i, j;

	if (!expanded) {
		X_FATAL("Patch", "patch not expanded");
	}
	expanded = false;
	if (width != maxWidth) {
		for (j = 0; j < height; j++) {
			for (i = 0; i < width; i++) {
				verts[j*width + i] = verts[j*maxWidth + i];
			}
		}
	}
	verts.resize(width * height);
}


void XMapPatch::Expand(void)
{
	int i, j;

	if (expanded) {
		X_FATAL("Patch", "patch alread expanded");
	}
	expanded = true;
	verts.resize(maxWidth * maxHeight);
	if (width != maxWidth) {
		for (j = height - 1; j >= 0; j--) {
			for (i = width - 1; i >= 0; i--) {
				verts[j*maxWidth + i] = verts[j*width + i];
			}
		}
	}
}

void XMapPatch::LerpVert(const xVert& a, const xVert& b, xVert& out) const
{
	out.pos = (0.5f * (a.pos + b.pos));
	out.normal = 0.5f * (a.normal + b.normal);
	out.uv = 0.5f * (a.uv + b.uv);
}

/*
Handles all the complicated wrapping and degenerate cases
Expects a Not expanded patch.
*/

void XMapPatch::GenerateNormals(void)
{
	int			i, j, k, dist;
	Vec3f		norm;
	Vec3f		sum;
	int			count;
	Vec3f		base;
	Vec3f		delta;
	int			x, y;
	Vec3f		around[8], temp;
	bool		good[8];
	bool		wrapWidth, wrapHeight;
	static int	neighbors[8][2] = {
		{ 0, 1 }, { 1, 1 }, { 1, 0 }, { 1, -1 }, { 0, -1 }, { -1, -1 }, { -1, 0 }, { -1, 1 }
	};

	//	assert(expanded == false);

	//
	// if all points are coplanar, set all normals to that plane
	//
	Vec3f		extent[3];
	float		offset;
	float		length;

	extent[0] = verts[width - 1].pos - verts[0].pos;
	extent[1] = verts[(height - 1) * width + width - 1].pos - verts[0].pos;
	extent[2] = verts[(height - 1) * width].pos - verts[0].pos;

	norm = extent[0].cross(extent[1]);
	if (norm.lengthSquared() == 0.0f) {
		norm = extent[0].cross(extent[2]);
		if (norm.lengthSquared() == 0.0f) {
			norm = extent[1].cross(extent[2]);
		}
	}

	// wrapped patched may not get a valid normal here
	length = norm.length();
	norm.normalize();
	if (length != 0.0f)
	{
		offset = verts[0].pos * norm;
		for (i = 1; i < width * height; i++) {
			float d = verts[i].pos * norm;
			if (math<float>::abs(d - offset) > COPLANAR_EPSILON) {
				break;
			}
		}

		if (i == width * height) {
			// all are coplanar
			for (i = 0; i < width * height; i++) {
				verts[i].normal = norm;
			}
			return;
		}
	}

	// check for wrapped edge cases, which should smooth across themselves
	wrapWidth = false;
	for (i = 0; i < height; i++) {
		delta = verts[i * width].pos - verts[i * width + width - 1].pos;
		if (delta.lengthSquared() > math<float>::square(1.0f)) {
			break;
		}
	}
	if (i == height) {
		wrapWidth = true;
	}

	wrapHeight = false;
	for (i = 0; i < width; i++) {
		delta = verts[i].pos - verts[(height - 1) * width + i].pos;
		if (delta.lengthSquared() > math<float>::square(1.0f)) {
			break;
		}
	}
	if (i == width) {
		wrapHeight = true;
	}

	for (i = 0; i < width; i++) {
		for (j = 0; j < height; j++) {
			count = 0;
			base = verts[j * width + i].pos;
			for (k = 0; k < 8; k++) {
				around[k] = Vec3f::zero();
				good[k] = false;

				for (dist = 1; dist <= 3; dist++) {
					x = i + neighbors[k][0] * dist;
					y = j + neighbors[k][1] * dist;
					if (wrapWidth) {
						if (x < 0) {
							x = width - 1 + x;
						}
						else if (x >= width) {
							x = 1 + x - width;
						}
					}
					if (wrapHeight) {
						if (y < 0) {
							y = height - 1 + y;
						}
						else if (y >= height) {
							y = 1 + y - height;
						}
					}

					if (x < 0 || x >= width || y < 0 || y >= height) {
						break;					// edge of patch
					}
					temp = verts[y * width + x].pos - base;

					length = norm.length();
					norm.normalize();

					if (length == 0.0f)
					{
						continue;				// degenerate edge, get more dist
					}
					else {
						good[k] = true;
						around[k] = temp;
						break;					// good edge
					}
				}
			}

			sum = Vec3f::zero();
			for (k = 0; k < 8; k++) {
				if (!good[k] || !good[(k + 1) & 7]) {
					continue;	// didn't get two points
				}
				norm = around[(k + 1) & 7].cross(around[k]);

				length = norm.length();
				norm.normalize();

				if (length == 0.0f) {
					continue;
				}
				sum += norm;
				count++;
			}
			if (count == 0) {
				//idLib::common->Printf("bad normal\n");
				count = 1;
			}
			verts[j * width + i].normal = sum;
			verts[j * width + i].normal.normalize();
		}
	}
}


void XMapPatch::GenerateIndexes(void)
{
	int i, j, v1, v2, v3, v4, index;

	indexes.resize((width - 1) * (height - 1) * 2 * 3, false);
	index = 0;
	for (i = 0; i < width - 1; i++) {
		for (j = 0; j < height - 1; j++) {
			v1 = j * width + i;
			v2 = v1 + 1;
			v3 = v1 + width + 1;
			v4 = v1 + width;
			indexes[index++] = v1;
			indexes[index++] = v3;
			indexes[index++] = v2;
			indexes[index++] = v1;
			indexes[index++] = v4;
			indexes[index++] = v3;
		}
	}

	GenerateEdgeIndexes();
}


void XMapPatch::Subdivide(float maxHorizontalError, float maxVerticalError, float maxLength, bool genNormals)
{
	int			i, j, k, l;
	xVert	prev, next, mid;
	Vec3f		prevxyz, nextxyz, midxyz;
	Vec3f		delta;
	float		maxHorizontalErrorSqr, maxVerticalErrorSqr, maxLengthSqr;

	// generate normals for the control mesh
	if (genNormals) {
		GenerateNormals();
	}

	maxHorizontalErrorSqr = math<float>::square(maxHorizontalError);
	maxVerticalErrorSqr = math<float>::square(maxVerticalError);
	maxLengthSqr = math<float>::square(maxLength);

	Expand();

	// horizontal subdivisions
	for (j = 0; j + 2 < width; j += 2) {
		// check subdivided midpoints against control points
		for (i = 0; i < height; i++) {
			for (l = 0; l < 3; l++) {
				prevxyz[l] = verts[i*maxWidth + j + 1].pos[l] - verts[i*maxWidth + j].pos[l];
				nextxyz[l] = verts[i*maxWidth + j + 2].pos[l] - verts[i*maxWidth + j + 1].pos[l];
				midxyz[l] = (verts[i*maxWidth + j].pos[l] + verts[i*maxWidth + j + 1].pos[l] * 2.0f +
					verts[i*maxWidth + j + 2].pos[l]) * 0.25f;
			}

			if (maxLength > 0.0f) {
				// if the span length is too long, force a subdivision
				if (prevxyz.lengthSquared() > maxLengthSqr || nextxyz.lengthSquared() > maxLengthSqr) {
					break;
				}
			}
			// see if this midpoint is off far enough to subdivide
			delta = verts[i*maxWidth + j + 1].pos - midxyz;
			if (delta.lengthSquared() > maxHorizontalErrorSqr) {
				break;
			}
		}

		if (i == height) {
			continue;	// didn't need subdivision
		}

		if (width + 2 >= maxWidth) {
			ResizeExpanded(maxHeight, maxWidth + 4);
		}

		// insert two columns and replace the peak
		width += 2;

		for (i = 0; i < height; i++) {
			LerpVert(verts[i*maxWidth + j], verts[i*maxWidth + j + 1], prev);
			LerpVert(verts[i*maxWidth + j + 1], verts[i*maxWidth + j + 2], next);
			LerpVert(prev, next, mid);

			for (k = width - 1; k > j + 3; k--) {
				verts[i*maxWidth + k] = verts[i*maxWidth + k - 2];
			}
			verts[i*maxWidth + j + 1] = prev;
			verts[i*maxWidth + j + 2] = mid;
			verts[i*maxWidth + j + 3] = next;
		}

		// back up and recheck this set again, it may need more subdivision
		j -= 2;
	}

	// vertical subdivisions
	for (j = 0; j + 2 < height; j += 2) {
		// check subdivided midpoints against control points
		for (i = 0; i < width; i++) {
			for (l = 0; l < 3; l++) {
				prevxyz[l] = verts[(j + 1)*maxWidth + i].pos[l] - verts[j*maxWidth + i].pos[l];
				nextxyz[l] = verts[(j + 2)*maxWidth + i].pos[l] - verts[(j + 1)*maxWidth + i].pos[l];
				midxyz[l] = (verts[j*maxWidth + i].pos[l] + verts[(j + 1)*maxWidth + i].pos[l] * 2.0f +
					verts[(j + 2)*maxWidth + i].pos[l]) * 0.25f;
			}

			if (maxLength > 0.0f) {
				// if the span length is too long, force a subdivision
				if (prevxyz.lengthSquared() > maxLengthSqr || nextxyz.lengthSquared() > maxLengthSqr) {
					break;
				}
			}
			// see if this midpoint is off far enough to subdivide
			delta = verts[(j + 1)*maxWidth + i].pos - midxyz;
			if (delta.lengthSquared() > maxVerticalErrorSqr) {
				break;
			}
		}

		if (i == width) {
			continue;	// didn't need subdivision
		}

		if (height + 2 >= maxHeight) {
			ResizeExpanded(maxHeight + 4, maxWidth);
		}

		// insert two columns and replace the peak
		height += 2;

		for (i = 0; i < width; i++) {
			LerpVert(verts[j*maxWidth + i], verts[(j + 1)*maxWidth + i], prev);
			LerpVert(verts[(j + 1)*maxWidth + i], verts[(j + 2)*maxWidth + i], next);
			LerpVert(prev, next, mid);

			for (k = height - 1; k > j + 3; k--) {
				verts[k*maxWidth + i] = verts[(k - 2)*maxWidth + i];
			}
			verts[(j + 1)*maxWidth + i] = prev;
			verts[(j + 2)*maxWidth + i] = mid;
			verts[(j + 3)*maxWidth + i] = next;
		}

		// back up and recheck this set again, it may need more subdivision
		j -= 2;
	}

	PutOnCurve();

	RemoveLinearColumnsRows();

	Collapse();

	// normalize all the lerped normals
	if (genNormals) {
		for (i = 0; i < width * height; i++) {
			verts[i].normal.normalize();
		}
	}

	GenerateIndexes();
}


void XMapPatch::SampleSinglePatchPoint(const xVert ctrl[3][3], float u, float v, xVert* out) const
{
	float	vCtrl[3][8];
	int		vPoint;
	int		axis;

	// find the control points for the v coordinate
	for (vPoint = 0; vPoint < 3; vPoint++) {
		for (axis = 0; axis < 8; axis++) {
			float a, b, c;
			float qA, qB, qC;
			if (axis < 3) {
				a = ctrl[0][vPoint].pos[axis];
				b = ctrl[1][vPoint].pos[axis];
				c = ctrl[2][vPoint].pos[axis];
			}
			else if (axis < 6) {
				a = ctrl[0][vPoint].normal[axis - 3];
				b = ctrl[1][vPoint].normal[axis - 3];
				c = ctrl[2][vPoint].normal[axis - 3];
			}
			else {
				a = ctrl[0][vPoint].uv[axis - 6];
				b = ctrl[1][vPoint].uv[axis - 6];
				c = ctrl[2][vPoint].uv[axis - 6];
			}
			qA = a - 2.0f * b + c;
			qB = 2.0f * b - 2.0f * a;
			qC = a;
			vCtrl[vPoint][axis] = qA * u * u + qB * u + qC;
		}
	}

	// interpolate the v value
	for (axis = 0; axis < 8; axis++) {
		float a, b, c;
		float qA, qB, qC;

		a = vCtrl[0][axis];
		b = vCtrl[1][axis];
		c = vCtrl[2][axis];
		qA = a - 2.0f * b + c;
		qB = 2.0f * b - 2.0f * a;
		qC = a;

		if (axis < 3) {
			out->pos[axis] = qA * v * v + qB * v + qC;
		}
		else if (axis < 6) {
			out->normal[axis - 3] = qA * v * v + qB * v + qC;
		}
		else {
			out->uv[axis - 6] = qA * v * v + qB * v + qC;
		}
	}
}

void XMapPatch::SampleSinglePatch(const xVert ctrl[3][3], int baseCol, int baseRow,
	int width, int horzSub, int vertSub, xVert* outVerts) const
{
	int		i, j;
	float	u, v;

	horzSub++;
	vertSub++;
	for (i = 0; i < horzSub; i++) {
		for (j = 0; j < vertSub; j++) {
			u = (float)i / (horzSub - 1);
			v = (float)j / (vertSub - 1);
			SampleSinglePatchPoint(ctrl, u, v, &outVerts[((baseRow + j) * width) + i + baseCol]);
		}
	}
}


void XMapPatch::SubdivideExplicit(int horzSubdivisions, int vertSubdivisions,
	bool genNormals, bool removeLinear)
{
	int i, j, k, l;
	xVert sample[3][3];
	int outWidth = ((width - 1) / 2 * horzSubdivisions) + 1;
	int outHeight = ((height - 1) / 2 * vertSubdivisions) + 1;
	xVert *dv = new xVert[outWidth * outHeight];

	// generate normals for the control mesh
	if (genNormals) {
		GenerateNormals();
	}

	int baseCol = 0;
	for (i = 0; i + 2 < width; i += 2) {
		int baseRow = 0;
		for (j = 0; j + 2 < height; j += 2) {
			for (k = 0; k < 3; k++) {
				for (l = 0; l < 3; l++) {
					sample[k][l] = verts[((j + l) * width) + i + k];
				}
			}
			SampleSinglePatch(sample, baseCol, baseRow, outWidth, horzSubdivisions, vertSubdivisions, dv);
			baseRow += vertSubdivisions;
		}
		baseCol += horzSubdivisions;
	}
	verts.resize(outWidth * outHeight);
	for (i = 0; i < outWidth * outHeight; i++) {
		verts[i] = dv[i];
	}

	delete[] dv;

	width = maxWidth = outWidth;
	height = maxHeight = outHeight;
	expanded = false;

	if (removeLinear) {
		Expand();
		RemoveLinearColumnsRows();
		Collapse();
	}

	// normalize all the lerped normals
	if (genNormals) {
		for (i = 0; i < width * height; i++) {
			verts[i].normal.normalize();
		}
	}

	GenerateIndexes();
}

X_NAMESPACE_END
