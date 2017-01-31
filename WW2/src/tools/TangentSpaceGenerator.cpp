#include <syscfg.h> // Path is project specific.
#include "tools/TangentSpaceGenerator.h"

TangentSpaceGenerator::TangentSpaceGenerator()
{
}

static inline int GetType(int flags)
{
	if(flags & OBJECT_LINESTRIP)
	{
		return GL_LINE_STRIP;
	}
	else if(flags & OBJECT_LINECLOSED)
	{
		return GL_LINE_STRIP;
	}
	else if(flags & OBJECT_TRIANGLEFAN)
	{
		if(flags & OBJECT_INDEXED)
		{
			return GL_TRIANGLES;
		}
		else
		{
			return GL_TRIANGLE_FAN;
		}
	}
	else if(flags & OBJECT_TRIANGLELIST)
	{
		return GL_TRIANGLES;
	}

    return 0;
}

void TangentSpaceGenerator::generate(piVectorConstPtr vx, // positions
                                     piVectorConstPtr nx, // normals
                                     piTexCoordConstPtr tx, // texture coords
                                     piINDICES* ptris,
                                     unsigned int numverts,
                                     unsigned int numtris,
                                     unsigned int flags)
{
    unsigned int vertex_count = numverts;

    if(ptris == nullptr)
    {
        T_.assign(vertex_count, Vec4f());
        B_.assign(vertex_count, Vec4f());
        N_.assign(vertex_count, Vec4f());
    }
    else
    {
        unsigned int index_count = numverts;

        T_.assign(index_count, Vec4f());
        B_.assign(index_count, Vec4f());
        N_.assign(index_count, Vec4f());

        for(unsigned int i = 0; i < index_count; i++)
        {
            indices_.push_back(i);
        }
    }

    unsigned int i; // VC6 doesn't like for-scoped variables

    unsigned int N = numtris * 3;

    switch(GetType(flags))
    {
        case GL_TRIANGLES:
        case GL_TRIANGLE_STRIP:
        case GL_TRIANGLE_FAN:
            {
                for(i = 0; i < N; i += 3)
                {
                    compute(vx, nx, tx, ptris, i, i + 1, i + 2);
                }

                break;
            }
        case GL_POINTS:
        case GL_LINES:
        case GL_LINE_STRIP:
        case GL_LINE_LOOP:
            {
                break;
            }
        default:
            {
                WriteLog(LOG_OLD, LOG_WARN, "Warning: TangentSpaceGenerator: unknown primitive mode");
            }
    }

    // normalize basis vectors and force the normal vector to match
    // the triangle normal's direction
    unsigned int attrib_count = 3; // 1 triangle

    if(ptris != nullptr)
    {
        attrib_count = numverts;
    }

    for(i = 0; i < attrib_count; ++i)
    {
        Vec4f &vT = T_[i];
        Vec4f &vB = B_[i];
        Vec4f &vN = N_[i];

        Vec3f txN = Vec3f(vT.x(), vT.y(), vT.z()) ^ Vec3f(vB.x(), vB.y(), vB.z());

        if(txN * Vec3f(vN.x(), vN.y(), vN.z()) >= 0)
        {
            vN = Vec4f(txN, 0);
        }
        else
        {
            vN = Vec4f(-txN, 0);
        }

        vT.normalize();
        vB.normalize();
        vN.normalize();
    }
    /* TO-DO: if indexed, compress the attributes to have only one
     * version of each (different indices for each one?) */
}

void TangentSpaceGenerator::generate(piUNIFIED_VERTEX* vertices,
                                     piINDICES* ptris,
                                     unsigned int numverts,
                                     unsigned int numtris,
                                     unsigned int flags)
{
    piVectorConstPtr vx = piVectorConstPtr((piVECTOR*)&vertices->x, sizeof(piUNIFIED_VERTEX));
    piVectorConstPtr nx = piVectorConstPtr((piVECTOR*)&vertices->nx, sizeof(piUNIFIED_VERTEX));
    piTexCoordConstPtr tx = piTexCoordConstPtr((piUV*)&vertices->tu, sizeof(piUNIFIED_VERTEX));

    generate(vx, nx, tx, ptris, numverts, numtris, flags);
}

