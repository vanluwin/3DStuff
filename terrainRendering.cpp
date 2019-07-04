#include "olcConsoleGameEngineSDL.h"
#include <fstream>
#include <strstream>
#include <algorithm>
using namespace std;

struct vec3d {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 1.0f;
};

struct triangle {
    vec3d p[3];

    // Color of the triangle
    unsigned short  sym;
    short col;
};

struct mesh {
    vector<triangle> tris;

    bool LoadFromObjectFile(string sFileName) {
        
        ifstream file(sFileName);
        if(!file.is_open())
            return false;

        // Local cache of vertices
        vector<vec3d> verts;

        while(!file.eof()) {
            char line[128];
            file.getline(line, 128);

            strstream s;
            s << line;

            char junk;

            if(line[0] == 'v') {
                vec3d v;
                s >> junk >> v.x >> v.y >> v.z;
                verts.push_back(v);
            } else if (line[0] == 'f') {
                int f[3];
                s >> junk >> f[0] >> f[1] >> f[2];

                tris.push_back({ verts[f[0] -1], verts[f[1] - 1], verts[f[2] - 1] });
            }
        }


        return true;
    }
};

struct mat4x4 {
    float m[4][4] = { 0 };
};

class Engine3D : public olcConsoleGameEngine {
    private:
        // Field of view
        float fTheta;

        // Direction that the 'player' is facing
        float fYaw;
        
        // Observer Position
        vec3d vCamera;
        vec3d vLookDir;

        mesh meshCube;
        mat4x4 matProj;

        vec3d MatrixMultiplyVector(mat4x4 &m, vec3d &i) {
            vec3d v;

            v.x = i.x * m.m[0][0] + i.y * m.m[1][0] + i.z * m.m[2][0] + i.w * m.m[3][0];
            v.y = i.x * m.m[0][1] + i.y * m.m[1][1] + i.z * m.m[2][1] + i.w * m.m[3][1];
            v.z = i.x * m.m[0][2] + i.y * m.m[1][2] + i.z * m.m[2][2] + i.w * m.m[3][2];
            v.w = i.x * m.m[0][3] + i.y * m.m[1][3] + i.z * m.m[2][3] + i.w * m.m[3][3];

            return v;
        }

        mat4x4 IdentityMatrix() {
            mat4x4 matrix;

            matrix.m[0][0] = 1.0f;
            matrix.m[1][1] = 1.0f;
            matrix.m[2][2] = 1.0f;
            matrix.m[3][3] = 1.0f;
        
            return matrix;
        }

        mat4x4 MatrixRotationX(float fAngleRad) {
            mat4x4 matrix;
            
            matrix.m[0][0] = 1.0f;
            matrix.m[1][1] = cosf(fAngleRad);
            matrix.m[1][2] = sinf(fAngleRad);
            matrix.m[2][1] = -sinf(fAngleRad);
            matrix.m[2][2] = cosf(fAngleRad);
            matrix.m[3][3] = 1.0f;

            return matrix;
        }

        mat4x4 MatrixRotationY(float fAngleRad) {
            mat4x4 matrix;
            
            matrix.m[0][0] = cosf(fAngleRad);
            matrix.m[0][2] = sinf(fAngleRad);
            matrix.m[2][0] = -sinf(fAngleRad);
            matrix.m[1][1] = 1.0f;
            matrix.m[2][2] = cosf(fAngleRad);
            matrix.m[3][3] = 1.0f;

            return matrix;
        }

        mat4x4 MatrixRotationZ(float fAngleRad) {
            mat4x4 matrix;
            
            matrix.m[0][0] = cosf(fAngleRad);
            matrix.m[0][1] = sinf(fAngleRad);
            matrix.m[1][0] = -sinf(fAngleRad);
            matrix.m[1][1] = cosf(fAngleRad);
            matrix.m[2][2] = 1.0f;
            matrix.m[3][3] = 1.0f;

            return matrix;
        }

        mat4x4 MatrixTranslation(float x, float y, float z) {
            mat4x4 matrix;
            
            matrix.m[0][0] = 1.0f;
            matrix.m[1][1] = 1.0f;
            matrix.m[2][2] = 1.0f;
            matrix.m[3][3] = 1.0f;
            matrix.m[3][0] = x;
            matrix.m[3][1] = y;
            matrix.m[3][2] = z;

            return matrix;
        }

        mat4x4 MatrixProjection(float fFovDegrees, float fAspectRatio, float fNear, float fFar) {
            float fFovRad = 1.0f  / tanf(fFovDegrees * 0.5f / 180.0f * 3.14159f);
            mat4x4 matrix;

            matrix.m[0][0] = fAspectRatio * fFovRad;
            matrix.m[1][1] = fFovRad;
            matrix.m[2][2] = fFar / (fFar - fNear);
            matrix.m[3][2] = (-fNear * fFar) / (fFar - fNear);
            matrix.m[2][3] = 1.0f;
            matrix.m[3][3] = 0.0f;
        
            return matrix;
        }

        mat4x4 MatrixMultiplyMatrix(mat4x4 &m1, mat4x4 &m2) {
            mat4x4 matrix;

            for(int c = 0; c < 4; c++) {
                for(int r = 0; r < 4; r++) {
                    matrix.m[r][c] = 
                        m1.m[r][0] * m2.m[0][c] + 
                        m1.m[r][1] * m2.m[1][c] + 
                        m1.m[r][2] * m2.m[2][c] + 
                        m1.m[r][3] * m2.m[3][c]
                    ;
                }
            }

            return matrix;
        }

        mat4x4 MatrixPointAt(vec3d &pos, vec3d &target, vec3d &up) {
            // Calculate new forward direction
            vec3d newForward = VectorSub(target, pos);
            newForward = Normalise(newForward);

            // Calculate new Up direction
            vec3d a = VectorMul(newForward, DotProduct(up, newForward));
            vec3d newUp = VectorSub(up, a);
            newUp = Normalise(newUp);

            // Calculate new Right direction
            vec3d newRight = CrossProduct(newUp, newForward);

            // Construct Dimensioning and Translation Matrix	
            mat4x4 matrix;
            matrix.m[0][0] = newRight.x;	matrix.m[0][1] = newRight.y;	matrix.m[0][2] = newRight.z;	matrix.m[0][3] = 0.0f;
            matrix.m[1][0] = newUp.x;		matrix.m[1][1] = newUp.y;		matrix.m[1][2] = newUp.z;		matrix.m[1][3] = 0.0f;
            matrix.m[2][0] = newForward.x;	matrix.m[2][1] = newForward.y;	matrix.m[2][2] = newForward.z;	matrix.m[2][3] = 0.0f;
            matrix.m[3][0] = pos.x;			matrix.m[3][1] = pos.y;			matrix.m[3][2] = pos.z;			matrix.m[3][3] = 1.0f;
        
            return matrix;
        }
        
        // Only for Rotation/Translation Matrices
        mat4x4 QuickInverse(mat4x4 &m) {
            mat4x4 matrix;
            
            matrix.m[0][0] = m.m[0][0]; matrix.m[0][1] = m.m[1][0]; matrix.m[0][2] = m.m[2][0]; matrix.m[0][3] = 0.0f;
            matrix.m[1][0] = m.m[0][1]; matrix.m[1][1] = m.m[1][1]; matrix.m[1][2] = m.m[2][1]; matrix.m[1][3] = 0.0f;
            matrix.m[2][0] = m.m[0][2]; matrix.m[2][1] = m.m[1][2]; matrix.m[2][2] = m.m[2][2]; matrix.m[2][3] = 0.0f;
            matrix.m[3][0] = -(m.m[3][0] * matrix.m[0][0] + m.m[3][1] * matrix.m[1][0] + m.m[3][2] * matrix.m[2][0]);
            matrix.m[3][1] = -(m.m[3][0] * matrix.m[0][1] + m.m[3][1] * matrix.m[1][1] + m.m[3][2] * matrix.m[2][1]);
            matrix.m[3][2] = -(m.m[3][0] * matrix.m[0][2] + m.m[3][1] * matrix.m[1][2] + m.m[3][2] * matrix.m[2][2]);
            matrix.m[3][3] = 1.0f;
            
            return matrix;
        }