void TangentSpaceGenerator::generate(OBJECT* geo)
{
    piVectorConstPtr vx = geo->RenderPositions();
    piVectorConstPtr nx = geo->RenderNormals();
    piTexCoordConstPtr tx = geo->RenderTexCoords();

    if(!vx.ptr || !nx.ptr || !tx.ptr)
    {
        return;
    }

    unsigned int vertex_count = geo->numverts;

    if(geo->ptris == nullptr)
    {
        T_.assign(vertex_count, Vec4f());
        B_.assign(vertex_count, Vec4f());
        N_.assign(vertex_count, Vec4f());
    }
    else
    {
        unsigned int index_count = geo->numverts;

        T_.assign(index_count, Vec4f());
        B_.assign(index_count, Vec4f());
        N_.assign(index_count, Vec4f());

        for(unsigned int i = 0; i < index_count; i++)
        {
            indices_.push_back(i);
        }
    }

    unsigned int i; // VC6 doesn't like for-scoped variables

    unsigned int N = geo->numtris * 3;

    switch(GetType(geo->flags))
    {
        case GL_TRIANGLES:
        case GL_TRIANGLE_STRIP:
        case GL_TRIANGLE_FAN:
            {
                for(i = 0; i < N; i += 3)
                {
                    compute(vx, nx, tx, geo->ptris, i, i + 1, i + 2);
                }

                break;
            }
        case GL_POINTS:
        case GL_LINES:
        case GL_LINE_STRIP:
        case GL_LINE_LOOP:
            {
                break;
            }
        default:
            {
                WriteLog(LOG_OLD, LOG_WARN, "Warning: TangentSpaceGenerator: unknown primitive mode");
            }
    }

    // normalize basis vectors and force the normal vector to match
    // the triangle normal's direction
    unsigned int attrib_count = 3; // 1 triangle

    if(geo->ptris != nullptr)
    {
        attrib_count = geo->numverts;
    }

    for(i = 0; i < attrib_count; ++i)
    {
        Vec4f &vT = T_[i];
        Vec4f &vB = B_[i];
        Vec4f &vN = N_[i];

        Vec3f txN = Vec3f(vT.x(), vT.y(), vT.z()) ^ Vec3f(vB.x(), vB.y(), vB.z());

        if(txN * Vec3f(vN.x(), vN.y(), vN.z()) >= 0)
        {
            vN = Vec4f(txN, 0);
        }
        else
        {
            vN = Vec4f(-txN, 0);
        }

        vT.normalize();
        vB.normalize();
        vN.normalize();
    }
    /* TO-DO: if indexed, compress the attributes to have only one
     * version of each (different indices for each one?) */
}