        vec3d VectorAdd(vec3d &v1, vec3d &v2) {
            return { v1.x + v2.x, v1.y + v2.y, v1.z + v2.z };
        }

        vec3d VectorSub(vec3d &v1, vec3d &v2) {
            return { v1.x - v2.x, v1.y - v2.y, v1.z - v2.z };
        }

        vec3d VectorMul(vec3d &v1, float k) {
            return { v1.x * k, v1.y * k, v1.z * k };
        }

        vec3d VectorDiv(vec3d &v1, float k) {
            return { v1.x / k, v1.y / k, v1.z / k };
        }

        float DotProduct(vec3d &v1, vec3d &v2) {
            return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
        }

        float VectorLength(vec3d &v) {
            return sqrtf(DotProduct(v, v));
        }

        vec3d Normalise(vec3d &v) {
            float l = VectorLength(v);
            return { v.x / l, v.y / l, v.z / l };
        }

        vec3d CrossProduct(vec3d &v1, vec3d &v2) {
            vec3d v;

            v.x = v1.y * v2.z - v1.z * v2.y;
            v.y = v1.z * v2.x - v1.x * v2.z;
            v.z = v1.x * v2.y - v1.y * v2.x;

            return v;
        }

        vec3d VectorIntersectPlane(vec3d &plane_p, vec3d &plane_n, vec3d &lineStart, vec3d &lineEnd) {
            plane_n = Normalise(plane_n);
            float plane_d = -DotProduct(plane_n, plane_p);
            float ad = DotProduct(lineStart, plane_n);
            float bd = DotProduct(lineEnd, plane_n);
            float t = (-plane_d - ad) / (bd - ad);
            vec3d lineStartToEnd = VectorSub(lineEnd, lineStart);
            vec3d lineToIntersect = VectorMul(lineStartToEnd, t);
            
            return VectorAdd(lineStart, lineToIntersect);
        }

        int TriangleClipAgainstPlane(vec3d plane_p, vec3d plane_n, triangle &in_tri, triangle &out_tri1, triangle &out_tri2) {
            // Making sure that the plane normal is inded normal
            plane_n = Normalise(plane_n);

            // Return signed shortest distance from point to plane, plane normal must be normalised
            auto dist = [&](vec3d &p) {
                vec3d n = Normalise(p);

                return (plane_n.x * p.x + plane_n.y * p.y + plane_n.z * p.z - DotProduct(plane_n, plane_p));
            };

            // Create two temporary storage arrays to classify points either side of plane
		    // If distance sign is positive, point lies on "inside" of plane
		    vec3d* inside_points[3];  int nInsidePointCount = 0;
            vec3d* outside_points[3]; int nOutsidePointCount = 0;

            // Get signed distance of each point in triangle to plane
		    float d0 = dist(in_tri.p[0]);
		    float d1 = dist(in_tri.p[1]);
            float d2 = dist(in_tri.p[2]);

            if (d0 >= 0) { 
                inside_points[nInsidePointCount++] = &in_tri.p[0]; 
            } else { 
                outside_points[nOutsidePointCount++] = &in_tri.p[0]; 
            }
            
            if (d1 >= 0) { 
                inside_points[nInsidePointCount++] = &in_tri.p[1]; 
            }else { 
                outside_points[nOutsidePointCount++] = &in_tri.p[1]; 
            }

            if (d2 >= 0) { 
                inside_points[nInsidePointCount++] = &in_tri.p[2]; 
            }else { 
                outside_points[nOutsidePointCount++] = &in_tri.p[2]; 
            }

            if (nInsidePointCount == 0) {
                // All points lie on the outside of plane, so clip whole triangle
                // It ceases to exist

                return 0; // No returned triangles are valid
            }

            if (nInsidePointCount == 3) {
                // All points lie on the inside of plane, so do nothing
                // and allow the triangle to simply pass through
                out_tri1 = in_tri;

                return 1; // Just the one returned original triangle is valid
            }

            if (nInsidePointCount == 1 && nOutsidePointCount == 2) {
                // Triangle should be clipped. As two points lie outside
                // the plane, the triangle simply becomes a smaller triangle

                // Copy appearance info to new triangle
                out_tri1.col =  in_tri.col;
                out_tri1.sym = in_tri.sym;

                // The inside point is valid, so keep that...
                out_tri1.p[0] = *inside_points[0];

                // but the two new points are at the locations where the 
                // original sides of the triangle (lines) intersect with the plane
                out_tri1.p[1] = VectorIntersectPlane(plane_p, plane_n, *inside_points[0], *outside_points[0]);
                out_tri1.p[2] = VectorIntersectPlane(plane_p, plane_n, *inside_points[0], *outside_points[1]);

                return 1; // Return the newly formed single triangle
            }

            if (nInsidePointCount == 2 && nOutsidePointCount == 1) {
                // Triangle should be clipped. As two points lie inside the plane,
                // the clipped triangle becomes a "quad". Fortunately, we can
                // represent a quad with two new triangles

                // Copy appearance info to new triangles
                out_tri1.col =  in_tri.col;
                out_tri1.sym = in_tri.sym;

                out_tri2.col =  in_tri.col;
                out_tri2.sym = in_tri.sym;

                // The first triangle consists of the two inside points and a new
                // point determined by the location where one side of the triangle
                // intersects with the plane
                out_tri1.p[0] = *inside_points[0];
                out_tri1.p[1] = *inside_points[1];
                out_tri1.p[2] = VectorIntersectPlane(plane_p, plane_n, *inside_points[0], *outside_points[0]);

                // The second triangle is composed of one of he inside points, a
                // new point determined by the intersection of the other side of the 
                // triangle and the plane, and the newly created point above
                out_tri2.p[0] = *inside_points[1];
                out_tri2.p[1] = out_tri1.p[2];
                out_tri2.p[2] = VectorIntersectPlane(plane_p, plane_n, *inside_points[1], *outside_points[0]);

                return 2; // Return two newly formed triangles which form a quad
            }

        }

        // Taken From Command Line Webcam Video
        CHAR_INFO GetColour(float lum) {
            short bg_col, fg_col;
            wchar_t sym;
            int pixel_bw = (int)(13.0f*lum);
            switch (pixel_bw)
            {
            case 0: bg_col = BG_BLACK; fg_col = FG_BLACK; sym = PIXEL_SOLID; break;

            case 1: bg_col = BG_BLACK; fg_col = FG_DARK_GREY; sym = PIXEL_QUARTER; break;
            case 2: bg_col = BG_BLACK; fg_col = FG_DARK_GREY; sym = PIXEL_HALF; break;
            case 3: bg_col = BG_BLACK; fg_col = FG_DARK_GREY; sym = PIXEL_THREEQUARTERS; break;
            case 4: bg_col = BG_BLACK; fg_col = FG_DARK_GREY; sym = PIXEL_SOLID; break;

            case 5: bg_col = BG_DARK_GREY; fg_col = FG_GREY; sym = PIXEL_QUARTER; break;
            case 6: bg_col = BG_DARK_GREY; fg_col = FG_GREY; sym = PIXEL_HALF; break;
            case 7: bg_col = BG_DARK_GREY; fg_col = FG_GREY; sym = PIXEL_THREEQUARTERS; break;
            case 8: bg_col = BG_DARK_GREY; fg_col = FG_GREY; sym = PIXEL_SOLID; break;

            case 9:  bg_col = BG_GREY; fg_col = FG_WHITE; sym = PIXEL_QUARTER; break;
            case 10: bg_col = BG_GREY; fg_col = FG_WHITE; sym = PIXEL_HALF; break;
            case 11: bg_col = BG_GREY; fg_col = FG_WHITE; sym = PIXEL_THREEQUARTERS; break;
            case 12: bg_col = BG_GREY; fg_col = FG_WHITE; sym = PIXEL_SOLID; break;
            default:
                bg_col = BG_BLACK; fg_col = FG_BLACK; sym = PIXEL_SOLID;
            }

            CHAR_INFO c;
            //c.Attributes = bg_col | fg_col;
            //c.Char.UnicodeChar = sym;
            c.colour = bg_col | fg_col;
            c.glyph = sym;
            
            //cout << bg_col << endl << fg_col << endl << sym << "\n\n";
            
            return c;
        }