void TangentSpaceGenerator::compute(piVectorConstPtr vx,
                                    piVectorConstPtr nx,
                                    piTexCoordConstPtr tx,
                                    piINDICES* tris,
                                    int a,
                                    int b,
                                    int c)
{
    unsigned short* indices = (unsigned short*)tris;

    unsigned short iA = indices[a];
    unsigned short iB = indices[b];
    unsigned short iC = indices[c];

    Vec3f P1;
    Vec3f P2;
    Vec3f P3;

    P1.set(vx[iA]);
    P2.set(vx[iB]);
    P3.set(vx[iC]);

    Vec3f N1;
    Vec3f N2;
    Vec3f N3;

    N1.set(nx[iA]);
    N2.set(nx[iB]);
    N3.set(nx[iC]);

    piUV uv1;
    piUV uv2;
    piUV uv3;

    uv1 = tx[iA];
    uv2 = tx[iB];
    uv3 = tx[iC];

    if(nx.ptr)
    {
        Vec3f V, T1, T2, T3, B1, B2, B3;

        V = Vec3f(P2.x() - P1.x(), uv2.u - uv1.u, uv2.v - uv1.v) ^
            Vec3f(P3.x() - P1.x(), uv3.u - uv1.u, uv3.v - uv1.v);

        if(V.x() != 0)
        {
            V.normalize();

            T1.x() += -V.y() / V.x();
            B1.x() += -V.z() / V.x();
            T2.x() += -V.y() / V.x();
            B2.x() += -V.z() / V.x();
            T3.x() += -V.y() / V.x();
            B3.x() += -V.z() / V.x();
        }

        V = Vec3f(P2.y() - P1.y(), uv2.u - uv1.u, uv2.v - uv1.v) ^
            Vec3f(P3.y() - P1.y(), uv3.u - uv1.u, uv3.v - uv1.v);

        if(V.x() != 0)
        {
            V.normalize();

            T1.y() += -V.y() / V.x();
            B1.y() += -V.z() / V.x();
            T2.y() += -V.y() / V.x();
            B2.y() += -V.z() / V.x();
            T3.y() += -V.y() / V.x();
            B3.y() += -V.z() / V.x();
        }

        V = Vec3f(P2.z() - P1.z(), uv2.u - uv1.u, uv2.v - uv1.v) ^
            Vec3f(P3.z() - P1.z(), uv3.u - uv1.u, uv3.v - uv1.v);

        if(V.x() != 0)
        {
            V.normalize();

            T1.z() += -V.y() / V.x();
            B1.z() += -V.z() / V.x();
            T2.z() += -V.y() / V.x();
            B2.z() += -V.z() / V.x();
            T3.z() += -V.y() / V.x();
            B3.z() += -V.z() / V.x();
        }

        Vec3f tempvec;
        tempvec = N1 ^ T1;
        T_[iA] = Vec4f(tempvec ^ N1, 0);
        tempvec = B1 ^ N1;
        B_[iA] = Vec4f(N1 ^ tempvec, 0);
        tempvec = N2 ^ T2;
        T_[iB] = Vec4f(tempvec ^ N2, 0);
        tempvec = B2 ^ N2;
        B_[iB] = Vec4f(N2 ^ tempvec, 0);
        tempvec = N3 ^ T3;
        T_[iC] = Vec4f(tempvec ^ N3, 0);
        tempvec = B3 ^ N3;
        B_[iC] = Vec4f(N3 ^ tempvec, 0);

        N_[iA] += Vec4f(N1, 0);
        N_[iB] += Vec4f(N2, 0);
        N_[iC] += Vec4f(N3, 0);
    }
    else
    {
        Vec3f face_normal = (P2 - P1) ^ (P3 - P1);

        Vec3f V;

        V = Vec3f(P2.x() - P1.x(), uv2.u - uv1.u, uv2.v - uv1.v) ^
            Vec3f(P3.x() - P1.x(), uv3.u - uv1.u, uv3.v - uv1.v);

        if(V.x() != 0)
        {
            V.normalize();

            T_[iA].x() += -V.y() / V.x();
            B_[iA].x() += -V.z() / V.x();
            T_[iB].x() += -V.y() / V.x();
            B_[iB].x() += -V.z() / V.x();
            T_[iC].x() += -V.y() / V.x();
            B_[iC].x() += -V.z() / V.x();
        }

        V = Vec3f(P2.y() - P1.y(), uv2.u - uv1.u, uv2.v - uv1.v) ^
            Vec3f(P3.y() - P1.y(), uv3.u - uv1.u, uv3.v - uv1.v);

        if(V.x() != 0)
        {
            V.normalize();

            T_[iA].y() += -V.y() / V.x();
            B_[iA].y() += -V.z() / V.x();
            T_[iB].y() += -V.y() / V.x();
            B_[iB].y() += -V.z() / V.x();
            T_[iC].y() += -V.y() / V.x();
            B_[iC].y() += -V.z() / V.x();
        }

        V = Vec3f(P2.z() - P1.z(), uv2.u - uv1.u, uv2.v - uv1.v) ^
            Vec3f(P3.z() - P1.z(), uv3.u - uv1.u, uv3.v - uv1.v);

        if(V.x() != 0)
        {
            V.normalize();

            T_[iA].z() += -V.y() / V.x();
            B_[iA].z() += -V.z() / V.x();
            T_[iB].z() += -V.y() / V.x();
            B_[iB].z() += -V.z() / V.x();
            T_[iC].z() += -V.y() / V.x();
            B_[iC].z() += -V.z() / V.x();
        }

        N_[iA] += Vec4f(face_normal, 0);
        N_[iB] += Vec4f(face_normal, 0);
        N_[iC] += Vec4f(face_normal, 0);
    }
}