    public:
        Engine3D() {
            m_sAppName = L"3D Window";
        }

        bool OnUserCreate() override {
           
            meshCube.LoadFromObjectFile("objects/mountains.obj");

            // Projection Matrix
            matProj = MatrixProjection(90.0f, (float)ScreenHeight() / (float)ScreenWidth(), 0.1f, 1000.0f);

            return true;
        }

        bool OnUserUpdate(float fElapsedTime) override {
            // Travel Upwards
            if (GetKey(VK_UP).bHeld)
			    vCamera.y += 8.0f * fElapsedTime;	

            // Travel Downwards
            if (GetKey(VK_DOWN).bHeld)
                vCamera.y -= 8.0f * fElapsedTime; 
            
            // Travel Along X-Axis
            if (GetKey(VK_LEFT).bHeld)
			    vCamera.x -= 8.0f * fElapsedTime;	

            // Travel Along X-Axis
		    if (GetKey(VK_RIGHT).bHeld)
                vCamera.x += 8.0f * fElapsedTime; 

            vec3d vForward = VectorMul(vLookDir, 8.0f * fElapsedTime);

            if (GetKey(L'W').bHeld)
			    vCamera = VectorAdd(vCamera, vForward);

            if (GetKey(L'S').bHeld)
                vCamera = VectorSub(vCamera, vForward);

            if (GetKey(L'A').bHeld)
                fYaw -= 2.0f * fElapsedTime;

            if (GetKey(L'D').bHeld)
                fYaw += 2.0f * fElapsedTime;

            // Clear the screen
            Fill(0 ,0, ScreenWidth(), ScreenHeight(), PIXEL_SOLID, FG_BLACK);

            // Roatation Matrices
            mat4x4 matRotZ, matRotX;
            //fTheta += 1.0f * fElapsedTime;

            // Rotation Matrices
            matRotZ = MatrixRotationZ(fTheta * 0.5f);
            matRotX = MatrixRotationX(fTheta);

            // Translation Matrix
            mat4x4 matTrans;
		    matTrans = MatrixTranslation(0.0f, 0.0f, 5.0f);

            // World Matrix
		    mat4x4 matWorld;
            matWorld = IdentityMatrix();
            matWorld = MatrixMultiplyMatrix(matRotZ, matRotX); // Transform by rotation
            matWorld = MatrixMultiplyMatrix(matWorld, matTrans); // Transform by translation

            vLookDir = { 0, 0, 1};
            vec3d vUp = { 0, 1, 0 };
            vec3d vTarget = { 0, 0, 1 };
            mat4x4 matCameraRot = MatrixRotationY(fYaw);
            vLookDir = MatrixMultiplyVector(matCameraRot, vTarget);
            vTarget = VectorAdd(vCamera, vLookDir);

            mat4x4 matCamera = MatrixPointAt(vCamera, vTarget, vUp);

            // View from camera
            mat4x4 matView = QuickInverse(matCamera);

            // Store triagles for rastering later
            vector<triangle> vecTrianglesToRaster;

            // Draw triangles
            for(auto tri : meshCube.tris) {

                triangle triProjected, triTransformed, triViewed;

                // World Matrix Transform
                triTransformed.p[0] = MatrixMultiplyVector(matWorld, tri.p[0]);
                triTransformed.p[1] = MatrixMultiplyVector(matWorld, tri.p[1]);
                triTransformed.p[2] = MatrixMultiplyVector(matWorld, tri.p[2]);

                // Calculate triangle normal
                vec3d normal, line1, line2;

                // Get lines either side of triangle
                line1 = VectorSub(triTransformed.p[1], triTransformed.p[0]);
                line2 = VectorSub(triTransformed.p[2], triTransformed.p[0]);

                // Unit normal to the trinagle surface
                normal = CrossProduct(line1, line2);
                normal = Normalise(normal);

                // Ray from tringle to camera
                vec3d vCameraRay = VectorSub(triTransformed.p[0], vCamera);

                if(DotProduct(normal, vCameraRay) < 0.0f){
                    // Illumination
                    vec3d lightDirection = { 0.0f, 0.0f, -1.0f };
                    lightDirection = Normalise(lightDirection);
                    
                    // How "aligned" are light direction and triangle surface normal?
                    float dp = max(0.1f, DotProduct(lightDirection, normal));; 

                    CHAR_INFO c = GetColour(dp);
                    triTransformed.col = c.colour;
                    triTransformed.sym = c.glyph;

                    // Convert World Sapce to View Space
                    triViewed.p[0] = MatrixMultiplyVector(matView, triTransformed.p[0]);
                    triViewed.p[1] = MatrixMultiplyVector(matView, triTransformed.p[1]);
                    triViewed.p[2] = MatrixMultiplyVector(matView, triTransformed.p[2]);

                    // Clip Viewed Triangle against near plane, this could form two additional
                    // additional triangles. 
                    int nClippedTriangles = 0;
                    triangle clipped[2];
                    nClippedTriangles = TriangleClipAgainstPlane(
                        { 0.0f, 0.0f, 0.1f }, 
                        { 0.0f, 0.0f, 1.0f }, 
                        triViewed, 
                        clipped[0], 
                        clipped[1]
                    );

                    for(int n = 0; n < nClippedTriangles; n++) {
                        // Project triangles from 3D --> 2D
                        triProjected.p[0] = MatrixMultiplyVector(matProj, clipped[n].p[0]);
                        triProjected.p[1] = MatrixMultiplyVector(matProj, clipped[n].p[1]);
                        triProjected.p[2] = MatrixMultiplyVector(matProj, clipped[n].p[2]);
                        triProjected.col = triTransformed.col;
                        triProjected.sym = triTransformed.sym;

                        // Scale into view
                        triProjected.p[0] = VectorDiv(triProjected.p[0], triProjected.p[0].w);
                        triProjected.p[1] = VectorDiv(triProjected.p[1], triProjected.p[1].w);
                        triProjected.p[2] = VectorDiv(triProjected.p[2], triProjected.p[2].w);

                        // Offset the screen space to the center of the screen 
                        vec3d vOffsetView = { 1, 1, 0 };
                        triProjected.p[0] = VectorAdd(triProjected.p[0], vOffsetView);
                        triProjected.p[1] = VectorAdd(triProjected.p[1], vOffsetView);
                        triProjected.p[2] = VectorAdd(triProjected.p[2], vOffsetView);

                        triProjected.p[0].x *= 0.5f * (float)ScreenWidth();
                        triProjected.p[0].y *= 0.5f * (float)ScreenHeight();
                        triProjected.p[1].x *= 0.5f * (float)ScreenWidth();
                        triProjected.p[1].y *= 0.5f * (float)ScreenHeight();
                        triProjected.p[2].x *= 0.5f * (float)ScreenWidth();
                        triProjected.p[2].y *= 0.5f * (float)ScreenHeight();

                        // Rasterize triangle
                        vecTrianglesToRaster.push_back(triProjected);   
                    }
                }
            }

            // Sort tringles from back to front
            sort(
                vecTrianglesToRaster.begin(),
                vecTrianglesToRaster.end(),
                [](triangle &t1, triangle &t2) {
                    float z1 = (t1.p[0].z + t1.p[1].z + t1.p[2].z) / 3.0f;
                    float z2 = (t2.p[0].z + t2.p[1].z + t2.p[2].z) / 3.0f;

                    return z1 > z2;
                }
            );

            for(auto &triToRaster : vecTrianglesToRaster) {
                // Clip triangles against all four screen edges
                triangle clipped[2];
                list<triangle> listTriangles;
                listTriangles.push_back(triToRaster);
                
                int nNewTriangles = 1;
                for (int p = 0; p < 4; p++) {
                    int nTrisToAdd = 0;
                    while (nNewTriangles > 0) {
                        // Take triangle from front of queue
                        triangle test = listTriangles.front();
                        listTriangles.pop_front();
                        nNewTriangles--;

                        // Clip it against a plane. We only need to test each 
                        // subsequent plane, against subsequent new triangles
                        // as all triangles after a plane clip are guaranteed
                        // to lie on the inside of the plane. I like how this
                        // comment is almost completely and utterly justified
                        switch (p) {
                            case 0:	
                                nTrisToAdd = TriangleClipAgainstPlane(
                                    { 0.0f, 0.0f, 0.0f }, 
                                    { 0.0f, 1.0f, 0.0f }, 
                                    test, clipped[0], clipped[1]
                                ); 
                                break;
                            case 1:	
                                nTrisToAdd = TriangleClipAgainstPlane(
                                    { 0.0f, (float)ScreenHeight() - 1, 0.0f }, 
                                    { 0.0f, -1.0f, 0.0f }, 
                                    test, clipped[0], clipped[1]
                                ); 
                                break;
                            case 2:	
                                nTrisToAdd = TriangleClipAgainstPlane(
                                    { 0.0f, 0.0f, 0.0f }, 
                                    { 1.0f, 0.0f, 0.0f }, 
                                    test, clipped[0], clipped[1]
                                );
                                break;
                            case 3:	
                                nTrisToAdd = TriangleClipAgainstPlane(
                                    { (float)ScreenWidth() - 1, 0.0f, 0.0f }, 
                                    { -1.0f, 0.0f, 0.0f }, 
                                    test, clipped[0], clipped[1]
                                ); 
                                break;
                        }

                        // Clipping may yield a variable number of triangles, so
                        // add these new ones to the back of the queue for subsequent
                        // clipping against next planes
                        for (int w = 0; w < nTrisToAdd; w++)
                            listTriangles.push_back(clipped[w]);
                    }
                    nNewTriangles = listTriangles.size();
                }

                // Draw the transformed, viewed, clipped, projected, sorted, clipped triangles
                for (auto &t : listTriangles) {
                    FillTriangle(
                        t.p[0].x, t.p[0].y, 
                        t.p[1].x, t.p[1].y, 
                        t.p[2].x, t.p[2].y, 
                        t.sym, t.col
                    );

                    DrawTriangle(
                        t.p[0].x, t.p[0].y, 
                        t.p[1].x, t.p[1].y, 
                        t.p[2].x, t.p[2].y, 
                        PIXEL_SOLID, FG_BLUE
                    );
                }
            }

            return true;
        }

};

int main() {

    Engine3D engine;

    if(engine.ConstructConsole(250, 200, 3, 3)) 
        engine.Start();
    else {
        cout << "Not able to start window!\n";
        exit(-1);
    }

    return 0;
}